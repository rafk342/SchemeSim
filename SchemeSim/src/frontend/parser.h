#pragma once
#include <iostream>
#include <scn/scan.h>
#include "core/DrawList.h"

class CircuitEditor;
class DrawableCircuit;

class CircuitParser
{
	CircuitEditor* m_editor = nullptr;

	__forceinline void WriteVec2f(std::ostream& os, const sf::Vector2f& v) { os << vfmt("{:.6f} {:.6f}\n", v.x, v.y); }
	__forceinline void WriteFloat(std::ostream& os, float f) { os << vfmt("{:.6f}\n", f); }
	__forceinline void WriteInt(std::ostream& os, int n) { os << n << '\n'; }

	__forceinline sf::Vector2f ReadVec2f(const std::string& line)
	{
		if (auto result = scn::scan<float, float>(line, "{} {}"))
		{
			auto [a, b] = result->values();
			return { a, b };
		}
		else
		{
			return { 0.0f, 0.0f };
		}
	}
	__forceinline float ReadFloat(const std::string& line)
	{
		if (auto result = scn::scan<float>(line, "{}"))
			return result->value();
		else
			return 0.0f;
	}
	__forceinline int ReadInt(const std::string& line)
	{
		if (auto result = scn::scan<int>(line, "{}"))
			return result->value();
		else
			return 0;
	}

public:

	CircuitParser(CircuitEditor* editor) : m_editor(editor) {}
	void LoadFromFile(const std::filesystem::path& path);
	void SaveToFile(const std::filesystem::path& path);
};
