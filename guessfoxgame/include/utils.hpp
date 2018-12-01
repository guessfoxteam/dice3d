#pragma once


#include <eosiolib/crypto.h>
#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/privileged.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/time.hpp>
#include <eosiolib/transaction.h>
#include <eosiolib/transaction.hpp>
#include <eosiolib/print.hpp>
// #include <iostream>
#include <string>
#include <cmath>


using namespace eosio;
using namespace std;


string uint64_string(uint64_t input) {
    string result;
    uint8_t base = 10;
    do {
        char c = input % base;
        input /= base;
        if (c < 10)
            c += '0';
        else
            c += 'A' - 10;
        result = c + result;
    } while (input);
    return result;
}

uint8_t from_hex(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    eosio_assert(false, "Invalid hex character");
    return 0;
}

size_t from_hex(const string& hex_str, char* out_data, size_t out_data_len) {
    auto i = hex_str.begin();
    uint8_t* out_pos = (uint8_t*)out_data;
    uint8_t* out_end = out_pos + out_data_len;
    while (i != hex_str.end() && out_end != out_pos) {
        *out_pos = from_hex((char)(*i)) << 4;
        ++i;
        if (i != hex_str.end()) {
            *out_pos |= from_hex((char)(*i));
            ++i;
        }
        ++out_pos;
    }
    return out_pos - (uint8_t*)out_data;
}

string to_hex(const char* d, uint32_t s) {
    std::string r;
    const char* to_hex = "0123456789abcdef";
    uint8_t* c = (uint8_t*)d;
    for (uint32_t i = 0; i < s; ++i)
        (r += to_hex[(c[i] >> 4)]) += to_hex[(c[i] & 0x0f)];
    return r;
}

string sha256_to_hex(const capi_checksum256& sha256) {
    return to_hex((char*)sha256.hash, sizeof(sha256.hash));
}

string sha1_to_hex(const capi_checksum160& sha1) {
    return to_hex((char*)sha1.hash, sizeof(sha1.hash));
}

// copied from boost https://www.boost.org/
template <class T>
inline void hash_combine(std::size_t& seed, const T& v) {
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

uint64_t uint64_hash(const string& hash) {
    return std::hash<string>{}(hash);
}

uint64_t uint64_hash(const capi_checksum256& hash) {
    return uint64_hash(sha256_to_hex(hash));
}

capi_checksum256 hex_to_sha256(const string& hex_str) {
    eosio_assert(hex_str.length() == 64, "invalid sha256");
    capi_checksum256 checksum;
    from_hex(hex_str, (char*)checksum.hash, sizeof(checksum.hash));
    return checksum;
}

capi_checksum160 hex_to_sha1(const string& hex_str) {
    eosio_assert(hex_str.length() == 40, "invalid sha1");
    capi_checksum160 checksum;
    from_hex(hex_str, (char*)checksum.hash, sizeof(checksum.hash));
    return checksum;
}

size_t sub2sep(const string& input,
               string* output,
               const char& separator,
               const size_t& first_pos = 0,
               const bool& required = false) {
    eosio_assert(first_pos != string::npos, "invalid first pos");
    auto pos = input.find(separator, first_pos);
    if (pos == string::npos) {
        eosio_assert(!required, "parse memo error");
        return string::npos;
    }
    *output = input.substr(first_pos, pos - first_pos);
    return pos;
}


void split(std::vector<std::string>& results, const std::string& memo,
               char separator) {
  auto start = memo.cbegin();
  auto end = memo.cend();

  for (auto it = start; it != end; ++it) {
    if (*it == separator) {
      results.emplace_back(start, it);
      start = it + 1;
    }
  }
  if (start != end) results.emplace_back(start, end);
}


/*
vector<string> split(const string& str, const string& delim)
{
    vector<string> tokens;
    size_t prev = 0, pos = 0;

    do
    {
        pos = str.find(delim, prev);
        if (pos == string::npos) pos = str.length();
        string token = str.substr(prev, pos-prev);
        tokens.push_back(token);
        prev = pos + delim.length();
    }
    while (pos < str.length() && prev < str.length());
    return tokens;
}
*/

