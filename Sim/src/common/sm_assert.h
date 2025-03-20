#pragma once
#include <iostream>
#include <format>

#define DEBUG_ASSERT_ENABLED 0

struct source_location
{
    const char* file_name;
    unsigned line_number;
};

#define CUR_SOURCE_LOCATION source_location({__FILE__, __LINE__})
namespace _asserts
{
    template<typename T>
    void small_assert(bool expr, const source_location& loc, const T& description)
    {
        if (!expr)
        {
            std::cerr << std::format("Assertion Failed : {} \nfile : {}\nline : {}", description, loc.file_name, loc.line_number) << std::endl;
            std::exit(-1);
        }
    }
}

#if DEBUG_ASSERT_ENABLED
#define SM_ASSERT(expr, descr) \
        _asserts::small_assert(expr, CUR_SOURCE_LOCATION, #descr)
#else
#define SM_ASSERT(expr, descr) expr
#endif
