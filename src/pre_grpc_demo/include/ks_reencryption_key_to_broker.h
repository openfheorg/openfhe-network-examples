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

#ifndef __KS_REENCRYPTION_KEY_TO_BROKER_H__
#define __KS_REENCRYPTION_KEY_TO_BROKER_H__

#include <sstream>
#include <iostream>

#include "key/key-ser.h"

#include "pre_server_call_data.h"

// all instances of this class created dynamically
class ReencryptionKeyRequestServicer : public CallData<pre_net::PreNetServer::AsyncService> {
public:
	ReencryptionKeyRequestServicer(pre_net::PreNetServer::AsyncService* service, grpc::ServerCompletionQueue* cq,
	    const Params& params0,
		lbcrypto::CryptoContext<lbcrypto::DCRTPoly>& cryptoContext,
	    std::unordered_map<pair, lbcrypto::PrivateKey<lbcrypto::DCRTPoly>, boost::hash<pair>>& PrivateKeys,
		std::unordered_map<pair, lbcrypto::PublicKey<lbcrypto::DCRTPoly>, boost::hash<pair>>& PublicKeys,
		std::vector<std::string>& AccessMap)
		: CallData(service, cq, CREATE), params(params0), responder_(&ctx_), cc(cryptoContext),
		PrivKeys(PrivateKeys),
		PubKeys(PublicKeys),
		amap (AccessMap) {
		Proceed();
	}

	void Proceed() override {
		if (status_ == CREATE) {
			status_ = PROCESS;

			service_->RequestReEncryptionKeyToBroker(&ctx_, &request_, &responder_, cq_, cq_, this);
		}
		else if (status_ == PROCESS) {
			new ReencryptionKeyRequestServicer(service_, cq_, params, cc, PrivKeys, PubKeys, amap);

			std::string client_name = request_.client_name();
            std::string client_type = request_.client_type();
            std::string upstream_client_name = request_.upstream_client_name();
            std::string upstream_client_type = request_.upstream_client_type();

            auto receiverPublicKey = PubKeys[{client_type, client_name}]; 
            auto senderPrivateKey = PrivKeys[{upstream_client_type, upstream_client_name}];

            if (!cc || !receiverPublicKey) {
				reply_.set_keyavailable(false);
			}
            
			//serialize receiver public key
			std::ostringstream os;
			lbcrypto::Serial::Serialize(receiverPublicKey, os, GlobalSerializationType);
			if (!os.str().size())
				OPENFHE_THROW(lbcrypto::serialize_error, "receiver public key serialization error");

            lbcrypto::EvalKey<lbcrypto::DCRTPoly> reencryptionKey;
            if (!senderPrivateKey) {
				if(upstream_client_type == "producer") {
					std::cerr << "Producer " << upstream_client_name << "'s private key has not been registered with the key server" << std::endl;
					reply_.set_producerregistered(false);
				} else {
					// internal client instance to send public key of broker and 
					// request for reencryption key from upstream key server
					KeyServerInternalClient internalClient_pre_server(params);

					std::string reply_publickeyclient, reply_reencryptionkey;
					internalClient_pre_server.PublicKeyfromServer(os.str(), client_type, client_name);
					internalClient_pre_server.ReEncryptionKeyToServer(reply_reencryptionkey, client_type, client_name, upstream_client_name, upstream_client_type);
					
					//set reencryption key in reply
					reply_.set_keyavailable(true);
					reply_.set_rekey(reply_reencryptionkey);
				}
			}
			else {
				if(upstream_client_type == "producer") {
					reply_.set_producerregistered(true);
				}
		        std::ostringstream osr;
				if (std::find(amap.begin(), amap.end(), client_name) != amap.end()) {
					reencryptionKey = cc->ReKeyGen(senderPrivateKey, receiverPublicKey);
				}
				
				//std::ostringstream os;
				lbcrypto::Serial::Serialize(reencryptionKey, osr, GlobalSerializationType);

				if (!osr.str().size())
					OPENFHE_THROW(lbcrypto::serialize_error, "Re-encryption key serialization error");
				if (TEST_MODE)
					write2File(osr.str(), "./server_reencryptionKey_sent_serialized.txt"); // TODO: added for testing only

				reply_.set_keyavailable(true);
				reply_.set_rekey(osr.str());
			}
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
	pre_net::MessageRequest request_;
	pre_net::MessageReturn reply_;

	grpc::ServerAsyncResponseWriter<pre_net::MessageReturn> responder_;
	
	lbcrypto::CryptoContext<lbcrypto::DCRTPoly>& cc;
	
	std::unordered_map<pair, lbcrypto::PrivateKey<lbcrypto::DCRTPoly>, boost::hash<pair>>& PrivKeys;
	std::unordered_map<pair, lbcrypto::PublicKey<lbcrypto::DCRTPoly>, boost::hash<pair>>& PubKeys;
    std::vector<std::string>& amap;
};


#endif // __KS_REENCRYPTION_KEY_TO_BROKER_H__

