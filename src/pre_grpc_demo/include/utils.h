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
#ifndef _UTILS_H_
#define _UTILS_H_

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <boost/functional/hash.hpp>

#include <iomanip>
#include <vector>

#include "utils/sertype.h"
#include "config.h"

constexpr bool TEST_MODE = true;

// GlobalSerializationType is a const static variable used in ALL calls to serialiaze and de-serialize.
// it can hold one of these 2 values: lbcrypto::SerType::BINARY or lbcrypto::SerType::JSON
//auto GlobalSerializationType = lbcrypto::SerType::BINARY;
const auto GlobalSerializationType = lbcrypto::SerType::BINARY;//JSON;

typedef std::pair<std::string,std::string> pair;

struct Params {
    // arguments to the executable

    // name of the process - key server name/producer name/ broker name/ consumer name
    std::string process_name;

    // security model for PRE - INDCPA, FIXED_NOISE_HRA, NOISE_FLOODING_HRA, NOISE_FLOODING_HRA_HYBRID
    std::string security_model;
    
    // flag to enable/disable authentication
    bool        disableSSLAuthentication;

    // variables used in the code
    std::string key_server_socket_address;
    std::string upstream_key_server_socket_address;
    std::string broker_socket_address;
    std::string upstream_broker_name;
    std::string upstream_broker_socket_address; 
    std::string channel_name;

    // ssl certificate files
    std::string credentials_location;
    std::string root_cert_file;
    std::string server_private_key_file;
    std::string server_cert_chain_file;
    std::string client_private_key_file;
    std::string client_cert_chain_file;

    // input/output files for keys and routing tables
    std::string producer_aes_key = "demoData/keys/producer_aes_key";
    std::string consumer_aes_key = "demoData/keys/consumer_aes_key";
    std::string routingtablepath = "demoData/routingTables/";
    std::string access_map_path;

    Params() : disableSSLAuthentication(false) {}
};

bool processInputParams(int argc, char* const argv[], Params& params, const char* optstring = "k:u:d:i:s:c:l:m:n:a:W:h");
void usage(const char* task);


std::string file2String(const std::string& filename);
std::string buffer2PrintableString(const std::string& str);
std::string buffer2PrintableString(const char* str, size_t len);

template<typename T>
void write2File(const T& obj, const std::string& fileName) {
    std::ofstream myfile(fileName, std::ios::out | std::ios::binary);
    if (myfile) {
        std::ostringstream oss;
        oss << obj;
        myfile << oss.str();
        myfile.close();
    }
    else
        std::cerr << "ERROR: can't open " << fileName << std::endl;
}

std::vector<std::string> SplitString(std::string data, std::string delimiter);
std::map<std::string,std::vector<std::string>> ParseRoutingTable(std::string routingtablepath, std::string broker_name, std::string destination_client_name);
void UpdateRoutingTable(std::string routingtablepath, std::string channel_name, std::string broker_name, std::string upstream_client_name, std::string downstream_client_name, bool source_sink_flag);
void ClearRoutingTable(std::string routingtablefile);
std::vector<std::string> ParseAccessMap(std::string accessmappath);

//function to read in a string containing a hexadecimal digits, returns a vector of
// bits (little endian, least sigingicant bit in index 0,  stored in integers.
// note a modulus p is also required, but at this time only p=2 is supported.
//

std::vector<int64_t> hexstr2intvec(std::string instr, unsigned int p);

//function to read in a vector of
// bits (little endian, least sigingicant bit in index 0,  stored in integers.
// and output a hex string.
// note a modulus p is also required, but at this time only p=2 is supported.
//

std::string intvec2hexstr(const std::vector<int64_t> in_bits, unsigned int p);
#endif // _UTILS_H_
