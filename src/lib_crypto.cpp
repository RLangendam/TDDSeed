#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/opensslv.h>
#include <openssl/rand.h>

#include <algorithm>
#include <fstream>
#include <memory>
#include <numeric>
#include <vector>

#include "lib.h"

using namespace std;

using EVP_CIPHER_CTX_free_ptr =
    std::unique_ptr<EVP_CIPHER_CTX, decltype(&::EVP_CIPHER_CTX_free)>;

void aes_encrypt(vector<byte> const& key, vector<byte> const& iv,
                 vector<byte> const& ptext, vector<byte>& ctext) {
  EVP_CIPHER_CTX_free_ptr ctx(EVP_CIPHER_CTX_new(), ::EVP_CIPHER_CTX_free);
  int rc =
      EVP_EncryptInit_ex(ctx.get(), EVP_aes_256_cbc(), NULL,
                         reinterpret_cast<unsigned char const*>(key.data()),
                         reinterpret_cast<unsigned char const*>(iv.data()));
  if (rc != 1) throw std::runtime_error("EVP_EncryptInit_ex failed");

  ctext.resize(ptext.size() + iv.size());
  int out_len1 = (int)ctext.size();

  rc = EVP_EncryptUpdate(
      ctx.get(), reinterpret_cast<unsigned char*>(ctext.data()), &out_len1,
      reinterpret_cast<const unsigned char*>(ptext.data()),
      static_cast<int>(ptext.size()));
  if (rc != 1) throw std::runtime_error("EVP_EncryptUpdate failed");

  int out_len2 = (int)ctext.size() - out_len1;
  rc = EVP_EncryptFinal_ex(
      ctx.get(), reinterpret_cast<unsigned char*>(ctext.data()) + out_len1,
      &out_len2);
  if (rc != 1) throw std::runtime_error("EVP_EncryptFinal_ex failed");

  ctext.resize(out_len1 + out_len2);
}

void aes_decrypt(vector<byte> const& key, vector<byte> const& iv,
                 vector<byte> const& ctext, vector<byte>& rtext) {
  EVP_CIPHER_CTX_free_ptr ctx(EVP_CIPHER_CTX_new(), ::EVP_CIPHER_CTX_free);
  int rc =
      EVP_DecryptInit_ex(ctx.get(), EVP_aes_128_ecb(), NULL,
                         reinterpret_cast<unsigned char const*>(key.data()),
                         reinterpret_cast<unsigned char const*>(iv.data()));
  if (rc != 1) throw std::runtime_error("EVP_DecryptInit_ex failed");

  rtext.resize(ctext.size());
  int out_len1 = (int)rtext.size();

  rc = EVP_DecryptUpdate(ctx.get(), (unsigned char*)&rtext[0], &out_len1,
                         reinterpret_cast<const unsigned char*>(ctext.data()),
                         static_cast<int>(ctext.size()));
  if (rc != 1) throw std::runtime_error("EVP_DecryptUpdate failed");

  int out_len2 = (int)rtext.size() - out_len1;
  rc = EVP_DecryptFinal_ex(
      ctx.get(), reinterpret_cast<unsigned char*>(rtext.data()) + out_len1,
      &out_len2);
  if (rc != 1) throw std::runtime_error("EVP_DecryptFinal_ex failed");

  rtext.resize(out_len1 + out_len2);
}

string decrypt_file_7() {
  auto const lines{read_file_lines("7.txt", &from_base64)};
  auto const encrypted =
      accumulate(lines.begin(), lines.end(), vector<byte>{},
                 [](vector<byte>&& memo, vector<byte> const& data) {
                   memo.insert(memo.end(), data.begin(), data.end());
                   return memo;
                 });
  ERR_print_errors_fp(stderr);
  EVP_add_cipher(EVP_aes_128_ecb());

  vector<byte> decrypted;

  string const key2{"YELLOW SUBMARINE"};
  vector<byte> key;
  transform(key2.begin(), key2.end(), back_inserter(key),
            [](char c) { return byte{static_cast<unsigned char>(c)}; });
  vector<byte> iv(16, byte{0});

  aes_decrypt(key, iv, encrypted, decrypted);
  return from_bytes(decrypted);
}
