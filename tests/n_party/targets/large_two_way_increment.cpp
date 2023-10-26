#include <string>
#include "node.h"
#include "node_utils.h"
#include <iostream>

void sendData(int partyIdx, int numParties, CommParams params, NodeImpl &myNode,
              bool reverse = false) {
  std::string myName;
  if (partyIdx == 0) {
    myName = "Server";
  } else {
    myName = "Client" + std::to_string(partyIdx);
  }
  if (params.NodeName == myName) {
    message_format newMsg;
    newMsg.NodeName = params.NodeName;
    newMsg.msgType = "HelloMsg";
    newMsg.Data = "Ping from " + myName;

    std::string sendToNode;

    if (reverse) {
      if (partyIdx == numParties / 2) {
        sendToNode = "Server";
      } else {
        sendToNode = "Client" + std::to_string(partyIdx - numParties / 2);
      }
    } else {
      sendToNode = "Client" + std::to_string(partyIdx + numParties / 2);
    }

    std::cout << myName + " sending msg:" + "\"" + newMsg.Data + "\""
              << " and sending to " + sendToNode << std::endl;
    myNode.sendMsg(sendToNode, newMsg);
  }
}

int recvData(int partyIdx, int numParties, CommParams params, NodeImpl &myNode,
             int recvCount, bool reverse = false) {
  std::string myName;
  if (partyIdx == 0) {
    myName = "Server";
  } else {
    myName = "Client" + std::to_string(partyIdx);
  }

  if (params.NodeName == myName) {
    std::string getFromNode;
    if (reverse) {
      if (partyIdx == numParties / 2) {
        getFromNode = "Server";
      } else {
        getFromNode = "Client" + std::to_string(partyIdx + numParties / 2);
      }
    } else {
      if (partyIdx == numParties / 2) {
        getFromNode = "Server";
      } else {
        getFromNode = "Client" + std::to_string(partyIdx - numParties / 2);
      }
    }
    auto reply1 = myNode.getMsg(getFromNode);
    while (!reply1.acknowledgement) {
      reply1 = myNode.getMsg(getFromNode);
    }

    auto toCout = "Got message: #" + std::to_string(recvCount) +
                  " where contents were " + "\"" + reply1.Data + "\"" +
                  " from " + getFromNode;
    std::cout << toCout << std::endl;
    recvCount += 1;
    return recvCount;
  }
  return recvCount;
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
  int recvCount = 0;
  int numParties = 50;

  // We do a full-cycle 5 times
  int NUM_REPS = 5;


  std::cout << "DEMARCATE START" << std::endl;
  for (int reps = 0; reps < NUM_REPS; reps++) {
    std::string myName;

    for (int partyIdx = 0; partyIdx < numParties; partyIdx++) {
      if (partyIdx == 0) {
        myName = "Server";
      } else {
        myName = "Client" + std::to_string(partyIdx);
      }
      if (params.NodeName == myName) {
        if (partyIdx < numParties / 2) {
          sendData(partyIdx, numParties, params, myNode);
        } else {
          recvCount = recvData(partyIdx, numParties, params, myNode, recvCount);
        }
      }
    }

    for (int partyIdx = numParties - 1; partyIdx > -1; partyIdx--) {
      if (partyIdx == 0) {
        myName = "Server";
      } else {
        myName = "Client" + std::to_string(partyIdx);
      }
      if (params.NodeName == myName){
        if (partyIdx < numParties / 2) {
          recvCount =
              recvData(partyIdx, numParties, params, myNode, recvCount, true);
        } else {
          sendData(partyIdx, numParties, params, myNode, true);
        }
      }

    }
  }

  std::cout << "DEMARCATE END" << std::endl;

  OPENFHE_DEBUG("End");
  myNode.Stop();  // exception thrown if not stopped
  return 0;
}