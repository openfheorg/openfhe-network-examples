// @file pre_server_request_from_another_server.h
// @author TPOC: info@DualityTech.com
//
// @copyright Copyright (c) 2021, Duality Technologies Inc.
// All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution. THIS SOFTWARE IS
// PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
// EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef __THRESH_SERVER_CRYPTOCONTEXT_REQUEST_FROM_CONTROLLER_SERVER_H__
#define __THRESH_SERVER_CRYPTOCONTEXT_REQUEST_FROM_CONTROLLER_SERVER_H__

#include "thresh_server_call_data.h"
#include <sstream>
#include <iostream>


// all instances of this class created dynamically
class CryptoContextToControllerRequestServicer : public CallData<thresh_net::ThreshServer::AsyncService> {
public:
	CryptoContextToControllerRequestServicer(thresh_net::ThreshServer::AsyncService* service, grpc::ServerCompletionQueue* cq,
	    lbcrypto::CryptoContext<lbcrypto::DCRTPoly>& cryptoContext)
		: CallData(service, cq, CREATE), responder_(&ctx_), cc(cryptoContext) {
		Proceed();
	}

	void Proceed() override {
		if (status_ == CREATE) {
			status_ = PROCESS;

			service_->RequestCryptoContextRequestFromServer_Controller(&ctx_, &request_, &responder_, cq_, cq_, this);
		}
		else if (status_ == PROCESS) {
			new CryptoContextToControllerRequestServicer(service_, cq_, cc);

			std::ostringstream os;
			lbcrypto::Serial::Serialize(cc, os, GlobalSerializationType);
			if (!os.str().size())
				OPENFHE_THROW(lbcrypto::serialize_error, "CryptoContext serialization error");
			if (TEST_MODE)
				write2File(os.str(), "./server_cc_sent_serialized.txt"); // TODO: added for testing only

			reply_.set_cryptocontext(os.str());

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
	thresh_net::CryptoContextRequest request_;
	thresh_net::CryptoContextReturn reply_;

	grpc::ServerAsyncResponseWriter<thresh_net::CryptoContextReturn> responder_;
	lbcrypto::CryptoContext<lbcrypto::DCRTPoly>& cc;
};


#endif // __THRESH_SERVER_CRYPTOCONTEXT_REQUEST_FROM_CONTROLLER_H__

