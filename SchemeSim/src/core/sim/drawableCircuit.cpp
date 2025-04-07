#include "drawableCircuit.h"


void eDrawableBase::SetRotation(float degrees)
{
	const sf::Angle NewRotation = sf::degrees(degrees);
	for (sf::Vector2f& pos : m_PinPositions)
	{
		sf::Vector2f initialPos = pos.rotatedBy(-m_Rotation);
		pos = initialPos.rotatedBy(NewRotation);
	}
	
	m_Rotation = NewRotation;
	m_sprite.setRotation(m_Rotation);
}


//----------------------------------------------------------------------------------------------------------------------------------------
//											ConnectionPoint

ConnectionDot::ConnectionDot(sf::Vector2f pos)
	: m_Position(pos)
{ }


void ConnectionDot::UpdateVisibility()
{
	for (auto& [weak, pin] : m_ConnectedElements)
		if (auto drawable = weak.lock())
			if (!drawable->IsWire()) 
			{
				m_ToElemPin = true;
				break;
			}
			else
			{
				m_ToElemPin = false;
			}
}


void ConnectionDot::Draw()
{
	if (!m_ToElemPin)
		DrawCircle(m_Position, Wire::WireThickness * 1.3f, sf::Color::Black);

	if (IsHoveredInUi)
		DrawCircle(m_Position, Wire::WireThickness * 1.5f, sf::Color::Red);
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


bool ConnectionDot::contains(std::weak_ptr<eDrawableBase> element) const
{
	return std::ranges::find_if(m_ConnectedElements, [element](const auto& pair) { return pair.first.lock() == element.lock(); }) != m_ConnectedElements.end();
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
//											Resistor



Resistor::Resistor()
	: eDrawableBase("assets\\resistor.png")
{
	GetTexture().setSmooth(true);
	m_PinPositions =
	{
		{ 0.0f, 184.0f }, 
		{ float(m_sprite.getTextureRect().size.x), 184.0f }
	};
}


void Resistor::UIParams(eElement* elem)
{
	eResistor* resistor = static_cast<eResistor*>(elem);
	ImGui::DragScalar(vfmt("Resistance##{}", u64(this)), ImGuiDataType_Double, &resistor->m_Resistance, 0.1f, nullptr, nullptr, "%.2f Ohm");
	ImGui::Text("Current: %.5f A", resistor->GetCurrent());
}


sf::Vector2f	Resistor::GetLocalPinPosition(int n)							{ return n == 0 ? m_PinPositions[0] : m_PinPositions[1]; }
int				Resistor::GetPinIndexFromLocalPosition(sf::Vector2f pos)		{ return pos == m_PinPositions[0] ? 0 : 1; }
void			Resistor::Draw()												{ dlDrawList::getWindow()->draw(m_sprite); }


//----------------------------------------------------------------------------------------------------------------------------------------
//											Battery

Battery::Battery()
	: eDrawableBase("assets\\battery.png")
{
	GetTexture().setSmooth(true);
	m_PinPositions =
	{
		{ float(m_sprite.getTextureRect().size.x), 131.0f }, // +
		{ 0.0f, 131.0f }, // -
	};
}


void Battery::UIParams(eElement* elem)
{
	eVoltageSource* vs = static_cast<eVoltageSource*>(elem);
	if (!vs)
		return;

	ImGui::DragScalar(vfmt("Voltage/Amplitude##{}", u64(this)),		ImGuiDataType_Double, &vs->m_Amplitude, 0.1f, nullptr, nullptr, "%.2f V");
	ImGui::DragScalar(vfmt("Frequency##{}", u64(this)),				ImGuiDataType_Double, &vs->m_Frequency, 0.1f, nullptr, nullptr, "%.2f Hz");
	ImGui::DragScalar(vfmt("Phase##{}", u64(this)),					ImGuiDataType_Double, &vs->m_Phase,		0.1f, nullptr, nullptr, "%.2f rad");

	ImGui::Text("Current: %.5f A", vs->GetCurrent(1));
}


int				Battery::GetPinIndexFromLocalPosition(sf::Vector2f pos)				{ return pos == m_PinPositions[0] ? 0 : 1; }
sf::Vector2f	Battery::GetLocalPinPosition(int n)									{ return n == 0 ? m_PinPositions[0] : m_PinPositions[1]; }
void			Battery::Draw()														{ dlDrawList::getWindow()->draw(m_sprite); }


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

	sf::Vector2f p1 = start + half;	   // p1 ______ p4
	sf::Vector2f p2 = start - half;	   //    |   /|
	sf::Vector2f p3 = end - half; 	   //    |  / |
	sf::Vector2f p4 = end + half;	   //    | /  |
									   // p2 |/___| p3
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

//	for (auto& segment : m_Segments)
//	{
//		DrawThickLine(segment.vStart, segment.vEnd, WireThickness, m_WireColor);
//	}

	float totalLength = 0.0f;
	for (const auto& segment : m_Segments)
		totalLength += segment.length();

	float lengthsSum = 0.0f;
	for (const auto& segment : m_Segments)
	{
		float SegLength = segment.length();
		float tStart = lengthsSum / totalLength;
		float tEnd = (lengthsSum + SegLength) / totalLength;

		sf::Color colorStart = lerpColor(m_StartColor, m_EndColor, tStart);
		sf::Color colorEnd = lerpColor(m_StartColor, m_EndColor, tEnd);
		DrawThickLineWithGradient(segment.vStart, segment.vEnd, WireThickness, colorStart, colorEnd);

		lengthsSum += SegLength;
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
	eElement* elem = circuit.GetElecticElement(this);
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
	u8 StartIntensity = u8(std::min(std::abs(StartVolt) / MaxVolt, 1.0) * 255);
	u8 EndIntensity = u8(std::min(std::abs(EndVolt) / MaxVolt, 1.0) * 255);

	if (StartVolt >= 0)
		m_StartColor = sf::Color(StartIntensity, 0, 0, 255);
	else
		m_StartColor = sf::Color(0, 0, StartIntensity, 255);

	if (EndVolt >= 0)
		m_EndColor = sf::Color(EndIntensity, 0, 0, 255);
	else
		m_EndColor = sf::Color(0, 0, EndIntensity, 255);
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
//											DrawableCircuit



std::optional<std::shared_ptr<ConnectionDot>> DrawableCircuit::SplitWire(std::shared_ptr<eDrawableBase> wireBase, u64 SegmentIndex, sf::Vector2f SplitPoint)
{
	if (!wireBase || !wireBase->IsWire())
		return std::nullopt;

	auto itLeft = std::ranges::find_if(m_DrawableElements, [wireBase](const auto& e) { return e.get() == wireBase.get(); });
	if (itLeft == m_DrawableElements.end())
		return std::nullopt;
	
	Wire* wireLeft = wireBase->As<Wire>();
	if (SegmentIndex >= wireLeft->GetSegments().size())
		return std::nullopt;
	
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
				eElement* elem = m_DrawableToElement.at(drawable.get());
				elem->GetEpin(pinIndex)->ConnectToNode(node);
			}
		}
	}

	m_Circuit->RebuildMatrix();
}


void DrawableCircuit::Draw()
{
	for (auto& elem : m_DrawableElements)
	{
		elem->Draw();
	}
	for (auto& [dot, node] : m_Connections)
	{
		dot->Draw();
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
		{
			elem->As<Wire>()->UpdateColor(*this);
		}
	}
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

			sf::Vector2f pointDir = point - segStart;
			float t = std::max(0.f, std::min(1.f, ( pointDir.dot(dir)) / (length * length) ));
			return segStart + dir * t;
		};


	auto minIt = std::ranges::min_element(segments, 
		[&](const Wire::Segment& a, const Wire::Segment& b)
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
		ClosestWirePointInfo result
		{
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


std::optional<std::pair<std::shared_ptr<eDrawableBase>, u64>> CircuitEditor::SearchForConnectionWithOtherWire(const sf::Vector2f& MousePos, sf::Vector2f& OutEnd, std::shared_ptr<eDrawableBase> selfWire)
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




CircuitEditor::CircuitEditor(DrawableCircuit& circuit)
	: m_DrawableCircuit(&circuit) 
	, m_Parser(this)
{ }

void CircuitEditor::DrawUI()
{
	auto PushTextColor = [](sf::Color color) { ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f)); };
	auto PopTextColor = []() { ImGui::PopStyleColor(); };


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
	}


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
			Wire* wire = drawable->As<Wire>();
			
			bool isHovered = !isWire && drawable->is_hovered();

			ImGui::TableNextRow();
			ImGui::TableNextColumn();

			if (isHovered)
				PushTextColor(sf::Color::Red);
			ImGui::Text(vfmt("Elements[{}]: {}",i, isWire ? "Wire" : elem->GetTypeName()));
			
			if (isHovered)
				PopTextColor();


			if (isWire && ImGui::Button(vfmt("Edit Wire ##{}", i)))
			{
				m_EditableWire = wire;
				m_WireEditOnStart = true;
			}
			else
			{
				if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !m_DraggableElement && !m_EditableWire)
				{
					m_DraggableElement = drawable.get();
					DragOffset = mousePos - drawable->GetPosition();
				}

				if (isHovered && ImGui::IsKeyPressed(ImGuiKey_R, false) && !m_EditableWire)
				{
					float elemRotation = drawable->GetRotation().asDegrees();
					drawable->SetRotation(elemRotation + 90.0f >= 360.0f ? 0.0f : elemRotation + 90.0f);
					UpdateConnectionDots();
				}
				ImGui::Indent();
					drawable->UIParams(elem);			
				ImGui::Unindent();
			}

			ImGui::TableNextColumn();
			if (ImGui::Button(vfmt("[X]##{}", i)))
				toRemove = i;
		}

		if (toRemove != m_DrawableCircuit->m_DrawableElements.size())
			m_DrawableCircuit->RemoveElement(toRemove);

		ImGui::EndTable();
	}


	if (ImGui::Button("Add Wire"))
		m_DrawableCircuit->AddWire();

	if (ImGui::Button("Add Resistor"))
		m_DrawableCircuit->AddResistor();


	if (ImGui::Button("Add Battery"))
		m_DrawableCircuit->AddBattery();

	ImGui::Separator();

	ImGui::Text("Connection Dots");
	for (auto& [dot, node] : m_DrawableCircuit->m_Connections)
	{
		ImGui::Separator();
		ImGui::Indent();
		ImGui::Text("Node voltage: %.2f V", node->GetVoltage());
		if (ImGui::Button(vfmt("Set as ground##{}",u64(dot.get()))))
		{
			m_DrawableCircuit->GetCircuit()->SetGroundNode(node);
		}

		ImGui::Text("Dot pos: %.2f, %.2f", dot->GetPosition().x, dot->GetPosition().y);
		ImGui::Text("Connected elements: %d", dot->GetNumConnectedElements());
		if (ImGui::IsItemHovered())
		{
			dot->IsHoveredInUi = true;
		}
		else
		{
			dot->IsHoveredInUi = false;
		}

		ImGui::Indent();
		for (auto& [elem, pinIndex] : dot->GetConnectedElements())
		{
			if (auto e = elem.lock())
			{
				bool isWire = e->IsWire();

				eElement* electricElem = m_DrawableCircuit->m_DrawableToElement.at(e.get());
				ImGui::Text("Element: %s", isWire ? "Wire" : electricElem->GetTypeName());
				ImGui::Text("Pin index: %d", pinIndex);
			}
		}
		ImGui::Unindent();
		ImGui::Unindent();
	}

	ImGui::Separator();
	if (ImGui::Button("Sync With Circuit"))
	{
		m_DrawableCircuit->SyncWithCircuit();
	}

	if (ImGui::Button("Solve"))
	{
		m_DrawableCircuit->m_Circuit->GetMatrix().Clear();
		m_DrawableCircuit->m_Circuit->StampElements();
		m_DrawableCircuit->m_Circuit->Solve();
		m_DrawableCircuit->m_Circuit->UpdateElements(0.000005);
		m_DrawableCircuit->UpdateWireColors();
	}

	ImGui::Text("Circuit Matrix");
	std::stringstream ss;
	m_DrawableCircuit->m_Circuit->GetMatrix().Print(ss);
	ImGui::Text(ss.str().c_str());




	if (m_EditableWire)
		m_DraggableElement = nullptr; // Make sure elements are not draggable while we are editing wire

	HandleDraggableElem(mousePos, DragOffset);
	HandleWireEditing();

	ImGui::End();
}


void CircuitEditor::HandleDraggableElem(sf::Vector2f mousePos, sf::Vector2f DragOffset)
{
	if (!m_DraggableElement)
		return;
	
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
		newDot->SetPosition(point);
		newDot->Connect(otherElem, pinIndex);
		wire->ConnectToDotAt<StartOrEnd>(newDot);
		newDot->UpdateConnectedElementsPositions();
	}
	else if (std::optional otherWireData = SearchForConnectionWithOtherWire(point, point, elem)) // we're close to another wire, split it and connect there
	{
		wire->DisconnectFromDotAt<StartOrEnd>();
		auto& [otherWireBase, segmentIndex] = *otherWireData;
		if (std::optional opt = m_DrawableCircuit->SplitWire(otherWireBase, segmentIndex, point))
		{
			std::shared_ptr middleDot = *opt;
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
		dot->UpdateConnectedElementsPositions();
	}
}


void CircuitParser::LoadFromFile(const std::filesystem::path& path)
{
	if (!m_editor || !m_editor->m_DrawableCircuit)
		return;

	DrawableCircuit* drawableCirc = m_editor->m_DrawableCircuit;

	std::ifstream file(path);
	if (!file.is_open())
		return;

	std::vector<std::string> lines;
	{
		std::string line;
		while (std::getline(file, line))
			lines.push_back(line);
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
				std::cout << "Creating dot at: " << line << "\n";
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
				switch (type)
				{
				case DrawableType::ResistorType:
					drawableCirc->AddResistor();
					break;
				case DrawableType::BatteryType:
					drawableCirc->AddBattery();
					break;
				default:
					break;
				}

				sf::Vector2f pos = ReadVec2f(lines[++i]);
				float rot = ReadFloat(lines[++i]);

				drawableCirc->m_DrawableElements.back()->SetPosition(pos);
				drawableCirc->m_DrawableElements.back()->SetRotation(rot);
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
		if (dot->HasConnectionWithElem())
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
	for (auto& elemBase : elements)
	{
		if (elemBase->IsWire())
			continue;
		file << "next\n";

		DrawableType type = elemBase->GetType();
		file << int(type) << "\n";
		
		sf::Vector2f pos = elemBase->GetPosition();
		float rot = elemBase->GetRotation().asDegrees();
		WriteVec2f(file, pos);
		WriteFloat(file, rot);
	}
	file << "ELEMENTS_END\n";
	
	file.flush();
	file.close();
}
