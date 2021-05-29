#include "lib.h"

#include <algorithm>
#include <array>
#include <boost/range/adaptor/sliced.hpp>
#include <boost/range/adaptor/strided.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/range/algorithm/transform.hpp>
#include <boost/range/combine.hpp>
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

string hex_to_string(string const &hex) {
  stringstream stream;
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

string hex_xor(string const &left, string const &right) {
  using namespace boost;
  string result;
  range::transform(combine(left, right), back_inserter(result),
                   [](auto const &pair) {
                     char l, r;
                     tie(l, r) = pair;
                     return char_to_hex(hex_to_char(l) ^ hex_to_char(r));
                   });
  return result;
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

auto hex_to_bytes(string const &message) {
  using namespace boost;
  using namespace range;
  using namespace adaptors;
  return combine(message | strided(2),
                 message | sliced(1, message.size()) | strided(2)) |
         transformed([](auto const &chars) -> char {
           char first, second;
           tie(first, second) = chars;
           return hex_to_char(first) * 16 + hex_to_char(second);
         });
}

string decrypt(string const &message, char key) {
  using namespace boost;
  using namespace range;
  using namespace adaptors;
  string result;
  transform(hex_to_bytes(message), back_inserter(result),
            [key](char ch) -> char { return ch ^ key; });
  return result;
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

string crack_file() {
  set<char> reference;
  reference.emplace('\n');
  reference.emplace(' ');
  for (char c{'a'}; c <= 'z'; ++c) reference.emplace(c);
  for (char c{'A'}; c <= 'Z'; ++c) reference.emplace(c);

  ifstream file{"4.txt"};

  array<string, 327> encrypted_messages;
  boost::copy(boost::istream_range<string>(file), encrypted_messages.begin());

  enum class state { unknown, discard, use };

  return get<1>(transform_reduce(
      execution::par, encrypted_messages.begin(), encrypted_messages.end(),
      make_tuple(state::discard, string{}),
      [&reference = as_const(reference)](tuple<state, string> &&left,
                                         tuple<state, string> &&right) {
        auto &[sl, ml] = left;
        auto &[sr, mr] = right;
        if (sl == state::discard || sr == state::use)
          return right;
        else if (sr == state::discard || sl == state::use)
          return left;
        else  // both sl and sr are state::unknown
        {
          auto const is_english{[&reference](string const &decrypted) {
            thread_local vector<char> chars, difference;
            chars.assign(decrypted.begin(), decrypted.end());
            sort(chars.begin(), chars.end());
            chars.erase(unique(chars.begin(), chars.end()), chars.end());
            difference.clear();
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

namespace {
struct repeated_chars_iterator
    : boost::stl_interfaces::iterator_interface<repeated_chars_iterator,
                                                std::random_access_iterator_tag,
                                                char, char> {
  constexpr repeated_chars_iterator(string const &first) noexcept
      : data(first) {}

  char operator*() const noexcept { return data.at(index % data.size()); }
  constexpr repeated_chars_iterator &operator+=(std::ptrdiff_t i) noexcept {
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

}  // namespace

string byte_to_hex(char byte) {
  ostringstream stream;
  stream << setw(2) << setfill('0') << hex << static_cast<int32_t>(byte);
  return stream.str();
}

string encrypt(string const &message, string const &key) {
  ostringstream stream;
  using namespace boost;
  range::transform(
      combine(message, boost::make_iterator_range_n(
                           repeated_chars_iterator{key}, message.size())),
      ostream_iterator<string>(stream), [](auto const &pair) {
        char l, r;
        tie(l, r) = pair;
        return byte_to_hex(l ^ r);
      });
  return stream.str();
}