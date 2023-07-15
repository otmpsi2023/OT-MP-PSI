#include "protocol/participant.h"

#include <fstream>
#include <thread>

const std::string serverName = "server";
const std::string rightNeighborName = "right";
const std::string leftNeighborName = "left";

const int rRange = 128;

// Initialize the participant
void Participant::Initialize() {
    if (role() == Role::client) {
        InitializeClient();
    } else if (role() == Role::server) {
        InitializeServer();
    }
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

    NTL::SetSeed(NTL::conv<NTL::ZZ>(options_.keys_seed));
    key_gen(&keys_, 2048, options_.num_parties-1, options_.num_parties);
    NTL::SetSeed(NTL::conv<NTL::ZZ>((long) time(nullptr)));
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

    NTL::SetSeed(NTL::conv<NTL::ZZ>(options_.keys_seed));
    key_gen(&keys_, 2048, options_.num_parties-1, options_.num_parties);
    NTL::SetSeed(NTL::conv<NTL::ZZ>((long) time(nullptr)));
}

// Execute the protocol
std::vector<long long> Participant::Execute(bool print) {
    endpoint_->ResetCounters();
    bf_.clear();

    std::vector<long> intersection;

    auto start = std::chrono::high_resolution_clock::now();

    if (role() == Role::server) {
        PrecomputeServer();
    } else {
        PrecomputeClient();
    }
    RingLatency(false);

    if (role() == Role::server) {
        intersection = ExecuteServer();
    } else {
        ExecuteClient();
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
    std::vector<long long> durations = {dur};
    if (role() == Role::server && print) {
        std::cout << "result size: " << intersection.size() << std::endl;
    }

    assert(rerand_count_ == rerands_.size());
    assert(scp_count_ == r_array_.size());

    return durations;
}


void Participant::PrecomputeServer(){
    one_encryptions_.clear();
    zero_encryptions_.clear();
    k_encryptions_.clear();
    l_encryptions_.clear();
    r_array_.clear();
    r_prime_array_.clear();
    r_prime_encrpytions_.clear();
    uint32 scp_num = (options_.num_parties - 1) * elements_.size() + elements_.size();
    if(one_encryptions_.capacity() < scp_num) {
        one_encryptions_.reserve(scp_num);
        zero_encryptions_.reserve(scp_num);
        k_encryptions_.reserve(scp_num);
        l_encryptions_.reserve(scp_num);
        r_array_.reserve(scp_num);
        r_prime_array_.reserve(scp_num);
        r_prime_encrpytions_.reserve(scp_num);
    }
    for(auto i = 0; i < scp_num; i++){
        one_encryptions_.push_back(encrypt(ZZ(1), keys_.public_key));
        zero_encryptions_.push_back(encrypt(ZZ(0), keys_.public_key));
        k_encryptions_.push_back(encrypt(ZZ(options_.num_hash_functions), keys_.public_key));
        l_encryptions_.push_back(encrypt(ZZ(options_.intersection_threshold), keys_.public_key));

        ZZ r = RandomBnd(NTL::ZZ(rRange - 1)) + 1;
        ZZ r_prime = RandomBnd(r - 1) + 1;
        r_array_.push_back(r);
        r_prime_encrpytions_.push_back(encrypt(r_prime, keys_.public_key));
    }


    rerands_.clear();
    uint32 rerands_size = elements_.size() * (options_.num_parties-1);
    rerands_size += 2 * elements_.size();
    if(rerands_.capacity() < rerands_size){
        rerands_.reserve(rerands_size);
    }

    for(auto i = 0; i < rerands_size; i++){
        rerands_.push_back(encrypt(ZZ(0), keys_.public_key));
    }

    rerand_count_ = 0;
    scp_count_ = 0;
}

void Participant::PrecomputeClient(){
    bf_.clear();
    for (long element : elements_) {
        bf_.insert(element);
    }
    bf_.encrypt_all(ebf_, keys_.public_key);

    r_array_.clear();
    r_prime_array_.clear();
    r_prime_encrpytions_.clear();
    uint32 scp_num = (options_.num_parties - 1) * elements_.size() + elements_.size();
    if(r_array_.capacity() < scp_num) {
        r_array_.reserve(scp_num);
        r_prime_array_.reserve(scp_num);
        r_prime_encrpytions_.reserve(scp_num);
    }

    for(auto i = 0; i < (options_.num_parties-1) * elements_.size() + elements_.size(); i++){
        ZZ r = RandomBnd(NTL::ZZ(rRange - 1)) + 1;
        ZZ r_prime = RandomBnd(r - 1) + 1;
        r_array_.push_back(r);
        r_prime_array_.push_back(r_prime);
    }

    rerands_.clear();
    uint32 rerands_size = 2 * (options_.num_parties-1) * elements_.size();
    rerands_size += 2 * (elements_.size());
    if(rerands_.capacity() < rerands_size) {
        rerands_.reserve(rerands_size);
    }

    for(auto i = 0; i < rerands_size; i++){
        rerands_.push_back(encrypt(ZZ(0), keys_.public_key));
    }
    rerand_count_ = 0;
    scp_count_ = 0;
}


// Execute the protocol
std::vector<long> Participant::ExecuteServer(){
    // receive ebf from all
    std::vector<std::vector<ZZ>> client_ebfs;
    client_ebfs.reserve(options_.num_parties-1);
    ZZ temp;
    for (const auto &remote: options_.party_list) {
        std::vector<ZZ> ebf;
        ebf.reserve(options_.bloom_filter_size);
        if (remote == options_.local_name) {
            continue;
        }
        for(auto i = 0; i < options_.bloom_filter_size; i++){
            ReceiveZz(remote, temp);
            ebf.emplace_back(temp);
        }

        client_ebfs.emplace_back(ebf);
    }

    std::vector<std::vector<ZZ>> client_ciphertexts;
    client_ciphertexts.reserve(options_.num_parties-1);
    for (int i = 0; i < options_.num_parties-1; ++i) {
        // Initialize an empty set for each client
        client_ciphertexts.emplace_back();
    }


    for (long element : elements_) {
        // Compute for the first hash function
        unsigned long index = BloomFilter::hash(element, 0) % options_.bloom_filter_size;

        std::vector<ZZ> client_ciphertext;
        client_ciphertext.reserve(options_.num_parties-1);
        for (int i = 0; i < options_.num_parties-1; ++i) {
            client_ciphertext.push_back(client_ebfs.at(i).at(index));
        }

        // Compute for the remaining hash functions
        for (int i = 1; i < options_.num_hash_functions; ++i) {
            for (int j = 0; j < options_.num_parties-1; ++j) {
                index = BloomFilter::hash(element, i) % options_.bloom_filter_size;

                client_ciphertext.at(j) = add_homomorphically(client_ciphertext.at(j),
                                                              client_ebfs.at(j).at(index),
                                                              keys_.public_key);
            }
        }

        // Rerandomize the ciphertext to prevent analysis due to the deterministic nature of homomorphic addition
        for (int i = 0; i < options_.num_parties-1; ++i) {
            client_ciphertexts.at(i).push_back(add_homomorphically(client_ciphertext.at(i), rerands_[rerand_count_++], keys_.public_key));
        }
    }

    std::vector<std::vector<ZZ>> client_comparisons;
    client_comparisons.reserve(client_ciphertexts.size());

    // first round scps
    for (int i = 0; i < client_ciphertexts.size(); ++i) {
        ZZ a,b, difference, a_1, a_2, c_encrypted;
        for (int j = 0; j < client_ciphertexts.at(i).size(); ++j) {
            // 1. Party X_1 computes the encryption of c = r(x_0 - x_1) - r', where r and r' are random
            a = k_encryptions_[scp_count_];
            b = client_ciphertexts.at(i).at(j);

            difference = subtract_homomorphically(a, b, keys_.public_key);
            c_encrypted = subtract_homomorphically(multiply_homomorphically(difference, r_array_[scp_count_], keys_.public_key),
                                                      r_prime_encrpytions_[scp_count_],
                                                      keys_.public_key);

            // 2. Send ciphertexts to other parties, with a_1 = Enc(1), a_2 = Enc(0), (a_3 = c_encrypted)
            a_1 = one_encryptions_[scp_count_];
            a_2 = zero_encryptions_[scp_count_++];

            SendZz(rightNeighborName, a_1);
            SendZz(rightNeighborName, a_2);
            SendZz(rightNeighborName, c_encrypted);

        }

    }

    std::vector<std::vector<ZZ>> a_1_arrays, a_2_arrays, c_encrypted_arrays;
    a_1_arrays.reserve(client_ciphertexts.size());
    a_2_arrays.reserve(client_ciphertexts.size());
    c_encrypted_arrays.reserve(client_ciphertexts.size());

    for (int i = 0; i < client_ciphertexts.size(); ++i) {
        std::vector<ZZ> a_1_array, a_2_array, c_encrypted_array;
        a_1_array.reserve(client_ciphertexts.at(i).size());
        a_2_array.reserve(client_ciphertexts.at(i).size());
        c_encrypted_array.reserve(client_ciphertexts.at(i).size());
        ZZ a_1, a_2, c_encrypted;
        for (int j = 0; j < client_ciphertexts.at(i).size(); ++j) {
            ReceiveZz(leftNeighborName, a_1);
            ReceiveZz(leftNeighborName, a_2);
            ReceiveZz(leftNeighborName, c_encrypted);

            a_1_array.push_back(a_1);
            a_2_array.push_back(a_2);
            c_encrypted_array.push_back(c_encrypted);
        }

        a_1_arrays.push_back(a_1_array);
        a_2_arrays.push_back(a_2_array);
        c_encrypted_arrays.push_back(c_encrypted_array);
    }

    for (int i = 0; i < client_ciphertexts.size(); ++i) {
        std::vector<ZZ> comparisons;
        comparisons.reserve(client_ciphertexts.at(i).size());
        ZZ a_1, a_2, c_encrypted;
        for (int j = 0; j < client_ciphertexts.at(i).size(); ++j) {
            a_1 = a_1_arrays[i][j];
            a_2 = a_2_arrays[i][j];
            c_encrypted = c_encrypted_arrays[i][j];

            std::vector<std::pair<long, ZZ>> decryption_shares;
            BroadcastZz(c_encrypted);

            ZZ share = partial_decrypt(c_encrypted, keys_.public_key,
                                       keys_.private_keys.at(index_-1));
            decryption_shares.emplace_back(1, share);

            for(auto k = 2; k <= options_.num_parties; k++){
                ReceiveZz("P"+std::to_string(k), share);
                decryption_shares.emplace_back(k, share);
            }


            if (combine_partial_decrypt(decryption_shares, keys_.public_key) <= 0) {
                comparisons.push_back(a_1);
            } else {
                comparisons.push_back(a_2);
            }

        }
        client_comparisons.push_back(comparisons);
    }

    std::vector<ZZ> summed_comparisons;
    summed_comparisons.reserve(elements_.size());
    for (int i = 0; i < elements_.size(); ++i) {
        // Initialize with the first client
        ZZ sum = client_comparisons.at(0).at(i);

        // Add remaining elements from other clients
        for (int j = 1; j < client_comparisons.size(); ++j) {
            sum = add_homomorphically(sum, client_comparisons.at(j).at(i), keys_.public_key);
        }

        // Rerandomize
        summed_comparisons.push_back(add_homomorphically(sum, rerands_[rerand_count_++], keys_.public_key));
    }

    std::vector<ZZ> element_ciphertexts;
    element_ciphertexts.reserve(summed_comparisons.size());

    // second round scps
    for (auto & summed_comparison : summed_comparisons) {
        ZZ a = l_encryptions_[scp_count_];
        ZZ b = summed_comparison;

        ZZ difference = subtract_homomorphically(a, b, keys_.public_key);
        ZZ c_encrypted = subtract_homomorphically(multiply_homomorphically(difference, r_array_[scp_count_], keys_.public_key),
                                                  r_prime_encrpytions_[scp_count_],
                                                  keys_.public_key);

        // 2. Send ciphertexts to other parties, with a_1 = Enc(1), a_2 = Enc(0), (a_3 = c_encrypted)
        ZZ a_1 = one_encryptions_[scp_count_];
        ZZ a_2 = zero_encryptions_[scp_count_++];

        SendZz(rightNeighborName, a_1);
        SendZz(rightNeighborName, a_2);
        SendZz(rightNeighborName, c_encrypted);
    }

    std::vector<ZZ> a_1_array, a_2_array, c_encrypted_array;
    a_1_array.reserve(elements_.size());
    a_2_array.reserve(elements_.size());
    c_encrypted_array.reserve(elements_.size());
    for (auto & summed_comparison : summed_comparisons) {
        ZZ a_1, a_2, c_encrypted, temp;
        ReceiveZz(leftNeighborName, a_1);
        ReceiveZz(leftNeighborName, a_2);
        ReceiveZz(leftNeighborName, c_encrypted);

        a_1_array.push_back(a_1);
        a_2_array.push_back(a_2);
        c_encrypted_array.push_back(c_encrypted);
    }

    int i = 0;
    for (auto & summed_comparison : summed_comparisons) {
        ZZ a_1 = a_1_array[i];
        ZZ a_2 = a_2_array[i];
        ZZ c_encrypted = c_encrypted_array[i++];
        std::vector<std::pair<long, ZZ>> decryption_shares;
        BroadcastZz(c_encrypted);

        ZZ share = partial_decrypt(c_encrypted, keys_.public_key,
                                   keys_.private_keys.at(index_-1));
        decryption_shares.emplace_back(1, share);

        for(auto k = 2; k <= options_.num_parties; k++){
            ReceiveZz("P"+std::to_string(k), share);
            decryption_shares.emplace_back(k, share);
        }

        if (combine_partial_decrypt(decryption_shares, keys_.public_key) <= 0) {
            temp = a_1;
        } else {
            temp = a_2;
        }


        element_ciphertexts.push_back(add_homomorphically(temp, rerands_[rerand_count_++], keys_.public_key));
    }




    std::vector<ZZ> decryptions;
    decryptions.reserve(element_ciphertexts.size());
    for (ZZ ciphertext : element_ciphertexts) {
        // Partial decryption (let threshold + 1 parties decrypt)
        std::vector<std::pair<long, ZZ>> decryption_shares;
        decryption_shares.reserve(options_.num_parties);
        BroadcastZz(ciphertext);

        ZZ share = partial_decrypt(ciphertext, keys_.public_key,
                                   keys_.private_keys.at(index_-1));
        decryption_shares.emplace_back(1, share);

        for(auto k = 2; k <= options_.num_parties; k++){
            ReceiveZz("P"+std::to_string(k), share);
            decryption_shares.emplace_back(k, share);
        }

        // Combining algorithm
        decryptions.push_back(combine_partial_decrypt(decryption_shares,
                                                      keys_.public_key));
    }


    // Output the final intersection by selecting the elements from the server set that correspond to a decryption of one (true)
    std::vector<long> intersection;
    for (int i = 0; i < elements_.size(); ++i) {
        if (decryptions.at(i) == 1) {
            intersection.push_back(elements_.at(i));
        }
    }

    return intersection;
}

// Execute the protocol
void Participant::ExecuteClient(){

    // send ebf to server
    for(auto i = 0; i < options_.bloom_filter_size; i++){
        SendZz(serverName, ebf_[i]);
    }

    // first round scps
    for (auto i = 0; i < options_.num_parties-1; ++i) {
        for (auto j = 0; j < elements_.size(); ++j) {
           ScpClient();
        }
    }
    ZZ c_encrypted;
    for (auto i = 0; i < options_.num_parties-1; ++i) {
        for (auto j = 0; j < elements_.size(); ++j) {
            ReceiveZz(serverName, c_encrypted);
            SendZz(serverName, partial_decrypt(c_encrypted, keys_.public_key,
                                               keys_.private_keys.at(index_-1)));
        }
    }


    //second round scps
    for (auto i = 0; i < elements_.size(); i++){
        ScpClient();
    }
    for (auto i = 0; i < elements_.size(); i++){
        ReceiveZz(serverName, c_encrypted);
        SendZz(serverName, partial_decrypt(c_encrypted, keys_.public_key,
                                           keys_.private_keys.at(index_-1)));
    }

    // decrypt
    for (auto i = 0; i < elements_.size(); i++){
        ReceiveZz(serverName, c_encrypted);
        SendZz(serverName, partial_decrypt(c_encrypted, keys_.public_key,
                                           keys_.private_keys.at(index_-1)));
    }
}


//ZZ Participant::ScpServer(ZZ a, ZZ b){
//    ZZ difference = subtract_homomorphically(a, b, keys_.public_key);
//    ZZ c_encrypted = subtract_homomorphically(multiply_homomorphically(difference, r_array_[scp_count_], keys_.public_key),
//                                              r_prime_encrpytions_[scp_count_],
//                                              keys_.public_key);
//
//    // 2. Send ciphertexts to other parties, with a_1 = Enc(1), a_2 = Enc(0), (a_3 = c_encrypted)
//    ZZ a_1 = one_encryptions_[scp_count_];
//    ZZ a_2 = zero_encryptions_[scp_count_++];
//
//    SendZz(rightNeighborName, a_1);
//    SendZz(rightNeighborName, a_2);
//    SendZz(rightNeighborName, c_encrypted);
//
//    ReceiveZz(leftNeighborName, a_1);
//    ReceiveZz(leftNeighborName, a_2);
//    ReceiveZz(leftNeighborName, c_encrypted);
//
//    std::vector<std::pair<long, ZZ>> decryption_shares;
//    BroadcastZz(c_encrypted);
//
//    ZZ share = partial_decrypt(c_encrypted, keys_.public_key,
//                               keys_.private_keys.at(index_-1));
//    decryption_shares.emplace_back(1, share);
//
//    for(auto k = 2; k <= options_.num_parties; k++){
//        ReceiveZz("P"+std::to_string(k), share);
//        decryption_shares.emplace_back(k, share);
//    }
//
//    if (combine_partial_decrypt(decryption_shares, keys_.public_key) <= 0) {
//        return a_1;
//    }
//
//    return a_2;
//
//}

void Participant::ScpClient(){
    ZZ a_1, a_2, c_encrypted;

    ReceiveZz(leftNeighborName, a_1);
    ReceiveZz(leftNeighborName, a_2);
    ReceiveZz(leftNeighborName, c_encrypted);


    bool b_i = rand() % 2;  // TODO: Check randomness
    if (b_i) {
        // Swap a_1 and a_2 with uniform probability
        std::swap(a_1, a_2);
    }

    a_1 = add_homomorphically(a_1, rerands_[rerand_count_++], keys_.public_key);
    a_2 = add_homomorphically(a_2, rerands_[rerand_count_++], keys_.public_key);


    c_encrypted = multiply_homomorphically(c_encrypted, ZZ((-2 * b_i + 1) * r_array_[scp_count_]), keys_.public_key);
    c_encrypted = add_homomorphically(c_encrypted,
                                      encrypt(ZZ((b_i * 2 - 1) * r_prime_array_[scp_count_++]), keys_.public_key),
                                      keys_.public_key);

    SendZz(rightNeighborName, a_1);
    SendZz(rightNeighborName, a_2);
    SendZz(rightNeighborName, c_encrypted);
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


