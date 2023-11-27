// @file another_server_test_request.h
// @author TPOC: info@DualityTech.com
//
// @copyright Copyright (c) 2021, Duality Technologies Inc.
// All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution. THIS SOFTWARE IS
// PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
// EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef __CONTROLLER_CIPHER_TEXT_FROM_CLIENT_REQUEST_H__
#define __CONTROLLER_CIPHER_TEXT_FROM_CLIENT_REQUEST_H__



#include "utils.h"
#include "thresh_server_call_data.h"
#include "utils/exception.h"
#include <iostream>
#include "cryptocontext-ser.h"
#include "key/key.h"
#include "key/key-ser.h"
#include "scheme/bfvrns/bfvrns-ser.h"
#include "utils/serial.h"
#include "thresh_controller_internal_client.h"

// all instances of this class created dynamically
class ControllerCipherTextRequestServicer : public CallData<thresh_net::ThreshController::AsyncService> {
public:
	ControllerCipherTextRequestServicer(thresh_net::ThreshController::AsyncService* service, grpc::ServerCompletionQueue* cq, const Params& params0,
	lbcrypto::CryptoContext<lbcrypto::DCRTPoly>& cc,
	std::unordered_map<uint32_t, lbcrypto::Plaintext>& random_s)
		: CallData(service, cq, CREATE), params(params0), responder_(&ctx_), controllercc(cc), rs(random_s) {
		Proceed();
	}

	void Proceed() override {
		if (status_ == CREATE) {
			status_ = PROCESS;

			service_->RequestCipherTextFromClientToController(&ctx_, &request_, &responder_, cq_, cq_, this);
		}
		else if (status_ == PROCESS) {
			new ControllerCipherTextRequestServicer(service_, cq_, params, controllercc, rs);
   
            uint32_t measure_id = request_.measure_id();

            //receive ciphertext from the rpc
			std::istringstream is(request_.ctext());
			if (!is.str().size())
				OPENFHE_THROW(lbcrypto::openfhe_error, "Received empty cipher text buffer");

            lbcrypto::Ciphertext<lbcrypto::DCRTPoly> ct;
			lbcrypto::Serial::Deserialize(ct, is, GlobalSerializationType);
            if (!ct)
				OPENFHE_THROW(lbcrypto::deserialize_error, "De-serialized ciphertext is NULL");
			if (TEST_MODE)
				write2File(ct, "./client_ciphertext_received_serialized.txt");

			//instance of internal client to request for cryptocontext, joint public key, final evalmult key from the key server
            ThreshControllerInternalClient internalClient_controller(params);

			std::string reply_publickeyclient, reply_evalmultkey;

			internalClient_controller.JointPublicKeyToController(reply_publickeyclient, params);
            internalClient_controller.EvalMultKeyToController(reply_evalmultkey);

            //receive and deserialize final joint public key from key server
            if (!reply_publickeyclient.size()){
		        OPENFHE_THROW(lbcrypto::openfhe_error, "Received empty serialized public key");
			}
				
			if (TEST_MODE){
				write2File(reply_publickeyclient, "./Final_public_key_received_serialized.txt");
			}
			
			lbcrypto::PublicKey<lbcrypto::DCRTPoly> FinalPubKey(nullptr);
			std::istringstream isFinalPubKey(reply_publickeyclient);
			lbcrypto::Serial::Deserialize(FinalPubKey, isFinalPubKey, GlobalSerializationType);

            //receive and deserialize final joint evalmult key from key server
			if (!reply_evalmultkey.size()) {
			   	OPENFHE_THROW(lbcrypto::openfhe_error, "Received empty evalmult key");
			}
				
			lbcrypto::EvalKey<lbcrypto::DCRTPoly> EvalMultKey(nullptr);
			std::istringstream isevalmultkey(reply_evalmultkey);
			lbcrypto::Serial::Deserialize(EvalMultKey, isevalmultkey, GlobalSerializationType);
			
            
            //generate a random vector s to mask the ciphertext received
            lbcrypto::Plaintext plainrandom_s;
            if (!rs[measure_id]) {
                unsigned int plaintextModulus = controllercc->GetCryptoParameters()->GetPlaintextModulus();
                std::vector<int64_t> random_svec;
		        random_svec.reserve(12);//ringsize);
		        for (size_t i = 0; i < 12; i++) {//ringsize; i++) { //generate a random array of shorts
			        random_svec.emplace_back(std::rand() % plaintextModulus);
		        }

		        //pack them into a packed plaintext (vector encryption)
		        plainrandom_s = controllercc->MakePackedPlaintext(random_svec);
				rs[measure_id] = plainrandom_s;
			} else {
                plainrandom_s = rs[measure_id];
			}
		    auto encryptedrandom_s = controllercc->Encrypt(FinalPubKey, plainrandom_s); // Encrypt

			//insert final evalmult key received from key server to cryptocontext and do ct*E(s)
			controllercc->InsertEvalMultKey({EvalMultKey});

            auto maskedCText = controllercc->EvalMult(encryptedrandom_s, ct);

            //serialize the masked ciphertext and send to client
            std::ostringstream os;
			lbcrypto::Serial::Serialize(maskedCText, os, GlobalSerializationType);
			if (!os.str().size())
				OPENFHE_THROW(lbcrypto::serialize_error, "CipherText serialization error");
			if (TEST_MODE)
				write2File(os.str(), "./server_ciphertext_sent_serialized.txt");

			reply_.set_textavailable(true);
			reply_.set_maskedctext(os.str());

			status_ = FINISH;
			responder_.Finish(reply_, grpc::Status::OK, this);
		}
		else {
			GPR_ASSERT(status_ == FINISH);
			// Once in the FINISH state, deallocate ourselves (CallData).
			delete this;
		}
	}

private:
    Params params;
	thresh_net::ControllerCipherText request_;
	thresh_net::ControllerCipherTextReturn reply_;

	grpc::ServerAsyncResponseWriter<thresh_net::ControllerCipherTextReturn> responder_;

	lbcrypto::CryptoContext<lbcrypto::DCRTPoly>& controllercc;
	std::unordered_map<uint32_t, lbcrypto::Plaintext>& rs;
};

#endif // __CONTROLLER_CIPHER_TEXT_FROM_CLIENT_REQUEST_H__
