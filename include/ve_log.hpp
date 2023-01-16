#pragma once
#include <chrono>
#include <iomanip>
#include <iostream>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>

#define VE_C_RED "\033[91m"
#define VE_C_GREEN "\033[92m"
#define VE_C_YELLOW "\033[93m"
#define VE_C_BLUE "\033[94m"
#define VE_C_PINK "\033[95m"
#define VE_C_LBLUE "\033[96m"
#define VE_C_WHITE "\033[0m"

#define VE_CHECKING

namespace ve
{
    inline std::string to_string(double number, uint32_t precision = 2)
    {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(precision) << number;
        return ss.str();
    }
}// namespace ve

#define VE_THROW(...)                         \
    {                                         \
        spdlog::error(__VA_ARGS__);           \
        std::string s(__FILE__);              \
        s.append(": ");                       \
        s.append(std::to_string(__LINE__));   \
        spdlog::throw_spdlog_ex(s);           \
    }

#if defined(VE_CHECKING)
#define VE_ASSERT(X, ...) if (!(X)) VE_THROW(__VA_ARGS__);
#define VE_CHECK(X, M) vk::resultCheck(X, M)
#else
#define VE_ASSERT(X, ...) X
#define VE_CHECK(X, M) void(X)
#endif