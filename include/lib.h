#pragma once

#include <string>
#include <tuple>
#include <vector>

std::string hex_to_base64(std::string const &hex);
std::string hex_xor(std::string const &left, std::string const &right);
std::tuple<std::string, char> crack(std::string const &message);
std::string crack_file_4();
std::string crack_file_6();
std::string encrypt(std::string const &key, std::string const &message);
std::size_t hamming_distance(std::string const &left, std::string const &right);