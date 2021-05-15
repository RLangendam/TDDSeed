#pragma once

#include <string>
#include <tuple>
#include <vector>

char hex_to_char(char c);
char char_to_hex(char c);
std::string hex_to_base64(std::string const &hex);
std::string hex_xor(std::string const &left, std::string const &right);
std::tuple<std::string, char> crack(std::string const &message);
std::string crack_file();
