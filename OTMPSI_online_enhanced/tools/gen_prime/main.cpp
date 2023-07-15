#include <NTL/ZZ.h>
#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace NTL;

const int numTrails = 60;

// Function to convert a ZZ type to a string
std::string zToString(const ZZ &z) {
    std::stringstream buffer;
    buffer << z;
    return buffer.str();
}

int main(int argc, char *argv[]) {
    long security_bits = 2048;
    long q = 11;
    long q_power = 55;

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-sec") {
            if (i + 1 < argc) {
                security_bits = strtol(argv[++i], nullptr, 10);
            }
        } else if (arg == "-q") {
            if (i + 1 < argc) {
                q = strtol(argv[++i], nullptr, 10);
            }
        } else if (arg == "-power") {
            if (i + 1 < argc) {
                q_power = strtol(argv[++i], nullptr, 10);
            }
        }
    }

    // Calculate raised_q as q^q_power
    ZZ raised_q(1);
    for (int i = 0; i < q_power; i++) {
        raised_q *= q;
    }

    // Calculate the number of bits for p
    long p_bits = security_bits + (NumBits(raised_q) / 32 + 2) * 32;

    ZZ p(0);
    ZZ large_prime(0);
    ZZ temp, pp2;
    while (NumBits(p) != p_bits) {
        // Generate a prime number with the specified number of security bits
        while (!ProbPrime(p, numTrails)) {
            GenPrime(large_prime, security_bits);

            // Try to find a prime number p with the specified number of bits
            for (int i = 0; i < 50; i++) {
                long bits_needed = p_bits - NumBits(large_prime) - NumBits(raised_q);
                GenPrime(pp2, bits_needed);
                temp = pp2 * raised_q * large_prime;
                while (NumBits(temp) < p_bits) temp *= 2;
                p = temp + 1;

                if (ProbPrime(p, 50)) {
                    break;
                }
            }
        }
    }

    // Verify that p is divisible by q^q_power
    temp = p - 1;
    int cnt = 0;
    while (temp % q == 0) {
        temp /= q;
        cnt++;
    }
    // assert(cnt == Q_POWER);
    assert(ProbPrime(p, numTrails));
    assert(ProbPrime(q, numTrails));
    assert(ProbPrime(large_prime, numTrails));
    assert(ProbPrime(pp2, numTrails));

    // Output information about the generated prime number
    std::string str = "";
    str += "-------------------------\n";
    str += "p_: " + zToString(p) + "\n";
    str += "p_ num bits: " + std::to_string(NumBits(p)) + "\n";
    str += "-------------------------\n";
    str += "large prime factor 1: " + zToString(large_prime) + "\n";
    str += "prime factor 2: " + zToString(pp2) + "\n";
    str += "q: " + std::to_string(q) + "\n";
    str += "q Power: " + std::to_string(cnt) + "\n";
    str += "-------------------------\n";

    // Write the output to a file
    std::string filename = "tools/gen_prime/prime.txt";
    std::ofstream outputFile;
    outputFile.open(filename);
    outputFile << str << std::endl;
    outputFile.close();
    std::cout << str << std::endl;

    return 0;
}
