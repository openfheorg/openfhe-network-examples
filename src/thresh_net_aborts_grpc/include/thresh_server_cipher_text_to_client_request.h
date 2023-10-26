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
#ifndef __THRESH_SERVER_CIPHER_TEXT_TO_CLIENT_REQUEST__
#define __THRESH_SERVER_CIPHER_TEXT_TO_CLIENT_REQUEST__

#include "thresh_server_call_data.h"
#include "utils.h"
#include "cryptocontext-ser.h"
#include "scheme/bfvrns/bfvrns-ser.h"
#include "utils/serial.h"
#include "utils/exception.h"
#include <sstream>


// all instances of this class created dynamically
class CipherTextToClientRequestServicer : public CallData {
public:
	CipherTextToClientRequestServicer(ThreshServer::AsyncService* service, ServerCompletionQueue* cq,
		std::unordered_map<uint32_t, lbcrypto::Ciphertext<lbcrypto::DCRTPoly>>& cTexts)
		: CallData(service, cq, CREATE), responder_(&ctx_), cipherTexts(cTexts) {
		Proceed();
	}

	void Proceed() override {
		if (status_ == CREATE) {
			status_ = PROCESS;

			service_->RequestCipherTextToClient(&ctx_, &request_, &responder_, cq_, cq_, this);
		}
		else if (status_ == PROCESS) {
			new CipherTextToClientRequestServicer(service_, cq_, cipherTexts);

            uint32_t client_id = request_.client_id();
			
            cipherText = cipherTexts[client_id];
			if (!cipherText) {
				reply_.set_textavailable(false);
			}
			else {
				std::ostringstream os;
				lbcrypto::Serial::Serialize(cipherText, os, GlobalSerializationType);
				if (!os.str().size())
					OPENFHE_THROW(lbcrypto::serialize_error, "CipherText serialization error");
				if (TEST_MODE)
					write2File(os.str(), "./server_ciphertext_sent_serialized.txt");

				reply_.set_textavailable(true);
				reply_.set_ciphertext(os.str());
			}
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
	thresh_net::CipherTextRequest request_;
	thresh_net::CipherTextReturn reply_;

	ServerAsyncResponseWriter<thresh_net::CipherTextReturn> responder_;

    lbcrypto::Ciphertext<lbcrypto::DCRTPoly> cipherText;
	std::unordered_map<uint32_t, lbcrypto::Ciphertext<lbcrypto::DCRTPoly>>& cipherTexts;
};

#endif // __THRESH_SERVER_CIPHER_TEXT_TO_CLIENT_REQUEST__
