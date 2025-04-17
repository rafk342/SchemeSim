#include "parser.h"
#include "drawableCircuit.h"

//----------------------------------------------------------------------------------------------------------------------------------------
//											Parser



void CircuitParser::LoadFromFile(const std::filesystem::path& path)
{
	if (!m_editor || !m_editor->m_DrawableCircuit)
		return;

	DrawableCircuit* drawableCirc = m_editor->m_DrawableCircuit;

	std::ifstream file(path);
	if (!file.is_open())
		return;

	std::vector<std::string> lines;
	lines.reserve(1000);
	{
		std::string line;
		while (std::getline(file, line))
			lines.emplace_back(std::move(line));
	}
	file.close();
	if (lines.empty())
		return;

	for (size_t i = 0; i < lines.size(); i++)
	{
		std::string& line = lines[i];
		if (line == "DOTS")
		{
			while (true)
			{
				if (i >= lines.size())
					break;

				line = lines[++i];
				if (line == "DOTS_END")
					break;

				drawableCirc->CreateConnectionDot(ReadVec2f(line));
			}
		}

		if (line == "WIRES")
		{
			while (true)
			{
				if (i >= lines.size())
					break;
				line = lines[++i];
				if (line == "WIRES_END")
					break;

				if (line == "next")
				{
					drawableCirc->AddWire();
					continue;
				}
				sf::Vector2f start = ReadVec2f(line);
				sf::Vector2f end = ReadVec2f(lines[++i]);
				drawableCirc->m_DrawableElements.back()->As<Wire>()->GetSegments().push_back({ start, end });
			}
		}

		if (line == "ELEMENTS")
		{
			while (true)
			{
				if (i >= lines.size())
					break;
				line = lines[++i];
				if (line == "next")
					continue;
				if (line == "ELEMENTS_END")
					break;

				DrawableType type = DrawableType(std::stoi(line));
				std::shared_ptr<eDrawableBase> drawableBase;
				switch (type)
				{
				case DrawableType::DRAWABLE_RESISTOR:
					drawableBase = drawableCirc->AddResistor();
					break;
				case DrawableType::DRAWABLE_BATTERY:
					drawableBase = drawableCirc->AddBattery();
					break;
				case DrawableType::DRAWABLE_CAPACITOR:
					drawableBase = drawableCirc->AddCapacitor();
					break;
				case DrawableType::DRAWABLE_INDUCTOR:
					drawableBase = drawableCirc->AddInductor();
					break;
				case DrawableType::DRAWABLE_DIODE:
					drawableBase = drawableCirc->AddDiode();
					break;
				default:
					SM_ASSERT(false, "Unknown element type");
					break;
				}

				sf::Vector2f pos = ReadVec2f(lines[++i]);
				float rot = ReadFloat(lines[++i]);
				bool flippedX = ReadInt(lines[++i]) != 0;
				bool flippedY = ReadInt(lines[++i]) != 0;
				std::string data = lines[++i];

				drawableBase->SetPosition(pos);
				drawableBase->SetRotation(rot);
				drawableBase->Parser_ReadElementData(drawableCirc->GetElecticElementFromDrawable(drawableBase.get()), data);
				if (flippedX)
					drawableBase->Flip(flipAxis::X);
				if (flippedY)
					drawableBase->Flip(flipAxis::Y);
			}
		}
	}

	m_editor->UpdateAllWireConnections();
	file.close();
}


void CircuitParser::SaveToFile(const std::filesystem::path& path)
{
	if (!m_editor)
		return;

	DrawableCircuit* drawableCirc = m_editor->m_DrawableCircuit;
	std::deque<std::shared_ptr<eDrawableBase>>& elements = drawableCirc->m_DrawableElements;
	std::unordered_map<std::shared_ptr<ConnectionDot>, eNode*>& connections = drawableCirc->m_Connections;
	if (!drawableCirc)
		return;

	std::ofstream file(path);
	if (!file.is_open())
		return;

	file << "DOTS\n";
	for (auto& [dot, node] : connections)
	{
		if (dot->HasConnectionWithAnyElem())
			continue;

		WriteVec2f(file, dot->GetPosition());
	}
	file << "DOTS_END\n";

	file << "WIRES\n";
	for (auto& elemBase : elements)
	{
		if (!elemBase->IsWire())
			continue;

		Wire* wire = elemBase->As<Wire>();
		auto& segments = wire->GetSegments();
		if (segments.empty())
			continue;

		file << "next\n";
		for (auto& segment : segments)
		{
			WriteVec2f(file, segment.vStart);
			WriteVec2f(file, segment.vEnd);
		}
	}
	file << "WIRES_END\n";

	file << "ELEMENTS\n";
	for (auto& drawableBase : elements)
	{
		if (drawableBase->IsWire())
			continue;
		file << "next\n";

		DrawableType type = drawableBase->GetType();
		file << int(type) << "\n";

		sf::Vector2f pos = drawableBase->GetPosition();
		float rot = drawableBase->GetRotation().asDegrees();
		WriteVec2f(file, pos);
		WriteFloat(file, rot);
		WriteInt(file, int(drawableBase->IsFlippedOverX()));
		WriteInt(file, int(drawableBase->IsFlippedOverY()));

		std::string data = drawableBase->Parser_WriteElementData(drawableCirc->GetElecticElementFromDrawable(drawableBase.get()));
		file << data << "\n";
	}
	file << "ELEMENTS_END\n";

	file.flush();
	file.close();
}
