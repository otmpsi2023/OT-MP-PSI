#include "protocol/participant.h"

#include <fstream>
#include <thread>

const std::string serverName = "server";
const std::string rightNeighborName = "right";
const std::string leftNeighborName = "left";

// Initialize the participant
void Participant::Initialize() {
    if (role() == Role::client) {
        InitializeClient();
    } else if (role() == Role::server) {
        InitializeServer();
    }

    DistributedKeyGeneration();
}

// Initialize the client participant
void Participant::InitializeClient() {
    // Connect to the server
    endpoint_->Connect(serverName, options_.server_address, options_.local_name);

    // Connect to the right neighbor
    endpoint_->Connect(rightNeighborName, options_.right_neighbor_address, leftNeighborName);

    // Wait for all connections to be established
    uint32 numConn = 3;
    while (endpoint_->GetRemoteNames().size() < numConn) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    endpoint_->StopListen();
}

// Initialize the server participant
void Participant::InitializeServer() {
    // Connect to the right neighbor
    endpoint_->Connect(rightNeighborName, options_.right_neighbor_address, leftNeighborName);

    // Wait for all connections to be established
    uint32 numConn = options_.num_parties + 1;
    while (endpoint_->GetRemoteNames().size() < numConn) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    endpoint_->StopListen();
}

// Perform distributed key generation
void Participant::DistributedKeyGeneration() {
    if (role() == Role::server) {
        DistributedKeyGenerationServer();
    } else if (role() == Role::client) {
        DistributedKeyGenerationClient();
    }
}

// Perform distributed key generation for the server participant
void Participant::DistributedKeyGenerationServer() {
    NTL::ZZ temp;
    std::vector<NTL::ZZ> zz_array;
    CollectZz(zz_array);

    // Compute the product of all beta values
    for (const auto &z: zz_array) {
        NTL::MulMod(beta_, beta_, z, options_.p);
    }

    // Broadcast the final beta value to all participants
    BroadcastZz(beta_);
}

// Perform distributed key generation for the client participant
void Participant::DistributedKeyGenerationClient() {
    // Send the local beta value to the server
    SendZz(serverName, beta_);

    // Receive the final beta value from the server
    ReceiveZz(serverName, beta_);
}

// Execute the protocol
std::vector<long long> Participant::Execute(bool print) {
    endpoint_->ResetCounters();
    bf_.Clear();

    ContainerSizeType size = 0;
    if (role() == Role::server) {
        size = options_.bloom_filter_size;
    }

    std::vector<Ciphertext> encrypted_bases(
            size); // encrypted bases that will be passed along the ring for the purpose of voting
    std::vector<NTL::ZZ> decrypted_bases(size); // decryption outputs
    std::vector<std::pair<int, uint64>> result; // OTMPSI final result
    std::vector<Ciphertext> rerand_array(
            options_.bloom_filter_size); // probabilistic encryption of 1, used for ReRand Algorithm

    auto start = std::chrono::high_resolution_clock::now();

    Prepare(encrypted_bases, rerand_array);
    RingLatency(false);

    auto preparation_done = std::chrono::high_resolution_clock::now();

    RingPass(encrypted_bases, rerand_array);

    Decrypt(decrypted_bases, encrypted_bases, rerand_array);

    FindIntersection(result, decrypted_bases);

    auto end = std::chrono::high_resolution_clock::now();

    auto preparation = std::chrono::duration_cast<std::chrono::milliseconds>(preparation_done - start).count();
    auto online = std::chrono::duration_cast<std::chrono::milliseconds>(end - preparation_done).count();
    std::vector<long long> durations = {preparation, online};
    if (role() == Role::server && print) {
        std::cout << "result size: " << result.size() << std::endl;
    }

    return durations;
}

// Check if a number is a generator
bool is_generator(const NTL::ZZ &g, const NTL::ZZ &p, const std::vector<NTL::ZZ> &ppFactors) {
    NTL::ZZ temp;
    for (const auto &f: ppFactors) {
        PowerMod(temp, g, (p - 1) / f, p);
        if (temp == 1) {
            return false;
        }
    }
    return true;
}

// Prepare for the protocol
void Participant::Prepare(std::vector<Ciphertext> &encrypted_bases, std::vector<Ciphertext> &rerand_array) {
    // Build the bloom filter
    for (const auto &e: elements_) {
        bf_.Insert(e);
    }

    // Invert bloom filter
    bf_.Invert();

    // Finish the preparation
    if (role() == Role::server) {
        PrepareServer(encrypted_bases, rerand_array);
    } else {
        PrepareClient(rerand_array);
    }
}

// Prepare for the protocol for the server participant
void Participant::PrepareServer(std::vector<Ciphertext> &encrypted_bases, std::vector<Ciphertext> &rerand_array) {
    NTL::ZZ vote_base; // vote vote_base
    NTL::ZZ vote_base_power =  // vote vote_base power. vote vote_base = generator ^ (q ^ (t-l+1))
            (options_.p - 1) / NTL::power(options_.q, (options_.num_parties - options_.intersection_threshold + 1));


    // random encrypted_bases
    RandomBnd(vote_base, options_.p);
    while (!is_generator(vote_base, options_.p, options_.phi_p_prime_factor_list)) {
        RandomBnd(vote_base, options_.p);
    }

    PowerMod(vote_base, vote_base, vote_base_power, options_.p);
    NTL::ZZ temp;
    for (auto i = 0; i < bf_.size(); i++) {
//        if (bf_.CheckPosition(i)) {
//            encrypted_bases[i] = std::make_pair(RandomBnd(options_.p - 1), RandomBnd(options_.p - 1));
//        } else {
//            Encrypt(encrypted_bases[i], vote_base);
//        }
        temp = vote_base;
        if (bf_.CheckPosition(i)) {
            NTL::PowerMod(temp, temp, options_.q, options_.p);
        }
        Encrypt(encrypted_bases[i], temp);
    }

    for (auto i = 0; i < options_.num_hash_functions * elements_.size(); i++) {
        Encrypt(rerand_array[i], NTL::ZZ(1));
    }
}

// Prepare for the protocol for the client participant
void Participant::PrepareClient(std::vector<Ciphertext> &rerand_array) {
    for (auto i = 0; i < bf_.size(); i++) {
        Encrypt(rerand_array[i], NTL::ZZ(1));
    }
}

// Pass the bases on the ring
void Participant::RingPass(std::vector<Ciphertext> &encrypted_bases, const std::vector<Ciphertext> &rerand_array) {
    Ciphertext temp;
    if (role() == Role::server) {
        RingPassServer(encrypted_bases);
    } else {
        RingPassClient(encrypted_bases, rerand_array);
    }
}

// Pass the bases on the ring for the server participant
void Participant::RingPassServer(std::vector<Ciphertext> &encrypted_bases) {
    for (const auto &base: encrypted_bases) {
        // if head, send ciphertexts to right neighbor to start
        SendCiphertext(rightNeighborName, base);
    }

    for (auto i = 0; i < bf_.size(); i++) {
        // receive from left neighbor to end this stage
        ReceiveCiphertext(leftNeighborName, encrypted_bases[i]);
    }
}

// Pass the bases on the ring for the client participant
void
Participant::RingPassClient(std::vector<Ciphertext> &encrypted_bases, const std::vector<Ciphertext> &rerand_array) {
    Ciphertext temp;
    for (auto i = 0; i < bf_.size(); i++) {
        // receive from left neighbor
        ReceiveCiphertext(leftNeighborName, temp);

        // raise to Power of q if it is a 1 in node's rbf
        if (bf_.CheckPosition(i)) {
            Power(temp, temp, options_.q);
        }

        // ReRand c
        Mul(temp, temp, rerand_array[i]);

        // send to right neighbor
        SendCiphertext(rightNeighborName, temp);
    }
}

// Decrypt the encrypted bases
void Participant::Decrypt(std::vector<NTL::ZZ> &decrypted_bases, std::vector<Ciphertext> &encrypted_bases,
                          const std::vector<Ciphertext> &rerand_array) {
    if (role() == Role::server) {
        DecryptServer(decrypted_bases, encrypted_bases, rerand_array);
    } else {
        DecryptClient();
    }
}

// Decrypt the encrypted bases for the server participant
void Participant::DecryptServer(std::vector<NTL::ZZ> &decrypted_bases, std::vector<Ciphertext> &encrypted_bases,
                                const std::vector<Ciphertext> &rerand_array) {
    int cnt = 0;
    NTL::ZZ temp;
    for (auto i = 0; i < decrypted_bases.size(); i++) {
        temp = decrypted_bases[i];

        if (!bf_.CheckPosition(i)) {  // if it is not a 1 in inverted bf[i], then decrypt it
            Mul(encrypted_bases[i], encrypted_bases[i], rerand_array[cnt]); // rerand the ciphertexts before sending out
            MutualDecryptServer(decrypted_bases[i], encrypted_bases[i]);
            cnt++;
        } else {
            decrypted_bases[i] = 1;
        }
    }

    // keep sending out dummy decryption request until there are k*n of them in total
    while (cnt < options_.num_hash_functions * elements_.size()) {
        BroadcastZz(RandomBnd(options_.p - 1));

        std::vector<NTL::ZZ> shares;
        shares.reserve(options_.party_list.size());

        CollectZz(shares);
        cnt++;
    }
}

// Decrypt the encrypted bases for the client participant
void Participant::DecryptClient() {
    for (auto i = 0; i < options_.num_hash_functions * elements_.size(); i++) {
        MutualDecryptClient();
    }
}

// Find the intersection of the sets
void Participant::FindIntersection(std::vector<std::pair<int, uint64>> &intersection,
                                   const std::vector<NTL::ZZ> &decrypted_bases) {
    if (role() == Role::server) {
        FindIntersectionServer(intersection, decrypted_bases);
    }
}

// Find the intersection of the sets for the server participant
void Participant::FindIntersectionServer(std::vector<std::pair<int, uint64>> &intersection,
                                         const std::vector<NTL::ZZ> &decrypted_bases) {
    int cnt;
    NTL::ZZ temp;
    CountBloomFilter rcbf(options_.bloom_filter_size, options_.murmurhash_seeds);

    // fill in the rcbf using the decrypted values
    for (auto i = 0; i < decrypted_bases.size(); i++) {
        temp = decrypted_bases[i];

        cnt = 0;
        while (temp != 1) {  // keep raising to the power of q until it is a 1, and count the number of operations
            PowerMod(temp, temp, options_.q, options_.p);
            cnt++;
        }
        if (cnt) {
            rcbf.Set(i, options_.intersection_threshold + cnt - 1); // get the actual votes from the count
        }
    }


    // perform membership tests using the rcbf
    int num;
    for (const auto &e: elements_) {
        num = rcbf.CheckElement(e);
        if (num > 0) {
            intersection.emplace_back(num, e);
        }
    }
}

// Perform mutual decryption for the server participant
void Participant::MutualDecryptServer(NTL::ZZ &result, const Ciphertext &c) {
    NTL::ZZ temp;
    // broadcast the first part of the ciphertext
    BroadcastZz(c.first);

    // prepare server's own decryption share
    std::vector<NTL::ZZ> shares;
    shares.reserve(options_.party_list.size());
    PartialDecrypt(temp, c.first);
    shares.push_back(std::move(temp));

    // collect decryption shares from all other parties
    CollectZz(shares);

    // fully decrypt using the decrpytion shares
    FullyDecrypt(result, shares, c.second);
}

// Perform mutual decryption for the client participant
void Participant::MutualDecryptClient() {
    NTL::ZZ d, temp;

    ReceiveZz(serverName, temp);
    PartialDecrypt(d, temp);
    SendZz(serverName, d);
}

// Get the ring latency
void Participant::RingLatency(bool print) {
    auto start = std::chrono::high_resolution_clock::now();
    if (role() == Role::server) {
        RingLatencyServer(start, print);
    } else {
        RingLatencyClient();
    }
}

// Get the ring latency, the server participant
void Participant::RingLatencyServer(std::chrono::high_resolution_clock::time_point start, bool print) {
    // dummy write and read
    uint8 dummy[2];
    endpoint_->Write(rightNeighborName, dummy, sizeof(dummy));
    endpoint_->Read(leftNeighborName, dummy, sizeof(dummy));

    auto end = std::chrono::high_resolution_clock::now();
    if (print) {
        std::cout << "Ring Latency: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
                  << "ms" << std::endl;
    }
}

// Get the ring latency, the client participant
void Participant::RingLatencyClient() {
    // dummy read and write
    uint8 dummy[2];
    endpoint_->Read(leftNeighborName, dummy, sizeof(dummy));
    endpoint_->Write(rightNeighborName, dummy, sizeof(dummy));
}

// Broadcast an NTL::ZZ to all remote participants
void Participant::BroadcastZz(const NTL::ZZ &n) {
    for (const auto &remote: options_.party_list) {
        if (remote == options_.local_name) {
            continue;
        }
        SendZz(remote, n);
    }
}

// Collect NTL::ZZs from all remote participants
void Participant::CollectZz(std::vector<NTL::ZZ> &zz_array) {
    NTL::ZZ temp;
    for (const auto &remote: options_.party_list) {
        if (remote == options_.local_name) {
            continue;
        }
        ReceiveZz(remote, temp);

        zz_array.push_back(std::move(temp));
    }
}

// Broadcast a ciphertext to all remote participants
void Participant::BroadcastCiphertext(const Ciphertext &ciphertext) {
    for (const auto &remote: options_.party_list) {
        if (remote == options_.local_name) {
            continue;
        }
        SendCiphertext(remote, ciphertext);
    }
}

// Collect ciphertexts from all remote participants
void Participant::CollectCiphertext(std::vector<Ciphertext> &ciphertext_array) {
    Ciphertext temp;
    for (const auto &remote: options_.party_list) {
        if (remote == options_.local_name) {
            continue;
        }
        ReceiveCiphertext(remote, temp);
        ciphertext_array.push_back(std::move(temp));
    }
}

