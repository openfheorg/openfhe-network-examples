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
#include "node.h"

int main(int argc, char** argv) {
  OPENFHE_DEBUG_FLAG(debug_flag_val);  // set to true to turn on DEBUG() statements

  CommParams params;
  // char optstring[] = "i:p:l:n:W:h";
  if (!processInputParams(argc, argv, params)) {
    usage(argv[0]);
    exit(EXIT_FAILURE);
  }

  NodeImpl mynode;

  mynode.Register(params);  // initiate and add nodes
  mynode.Start();

  std::cout << "My name " << params.NodeName << std::endl;
  std::string demarcate =
      "#####################################################\n";

  std::string sendtoNode, getfromNode;
  if (params.NodeName == "Node1") {  // The broadcaster
    message_format Msgsent;
    Msgsent.NodeName = params.NodeName;
    Msgsent.msgType = "HelloMsg";
    Msgsent.Data = "did you get my broadcast";

    mynode.broadcastMsg(Msgsent);

    std::cout << demarcate;
    std::cout << "Main node finished sending" << std::endl;
    std::cout << demarcate;
  } else {
    getfromNode = "Node1";
    auto reply1 = mynode.getMsg(getfromNode);

    while (!reply1.acknowledgement) {
      std::cout << "Nack for sendmsg" << std::endl;
      sleep(2);
      reply1 = mynode.getMsg(getfromNode);
    }
    std::cout << demarcate;
    std::cout << getfromNode << " says " << reply1.Data << std::endl;
    std::cout << demarcate;
    std::cout << "Finished getting blocking broadcast. Now doing non-blocking"
              << std::endl;
  }

  OPENFHE_DEBUG("End");
  mynode.Stop();  // exception thrown if not stopped
  return 0;
}