/***
 * � 2021 Duality Technologies, Inc. All rights reserved.
 * This is a proprietary software product of Duality Technologies, Inc.
 *protected under copyright laws and international copyright treaties, patent
 *law, trade secret law and other intellectual property rights of general
 *applicability. Any use of this software is strictly prohibited absent a
 *written agreement executed by Duality Technologies, Inc., which provides
 *certain limited rights to use this software. You may not copy, distribute,
 *make publicly available, publicly perform, disassemble, de-compile or reverse
 *engineer any part of this software, breach its security, or circumvent,
 *manipulate, impair or disrupt its operation.
 ***/
#ifndef __THRESH_SERVER_PUBLIC_KEY_TO_CLIENT_REQUEST__
#define __THRESH_SERVER_PUBLIC_KEY_TO_CLIENT_REQUEST__

#include "thresh_server_call_data.h"
#include "utils.h"
#include "cryptocontext-ser.h"
#include "key/key.h"
#include "scheme/bfvrns/bfvrns-ser.h"
#include "utils/serial.h"
#include "utils/exception.h"
#include <sstream>

// all instances of this class created dynamically
class PublicKeyToClientRequestServicer : public CallData {
public:
	PublicKeyToClientRequestServicer(ThreshServer::AsyncService* service, ServerCompletionQueue* cq,
		std::unordered_map<uint32_t, std::string>& PublicKeys, std::unordered_map<uint32_t, bool>& PublicKeysReady)
		: CallData(service, cq, CREATE), responder_(&ctx_), pkeys(PublicKeys), pkeysready(PublicKeysReady) {
		Proceed();
	}

	void Proceed() override {
		if (status_ == CREATE) {
			status_ = PROCESS;

			service_->RequestPublicKeyToClient(&ctx_, &request_, &responder_, cq_, cq_, this);
		}
		else if (status_ == PROCESS) {
			new PublicKeyToClientRequestServicer(service_, cq_, pkeys, pkeysready);

			//send the correct public key from map pkeys by using client id
			uint32_t client_id = request_.client_id();

            pkeyready = pkeysready[client_id];
			if (!pkeyready) {
				reply_.set_keyavailable(false);
			}
			else {
				pkey = pkeys[client_id];

				reply_.set_keyavailable(true);
				reply_.set_pkey(pkey);
			}

			if (TEST_MODE)
				write2File(reply_.pkey(), "./server_pubkey_to_client_" + std::to_string(client_id) + "_serialized.txt");

			status_ = FINISH;
			responder_.Finish(reply_, Status::OK, this);
		}
		else {
			GPR_ASSERT(status_ == FINISH);
			// Once in the FINISH state, deallocate ourselves (CallData).
			delete this;
		}
	}

private:
	thresh_net::PublicKeyRequest request_;
	thresh_net::PublicKeyReturn reply_;

	ServerAsyncResponseWriter<thresh_net::PublicKeyReturn> responder_;
	std::string pkey;
	bool pkeyready;
	
	std::unordered_map<uint32_t, std::string>& pkeys;
	std::unordered_map<uint32_t, bool>& pkeysready;
};

#endif // __THRESH_SERVER_PUBLIC_KEY_TO_CLIENT_REQUEST__
