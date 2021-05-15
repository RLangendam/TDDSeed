#include <catch2/catch.hpp>
#include <iomanip>
#include <iostream>

#include "lib.h"

SCENARIO("Challenge 1") {
  GIVEN("Set 1") {
    CHECK(
        "SSdtIGtpbGxpbmcgeW91ciBicmFpbiBsaWtlIGEgcG9pc29ub3VzIG11c2hyb29t" ==
        hex_to_base64("49276d206b696c6c696e6720796f757220627261696e206c696b6520"
                      "6120706f69736f6e6f7573206d757368726f6f6d"));
  }
  GIVEN("Set 2") {
    CHECK("746865206B696420646F6E277420706C6179" ==
          hex_xor("1c0111001f010100061a024b53535009181c",
                  "686974207468652062756c6c277320657965"));
  }
  GIVEN("Set 3") {
    auto const [message, key] = crack(
        "1b37373331363f78151b7f2b783431333d78397828372d363c78373e783a393b3736");
    CHECK(message == "Cooking MC's like a pound of bacon");
    CHECK(key == 0x58);
  }
  GIVEN("Set 4") {
    auto const result{crack_file()};
    CHECK(result.size() == 1);
    CHECK(result.front() == "Now that the party is jumping\n");
  }
}
