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
    default=2 ** 6
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
    default=3
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
    default=504455384717768984733637226286619865699222797429951205972064252552082111879365080803740346691344850591595525241046647455185929687333449322040690082182676510044379188427303111982959748691977639000336407181476046674714570791368924936208908525473790387671167582503992336135814449561622265386335764161936695866513924711730568437485396726107933638094000814064746298734850146621936103853804956023802489364670056336178789820357188867856717730595296389978914476274500608922252699574885933154605788729572636806598723603657147824165612546943444434944620159863659965493650238475131272866304544100894997519971305243162782127156575234583473192323480865663682264288809808003177662518507992861815947)
parser.add_argument(
    "--p_bits", type=int, help="The number of bits in p", default=2272)

parser.add_argument(
    "--prime_factor_1",
    type=int,
    help="The first prime factor",
    default=26131312350540638936043097043127290965053205224514969254770839580009454296406752876907690380088106937893175713818220232883416910377393980082243715105529817860733340738062619482830228226163387537530552469794805272351786279084791884123081386442928584265666782146692959939201416041406551686596790646895073669874283308153237203506917669247907209162602169681527545356197360911349738204224768295973794727170327435868582471742822882938530069483587405863768389190694432019048504242168389947996783455746938940160905696942747678467434150789364405787931097860317651480845599309393195925653117510243730073746046736001089051904091)
parser.add_argument(
    "--prime_factor_2", type=int, help="The second prime factor", default=5105448053)
parser.add_argument("-q", "--q", type=int, help="The q value", default=11)
parser.add_argument("--q_power", type=int, help="The power of q", default=55)

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
