#include "drawableCircuit.h"


//----------------------------------------------------------------------------------------------------------------------------------------
//											ConnectionPoint

ConnectionDot::ConnectionDot(sf::Vector2f pos)
	: m_Position(pos)
{ }


void ConnectionDot::UpdateVisibility()
{
	m_ToElemPin = std::ranges::any_of(m_ConnectedElements, [](std::pair<std::weak_ptr<eDrawableBase>, PinIndex>& pair)
		{
			if (std::shared_ptr drawable = pair.first.lock()) 
			{
				return !drawable->IsWire();
			}
			return false;
		});
}


void ConnectionDot::Draw()
{
	if (!m_ToElemPin)
		DrawCircle(m_Position, (Wire::WireThickness / 2.0f) * 2.0f, sf::Color::Black);

	if (IsHoveredInUi)
		DrawCircle(m_Position, (Wire::WireThickness / 2.0f) * 2.0f, sf::Color::Red);
}


void ConnectionDot::Connect(std::weak_ptr<eDrawableBase> element, int pinIndex)
{
	if (!element.lock())
		return;

	if (!contains(element))
		m_ConnectedElements.emplace_back(element, pinIndex);

	UpdateVisibility();
}


void ConnectionDot::Release(std::weak_ptr<eDrawableBase> element)
{
	if (!element.lock())
		return;

	auto it = std::ranges::find_if(m_ConnectedElements, [element](const auto& pair) { return pair.first.lock() == element.lock(); });
	if (it != m_ConnectedElements.end())
		m_ConnectedElements.erase(it);

	UpdateVisibility();
}


bool ConnectionDot::contains(std::weak_ptr<eDrawableBase> element)
{
	if (element.expired())
		return false;
	CleanupFromExpiredElements();
	return std::ranges::find_if(m_ConnectedElements, [element](const auto& pair) { return pair.first.lock() == element.lock(); }) != m_ConnectedElements.end();
}


void ConnectionDot::SetPosition(sf::Vector2f pos)
{
	m_Position = pos; 
	UpdateConnectedElementsPositions();
}


void ConnectionDot::UpdateConnectedElementsPositions()
{
	if (m_ConnectedElements.empty())
		return;

	sf::Vector2f NewPos = m_Position;
	bool positionChanged = false;

	for (auto& [WeakElem, pinIndex] : m_ConnectedElements)
	{
		if (auto elem = WeakElem.lock())
		{
			sf::Vector2f pinPos = elem->GetGlobalPinPosition(pinIndex);
			if (pinPos != m_Position)
			{
				NewPos = pinPos;
				positionChanged = true;
				break;
			}
		}
	}

	if (!positionChanged)
		return;

	for (auto& [WeakElem, pinIndex] : m_ConnectedElements)
	{
		if (auto elem = WeakElem.lock())
		{
			if (elem->IsWire())
			{
				Wire* wire = elem->As<Wire>();
				auto& segments = wire->GetSegments();
			
				if (pinIndex == Wire::PinPoint::Start && !segments.empty())
				{
					segments.front().vStart = NewPos;
				}
				else if (pinIndex == Wire::PinPoint::End && !segments.empty())
				{
					segments.back().vEnd = NewPos;
				}
			}
			else
			{
				sf::Vector2f currentPinPos = elem->GetGlobalPinPosition(pinIndex);
				sf::Vector2f offset = NewPos - currentPinPos;
				elem->SetPosition(elem->GetPosition() + offset);
			}
		}
	}
	m_Position = NewPos;
}


void ConnectionDot::CleanupFromExpiredElements()
{
	std::erase_if(m_ConnectedElements, [](const auto& pair) { return pair.first.expired(); });
}


//----------------------------------------------------------------------------------------------------------------------------------------
//											Wire


Wire::Wire()
	: eDrawableBase({})
{ }


void Wire::DrawThickLine(sf::Vector2f start, sf::Vector2f end, float thickness, sf::Color color)
{
	sf::Vector2f dir = end - start;
	float len = dir.length();

	sf::RectangleShape line({ len, thickness });
	line.setFillColor(color);
	line.setOrigin({ 0, thickness / 2 });
	line.setPosition(start);
	line.setRotation(dir.normalized().angle());

	DrawCircle(start, thickness / 2.0f, color);
	DrawCircle(end, thickness / 2.0f, color);
	dlDrawList::getWindow()->draw(line);
}


void Wire::DrawThickLineWithGradient(sf::Vector2f start, sf::Vector2f end, float thickness, sf::Color color1, sf::Color color2)
{
	sf::Vector2f dir = end - start;
	sf::Vector2f norm = dir.normalized();
	sf::Vector2f perp(-norm.y, norm.x);

	sf::Vector2f half = perp * (thickness / 2.0f);

	sf::Vector2f p1 = start + half;
	sf::Vector2f p2 = start - half;
	sf::Vector2f p3 = end - half;
	sf::Vector2f p4 = end + half;
	
	sf::Vertex vertices[] = 
	{
		{ p1, color1 },
		{ p2, color1 },
		{ p4, color2 },

		{ p3, color2 },
		{ p4, color2 },
		{ p2, color1 },
	};

	DrawCircle(start, thickness / 2.0f, color1);
	DrawCircle(end, thickness / 2.0f, color2);
	dlDrawList::getWindow()->draw(vertices, 6, sf::PrimitiveType::Triangles);
}


void Wire::Draw()
{
	if (m_Segments.empty())
		return;

	if (IsHoveredInUi)
	{
		for (auto& segment : m_Segments)
		{
			DrawThickLine(segment.vStart, segment.vEnd, WireThickness * 1.2f, sf::Color::Red);
		}
	}
	else
	{
		float fullLength = 0.0f;
		for (const auto& segment : m_Segments)
			fullLength += segment.length();

		float lengthsSum = 0.0f;
		for (const auto& segment : m_Segments)
		{
			float SegLength = segment.length();
			float tStart = lengthsSum / fullLength;
			float tEnd = (lengthsSum + SegLength) / fullLength;

			sf::Color colorStart = lerpColor(m_StartColor, m_EndColor, tStart);
			sf::Color colorEnd = lerpColor(m_StartColor, m_EndColor, tEnd);
			DrawThickLineWithGradient(segment.vStart, segment.vEnd, WireThickness, colorStart, colorEnd);

			lengthsSum += SegLength;
		}
	}
}


sf::Vector2f Wire::GetGlobalPinPosition(int n)
{
	if (m_Segments.empty())
		return sf::Vector2f(0, 0);

	if (n == 0)
		return m_Segments[0].vStart;
	else if (n == 1)
		return m_Segments[m_Segments.size() - 1].vEnd;
	else
		return sf::Vector2f(0, 0);
}


sf::Vector2f Wire::GetLocalPinPosition(int n)
{
	return GetGlobalPinPosition(n);
}


int Wire::GetPinIndexFromLocalPosition(sf::Vector2f pos)
{
	return pos == m_Segments[0].vStart ? 0 : 1;
}


std::shared_ptr<eDrawableBase> Wire::SplitSelf(DrawableCircuit& circuit, u64 SegmentIndex, sf::Vector2f SplitPoint)
{
	if (SegmentIndex >= m_Segments.size())
		return nullptr;
		
	std::shared_ptr newWirePtr = circuit.AddWire();
	Wire& left = *this;
	Wire& right = *newWirePtr->As<Wire>();
		
	Segment& SegmentToSplit = left.m_Segments[SegmentIndex];
	sf::Vector2f SegmentPosStart = SegmentToSplit.vStart;
	sf::Vector2f SegmentPosEnd = SegmentToSplit.vEnd;
		
	left.m_Segments.erase(left.m_Segments.begin() + SegmentIndex);
	left.m_Segments.insert(m_Segments.begin() + SegmentIndex, { { SegmentPosStart, SplitPoint }, { SplitPoint, SegmentPosEnd }, });
		
	right.m_Segments = std::deque<Segment>(left.m_Segments.begin() + SegmentIndex + 1, left.m_Segments.end());
	left.m_Segments.erase(left.m_Segments.begin() + SegmentIndex + 1, left.m_Segments.end());


	return newWirePtr;
}


void Wire::UpdateColor(DrawableCircuit& circuit)
{
	eElement* elem = circuit.GetElecticElementFromDrawable(this);
	if (!elem)
		return;

	double StartVolt = 0.0;
	double EndVolt = 0.0;
	if (auto dot = m_StartDot.lock())
	{
		eNode* node = circuit.GetElecticNode(dot);
		if (node)
			StartVolt = node->GetVoltage();
	}

	if (auto dot = m_EndDot.lock())
	{
		eNode* node = circuit.GetElecticNode(dot);
		if (node)
			EndVolt = node->GetVoltage();
	}

	double MaxVolt = 5.0;

	float StartIntensity = std::abs(StartVolt) / MaxVolt;
	float EndIntensity = std::abs(EndVolt) / MaxVolt;

	StartIntensity = std::min(StartIntensity, 1.0f);
	EndIntensity = std::min(EndIntensity, 1.0f);

	float magic = 7.5f;
	magic *= magic;

	StartIntensity = std::log(StartIntensity * magic + 1.0f) / std::log(magic);
	EndIntensity = std::log(EndIntensity * magic + 1.0f) / std::log(magic);
	
	StartIntensity = std::clamp(StartIntensity, 0.0f, 1.0f);
	EndIntensity = std::clamp(EndIntensity, 0.0f, 1.0f);


	if (StartVolt >= 0)
		m_StartColor = lerpColor(sf::Color::Black, PositiveVoltColor, StartIntensity);
	else
		m_StartColor = lerpColor(sf::Color::Black, NegativeVoltColor, StartIntensity);


	if (EndVolt >= 0)
		m_EndColor = lerpColor(sf::Color::Black, PositiveVoltColor, EndIntensity);
	else
		m_EndColor = lerpColor(sf::Color::Black, NegativeVoltColor, EndIntensity);
}


void Wire::DrawCurrentDots(eElement* elem)
{
	const float spacing = 75.0f;
	float current = static_cast<eResistor*>(elem)->GetCurrent();
	if (Simulation::GetState() == SIM_PAUSED) 
		current = 0.0f;

	for (auto& seg : m_Segments)
	{
		seg.curcount += current * (std::pow(CurrentSpeedScalar, 2.0f) / 100.0f);
		if (std::isinf(seg.curcount))
			seg.curcount = 0.0f;
		
		sf::Vector2f dir = seg.vEnd - seg.vStart;
		sf::Vector2f normalized = dir.normalized();
		float length = dir.length();

		float SpacingOffset = std::fmod(seg.curcount, spacing);
		if (SpacingOffset < 0)
			SpacingOffset += spacing;

		for (float di = SpacingOffset; di < length; di += spacing)
		{
			sf::Vector2f DotPos = seg.vStart + (normalized * di);
			DrawCircle(DotPos, (Wire::WireThickness / 2.0f) * 0.7f, CurrentDotsColor);
		}
	}
}


template<Wire::PinPoint StartOrEnd>
void Wire::ConnectToDotAt(std::shared_ptr<ConnectionDot> dot)
{
	DisconnectFromDotAt<StartOrEnd>();

	if constexpr (StartOrEnd == Start)
	{
		m_StartDot = dot;
		dot->Connect(shared_from_this(), Start);
		if (!m_Segments.empty())
			m_Segments.front().vStart = dot->GetPosition();
	}
	else
	{
		m_EndDot = dot;
		dot->Connect(shared_from_this(), End);
		if (!m_Segments.empty())
			m_Segments.back().vEnd = dot->GetPosition();
	}
}


template<Wire::PinPoint StartOrEnd>
void Wire::DisconnectFromDotAt()
{
	if (StartOrEnd == Start && !m_StartDot.expired())
	{
		if (auto dot = m_StartDot.lock())
			dot->Release(shared_from_this());
		m_StartDot.reset();
	}
	else if (StartOrEnd == End && !m_EndDot.expired())
	{
		if (auto dot = m_EndDot.lock())
			dot->Release(shared_from_this());
		m_EndDot.reset();
	}
}




//----------------------------------------------------------------------------------------------------------------------------------------
//											Oscilloscope


Oscilloscope::Oscilloscope(std::shared_ptr<eDrawableBase> drawable)
	: m_Drawable(drawable)
	, m_ShowVoltage(true)
	, m_ShowCurrent(true)
{
}

void Oscilloscope::Init(OwnerListTy* owner, OwnerListTy::iterator it)
{
	m_OwnerList = owner;
	m_selfIt = it;
}

Oscilloscope::~Oscilloscope()
{
}

void Oscilloscope::DrawPlot()
{
	if (m_Drawable.expired())
	{
		m_OwnerList->erase(m_selfIt);
		return;
	}

	if (ImPlot::BeginPlot(vfmt("##myPlot{}", u64(this))))
	{
		const double simTime = Simulation::CircTime();
		ImPlot::SetupAxisLimits(ImAxis_X1, simTime - 1.0, simTime, ImGuiCond_Always);
		ImPlot::SetupAxisLimits(ImAxis_Y1, -1, 1);

		if (m_ShowVoltage && m_VoltData.Data.size() > 0)
			ImPlot::PlotLine(vfmt("Vd##{}", u64(this)), &m_VoltData.Data[0].x, &m_VoltData.Data[0].y, m_VoltData.Data.size(), 0, m_VoltData.Offset, 2 * sizeof(float));

		if (m_ShowCurrent && m_CurrentData.Data.size() > 0)
			ImPlot::PlotLine(vfmt("I##{}", u64(this)), &m_CurrentData.Data[0].x, &m_CurrentData.Data[0].y, m_CurrentData.Data.size(), 0, m_CurrentData.Offset, 2 * sizeof(float));

		ImPlot::EndPlot();
	}

	if (ImGui::Button(vfmt("Delete##{}",u64(this))))
		m_OwnerList->erase(m_selfIt);
}

void Oscilloscope::AddVoltData(float time, float voltage)
{
	m_VoltData.AddPoint(time, voltage);
}

void Oscilloscope::AddCurrentData(float time, float current)
{
	m_CurrentData.AddPoint(time, current);
}

void Oscilloscope::Reset()
{
	m_VoltData.Erase();
	m_CurrentData.Erase();
}



//----------------------------------------------------------------------------------------------------------------------------------------
//											DrawableCircuit



std::shared_ptr<ConnectionDot> DrawableCircuit::SplitWire(std::shared_ptr<eDrawableBase> wireBase, u64 SegmentIndex, sf::Vector2f SplitPoint)
{
	if (!wireBase || !wireBase->IsWire())
		return nullptr;

	auto itLeft = std::ranges::find_if(m_DrawableElements, [wireBase](const auto& e) { return e.get() == wireBase.get(); });
	if (itLeft == m_DrawableElements.end())
		return nullptr;
	
	Wire* wireLeft = wireBase->As<Wire>();
	if (SegmentIndex >= wireLeft->GetSegments().size())
		return nullptr;
	
	std::shared_ptr WireRightBase = wireLeft->SplitSelf(*this, SegmentIndex, SplitPoint);
	Wire* wireRight = WireRightBase->As<Wire>();

	std::shared_ptr NewDot = CreateConnectionDot(SplitPoint);
	wireRight->ConnectToDotAt<Wire::Start>(NewDot);
	wireLeft->ConnectToDotAt<Wire::End>(NewDot);

	return NewDot;
}


void DrawableCircuit::SyncWithCircuit()
{
	CleanupConnections();
	for (auto& [dot, node] : m_Connections)
	{
		node->ReleaseAllPins();

		for (auto& [WeakDrawable, pinIndex] : dot->GetConnectedElements())
		{
			if (auto drawable = WeakDrawable.lock())
			{
				GetElecticElementFromDrawable(drawable.get())->GetEpin(pinIndex)->ConnectToNode(node);
			}
		}
	}

	m_Circuit->RebuildMatrix();
}

void DrawableCircuit::Destroy()
{
	m_DrawableElements.clear();
	for (auto& [ptr, elem] : m_DrawableToElement)
	{
		m_Circuit->RemoveElement(elem);
	}
	m_DrawableToElement.clear();

	for (auto& [dot, node] : m_Connections)
	{
		m_Circuit->RemoveNode(node);
	}
	m_Connections.clear();
}


void DrawableCircuit::Draw(float frameTime)
{
	for (auto& elem : m_DrawableElements)
	{
		if (elem->IsWire())
			continue;

		elem->Draw();
	}

	for (auto& elem : m_DrawableElements)
	{
		if (!elem->IsWire())
			continue;

		Wire* wire = elem->As<Wire>();
		wire->UpdateColor(*this);
		wire->Draw();
		wire->DrawCurrentDots(GetElecticElementFromDrawable(elem.get()));
	}

	for (auto& [dot, node] : m_Connections)
	{
		dot->Draw();
		drawText(vfmt("{}", node->GetIndex()), dot->GetPosition().x, dot->GetPosition().y, 40, sf::Color::Red);
	}

	for (auto& elem : m_DrawableElements)
	{
		elem->Update(*this, GetElecticElementFromDrawable(elem.get()));
	}
}


void DrawableCircuit::RemoveElement(eDrawableBase* element)
{
	if (!element)
		return;

	auto it = std::ranges::find_if(m_DrawableElements, [element](std::shared_ptr<eDrawableBase>& e) { return e.get() == element; });
	if (it == m_DrawableElements.end())
		return;

	RemoveElement(std::distance(m_DrawableElements.begin(), it));
}


void DrawableCircuit::RemoveElement(u64 index)
{
	if (index >= m_DrawableElements.size())
		return;

	auto it = m_DrawableElements.begin() + index;
	eDrawableBase* element = it->get();

	m_DrawableElements.erase(it);
	m_Circuit->RemoveElement(m_DrawableToElement.at(element));
	m_DrawableToElement.erase(element);

	CleanupConnections();
}


std::shared_ptr<ConnectionDot> DrawableCircuit::CreateConnectionDot(sf::Vector2f pos)
{
	auto dot = std::make_shared<ConnectionDot>(pos);
	m_Connections[dot] = m_Circuit->CreateNode();
	return dot;
}


void DrawableCircuit::UpdateWireColors()
{
	for (auto& elem : m_DrawableElements)
	{
		if (elem->IsWire())
			elem->As<Wire>()->UpdateColor(*this);
	}
}

eElement* DrawableCircuit::GetElecticElementFromDrawable(eDrawableBase* drawable)
{
	auto it = m_DrawableToElement.find(drawable);
	if (it != m_DrawableToElement.end())
		return it->second;
	return nullptr;
}

eNode* DrawableCircuit::GetElecticNode(std::shared_ptr<ConnectionDot> dot)
{
	auto it = m_Connections.find(dot);
	if (it != m_Connections.end())
		return it->second;
	return nullptr;
}


void DrawableCircuit::CleanupConnections()
{
	std::vector<std::shared_ptr<ConnectionDot>> toRemove;
	for (auto& [dot, node] : m_Connections)
	{
		dot->CleanupFromExpiredElements();
		if (dot->GetNumConnectedElements() <= 1)
		{
			m_Circuit->RemoveNode(node);
			toRemove.push_back(dot);
		}
	}

	for (auto& dot : toRemove)
	{
		m_Connections.erase(dot);
	}
}



//----------------------------------------------------------------------------------------------------------------------------------------
//											Editor


sf::Vector2f CircuitEditor::SnapToGrid(sf::Vector2f pos, float GridScale)
{
	return sf::Vector2f( 
		std::round(pos.x / GridScale) * GridScale,
		std::round(pos.y / GridScale) * GridScale );
}


std::optional<std::shared_ptr<ConnectionDot>> CircuitEditor::GetClosestConnectionDot(sf::Vector2f& OutPos)
{
	for (auto& [dot, node] : m_DrawableCircuit->m_Connections)
	{
		float dist = (OutPos - dot->GetPosition()).length();
		if (dist < 20.0f)
		{
			OutPos = dot->GetPosition();
			return dot;
		}
	}
	return std::nullopt;
}


std::optional<CircuitEditor::ClosestPinData> CircuitEditor::GetClosestElementPinInfo(sf::Vector2f pos)
{
	for (auto& elem : m_DrawableCircuit->m_DrawableElements)
	{
		if (elem->IsWire())
			continue;

		sf::Vector2f elemPos = elem->GetPosition();

		for (size_t i = 0; i < elem->GetNumPins(); i++)
		{
			sf::Vector2f localPinPos = elem->GetLocalPinPosition(i);
			sf::Vector2f pinPos = elem->GetPosition() + localPinPos;	
			float dist = (pos - pinPos).length();
			if (dist < 20.0f)
				return ClosestPinData{ .PinPos = pinPos,
									   .PinIndex = elem->GetPinIndexFromLocalPosition(localPinPos),
									   .DrawableElement = elem };
		}
	}
	return std::nullopt;
}


std::optional<CircuitEditor::ClosestWirePointInfo> CircuitEditor::GetClosestPointOnWire(Wire& wire, sf::Vector2f mousePos, float maxDistance)
{
	auto& segments = wire.GetSegments();

	if (segments.empty())
		return std::nullopt;

	auto GetClosestPointOnSegment = [](sf::Vector2f point, sf::Vector2f segStart, sf::Vector2f segEnd) -> sf::Vector2f 
		{
			sf::Vector2f dir = segEnd - segStart;
			float length = dir.length();
			if (length == 0)
				return segStart;

			sf::Vector2f dirToPoint = point - segStart;

			float t = dirToPoint.dot(dir) / (length * length);
			t = std::clamp(t, 0.0f, 1.0f);

			return segStart + dir * t;
		};


	auto minIt = std::ranges::min_element(segments, [&](const Wire::Segment& a, const Wire::Segment& b)
		{
			sf::Vector2f pointA = GetClosestPointOnSegment(mousePos, a.vStart, a.vEnd);
			sf::Vector2f pointB = GetClosestPointOnSegment(mousePos, b.vStart, b.vEnd);
			float distA = (mousePos - pointA).length();
			float distB = (mousePos - pointB).length();
			return distA < distB;
		});

	sf::Vector2f closestPoint = GetClosestPointOnSegment(mousePos, minIt->vStart, minIt->vEnd);
	float minDistance = (mousePos - closestPoint).length();
	
	if (minDistance <= maxDistance)
	{
		ClosestWirePointInfo result {
			.position = closestPoint,
			.segmentIndex = u64(std::distance(segments.begin(), minIt)),
			.distance = minDistance,
		};
		return result;
	}
	else
	{
		return std::nullopt;
	}
}


std::optional<std::pair<std::shared_ptr<eDrawableBase>, u64>> CircuitEditor::SearchForNearestWire(const sf::Vector2f& MousePos, sf::Vector2f& OutEnd, std::shared_ptr<eDrawableBase> selfWire)
{
	for (auto& elem : m_DrawableCircuit->m_DrawableElements)
	{
		if (elem == selfWire)
			continue;

		if (!elem->IsWire())
			continue;

		if (std::optional info = GetClosestPointOnWire(*elem->As<Wire>(), MousePos))
		{
			OutEnd = info->position;
			return std::make_pair( elem, info->segmentIndex );
		}
	}
	return std::nullopt;
}




void CircuitEditor::AddOscilloscope(std::shared_ptr<eDrawableBase> drawable)
{
	if (!drawable)
		return;

	auto it = m_Oscilloscopes.insert(m_Oscilloscopes.end(), std::make_shared<Oscilloscope>(drawable));
	(*it)->Init(&m_Oscilloscopes, it);
	Simulation::RegisterOscilloscope(m_DrawableCircuit->GetElecticElementFromDrawable(drawable.get()), (*it));
	m_DrawableToOscilloscope[drawable.get()] = *it;
}

void CircuitEditor::ResetOscilloscopes()
{
	for (auto& osc : m_Oscilloscopes)
	{
		osc->Reset();
	}
}

void CircuitEditor::RemoveOscilloscope(std::shared_ptr<eDrawableBase> drawable)
{
	if (!drawable)
		return;

	auto it = m_DrawableToOscilloscope.find(drawable.get());
	if (it != m_DrawableToOscilloscope.end())
	{
		if (auto osc = it->second.lock())
			m_Oscilloscopes.erase(osc->GetSelfIt());
		
		m_DrawableToOscilloscope.erase(it);
	}
}


CircuitEditor::CircuitEditor(DrawableCircuit& circuit)
	: m_DrawableCircuit(&circuit) 
	, m_Parser(this)
{ }


void CircuitEditor::DrawUI()
{
	auto PushTextColor = [](sf::Color color) { ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f)); };
	auto PopTextColor = [] { ImGui::PopStyleColor(); };


	ImGui::Begin("Editor");

	if (m_EditableWire)
		ImGui::Text("Editing Wire");


	sf::Vector2f mousePos = gSFMLRenderer.GetWorldMousePos();
	static sf::Vector2f DragOffset{};
	
	if (ImGui::Button("Export"))
	{
		m_Parser.SaveToFile("circuit.txt");
	}

	if (ImGui::Button("Import"))
	{
		m_Parser.LoadFromFile("circuit.txt");
		m_DrawableCircuit->SyncWithCircuit();
		//Simulation::SetState(SIM_ON_START);
	}

	auto state = Simulation::GetState();
	ImGui::Text(vfmt("Simulation state : {}",
		state == SIM_STOPPED ? "STOPPED" : 
		state == SIM_RUNNING ? "RUNNING" : 
		state == SIM_ON_START ? "SIM_ON_START" : "PAUSED"));

	if (ImGui::Button("Start sim"))
	{
		m_DrawableCircuit->SyncWithCircuit();
		Simulation::SetState(SIM_ON_START);
		ResetOscilloscopes();
	}

	if (ImGui::Button("Stop sim"))
		Simulation::SetState(SIM_STOPPED);

	if (ImGui::Button("Pause sim"))
		Simulation::SetState(SIM_PAUSED);


	if (ImGui::Button("Continue sim"))
		Simulation::SetState(SIM_RUNNING);


	ImGui::Text("t: %.6f s", Simulation::CircTime());
	float min = 0.01f;
	float max = 1.0f;
	ImGui::SliderScalar("Sim speed", ImGuiDataType_Float, &Simulation::SimSpeed(), &min, &max, "%.5f");



	if (ImGui::BeginTable("Elements", 2, ImGuiTableFlags_Borders))
	{
		ImGui::TableSetupColumn("ElementsColumn");
		ImGui::TableSetupColumn("Remove", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("Remove").x + 5);

		size_t toRemove = m_DrawableCircuit->m_DrawableElements.size();
		for (size_t i = 0; i < m_DrawableCircuit->m_DrawableElements.size(); i++)
		{
			auto& drawable = m_DrawableCircuit->m_DrawableElements[i];
			auto* elem = m_DrawableCircuit->m_DrawableToElement.at(drawable.get());
			
			bool isWire = drawable->IsWire();
			
			bool isHovered = !isWire && drawable->IsHovered();

			ImGui::TableNextRow();
			ImGui::TableNextColumn();

			if (isHovered)
				PushTextColor(sf::Color::Red);
			ImGui::Text(vfmt("Elements[{}]: {}",i, isWire ? "Wire" : elem->GetTypeName()));
			
			if (isHovered)
				PopTextColor();


			if (isWire)
			{
				Wire* wire = drawable->As<Wire>();
				ImGui::SameLine(200.0f);
				ImGui::Text("I: %.3f A", m_DrawableCircuit->GetElecticElementFromDrawable(wire)->GetCurrent());
				ImGui::SameLine(200 + ImGui::CalcTextSize("I: 0.0000f A").x);
				if (ImGui::Button(vfmt("Edit Wire ##{}", i)))
				{
					m_EditableWire = wire;
					m_WireEditOnStart = true;
				}

				if (ImGui::IsItemHovered())
					wire->IsHoveredInUi = true;
				else
					wire->IsHoveredInUi = false;

			}
			else
			{
				if (ImGui::TreeNode(vfmt("Settings##{}", i)))
				{
						ImGui::Indent();
						drawable->UIParams(elem);
						ImGui::Unindent();

					ImGui::TreePop();
				}

				if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !m_DraggableElement && !m_EditableWire)
				{
					m_DraggableElement = drawable.get();
					DragOffset = mousePos - drawable->GetPosition();
				}

				if (!m_DrawableToOscilloscope.contains(drawable.get()))
				{
					if (ImGui::Button(vfmt("Connect oscilloscope ##{}", i)))
					{
						AddOscilloscope(drawable);
					}
				}
				else
				{
					ImGui::Separator();
					if (ImGui::TreeNode(vfmt("Oscilloscope##{}", u64(drawable.get()))))
					{
						if (auto osc = m_DrawableToOscilloscope.at(drawable.get()).lock())
							osc->DrawPlot();
						else
							m_DrawableToOscilloscope.erase(drawable.get());

						ImGui::TreePop();
					}
				}
			}

			ImGui::TableNextColumn();
			if (ImGui::Button(vfmt("[X]##{}", i)))
				toRemove = i;
		}

		if (toRemove != m_DrawableCircuit->m_DrawableElements.size())
		{
			if (m_DrawableToOscilloscope.contains(m_DrawableCircuit->m_DrawableElements[toRemove].get()))
				RemoveOscilloscope(m_DrawableCircuit->m_DrawableElements[toRemove]);
			
			m_DrawableCircuit->RemoveElement(toRemove);
			m_DrawableCircuit->SyncWithCircuit();
		}
		ImGui::EndTable();
	}


	if (ImGui::Button("Add Wire"))
		m_DrawableCircuit->AddWire();
	
	if (ImGui::Button("Add Battery"))
		m_DrawableCircuit->AddBattery();

	if (ImGui::Button("Add Resistor"))
		m_DrawableCircuit->AddResistor();

	if (ImGui::Button("Add Capacitor"))
		m_DrawableCircuit->AddCapacitor();

	if (ImGui::Button("Add Inductor"))
		m_DrawableCircuit->AddInductor();

	if (ImGui::Button("Add Diode"))
		m_DrawableCircuit->AddDiode();

	if (ImGui::Button("Add Button"))
		m_DrawableCircuit->AddButton();

	if (ImGui::Button("Add Neutral Coil"))
		m_DrawableCircuit->AddNeutralRelayCoil();

	if (ImGui::Button("Add neutral coil with 3rd relyability class"))
		m_DrawableCircuit->AddNeutralRelayCoil3Class();

	if (ImGui::Button("Add neitral coil with delay"))
		m_DrawableCircuit->AddNeutralRelayCoilWithDelay();

	if (ImGui::Button("Add neutral coil with delay and 3rd relyability class"))
		m_DrawableCircuit->AddNeutralRelayCoilWithDelay3Class();

	if (ImGui::Button("Add neutral coil with rectifier"))
		m_DrawableCircuit->AddNeutralRelayCoilWithDiode();

	if (ImGui::Button("Add Transformer"))
		m_DrawableCircuit->AddTransformer();
	 
	if (ImGui::Button("Add diode bridge"))
		m_DrawableCircuit->AddDiodeBridge();


	ImGui::Separator();

	ImGui::Text("Connection Dots");
	for (auto& [dot, node] : m_DrawableCircuit->m_Connections)
	{
		ImGui::Separator();
		
		//ImGui::Text("NodeIdx: %d\nNode voltage: %.2f V\nDot pos: %.2f, %.2f", node->GetIndex(), node->GetVoltage(), dot->GetPosition().x, dot->GetPosition().y);
		bool opened = ImGui::TreeNode(vfmt("Node: {}##{}", node->GetIndex(), u64(dot.get())));
		if (ImGui::IsItemHovered())
			dot->IsHoveredInUi = true;
		else
			dot->IsHoveredInUi = false;

		ImGui::SameLine(200.0f);
		ImGui::Text("V: %.3f", node->GetVoltage());


		if (opened)
		{
			ImGui::Indent();
			
			ImGui::Text("Dot pos: %.2f, %.2f", dot->GetPosition().x, dot->GetPosition().y);

			if (ImGui::Button(vfmt("Set as ground##{}", u64(dot.get()))))
				m_DrawableCircuit->GetCircuit()->SetGroundNode(node);

			ImGui::Text("Connected elements: %d", dot->GetNumConnectedElements());

			ImGui::Indent();
			for (auto& [elem, pinIndex] : dot->GetConnectedElements())
			{
				if (auto e = elem.lock())
				{
					bool isWire = e->IsWire();

					eElement* electricElem = m_DrawableCircuit->GetElecticElementFromDrawable(e.get());
					ImGui::Text("Element: %s", isWire ? "Wire" : electricElem->GetTypeName());
					ImGui::Text("Pin index: %d", pinIndex);
				}
			}
			ImGui::Unindent();
			ImGui::Unindent();
			
			ImGui::TreePop();
		}
	}


	float PositiveVoltColor[4] = { Wire::PositiveVoltColor.r / 255.0f, Wire::PositiveVoltColor.g / 255.0f, Wire::PositiveVoltColor.b / 255.0f, Wire::PositiveVoltColor.a / 255.0f };
	float NegativeVoltColor[4] = { Wire::NegativeVoltColor.r / 255.0f, Wire::NegativeVoltColor.g / 255.0f, Wire::NegativeVoltColor.b / 255.0f, Wire::NegativeVoltColor.a / 255.0f };
	float CurrentDotsColor[4] = { Wire::CurrentDotsColor.r / 255.0f, Wire::CurrentDotsColor.g / 255.0f, Wire::CurrentDotsColor.b / 255.0f, Wire::CurrentDotsColor.a / 255.0f };

	if (ImGui::ColorEdit4("Positive volt color", PositiveVoltColor))
	{
		Wire::PositiveVoltColor = sf::Color(PositiveVoltColor[0] * 255, PositiveVoltColor[1] * 255, PositiveVoltColor[2] * 255, PositiveVoltColor[3] * 255);
		m_DrawableCircuit->UpdateWireColors();
	}

	if (ImGui::ColorEdit4("Negative volt color", NegativeVoltColor))
	{
		Wire::NegativeVoltColor = sf::Color(NegativeVoltColor[0] * 255, NegativeVoltColor[1] * 255, NegativeVoltColor[2] * 255, NegativeVoltColor[3] * 255);
		m_DrawableCircuit->UpdateWireColors();
	}

	if (ImGui::ColorEdit4("Current dots color", CurrentDotsColor))
	{
		Wire::CurrentDotsColor = sf::Color(CurrentDotsColor[0] * 255, CurrentDotsColor[1] * 255, CurrentDotsColor[2] * 255, CurrentDotsColor[3] * 255);
		m_DrawableCircuit->UpdateWireColors();
	}
	ImGui::SliderFloat("Current speed", &Wire::CurrentSpeedScalar, 0.0f, 100.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);

	ImGui::Separator();

	//if (ImGui::Button("Solve"))
	//{
	//	m_DrawableCircuit->SyncWithCircuit();
	//	m_DrawableCircuit->m_Circuit->GetMatrix().Clear();
	//	m_DrawableCircuit->m_Circuit->StampElements(0.001);
	//	m_DrawableCircuit->m_Circuit->Solve();
	//	m_DrawableCircuit->m_Circuit->UpdateElements(0.000005);
	//	m_DrawableCircuit->UpdateWireColors();
	//}

	std::stringstream ss;
	m_DrawableCircuit->m_Circuit->GetMatrix().Print(ss);
	ImGui::Text(ss.str().c_str());


	if (m_EditableWire || ImGui::GetIO().WantCaptureMouse)
		m_DraggableElement = nullptr; // Make sure elements are not draggable while we are editing wire or mouse is not captured by imgui

	HandleDraggableElem(mousePos, DragOffset);
	HandleWireEditing();

	ImGui::End();
}


void CircuitEditor::HandleDraggableElem(sf::Vector2f mousePos, sf::Vector2f DragOffset)
{
	if (!m_DraggableElement)
		return;
	
	if (ImGui::IsKeyPressed(ImGuiKey_R, false))
	{
		float elemRotation = m_DraggableElement->GetRotation().asDegrees();
		elemRotation += 45.0f;
		m_DraggableElement->SetRotation(std::fmod(elemRotation, 360.0f));
	}

	if (ImGui::IsKeyPressed(ImGuiKey_X, false))
	{
		m_DraggableElement->Flip(flipAxis::X);
	}
	if (ImGui::IsKeyPressed(ImGuiKey_Y, false))
	{
		m_DraggableElement->Flip(flipAxis::Y);
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift))
	{
		sf::Vector2f alignedPos = SnapToGrid(mousePos - DragOffset + m_DraggableElement->GetLocalPinPosition(0));
		m_DraggableElement->SetPosition(alignedPos - m_DraggableElement->GetLocalPinPosition(0));
	}
	else
	{
		m_DraggableElement->SetPosition(mousePos - DragOffset);
	}

	if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
	{
		m_DraggableElement = nullptr;
	}

	UpdateConnectionDots();
}



void CircuitEditor::HandleWireEditing()
{
	if (!m_EditableWire)
		return;
	
	auto& wire = *m_EditableWire;
	sf::Vector2f MousePos = gSFMLRenderer.GetWorldMousePos();
	auto* segments = &wire.GetSegments();
	
	if (segments->size() == 0)
	{
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift))
			MousePos = SnapToGrid(MousePos);
	
		DrawCircle(MousePos, Wire::WireThickness, sf::Color::Black);
		
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			segments->push_back({ MousePos, MousePos });
		
		m_WireEditOnStart = false;
	}
	else
	{
		auto* lastSegment = &segments->back();
		if (m_WireEditOnStart)
		{
			segments->push_back({ lastSegment->vEnd, MousePos });
			lastSegment = &segments->back();
			m_WireEditOnStart = false;
		}

		lastSegment->vEnd = MousePos;
		sf::Vector2f& end = lastSegment->vEnd;


		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift))
			end = SnapToGrid(end);

		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			segments->push_back({ end, MousePos });
	}


	if (ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_Backspace, false))
	{
		if (segments->size() != 0)
			segments->pop_back();
	}


	if (ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_Escape, false))
	{
		if (segments->size() != 0)
			segments->pop_back();

		UpdateAllWireConnections();
		m_DrawableCircuit->SyncWithCircuit();

		m_EditableWire = nullptr;
	}
}


void CircuitEditor::UpdateAllWireConnections()
{
	for (size_t i = 0; i < m_DrawableCircuit->m_DrawableElements.size(); i++)
	{
		auto& elem = m_DrawableCircuit->m_DrawableElements[i];
		if (elem->IsWire())
		{
			Wire* w = elem->As<Wire>();
			if (w->GetSegments().size() == 0)
			{
				w->DisconnectFromDotAt<Wire::Start>();
				w->DisconnectFromDotAt<Wire::End>();
			}
			else
			{
				UpdateConnectionData<Wire::Start>(i, w->GetSegments().front().vStart);
				UpdateConnectionData<Wire::End>(i, w->GetSegments().back().vEnd);
			}
		}
	}

	UpdateConnectionDots();
}




template<Wire::PinPoint StartOrEnd>
void CircuitEditor::UpdateConnectionData(u64 WireIndex, sf::Vector2f& point)
{
	auto& elem = m_DrawableCircuit->m_DrawableElements[WireIndex];
	Wire* wire = elem->As<Wire>();

	if (std::optional dotOpt = GetClosestConnectionDot(point)) // Position is close to some connection dot
	{
		wire->DisconnectFromDotAt<StartOrEnd>();

		auto& dot = *dotOpt;
		point = dot->GetPosition();
		wire->ConnectToDotAt<StartOrEnd>(dot);
	}
	else if (std::optional pinInfo = GetClosestElementPinInfo(point)) // Position is close to some pin
	{
		wire->DisconnectFromDotAt<StartOrEnd>();

		auto& [pos, pinIndex, otherElem] = *pinInfo;
		point = pos;
		auto newDot = m_DrawableCircuit->CreateConnectionDot(point);
		newDot->Connect(otherElem, pinIndex);
		wire->ConnectToDotAt<StartOrEnd>(newDot);
		newDot->SetPosition(point);
	}
	else if (std::optional otherWireData = SearchForNearestWire(point, point, elem)) // we're close to another wire
	{
		wire->DisconnectFromDotAt<StartOrEnd>();
		auto& [otherWireBase, segmentIndex] = *otherWireData;
		Wire* otherWire = otherWireBase->As<Wire>();
		auto& otherSegments = otherWire->GetSegments();
		
		if (segmentIndex == 0 && point == otherSegments.front().vStart)
		{
			std::shared_ptr newDot = m_DrawableCircuit->CreateConnectionDot(point);
			otherWire->ConnectToDotAt<Wire::Start>(newDot);
			wire->ConnectToDotAt<StartOrEnd>(newDot);
		}
		else if (segmentIndex == otherSegments.size() - 1 && point == otherSegments.back().vEnd)
		{
			std::shared_ptr newDot = m_DrawableCircuit->CreateConnectionDot(point);
			otherWire->ConnectToDotAt<Wire::End>(newDot);
			wire->ConnectToDotAt<StartOrEnd>(newDot);
		}
		else if (std::shared_ptr middleDot = m_DrawableCircuit->SplitWire(otherWireBase, segmentIndex, point))
		{
			wire->ConnectToDotAt<StartOrEnd>(middleDot);
		}
	}
	else
	{
		wire->DisconnectFromDotAt<StartOrEnd>();
	}
}



void CircuitEditor::UpdateConnectionDots()
{
	m_DrawableCircuit->CleanupConnections();
	for (auto& [dot, node] : m_DrawableCircuit->m_Connections)
	{
		dot->SetPosition(dot->GetPosition()); // update connected elements positions
	}
}
