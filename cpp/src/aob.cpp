/*
 * pyMeow C++ -- AOB scanner implementation
 * https://github.com/qb-0/pyMeow
 *
 * Port of aob1() / aob2() from memcore.nim (credits to Iago Beuller for the
 * original smart match order) with two fixes:
 *   - half wildcards ("?B", "B?") are compared numerically, so the case of the
 *     hex digits in the pattern no longer matters
 *   - degenerate patterns (a single concrete byte, only wildcards, ...) can no
 *     longer crash the match order builder
 */
#include "pymeow/internal/aob.hpp"

#include "pymeow/error.hpp"

#include <algorithm>
#include <cctype>

namespace pymeow {
namespace detail {

namespace {

int hexDigitValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

bool isWildcardChar(char c) { return c == '?'; }

inline bool tokenMatches(const AobToken& token, std::uint8_t byte) {
    return static_cast<std::uint8_t>(byte & token.mask) == static_cast<std::uint8_t>(token.value & token.mask);
}

void removeValue(std::vector<int>& values, int value) {
    const auto it = std::find(values.begin(), values.end(), value);
    if (it != values.end()) values.erase(it);
}

void dedupe(std::vector<int>& values) {
    std::vector<int> unique;
    unique.reserve(values.size());
    for (int v : values) {
        if (std::find(unique.begin(), unique.end(), v) == unique.end()) unique.push_back(v);
    }
    values.swap(unique);
}

/* Check order of aob1: the last concrete byte first, then the middle one, then
 * the remaining tokens. Rejecting most candidates after one or two compares is
 * what makes the scan fast. */
std::vector<int> buildMatchOrder(const std::vector<AobToken>& tokens) {
    std::vector<int> order;
    const int count = static_cast<int>(tokens.size());

    if (count > 2) {
        const int middleIndex = (count / 2) - 1;
        int midIndex = 0;
        int lastIndex = 0;
        for (int i = 0; i < count; ++i) {
            if (tokens[static_cast<std::size_t>(i)].mask == 0) continue; /* "??" is never compared */
            if (tokens[static_cast<std::size_t>(i)].mask == 0xFF) {       /* concrete byte */
                if (i <= middleIndex || midIndex == 0) midIndex = i;
                lastIndex = i;
            }
            order.push_back(i);
        }
        if (order.empty()) return order;

        if (lastIndex == 0) {
            lastIndex = order.back();
            order.pop_back();
        } else {
            removeValue(order, lastIndex);
        }
        removeValue(order, midIndex);

        if (lastIndex != midIndex) {
            const std::size_t midPos = order.size() < 1 ? order.size() : 1;
            order.insert(order.begin() + static_cast<std::ptrdiff_t>(midPos), midIndex);
        }
        order.insert(order.begin(), lastIndex);
    } else {
        for (int i = 0; i < count; ++i) {
            if (tokens[static_cast<std::size_t>(i)].mask != 0) order.push_back(i);
        }
    }

    dedupe(order);
    return order;
}

std::vector<std::size_t> scanSmart(const std::uint8_t* data, std::size_t size, const AobPattern& pattern, bool single) {
    std::vector<std::size_t> result;
    if (pattern.empty() || size < pattern.size() || pattern.matchOrder.empty()) return result;

    const std::size_t lastStart = size - pattern.size();
    for (std::size_t i = 0; i <= lastStart; ++i) {
        bool found = true;
        for (int index : pattern.matchOrder) {
            if (!tokenMatches(pattern.tokens[static_cast<std::size_t>(index)], data[i + static_cast<std::size_t>(index)])) {
                found = false;
                break;
            }
        }
        if (found) {
            result.push_back(i);
            if (single) break;
        }
    }
    return result;
}

std::vector<std::size_t> scanSimple(const std::uint8_t* data, std::size_t size, const AobPattern& pattern, bool single) {
    std::vector<std::size_t> result;
    if (pattern.empty() || size < pattern.size()) return result;

    const std::size_t lastStart = size - pattern.size();
    for (std::size_t i = 0; i <= lastStart; ++i) {
        std::size_t matched = 0;
        while (matched < pattern.size() && tokenMatches(pattern.tokens[matched], data[i + matched])) ++matched;
        if (matched == pattern.size()) {
            result.push_back(i);
            if (single) break;
        }
    }
    return result;
}

} /* namespace */

AobPattern parseAobPattern(const std::string& pattern) {
    std::string compact;
    compact.reserve(pattern.size());
    for (char c : pattern) {
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') continue;
        compact += c;
    }
    if (compact.empty()) throw InvalidArgumentError("Invalid pattern: empty");
    if (compact.size() % 2 != 0)
        throw InvalidArgumentError("Invalid pattern: '" + pattern + "' (odd number of hex characters)");

    AobPattern result;
    result.tokens.reserve(compact.size() / 2);
    for (std::size_t i = 0; i < compact.size(); i += 2) {
        const char hi = compact[i];
        const char lo = compact[i + 1];
        const bool hiWild = isWildcardChar(hi);
        const bool loWild = isWildcardChar(lo);
        AobToken token;
        if (hiWild && loWild) {
            token.value = 0;
            token.mask = 0x00;
        } else if (hiWild) {
            const int low = hexDigitValue(lo);
            if (low < 0) throw InvalidArgumentError("Invalid pattern: '" + pattern + "'");
            token.value = static_cast<std::uint8_t>(low);
            token.mask = 0x0F;
        } else if (loWild) {
            const int high = hexDigitValue(hi);
            if (high < 0) throw InvalidArgumentError("Invalid pattern: '" + pattern + "'");
            token.value = static_cast<std::uint8_t>(high << 4);
            token.mask = 0xF0;
        } else {
            const int high = hexDigitValue(hi);
            const int low = hexDigitValue(lo);
            if (high < 0 || low < 0) throw InvalidArgumentError("Invalid pattern: '" + pattern + "'");
            token.value = static_cast<std::uint8_t>((high << 4) | low);
            token.mask = 0xFF;
        }
        result.tokens.push_back(token);
    }
    result.matchOrder = buildMatchOrder(result.tokens);
    return result;
}

std::vector<std::size_t> aobScan(const std::uint8_t* data, std::size_t size, const AobPattern& pattern, bool single,
                                 int algorithm) {
    if (data == nullptr || size == 0) return std::vector<std::size_t>();
    return (algorithm == 0) ? scanSmart(data, size, pattern, single) : scanSimple(data, size, pattern, single);
}

std::vector<std::size_t> aobScanBruteForce(const std::uint8_t* data, std::size_t size, const AobPattern& pattern, bool single) {
    std::vector<std::size_t> result;
    if (data == nullptr || pattern.empty() || size < pattern.size()) return result;
    const std::size_t lastStart = size - pattern.size();
    for (std::size_t i = 0; i <= lastStart; ++i) {
        bool found = true;
        for (std::size_t t = 0; t < pattern.size(); ++t) {
            const AobToken& token = pattern.tokens[t];
            if ((data[i + t] & token.mask) != (token.value & token.mask)) {
                found = false;
                break;
            }
        }
        if (found) {
            result.push_back(i);
            if (single) break;
        }
    }
    return result;
}

} /* namespace detail */
} /* namespace pymeow */
