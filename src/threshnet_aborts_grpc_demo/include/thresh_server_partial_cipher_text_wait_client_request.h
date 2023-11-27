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
#ifndef __THRESH_SERVER_PARTIAL_CIPHER_TEXT_WAIT_CLIENT_REQUEST__
#define __THRESH_SERVER_PARTIAL_CIPHER_TEXT_WAIT_CLIENT_REQUEST__

#include "thresh_server_call_data.h"
#include "utils.h"
#include "cryptocontext-ser.h"
#include "scheme/bfvrns/bfvrns-ser.h"
#include "utils/serial.h"
#include "utils/exception.h"
#include <sstream>


// all instances of this class created dynamically
class PartialCipherTextWaitClientRequestServicer : public CallData {
public:
	PartialCipherTextWaitClientRequestServicer(ThreshServer::AsyncService* service, ServerCompletionQueue* cq,
		std::unordered_map<uint32_t, bool>& CTRecdFlag)
		: CallData(service, cq, CREATE), responder_(&ctx_), CTRecd (CTRecdFlag) {
		Proceed();
	}

	void Proceed() override {
		if (status_ == CREATE) {
			status_ = PROCESS;

			service_->RequestPartialCipherTextsReceivedWait(&ctx_, &request_, &responder_, cq_, cq_, this);
		}
		else if (status_ == PROCESS) {
			new PartialCipherTextWaitClientRequestServicer(service_, cq_, CTRecd);

            uint32_t client_id = request_.client_id();
			CTRecdval = CTRecd[client_id];
			reply_.set_textavailable(CTRecdval);

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
	thresh_net::PartialCTFlagRequest request_;
	thresh_net::PartialCTFlagReturn reply_;

	ServerAsyncResponseWriter<thresh_net::PartialCTFlagReturn> responder_;

    bool CTRecdval;
	std::unordered_map<uint32_t, bool>& CTRecd;
};

#endif // __THRESH_SERVER_PARTIAL_CIPHER_TEXT_WAIT_CLIENT_REQUEST__
