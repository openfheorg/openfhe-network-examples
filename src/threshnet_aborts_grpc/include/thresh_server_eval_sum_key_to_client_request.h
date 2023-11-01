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
#ifndef __THRESH_SERVER_EVAL_SUM_KEY_TO_CLIENT_REQUEST__
#define __THRESH_SERVER_EVAL_SUM_KEY_TO_CLIENT_REQUEST__

#include "thresh_server_call_data.h"
#include "utils.h"
#include "cryptocontext-ser.h"
#include "scheme/bfvrns/bfvrns-ser.h"
#include "key/key.h"
#include "key/key-ser.h"
#include "utils/serial.h"
#include "utils/exception.h"
#include <sstream>

// all instances of this class created dynamically
class EvalSumKeyToClientRequestServicer : public CallData {
public:
	EvalSumKeyToClientRequestServicer(ThreshServer::AsyncService* service, ServerCompletionQueue* cq,
	    std::unordered_map<pair, std::string, boost::hash<pair>>& evalKeys, std::unordered_map<pair, bool, boost::hash<pair>>& evalKeysReady)
		: CallData(service, cq, CREATE), responder_(&ctx_), ekeys(evalKeys), ekeysready(evalKeysReady) {
		Proceed();
	}

	void Proceed() override {
		if (status_ == CREATE) {
			status_ = PROCESS;

			service_->RequestevalSumKeyToClient(&ctx_, &request_, &responder_, cq_, cq_, this);
		}
		else if (status_ == PROCESS) {
			new EvalSumKeyToClientRequestServicer(service_, cq_, ekeys, ekeysready);

			//send the correct eval mult key from map ekeys by using client id
			uint32_t client_id = request_.client_id();
            uint32_t index_num = request_.index();
            
			ekeyready = ekeysready[{client_id, index_num}];

			if (!ekeyready) {
				reply_.set_keyavailable(false);
			}
			else {
				ekey = ekeys[{client_id, index_num}];
				
				reply_.set_keyavailable(true);
				reply_.set_ekey(ekey);
			}

			if (TEST_MODE)
				write2File(reply_.ekey(), "./server_evalsumkey_to_client_serialized.txt");

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
	thresh_net::EvalSumKeyRequest request_;
	thresh_net::EvalSumKeyReturn reply_;

	ServerAsyncResponseWriter<thresh_net::EvalSumKeyReturn> responder_;
	std::string ekey;
	bool ekeyready;
	
	std::unordered_map<pair, std::string, boost::hash<pair>>& ekeys;
	std::unordered_map<pair, bool, boost::hash<pair>>& ekeysready;
};

#endif // __THRESH_SERVER_EVAL_SUM_KEY_TO_CLIENT_REQUEST__
