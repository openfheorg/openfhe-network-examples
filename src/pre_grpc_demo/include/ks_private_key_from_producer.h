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
#ifndef __KS_PRIVATE_KEY_FROM_PRODUCER_H__
#define __KS_PRIVATE_KEY_FROM_PRODUCER_H__

#include <sstream>
#include <iostream>

#include "key/key.h"
#include "utils/serial.h"
#include "utils/exception.h"

#include "utils.h"
#include "pre_server_call_data.h"

// all instances of this class created dynamically
class PrivateKeyRequestServicer : public CallData<pre_net::PreNetServer::AsyncService> {
public:
	PrivateKeyRequestServicer(pre_net::PreNetServer::AsyncService* service, grpc::ServerCompletionQueue* cq,
		std::unordered_map<pair, lbcrypto::PrivateKey<lbcrypto::DCRTPoly>, boost::hash<pair>>& PrivateKeys)
		: CallData(service, cq, CREATE), responder_(&ctx_), PrivKeys(PrivateKeys) {
		Proceed();
	}

	void Proceed() override {
		if (status_ == CREATE) {
			status_ = PROCESS;

			service_->RequestPrivateKeyFromProducer(&ctx_, &request_, &responder_, cq_, cq_, this);
		}
		else if (status_ == PROCESS) {
			new PrivateKeyRequestServicer(service_, cq_, PrivKeys);

            std::string client_name = request_.client_name();

			if (TEST_MODE)
				write2File(request_.pkey(), "./server_private_key_received_serialized.txt"); // TODO: added for testing only
			std::istringstream is(request_.pkey());
			if (!is.str().size())
				OPENFHE_THROW(lbcrypto::openfhe_error, "Received empty private key buffer");
			lbcrypto::Serial::Deserialize(pkey, is, GlobalSerializationType);
			if (!pkey)
				OPENFHE_THROW(lbcrypto::deserialize_error, "Private key de-serialization error");

            PrivKeys[{"producer", client_name}] = pkey;

			reply_.set_acknowledgement("Received PKEY. Thank you, Your Server");
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
	lbcrypto::PrivateKey<lbcrypto::DCRTPoly> pkey;
	std::unordered_map<pair, lbcrypto::PrivateKey<lbcrypto::DCRTPoly>, boost::hash<pair>>& PrivKeys;
};

#endif // __KS_PRIVATE_KEY_FROM_PRODUCER_H__
