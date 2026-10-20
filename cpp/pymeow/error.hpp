/*
 * pyMeow C++ -- error types
 * https://github.com/qb-0/pyMeow
 */
#ifndef PYMEOW_ERROR_HPP
#define PYMEOW_ERROR_HPP

#include "defs.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>

namespace pymeow {

/* Base class of every exception thrown by pyMeow. */
struct PYMEOW_API Error : std::runtime_error {
    explicit Error(const std::string& what) : std::runtime_error(what) {}
};

/* Raised when a process / module / window / font could not be found. */
struct PYMEOW_API NotFoundError : Error {
    explicit NotFoundError(const std::string& what) : Error(what) {}
};

/* Raised when reading or writing remote memory failed
 * (equivalent of the Python AccessViolationDefect / memory error). */
struct PYMEOW_API MemoryError : Error {
    std::uintptr_t address;
    int code;

    MemoryError(const std::string& operation, std::uintptr_t addr, int errCode, const std::string& errText)
        : Error(operation + " failed [Address: 0x" + toHex(addr) + "] [Error: " + std::to_string(errCode) + " - " + errText +
                "]"),
          address(addr), code(errCode) {}

    static std::string toHex(std::uintptr_t value) {
        static const char* digits = "0123456789ABCDEF";
        if (value == 0) return "0";
        char buffer[24];
        int i = static_cast<int>(sizeof(buffer));
        buffer[--i] = '\0';
        while (value != 0 && i > 0) {
            buffer[--i] = digits[value & 0xF];
            value >>= 4;
        }
        return std::string(&buffer[i]);
    }
};

/* Raised by worldToScreen() when the position is behind the camera.
 * Use worldToScreenNoExc() if you prefer a flag over an exception. */
struct PYMEOW_API OutOfBoundsError : Error {
    explicit OutOfBoundsError(const std::string& what = "2D Position out of bounds") : Error(what) {}
};

/* Raised for invalid arguments (AOB pattern, range, font id, ...). */
struct PYMEOW_API InvalidArgumentError : Error {
    explicit InvalidArgumentError(const std::string& what) : Error(what) {}
};

/* Last OS error of the current thread: (code, message). */
PYMEOW_API std::pair<int, std::string> getOsError();
PYMEOW_API std::string errorString();

} /* namespace pymeow */

#endif /* PYMEOW_ERROR_HPP */
