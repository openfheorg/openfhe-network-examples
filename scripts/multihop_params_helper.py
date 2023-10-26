# @file multihop_params_helper.py: Header for the standard values for Lattice Parms, as
# determined by homomorphicencryption.org
# @author TPOC: contact@palisade-crypto.org

# The functions are written to choose the minimum ciphertext modulus size qk and the maximum 
# number of hops d supported by each PRE scheme

# 1. Functions with '_prev' refers to the PRE protocol without rerandomization or any noise flooding or fixed larger noise
# 2. Functions with '_noiseflooding' refers to the PRE protocol with rerandomization and fixed 20 bits noise flooding
# 3. Functions with '_noiseflooding_ps' refers to the PRE protocol with rerandomization and provably secure noise flooding

import math

    
sigma_e = 3.2 #standard deviation of the discrete gaussian distribution used in the PRE scheme.

# B_e is the bound for the error B_E = sqrt(alpha)*sigma_e where alpha is determined based on decryption failure rate erfc(alpha) probability of coefficient exceeding the bound B_e)
alpha = 6
B_e = alpha*sigma_e 

numqueries = 1#1024 #1048576#2^20 1099511627776 #2^40


#number of digits for hybrid key switching
dnum = 3
def find_qk_indcpa(d,n,p,r, lwl_k = 1, upl_k = 60):
    k = -1
    for i in range(lwl_k,upl_k):
        if 2**i > 6*math.sqrt(n)*p*B_e*(1+d*(1+((2**r - 1)*((i/r)+1)))) + 2*d*p*B_e:
            k = i
            break
    if (k == -1):
        print("Cannot find q bits <= 60!!!")
        quit()
    #print("q bits")
    #print(k)
    return k


def find_d_indcpa(k,n,p,r, lwl_d = 1, upl_d= 2**60):
    max_d = 0
    set_d=0
    for i in range(lwl_d,upl_d):
        test = 6*math.sqrt(n)*p*B_e*(1+i*(1+((2**r - 1)*((k/r)+1)))) + 2*i*p*B_e
        if 2**k <= test:
            #print(2**k, " <? ", test)
            set_d=1
            max_d = i
            break

    #print("d")    
    #print(max_d)
    if((set_d==0) and (max_d==0)):
        max_d = upl_d
    return max_d
    
def find_qk_fixednoise(d,n,p,r, lwl_k = 1, upl_k = 60):
    k = -1
    for i in range(lwl_k,upl_k):
        if 2**i > 6*math.sqrt(n)*p*B_e*(1+d*(1+((2**r - 1)*((i/r)+1))))+2*d*alpha*(2**20)*p:
            k = i
            break
    if (k == -1):
        print("Cannot find q bits <= 60!!!")
        quit()
    #print("q bits")
    #print(k)
    return k


def find_d_fixednoise(k,n,p,r, lwl_d = 1, upl_d= 2**60):
    max_d = 0
    set_d=0
    for i in range(lwl_d,upl_d):
        test = 6*math.sqrt(n)*p*B_e*(1+i*(1+((2**r - 1)*((k/r)+1))))+2*i*alpha*(2**20)*p
        if 2**k <= test:
            #print(2**k, " <? ", test)
            set_d=1
            max_d = i
            break

    #print("d")    
    #print(max_d)
    if((set_d==0) and (max_d==0)):
        max_d = upl_d
    return max_d

def find_qk_noiseflooding_ps(d,n,p,r, lwl_k = 1, upl_k = 60, stat_sec = 30):
    print ("number of queries: ", numqueries)
    k = -1
    for i in range(lwl_k,upl_k):
        test = 6*math.sqrt(n)*p*B_e*(1+d*(1+((2**r - 1)*((i/r)+1)))) + 2*p*((2**(stat_sec*d)*math.sqrt(24*numqueries))*3*(d+1)*(i/r)*math.sqrt(n)*((2**r)-1)*B_e)
        #test = 2*math.sqrt(n)*p*B_e*(3+d*(1+3*(2**r - 1)*((i/r)+1)))+4*((2**(stat_sec*d)*math.sqrt(24*numqueries))*3*(i/r)*math.sqrt(n)*((2**r)-1)*B_e)*math.sqrt(n)*p
        print("noise flooding bits in find_qk: ", test)
        if 2**i > test:
            k = i
            break
    if (k == -1):
        print("Cannot find q bits <= 60!!!")
        quit()
    #print("q bits")
    #print(k)
    return k
    
def find_d_noiseflooding_ps(k,n,p,r, lwl_d = 1, upl_d= 2**20, stat_sec = 30):
    print ("number of queries: ", numqueries)
    max_d = 0
    set_d=0
    for i in range(lwl_d,upl_d):
        test = 6*math.sqrt(n)*p*B_e*(1+i*(1+((2**r - 1)*((k/r)+1)))) + + 2*p*((2**(stat_sec*i)*math.sqrt(24*numqueries))*3*(i+1)*(k/r)*math.sqrt(n)*((2**r)-1)*B_e)
        #test = 2*math.sqrt(n)*p*B_e*(3+i*(1+3*(2**r - 1)*((k/r)+1)))+4*((2**(stat_sec*i)*math.sqrt(24*numqueries))*3*(k/r)*math.sqrt(n)*((2**r)-1)*B_e)*math.sqrt(n)*p
        print("noise flooding bits in find_d loop: ", test)
        if 2**k <= test:
            #print(2**k, " <? ", test)
            set_d=1
            max_d = i
            break

    #print("d")    
    #print(max_d)
    if((set_d==0) and (max_d==0)):
        max_d = upl_d
    return max_d

def find_qk_noiseflooding_hybrid_ps(d,n,p,r, lwl_k = 1, upl_k = 60, stat_sec = 30):
    print ("number of queries: ", numqueries)
    k = -1
    alpha_digit_size = math.ceil((d + 1)/dnum)
    for i in range(lwl_k,upl_k):
        if 2**i > 6*math.sqrt(n)*p*B_e*(1+d*(1+((2**r - 1)*((i/r)+1)))) + 2*d*alpha_digit_size*(1+math.sqrt(n)) + 2*p*(math.sqrt(24*numqueries))*(2**(stat_sec*d)*alpha_digit_size*(dnum*math.sqrt(n)*B_e + 1 + math.sqrt(n))):
        #if 2**i > 2*math.sqrt(n)*p*B_e*(3+d*(1+3*alpha_digit_size*dnum))+2*alpha_digit_size*(1+math.sqrt(n)) + 8*(math.sqrt(24*numqueries))*2**(stat_sec*d)*(alpha_digit_size*(dnum*math.sqrt(n)*B_e + 1 + math.sqrt(n))):
            k = i
            break
    if (k == -1):
        print("Cannot find q bits <= 60!!!")
        quit()
    #print("q bits")
    #print(k)
    return k
    
def find_d_noiseflooding_hybrid_ps(k,n,p,r, lwl_d = 1, upl_d= 2**20, stat_sec = 30):
    print ("number of queries: ", numqueries)
    max_d = 0
    set_d=0
    for i in range(lwl_d,upl_d):
        alpha_digit_size = math.ceil((i + 1)/dnum)
        test = 6*math.sqrt(n)*p*B_e*(1+i*(1+((2**r - 1)*((k/r)+1)))) + 2*i*alpha_digit_size*(1+math.sqrt(n)) + 2*p*(math.sqrt(24*numqueries))*(2**(stat_sec*i)*alpha_digit_size*(dnum*math.sqrt(n)*B_e + 1 + math.sqrt(n)))
        #test = 2*math.sqrt(n)*p*B_e*(3+i*(1+3*alpha_digit_size*dnum))+2*alpha_digit_size*(1+math.sqrt(n)) + 8*(math.sqrt(24*numqueries))*2**(stat_sec*i)*(alpha_digit_size*(dnum*math.sqrt(n)*B_e + 1 + math.sqrt(n)))
        if 2**k <= test:
            #print(2**k, " <? ", test)
            set_d=1
            max_d = i
            break

    #print("d")    
    #print(max_d)
    if((set_d==0) and (max_d==0)):
        max_d = upl_d
    return max_d

def find_qk_dvr_ps(d,n,p,r, lwl_k = 1, upl_k = 60, circuit_privacy = 30):
    ki = -1
    k0 = 0

    print("***** n: ", n)
    print("***** d: ", d)
    
    for j in range(1,60):
        if 2**j > p*(2*math.sqrt(n) + 1):
            k0 = j
            break
            
    for i in range(1,60):
        if 2**i > 6*(2**circuit_privacy)*p*math.sqrt(n)*n*B_e:
            ki = i
            break
    if (ki == -1):
        print("Cannot find q bits <= 60!!!")
        quit()
    

                    
    k = k0 + d*ki
    
    print("************ ki: ", ki)
    print("************ k: ", k)
    print("************ k0: ", k0)
    print("************ d: ", d)
    
    return k
    
def find_d_dvr_ps(k,n,p,r, lwl_d = 1, upl_d= 2**20, circuit_privacy = 30):
    max_d = 0
    set_d=0
    
    print("##### n: ", n)

    for i in range(1,60):
        if 2**i > p*(2*math.sqrt(n) + 1):
            k0 = i
            break
            
    for i in range(lwl_d,upl_d):
        ki = (k - k0)/i
        print("############ ki: ", ki)
        print("############ k: ", k)
        print("############ k0: ", k0)
        print("############ d: ", i)
        
        test = 6*p*math.sqrt(n)*n*(2**circuit_privacy)*B_e
        print("############ test: ", math.log2(test))
        print("############ is d set: ", set_d)
        if 2**ki <= test:
            #print(2**k, " <? ", test)
            set_d=1
            max_d = i
            break


    if((set_d==0) and (max_d==0)):
        max_d = upl_d
    return max_d

def find_qk_trapdoor_ps(d,n,p,r, lwl_k = 2, upl_k = 60, stat_sec = 128):
    print ("number of queries: ", numqueries)
    k = -1
    for i in range(lwl_k,upl_k):
        rsigma = math.sqrt(2/math.pi)*(2**r)*math.sqrt(math.log(2*n*(i/r)) + stat_sec) # what's the norm(e) here in lemma 3.7?
        rtilde = math.sqrt(n*(i/r))*B_e*rsigma
        test = 6*math.sqrt(n)*p*B_e + 2*d*p*math.sqrt(n)*rtilde*(rsigma+B_e)
        print("noise flooding bits in find_qk: ", math.log2(test))
        if 2**i > test:
            k = i
            break
    if (k == -1):
        print("Cannot find q bits <= 60!!!")
        quit()
    #print("q bits")
    #print(k)
    return k
    
def find_d_trapdoor_ps(k,n,p,r, lwl_d = 1, upl_d= 2**20, stat_sec = 128):
    print ("number of queries: ", numqueries)
    max_d = 0
    set_d=0
    for i in range(lwl_d,upl_d):
        rsigma = math.sqrt(2/math.pi)*(2**r)*math.sqrt(math.log(2*n*(k/r)) + stat_sec) # what's the norm(e) here in lemma 3.7?
        rtilde = math.sqrt(n*(k/r))*B_e*rsigma
        test = 6*math.sqrt(n)*p*B_e + 2*i*p*math.sqrt(n)*rtilde*(rsigma+B_e)
        print("noise flooding bits in find_d loop: ", math.log2(test))
        if 2**k <= test:
            #print(2**k, " <? ", test)
            set_d=1
            max_d = i
            break

    #print("d")    
    #print(max_d)
    if((set_d==0) and (max_d==0)):
        max_d = upl_d
    return max_d

def isPowerOfTwo(n):
    return (math.ceil(Log2(n)) == math.floor(Log2(n)));

def test_range(val, low, hi):
    if val in range(low, hi+1):
        return
    else:
        msg = f"input not in valid range ({low} - {hi})"
        raise Exception(msg)

def test_in(val, bucket):
    if val in bucket:
        return
    else:
        msg = f"input not in valid set {bucket}"
        raise Exception(msg)

