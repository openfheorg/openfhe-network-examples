#!/usr/bin/python

'''Approach for determining parameters for multihop
1)	Pick security level
2)	Set p=2
3)	Specify minimum ringsize (payload size is ringsize bits max)
4)	Select a minimum #hops (d)
5)	Select a minimum depth of computation (assume 0 for initial work). 
6)	Adjust r, to satisfy (3) and (4)
Measure enc/rec/dec time, and throughput ( #bits in (3+/these times) and document.

In later design stages, when we care about optimizing performance, we
may decide to adjust these in order to maximize some design metric
(such as throughput etc.)

Keep in mind various payload sizes: 
Aes key very small (128-256 etc encrypted as bits). 
Typical wireless packet size ( 1500 bits) .

1. Functions with '_indcpa' refers to the PRE protocol without rerandomization or any noise flooding or fixed larger noise
2. Functions with '_noiseflooding' refers to the PRE protocol with rerandomization and fixed 20 bits noise flooding
3. Functions with '_noiseflooding_ps' refers to the PRE protocol with rerandomization and provably secure noise flooding
'''

import stdlatticeparams as stdlat
import multihop_params_helper as helperfncs
import math

import sys

mode = sys.argv[1]
digit_optimize = eval(sys.argv[2])
assert isinstance(digit_optimize, bool), TypeError('second argument must be boolean to indicate optimizing digit size')


print("parameters for mode: ", mode)
print("optimimze for digit size: ", digit_optimize)

# the function parameter_selector takes as input the mode of the PRE scheme ('indcpa', 'fixednf', 'psnf') and boolean to indicate if the relinearization window r needs to be optimized. It is set to True by default. Setting it to False sets r value to 1.

def parameter_selector(scheme, optimize_r=1):
    print("Multihop parameter selector. (Note inputs are not range verified)")
    dist_type = int(input("Enter Distribution (0 = HEStd_uniform, 1 = HEStd_error, 2 = HEStd_ternary): "))
    helperfncs.test_range(dist_type, 0, 2)

    sec_level = int(input("Enter Security level (0 = 128, 1 = 192, 2 = 256): "))
    helperfncs.test_range(sec_level, 0, 2)

    payload_bits = int(input("Enter payload bits: "))

    p = int(input("Enter p (2,4,8,16,256,65536 smaller is faster): "))
    helperfncs.test_in(p, [2,4,8,16,256,512,1024,4096,65536])

    min_hops = int(input("Enter minimum # hops: "))
    if min_hops < 1:
        raise Exception("input must be at least 1")    

    #not used right now in the parameter generation
    min_depth = int(input("Enter minimum multiplicative depth: "))
    if min_depth < 0:
        raise Exception("input must be positive")    

    '''
    #hardwire these for development
    dist_type = 2
    sec_level = 1
    payload_bits = 256
    p = 2
    min_hops = 8
    min_depth = 0
    '''

    dist_type = stdlat.DistType(dist_type)
    sec_level = stdlat.SecLev(sec_level)

    #print (sec_level)


    min_ringsize = int(payload_bits/math.log2(p))
    print("Minimum ringsize required = ", min_ringsize)

    for ringsize in [1024, 2048, 4096, 8192, 16384, 32768]:
        if min_ringsize <= ringsize:
            break
    if min_ringsize>ringsize:
        print("Warning, cannot set ringsize to ", min_ringsize, " with standard security")
    print("ringsize set to ", ringsize)



    while ringsize <= 32768:
        set_r=0
        set_qk=0
        print("---\nadjusting parameters for ringsize: ", ringsize)

        logQ = stdlat.LogQ[(dist_type, ringsize, sec_level)]

        print("Security requires logQ = ",logQ, " ringsize = ", ringsize);

        #we now adjust r to satisfy the above. start at ceil(logQ/3) and try lower
        #in a loop

        if(optimize_r):
            for this_r in range(math.ceil(logQ/3.0), 0, -1):
            	if (logQ%this_r == 0):
	                if(scheme=="indcpa"):
        	            this_d = helperfncs.find_d_indcpa(logQ, ringsize, p, this_r)
        	        elif(scheme=="fixednf"):
        	            this_d = helperfncs.find_d_fixednoise(logQ, ringsize, p, this_r)
        	        elif(scheme=="psnf"):
        	            this_d = helperfncs.find_d_noiseflooding_ps(logQ, ringsize, p, this_r)
        	        elif(scheme=="hybridpsnf"):
        	            this_d = helperfncs.find_d_noiseflooding_hybrid_ps(logQ, ringsize, p, this_r)
        	        elif(scheme=="dvr_psnf"):
        	            this_d = helperfncs.find_d_dvr_ps(logQ, ringsize, p, this_r)
        	        elif(scheme=="trapdoor_psnf"):
        	            this_d = helperfncs.find_d_trapdoor_ps(logQ, ringsize, p, this_r)
        	                     
        	        if (this_d >= min_hops):
        	            r = this_r
        	            set_r=1
        	            print("resulting r ", r, "d ", this_d, " satisfies min hops, stopping")
        	            break

        else:
            this_r=1
            if(scheme=="indcpa"):
                this_d = helperfncs.find_d_indcpa(logQ, ringsize, p, this_r)
            elif(scheme=="fixednf"):
                this_d = helperfncs.find_d_fixednoise(logQ, ringsize, p, this_r)
            elif(scheme=="psnf"):
                this_d = helperfncs.find_d_noiseflooding_ps(logQ, ringsize, p, this_r)
            elif(scheme=="hybridpsnf"):
                this_d = helperfncs.find_d_noiseflooding_hybrid_ps(logQ, ringsize, p, this_r)
            elif(scheme=="dvr_psnf"):
                this_d = helperfncs.find_d_dvr_ps(logQ, ringsize, p, this_r)
            elif(scheme=="trapdoor_psnf"):
                this_d = helperfncs.find_d_trapdoor_ps(logQ, ringsize, p, this_r)
                                                    
            if (this_d >= min_hops):
                r = this_r
                set_r=1
                print("resulting r ", r, "d ", this_d, " satisfies min hops, stopping")
        


        #verify Q is ok
        if(set_r==1):
            if(scheme=="indcpa"):
                set_qk=1
                qk = helperfncs.find_qk_indcpa(min_hops, ringsize, p, r, lwl_k = logQ, upl_k = 60)
            elif(scheme=="fixednf"):
                set_qk=1
                qk = helperfncs.find_qk_fixednoise(min_hops, ringsize, p, r, lwl_k = logQ, upl_k = 476)
            elif(scheme=="psnf"):
                set_qk=1
                qk = helperfncs.find_qk_noiseflooding_ps(min_hops, ringsize, p, r, lwl_k = logQ, upl_k = 476)
            elif(scheme=="hybridpsnf"):
                set_qk=1
                qk = helperfncs.find_qk_noiseflooding_hybrid_ps(min_hops, ringsize, p, r, lwl_k = logQ, upl_k = 476)
            elif(scheme=="dvr_psnf"):
                set_qk=1
                qk = helperfncs.find_qk_dvr_ps(min_hops, ringsize, p, r, lwl_k = logQ, upl_k = 476)
            elif(scheme=="trapdoor_psnf"):
                set_qk=1
                qk = helperfncs.find_qk_trapdoor_ps(min_hops, ringsize, p, r, lwl_k = logQ, upl_k = 476)
	
        if(set_qk==1):
            if (qk <= logQ):
                print("resulting qk: ", qk, " <= logQ: ",logQ, "  satisfies security")
                break

            # if we get here qk is too big we need to move to a larger ring size
            print("resulting qk: ", qk, " > logQ: ",logQ, " does not satisfy security")
            print("increasing ringsize in an attempt to satisfy security")
        ringsize = ringsize *2

    if(scheme=="indcpa"):
        max_hops = helperfncs.find_d_indcpa(logQ, ringsize, p, r) - 1
    elif(scheme=="fixednf"):
        max_hops = helperfncs.find_d_fixednoise(logQ, ringsize, p, r) - 1
    elif(scheme=="psnf"):
        max_hops = helperfncs.find_d_noiseflooding_ps(logQ, ringsize, p, r) - 1
    elif(scheme=="hybridpsnf"):
        max_hops = helperfncs.find_d_noiseflooding_hybrid_ps(logQ, ringsize, p, r) - 1
    elif(scheme=="dvr_psnf"):
        max_hops = helperfncs.find_d_dvr_ps(logQ, ringsize, p, r) - 1
    elif(scheme=="trapdoor_psnf"):
        max_hops = helperfncs.find_d_trapdoor_ps(logQ, ringsize, p, r) - 1
        
    print("final parameters")
    print("dist_type: ",dist_type)
    print("sec_level: ", sec_level)
    print("ringsize: ", ringsize)
    print("requested payload_bits: ", payload_bits)
    print("max payload_bits: ", ringsize*math.log2(p))
    print("p: ", p)
    print("min_hops: ", min_hops)
    print("max_hops (upto 2^60): ", max_hops)
    print("min_depth: ", min_depth)
    print("ringsize: ", ringsize)
    print("r: ", r)
    print("logQ: ", logQ)
    print("logQ/r: %4.1f" % (logQ/r))
    

#example call to function parameter_selector
#parameter_selector("indcpa", False)
#parameter_selector("fixednf", False)
#parameter_selector("psnf", False)
#parameter_selector("hybridpsnf", False)
#parameter_selector("dvr_psnf", False)
#parameter_selector("trapdoor_psnf", True)
parameter_selector(mode, digit_optimize)
