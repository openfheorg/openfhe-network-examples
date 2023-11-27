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
#ifndef __THRESH_SERVER_SECRET_SHARES_REQUEST__
#define __THRESH_SERVER_SECRET_SHARES_REQUEST__

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
class SecretSharesRequestServicer : public CallData {
public:
	SecretSharesRequestServicer(ThreshServer::AsyncService* service, ServerCompletionQueue* cq,
		std::unordered_map<uint32_t, std::unordered_map<uint32_t, lbcrypto::DCRTPoly>>& SecretShares)
		: CallData(service, cq, CREATE), responder_(&ctx_), ssm(SecretShares) {
		Proceed();
	}

	void Proceed() override {
		if (status_ == CREATE) {
			status_ = PROCESS;

			service_->RequestSecretSharesFromClient(&ctx_, &request_, &responder_, cq_, cq_, this);
		}
		else if (status_ == PROCESS) {
			new SecretSharesRequestServicer(service_, cq_, ssm);

			if (TEST_MODE)
				write2File(request_.secretshares(), "./server_secret_shares_received_serialized.txt");
			std::istringstream is(request_.secretshares());
			if (!is.str().size())
				OPENFHE_THROW(lbcrypto::openfhe_error, "Received empty secret shares vector buffer");

            uint32_t client_id = request_.client_id();
			lbcrypto::Serial::Deserialize(ss, is, GlobalSerializationType);
			if (ss.size() == 0)
				OPENFHE_THROW(lbcrypto::deserialize_error, "Secret shares vector de-serialization error");

            ssm[client_id] = ss;

			reply_.set_acknowledgement("Received SecretShares. Thank you, Your Server");
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
	thresh_net::SecretKeyShares request_;
	thresh_net::SecretKeySharesAck reply_;

	ServerAsyncResponseWriter<thresh_net::SecretKeySharesAck> responder_;
	std::unordered_map<uint32_t, lbcrypto::DCRTPoly> ss;
    std::unordered_map<uint32_t, std::unordered_map<uint32_t, lbcrypto::DCRTPoly>>& ssm;
	
};

#endif // __THRESH_SERVER_SECRET_SHARES_REQUEST__
