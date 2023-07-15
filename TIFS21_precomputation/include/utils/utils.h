#ifndef OTMPSI_UTILS_UTILS_H_
#define OTMPSI_UTILS_UTILS_H_

#include "common.h"

// Function to read an experiment configuration from a JSON file
void NewConfigFromJsonFile(ExperimentConfig &config, const std::string &json_file);

// Function to generate a set of elements
void generate_set(std::vector<ElementType> &set, const ExperimentConfig &config);

// Helper method to format a number of bytes in a more readable form
std::string FormatBytes(uint64 bytes);

#endif // OTMPSI_UTILS_UTILS_H_
