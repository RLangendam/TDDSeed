#include <catch2/catch.hpp>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <numeric>

#include "lib.h"

SCENARIO("Challenge 1") {
  GIVEN("Set 1") {
    CHECK(
        "SSdtIGtpbGxpbmcgeW91ciBicmFpbiBsaWtlIGEgcG9pc29ub3VzIG11c2hyb29t" ==
        hex_to_base64("49276d206b696c6c696e6720796f757220627261696e206c696b6520"
                      "6120706f69736f6e6f7573206d757368726f6f6d"));
  }
  GIVEN("Set 2") {
    CHECK("746865206b696420646f6e277420706c6179" ==
          hex_xor("1c0111001f010100061a024b53535009181c",
                  "686974207468652062756c6c277320657965"));
  }
  GIVEN("Set 3") {
    auto const [message, key] = crack(
        "1b37373331363f78151b7f2b783431333d78397828372d363c78373e783a393b3736");
    CHECK(message == "Cooking MC's like a pound of bacon");
    CHECK(key == 0x58);
  }
  GIVEN("Set 4") { CHECK(crack_file() == "Now that the party is jumping\n"); }
  GIVEN("Set 5") {
    std::string const message{
        "Burning 'em, if you ain't quick and nimble\nI go crazy when "
        "I hear a cymbal"};
    CHECK(encrypt(message, "ICE") ==
          "0b3637272a2b2e63622c2e69692a23693a2a3c6324202"
          "d623d63343c2a2622632427"
          "2765272a282b2f20430a652e2c652a3124333a653e2b2"
          "027630c692b202831652863"
          "26302e27282f");
  }
}
