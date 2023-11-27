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
#ifndef __THRESH_SERVER_ABORTS_PARTIAL_CIPHER_TEXT_TO_CLIENT_REQUEST__
#define __THRESH_SERVER_ABORTS_PARTIAL_CIPHER_TEXT_TO_CLIENT_REQUEST__

#include "thresh_server_call_data.h"
#include "utils.h"
#include "cryptocontext-ser.h"
#include "scheme/bfvrns/bfvrns-ser.h"
#include "utils/serial.h"
#include "utils/exception.h"
#include <sstream>


// all instances of this class created dynamically
class AbortsPartialCipherTextToClientsRequestServicer : public CallData {
public:
	AbortsPartialCipherTextToClientsRequestServicer(ThreshServer::AsyncService* service, ServerCompletionQueue* cq,
		std::unordered_map<pair, std::vector<lbcrypto::Ciphertext<lbcrypto::DCRTPoly>>, boost::hash<pair>>& AbortscTexts)
		: CallData(service, cq, CREATE), responder_(&ctx_), cipherTexts(AbortscTexts) {
		Proceed();
	}

	void Proceed() override {
		if (status_ == CREATE) {
			status_ = PROCESS;

			service_->RequestAbortsPartialCipherTextToClients(&ctx_, &request_, &responder_, cq_, cq_, this);
		}
		else if (status_ == PROCESS) {
			new AbortsPartialCipherTextToClientsRequestServicer(service_, cq_, cipherTexts);

            uint32_t received_client_id = request_.received_client_id();
            uint32_t recovered_client_id = request_.recovered_client_id();

            cipherText = cipherTexts[{received_client_id, recovered_client_id}];
			if (cipherText.size() == 0) {
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
	thresh_net::AbortsCipherTextRequest request_;
	thresh_net::AbortsCipherTextReturn reply_;

	ServerAsyncResponseWriter<thresh_net::AbortsCipherTextReturn> responder_;

    std::vector<lbcrypto::Ciphertext<lbcrypto::DCRTPoly>> cipherText;
	std::unordered_map<pair, std::vector<lbcrypto::Ciphertext<lbcrypto::DCRTPoly>>, boost::hash<pair>>& cipherTexts;
};

#endif // __THRESH_SERVER_ABORTS_PARTIAL_CIPHER_TEXT_TO_CLIENT_REQUEST__
