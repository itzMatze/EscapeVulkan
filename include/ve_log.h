#pragma once
#include <iostream>
#include <chrono>
#include <iomanip>

#define VE_LOGGING
#define VE_CHECKING

#define RED "\033[91m"
#define GREEN "\033[92m"
#define YELLOW "\033[93m"
#define BLUE "\033[94m"
#define PINK "\033[95m"
#define L_BLUE "\033[96m"
#define WHITE "\033[0m"

auto get_time()
{
    auto time_point = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(time_point);
    return std::put_time(std::localtime(&t), L_BLUE "[%H:%M:%S] " WHITE);
}

#if defined(VE_LOGGING)
    #define VE_LOG_CONSOLE(X) std::cout << get_time() << X
    #define VE_OK_CONSOLE(X) std::cout << get_time() << GREEN << X << WHITE
    #define VE_WARN_CONSOLE(X) std::cout << get_time() << YELLOW << X << WHITE
    #define VE_ERR_CONSOLE(X) std::cerr << get_time() << RED << X << WHITE
#else
    #define VE_LOG_CONSOLE(X)
    #define VE_OK_CONSOLE(X)
    #define VE_WARN_CONSOLE(X)
    #define VE_ERR_CONSOLE(X)
#endif

#if defined(VE_CHECKING)
    #define VE_CHECK(X,M) vk::resultCheck(X,M)
#else
    #define VE_CHECK(X,M) void(X)
#endif