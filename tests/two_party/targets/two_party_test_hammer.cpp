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
    // Sleep first while the server fills up our queue
    std::this_thread::sleep_for(std::chrono::milliseconds(10000));
    std::string getfromNode = "Server";
    std::cout << "DEMARCATE START" << std::endl;
    int serverQSize = mynode.getQueueSize(getfromNode);
    std::cout << "Client - Server Q size: " << std::to_string(serverQSize)
              << std::endl;
    while (serverQSize > 0) {
      auto reply1 = mynode.getMsg(getfromNode);
      while (!reply1.acknowledgement) {
        reply1 = mynode.getMsg(getfromNode);
      }

      std::string toCout =
          "Got message " + reply1.Data +
          ". Remaining in queue: " + std::to_string(serverQSize - 1);
      std::cout << toCout << std::endl;
      serverQSize = mynode.getQueueSize(getfromNode);
    }
    std::cout << "DEMARCATE END" << std::endl;
  }

  OPENFHE_DEBUG("End");
  mynode.Stop();  // exception thrown if not stopped
  return 0;
}