#pragma once
#include <iostream>
#include <format>

#define ASSERTS_ENABLED 1


#if ASSERTS_ENABLED
struct source_location
{
    const char* file_name;
    unsigned line_number;
};

#define CUR_SOURCE_LOCATION source_location({__FILE__, __LINE__})
namespace _asserts
{
    template<typename T>
    void myAssert(bool expr, const source_location& loc, const T& description)
    {
        if (!expr)
        {
            std::cerr << std::format("Assertion Failed : {} \nfile : {}\nline : {}", description, loc.file_name, loc.line_number) << std::endl;
            std::exit(-1);
        }
    }
}

#define SM_ASSERT(expr, descr) \
        _asserts::myAssert(expr, CUR_SOURCE_LOCATION, #descr)
#else
#define SM_ASSERT(expr, descr) expr
#endif
