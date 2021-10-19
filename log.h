#pragma once

#include <iostream>
#include <sstream>
#include <string>

namespace NLogging {

////////////////////////////////////////////////////////////////////////////////

constexpr int verbosity = 3;

inline void log(
    int level,
    const std::string& tag,
    const std::string& message)
{
    if (level <= verbosity) {
        // TODO log current time
        std::cerr << "[" << tag << "] " << message << "\n";
    }
}

inline void log(
    int level,
    const std::string& tag,
    const std::stringstream& message)
{
    log(level, tag, message.str());
}

template <class T>
void log_debug(const T& message)
{
    log(4, "D", message);
}

template <class T>
void log_info(const T& message)
{
    log(3, "I", message);
}

template <class T>
void log_error(const T& message)
{
    log(1, "E", message);
}

inline void verify(bool condition, const std::string& message)
{
    if (!condition) {
        log_error(message);

        abort();
    }
}

}   // namespace NLogging
