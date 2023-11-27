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

#include "utils/sertype.h"

#include "config.h"
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <boost/functional/hash.hpp>


constexpr bool TEST_MODE = true;
constexpr bool debug_flag_val = false;
constexpr bool PAUSE_FLAG = false;  //DBC note this should become a parameter.

// GlobalSerializationType is a const static variable used in ALL calls to serialiaze and de-serialize.
// it can hold one of these 2 values: lbcrypto::SerType::BINARY or lbcrypto::SerType::JSON
const auto GlobalSerializationType = lbcrypto::SerType::BINARY;
typedef std::pair<uint32_t,uint32_t> pair;

struct Params {
    // arguments to the executable
    std::string host_name;
    std::string process_name;
    std::string port;
    std::string credentials_location;
    std::string client_id;
    std::string Num_of_parties;
    std::string aborts;
    std::string computation;
    
    bool        disableSSLAuthentication;

    // variables used in the code
    std::string socket_address;
    std::string root_cert_file;
    std::string server_private_key_file;
    std::string server_cert_chain_file;
    std::string client_private_key_file;
    std::string client_cert_chain_file;

    std::string producer_aes_key = "demoData/keys/producer_aes_key.txt";
    std::string consumer_aes_key = "demoData/keys/consumer_aes_key";
    Params() : disableSSLAuthentication(false) {}
};

bool processInputParams(int argc, char* const argv[], Params& params, const char* optstring = "i:p:l:d:m:n:ac:W:h");
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
#endif // _UTILS_H_
