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


  int numToSend = 50;                 //
  if (params.NodeName == "Server") {  // The broadcaster
    std::string sendToNode = "Client1";
    message_format Msgsent;
    Msgsent.NodeName = params.NodeName;
    Msgsent.msgType = "HelloMsg";
    std::cout << "DEMARCATE START" << std::endl;
    for (int i = 0; i < numToSend; i++) {
      Msgsent.Data = "Msg:" + std::to_string(i);
      mynode.sendMsg(sendToNode, Msgsent);
    }
    std::cout << "DEMARCATE END" << std::endl;
  } else {
    std::string getfromNode = "Server";
    int seenMsgs = 0;
    std::cout << "DEMARCATE START" << std::endl;
    while (seenMsgs < numToSend) {
      auto reply1 = mynode.getMsg(getfromNode);
      while (!reply1.acknowledgement) {
        reply1 = mynode.getMsg(getfromNode);
      }
      auto toCout = "Got message: " + std::to_string(seenMsgs);
      std::cout << toCout << std::endl;
      seenMsgs += 1;
    }
    std::cout << "DEMARCATE END" << std::endl;
  }

  OPENFHE_DEBUG("End");
  mynode.Stop();  // exception thrown if not stopped
  return 0;
}
