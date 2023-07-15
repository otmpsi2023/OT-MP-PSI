
#include <fstream>

#include "protocol/participant.h"
#include "utils/common.h"
#include "utils/utils.h"
#include <chrono>
#include <thread>


int main(int argc, char *argv[]) {

    ExperimentConfig config;
    NewConfigFromJsonFile(config, argv[1]);

    std::vector<ElementType> set;
    set.reserve(config.element_set_size);
    generate_set(set, config);

    Participant participant(config.options, set);

    participant.Initialize();

    participant.RingLatency(false);
    participant.RingLatency(true);

    auto durations = participant.Execute(true);

    participant.Stop();

    if (config.options.role == Role::server) {
        std::stringstream ss;
        ss << "-----------------------------------\n"
           << std::left << std::setw(26) << "Number of parties: " << config.options.num_parties << "\n"
           << std::left << std::setw(26) << "Intersection threshold: " << config.options.intersection_threshold << "\n"
           << std::left << std::setw(26) << "Set size: " << set.size() << "\n"
           << "-----------------------------------\n"
           << std::left << std::setw(26) << "Total execution time: " << (durations[0]) << "ms \n"
           << std::left << std::setw(26) << "Server data sent: "
           << FormatBytes(participant.GetTotalBytesSent()) << " \n"
           << std::left << std::setw(26) << "Server data received: "
           << FormatBytes(participant.GetTotalBytesReceived());
        std::string str = ss.str();
        std::cout << str << std::endl;
    }

    // Sleep for 2 seconds to avoid cout conflicts
    std::this_thread::sleep_for(std::chrono::seconds(2));

    if (config.options.local_name == "P2") {
        std::stringstream ss;
        ss << std::left << std::setw(26) << "Client data sent: " << FormatBytes(participant.GetTotalBytesSent())
           << " \n"
           << std::left << std::setw(26) << "Client data received: " << FormatBytes(participant.GetTotalBytesReceived())
           << " \n";
        std::string str = ss.str();
        std::cout << str << std::endl;
    }

    return 0;
}
