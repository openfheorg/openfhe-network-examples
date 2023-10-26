// @file register_functions.h
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
> initialize_map -- create a vector map of virtual vectors (that will be used to accumulate data 
from the nodes along a path)
> get_offset -- get the offset for a virtual vector 
> get_length -- get the length of a virtual vector
> write(x,i) --	write to ith register (assuming zero initial value)	vector add zeros except for the ith entry which is x.
> owrite(x,i) -- overwrite ith register (assuming non zero value) vector multiply ones except for ith entry == 0, then do vector add above.
> mul(x,i)	-- multiply ith register by constant vector multiply by ones except for ith entry == x
> add(x,i)/sub(x,i)	add/subtract ith register with constant	Identical to Write(x,i) or Write(-x,-i)
> unpackPT(pt) -- unpack the plaintext into map of virtual vectors.
*/

#ifndef __REGISTER_FUNCTIONS_H__
#define __REGISTER_FUNCTIONS_H__

#include <iostream>
#include "utils/exception.h"

//define the container for the mapping 
static std::map<std::string, std::pair<int,int>> map2vector;

void initialize_map(unsigned int ringsize) {
       //create vector map for virtual vectors
	//build the mapping this is unique for each example:
	// here we are building 4 virtual vector registers.
	//number of virtual vectors can be determined as ringdimension/num_of_nodes
	//std::map<std::string, std::pair<int,int>> map2vector;

	unsigned int n_nodes = 4;   //for this example 

	//unsigned int n_elements = ringsize; //bg
	unsigned int n_elements = ringsize/2; //ckks

	unsigned int offset = 0;

	unsigned int length = n_nodes;
	map2vector.insert({"v1", {offset, length}});
	offset = offset + length;

	//use the same length for the next two vectors
	map2vector.insert({"v2", {offset, length}});
	offset = offset + length;

	map2vector.insert({"v3", {offset, length}});
	offset = offset + length;

    //now make the next vector just be length 1
    length = 1;
	map2vector.insert({"n", {offset, length}});
	offset = offset + length;

	// verify that the overall storage still fits in ringsize. 

	if (offset > n_elements) {
		std::cerr << "number of entries greater than allowed by ring dimension" << std::endl;
	    exit(EXIT_FAILURE);
	}
}

//helper function
int get_offset(std::string name, int index){
  auto itr = map2vector.find(name);  //note map2vector is either global or passed in

  if (itr == map2vector.end()){
	    std::cerr<<"cannot find register named "+name <<std::endl;
	    return -1;
  }

  //itr points to the offest and length
  auto offset = itr->second.first;
  auto length = itr->second.second;
  
  if (index >= length){ //verify you are not writing past the end of the virtual register. 
	    std::cerr<<"trying to write past end of register named "+name <<std::endl;
	    return -1;
  }  
  return offset;
}

int get_length(std::string name){
  auto itr = map2vector.find(name);  //note map2vector is either global or passed in

  if (itr == map2vector.end()){
	    std::cerr<<"cannot find register named "+name <<std::endl;
	    return -1;
  }

  //itr points to the offest and length
  auto length = itr->second.second;
  
  return length;
}

// code for write
void write(lbcrypto::Ciphertext<lbcrypto::DCRTPoly>& cipherText, lbcrypto::CryptoContext<lbcrypto::DCRTPoly>& cc, std::string name, float val, int index) {

    //note for ckks this may need to be complex, not sure how the API works nowdays
    auto offset = get_offset(name, index);
    auto length = get_length(name);

    //invec.reserve(offset+length);
    std::vector<std::complex<double>> invec(offset+length, 0.0); // start with a zero vector

    if (offset == -1){
	    std::cerr << "No offset set for " << name << std::endl;
    }  
    
    // ok it is safe to write
    invec[offset+index] = val;

    //encode th plaintext as a packed vector
    auto ptvec = cc->MakeCKKSPackedPlaintext(invec);
	
    lbcrypto::Ciphertext<lbcrypto::DCRTPoly> temp(nullptr);
	 //add it to the ciphertext
	 temp = cc->EvalAdd(cipherText, ptvec);  //note you have to make input_ct global or pass it in to the function.
	 cipherText = temp;

	 return;
}


// code for mul

void mul(lbcrypto::Ciphertext<lbcrypto::DCRTPoly>& cipherText, lbcrypto::CryptoContext<lbcrypto::DCRTPoly>& cc, std::string name, float val, int index) {

    //note for ckks this may need to be complex, not sure how the API works nowdays
    auto offset = get_offset(name, index);
    
	auto length = get_length(name);

    //invec.reserve(offset+length);
    std::vector<std::complex<double>> invec(offset+length, 1.0); // start with a ones  vector

    if (offset == -1){
	    std::cerr << "offset not found" << std::endl;
       exit(EXIT_FAILURE);
    }  

    // ok it is safe to write
    invec[offset+index] = val;

    //encode th plaintext as a packed vector
    auto ptvec = cc->MakeCKKSPackedPlaintext(invec);
	
    lbcrypto::Ciphertext<lbcrypto::DCRTPoly> temp(nullptr);
	  //add it to the ciphertext
	  temp = cc->EvalMult(cipherText, ptvec);  //note you have to make input_ct global or pass it in to the function.
	
    cipherText = temp;
	return;
}



//function for add
 void add(lbcrypto::Ciphertext<lbcrypto::DCRTPoly>& cipherText, lbcrypto::CryptoContext<lbcrypto::DCRTPoly>& cc, std::string name, float val, int index){
    write(cipherText, cc, name, val, index);
 }

 void sub(lbcrypto::Ciphertext<lbcrypto::DCRTPoly>& cipherText, lbcrypto::CryptoContext<lbcrypto::DCRTPoly>& cc, std::string name, float val, int index){
    write(cipherText, cc, name, -val, index);
 }


//function owrite
 void owrite(lbcrypto::Ciphertext<lbcrypto::DCRTPoly>& cipherText, lbcrypto::CryptoContext<lbcrypto::DCRTPoly>& cc, std::string name, float val, int index){
    mul(cipherText, cc, name, 0.0, index);
    add(cipherText, cc, name, val, index);
 }

//function unpack CT
std::map<std::string,std::vector<double>> unpackPT(std::vector<std::complex<double>> pt){
//this function unpacks the vectors in the pt.

    std::map<std::string,std::vector<double>> outmap;

    //loop over all the items in map2vector
    for (auto itr = map2vector.begin(); itr != map2vector.end(); ++itr) {
        auto name = itr->first;
        auto olpair = itr->second;

        int offset = olpair.first;
        int length = olpair.second;

        std::vector<std::complex<double>>::const_iterator first = pt.begin() + offset;
        std::vector<std::complex<double>>::const_iterator last = pt.begin() + offset + length;
        std::vector<std::complex<double>> tmp(first, last);
        std::vector<double> doubletmp; 
        for (auto itr2: tmp){
            doubletmp.push_back(itr2.real());
        }
        //vector<std::complex<double>> tmp = {pt.begin() + offset, pt.begin() + offset + length};
        // note tmp should be of size() == length
        outmap.insert({name, doubletmp});
    }
    return outmap;
}
#endif // __REGISTER_FUNCTIONS_H__