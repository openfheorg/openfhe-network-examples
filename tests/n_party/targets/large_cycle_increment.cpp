/**
 * Here we implement a cycle, which comprises 3 nodes: A, B, C. The routing is
 * as follows:
 *
 *  1) A->B
 *  2) B->C
 *  3) C->A
 *
 *  where in between each send, we send a number to be incremented
 */

#include <string>
#include <iostream>

#include "node.h"
#include "node_utils.h"
void sendData(std::string myName, std::string sendToNode, CommParams params,
              NodeImpl &myNode, std::string msgSoFar) {
  if (params.NodeName == myName) {
    message_format newMsg;
    newMsg.NodeName = params.NodeName;
    newMsg.msgType = "HelloMsg";
    newMsg.Data = msgSoFar + "->" + myName;

    std::cout << myName + " sending msg:"
              << "\"" << newMsg.Data << "\""
              << " to " << sendToNode << std::endl;
    myNode.sendMsg(sendToNode, newMsg);
  }
}

std::string recvData(std::string myName, std::string recvFromNode, CommParams params,
              NodeImpl &myNode, std::string msgSoFar) {
  if (params.NodeName == myName) {
    auto reply1 = myNode.getMsg(recvFromNode);
    while (!reply1.acknowledgement) {
      reply1 = myNode.getMsg(recvFromNode);
    }

    std::cout << "Got: "
              << "\"" << reply1.Data << "\""
              << " from " << recvFromNode<< std::endl;

    return msgSoFar + reply1.Data;
  }
  return msgSoFar;
}

int main(int argc, char **argv) {
  OPENFHE_DEBUG_FLAG(debug_flag_val);  // set to true to turn on DEBUG() statements

  CommParams params;
  // char optstring[] = "i:p:l:n:W:h";
  if (!processInputParams(argc, argv, params)) {
    usage(argv[0]);
    exit(EXIT_FAILURE);
  }
  // Demarcate where the tests start

  NodeImpl myNode;

  myNode.Register(params);  // initiate and add nodes
  myNode.Start();
  int numParties = 60;

  // We do a full-cycle 5 times
  int NUM_REPS = 1;
  int CYCLE_SIZE = 3;

  std::cout << "DEMARCATE START" << std::endl;
  for (int reps = 0; reps < NUM_REPS; reps++) {
    std::string anchor;
    std::string oneAhead;
    std::string twoAhead;
    for (int senderIdx = 0; senderIdx < numParties + 1;
         senderIdx = senderIdx + CYCLE_SIZE) {
      if (senderIdx == 0) {
        anchor = "Server";
        oneAhead = "Client1";
        twoAhead = "Client2";
      } else {
        anchor = "Client" + std::to_string(senderIdx);
        oneAhead = "Client" + std::to_string(senderIdx + 1);
        twoAhead = "Client" + std::to_string(senderIdx + 2);
      }

      std::string msgSoFar = "";
      if (params.NodeName == anchor) {
//        std::cout << "ANCHOR: sending to" << oneAhead
//                  << " receiving from " << twoAhead << std::endl;
                sendData(anchor, oneAhead, params, myNode, msgSoFar);
                msgSoFar = recvData(anchor, twoAhead, params, myNode, msgSoFar);
      } else if (params.NodeName == oneAhead) {
//        std::cout << "ONE-AHEAD: receiving from " << anchor
//                  << " sending to " << twoAhead << std::endl;
                msgSoFar = recvData(oneAhead, anchor, params, myNode, msgSoFar);
                sendData(oneAhead, twoAhead, params, myNode, msgSoFar);
      } else if (params.NodeName == twoAhead) {
//        std::cout << "TWO-AHEAD: receiving from " << oneAhead
//                  << " sending to " << anchor << std::endl;
                msgSoFar = recvData(twoAhead, oneAhead, params, myNode, msgSoFar);
                sendData(twoAhead, anchor, params, myNode, msgSoFar);
      }
    }
  }

  std::cout << "DEMARCATE END" << std::endl;

  OPENFHE_DEBUG("End");
  myNode.Stop();  // exception thrown if not stopped
  return 0;
}