#include <string>
#include "node.h"
#include "node_utils.h"
#include <iostream>
#include <bitset>
#include <limits>

int main(int argc, char** argv) {
  OPENFHE_DEBUG_FLAG(debug_flag_val);  // set to true to turn on DEBUG() statements

  int bitStringSize = 7000000;
  std::string largeNumberBinary(bitStringSize, 1);
  for (int i = 0; i < bitStringSize; i++) {
    if (i % 2 == 0) {
      largeNumberBinary[i] = 0;
    }
  }
  std::cout << "Large binary generated" << std::endl;

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


  if (params.NodeName == "Server") {  // The broadcaster
    std::string sendToNode = "Client1";
    message_format Msgsent;
    Msgsent.NodeName = params.NodeName;
    Msgsent.msgType = "HelloMsg";
    Msgsent.Data = largeNumberBinary;
    std::cout << "DEMARCATE START" << std::endl;
    mynode.sendMsg(sendToNode, Msgsent);
    std::cout << "DEMARCATE END" << std::endl;
  } else {
    // Sleep first while the server fills up our queue
    std::string getfromNode = "Server";
    std::cout << "DEMARCATE START" << std::endl;
    auto reply1 = mynode.getMsg(getfromNode);
    while (!reply1.acknowledgement) {
      reply1 = mynode.getMsg(getfromNode);
    }
    std::string recvString = reply1.Data;
    if (recvString == largeNumberBinary){
      std::cout << "Original string and received string are equal" << std::endl;
    } else {
      std::cout << "Original string and received string are NOT equal" << std::endl;
    }
    std::cout << "DEMARCATE END" << std::endl;
  }

  OPENFHE_DEBUG("End");
  mynode.Stop();  // exception thrown if not stopped
  return 0;
}