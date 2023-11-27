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
#ifndef __THRESH_SERVER_CALL_DATA__
#define __THRESH_SERVER_CALL_DATA__


#include <grpc/support/log.h>
#include <grpcpp/grpcpp.h>

#ifdef BAZEL_BUILD
#include "thresh_net_aborts_grpc/protos/thresh_net.grpc.pb.h"
#else
#include "thresh_net.grpc.pb.h"
#endif


using grpc::ServerCompletionQueue;
using grpc::ServerContext;
using grpc::ServerAsyncResponseWriter;
using grpc::Status;

using thresh_net::ThreshServer;

enum CallStatus {
	CREATE,
	PROCESS,
	FINISH
};


// CallData is the base class for classes encompasing the state and logic needed to serve a request.
template <typename AsyncService>
class CallData {
public:
	CallData(AsyncService* service, ServerCompletionQueue* cq, CallStatus status)
		: service_(service), cq_(cq), status_(status) {}
	virtual ~CallData() = default;
	virtual void Proceed() = 0;

protected:
	AsyncService* service_;
	ServerCompletionQueue* cq_;
	// Context for the rpc, allowing to tweak aspects of it such as the use
	// of compression, authentication, as well as to send metadata back to the
	// client.
	ServerContext ctx_;

	CallStatus status_;  // The current serving state.
};
#endif // __THRESH_SERVER_CALL_DATA__
