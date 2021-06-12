#pragma once

#include <string>
#include <tuple>
#include <vector>

std::string hex_to_base64(std::string const &hex);
std::string hex_xor(std::string const &left, std::string const &right);
std::tuple<std::string, char> crack(std::string const &message);
std::string crack_file_4();
std::tuple<std::string, std::string> crack_file_6();
std::string encrypt(std::string const &key, std::string const &message);
std::size_t hamming_distance(std::string const &left, std::string const &right);
std::vector<std::byte> to_bytes(std::string const &message);
std::string from_bytes(std::vector<std::byte> const &bytes);
std::string to_hex(std::vector<std::byte> const &bytes);
std::vector<std::byte> from_hex(std::string const &message);
std::string to_base64(std::vector<std::byte> const &bytes);
std::vector<std::byte> from_base64(std::string const &message);
std::vector<std::vector<std::byte>> read_file_lines(
    std::string const &filename,
    std::vector<std::byte> (*decoder)(std::string const &));
std::tuple<std::size_t, float> crack_file_8();
