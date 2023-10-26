/**
* If we are the server we need to make sure we send the message(s)
*  to both clients(`parties` in the code)
 */
#include <string>
#include "node.h"
#include "node_utils.h"
#include <iostream>

int main(int argc, char** argv) {
  OPENFHE_DEBUG_FLAG(debug_flag_val);  // set to true to turn on DEBUG() statements

  CommParams params;
  // char optstring[] = "i:p:l:n:W:h";
  if (!processInputParams(argc, argv, params)) {
    usage(argv[0]);
    exit(EXIT_FAILURE);
  }
  // Demarcate where the tests start

  NodeImpl mynode;

  mynode.Register(params);  // initiate and add nodes
  mynode.Start();
  int recvCount = 0;
  int numParties = 50;
  message_format Msgsent;
  Msgsent.NodeName = params.NodeName;
  Msgsent.msgType = "HelloMsg";
  Msgsent.Data = "Inital blocking setup connection from: " + params.NodeName;
  mynode.broadcastMsg(Msgsent);

  std::cout << "**********************************************************"
            << std::endl;

  std::cout << "DEMARCATE START" << std::endl;
  for (int i = 0; i < numParties * 2; i++) {
    std::string formattedName;
    int wrapAround = i % numParties;

    if (wrapAround == 0) {
      formattedName = "Server";
    } else {
      formattedName = "Client" + std::to_string(wrapAround);
    }
    if (params.NodeName == formattedName) {  // The broadcaster
      message_format newMsg;
      newMsg.NodeName = params.NodeName;
      newMsg.msgType = "HelloMsg";
      newMsg.Data = "Async broadcast from: " + std::to_string(wrapAround);
      std::cout << formattedName + " sending data" + newMsg.Data << std::endl;
      mynode.broadcastMsgNoWait(newMsg);
    } else {
      std::string getfromNode = formattedName;
      auto reply1 = mynode.getMsg(getfromNode);
      while (!reply1.acknowledgement) {
        reply1 = mynode.getMsg(getfromNode);
      }

      auto toCout = "Got message: #" + std::to_string(recvCount) +
                    " where contents were " + reply1.Data;
      std::cout << toCout << std::endl;
      recvCount += 1;
    }
  }
  std::cout << "DEMARCATE END" << std::endl;

  OPENFHE_DEBUG("End");
  mynode.Stop();  // exception thrown if not stopped
  return 0;
}