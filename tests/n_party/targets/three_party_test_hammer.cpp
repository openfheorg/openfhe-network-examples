/**
 * Have both clients sending multiple messages to the server and ensure nothing gets dropped.
 *      This amounts to making the server sleep and have the clients send over tons of messages
 *      and we just want to make sure it's all in order
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


  /**
   * If we are the clients, we just send the message to the server
   */
  if (params.NodeName == "Server") {  // The broadcaster
    std::cout << "Taking a nap" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(10000));
    std::string c1 = "Client1";
    std::string c2 = "Client2";
    int c1QSize = mynode.getQueueSize(c1);
    int c2QSize = mynode.getQueueSize(c2);
    std::cout << "DEMARCATE START" << std::endl;

    std::cout << "Server - Client1 Q size: " << std::to_string(c1QSize) << std::endl;
    std::cout << "Server - Client2 Q size: " << std::to_string(c2QSize) << std::endl;
    while ((c1QSize > 0) && (c2QSize) > 0){
      if (c1QSize > 0){
        auto reply1 = mynode.getMsg(c1);
        while (!reply1.acknowledgement) {
          reply1 = mynode.getMsg(c1);
        }
        std::string toCout = "Got message " + reply1.Data + ". Remaining in queue: " +
            std::to_string(c1QSize - 1);
        std::cout << toCout << std::endl;
      }
      if (c2QSize > 0){
        auto reply1 = mynode.getMsg(c2);
        while (!reply1.acknowledgement) {
          reply1 = mynode.getMsg(c2);
        }
        std::string toCout =
            "Got message " + reply1.Data +
            ". Remaining in queue: " + std::to_string(c2QSize - 1);
        std::cout << toCout << std::endl;
      }

      c1QSize = mynode.getQueueSize(c1);
      c2QSize = mynode.getQueueSize(c2);
    }
    std::cout << "DEMARCATE END" << std::endl;
  } else{  // clients sending to the server

    int numToSend = 50;  //
    message_format Msgsent;
    Msgsent.NodeName = params.NodeName;
    Msgsent.msgType = "HelloMsg";
    std::string sendMsgTo = "Server";
    std::string msgData = "Hello from node: " + params.NodeName + '\t';
    for (int start = 0; start < numToSend; start ++ ){
      Msgsent.Data = msgData + std::to_string(start);
      mynode.sendMsg(sendMsgTo, Msgsent, 0, 100);
    }
  }
  OPENFHE_DEBUG("End");
  mynode.Stop();  // exception thrown if not stopped
  return 0;
}