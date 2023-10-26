/**
 * Tests what happens if we are expecting a message from a sender but that sender
 *      has dropped out
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

  int numToSend = 5;  //
  int serverSleep = 2;

  if (params.NodeName == "Server") {  // The broadcaster
    message_format Msgsent;
    sleep(serverSleep);
    Msgsent.NodeName = params.NodeName;
    Msgsent.msgType = "HelloMsg";
    std::cout << "DEMARCATE START" << std::endl;
    std::vector<std::string> parties = {"Client1", "Client2"};
    for (int i = 0; i < numToSend; i++) {
      Msgsent.Data = "Msg:" + std::to_string(i);
      mynode.broadcastMsg(Msgsent);

      if (i== 2) {
        std::cout << "Server dropout" << std::endl;
        std::cout << "DEMARCATE END" << std::endl;
        exit(EXIT_SUCCESS);
      }
    }
  } else {
    int seenMsgs = 1;
    int sleepTime = 100;
    int timeoutTol = 500;
    sleep(serverSleep * 2);

    std::string getfromNode = "Server";
    std::cout << "DEMARCATE START" << std::endl;
    while (seenMsgs < (numToSend + 1)) {
      auto reply1 = mynode.getMsgWait(getfromNode, "", sleepTime, timeoutTol);
      std::string toCout;
      if (reply1.acknowledgement) {
        toCout = "Got message: " + std::to_string(seenMsgs);
      } else {
        toCout = "Timeout on getting message: #" + std::to_string(seenMsgs);
      }

      std::cout << toCout << std::endl;
      seenMsgs += 1;
    } // END-WHILE (seenMsgs < (numToSend + 1))
  }
  std::cout << "DEMARCATE END" << std::endl;

  OPENFHE_DEBUG("End");
  mynode.Stop();  // exception thrown if not stopped
  return 0;
}