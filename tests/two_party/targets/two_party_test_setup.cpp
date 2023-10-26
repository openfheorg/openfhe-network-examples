/**
 * Tests adding a node
 * Tests that all the nodes were setup correctly
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

  auto connectedNodes = mynode.getConnectedNodes();
  auto numNodes = std::to_string(connectedNodes.size());
  auto connectedAddresses = mynode.getConnectedAddresses();
  auto msgQueuesInfo = mynode.getMsgQueuesInformation();
  auto msgQueuesNames = std::get<0>(msgQueuesInfo);
  auto msgQSize = std::get<1>(msgQueuesInfo);

  mynode.Register(params);  // initiate and add nodes
  mynode.Start();

  // Initial values. Should all be empty
  std::cout << "DEMARCATE START" << std::endl;
  std::cout << "Initial ConnectedNodes Map Size: " << numNodes << std::endl;
  std::cout << "Initial Connected Addresses Size: " << connectedAddresses.size() << std::endl;
  std::cout << "Initial msgQueue Size: " << msgQSize << std::endl;

  /**
   * Reload all the variables which should now be non-empty
   */

  connectedNodes = mynode.getConnectedNodes();
  numNodes = std::to_string(connectedNodes.size());
  connectedAddresses = mynode.getConnectedAddresses();
  msgQueuesInfo = mynode.getMsgQueuesInformation();
  msgQueuesNames = std::get<0>(msgQueuesInfo);
  msgQSize = std::get<1>(msgQueuesInfo);

  std::cout << "New Map Size: " << numNodes << std::endl;
  std::cout << "New Connected Addresses Size: " << connectedAddresses.size()
            << std::endl;
  std::cout << "New msgQueue size: " << msgQSize << std::endl;

  for (auto& item : connectedNodes) {
    std::cout << "New Connected Nodes: Name: " << item << std::endl;
  }
  for (auto& item : connectedAddresses) {
    std::cout << "New Connected Addresses: Name: " << item.first;
    std::cout << ", Address: " << item.second << std::endl;
  }

  for (auto& item : msgQueuesNames) {
    std::cout << "New messageQueue Names: " << item << std::endl;
  }
  std::cout << "DEMARCATE END" << std::endl;

  OPENFHE_DEBUG("End");
  mynode.Stop();  // exception thrown if not stopped
  return 0;
}
