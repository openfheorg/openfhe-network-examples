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

#ifndef __KS_BROKER_PUBLIC_KEY_TO_BROKER_H__
#define __KS_BROKER_PUBLIC_KEY_TO_BROKER_H__

#include <sstream>
#include <iostream>

#include "pre_server_call_data.h"
#include "ks_ks_internal_client.h"

// all instances of this class created dynamically
class PublicKeyToBrokerRequestServicer : public CallData<pre_net::PreNetServer::AsyncService> {
public:
	PublicKeyToBrokerRequestServicer(pre_net::PreNetServer::AsyncService* service, grpc::ServerCompletionQueue* cq,
	    const Params& params0,
	    std::unordered_map<pair, lbcrypto::PublicKey<lbcrypto::DCRTPoly>, boost::hash<pair>>& PublicKeys)
		: CallData(service, cq, CREATE), params(params0), responder_(&ctx_), pkeys(PublicKeys) {
		Proceed();
	}

	void Proceed() override {
		if (status_ == CREATE) {
			status_ = PROCESS;

			service_->RequestPublicKeyToBroker(&ctx_, &request_, &responder_, cq_, cq_, this);
		}
		else if (status_ == PROCESS) {
			new PublicKeyToBrokerRequestServicer(service_, cq_, params, pkeys);

			std::string client_type = request_.client_type();
			std::string client_name = request_.client_name();

			pkey = pkeys[{client_type, client_name}];
			std::string reply_publickeyclient;
			if (!pkey) {
				if (client_type == "producer") {
					std::cerr << "Producer " << client_name << "'s public key has not been registered with the key server" << std::endl;
					reply_.set_producerregistered(false);
				} else {
					//internal client to check with upstream key server
					KeyServerInternalClient internalClient_pre_server(params);
					
					internalClient_pre_server.PublicKeyToServer(reply_publickeyclient, client_type, client_name);
					
					if(!reply_publickeyclient.size()) {
					reply_.set_keyavailable(false);
					}
					else {
						reply_.set_keyavailable(true);
						reply_.set_pkey(reply_publickeyclient);
					}
				}
			}
			else {
				if (client_type == "producer") {
					reply_.set_producerregistered(true);
				}
				std::ostringstream os;
				lbcrypto::Serial::Serialize(pkey, os, GlobalSerializationType);
				if (!os.str().size())
					OPENFHE_THROW(lbcrypto::serialize_error, "Producer public key serialization error");
					
				reply_.set_keyavailable(true);
				reply_.set_pkey(os.str());
			}

			if (TEST_MODE)
				write2File(reply_.pkey(), "./server_pubkey_to_consumer_serialized.txt");

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
	lbcrypto::PublicKey<lbcrypto::DCRTPoly> pkey;
	
	std::unordered_map<pair, lbcrypto::PublicKey<lbcrypto::DCRTPoly>, boost::hash<pair>>& pkeys;
};


#endif // __KS_BROKER_PUBLIC_KEY_TO_BROKER_H__

