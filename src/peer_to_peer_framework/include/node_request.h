/***
 * Copyright (c) 20212021 Duality Technologies, Inc. All rights reserved.
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
//This file defines the messageservicer for the incoming messages recieved from other nodes to the node server 
// and is called by HandleRpcs in node.h.
//The received messages are assigned to the appropriate msgqueue in the msgQueues map of the node server.


#ifndef __NODE_REQUEST__
#define __NODE_REQUEST__

#include "node_utils.h"
#include "node_server_async_base.h"
#include "utils/exception.h"
#include "utils/debug.h"
#include "node.h"
#include <iostream>
#include <memory>
#include <string>

// ===================================================================================
// all instances of this class created dynamically
class MessageServicer : public nodeServerBaseClass<peer_to_peer::NodeServer::AsyncService> {
public:
  OPENFHE_DEBUG_FLAG(false);  // set to true to turn on DEBUG() statements
  MessageServicer(peer_to_peer::NodeServer::AsyncService* service, grpc::ServerCompletionQueue* cq,
				  std::map<std::string, tsqueue<message_format>>& messages)
	: nodeServerBaseClass(service, cq, CREATE), responder_(&ctx_), msgs(messages) {
	Proceed();
  }
  
  void Proceed() override {
	if (status_ == CREATE) {
	  OPENFHE_DEBUG("node_request.h: enter Proceed CREATE");		  
	  status_ = PROCESS;
	  
	  service_->RequestsendGRPCMsg(&ctx_, &request_, &responder_, cq_, cq_, this);
	  OPENFHE_DEBUG("node_request.h: exit Proceed CREATE");		  
	}
	else if (status_ == PROCESS) {
	  OPENFHE_DEBUG("node_request.h: enter Proceed PROCESS");		  
	  new MessageServicer(service_, cq_, msgs);

			TimeVar t;
			TIC(t);

            msg.NodeName = request_.node_name();
			msg.Data = request_.data();
			msg.msgType = request_.data_type();
	
            msgs[msg.NodeName].push(msg);

            auto elapsed_time = TOC_MS(t);

    	    OPENFHE_DEBUG("node_request.h: msg push to queue time: " << elapsed_time << " ms\n");

            OPENFHE_DEBUG("node_request.h: before setting reply acknowledgement");
            reply_.set_acknowledgement(true);			
            OPENFHE_DEBUG("node_request.h: after setting reply acknowledgement");
			status_ = FINISH;
			responder_.Finish(reply_, grpc::Status::OK, this);
			OPENFHE_DEBUG("node_request.h: exit Proceed PROCESS");
		}
		else {
		  OPENFHE_DEBUG("node_request.h: enter Proceed FINISH");		  
			GPR_ASSERT(status_ == FINISH);
			// Once in the FINISH state, deallocate ourselves (nodeServerBaseClass).
			OPENFHE_DEBUG("node_request.h: deleting this");		  
			delete this;
		}
	}

private:
	peer_to_peer::RequestMessage request_;
	peer_to_peer::ReturnMessage reply_;

	grpc::ServerAsyncResponseWriter<peer_to_peer::ReturnMessage> responder_;
    message_format msg;
	std::map<std::string, tsqueue<message_format>>& msgs;
};

#endif // __NODE_REQUEST__
