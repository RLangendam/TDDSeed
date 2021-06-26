#include <algorithm>

#include "lib.h"

using namespace std;

string pkcs7(string const& data, size_t block_size) {
  auto bytes{to_bytes(data)};
  auto const padding_size{block_size - (bytes.size() % block_size)};
  generate_n(back_inserter(bytes), padding_size,
             [padding_size = static_cast<byte>(padding_size)]() {
               return padding_size;
             });
  return from_bytes(bytes);
}