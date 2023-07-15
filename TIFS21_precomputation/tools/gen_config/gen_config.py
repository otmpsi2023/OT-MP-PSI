import argparse
import json
import math
import os
import random

# Create the parser
parser = argparse.ArgumentParser(description="haha")

# Add the arguments
parser.add_argument(
    "--set_size",
    type=int,
    help="The size of the set",
    default=2 ** 4
)
parser.add_argument(
    "--false_positive_rate",
    type=float,
    help="The false positive rate",
    default=0.01
)
parser.add_argument(
    "--number_of_parties",
    type=int,
    help="The number of parties",
    default=6
)
parser.add_argument(
    "--intersection_threshold",
    type=int,
    help="The intersection threshold",
    default=2
)
parser.add_argument(
    "--benchmark_rounds",
    type=int,
    help="The number of benchmark rounds",
    default=50
)

parser.add_argument(
    "--server_port",
    type=int,
    help="The server port starts from",
    default=20081
)

# Argument to control whether or not to print the values of the arguments
parser.add_argument("--no_print", action="store_true", help="Do not print to output")

# Parse the arguments
args = parser.parse_args()

# Parameters
INT_MAX = 2147483647

# control the difference between sets
same_amount = args.set_size // 3
diff_step = max(args.set_size // (args.number_of_parties), 2)
same_seed = random.randint(1, INT_MAX)
keys_seed = random.randint(1, INT_MAX)

buffer_size = math.ceil(2048 / 8) * 2 + 1


# helper functions


def pow_mod(a, k, n):
    """Calculates a^k mod n"""
    b = 1
    while b <= k:
        b = b << 1
    b = b >> 1
    tmp = 1
    i = 0
    while b:
        tmp = (tmp * tmp) % n
        if k & b:
            tmp = (tmp * a) % n
        b = b >> 1
        i += 1
    return tmp


def is_generator(g, p, factors):
    """Checks if g is a generator for p given its prime factors"""
    for f in factors:
        if pow_mod(g, (p - 1) // f, p) == 1:
            return False
    return True


def bf_size(n, p):
    """Calculates the size of a Bloom filter given n and p"""
    m = -(n * math.log(p)) / (math.log(2) ** 2)
    return math.ceil(m)


def get_bf_size(n, p, t, l):
    """Calculates the size of a Bloom filter given n, p, t and l"""
    return bf_size(n * (t - l), p)


def get_number_of_hash_functions(p):
    """Calculates the number of hash functions for a Bloom filter given m and n"""
    return int(round(-math.log2(p), 0))


bloom_filter_size = get_bf_size(
    args.set_size,
    args.false_positive_rate,
    args.number_of_parties,
    args.intersection_threshold)

number_of_hash_functions = get_number_of_hash_functions(args.false_positive_rate)


party_list = []
for i in range(1, args.number_of_parties + 1):
    party_list += ["P" + str(i)]

    
    
# Check if the no_print argument is set
if not args.no_print:
    # Access and print the values of the arguments
    print(f"The set size is: {args.set_size}")
    print(f"The false positive rate is: {args.false_positive_rate}")
    print(f"The number of parties is: {args.number_of_parties}")
    print(f"The intersection threshold is: {args.intersection_threshold}")
    print(f"The number of benchmark rounds is: {args.benchmark_rounds}")
    print(f"The server port starts from: {args.server_port}")
    print(f"The size of bloom filter is: {bloom_filter_size}")
    print(f"The number of hash functions is: {number_of_hash_functions}")

config = {
    "setSize": args.set_size,
    "bloomFilterSize": bloom_filter_size,
    "sameNum": args.set_size,
    "sameSeed": same_seed,
    "diffSeed": 0,
    "numberOfParties": args.number_of_parties,
    "threshold": args.intersection_threshold,
    "benchmarkRounds": args.benchmark_rounds,
    "numberOfHashFunctions": number_of_hash_functions,
    "isServer": False,
    "port": 20081,
    "localName": "",
    "serverAddress": "",
    "rightNeighborAddress": "",
    "allParties": party_list,
    "bufferSize": buffer_size,
    "keysSeed": keys_seed,
    "index": 0
}

# clean the dir
dir = './config'
for f in os.listdir(dir):
    os.remove(os.path.join(dir, f))

# compose the config JSON and write to file
diffSeeds = []
for i in range(1, args.number_of_parties + 1):
    config["sameNum"] = max([args.set_size - (i - 1) * diff_step, same_amount])
    diffSeed = random.randint(2, INT_MAX)
    while (diffSeed in diffSeeds):
        diffSeed = random.randint(2, INT_MAX)
    config["diffSeed"] = diffSeed
    diffSeeds += [diffSeed]

    if i == 1:
        config["isServer"] = True
    else:
        config["isServer"] = False

    config["port"] = args.server_port + i - 1
    config["localName"] = "P" + str(i)
    config["serverAddress"] = "127.0.0.1:" + str(args.server_port)
    config["rightNeighborAddress"] = "127.0.0.1:" + \
                                     str(args.server_port + (i) % (args.number_of_parties))
    config["index"] = i

    json_object = json.dumps(config, indent=4)
    file = os.path.abspath("config/P" + str(i) + "_config.json")
    
    if not args.no_print:
        print("write to ", file)

    with open(file, 'w') as outfile:
        outfile.write(json_object)

bScript = "#! /bin/bash \n"
for i in range(2, args.number_of_parties + 1):
    bScript += "./bin/main ./config/P" + \
               str(i) + "_config.json & \n"
bScript += "./bin/main ./config/P1_config.json"
file = os.path.abspath("./tools/run.sh")
with open(file, 'w') as outfile:
    outfile.write(bScript)
    
if not args.no_print:
    print("write to ", file)

bScript = "#! /bin/bash \n"
for i in range(2, args.number_of_parties + 1):
    bScript += "./bin/benchmark ./config/P" + \
               str(i) + "_config.json & \n"
bScript += "./bin/benchmark ./config/P1_config.json"
file = os.path.abspath("./tools/benchmark/benchmark.sh")
with open(file, 'w') as outfile:
    outfile.write(bScript)
    
if not args.no_print:
    print("write to ", file)
