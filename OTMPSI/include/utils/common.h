#ifndef OTMPSI_UTILS_COMMON_H_
#define OTMPSI_UTILS_COMMON_H_

#include <NTL/ZZ.h>

#include <string>
#include <vector>

#include "third_party/nlohmann/json.hpp"

// Define some commonly used types
typedef unsigned char uint8; // 8 bit unsigned integer
typedef unsigned int uint32; // 32 bit unsigned integer
typedef unsigned long uint64; // 64 bit unsigned integer

// Define the type of elements in the set
typedef uint32 ElementType;
// Define the type of container sizes
typedef uint64 ContainerSizeType;

// Define the maximum value for the element type
const ElementType elementTypeMax = UINT32_MAX;
// Define the maximum value for the container size type
const ContainerSizeType containerSizeTypeMax = UINT64_MAX;

// Define the number of words in an element type
const int elementTypeWords = 4;

// Enum for the role of a party in the protocol
enum Role {
    client = 0,
    server = 1,
};

// Struct for storing options for the protocol
struct Options {
    uint32 num_parties; // number of parties
    uint32 intersection_threshold; // intersection_threshold for intersection elements_

    uint8 num_hash_functions; // number of hash functions
    std::vector<uint32> murmurhash_seeds; // murmurhash seeds

    ContainerSizeType bloom_filter_size; // size of Bloom Filter.

    Role role; // client or server
    uint32 port; // server listening port
    std::string local_name; // local_name
    std::string server_address; // address of head
    std::string right_neighbor_address; // address of right neighbor on the ring
    std::vector<std::string> party_list; // all parties' name
    uint32 num_bytes_field_numbers; // number of bytes for numbers belongs to prime field p_

    NTL::ZZ p; // large prime p_, 1024 bits. p_-1 also needs to have large prime factor
    NTL::ZZ q; // small prime q.
    NTL::ZZ power_q; // Power of q in p_-1.
    NTL::ZZ alpha; // alpha

    std::vector<NTL::ZZ> phi_p_prime_factor_list; // phi(p_) factors
};

// Struct for storing experiment configuration
struct ExperimentConfig {
    ContainerSizeType element_set_size;
    ContainerSizeType num_same_items;
    uint32 same_item_seed;
    uint32 diff_item_seed;
    uint32 benchmark_rounds;

    Options options;
};

#endif // OTMPSI_UTILS_COMMON_H_
