#ifndef OTMPSI_CRYPTO_THRESBOLDELGAMAL_H_
#define OTMPSI_CRYPTO_THRESBOLDELGAMAL_H_

#include <NTL/ZZ.h>

#include <utility>
#include <vector>

// Define a Ciphertext type as a pair of ZZ values
typedef std::pair<NTL::ZZ, NTL::ZZ> Ciphertext;

// KeyHolder class for threshold ElGamal encryption
class KeyHolder {
public:
    // Delete the default constructor
    KeyHolder() = delete;

    // Constructor that takes the public parameters p and alpha, and a list of prime factors of p-1
    KeyHolder(const NTL::ZZ &p, const NTL::ZZ &alpha, std::vector<NTL::ZZ> p_prime_factor_list)
            : p_(p), alpha_(alpha), p_prime_factor_list_(std::move(p_prime_factor_list)) {
        // Generate a random secret key a
        NTL::RandomBnd(a_, p);
        while (a_ < 1) {
            NTL::RandomBnd(a_, p - 1);
        }
        // Compute the public key beta = alpha^a mod p
        NTL::PowerMod(beta_, alpha, a_, p);
    }

    // Default destructor
    ~KeyHolder() = default;

    // Method to encrypt a plaintext message
    void Encrypt(Ciphertext &ciphertext, const NTL::ZZ &plaintext);

    // Method to fully decrypt a ciphertext using decryption shares from multiple key holders
    void FullyDecrypt(NTL::ZZ &plaintext, const std::vector<NTL::ZZ> &decryption_shares, const NTL::ZZ &c2);

    // Method to partially decrypt a ciphertext and produce a decryption share
    inline void PartialDecrypt(NTL::ZZ &decryption_share, const NTL::ZZ &c1);

    // Method to exponentiate a ciphertext
    inline void Power(Ciphertext &dest, const Ciphertext &src, const NTL::ZZ &exponent);

    // Method to multiply two ciphertexts
    inline void Mul(Ciphertext &dest, const Ciphertext &src1, const Ciphertext &src2);

    // Method to rerandomize a ciphertext
    inline void ReRand(Ciphertext &dest, const Ciphertext &src);

protected:
    // Public parameters
    NTL::ZZ p_;
    NTL::ZZ alpha_;
    NTL::ZZ beta_;
    std::vector<NTL::ZZ> p_prime_factor_list_;

    // Method to check if a number is coprime with phi(p)
    bool CoprimeWithPhiP(const NTL::ZZ &k);

private:
    // Secret key
    NTL::ZZ a_;
};

// Method to partially decrypt a ciphertext and produce a decryption share
void KeyHolder::PartialDecrypt(NTL::ZZ &decryption_share, const NTL::ZZ &c1) {
    PowerMod(decryption_share, c1, -a_, p_);
}

// Method to exponentiate a ciphertext
void KeyHolder::Power(Ciphertext &dest, const Ciphertext &src, const NTL::ZZ &exponent) {
    PowerMod(dest.first, src.first, exponent, p_);
    PowerMod(dest.second, src.second, exponent, p_);
}

// Method to multiply two ciphertexts
void KeyHolder::Mul(Ciphertext &dest, const Ciphertext &src1, const Ciphertext &src2) {
    MulMod(dest.first, src1.first, src2.first, p_);
    MulMod(dest.second, src1.second, src2.second, p_);
}

// Method to rerandomize a ciphertext
void KeyHolder::ReRand(Ciphertext &dest, const Ciphertext &src) {
    Ciphertext r;
    Encrypt(r, NTL::ZZ(1));
    Mul(dest, src, r);
}

#endif // OTMPSI_CRYPTO_THRESBOLDELGAMAL_H_
