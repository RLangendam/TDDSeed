#include <catch2/catch.hpp>
#include <iomanip>
#include <iostream>

#include "lib.h"

std::string hex_to_string(std::string const &hex) {
  std::stringstream stream;
  unsigned char c{0};
  bool high{false};
  for (char each : hex) {
    if (high) {
      c |= hex_to_char(each);
      stream << c;
      high = false;
    } else {
      c = hex_to_char(each) << 4;
      high = true;
    }
  }
  return stream.str();
}

TEST_CASE("set 1") {
  CHECK("SSdtIGtpbGxpbmcgeW91ciBicmFpbiBsaWtlIGEgcG9pc29ub3VzIG11c2hyb29t" ==
        hex_to_base64("49276d206b696c6c696e6720796f757220627261696e206c696b6520"
                      "6120706f69736f6e6f7573206d757368726f6f6d"));
  CHECK("746865206B696420646F6E277420706C6179" ==
        hex_xor("1c0111001f010100061a024b53535009181c",
                "686974207468652062756c6c277320657965"));
  CHECK("Cooking MC's like a pound of bacon" ==
        hex_to_string(hex_xor("585858585858585858585858585858585858585858585858"
                              "58585858585858585858",
                              "1b37373331363f78151b7f2b783431333d78397828372d36"
                              "3c78373e783a393b3736")));
}

// TEST_CASE("inverses") {
//   for (char c{std::numeric_limits<char>::min()};
//        c < std::numeric_limits<char>::max(); ++c) {
//     CHECK(c == hex_to_char(char_to_hex(c)));
//     CHECK(c == char_to_hex(hex_to_char(c)));
//   }
// }
