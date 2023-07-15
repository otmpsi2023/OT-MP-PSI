#ifndef OTMPSI_UTILS_BLOOMFILTER_H_
#define OTMPSI_UTILS_BLOOMFILTER_H_

#include <NTL/ZZ.h>

#include <boost/dynamic_bitset.hpp>
#include <vector>

#include "common.h"

// Class for a Bloom filter
class BloomFilter {
public:
    // Delete the default constructor
    BloomFilter() = delete;

    // Default destructor
    ~BloomFilter() = default;

    // Constructor that takes the size of the filter and a vector of MurmurHash seeds
    BloomFilter(const ContainerSizeType &size, const std::vector<uint32> &murmurhash_seeds)
            : size_(size), bit_array_(boost::dynamic_bitset<>(size)), murmurhash_seeds_(murmurhash_seeds) {};

    // Method to get the size of the filter
    [[nodiscard]] inline ContainerSizeType size() const;

    // Method to invert the filter
    inline void Invert();

    // Method to clear the filter
    inline void Clear();

    // Method to check if a position in the filter is set
    inline bool CheckPosition(const ContainerSizeType &pos);

    // Method to insert an element into the filter
    void Insert(const ElementType &e);

    // Method to check if an element is in the filter
    bool CheckElement(const ElementType &e);

private:
    ContainerSizeType size_; // size of the bloom filter
    boost::dynamic_bitset<> bit_array_; // underlying bit array
    std::vector<uint32> murmurhash_seeds_; // murmurhash seeds for hash functions
};

// Method to get all the hashed positions using the murmurhash seeds
std::vector<ContainerSizeType>
GetHashPositions(const ElementType &e, ContainerSizeType size, const std::vector<uint> &murmurhash_seeds);

// Method to get the size of the filter
ContainerSizeType BloomFilter::size() const { return size_; }

// Method to check if a position in the filter is set
bool BloomFilter::CheckPosition(const ContainerSizeType &pos) { return bit_array_[pos] == 1; }

// Method to invert the filter
void BloomFilter::Invert() { bit_array_.flip(0, size_); }

// Method to clear the filter
void BloomFilter::Clear() { bit_array_.reset(); }


#endif // OTMPSI_UTILS_BLOOMFILTER_H_
