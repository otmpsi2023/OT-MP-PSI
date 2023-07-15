#include "utils/bloom_filter.h"

#include "third_party/smhasher/MurmurHash3.h"


std::vector<ContainerSizeType>
GetHashPositions(const ElementType &e, ContainerSizeType size, const std::vector<uint> &murmurhash_seeds) {
    std::vector<ContainerSizeType> positions;
    positions.reserve(murmurhash_seeds.size());

    uint64 hash[2];
    // Compute multiple hash values for the element using different seeds
    for (auto &seed: murmurhash_seeds) {
        MurmurHash3_x86_128(&e, elementTypeWords, seed, hash);
        ContainerSizeType pos = hash[0] % size;
        positions.emplace_back(pos);
    }

    return positions;
}


// Method to insert an element into the Bloom filter
void BloomFilter::Insert(const ElementType &e) {
    // Set all positions to 1
    for (auto pos: GetHashPositions(e, size_, murmurhash_seeds_)) {
        bit_array_[pos] = 1;
    }
}

// Method to check if an element is in the Bloom filter
bool BloomFilter::CheckElement(const ElementType &e) {
    // Check whether all the positions are 1
    for (auto pos: GetHashPositions(e, size_, murmurhash_seeds_)) {
        if (bit_array_[pos] == 0) {
            return false;
        }
    }
    return true;
}


