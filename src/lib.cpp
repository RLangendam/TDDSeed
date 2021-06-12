#include "lib.h"

#include <algorithm>
#include <array>
#include <boost/range/adaptor/filtered.hpp>
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
#include <boost/range/join.hpp>
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
static char hex_to_char(char c) {
  if (c >= 'a' && c <= 'f') {
    return 10 + c - 'a';
  } else {
    return c - '0';
  }
}

template <typename T, int W>
struct fixed_width_value {
  fixed_width_value(T v_) : v(v_) {}
  T v;
};

template <typename T, int W>
ostream &operator<<(ostream &ostr, const fixed_width_value<T, W> &fwv) {
  return ostr << setw(W) << fwv.v;
}

template <typename E, typename T>
basic_ostream<E, T> &hex_encoding(basic_ostream<E, T> &stream) {
  return stream << setfill('0') << hex;
}

template <typename S>
auto hex_output_iterator(S &stream) {
  return ostream_iterator<fixed_width_value<int, 2>>{stream << hex_encoding};
}

struct repeated_iterator
    : boost::stl_interfaces::iterator_interface<
          repeated_iterator, random_access_iterator_tag, byte, byte> {
  constexpr repeated_iterator(vector<byte> const &first) noexcept
      : data(first) {}

  byte operator*() const noexcept { return data.at(index % data.size()); }
  constexpr repeated_iterator &operator+=(ptrdiff_t i) noexcept {
    index += i;
    return *this;
  }
  constexpr auto operator-(repeated_iterator other) const noexcept {
    return index - other.index;
  }

 private:
  vector<byte> const &data;
  difference_type index{0};
};

template <typename C>
string stream_to_string(C callable) {
  ostringstream stream;
  callable(stream);
  return stream.str();
}

auto hex_decoded(string const &message) {
  using namespace boost;
  using namespace range;
  using namespace adaptors;
  return combine(
             message | strided(2),
             join(message, message.size() % 2 ? string{} : string(1, '\0')) |
                 sliced(1, message.size() + (message.size() % 2)) |
                 strided(2)) |
         transformed([](boost::tuple<char, char> const &p) -> byte {
           return static_cast<byte>(hex_to_char(boost::get<0>(p)) * 0x10 +
                                    hex_to_char(boost::get<1>(p)));
         });
}

auto as_bytes(string const &message) {
  return message | boost::adaptors::transformed(
                       [](char c) { return static_cast<byte>(c); });
}

class base64_sextet {
 public:
  base64_sextet() = default;
  static base64_sextet from_sextet(byte sextet) { return {sextet}; }

  byte get_sextet_byte() const { return sextet; }

 private:
  base64_sextet(byte sextet) : sextet{sextet} {}

 private:
  byte sextet;
};

static string const base64_lookup{
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz012345"
    "6789+/"};

template <typename E, typename T>
basic_ostream<E, T> &operator<<(basic_ostream<E, T> &stream,
                                base64_sextet const &bs) {
  return stream << base64_lookup[to_integer<size_t>(bs.get_sextet_byte())];
}

template <typename E, typename T>
basic_istream<E, T> &operator>>(basic_istream<E, T> &stream,
                                base64_sextet &bs) {
  char letter;
  stream >> letter;
  bs = base64_sextet::from_sextet(static_cast<byte>(
      letter == '\0' ? 0
                     : distance(base64_lookup.begin(),
                                find(base64_lookup.begin(), base64_lookup.end(),
                                     letter))));
  return stream;
}

template <typename I>
struct sextet_iterator
    : boost::stl_interfaces::iterator_interface<sextet_iterator<I>,
                                                forward_iterator_tag,
                                                base64_sextet, base64_sextet> {
  constexpr sextet_iterator(I &&where) noexcept : where(where) {}
  constexpr sextet_iterator(I &&where, I &&end) noexcept
      : where(where), end(end) {}

  base64_sextet operator*() const noexcept {
    byte result;
    if (bit_index < 3) {
      result = (*where & (byte{0b11111100} >> bit_index)) >> (2 - bit_index);
    } else {
      constexpr auto high = 0b11111111;
      result = (*where & static_cast<byte>(high >> bit_index))
               << (bit_index - 2);
      auto const n{boost::next(where)};
      if (n != end) {
        auto const shift{10 - bit_index};
        result |= (*n & static_cast<byte>((high << shift) & high)) >> shift;
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
    return where == other.where;
  }

 private:
  I where;
  I end;
  size_t bit_index{0};
};

template <typename T>
auto as_sextets(T &&byte_range) {
  using I = sextet_iterator<decltype(boost::begin(byte_range))>;
  return boost::make_iterator_range(
      I{boost::begin(byte_range), boost::end(byte_range)},
      I{boost::end(byte_range)});
}

template <typename I>
struct octet_iterator
    : boost::stl_interfaces::iterator_interface<
          octet_iterator<I>, forward_iterator_tag, byte, byte> {
  constexpr octet_iterator(I &&where) noexcept : where(where) {}
  constexpr octet_iterator(I &&where, I &&end) noexcept
      : where(where), end(end) {}

  byte operator*() const noexcept {
    byte result;
    if (bit_index == 0) {
      result = where->get_sextet_byte() << 2;
      auto const n{boost::next(where)};
      if (n != end) result |= (n->get_sextet_byte() & byte{0b110000}) >> 4;
    } else if (bit_index == 2) {
      result = (where->get_sextet_byte() & byte{0b001111}) << 4;
      auto const n{boost::next(where)};
      if (n != end) result |= (n->get_sextet_byte() & byte{0b111100}) >> 2;
    } else {  // bit_index == 4
      result = (where->get_sextet_byte() & byte{0b000011}) << 6;
      auto const n{boost::next(where)};
      if (n != end) result |= n->get_sextet_byte();
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
    return where == other.where;
  }

 private:
  I where;
  I end;
  size_t bit_index{0};
};

template <typename T>
auto as_octets(T &&sextet_range) {
  using I = octet_iterator<decltype(boost::begin(sextet_range))>;
  return boost::make_iterator_range(
      I{boost::begin(sextet_range), boost::end(sextet_range)},
      I{boost::end(sextet_range)});
}
}  // namespace

string hex_to_base64(string const &hex) {
  return stream_to_string([&](ostream &stream) {
    using namespace boost;
    using namespace range;
    using namespace adaptors;
    copy(as_sextets(hex_decoded(hex)), ostream_iterator<base64_sextet>(stream));
  });
}

auto get_frequencies(vector<byte> const &data) {
  unordered_map<byte, size_t> counts;
  {
    for (auto b : data) {
      auto const found{counts.find(b)};
      if (found == counts.end())
        counts.emplace(b, 1);
      else
        ++found->second;
    }
  }
  array<tuple<byte, size_t>, numeric_limits<char>::max()> frequencies;
  {
    uint8_t i{0};
    for (auto &[b, cnt] : frequencies) {
      b = static_cast<byte>(i);
      i++;
      auto const found(counts.find(b));
      if (found == counts.end())
        cnt = 0;
      else
        cnt = found->second;
    }
  }
  return frequencies;
}

string hex_xor(string const &left, string const &right) {
  return stream_to_string([&](ostream &stream) {
    using namespace boost;
    range::transform(
        combine(hex_decoded(left), hex_decoded(right)),
        hex_output_iterator(stream), [](auto const &p) {
          return to_integer<int>(boost::get<0>(p) ^ boost::get<1>(p));
        });
  });
}

auto decrypt(vector<byte> const &message, byte key) {
  vector<byte> decrypted;
  decrypted.reserve(message.size());
  boost::range::transform(message, back_inserter(decrypted),
                          [key](byte b) { return b ^ key; });
  return decrypted;
}

template <typename T>
size_t calculate_score(T const &left, T const &right) {
  return boost::inner_product(
      left, right, size_t{0}, plus<size_t>{},
      [](auto const &l, auto const &r) { return get<1>(l) * get<1>(r); });
}

auto const &get_reference() {
  static bool initialized{false};
  static decltype(get_frequencies(declval<vector<byte> const &>())) reference;
  if (!initialized) {
    vector<byte> bytes;
    {
      wifstream file{"english.txt"};
      boost::transform(boost::istream_range<wchar_t>(file >> noskipws) |
                           boost::adaptors::filtered([](wchar_t c) {
                             return c >= numeric_limits<uint8_t>::min() &&
                                    c <= numeric_limits<uint8_t>::max();
                           }),
                       back_inserter(bytes),
                       [](wchar_t c) { return static_cast<byte>(c); });
    }
    reference = get_frequencies(bytes);
    initialized = true;
  }
  return reference;
}

tuple<vector<byte>, byte> crack_impl(vector<byte> const &message) {
  auto const &reference{get_reference()};
  vector<byte> best_decrypted;
  size_t max_score{0};
  byte best_key{0};
  constexpr uint16_t max_key{numeric_limits<char>::max()};
  for (uint16_t key{0}; key <= max_key; ++key) {
    auto decrypted{decrypt(message, static_cast<byte>(key))};
    auto const frequencies{get_frequencies(decrypted)};
    auto score{calculate_score(reference, frequencies)};
    if (score > max_score) {
      max_score = score;
      best_decrypted = move(decrypted);
      auto x = from_bytes(best_decrypted);
      best_key = static_cast<byte>(key);
    }
  }
  return make_tuple(best_decrypted, best_key);
}

tuple<string, char> crack(string const &message) {
  auto const bytes{from_hex(message)};
  auto const [cracked_bytes, key] = crack_impl(bytes);
  return make_tuple(from_bytes(cracked_bytes), to_integer<char>(key));
}

set<byte> const &get_word_characters() {
  static set<byte> reference;
  static bool initialized{false};
  if (!initialized) {
    reference.emplace(byte{'\n'});
    reference.emplace(byte{' '});
    for (uint8_t c{'a'}; c <= 'z'; ++c) reference.emplace(byte{c});
    for (uint8_t c{'A'}; c <= 'Z'; ++c) reference.emplace(byte{c});
  }
  return reference;
}

auto crack_messages(vector<vector<byte>> const &encrypted_messages) {
  enum class state { unknown, discard, use };

  return transform_reduce(
      execution::par, encrypted_messages.begin(), encrypted_messages.end(),
      make_tuple(state::discard, vector<byte>{}, byte{0}),
      [](auto &&left, auto &&right) {
        auto &[sl, ml, kl] = left;
        auto &[sr, mr, kr] = right;
        if (sl == state::discard || sr == state::use)
          return right;
        else if (sr == state::discard || sl == state::use)
          return left;
        else  // both sl and sr are state::unknown
        {
          auto const is_english{[](vector<byte> const &decrypted) {
            thread_local vector<byte> chars, difference;
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
            return make_tuple(state::use, move(ml), kl);
          else if (is_english(mr))
            return make_tuple(state::use, move(mr), kr);
          else
            return make_tuple(state::discard, vector<byte>{}, byte{0});
        }
      },
      [](vector<byte> const &encrypted) {
        return tuple_cat(make_tuple(state::unknown), crack_impl(encrypted));
      });
}

string crack_file_4() {
  vector<vector<byte>> encrypted_messages;
  {
    ifstream file{"4.txt"};
    boost::transform(boost::istream_range<string>(file),
                     back_inserter(encrypted_messages), from_hex);
  }
  return from_bytes(get<1>(crack_messages(encrypted_messages)));
}

string encrypt(string const &message, string const &key) {
  return stream_to_string([&](ostream &stream) {
    using namespace boost;
    auto const key_bytes{to_bytes(key)};
    range::transform(
        combine(as_bytes(message),
                make_iterator_range_n(repeated_iterator{key_bytes},
                                      message.size())),
        hex_output_iterator(stream), [](boost::tuple<byte, byte> const &p) {
          return to_integer<int>(boost::get<0>(p) ^ boost::get<1>(p));
        });
  });
}

template <typename R1, typename R2>
size_t hamming_distance_impl(R1 const &left, R2 const &right) {
  return boost::inner_product(
      left, right, 0ull, plus<size_t>{},
      [](byte l, byte r) { return popcount(to_integer<uint8_t>(l ^ r)); });
}

size_t hamming_distance(string const &left, string const &right) {
  return hamming_distance_impl(left | boost::adaptors::transformed([](char c) {
                                 return static_cast<byte>(c);
                               }),
                               right | boost::adaptors::transformed([](char c) {
                                 return static_cast<byte>(c);
                               }));
}

vector<byte> to_bytes(string const &message) {
  vector<byte> bytes;
  bytes.reserve(message.size());
  boost::range::transform(message, back_inserter(bytes),
                          [](char c) { return static_cast<byte>(c); });
  return bytes;
}

string from_bytes(vector<byte> const &bytes) {
  return stream_to_string([&](ostream &stream) {
    boost::range::transform(bytes, ostream_iterator<char>(stream),
                            [](byte b) { return static_cast<char>(b); });
  });
}

string to_hex(vector<byte> const &bytes) {
  return stream_to_string([&](ostream &stream) {
    boost::range::transform(bytes, hex_output_iterator(stream),
                            &to_integer<int>);
  });
}

vector<byte> from_hex(string const &message) {
  vector<byte> bytes;
  bytes.reserve(message.size() / 2);
  boost::range::copy(hex_decoded(message), back_inserter(bytes));
  return bytes;
}

string to_base64(vector<byte> const &bytes) {
  return stream_to_string([&](ostream &stream) {
    boost::range::copy(as_sextets(bytes),
                       ostream_iterator<base64_sextet>(stream));
  });
}

template <typename S>
auto from_base64_impl(S &stream) {
  vector<byte> bytes;
  vector<base64_sextet> sextets;
  boost::copy(boost::istream_range<base64_sextet>(stream),
              back_inserter(sextets));
  boost::range::copy(as_octets(sextets), back_inserter(bytes));
  return bytes;
}

vector<byte> from_base64(string const &message) {
  istringstream stream{message};
  return from_base64_impl(stream);
}

vector<vector<byte>> transpose(vector<byte> const &data, size_t stride) {
  vector<vector<byte>> transposed;
  transposed.reserve(data.size() / stride + 1);
  boost::transform(boost::irange(stride), back_inserter(transposed),
                   [&data, stride](size_t slice) {
                     vector<byte> bytes;
                     boost::copy(
                         data | boost::adaptors::sliced(slice, data.size()) |
                             boost::adaptors::strided(stride),
                         back_inserter(bytes));
                     return bytes;
                   });
  return transposed;
}

tuple<string, string> crack_file_6() {
  vector<byte> encrypted;
  {
    ifstream file{"6.txt"};
    encrypted = from_base64_impl(file);
  }
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
                    boost::make_iterator_range(
                        next(encrypted.begin(), key_size * i),
                        next(encrypted.begin(), key_size * (i + 1))),
                    boost::make_iterator_range(
                        next(encrypted.begin(), key_size * j),
                        next(encrypted.begin(), key_size * (j + 1)))) /
                static_cast<float>(key_size);
            ++count;
          }
        average_distance /= count;
      });
  sort(key_sizes.begin(), key_sizes.end());
  auto const best_key_size{get<1>(key_sizes.front())};
  auto blocks{transpose(encrypted, best_key_size)};
  for (auto &block : blocks)
    if (block.size() % 2) block.emplace_back();

  vector<tuple<vector<byte>, byte>> decrypted;
  boost::transform(blocks, back_inserter(decrypted), crack_impl);
  ostringstream key_stream;
  vector<vector<byte>> decrypted_blocks;
  for (auto &[d, k] : decrypted) {
    key_stream << to_integer<char>(k);
    decrypted_blocks.emplace_back(move(d));
  }

  auto const shortest_block_size{
      min_element(decrypted_blocks.begin(), decrypted_blocks.end(),
                  [](auto const &left, auto const &right) {
                    return left.size() < right.size();
                  })
          ->size()};

  vector<byte> decrypted2;
  decrypted2.reserve(
      accumulate(decrypted_blocks.begin(), decrypted_blocks.end(), 0ull,
                 [](auto m, auto const &b) { return m + b.size(); }));
  for (auto i{0u}; i < shortest_block_size; ++i)
    for (auto const &b : decrypted_blocks) {
      decrypted2.emplace_back(b[i]);
    }

  return make_tuple(key_stream.str(), from_bytes(decrypted2));
}