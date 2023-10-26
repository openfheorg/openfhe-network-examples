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
#ifndef __THRESH_SERVER_EVAL_MULT_KEY_TO_CONTROLLER_REQUEST__
#define __THRESH_SERVER_EVAL_MULT_KEY_TO_CONTROLLER_REQUEST__

#include "thresh_server_call_data.h"
#include "utils.h"
#include "key/key-ser.h"
#include "cryptocontext-ser.h"
#include "scheme/bfvrns/bfvrns-ser.h"
#include "utils/serial.h"
#include "utils/exception.h"
#include <sstream>

// all instances of this class created dynamically
class EvalMultKeyToControllerRequestServicer : public CallData<thresh_net::ThreshServer::AsyncService> {
public:
	EvalMultKeyToControllerRequestServicer(ThreshServer::AsyncService* service, ServerCompletionQueue* cq,
	    std::unordered_map<uint32_t, std::string>& evalKeys, std::unordered_map<uint32_t, bool>& evalKeysReady)
		: CallData(service, cq, CREATE), responder_(&ctx_), ekeys(evalKeys), ekeysready(evalKeysReady) {
		Proceed();
	}

	void Proceed() override {
		if (status_ == CREATE) {
			status_ = PROCESS;

			service_->RequestEvalMultKeyToController(&ctx_, &request_, &responder_, cq_, cq_, this);
		}
		else if (status_ == PROCESS) {
			// Spawn a new EvalMixMultKeyToClientRequestServicer instance to serve new clients while we process
			// the one for this EvalMixMultKeyToClientRequestServicer. The instance will deallocate itself as
			// part of its FINISH state.
			// TODO (dsuponit): do we really need this "new" instance in every Proceed()??? I am still not sure
			new EvalMultKeyToControllerRequestServicer(service_, cq_, ekeys, ekeysready);

			ekeyready = ekeysready[1];

			if (!ekeyready) {
				reply_.set_keyavailable(false);
			}
			else {
				ekey = ekeys[1];

				reply_.set_keyavailable(true);
				reply_.set_ekey(ekey);//os.str());
			}

			if (TEST_MODE)
				write2File(reply_.ekey(), "./server_evalmixmultkey_to_client_serialized.txt");

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
	thresh_net::EvalMultKeyRequest request_;
	thresh_net::EvalMultKeyReturn reply_;

	ServerAsyncResponseWriter<thresh_net::EvalMultKeyReturn> responder_;
	std::string ekey;
	bool ekeyready;
	
	std::unordered_map<uint32_t, std::string>& ekeys;
	std::unordered_map<uint32_t, bool>& ekeysready;
};

#endif // __THRESH_SERVER_EVAL_MULT_KEY_TO_CONTROLLER_REQUEST__
