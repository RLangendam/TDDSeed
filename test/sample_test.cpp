#include <catch2/catch.hpp>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string_view>

#include "lib.h"
namespace std {

template <typename E, typename T>
basic_ostream<E, T>& operator<<(basic_ostream<E, T>& stream, byte b) {
  return stream << to_integer<int>(b);
}

template <typename E, typename T, typename X>
basic_ostream<E, T>& operator<<(basic_ostream<E, T>& stream,
                                vector<X> const& v) {
  for (auto x : v) stream << x << ", ";
  return stream;
}

template <typename C, typename E, class TupType, size_t... I>
void print(basic_ostream<C, E>& stream, const TupType& _tup,
           index_sequence<I...>) {
  stream << "(";
  (..., (stream << (I == 0 ? "" : ", ") << get<I>(_tup)));
  stream << ")\n";
}

template <typename C, typename E, typename... T>
basic_ostream<C, E>& operator<<(basic_ostream<C, E>& stream,
                                const tuple<T...>& _tup) {
  print(stream, _tup, make_index_sequence<sizeof...(T)>());
  return stream;
}

}  // namespace std

SCENARIO("Set 1") {
  GIVEN("Challenge 1") {
    CHECK(
        "SSdtIGtpbGxpbmcgeW91ciBicmFpbiBsaWtlIGEgcG9pc29ub3VzIG11c2hyb29t" ==
        hex_to_base64("49276d206b696c6c696e6720796f757220627261696e206c696b6520"
                      "6120706f69736f6e6f7573206d757368726f6f6d"));
  }
  GIVEN("Challenge 2") {
    CHECK("746865206b696420646f6e277420706c6179" ==
          hex_xor("1c0111001f010100061a024b53535009181c",
                  "686974207468652062756c6c277320657965"));
  }
  GIVEN("Challenge 3") {
    auto const [message, key] = crack(
        "1b37373331363f78151b7f2b783431333d78397828372d363c78373e783a393b3736");
    CHECK(message == "Cooking MC's like a pound of bacon");
    CHECK(key == 0x58);
  }
  GIVEN("Challenge 4") {
    CHECK(crack_file_4() == "Now that the party is jumping\n");
  }
  GIVEN("Challenge 5") {
    std::string const message{
        "Burning 'em, if you ain't quick and nimble\nI go crazy when "
        "I hear a cymbal"};
    CHECK(encrypt(message, "ICE") ==
          "0b3637272a2b2e63622c2e69692a23693a2a3c6324202d623d63343c2a2622632427"
          "2765272a282b2f20430a652e2c652a3124333a653e2b2027630c692b202831652863"
          "26302e27282f");
  }
  GIVEN("Challenge 6") {
    std::string const message{"Don't change me, please!"};
    CHECK(from_bytes(to_bytes(message)) == message);
    CHECK(from_bytes(from_hex(to_hex(to_bytes(message)))) == message);
    CHECK(from_bytes(from_base64(to_base64(to_bytes(message)))) == message);
    CHECK(37 == hamming_distance("this is a test", "wokka wokka!!!"));
    auto [key, decrypted] = crack_file_6();
    CHECK(key == "Terminator X: Bring the noise");
    CHECK(std::string_view{decrypted.c_str(), 24} ==
          "I'm back and I'm ringin'");
  }
  GIVEN("Challenge 7") {
    auto const decrypted{decrypt_file_7()};
    CHECK(std::string_view{decrypted.c_str(), 24} ==
          "I'm back and I'm ringin'");
  }
  GIVEN("Challenge 8") {
    auto const [index, similarity] = crack_file_8();
    CHECK(132 == index);
    CHECK(similarity == Approx(0.00967f).margin(0.001f));
  }
}

SCENARIO("Set 2") {
  GIVEN("Challenge 9") {
    CHECK("YELLOW SUBMARINE\x04\x04\x04\x04" == pkcs7("YELLOW SUBMARINE", 20));
  }

  GIVEN("Challenge 10") {
    auto const decrypted{decrypt_file_10()};
    CHECK(std::string_view{decrypted.c_str(), 24} ==
          "I'm back and I'm ringin'");
    }
}