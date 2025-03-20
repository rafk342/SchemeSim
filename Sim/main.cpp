#include <iostream>
#include <thread>
#include <functional>
#include <print>

#include "base/SFMLRenderer.h"
#include <windows.h>

//void* operator new(size_t sz) noexcept
//{
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
	Console console;

	SFMLRenderer::Create()->Init()->OnRender()->Destroy();
	
    return 0;
}
