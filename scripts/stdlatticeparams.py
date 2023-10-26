# @file stdlatticeparms.h: Header for the standard values for Lattice Parms, as
# determined by homomorphicencryption.org
# @author TPOC: contact@palisade-crypto.org

# this is the representation of the standard lattice parameters defined in the
# Homomorphic Encryption Standard, as defined by
# http://homomorphicencryption.org

# given a distribution type and a security level, you can get the maxQ for a
# given ring dimension, and you can get the ring dimension given a maxQ

# The code below is very specific to the layout of the DistributionType and
# SecurityLevel enums IF you change them, go look at and change byRing and
# byLogQ

from enum import Enum

class DistType(Enum):
    HEStd_uniform = 0
    HEStd_error = 1
    HEStd_ternary = 2


class SecLev(Enum):
    HEStd_128_classic = 0
    HEStd_192_classic = 1
    HEStd_256_classic = 2
    HEStd_NotSet = 3


LogQ = { }

LogQ[(DistType.HEStd_uniform, 1024, SecLev.HEStd_128_classic)] = 29
LogQ[(DistType.HEStd_uniform, 1024, SecLev.HEStd_192_classic)] = 21
LogQ[(DistType.HEStd_uniform, 1024, SecLev.HEStd_256_classic)] = 16
LogQ[(DistType.HEStd_uniform, 2048, SecLev.HEStd_128_classic)] = 56
LogQ[(DistType.HEStd_uniform, 2048, SecLev.HEStd_192_classic)] = 39
LogQ[(DistType.HEStd_uniform, 2048, SecLev.HEStd_256_classic)] = 31
LogQ[(DistType.HEStd_uniform, 4096, SecLev.HEStd_128_classic)] = 111
LogQ[(DistType.HEStd_uniform, 4096, SecLev.HEStd_192_classic)] = 77
LogQ[(DistType.HEStd_uniform, 4096, SecLev.HEStd_256_classic)] = 60
LogQ[(DistType.HEStd_uniform, 8192, SecLev.HEStd_128_classic)] = 220
LogQ[(DistType.HEStd_uniform, 8192, SecLev.HEStd_192_classic)] = 154
LogQ[(DistType.HEStd_uniform, 8192, SecLev.HEStd_256_classic)] = 120
LogQ[(DistType.HEStd_uniform, 16384, SecLev.HEStd_128_classic)] = 440
LogQ[(DistType.HEStd_uniform, 16384, SecLev.HEStd_192_classic)] = 307
LogQ[(DistType.HEStd_uniform, 16384, SecLev.HEStd_256_classic)] = 239
LogQ[(DistType.HEStd_uniform, 32768, SecLev.HEStd_128_classic)] = 880
LogQ[(DistType.HEStd_uniform, 32768, SecLev.HEStd_192_classic)] = 612
LogQ[(DistType.HEStd_uniform, 32768, SecLev.HEStd_256_classic)] = 478

LogQ[(DistType.HEStd_error, 1024, SecLev.HEStd_128_classic)] = 29
LogQ[(DistType.HEStd_error, 1024, SecLev.HEStd_192_classic)] = 21
LogQ[(DistType.HEStd_error, 1024, SecLev.HEStd_256_classic)] = 16
LogQ[(DistType.HEStd_error, 2048, SecLev.HEStd_128_classic)] = 56
LogQ[(DistType.HEStd_error, 2048, SecLev.HEStd_192_classic)] = 39
LogQ[(DistType.HEStd_error, 2048, SecLev.HEStd_256_classic)] = 31
LogQ[(DistType.HEStd_error, 4096, SecLev.HEStd_128_classic)] = 111
LogQ[(DistType.HEStd_error, 4096, SecLev.HEStd_192_classic)] = 77
LogQ[(DistType.HEStd_error, 4096, SecLev.HEStd_256_classic)] = 60
LogQ[(DistType.HEStd_error, 8192, SecLev.HEStd_128_classic)] = 220
LogQ[(DistType.HEStd_error, 8192, SecLev.HEStd_192_classic)] = 154
LogQ[(DistType.HEStd_error, 8192, SecLev.HEStd_256_classic)] = 120
LogQ[(DistType.HEStd_error, 16384, SecLev.HEStd_128_classic)] = 440
LogQ[(DistType.HEStd_error, 16384, SecLev.HEStd_192_classic)] = 307
LogQ[(DistType.HEStd_error, 16384, SecLev.HEStd_256_classic)] = 239
LogQ[(DistType.HEStd_error, 32768, SecLev.HEStd_128_classic)] = 883
LogQ[(DistType.HEStd_error, 32768, SecLev.HEStd_192_classic)] = 613
LogQ[(DistType.HEStd_error, 32768, SecLev.HEStd_256_classic)] = 478

LogQ[(DistType.HEStd_ternary, 1024, SecLev.HEStd_128_classic)] = 27
LogQ[(DistType.HEStd_ternary, 1024, SecLev.HEStd_192_classic)] = 19
LogQ[(DistType.HEStd_ternary, 1024, SecLev.HEStd_256_classic)] = 14
LogQ[(DistType.HEStd_ternary, 2048, SecLev.HEStd_128_classic)] = 54
LogQ[(DistType.HEStd_ternary, 2048, SecLev.HEStd_192_classic)] = 37
LogQ[(DistType.HEStd_ternary, 2048, SecLev.HEStd_256_classic)] = 29
LogQ[(DistType.HEStd_ternary, 4096, SecLev.HEStd_128_classic)] = 109
LogQ[(DistType.HEStd_ternary, 4096, SecLev.HEStd_192_classic)] = 75
LogQ[(DistType.HEStd_ternary, 4096, SecLev.HEStd_256_classic)] = 58
LogQ[(DistType.HEStd_ternary, 8192, SecLev.HEStd_128_classic)] = 218
LogQ[(DistType.HEStd_ternary, 8192, SecLev.HEStd_192_classic)] = 152
LogQ[(DistType.HEStd_ternary, 8192, SecLev.HEStd_256_classic)] = 118
LogQ[(DistType.HEStd_ternary, 16384, SecLev.HEStd_128_classic)] = 438
LogQ[(DistType.HEStd_ternary, 16384, SecLev.HEStd_192_classic)] = 305
LogQ[(DistType.HEStd_ternary, 16384, SecLev.HEStd_256_classic)] = 237
LogQ[(DistType.HEStd_ternary, 32768, SecLev.HEStd_128_classic)] = 881
LogQ[(DistType.HEStd_ternary, 32768, SecLev.HEStd_192_classic)] = 611
LogQ[(DistType.HEStd_ternary, 32768, SecLev.HEStd_256_classic)] = 476

