#include "crypto/threshold_elgamal.h"

// Method to find square root of a ciphertext. The function assigns src to dest if src is not a square in the finite field
void KeyHolder::SquareRoot(Ciphertext &dest, const Ciphertext &src) {
    NTL::ZZ root1, root2;
    NTL::SqrRootMod(root1, src.first, p_);
    NTL::SqrRootMod(root2, src.second, p_);

    if (root1 == 0 || root2 == 0) {
        dest = src;
    } else {
        dest.first = root1;
        dest.second = root2;
    }
}

// Method to encrypt a plaintext message using the ElGamal encryption scheme
void KeyHolder::Encrypt(Ciphertext &ciphertext, const NTL::ZZ &plaintext) {
    // Generate a random number that is coprime with p
    NTL::ZZ random_num;
    NTL::RandomBnd(random_num, p_);
    while (!CoprimeWithPhiP(random_num) || random_num < 3 || random_num > p_ - 3) {
        random_num += 1;
    }
    // Compute the first component of the ciphertext as c1 = alpha^random_num mod p
    NTL::PowerMod(ciphertext.first, alpha_, random_num, p_);
    // Compute the second component of the ciphertext as c2 = beta^random_num * plaintext mod p
    NTL::PowerMod(ciphertext.second, beta_, random_num, p_);
    NTL::MulMod(ciphertext.second, ciphertext.second, plaintext, p_);
}

// Method to fully decrypt a ciphertext using decryption shares from multiple key holders
void KeyHolder::FullyDecrypt(NTL::ZZ &plaintext, const std::vector<NTL::ZZ> &decryption_shares, const NTL::ZZ &c2) {
    // Compute the product of all decryption shares
    plaintext = c2;
    for (const auto &it: decryption_shares) {
        NTL::MulMod(plaintext, plaintext, it, p_);
    }
}

// Method to check if a number is coprime with phi(p)
bool KeyHolder::CoprimeWithPhiP(const NTL::ZZ &k) {
    // Check if k is negative
    if (k < 0) {
        return false;
    }

    // Check if k is divisible by any prime factor of p-1
    if (all_of(phi_p_prime_factor_list_.begin(), phi_p_prime_factor_list_.end(),
               [k](const NTL::ZZ &f) { return k % f == 0; })) {
        return false;
    }

    return true;
}
