/***
 * Copyright (c) 2021Duality Technologies, Inc. All rights reserved.
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
#ifndef __NODE_SERVER_ASYNC_BASE__
#define __NODE_SERVER_ASYNC_BASE__


#include <grpc/support/log.h>
#include <grpcpp/grpcpp.h>

#ifdef BAZEL_BUILD
#include "peer_to_peer/protos/peer_to_peer.grpc.pb.h"
#else
#include "peer_to_peer.grpc.pb.h"
#endif


enum rpcCallStatus {
	CREATE,
	PROCESS,
	FINISH
};


// nodeServerBaseClass is the base class for classes encompasing the state and logic needed to serve a request.
template <typename AsyncService>
class nodeServerBaseClass {
public:
	nodeServerBaseClass(AsyncService* service, grpc::ServerCompletionQueue* cq, rpcCallStatus status)
		: service_(service), cq_(cq), status_(status) {}
	virtual ~nodeServerBaseClass() = default;
	virtual void Proceed() = 0;

protected:
	AsyncService* service_;
	grpc::ServerCompletionQueue* cq_;
	// Context for the rpc, allowing to tweak aspects of it such as the use
	// of compression, authentication, as well as to send metadata back to the
	// client.
	grpc::ServerContext ctx_;

	rpcCallStatus status_;  // The current serving state.
};
#endif // __PEER_TO_PEER_CALL_DATA__
