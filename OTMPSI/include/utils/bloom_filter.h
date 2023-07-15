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

// Method to get the size of the filter
ContainerSizeType BloomFilter::size() const { return size_; }

// Method to check if a position in the filter is set
bool BloomFilter::CheckPosition(const ContainerSizeType &pos) { return bit_array_[pos] == 1; }

// Method to invert the filter
void BloomFilter::Invert() { bit_array_.flip(0, size_); }

// Method to clear the filter
void BloomFilter::Clear() { bit_array_.reset(); }

// Class for a counting Bloom filter
class CountBloomFilter {
public:
    // Delete the default constructor
    CountBloomFilter() = delete;

    // Default destructor
    ~CountBloomFilter() = default;

    // Constructor that takes the size of the filter and a vector of MurmurHash seeds
    CountBloomFilter(const ContainerSizeType &size, const std::vector<uint32> &murmurhashSeeds)
            : size_(size), counter_array_(std::vector<uint32>(size)), murmurhash_seeds_(murmurhashSeeds) {};

    // Method to get the size of the filter
    inline ContainerSizeType size() const;

    // Method to insert an element into the filter
    void Insert(const ElementType &element);

    // Method to remove an element from the filter
    void Remove(const ElementType &element);

    // Method to set the value at a position in the filter
    inline void Set(const ContainerSizeType &position, const uint32 &val);

    // Method to check if an element is in the filter
    uint32 CheckElement(const ElementType &element);

    // Method to get the value at a position in the filter
    inline uint32 CheckPosition(const ContainerSizeType &pos);

private:
    uint32 size_;
    std::vector<uint32> counter_array_;
    std::vector<uint32> murmurhash_seeds_;
};

// Method to get the size of the filter
ContainerSizeType CountBloomFilter::size() const { return size_; }

// Method to get the value at a position in the filter
uint32 CountBloomFilter::CheckPosition(const ContainerSizeType &pos) { return counter_array_[pos]; }

// Method to set the value at a position in the filter
void CountBloomFilter::Set(const ContainerSizeType &position, const uint32 &val) { counter_array_[position] = val; }

#endif // OTMPSI_UTILS_BLOOMFILTER_H_
