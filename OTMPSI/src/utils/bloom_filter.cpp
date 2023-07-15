#include "utils/bloom_filter.h"

#include "third_party/smhasher/MurmurHash3.h"

// Method to insert an element into the Bloom filter
void BloomFilter::Insert(const ElementType &e) {
    uint64 hash[2];
    // Compute multiple hash values for the element using different seeds
    for (auto &seed: murmurhash_seeds_) {
        MurmurHash3_x86_128(&e, elementTypeWords, seed, hash);
        // Set the corresponding bit in the bit array
        uint32 pos = hash[0] % size_;
        bit_array_[pos] = 1;
    }
}

// Method to check if an element is in the Bloom filter
bool BloomFilter::CheckElement(const ElementType &e) {
    uint64 hash[2];
    // Compute multiple hash values for the element using different seeds
    for (auto &seed: murmurhash_seeds_) {
        MurmurHash3_x86_128(&e, elementTypeWords, seed, &hash);
        // Check if the corresponding bit in the bit array is set
        uint32 pos = hash[0] % size_;
        if (bit_array_[pos] == 0) {
            return false;
        }
    }
    return true;
}

// Method to insert an element into the counting Bloom filter
void CountBloomFilter::Insert(const ElementType &element) {
    uint64 hash[2];
    // Compute multiple hash values for the element using different seeds
    for (auto &seed: murmurhash_seeds_) {
        MurmurHash3_x86_128(&element, elementTypeWords, seed, &hash);
        // Increment the corresponding counter in the counter array
        uint32 pos = hash[0] % size_;
        counter_array_[pos] += 1;
    }
}

// Method to remove an element from the counting Bloom filter
void CountBloomFilter::Remove(const ElementType &element) {
    uint64 hash[2];
    // Compute multiple hash values for the element using different seeds
    for (auto &seed: murmurhash_seeds_) {
        MurmurHash3_x86_128(&element, elementTypeWords, seed, &hash);
        // Decrement the corresponding counter in the counter array
        uint32 pos = hash[0] % size_;
        counter_array_[pos] -= 1;
    }
}

// Method to check if an element is in the counting Bloom filter
uint32 CountBloomFilter::CheckElement(const ElementType &element) {
    uint32 r = INT32_MAX;
    uint64 hash[2];
    // Compute multiple hash values for the element using different seeds
    for (auto &seed: murmurhash_seeds_) {
        MurmurHash3_x86_128(&element, elementTypeWords, seed, &hash);
        // Find the minimum value of the corresponding counters in the counter array
        uint32 pos = hash[0] % size_;
        r = std::min(r, counter_array_[pos]);
    }

    return r;
}
