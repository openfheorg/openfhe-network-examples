/***
 * Copyright (c) 2021 2021 Duality Technologies, Inc. All rights reserved.
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
#ifndef _NODE_UTILS_H_
#define _NODE_UTILS_H_

#include "utils/sertype.h"

#include "config.h"
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
//
#include <memory>
//
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>

constexpr bool TEST_MODE = true;
constexpr bool debug_flag_val = false;
constexpr bool PAUSE_FLAG = false; 

// GlobalSerializationType is a const static variable used in ALL calls to serialiaze and de-serialize.
// it can hold one of these 2 values: lbcrypto::SerType::BINARY or lbcrypto::SerType::JSON
const auto GlobalSerializationType = lbcrypto::SerType::BINARY;

typedef std::pair<std::string,std::string> pair;

struct CommParams {
    // arguments to the executable

    std::string NodeName;
    std::string socket_address;
    std::string NetworkMapFile;
	std::string ApplicationFile;
	std::string ExtraArgument;
    std::string credentials_location;
    
    bool        disableSSLAuthentication;

    // variables used in the code
    std::string root_cert_file;
    std::string server_private_key_file;
    std::string server_cert_chain_file;
    std::string client_private_key_file;
    std::string client_cert_chain_file;

    CommParams() : disableSSLAuthentication(false) {}
};

bool processInputParams(int argc, char* const argv[], CommParams& params, const char* optstring = "n:s:m:f:e:l:W:h");
void usage(const char* task);

struct message_format
{
	//todo: determine the message content that needs to be serialized
	//main data content
	bool acknowledgement;
    std::string NodeName;
	std::string NodeAddress;
	std::string msgType;
	std::string Data;

    //additional data content
	std::string sender_type;
	std::string sender_name;
	std::string receiver_name;
	std::string receiver_type;

    //operation specific
	std::string additional_data;
};

template<typename T>
class tsqueue
{
public:
	tsqueue() = default;
	tsqueue(const tsqueue<T>&) = delete;
	virtual ~tsqueue() { clear(); }

public:
    std::string NodeName;
	std::string NodeAddress;
	// Returns and maintains item at front of Queue
	const T& front()
	{
		std::unique_lock<std::mutex> lock(muxQueue);
		return Queue.front();
	}

	// Returns and maintains item at back of Queue
	const T& back()
	{
		std::unique_lock<std::mutex> lock(muxQueue);
		return Queue.back();
	}

	// Removes and returns item from front of Queue
	T pop()
	{
		std::unique_lock<std::mutex> lock(muxQueue);
        
		T t;
		if (!Queue.empty()) {
            t = std::move(Queue.front());
		    Queue.pop();
		}
		
		return t;
	}

    T pop_with_type(std::string& msgtype)
	{
		T t;
		std::unique_lock<std::mutex> lock(muxQueue);
		while (!Queue.empty()) {
			if (Queue.front().msgType == msgtype) {
				t = std::move(Queue.front());
				Queue.pop();
				break;
			}
			Queue.pop();
		}
		
		return t;
	}

	bool check_msg_type(std::string& msgtype)
	{
		bool found = false;
		if (Queue.front().msgType == msgtype) {
			found = true;
		}
		return found;
	}
	
	// Adds an item to back of Queue
	void push(const T& item)
	{
		std::unique_lock<std::mutex> lock(muxQueue);
		Queue.emplace(std::move(item));

		std::unique_lock<std::mutex> ul(muxBlocking);
		cvBlocking.notify_one();
	}

	// Returns true if Queue has no items
	bool empty()
	{
		std::unique_lock<std::mutex> lock(muxQueue);
		return Queue.empty();
	}

	// Returns number of items in Queue
	size_t count()
	{
		std::unique_lock<std::mutex> lock(muxQueue);
		return Queue.size();
	}

	// Clears Queue
	void clear()
	{
		std::unique_lock<std::mutex> lock(muxQueue);
		while (!Queue.empty())
		{
			Queue.pop();
		}

	}

	void wait()
	{
		while (empty())
		{
			std::unique_lock<std::mutex> ul(muxBlocking);
			cvBlocking.wait(ul);
		}
	}

	// Overload = .
    tsqueue<T>& operator=(const tsqueue<T>& a) {
        this->NodeName = a.NodeName;
        this->NodeAddress = a.NodeAddress;
        return *this;
    }

protected:
	std::mutex muxQueue;
	std::queue<T> Queue;
	std::condition_variable cvBlocking;
	std::mutex muxBlocking;
};

std::string file2String(const std::string& filename);
std::string buffer2PrintableString(const std::string& str);
std::string buffer2PrintableString(const char* str, size_t len);
std::vector<std::string> SplitString(std::string& data, std::string delimiter);

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
#endif // _NODE_UTILS_H_
