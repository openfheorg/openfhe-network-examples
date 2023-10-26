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
#ifndef __THRESH_SERVER_EVAL_KEY_REQUEST__
#define __THRESH_SERVER_EVAL_KEY_REQUEST__

#include "utils.h"
#include "thresh_server_call_data.h"
#include "cryptocontext-ser.h"
#include "key/key.h"
#include "key/key-ser.h"
#include "scheme/bfvrns/bfvrns-ser.h"
#include "utils/serial.h"
#include "utils/exception.h"
#include <sstream>
#include <iostream>


// all instances of this class created dynamically
class EvalMultKeyRequestServicer : public CallData {
public:
	EvalMultKeyRequestServicer(ThreshServer::AsyncService* service, ServerCompletionQueue* cq,
	    std::unordered_map<uint32_t, std::string>& evalKeyMults, std::unordered_map<uint32_t, bool>& evalKeyMultsReady)
		: CallData(service, cq, CREATE), responder_(&ctx_), ekeys(evalKeyMults), ekeysready(evalKeyMultsReady) {
		Proceed();
	}

	void Proceed() override {
		if (status_ == CREATE) {
			status_ = PROCESS;
			service_->RequestevalMultKeyFromClient(&ctx_, &request_, &responder_, cq_, cq_, this);
		}
		else if (status_ == PROCESS) {
			new EvalMultKeyRequestServicer(service_, cq_, ekeys, ekeysready);

			uint32_t client_id = request_.client_id();

			if (TEST_MODE)
				write2File(request_.ekey(), "./server_public_key_received_serialized.txt"); // TODO: added for testing only
			std::istringstream is(request_.ekey());
			
			if (!is.str().size())
				OPENFHE_THROW(lbcrypto::openfhe_error, "Received empty public key buffer");


			reply_.set_acknowledgement("Received EKEY. Thank you, Your Server");

            ekeys[client_id] = request_.ekey();
            ekeysready[client_id] = true;

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
	thresh_net::EvalMultKey request_;
	thresh_net::EvalMultKeyAck reply_;

	ServerAsyncResponseWriter<thresh_net::EvalMultKeyAck> responder_;

	std::unordered_map<uint32_t, std::string>& ekeys;
	std::unordered_map<uint32_t, bool>& ekeysready;
};

#endif // __THRESH_SERVER_EVAL_KEY_REQUEST__
