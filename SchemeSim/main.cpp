#include <iostream>
#include <thread>
#include <functional>
#include <print>

#include "core/SFMLRenderer.h"
#include <windows.h>
#include <format>
#include "helpers/Helpers.h"

//void* operator new(size_t sz) noexcept
//{
//	//Utils::printStackTrace();
// 
//	static size_t count = 0;
//	count++;
//	std::cout << " i : " << count << " alloc : " << sz << std::endl;
//	return malloc(sz);
//}

//int main()
//{
//    SFMLRenderer* renderer = SFMLRenderer::Create();
//    renderer->Init();
//    renderer->OnRender();
//    SFMLRenderer::Destroy();
//    return 0;
//}

inline const char* GetExceptionName(DWORD code)
{
	switch (code)
	{
		case EXCEPTION_ACCESS_VIOLATION          : return "EXCEPTION_ACCESS_VIOLATION";
		case EXCEPTION_DATATYPE_MISALIGNMENT     : return "EXCEPTION_DATATYPE_MISALIGNMENT";
		case EXCEPTION_BREAKPOINT                : return "EXCEPTION_BREAKPOINT";
		case EXCEPTION_SINGLE_STEP               : return "EXCEPTION_SINGLE_STEP";
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED     : return "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
		case EXCEPTION_FLT_DENORMAL_OPERAND      : return "EXCEPTION_FLT_DENORMAL_OPERAND";
		case EXCEPTION_FLT_DIVIDE_BY_ZERO        : return "EXCEPTION_FLT_DIVIDE_BY_ZERO";
		case EXCEPTION_FLT_INEXACT_RESULT        : return "EXCEPTION_FLT_INEXACT_RESULT";
		case EXCEPTION_FLT_INVALID_OPERATION     : return "EXCEPTION_FLT_INVALID_OPERATION";
		case EXCEPTION_FLT_OVERFLOW              : return "EXCEPTION_FLT_OVERFLOW";
		case EXCEPTION_FLT_STACK_CHECK           : return "EXCEPTION_FLT_STACK_CHECK";
		case EXCEPTION_FLT_UNDERFLOW             : return "EXCEPTION_FLT_UNDERFLOW";
		case EXCEPTION_INT_DIVIDE_BY_ZERO        : return "EXCEPTION_INT_DIVIDE_BY_ZERO";
		case EXCEPTION_INT_OVERFLOW              : return "EXCEPTION_INT_OVERFLOW";
		case EXCEPTION_PRIV_INSTRUCTION          : return "EXCEPTION_PRIV_INSTRUCTION";
		case EXCEPTION_IN_PAGE_ERROR             : return "EXCEPTION_IN_PAGE_ERROR";
		case EXCEPTION_ILLEGAL_INSTRUCTION       : return "EXCEPTION_ILLEGAL_INSTRUCTION";
		case EXCEPTION_NONCONTINUABLE_EXCEPTION  : return "EXCEPTION_NONCONTINUABLE_EXCEPTION";
		case EXCEPTION_STACK_OVERFLOW            : return "EXCEPTION_STACK_OVERFLOW";
		case EXCEPTION_INVALID_DISPOSITION       : return "EXCEPTION_INVALID_DISPOSITION";
		case EXCEPTION_GUARD_PAGE                : return "EXCEPTION_GUARD_PAGE";
		case EXCEPTION_INVALID_HANDLE            : return "EXCEPTION_INVALID_HANDLE";
		default									 : return "UNKNOWN EXCEPTION";
	}
}

inline LONG WINAPI exception_filter(EXCEPTION_POINTERS* info)
{
	Utils::printStackTrace();

	int result = MessageBoxA(nullptr, std::format("Code (0x{:08X}) -> {}\n", info->ExceptionRecord->ExceptionCode, GetExceptionName(info->ExceptionRecord->ExceptionCode)).c_str(), "Exception", MB_ICONERROR | MB_OKCANCEL);

	if (result == IDCANCEL)
	{
		TerminateProcess(HANDLE(GetCurrentProcess()), 0);
		return EXCEPTION_CONTINUE_SEARCH;
	}

	__debugbreak();
	return EXCEPTION_EXECUTE_HANDLER;
}


class Console
{
	FILE* pFile;
public:

	Console()
	{
		AllocConsole();
		freopen_s(&pFile, "CONOUT$", "w", stdout);
	}

	~Console()
	{
		system("pause");
		fclose(pFile);
		FreeConsole();
	}
};


int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
	SetUnhandledExceptionFilter(exception_filter);

	Console console;
	__try
	{
		SFMLRenderer::Create()->Init()->OnRender()->Destroy();
	}
	__except (exception_filter(GetExceptionInformation()))
	{

	}

    return 0;
}
