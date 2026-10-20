/*
 * pyMeow C++ -- internal array of bytes (AOB) scanner
 * https://github.com/qb-0/pyMeow
 *
 * Internal header: not part of the public API (see memcore.hpp for aobScan*).
 *
 * Pattern syntax (identical to the Python module):
 *   "89 01 48 8B 43"   exact bytes, spaces are optional
 *   "89 ?? 48"         ?? is a full wildcard
 *   "89 ?B 48"         ?B matches the low nibble only
 *   "8B C? 48"         C? matches the high nibble only
 *
 * Algorithm 0 (default) checks the last and the middle concrete byte of the
 * pattern first ("smart" order, ported from aob1 in memcore.nim), algorithm 1
 * is a plain left to right compare (aob2).
 */
#ifndef PYMEOW_INTERNAL_AOB_HPP
#define PYMEOW_INTERNAL_AOB_HPP

#include "../common.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace pymeow {
namespace detail {

struct AobToken {
    std::uint8_t value = 0; /* expected byte (masked)                     */
    std::uint8_t mask = 0;  /* 0xFF exact, 0x0F low nibble, 0xF0 high nibble, 0x00 wildcard */
};

struct AobPattern {
    std::vector<AobToken> tokens;
    std::vector<int> matchOrder; /* indices into tokens, most selective first */
    std::size_t size() const { return tokens.size(); }
    bool empty() const { return tokens.empty(); }
};

/* Parses the pattern, throws InvalidArgumentError for malformed input. */
PYMEOW_API AobPattern parseAobPattern(const std::string& pattern);

/* Returns offsets (not absolute addresses) of every match inside data. */
PYMEOW_API std::vector<std::size_t> aobScan(const std::uint8_t* data, std::size_t size, const AobPattern& pattern,
                                            bool single, int algorithm = 0);

/* Reference implementation used by the unit tests. */
PYMEOW_API std::vector<std::size_t> aobScanBruteForce(const std::uint8_t* data, std::size_t size, const AobPattern& pattern,
                                                      bool single);

} /* namespace detail */
} /* namespace pymeow */

#endif /* PYMEOW_INTERNAL_AOB_HPP */
