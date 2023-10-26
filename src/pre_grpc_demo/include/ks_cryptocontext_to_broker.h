// @file pre_server_request_from_another_server.h
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

#ifndef __KS_CRYPTOCONTEXT_TO_BROKER_H__
#define __KS_CRYPTOCONTEXT_TO_BROKER_H__

#include <sstream>
#include <iostream>

#include "scheme/bgvrns/cryptocontext-bgvrns.h"
#include "cryptocontext-ser.h"
#include "scheme/bgvrns/bgvrns-ser.h"
#include "gen-cryptocontext.h"

#include "pre_server_call_data.h"


// all instances of this class created dynamically
class CryptoContextRequest_BrokerServicer : public CallData<pre_net::PreNetServer::AsyncService> {
public:
	CryptoContextRequest_BrokerServicer(pre_net::PreNetServer::AsyncService* service, grpc::ServerCompletionQueue* cq,
	    lbcrypto::CryptoContext<lbcrypto::DCRTPoly>& cryptoContext,
		std::unordered_map<pair, lbcrypto::PrivateKey<lbcrypto::DCRTPoly>, boost::hash<pair>>& PrivKeys,
		std::unordered_map<pair, lbcrypto::PublicKey<lbcrypto::DCRTPoly>, boost::hash<pair>>& PubKeys)
		: CallData(service, cq, CREATE), responder_(&ctx_), cc(cryptoContext), prkeys(PrivKeys), pkeys(PubKeys) {
        
		std::ostringstream osc1;
			lbcrypto::Serial::Serialize(cryptoContext, osc1, GlobalSerializationType);
			if (!osc1.str().size())
				OPENFHE_THROW(lbcrypto::serialize_error, "CryptoContext serialization error");
			if (TEST_MODE)
				write2File(osc1.str(), "./server_cryptoContext_sent_serialized.txt");
		
		std::ostringstream osc;
			lbcrypto::Serial::Serialize(cc, osc, GlobalSerializationType);
			if (!osc.str().size())
				OPENFHE_THROW(lbcrypto::serialize_error, "CryptoContext serialization error");
			if (TEST_MODE)
				write2File(osc.str(), "./server_cc_sent_serialized_c.txt");


		Proceed();
	}

	void Proceed() override {
		if (status_ == CREATE) {
			status_ = PROCESS;

			service_->RequestCryptoContextRequestFromServer_Broker(&ctx_, &request_, &responder_, cq_, cq_, this);
		}
		else if (status_ == PROCESS) {
			new CryptoContextRequest_BrokerServicer(service_, cq_, cc, prkeys, pkeys);

			std::ostringstream os;
			lbcrypto::Serial::Serialize(cc, os, GlobalSerializationType);
			if (!os.str().size())
				OPENFHE_THROW(lbcrypto::serialize_error, "CryptoContext serialization error");
			if (TEST_MODE)
				write2File(os.str(), "./server_cc_sent_serialized.txt"); // TODO: added for testing only

			reply_.set_cryptocontext(os.str());

			std::string client_name = request_.client_name();

			std::cout << "Server keygen for broker" << std::endl;
			lbcrypto::KeyPair<lbcrypto::DCRTPoly> keyPair;
			keyPair = cc->KeyGen();
			
			pkeys[{"broker", client_name}] = keyPair.publicKey;
			prkeys[{"broker", client_name}] = keyPair.secretKey;

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
	pre_net::MessageRequest request_;
	pre_net::MessageReturn reply_;

	grpc::ServerAsyncResponseWriter<pre_net::MessageReturn> responder_;
	lbcrypto::CryptoContext<lbcrypto::DCRTPoly>& cc;

    std::unordered_map<pair, lbcrypto::PrivateKey<lbcrypto::DCRTPoly>, boost::hash<pair>>& prkeys;
	std::unordered_map<pair, lbcrypto::PublicKey<lbcrypto::DCRTPoly>, boost::hash<pair>>& pkeys;
};


#endif // __KS_CRYPTOCONTEXT_TO_BROKER_H__

