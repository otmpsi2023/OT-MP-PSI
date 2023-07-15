
#include <cstdlib>
#include <fstream>

#include "protocol/participant.h"
#include "third_party/smhasher/MurmurHash3.h"
#include "utils/common.h"
#include "utils/utils.h"

int main(int argc, char *argv[]) {
    assert(argc == 2);

    ExperimentConfig config;
    NewConfigFromJsonFile(config, argv[1]);

    std::vector<ElementType> set;
    set.reserve(config.element_set_size);
    generate_set(set, config);

    std::vector<std::vector<long long>> durations;
    std::vector<uint64> data_send_amounts;
    std::vector<uint64> data_receive_amounts;
    std::vector<std::chrono::duration<double>> network_durations;

    durations.reserve(config.benchmark_rounds);

    Participant participant(config.options, set);

    if (config.options.role == Role::server) {
        std::cout << "*******************************************************" << std::endl;
    }
    participant.Initialize();
    participant.RingLatency(false);
    participant.RingLatency(true);

    srand(time(0));
    for (auto i = 0; i < config.benchmark_rounds; i++) {
        config.same_item_seed += 1;
        config.diff_item_seed += rand();
        generate_set(set, config);
        participant.ChangeElementSet(set);
        participant.RingLatency(false);
        durations.emplace_back(participant.Execute(false));
    }
    participant.Stop();

    if (config.options.role == Role::server) {
        long long total_avg = 0;
        for (auto duration: durations) {
            total_avg += duration[0];
        }

        total_avg /= config.benchmark_rounds;

        double online_sd = 0;
        double total_sd = 0;

        for (auto d: durations) {
            total_sd += pow(d[0] - total_avg, 2);
        }

        online_sd = sqrt(online_sd / config.benchmark_rounds);
        total_sd = sqrt(total_sd / config.benchmark_rounds);

        std::stringstream ss;
        ss << "-----------------------------------\n"
           << "Benchmark rounds: " << config.benchmark_rounds << "\n"
           << "-----------------------------------\n"
           << std::left << std::setw(26) << "Number of participants: " << config.options.num_parties << "\n"
           << std::left << std::setw(26) << "Intersection threshold: " << config.options.intersection_threshold
           << "\n"
           << std::left << std::setw(26) << "Set size: " << set.size() << "\n"
           << "-----------------------------------\n"
           << std::left << std::setw(26) << "Total: " << total_avg << " +- " << total_sd << "ms\n"
           << std::left << std::setw(26) << "Server data sent: " << FormatBytes(participant.GetTotalBytesSent())
           << " \n"
           << std::left << std::setw(26) << "Server data received: "
           << FormatBytes(participant.GetTotalBytesReceived()) << "\n";
        std::string str = ss.str();
        std::cout << str << std::endl;
    }

    // Sleep for 1 seconds to avoid cout conflicts
    std::this_thread::sleep_for(std::chrono::seconds(1));

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
