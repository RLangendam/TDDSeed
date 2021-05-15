#include "lib.h"

#include <algorithm>
#include <boost/range/algorithm/transform.hpp>
#include <boost/range/combine.hpp>
#include <iterator>
#include <sstream>
#include <vector>

using namespace std;
using namespace boost;

char hex_to_char(char c) {
  c = static_cast<char>(toupper(c));
  if (c >= 'A' && c <= 'F') {
    return 10 + c - 'A';
  } else {
    return c - '0';
  }
}

char char_to_hex(char c) {
  if (c > 9)
    return c - 10 + 'A';
  else
    return c + '0';
}

string hex_to_base64(string const &hex) {
  static auto const base64{
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"};
  string result;
  uint16_t bits{0};
  size_t bit_index{0};
  for (auto c : hex) {
    c = hex_to_char(c);
    if (bit_index == 3) {
      result.push_back(base64[bits >> 6]);
      result.push_back(base64[bits & 0b111111]);
      bit_index = 0;
      bits = 0;
    } else {
      bits <<= 4;
    }
    bits |= c;
    ++bit_index;
  }
  switch (bit_index) {
    case 0:
      break;
    case 1:
      result.push_back(base64[bits << 2]);
      break;
    case 2:
      result.push_back(base64[bits >> 2]);
      result.push_back(base64[(bits & 0b11) << 4]);
      break;
    case 3:
      result.push_back(base64[bits >> 6]);
      result.push_back(base64[bits & 0b111111]);
      break;
  }
  return result;
}

string hex_xor(string const &left, string const &right) {
  string result;
  range::transform(combine(left, right), back_inserter(result),
                   [](auto const &pair) {
                     char l, r;
                     tie(l, r) = pair;
                     return char_to_hex(hex_to_char(l) ^ hex_to_char(r));
                   });
  return result;
}
