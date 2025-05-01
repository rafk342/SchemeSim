#include "SFMLRenderer.h"
#include "core/DrawList.h"
#include <chrono>
#include "Timer.h"
#include "frontend/drawableCircuit.h"
#include "imgui.h"
#include "imgui-SFML.h"
#include "implot.h"

SFMLRenderer* SFMLRenderer::Create()
{
	if (!self)
		self = new SFMLRenderer();

	return self;
}

void SFMLRenderer::Destroy()
{
	if (self)
	{
		delete self;
		self = nullptr;
	}
}

SFMLRenderer* SFMLRenderer::Get()
{
	return self;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#define CREATE_WINDOW 1

static const ImWchar fontRange[] =
{
	//Latin
	0x0020, 0x00FF, // Basic Latin + Latin Supplement
	0x0400, 0x052F, // Cyrillic + Cyrillic Supplement
	0x2DE0, 0x2DFF, // Cyrillic Extended-A
	0xA640, 0xA69F, // Cyrillic Extended-B
	//Chinese
	0x2000, 0x206F, // General Punctuation
	0x3000, 0x30FF, // CJK Symbols and Punctuations, Hiragana, Katakana
	0x31F0, 0x31FF, // Katakana Phonetic Extensions
	0xFF00, 0xFFEF, // Half-width characters
	0xFFFD, 0xFFFD, // Invalid
	0x4e00, 0x9FAF, // CJK Ideograms
	0,
};


SFMLRenderer* SFMLRenderer::Init()
{
#if CREATE_WINDOW
	auto mode = sf::VideoMode({ 1920, 1080 });
	sf::ContextSettings settings;
	settings.antiAliasingLevel = 8;

	m_Window = std::make_unique<sf::RenderWindow>(mode, "Wnd", sf::Style::Default, sf::State::Windowed, settings);
	m_Window->setFramerateLimit(250);
	
	m_view.setSize(sf::Vector2f(m_Window->getSize().x, m_Window->getSize().y));
	m_view.setCenter(sf::Vector2f(m_Window->getSize()) / 2.0f);
	m_view.zoom(1);
	m_Window->setView(m_view);

	SM_ASSERT(m_font.openFromFile("assets\\calibri.ttf"), "SFMLRenderer::Init() -> Failed to load font");

	SM_ASSERT(ImGui::SFML::Init(*m_Window, false), "ImGui::SFML::Init failed");

	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->Clear();
	io.Fonts->AddFontFromFileTTF("assets\\calibri.ttf", 16.0f);
	ImGui::SFML::UpdateFontTexture();
	
	ImPlot::CreateContext();
	ImVec4* colors = ImPlot::GetStyle().Colors;
	colors[ImPlotCol_FrameBg] = ImVec4(0.09f, 0.17f, 0.27f, 0.54f);
	colors[ImPlotCol_AxisBgHovered] = ImVec4(0.13f, 0.18f, 0.24f, 1.00f);
	colors[ImPlotCol_AxisBgActive] = ImVec4(0.14f, 0.23f, 0.31f, 1.00f);

#endif
	return this;
}



void DrawGraph(const Circuit::ResultsType& results, double totalTime, double maxVoltage)
{
	if (results.empty())
		return;

	float xMax = totalTime * 1000;
	float yMax = 100;
	float xOffset = 50;
	float yOffset = 20;

	sf::Color colors[] =
	{
		sf::Color::Red,
		sf::Color::Green,
		sf::Color::Blue,
		sf::Color::Cyan,
		sf::Color::Magenta,
		sf::Color::Yellow,
	};

	std::map<int, sf::VertexArray> graphs;
	for (const auto& [nodeIndex, data] : results)
	{
		sf::VertexArray line(sf::PrimitiveType::LineStrip, data.size());
		graphs[nodeIndex] = line;
	}


	sf::VertexArray grid(sf::PrimitiveType::Lines);
	int xGridLines = 10;
	int yGridLines = 10;
	float xStep = xMax / xGridLines;
	float yStep = yMax / yGridLines;


	sf::Color gridColor(50, 50, 50, 180);
	sf::Color DivColors(0, 0, 0, 200);

	for (int i = 0; i <= xGridLines; ++i)
	{
		float x = (i * xStep) + xOffset;
		grid.append(sf::Vertex(sf::Vector2f(x, yOffset), gridColor));
		grid.append(sf::Vertex(sf::Vector2f(x, yOffset + yMax), gridColor));

		float timeValue = (i * totalTime) / xGridLines;
		drawText(vfmt("{:.2f}s", timeValue), x - 10, yOffset + yMax + 5, 12, DivColors);
	}

	for (int i = 0; i <= yGridLines; ++i)
	{
		float y = (i * yStep) + yOffset;
		grid.append(sf::Vertex(sf::Vector2f(xOffset, y), gridColor));
		grid.append(sf::Vertex(sf::Vector2f(xOffset + xMax, y), gridColor));

		float voltageValue = (yGridLines - i) * maxVoltage / yGridLines;
		drawText(vfmt("{:.1f}V", voltageValue), xOffset - 40, y - 6, 12, DivColors);
	}


	dlDrawList::getWindow()->draw(grid);

	int lineIdx = 0;
	for (const auto& [nodeIndex, data] : results)
	{
		auto& line = graphs[nodeIndex];
		for (size_t i = 0; i < data.size(); ++i)
		{
			auto [time, voltage] = data[i];
			
			float x = xOffset + (time / totalTime) * xMax;
			float y = yOffset + yMax - (voltage / maxVoltage) * yMax;
			
			line[i].position = sf::Vector2f(x, y);
			line[i].color = colors[nodeIndex % std::size(colors)];
		}
		
		drawText(vfmt("Node {}", nodeIndex), xOffset + xMax + 20, yOffset + lineIdx * 30, 20, colors[nodeIndex % std::size(colors)]);
		lineIdx++;
	}

	for (const auto& [nodeIndex, line] : graphs)
	{
		dlDrawList::getWindow()->draw(line);
	}
}


void DrawGrid(const sf::Vector2f& gridSize, const sf::Vector2f& cellSize, const sf::Color& color = sf::Color::Black)
{
	sf::VertexArray lines(sf::PrimitiveType::Lines);

	for (float x = 0; x <= gridSize.x; x += cellSize.x) 
	{
		lines.append(sf::Vertex(sf::Vector2f(x, 0), color));
		lines.append(sf::Vertex(sf::Vector2f(x, gridSize.y), color));
		
		drawText(vfmt("{:.2f}", x), x, 0, 12, color);
	}

	for (float y = 0; y <= gridSize.y; y += cellSize.y) 
	{
		lines.append(sf::Vertex(sf::Vector2f(0, y), color));
		lines.append(sf::Vertex(sf::Vector2f(gridSize.x, y), color));

		drawText(vfmt("{:.2f}", y), 0, y, 12, color);
	}

	dlDrawList::getWindow()->draw(lines);
}


class CircuitRef : WidgetBase
{
public:
	CircuitRef()
		: WidgetBase("assets\\circuitRef1.png")
	{
		GetTexture().setSmooth(true);
	}

	void Draw(sf::Color col)
	{
		m_sprite.setColor(col);
		dlDrawList::getWindow()->draw(m_sprite);
	}
};

SFMLRenderer* SFMLRenderer::OnRender()
{
	sf::Vector2f PrevMousePos{};
	sf::Vector2f CurrentMousePos{};
	Circuit circuit;
	DrawableCircuit drawableCircuit(circuit);
	CircuitEditor editor(drawableCircuit);
	Simulation::Init(&circuit);

	CircuitRef circuitRef;


#if CREATE_WINDOW

	sf::Clock DeltaClock;
	while (m_Window->isOpen())
	{
		sf::Time dt = DeltaClock.restart();
		
		m_frameTime = dt.asSeconds();
		m_fps = 1.f / m_frameTime;
	
		CurrentMousePos = sf::Vector2f(sf::Mouse::getPosition());
		m_DeltaMouse = CurrentMousePos - PrevMousePos;
		PrevMousePos = CurrentMousePos;

		Simulation::Simulate(m_frameTime);

		HandleEvents();
		
		ImGui::SFML::Update(*m_Window, dt);
	
		m_Window->setView(m_view);
		m_Window->clear(sf::Color(200, 200, 200));
		

		{
			ImGui::ShowDemoWindow();
			ImPlot::ShowDemoWindow();

			static sf::Color circRefColor(255, 255, 255, 50);
			if (ImGui::Begin("wnd1"))
			{
				float col[4] = { circRefColor.r / 255.0f, circRefColor.g / 255.0f, circRefColor.b / 255.0f, circRefColor.a / 255.0f };
				
				if (ImGui::ColorEdit4("CircuitRef color", col))
					circRefColor = sf::Color(col[0] * 255, col[1] * 255, col[2] * 255, col[3] * 255);		

				circuitRef.Draw(circRefColor);
			}
			ImGui::End();

			editor.DrawUI();

			DrawGrid({ 10000.0f,10000.0f }, { 100.0f, 100.0f }, sf::Color(164, 164, 164, 255));
			drawText(vfmt("FPS: {:.3f}", m_fps), 10, 10, 150, sf::Color(0, 0, 0, 255));
			drawableCircuit.Draw(m_frameTime);
		}

		ImGui::SFML::Render(*m_Window);
		m_Window->display();

	}
#endif

	ImPlot::DestroyContext();
	ImGui::SFML::Shutdown();
	drawableCircuit.Destroy();

	return this;
}


void SFMLRenderer::HandleEvents()
{
	if (m_Window->hasFocus())
		if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Right))
			if (!ImGui::GetIO().WantCaptureMouse)
			{
				sf::Vector2f scaledMove = {
					m_DeltaMouse.x * (m_view.getSize().x / m_Window->getSize().x),
					m_DeltaMouse.y * (m_view.getSize().y / m_Window->getSize().y)
				};
				m_view.move(-scaledMove);
			}

	while (std::optional event = m_Window->pollEvent())
	{
		ImGui::SFML::ProcessEvent(*m_Window, *event);

		if (event->is<sf::Event::Closed>())
		{
			m_Window->close();
			continue;
		}

		if (auto* resized = event->getIf<sf::Event::Resized>())
		{
			auto [width, height] = resized->size;

			float NewAspectRatio = float(width) / float(height);
			auto CurrSize = m_view.getSize();
			m_view.setSize({ CurrSize.x, CurrSize.x / NewAspectRatio });
		}
		
		if (m_Window->hasFocus())
		{
			auto* scrolledEvent = event->getIf<sf::Event::MouseWheelScrolled>();
			if (scrolledEvent && !ImGui::GetIO().WantCaptureMouse)
			{
				if (scrolledEvent->delta > 0)
					m_view.zoom(0.95);

				else if (scrolledEvent->delta < 0)
					m_view.zoom(1.05f);
			}
		}
	}
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


sf::View*			SFMLRenderer::GetSfView()		{ return &m_view;}
sf::RenderWindow*	SFMLRenderer::GetSfWindow()		{ return m_Window.get();}
sf::Font&			SFMLRenderer::GetFont()			{ return m_font;}
sf::Vector2f		SFMLRenderer::GetDeltaMouse()	{ return m_DeltaMouse; }

sf::Vector2f SFMLRenderer::GetWorldMousePos()
{
	return m_Window->mapPixelToCoords(sf::Mouse::getPosition(*m_Window), m_view);
}
