#include "utils/utils.h"

#include <NTL/ZZ.h>

#include <fstream>
#include <sstream>
#include <vector>

// Function to read an experiment configuration from a JSON file
void NewConfigFromJsonFile(ExperimentConfig &config, const std::string &json_file) {
    // Read the JSON file into a string
    std::ifstream fJson(json_file);
    std::stringstream buffer;
    buffer << fJson.rdbuf();
    // Parse the JSON string
    auto cJson = nlohmann::json::parse(buffer.str());

    // Set the values of the experiment configuration from the JSON object
    config.element_set_size = cJson["setSize"].get<ContainerSizeType>();
    config.options.bloom_filter_size = cJson["bloomFilterSize"].get<ContainerSizeType>();
    config.num_same_items = cJson["sameNum"].get<ContainerSizeType>();
    config.same_item_seed = cJson["sameSeed"].get<uint32>();
    config.diff_item_seed = cJson["diffSeed"].get<uint32>();
    config.benchmark_rounds = cJson["benchmarkRounds"].get<uint32>();

    config.options.num_parties = cJson["numberOfParties"].get<uint32>();
    config.options.intersection_threshold = cJson["threshold"].get<uint32>();
    config.options.num_hash_functions = cJson["numberOfHashFunctions"].get<uint32>();
    config.options.role = cJson["isServer"].get<bool>() ? Role::server : Role::client;
    config.options.port = cJson["port"].get<int>();
    config.options.local_name = cJson["localName"].get<std::string>();
    config.options.server_address = cJson["serverAddress"].get<std::string>();
    config.options.right_neighbor_address = cJson["rightNeighborAddress"].get<std::string>();
    config.options.party_list = cJson["allParties"].get<std::vector<std::string>>();

    config.options.num_bytes_field_numbers = cJson["bufferSize"].get<int>();


    config.options.keys_seed = cJson["keysSeed"].get<uint32>();
    config.options.index = cJson["index"].get<uint32>();
}

// Function to generate a set of elements
void generate_set(std::vector<ElementType> &set, const ExperimentConfig &config) {
    set.clear();
    long random_num = 0;

    // Generate the same elements using the same seed
    NTL::SetSeed(NTL::conv<NTL::ZZ>(config.same_item_seed));
    for (auto i = 0; i < config.num_same_items; i++) {
        NTL::RandomBnd(random_num, elementTypeMax);
        set.emplace_back(random_num);
    }

    // Generate the different elements using the different seed
    NTL::SetSeed(NTL::conv<NTL::ZZ>(config.diff_item_seed));
    for (auto i = config.num_same_items; i < config.element_set_size; i++) {
        NTL::RandomBnd(random_num, elementTypeMax);
        set.emplace_back(random_num);
    }
    assert(set.size() == config.element_set_size);

    // Set the seed to the current time
    NTL::SetSeed(NTL::conv<NTL::ZZ>((long) time(nullptr))); // TODO: uncomment this
}

// Helper method to format a number of bytes in a more readable form
std::string FormatBytes(uint64 bytes) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    constexpr uint64 kOneKilobyte = 1024;
    constexpr uint64 kOneMegabyte = kOneKilobyte * 1024;
    constexpr uint64 kOneGigabyte = kOneMegabyte * 1024;
    if (bytes < kOneKilobyte) {
        oss << bytes << " B";
    } else if (bytes < kOneMegabyte) {
        oss << static_cast<double>(bytes) / kOneKilobyte << " KB";
    } else if (bytes < kOneGigabyte) {
        oss << static_cast<double>(bytes) / kOneMegabyte << " MB";
    } else {
        oss << static_cast<double>(bytes) / kOneGigabyte << " GB";
    }
    return oss.str();
}
