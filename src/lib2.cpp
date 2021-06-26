#include <algorithm>
#include <cstddef>
#include <numeric>
#include <vector>

#include "lib.h"

using namespace std;

vector<byte> pkcs7(vector<byte> const& data, size_t block_size) {
  auto bytes{data};
  auto padding_size{block_size - (bytes.size() % block_size)};
  if (padding_size == 0) {
    padding_size = block_size;
  }
  generate_n(back_inserter(bytes), padding_size,
             [padding_size = static_cast<byte>(padding_size)]() {
               return padding_size;
             });
  return bytes;
}

string pkcs7(string const& data, size_t block_size) {
  return from_bytes(pkcs7(to_bytes(data), block_size));
}

vector<byte> read_file_from_base64_lines(string const& filename) {
  auto const lines{read_file_lines(filename, &from_base64)};
  return accumulate(lines.begin(), lines.end(), vector<byte>{},
                    [](vector<byte>&& memo, vector<byte> const& data) {
                      memo.insert(memo.end(), data.begin(), data.end());
                      return memo;
                    });
}

vector<byte> pkcs7_inverse(vector<byte> const& padded) {
  return vector<byte>{
      padded.begin(),
      next(padded.begin(), padded.size() - static_cast<size_t>(padded.back()))};
}

vector<byte> aes_cbc_decrypt(vector<byte> const& key, vector<byte> const& iv,
                             vector<byte> const& encrypted) {
  auto const block_size{iv.size()};
  vector<byte> decrypted;
  decrypted.reserve(encrypted.size());
  vector<byte> chain_block{iv};
  auto slice_begin{encrypted.begin()};
  for (; distance(slice_begin, encrypted.end()) >=
         static_cast<ptrdiff_t>(block_size);
       slice_begin += block_size) {
    vector<byte> encrypted_block{slice_begin, next(slice_begin, block_size)};
    auto const decrypted_block{hex_xor(
        chain_block, aes_decrypt(key, chain_block, encrypted_block, 0))};
    decrypted.insert(decrypted.end(), decrypted_block.begin(),
                     decrypted_block.end());
    chain_block = encrypted_block;
  }
  return pkcs7_inverse(decrypted);
}

string decrypt_file_10() {
  auto const encrypted{read_file_from_base64_lines("10.txt")};
  constexpr size_t block_size{16};
  auto const key{to_bytes("YELLOW SUBMARINE")};
  vector<byte> iv(block_size, byte{0});
  return from_bytes(aes_cbc_decrypt(key, iv, encrypted));
}

vector<byte> aes_cbc_encrypt(vector<byte> const& key, vector<byte> const& iv,
                             vector<byte> decrypted) {
  auto const block_size{iv.size()};
  vector<byte> encrypted;
  decrypted = pkcs7(decrypted, block_size);
  encrypted.reserve(decrypted.size());
  vector<byte> chain_block{iv};
  for (auto slice_begin{decrypted.begin()};
       distance(slice_begin, decrypted.end()) >=
       static_cast<ptrdiff_t>(block_size);
       slice_begin += block_size) {
    chain_block = hex_xor(
        chain_block, vector<byte>{slice_begin, next(slice_begin, block_size)});
    auto encrypted_block{aes_encrypt(key, chain_block, chain_block, 0)};
    encrypted.insert(encrypted.end(), encrypted_block.begin(),
                     encrypted_block.end());
    chain_block = encrypted_block;
  }
  return encrypted;
}

vector<byte> encrypt_file_10(string const& decrypted) {
  constexpr size_t block_size{16};
  auto const key{to_bytes("YELLOW SUBMARINE")};
  vector<byte> iv(block_size, byte{0});
  return aes_cbc_encrypt(key, iv, to_bytes(decrypted));
}