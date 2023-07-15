#ifndef OTMPSI_PARTICIPANT_H
#define OTMPSI_PARTICIPANT_H

#include <chrono>
#include <vector>

#include "crypto/threshold_paillier.h"
#include "network/tcp_endpoint.h"
#include "utils/bloom_filter.h"
#include "utils/common.h"

class Participant {
public:
    Participant(const Options &options, const std::vector<ElementType> &set)
            : endpoint_(new TcpEndpoint(options.port)),
              elements_(set),
              bf_(options.bloom_filter_size, options.num_hash_functions),
              options_(options),
              index_(options.index){
        endpoint_->Start();
    };

    Participant() = delete;

    ~Participant() = default;

    [[nodiscard]] Role role() const { return options_.role; };

    void ChangeElementSet(const std::vector<ElementType> &new_set) { elements_ = new_set; };

    void Initialize();

    void RingLatency(bool print);

    inline void Stop();

    std::vector<long long> Execute(bool print);

    inline uint64 GetTotalBytesSent() const;

    inline uint64 GetTotalBytesReceived() const;

private:
    Endpoint *endpoint_;
    std::vector<ElementType> elements_;
    BloomFilter bf_;
    Options options_;
    Keys keys_;
    uint32 index_;

    std::vector<ZZ> ebf_;
    std::vector<ZZ> one_encryptions_;
    std::vector<ZZ> zero_encryptions_;
    std::vector<ZZ> k_encryptions_;
    std::vector<ZZ> l_encryptions_;
    std::vector<ZZ> r_array_;
    std::vector<ZZ> r_prime_array_;
    std::vector<ZZ> r_prime_encrpytions_;
    std::vector<ZZ> rerands_;
    uint32 rerand_count_;
    uint32 scp_count_;

    void InitializeServer();

    void InitializeClient();

    void PrecomputeServer();
    void PrecomputeClient();

    std::vector<long> ExecuteServer();

    void ExecuteClient();

    ZZ ScpServer(ZZ a, ZZ b);
    void ScpClient();

    inline void SendZz(const std::string &remote, const NTL::ZZ &n);

    inline void ReceiveZz(const std::string &remote, NTL::ZZ &n);

    void BroadcastZz(const NTL::ZZ &n);

    void CollectZz(std::vector<NTL::ZZ> &zz_array);

    void RingLatencyServer(std::chrono::high_resolution_clock::time_point start, bool print);

    void RingLatencyClient();
};


void Participant::SendZz(const std::string &remote, const NTL::ZZ &n) {
    unsigned char buf[options_.num_bytes_field_numbers];
    BytesFromZZ(buf, n, options_.num_bytes_field_numbers);
    endpoint_->Write(remote, buf, options_.num_bytes_field_numbers);
}

void Participant::ReceiveZz(const std::string &remote, NTL::ZZ &n) {
    unsigned char buf[options_.num_bytes_field_numbers];
    endpoint_->Read(remote, buf, options_.num_bytes_field_numbers);
    ZZFromBytes(n, buf, options_.num_bytes_field_numbers);
}

void Participant::Stop() { endpoint_->Stop(); }

// Method to get the total amount of data sent in a more readable form
uint64 Participant::GetTotalBytesSent() const {
    return endpoint_->GetTotalBytesSent();
}

// Method to get the total amount of data received in a more readable form
uint64 Participant::GetTotalBytesReceived() const {
    return endpoint_->GetTotalBytesReceived();
}

#endif  // OTMPSI_PARTICIPANT_H