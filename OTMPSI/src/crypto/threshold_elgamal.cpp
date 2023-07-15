#include "crypto/threshold_elgamal.h"

// Method to encrypt a plaintext message using the ElGamal encryption scheme
void KeyHolder::Encrypt(Ciphertext &ciphertext, const NTL::ZZ &plaintext) {
    // Generate a random number that is coprime with p
    NTL::ZZ random_num;
    RandomBnd(random_num, p_);
    while (!CoprimeWithPhiP(random_num) || random_num < 3 || random_num > p_ - 3) {
        random_num += 1;
    }
    // Compute the first component of the ciphertext as c1 = alpha^random_num mod p
    PowerMod(ciphertext.first, alpha_, random_num, p_);
    // Compute the second component of the ciphertext as c2 = beta^random_num * plaintext mod p
    PowerMod(ciphertext.second, beta_, random_num, p_);
    MulMod(ciphertext.second, ciphertext.second, plaintext, p_);
}

// Method to fully decrypt a ciphertext using decryption shares from multiple key holders
void KeyHolder::FullyDecrypt(NTL::ZZ &plaintext, const std::vector<NTL::ZZ> &decryption_shares, const NTL::ZZ &c2) {
    // Compute the product of all decryption shares
    plaintext = c2;
    for (const auto &it: decryption_shares) {
        MulMod(plaintext, plaintext, it, p_);
    }
}

// Method to check if a number is coprime with p
bool KeyHolder::CoprimeWithPhiP(const NTL::ZZ &k) {
    // Check if k is negative
    if (k < 0) {
        return false;
    }

    // Check if k is divisible by any prime factor of p-1
    if (all_of(p_prime_factor_list_.begin(), p_prime_factor_list_.end(),
               [k](const NTL::ZZ &f) { return k % f == 0; })) {
        return false;
    }

    return true;
}
