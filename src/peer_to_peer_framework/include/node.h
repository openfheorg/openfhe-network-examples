// @file another_server.h
// @author TPOC: info@DualityTech.com
//
// @copyright Copyright (c) 2021, Duality Technologies Inc.
// All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution. THIS SOFTWARE IS
// PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
// EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// This file defines all the methods and variables of the node object.
/*
Methods:
> Init -- Initialize gRPC server listening on the port in the socket address.
> Register -- Calls Init and AddNodes on the current node name.
> LookUpNetworkMap -- Reads the Network map file and assigns the connected nodes and addresses of
NodeName to the vector connectedNodes and map connectedAddresses respectively.
> AddNodes -- calls LookUpNetworkMap to get all nodes connected to NodeName and
then calls AddNode for each of the connected nodes.
> AddNode -- creates message queue for the node NodeName and adds it to the map msgQueues.
> sendMsgNoWait -- sends message to another node SendtoNodeName but does not wait to check the acknowledgement.
This is handled through sendmessage in node_client.h.
> sendMsg -- calls sendMsgNoWait and sends a message to another node SendtoNodeName until an acknowledgment
is received.
> getMsg -- receive a message from getFromNode and assigns it to its message queue in the map msgQueues.
> getMsgWait -- calls getMsg to receive a message from getFromNode until the message is found in the corresponding
msg queue or a timeout.
> getMsgByType -- check for a specific message type and receive the message from getFromNode.
> getMsgByTypeWait -- calls getMsgByType check for a specific message type and receive the message from getFromNode
until the message is found in the corresponding msg queue or a timeout.
> HandleRpcs -- gRPC message servicer to handle the messages received by the node server.
The messages are handled by node_request.h.
> Run -- call to the HandleRpcs method.
> Start -- calls Run in a separate thread allowing the node server to listen and process messages through HandleRpcs.
> Stop -- stops the server thread of the node.
*/

#ifndef __NODE_H__
#define __NODE_H__

#include <iostream>
#include <algorithm>
#include <tuple>

#include "node_request.h"
#include "node_client.h"
#include "openfhe.h"
#include "utils/exception.h"
#include "node_utils.h"
#include "utils/debug.h"

#include "gen-cryptocontext.h"
#include "cryptocontext-ser.h"
#include "key/key.h"
#include "key/key-ser.h"
#include "utils/serial.h"

#include "scheme/bfvrns/bfvrns-ser.h"
#include "scheme/ckksrns/ckksrns-ser.h"

//#include "scheme/bfvrns/cryptocontext-bfvrns.h"
//#include "scheme/ckksrns/cryptocontext-ckksrns.h"

using grpc::Server;
using grpc::ServerBuilder;
class NodeImpl final {
public:
    OPENFHE_DEBUG_FLAG(debug_flag_val);  // set to true to turn on DEBUG() statements
    ~NodeImpl() {
        server_->Shutdown();
        // Always shutdown the completion queue after the server.
        cq_->Shutdown();
    }

    void Init(std::string NodeName, std::string socket_address) {
        MyNodeName = NodeName;
        ServerBuilder builder;
        // from https://github.com/grpc/grpc/blob/master/include/grpc/impl/codegen/grpc_types.h:
        // GRPC doesn't set size limit for sent messages by default. However, the max size of received messages
        // is limited by "#define GRPC_DEFAULT_MAX_RECV_MESSAGE_LENGTH (4 * 1024 * 1024)", which is only 4194304.
        // for every GRPC application the received message size must be set individually:
        //     for a client it may be "-1" (unlimited): SetMaxReceiveMessageSize(-1)
        //     for a server INT_MAX (from #include <climits>) should do it: SetMaxReceiveMessageSize(INT_MAX)
        // see https://nanxiao.me/en/message-length-setting-in-grpc/ for more information on the message size
        builder.SetMaxReceiveMessageSize(INT_MAX);

        std::shared_ptr<grpc::ServerCredentials> creds = nullptr;
        if (nodeparams.disableSSLAuthentication) {
            std::cout << MyNodeName << " SSL disabled " << std::endl;
            creds = grpc::InsecureServerCredentials();
        } else {
            std::cout << MyNodeName << " SSL enabled " << std::endl;
            grpc::SslServerCredentialsOptions::PemKeyCertPair keyCert = {
                file2String(nodeparams.server_private_key_file),
                file2String(nodeparams.server_cert_chain_file) };
            grpc::SslServerCredentialsOptions opts;
            opts.pem_root_certs = file2String(nodeparams.root_cert_file);
            opts.pem_key_cert_pairs.push_back(keyCert);
            creds = grpc::SslServerCredentials(opts);
        }
        builder.AddListeningPort(socket_address, creds);
        builder.RegisterService(&service_);
        cq_ = builder.AddCompletionQueue();
        server_ = builder.BuildAndStart();
        std::cout << MyNodeName << " server listening on " << socket_address << std::endl;
    }

    void LookUpNetworkMap(std::string NodeName, std::string NetworkMapFile) {
        //file or parse from cmd line arguments.
        bool node_found = false;
        std::string data;

        // network map comment
        // Given NodeName == Node 1 (for example) A given line in the NetworkMap might be
        //    Node1-Node2@localhost:50052,Node5@localhost:50055
        std::string NetworkMapfile = file2String(NetworkMapFile);
        std::istringstream NetworkMap (NetworkMapfile);

        // network map comment
        while ((NetworkMap >> data) || (!node_found)) {
            // datasplit = {"Node 1", "Node 2", ...}
            std::vector<std::string> datasplit = SplitString(data, "-");

            if (NodeName == datasplit[0]) {
                node_found = true;
                OPENFHE_DEBUG("nodes from network map: " << datasplit[1]);

                // (vec) {Node2@localhost:50052
                //        Node5@localhost:50055}
                std::vector<std::string> nodes = SplitString(datasplit[1],",");
                OPENFHE_DEBUG("nodes size after datasplit: " << nodes.size());
                for(size_t i = 0; i < nodes.size(); i++) {
                    std::vector<std::string> nodeSplit = SplitString(nodes[i],"@");  // nodessplit: <Node2, localhost:ABCDE>

                    OPENFHE_DEBUG("adding node from network map: " << nodeSplit[0]);
                    connectedNodes.push_back(nodeSplit[0]);
                    connectedAddresses[nodeSplit[0]] = nodeSplit[1];
                    establishedConnections[nodeSplit[0]] = false;
                }
            }
        }
        if (!node_found) {
            std::cerr << "Node " << NodeName << "is not in the network map" << std::endl;
            EXIT_FAILURE;
        }
    }

    std::vector<std::string> getConnectedNodes() const {
        return connectedNodes;
    }


    void broadcastMsg(std::map<std::string, std::string> toSendTo,
                          message_format& msg, usint sleepTimeMsec = 1,
                          usint timeoutMsec = 0) {
        //#pragma omp parallel for
        for (auto const& entry : toSendTo) {
            auto nodeName = entry.first;
            this->sendMsg(nodeName, msg, sleepTimeMsec, timeoutMsec);
        }
    }

    void broadcastMsg(message_format& msg, usint sleepTimeMsec = 1,
                          usint timeoutMsec = 0) {
        return this->broadcastMsg(connectedAddresses, msg, sleepTimeMsec, timeoutMsec);
    }

    template<typename T>
    void broadcastSerialMsg(T& cryptoObject, message_format& msg,
                                std::string msgType, usint sleepTimeMsec = 1,
                                usint timeoutMsec = 0) {
        std::ostringstream os;
        lbcrypto::Serial::Serialize(cryptoObject, os, GlobalSerializationType);
        if (!os.str().size())
            OPENFHE_THROW(lbcrypto::serialize_error, "Serialized " + msgType + " is empty");

        //send to other node
        msg.msgType = msgType;
        msg.Data = os.str();

        TIC(t);

        this->broadcastMsg(connectedAddresses, msg, sleepTimeMsec, timeoutMsec);

        auto elapsed_seconds = TOC_MS(t);

    	OPENFHE_DEBUG(msgType << " Broadcast serial Msg time: " << elapsed_seconds << " ms\n");
    }

    void AddNodes(std::string& NodeName, std::string& NetworkMapFile) {
        LookUpNetworkMap(NodeName, NetworkMapFile);
        OPENFHE_DEBUG("Nodes to add in addnodes in node.h " << connectedNodes.size());
        for (size_t i = 0; i < connectedNodes.size(); i++) {
            OPENFHE_DEBUG("iteration to add in addnodes in node.h " << connectedNodes[i]);
            AddNode(connectedNodes[i], connectedAddresses[connectedNodes[i]]);
        }
        OPENFHE_DEBUG("At end of Addnodes in node.h ");
    }

    void AddNode(std::string& NodeName, std::string& socket_address) {
        //create message queue for the node NodeName
        tsqueue<message_format> m_qMessagesIn;

        m_qMessagesIn.NodeName = NodeName;
        m_qMessagesIn.NodeAddress = socket_address;

        msgQueues[NodeName] = m_qMessagesIn;
    }

    std::map<std::string, std::string> getConnectedAddresses() const {
        return connectedAddresses;
    };

    std::tuple<std::vector<std::string>, int> getMsgQueuesInformation() const {
        std::vector<std::string> mapHolder;

        for (auto &v: msgQueues) {
            mapHolder.emplace_back(v.first);
        }
        return std::make_tuple(mapHolder, msgQueues.size());
    }

    int getQueueSize(std::string &nodeName) {
        return msgQueues[nodeName].count();
    }


    void Register(CommParams& params) {
        nodeparams = params;
        MyNodeName = params.NodeName;
        std::string MyNodeAddress = params.socket_address;
        std::string NetworkMapFile = params.NetworkMapFile;

        this->Init(MyNodeName, MyNodeAddress);
        this->AddNodes(MyNodeName, NetworkMapFile);
    }

    /**
     * Send a message to another node with some special logic inside
     * @param sendtoNode The node to send the payload
     * @param Msgsent The payload contents
     * @param sleepTimeMsec The sleep time in milliseconds (10_000 == 10 seconds)
     * @param timeoutMsec The total time before we quit trying
     * @param verbose Whether to output information about nacks and trying to establish connections. Should this be a DEBUG
     * @param allowOverride If we have not connected to a node before, we might want to increase the tolerances. By default we allow overriding
     *
     */
    void sendMsg(std::string& sendtoNode, message_format& Msgsent,
                     usint sleepTimeMsec = 1, usint timeoutMsec = 0,
                     bool verbose=false, bool allowOverride=true
                     ) {

        TIC(t);

        auto reply = this->sendMsgNoWait(sendtoNode, Msgsent);

        auto elapsed_seconds = TOC_MS(t);

        OPENFHE_DEBUG("SendMsg no wait time: " << elapsed_seconds << " ms\n");

        // first establish that the other node exists in the map
        // if (establishedConnections.find(sendtoNode) ==
        //    establishedConnections.end()) {
        // Now that we know it is, we check if we have already established a connection
        TIC(t);
        if (establishedConnections[sendtoNode] == false && allowOverride) {
            sleepTimeMsec = 10;  // sleep 10 ms at a time
            timeoutMsec = 10000;  // wait a total of 10 seconds
            OPENFHE_DEBUG("First connection to " << sendtoNode << ". Bumping tolerances for establishing connection. Sleep time: " << sleepTimeMsec << "timeout time: " << timeoutMsec);

            std::vector<std::string> star={"|","/","-","\\"};
            int starptr = 0;

            usint wait_ctr = 0;
            while ((reply.error_msg() == "failed to connect to all addresses") || (((wait_ctr * sleepTimeMsec) > timeoutMsec) && (timeoutMsec != 0))) {
                std::this_thread::sleep_for(std::chrono::milliseconds(sleepTimeMsec));
                reply = this->sendMsgNoWait(sendtoNode, Msgsent);

                std::cout << star[starptr]<< "\r" << std::flush;
                starptr = (starptr+1)%star.size();
                wait_ctr++;
            }
        }

        usint timeoutCtr = 0;

        while (!reply.acknowledgement()) {
            if (verbose) {
                std::cout << MyNodeName << " Nack for sendMsg" << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepTimeMsec));
            reply = this->sendMsgNoWait(sendtoNode, Msgsent);

            // Now we consider how many times we've slept by how long each sleep is
            //    and compare that against how long our timeout was
            if (((timeoutCtr * sleepTimeMsec) > timeoutMsec) && (timeoutMsec != 0)) {
                if (verbose) {
                    std::cerr << MyNodeName << " sendMsg timed out" << std::endl;
                }
                break;
            }
            timeoutCtr++;
        }
        elapsed_seconds = TOC_MS(t);
        // Note that we've connected to this node before so we do not bump the tolerances next time
        establishedConnections[sendtoNode] = true;

        OPENFHE_DEBUG("SendMsg wait time: " << elapsed_seconds << " ms\n");
    }

    template<typename T>
    void sendSerialMsg(T& cryptoObject, message_format& msgSent,
                           std::string msgType, std::string sendtoNode,
                           usint sleepTimeMsec = 1, usint timeoutMsec = 0) {
        std::ostringstream os;
        lbcrypto::Serial::Serialize(cryptoObject, os, GlobalSerializationType);
        if (!os.str().size())
            OPENFHE_THROW(lbcrypto::serialize_error, "Serialized " + msgType + " is empty");

        if(debug_flag_val)
            write2File(os.str(), "./node_" + msgType + "_from_" + this->MyNodeName + "_to_" + sendtoNode + "_sent_serialized.txt");

        OPENFHE_DEBUG(msgType << " message size: " << os.str().size() << "\n");

        TIC(t);
        //send to other node
        msgSent.msgType = msgType;
        msgSent.Data = os.str();

        auto elapsed_seconds = TOC_MS(t);

        OPENFHE_DEBUG(msgType << " assign msg in SendSerialMsg time: " << elapsed_seconds << " ms\n");

        TIC(t);

        this->sendMsg(sendtoNode, msgSent, sleepTimeMsec, timeoutMsec);

        elapsed_seconds = TOC_MS(t);

    	OPENFHE_DEBUG(msgType << " SendMsg in SendSerialMsg time: " << elapsed_seconds << " ms\n");
    }


    message_format getMsg(std::string& getFromNode, std::string msgtype="") {
        message_format msg;
        msg.acknowledgement = false;

        if (!msgQueues[getFromNode].empty()) {
            if (msgtype.size() == 0) {
                msg = msgQueues[getFromNode].pop();
            } else {
                msg = msgQueues[getFromNode].pop_with_type(msgtype);
            }
            msg.acknowledgement = true;
        }
        return msg;
    }

    message_format getMsgWait(std::string& getFromNode, std::string msgType = "",
                              usint sleepTimeMsec = 1,
                              usint timeoutMsec = 0,
                              bool verbose=false) {
        TIC(t);

        auto reply = this->getMsg(getFromNode, msgType);

        std::vector<std::string> star = {"|", "/", "-", "\\"};
        int starptr = 0;

        usint timeoutCtr = 0;
        while (!reply.acknowledgement) {
            std::cout << star[starptr] << "\r" << std::flush;
            starptr = (starptr + 1) % star.size();
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepTimeMsec));
            reply = this->getMsg(getFromNode, msgType);
            if (((timeoutCtr * sleepTimeMsec) > timeoutMsec) && (timeoutMsec != 0)) {
                if (verbose) {
                    std::cerr << MyNodeName << " getMsgWait timed out";
                }
                break;
            }
            timeoutCtr++;
        }

        auto elapsed_seconds = TOC_MS(t);

        OPENFHE_DEBUG(msgType << " GetMsg wait time: " << elapsed_seconds << " ms\n");

        return reply;
    }

    template<typename T>
    void getSerialMsgWait(T& cryptoObject, std::string msgType, std::string getFromNode) {
        TIC(t);

        auto serializedObject = this->getMsgWait(getFromNode).Data;

        auto elapsed_seconds = TOC_MS(t);

        OPENFHE_DEBUG(msgType << " getMsgWait in getSerialMsgWait time: " << elapsed_seconds << " ms\n");

        if (!serializedObject.size())
            OPENFHE_THROW(lbcrypto::openfhe_error, "Received empty serialized object " + msgType);

        if (debug_flag_val)
            write2File(serializedObject, "./node_" + msgType + "_from_" + getFromNode + "_to_" + this->MyNodeName + "_received_deserialized.txt");

        std::istringstream is(serializedObject);

        TIC(t);
        lbcrypto::Serial::Deserialize(cryptoObject, is, GlobalSerializationType);
        elapsed_seconds = TOC_MS(t);

        OPENFHE_DEBUG(msgType << "Deserialize in getSerialMsgWait time: " << elapsed_seconds << " ms\n");
        OPENFHE_DEBUG(msgType << " deserialized message size: " << is.str().size() << "\n");
    }


    bool checkMsg(std::string& getFromNode, std::string msgType) {
        bool msgfound = msgQueues[getFromNode].check_msg_type(msgType);
        return msgfound;
    }

    void Run() {
        HandleRpcs();
    }

    bool Start() {
        try
        {
            // Launch the node in its own thread
            m_threadContext = std::thread([this]() { Run(); });
        }
        catch (std::exception& e)
        {
            // Something prohibited the server from listening
            std::cerr << "[SERVER] Exception: " << e.what() << "\n";
            return false;
        }
        std::cout << MyNodeName << " [SERVER] Started!\n";
        return true;
    }

    // Stops the server!
    void Stop() {
        server_->Shutdown();
        // Always shutdown the completion queue after the server.
        cq_->Shutdown();

        // Tidy up the context thread
        if (m_threadContext.joinable()) m_threadContext.join();
            // Inform someone, anybody, if they care...
            std::cout << MyNodeName << " [SERVER] Stopped!\n";
    }

    void set_keyPair(lbcrypto::KeyPair<lbcrypto::DCRTPoly>& generatedkeyPair) {
        keyPair = generatedkeyPair;
    }

    void set_FinalPubKey(lbcrypto::PublicKey<lbcrypto::DCRTPoly>& PublicKey) {
        FinalPubKey = PublicKey;
    }

    void set_evalMultFinal(lbcrypto::EvalKey<lbcrypto::DCRTPoly>& evalMultKey) {
        evalMultFinal = evalMultKey;
    }

    void set_evalSumFinal(std::shared_ptr<std::map<usint, lbcrypto::EvalKey<lbcrypto::DCRTPoly>>>& evalSumKey) {
        evalSumFinal = evalSumKey;
    }

    lbcrypto::KeyPair<lbcrypto::DCRTPoly> get_keyPair() const {
        return keyPair;
    }

    lbcrypto::PublicKey<lbcrypto::DCRTPoly> get_FinalPubKey() const {
        return FinalPubKey;
    }

    lbcrypto::EvalKey<lbcrypto::DCRTPoly> get_evalMultFinal() const {
        return evalMultFinal;
    }

    std::shared_ptr<std::map<usint, lbcrypto::EvalKey<lbcrypto::DCRTPoly>>> get_evalSumFinal() const {
        return evalSumFinal;
    }

    TimeVar t;

    /**
     * Send a message (`msg`) to all in the given mapping (`toSendTo`)
     * @param toSendTo
     * @param msg
     * @return
     */
    std::vector<peer_to_peer::ReturnMessage> broadcastMsgNoWait(
            std::map<std::string, std::string> toSendTo, message_format& msg) {
        std::vector<peer_to_peer::ReturnMessage> responses;
        for (auto const& entry : toSendTo) {
            auto nodeName = entry.first;
            auto resp = this->sendMsgNoWait(nodeName, msg);
            responses.emplace_back(resp);
        }
        return responses;
    }

    /**
     * Send a message (`msg`) to all connected addresses
     * @param msg
     * @return
     */
    std::vector<peer_to_peer::ReturnMessage> broadcastMsgNoWait(message_format& msg) {
        return this->broadcastMsgNoWait(this->connectedAddresses, msg);
    }


private:
    // This can be run in multiple threads if needed.
    void HandleRpcs() {
        new MessageServicer(&service_, cq_.get(), msgQueues);
        void* tag;  // uniquely identifies a request.
        bool ok = false;
        bool loop = true;
        while (loop) {
            // Block waiting to read the next event from the completion queue. The
            // event is uniquely identified by its tag, which in this case is the
            // memory address of a nodeServerBaseClass instance.
            // The return value of Next should always be checked. This return value
            // tells us whether there is any kind of event or cq_ is shutting down.
            GPR_ASSERT(cq_->Next(&tag, &ok));
            //GPR_ASSERT(ok);

            if (ok) {
                static_cast<nodeServerBaseClass<peer_to_peer::NodeServer::AsyncService>*>(tag)->Proceed();
            } else {
                std::cout << MyNodeName << " server completion queue shutdown" << std::endl;
                loop = false;
            }
        }
    }

    peer_to_peer::ReturnMessage sendMsgNoWait(std::string& SendtoNodeName, message_format& Msg) {
        std::string receiver_node_socket_address;
        if (connectedAddresses.find(SendtoNodeName) == connectedAddresses.end()) {
            std::cerr << "Node " << SendtoNodeName
                      << "is not in the network map for Node " << MyNodeName
                      << std::endl;
            exit(EXIT_FAILURE);
        } else {
            receiver_node_socket_address = connectedAddresses[SendtoNodeName];
        }

        NodeClient nodeClient(receiver_node_socket_address, nodeparams);

        auto reply = nodeClient.sendGRPCMsg(SendtoNodeName, Msg);
        return reply;
    }

    std::unique_ptr<grpc::ServerCompletionQueue> cq_;
    peer_to_peer::NodeServer::AsyncService service_;
    std::unique_ptr<grpc::Server> server_;

    std::map<std::string, tsqueue<message_format>> msgQueues;   //
    std::map<std::string, std::string> connectedAddresses;      // "Node2": "localhost:50052",
    std::map<std::string, bool> establishedConnections;         // "Node2": true
    std::vector<std::string> connectedNodes;                    // <Node2, Node3, ... >
    std::string MyNodeName;
    CommParams nodeparams;
    std::thread m_threadContext;

    lbcrypto::KeyPair<lbcrypto::DCRTPoly> keyPair;
    lbcrypto::PublicKey<lbcrypto::DCRTPoly> FinalPubKey;
    lbcrypto::EvalKey<lbcrypto::DCRTPoly> evalMultFinal;
    std::shared_ptr<std::map<usint, lbcrypto::EvalKey<lbcrypto::DCRTPoly>>> evalSumFinal;
};
#endif // __NODE_H__

