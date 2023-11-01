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
#ifndef __THRESH_SERVER_PARTIAL_CIPHER_TEXT_REQUEST__
#define __THRESH_SERVER_PARTIAL_CIPHER_TEXT_REQUEST__

#include "utils.h"
#include "thresh_server_call_data.h"
#include "cryptocontext-ser.h"
#include "scheme/bfvrns/bfvrns-ser.h"
#include "utils/serial.h"
#include "utils/exception.h"
#include <iostream>
#include <memory>
#include <string>



// ============================================================================================
// all instances of this class created dynamically
class PartialCipherTextRequestServicer : public CallData {
public:
	PartialCipherTextRequestServicer(ThreshServer::AsyncService* service, ServerCompletionQueue* cq,
		std::unordered_map<uint32_t, std::vector<lbcrypto::Ciphertext<lbcrypto::DCRTPoly>>>& CipherTexts, std::unordered_map<uint32_t, bool>& CTRecdFlag)
		: CallData(service, cq, CREATE), responder_(&ctx_), cts(CipherTexts), CTRecd (CTRecdFlag) {
		Proceed();
	}

	void Proceed() override {
		if (status_ == CREATE) {
			status_ = PROCESS;

			service_->RequestPartialCipherTextFromClient(&ctx_, &request_, &responder_, cq_, cq_, this);
		}
		else if (status_ == PROCESS) {
			new PartialCipherTextRequestServicer(service_, cq_, cts, CTRecd);

			if (TEST_MODE)
				write2File(request_.ctext(), "./server_ciphertext_received_serialized.txt");
			std::istringstream is(request_.ctext());
			if (!is.str().size())
				OPENFHE_THROW(lbcrypto::openfhe_error, "Received empty cipher text buffer");

            uint32_t client_id = request_.client_id();
			lbcrypto::Serial::Deserialize(ct, is, GlobalSerializationType);

            cts[client_id] = ct;
			CTRecd[client_id] = true;

			reply_.set_acknowledgement("Received partial CTEXT. Thank you, Your Server");
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
	thresh_net::CipherText request_;
	thresh_net::CipherTextAck reply_;

	ServerAsyncResponseWriter<thresh_net::CipherTextAck> responder_;
	std::vector<lbcrypto::Ciphertext<lbcrypto::DCRTPoly>> ct;
	
    std::unordered_map<uint32_t, std::vector<lbcrypto::Ciphertext<lbcrypto::DCRTPoly>>>& cts;
	std::unordered_map<uint32_t, bool>& CTRecd;
};

#endif // __THRESH_SERVER_PARTIAL_CIPHER_TEXT_REQUEST__
