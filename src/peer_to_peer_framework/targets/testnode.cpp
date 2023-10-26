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
	//char optstring[] = "i:p:l:n:W:h";
	if (!processInputParams(argc, argv, params)) {
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	int num_of_msgs = 10;
	NodeImpl mynode;

	message_format Msgsent;

	mynode.Register(params); //initiate and add nodes
    mynode.Start();
	
	std::cout << "My name " << params.NodeName << std::endl;

    std::string sendtoNode, getfromNode;
	if (params.NodeName == "Node1") {
		sendtoNode = "Node2";
	} else if (params.NodeName == "Node2")
	{
		sendtoNode = "Node3";
	} else if (params.NodeName == "Node3")
	{
		sendtoNode = "Node4";
	} else if (params.NodeName == "Node4")
	{
		sendtoNode = "Node5";
	} else {
		sendtoNode = "Node1";
	}

    Msgsent.NodeName = params.NodeName;
	Msgsent.msgType = "HelloMsg";

    for (int i = 1; i < num_of_msgs; i++) {
        
        Msgsent.Data = "sayHello " + std::to_string(i) +" from " + params.NodeName;

		mynode.sendMsg(sendtoNode, Msgsent);
	}	

    OPENFHE_DEBUG("here before SendNodeMsg in node.cpp");
	

    if (params.NodeName == "Node1") {
		getfromNode = "Node5";
	} else if (params.NodeName == "Node2")
	{
		getfromNode = "Node1";
	} else if (params.NodeName == "Node3")
	{
		getfromNode = "Node2";
	} else if (params.NodeName == "Node4")
	{
		getfromNode = "Node3";
	} else {
		getfromNode = "Node4";
	}

    for(int i = 1; i < num_of_msgs; i++) {
        auto reply1 = mynode.getMsg(getfromNode);

	    while (!reply1.acknowledgement) {
            std::cout << "Nack for sendmsg" << std::endl;    
		    sleep(2);
		    reply1 = mynode.getMsg(getfromNode);
	    }
		std::cout << "Message from " << getfromNode << " is " << reply1.Data <<std::endl;
	}

    //verify sendsyncmsg and getsyncmsg
    Msgsent.msgType = "test";
    Msgsent.Data = "testdata";

    mynode.sendMsg(sendtoNode, Msgsent);

    //check the msgtype of front of queue
	bool chkMsgType = mynode.checkMsg(getfromNode, "test1");
    std::cout << "check msg type expected value: 0, actual: " << chkMsgType << std::endl;

    auto serialdata = mynode.getMsgWait(getfromNode).Data;
    std::cout << "testing getMsgWait Message from " << getfromNode << " is " << serialdata <<std::endl;

    //verify getmsg with msgtype
	//verify sendsyncmsg and getsyncmsg
    Msgsent.msgType = "test1";
    Msgsent.Data = "testdata1";

    mynode.sendMsg(sendtoNode, Msgsent);

    //check the msgtype of front of queue
	chkMsgType = mynode.checkMsg(getfromNode, "test1");
    std::cout << "check msg type expected value: 1, actual: " << chkMsgType << std::endl;

    serialdata = mynode.getMsgWait(getfromNode, "test1",2, 0).Data;
    std::cout << "testing getMsgByTypeWait with msgtype Message from " << getfromNode << " is " << serialdata <<std::endl;

	mynode.Stop(); //exception thrown if not stopped
	return 0;
}
