#include "lib.h"

#include <algorithm>
#include <array>
#include <boost/range/adaptor/indexed.hpp>
#include <boost/range/adaptor/sliced.hpp>
#include <boost/range/adaptor/strided.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/range/algorithm/for_each.hpp>
#include <boost/range/algorithm/transform.hpp>
#include <boost/range/combine.hpp>
#include <boost/range/irange.hpp>
#include <boost/range/istream_range.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/range/numeric.hpp>
#include <boost/stl_interfaces/iterator_interface.hpp>
#include <cctype>
#include <execution>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <set>
#include <sstream>
#include <tuple>
#include <unordered_map>
#include <valarray>
#include <vector>

using namespace std;

namespace {
class hex_byte {
 public:
  hex_byte() = default;
  static hex_byte from_char(char c) { return {c}; }
  static hex_byte from_two_chars(char first, char second) {
    return hex_byte{hex_to_char(first) * 0x10 + hex_to_char(second)};
  }
  static hex_byte from_pair_of_chars(boost::tuple<char, char> const &p) {
    return from_two_chars(boost::get<0>(p), boost::get<1>(p));
  }

  auto get_byte() const { return byte; }

  hex_byte operator^(hex_byte const &other) const {
    return {byte ^ other.byte};
  }

 private:
  hex_byte(char byte) : byte{byte} {}

  static char hex_to_char(char c) {
    if (c >= 'a' && c <= 'f') {
      return 10 + c - 'a';
    } else {
      return c - '0';
    }
  }

 private:
  char byte;
};

template <typename E, typename T>
basic_ostream<E, T> &operator<<(basic_ostream<E, T> &stream,
                                hex_byte const &hb) {
  return stream << setw(2) << setfill('0') << hex
                << static_cast<int32_t>(hb.get_byte());
}

template <typename E, typename T>
basic_istream<E, T> &operator>>(basic_istream<E, T> &stream, hex_byte &hb) {
  char first, second;
  stream >> first >> second;
  hb = hex_byte::from_two_chars(first, second);
  return stream;
}

struct repeated_chars_iterator
    : boost::stl_interfaces::iterator_interface<
          repeated_chars_iterator, random_access_iterator_tag, char, char> {
  constexpr repeated_chars_iterator(string const &first) noexcept
      : data(first) {}

  char operator*() const noexcept { return data.at(index % data.size()); }
  constexpr repeated_chars_iterator &operator+=(ptrdiff_t i) noexcept {
    index += i;
    return *this;
  }
  constexpr auto operator-(repeated_chars_iterator other) const noexcept {
    return index - other.index;
  }

 private:
  string const &data;
  difference_type index{0};
};

string stream_to_string(auto callable) {
  ostringstream stream;
  callable(stream);
  return stream.str();
}

auto to_hex_bytes(string const &message) {
  using namespace boost;
  using namespace range;
  using namespace adaptors;
  return combine(message | strided(2),
                 message | sliced(1, message.size()) | strided(2)) |
         transformed(hex_byte::from_pair_of_chars);
}

class base64_sextet {
 public:
  base64_sextet() = default;
  static base64_sextet from_sextet(char sextet) { return {sextet}; }

  char get_byte() const { return sextet; }

 private:
  base64_sextet(char sextet) : sextet{sextet} {}

 private:
  char sextet;
};

static string const base64_lookup{
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz012345"
    "6789+/"};

template <typename E, typename T>
basic_ostream<E, T> &operator<<(basic_ostream<E, T> &stream,
                                base64_sextet const &bs) {
  return stream << base64_lookup[bs.get_byte()];
}

template <typename E, typename T>
basic_istream<E, T> &operator>>(basic_istream<E, T> &stream,
                                base64_sextet &bs) {
  char letter;
  stream >> letter;
  bs = base64_sextet::from_sextet(static_cast<char>(
      distance(base64_lookup.begin(),
               find(base64_lookup.begin(), base64_lookup.end(), letter))));
  return stream;
}

template <typename I>
struct sextet_iterator
    : boost::stl_interfaces::iterator_interface<sextet_iterator<I>,
                                                forward_iterator_tag,
                                                base64_sextet, base64_sextet> {
  constexpr sextet_iterator(I &&where) noexcept : where(where) {}

  base64_sextet operator*() const noexcept {
    char result;
    if (bit_index < 3) {
      result = (*where & (0b11111100 >> bit_index)) >> (2 - bit_index);
    } else {
      constexpr auto high = 0b11111111;
      result = (*where & (high >> bit_index)) << (bit_index - 2);
      auto const n{boost::next(where)};
      if (n != I{}) {
        auto const shift{10 - bit_index};
        result |= (*n & ((high << shift) & high)) >> shift;
      }
    }
    return base64_sextet::from_sextet(result);
  }

  constexpr sextet_iterator &operator+=(ptrdiff_t i) noexcept {
    bit_index += 6 * i;
    advance(where, bit_index / 8);
    bit_index %= 8;
    return *this;
  }

  bool operator==(sextet_iterator const &other) const {
    return where == other.where && bit_index == other.bit_index;
  }

 private:
  I where;
  size_t bit_index{0};
};

template <typename T>
auto as_sextets(T &&byte_range) {
  using I = sextet_iterator<decltype(boost::begin(byte_range))>;
  return boost::make_iterator_range(I{boost::begin(byte_range)},
                                    I{boost::end(byte_range)});
}

template <typename I>
struct octet_iterator
    : boost::stl_interfaces::iterator_interface<
          octet_iterator<I>, forward_iterator_tag, char, char> {
  constexpr octet_iterator(I &&where) noexcept : where(where) {}

  char operator*() const noexcept {
    char result;
    if (bit_index == 0) {
      result = where->get_byte() << 2;
      auto const n{boost::next(where)};
      if (n != I{}) result |= (n->get_byte() & 0b110000) >> 4;
    } else if (bit_index == 2) {
      result = (where->get_byte() & 0b001111) << 4;
      auto const n{boost::next(where)};
      if (n != I{}) result |= (n->get_byte() & 0b111100) >> 2;
    } else {  // bit_index == 4
      result = (where->get_byte() & 0b000011) << 6;
      auto const n{boost::next(where)};
      if (n != I{}) result |= n->get_byte();
    }
    return result;
  }

  constexpr octet_iterator &operator+=(ptrdiff_t i) noexcept {
    bit_index += 8 * i;
    advance(where, bit_index / 6);
    bit_index %= 6;
    return *this;
  }

  bool operator==(octet_iterator const &other) const {
    return where == other.where && bit_index == other.bit_index;
  }

 private:
  I where;
  size_t bit_index{0};
};

template <typename T>
auto as_octets(T &&sextet_range) {
  using I = octet_iterator<decltype(boost::begin(sextet_range))>;
  return boost::make_iterator_range(I{boost::begin(sextet_range)},
                                    I{boost::end(sextet_range)});
}
}  // namespace

string hex_to_base64(string const &hex) {
  return stream_to_string([&](ostream &stream) {
    using namespace boost;
    using namespace range;
    using namespace adaptors;
    copy(as_sextets(to_hex_bytes(hex) |
                    transformed(mem_fn(&hex_byte::get_byte))),
         ostream_iterator<base64_sextet>(stream));
  });
}

struct ci_hash {
  size_t operator()(char c) const { return hash<int>{}(tolower(c)); }
};

auto get_frequencies(istream &stream) {
  unordered_map<char, size_t, ci_hash> counts;
  {
    char c;
    while (stream >> noskipws >> c) {
      auto const found{counts.find(c)};
      if (found == counts.end())
        counts.emplace(c, 1);
      else
        ++found->second;
    }
  }
  array<tuple<char, size_t>, 255> frequencies;
  char i{0};
  for (auto &[ch, cnt] : frequencies) {
    ch = i++;
    auto const found(counts.find(ch));
    if (found == counts.end())
      cnt = 0;
    else
      cnt = found->second;
  }
  return frequencies;
}

string hex_xor(string const &left, string const &right) {
  return stream_to_string([&](ostream &stream) {
    using namespace boost;
    range::transform(combine(to_hex_bytes(left), to_hex_bytes(right)),
                     ostream_iterator<hex_byte>{stream}, [](auto const &p) {
                       return boost::get<0>(p) ^ boost::get<1>(p);
                     });
  });
}

string decrypt(string const &message, char key) {
  return stream_to_string([&](ostream &stream) {
    using namespace boost;
    using namespace range;
    using namespace adaptors;
    transform(to_hex_bytes(message), ostream_iterator<char>(stream),
              [key = hex_byte::from_char(key)](hex_byte ch) {
                return (ch ^ key).get_byte();
              });
  });
}

template <typename T>
size_t calculate_score(T const &left, T const &right) {
  return boost::inner_product(
      left, right, size_t{0}, plus<size_t>{},
      [](auto const &l, auto const &r) { return get<1>(l) * get<1>(r); });
}

auto const &get_reference() {
  static bool initialized{false};
  static decltype(get_frequencies(declval<istream &>())) reference;
  if (!initialized) {
    ifstream file{"english.txt"};
    reference = get_frequencies(file);
    initialized = true;
  }
  return reference;
}

tuple<string, char> crack(string const &message) {
  auto const &reference{get_reference()};
  string best_decrypted;
  size_t max_score{0};
  char best_key{0};
  for (uint16_t key{0}; key <= numeric_limits<char>::max(); ++key) {
    auto decrypted{decrypt(message, static_cast<char>(key))};
    istringstream stream{decrypted};
    auto const frequencies{get_frequencies(stream)};
    auto score{calculate_score(reference, frequencies)};
    if (score > max_score) {
      max_score = score;
      best_decrypted = move(decrypted);
      best_key = static_cast<char>(key);
    }
  }
  return make_tuple(best_decrypted, best_key);
}

set<char> const &get_word_characters() {
  static set<char> reference;
  static bool initialized{false};
  if (!initialized) {
    reference.emplace('\n');
    reference.emplace(' ');
    for (char c{'a'}; c <= 'z'; ++c) reference.emplace(c);
    for (char c{'A'}; c <= 'Z'; ++c) reference.emplace(c);
  }
  return reference;
}

string crack_messages(auto const &encrypted_messages) {
  enum class state { unknown, discard, use };

  return get<1>(transform_reduce(
      execution::par, encrypted_messages.begin(), encrypted_messages.end(),
      make_tuple(state::discard, string{}),
      [](tuple<state, string> &&left, tuple<state, string> &&right) {
        auto &[sl, ml] = left;
        auto &[sr, mr] = right;
        if (sl == state::discard || sr == state::use)
          return right;
        else if (sr == state::discard || sl == state::use)
          return left;
        else  // both sl and sr are state::unknown
        {
          auto const is_english{[](string const &decrypted) {
            thread_local vector<char> chars, difference;
            chars.assign(decrypted.begin(), decrypted.end());
            sort(chars.begin(), chars.end());
            chars.erase(unique(chars.begin(), chars.end()), chars.end());
            difference.clear();
            auto const &reference{get_word_characters()};
            set_difference(chars.begin(), chars.end(), reference.begin(),
                           reference.end(), back_inserter(difference));
            return difference.empty();
          }};
          if (is_english(ml))
            return make_tuple(state::use, move(ml));
          else if (is_english(mr))
            return make_tuple(state::use, move(mr));
          else
            return make_tuple(state::discard, string{});
        }
      },
      [](string const &encrypted) {
        return make_tuple(state::unknown, move(get<0>(crack(encrypted))));
      }));
}

string crack_file_4() {
  ifstream file{"4.txt"};

  array<string, 327> encrypted_messages;
  boost::copy(boost::istream_range<string>(file), encrypted_messages.begin());

  return crack_messages(encrypted_messages);
}

string encrypt(string const &message, string const &key) {
  return stream_to_string([&](ostream &stream) {
    using namespace boost;
    range::transform(
        combine(message, make_iterator_range_n(repeated_chars_iterator{key},
                                               message.size())),
        ostream_iterator<hex_byte>(stream), [](auto const &pair) {
          char l, r;
          tie(l, r) = pair;
          return hex_byte::from_char(l ^ r);
        });
  });
}

size_t hamming_distance_impl(string_view const &left,
                             string_view const &right) {
  return boost::inner_product(
      left, right, 0ull, plus<size_t>{},
      [](char l, char r) { return popcount(static_cast<uint8_t>(l ^ r)); });
}

size_t hamming_distance(string const &left, string const &right) {
  return hamming_distance_impl(left, right);
}

string crack_file_6() {
  ostringstream stream;
  {
    ifstream file{"6.txt"};
    boost::copy(as_octets(boost::istream_range<base64_sextet>(file)),
                ostream_iterator<char>(stream));
  }
  auto const encrypted{stream.str()};
  vector<tuple<float, size_t>> key_sizes(40);
  boost::range::for_each(
      key_sizes | boost::adaptors::indexed(2), [&encrypted](auto const &proxy) {
        auto &[average_distance, key_size] = proxy.value() =
            make_tuple(0.f, proxy.index());
        size_t count{0};
        for (size_t i{0}; i < 5; ++i)
          for (size_t j{i}; j < 5; ++j) {
            average_distance +=
                hamming_distance_impl(
                    string_view{next(encrypted.begin(), key_size * i),
                                next(encrypted.begin(), key_size * (i + 1))},
                    string_view{next(encrypted.begin(), key_size * j),
                                next(encrypted.begin(), key_size * (j + 1))}) /
                static_cast<float>(key_size);
            ++count;
          }
        average_distance /= count;
      });
  sort(key_sizes.begin(), key_sizes.end());
  auto const best_key_size{get<1>(key_sizes.front())};
  vector<string> blocks;
  blocks.reserve(encrypted.size() / best_key_size + 1);
  boost::transform(
      boost::irange(best_key_size), back_inserter(blocks),
      [&encrypted, best_key_size](size_t slice) {
        return stream_to_string(
            [&encrypted, slice, best_key_size](ostream &stream) {
              boost::copy(encrypted |
                              boost::adaptors::sliced(slice, encrypted.size()) |
                              boost::adaptors::strided(best_key_size),
                          ostream_iterator<char>(stream));
            });
      });
  for (auto &block : blocks)
    if (block.size() % 2) block.push_back(' ');
  return crack_messages(blocks);
}