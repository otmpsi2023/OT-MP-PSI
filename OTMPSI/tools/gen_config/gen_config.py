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
    default=2 ** 8
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
    default=50
)
parser.add_argument(
    "--intersection_threshold",
    type=int,
    help="The intersection threshold",
    default=25
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

parser.add_argument(
    "-p", "--p", type=int, help="The p value",
    default=1363206533578718249610827093048887952664410988290312252008351212295356071903453053191721401067850577953050410900317350007950525564512418215465080535085457187463795333424852613246274739018555245409052446767070874183043358454906728919810643889294518602756197187913781308017146145779262145982293852772794946820980801967072845336320893003471423526870391995484567076189396114242619729920147931642102439541511031515598645549174556231414882024308509041581359737520189271661009167139154894907145343275552463137237630502284175554502718971483405077487139660591752391907055240725155522920053093730526866930619874074638208946788140726404552618626083803103233)
parser.add_argument(
    "--p_bits", type=int, help="The number of bits in p", default=2144)

parser.add_argument(
    "--prime_factor_1",
    type=int,
    help="The first prime factor",
    default=23870916256977059453531361624695294206301491530933871368965993398489267262385692719751518695403289250228024815294557892087072686172808442392171962978712098865763355442412048326910238839664117837729118144678419718173824348660408294460045619328381847853242594624470503466830296343277532582816785615504478672667369606094958828109989057237333639717132475195383330682537206838730153477414702742285665834646869445239103049424317802806774967087098667009025873404601797626343803355926635714963789197359427914619564653248259747085900193564090613407423789426904976915673387047428916684038767360101519343024952048719665870312157)
parser.add_argument(
    "--prime_factor_2", type=int, help="The second prime factor", default=792524711141)
parser.add_argument("-q", "--q", type=int, help="The q value", default=2)
parser.add_argument("--q_power", type=int, help="The power of q", default=56)

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

buffer_size = math.ceil(args.p_bits / 8)


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
    return bf_size(n * (t - l + 1), p)


def get_number_of_hash_functions(p):
    """Calculates the number of hash functions for a Bloom filter given m and n"""
    return int(round(-math.log2(p), 0))


bloom_filter_size = get_bf_size(
    args.set_size,
    args.false_positive_rate,
    args.number_of_parties,
    args.intersection_threshold)

number_of_hash_functions = get_number_of_hash_functions(args.false_positive_rate)

# hash functions
murmurhash_seeds = [random.randint(2, INT_MAX)
                    for _ in range(number_of_hash_functions)]

alpha = random.randint(2, args.p)
while not is_generator(alpha, args.p, [args.q, args.prime_factor_1, args.prime_factor_2, 2]):
    alpha = random.randint(2, args.p)

party_list = []
for i in range(1, args.number_of_parties + 1):
    party_list += ["P" + str(i)]

pp_list = [str(args.prime_factor_1), str(args.prime_factor_2), str(args.q)]
if args.q != 2:
    pp_list += [str(2)]

# Check if the no_print argument is set
if not args.no_print:
    # Access and print the values of the arguments
    print(f"The set size is: {args.set_size}")
    print(f"The false positive rate is: {args.false_positive_rate}")
    print(f"The number of parties is: {args.number_of_parties}")
    print(f"The intersection threshold is: {args.intersection_threshold}")
    print(f"The number of benchmark rounds is: {args.benchmark_rounds}")
    print(f"The server port starts from: {args.server_port}")
    print(f"The q value is: {args.q}")
    print(f"The power of q is: {args.q_power}")
    print(f"The number of bits in p is: {args.p_bits}")
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
    "murmurhashSeeds": murmurhash_seeds,
    "isServer": False,
    "port": 20081,
    "localName": "",
    "serverAddress": "",
    "rightNeighborAddress": "",
    "allParties": party_list,
    "p": str(args.p),
    "phiPPrimeFactors": pp_list,
    "q": str(args.q),
    "qPower": str(args.q_power),
    "alpha": str(alpha),
    "bufferSize": buffer_size
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
