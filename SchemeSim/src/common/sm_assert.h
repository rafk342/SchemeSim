#pragma once
#include <iostream>
#include <format>
#include <stacktrace>
namespace winapi
{
    #include <Windows.h>
}

#define ASSERTS_ENABLED 1


#if ASSERTS_ENABLED
struct source_location
{
    const char* file_name;
    long line_number;
};

#define CUR_SOURCE_LOCATION source_location({__FILE__, __LINE__})
namespace _asserts
{
    template<typename T>
    void myAssert(bool expr, const source_location& loc, const T& description)
    {
        if (!expr)
        {
            std::cout << "Stacktrace:\n";

            for (auto& frame : std::stacktrace::current())
            {
                std::cout << std::format("\t\t {:<80}| Line: {:<15}| File: {:<15}\n", frame.description(), frame.source_line(), frame.source_file());
            }
            winapi::MessageBoxA(nullptr, std::format("Assertion Failed: {} \nfile : {}\nline : {}\n", description, loc.file_name, loc.line_number).c_str(), "Assert", MB_ICONERROR | MB_OK);
            __debugbreak();
        }
    }
}

#define SM_ASSERT(expr, descr) \
        _asserts::myAssert(expr, CUR_SOURCE_LOCATION, #descr)
#else
#define SM_ASSERT(expr, descr) expr
#endif
