#include "circuit.h"

void Circuit::Reset()
{
	m_Elements.clear();
	m_Nodes.clear();
	m_Matrix.Reset();

	m_GroundNode = nullptr;
	m_CurrTime = 0.0;
}


void Circuit::ResetElements()
{
	for (auto& elem : m_Elements)
	{
		elem->Reset();
	}
}


eNode* Circuit::CreateNode()
{
	u64 index = m_Nodes.size();
	auto& node = m_Nodes.emplace_back(std::make_unique<eNode>(index));

	int n = m_Nodes.size() - 1;
	if (n > 0)
		m_Matrix.Resize(n); // n-1 (Without the ref gnd)

	return node.get();
}


void Circuit::RemoveNode(eNode* node)
{
	if (!node)
		return;

	if (node == m_GroundNode)
		m_GroundNode = nullptr;

	auto it = std::ranges::find_if(m_Nodes, [node](const UPtrNodeTy& n)
		{
			return n.get() == node;
		});

	if (it != m_Nodes.end())
		m_Nodes.erase(it);

	RebuildMatrix();
}


void Circuit::RemoveElement(eElement* element)
{
	auto it = std::ranges::find_if(m_Elements, [element](const UPtrElementTy& e) { return e.get() == element; });

	if (it != m_Elements.end())
	{
		m_Elements.erase(it);			// Pins will be released from connected nodes in destructor
		RebuildMatrix();
	}
}


void Circuit::Connect(ePin* pin, eNode* node)
{
	if (pin && node)
		pin->ConnectToNode(node);
}


void Circuit::StampElements()
{
	if (!m_GroundNode)
		m_GroundNode = LookupGroundNode();

	for (auto& element : m_Elements)
	{
		element->Stamp(m_Matrix, m_GroundNode, 0.0f);
	}
}


void Circuit::RebuildMatrix()
{
	m_Matrix.Reset();
	if (m_Nodes.empty())
		return;

	ResetElements();

	for (size_t i = 0; i < m_Nodes.size(); ++i)
		m_Nodes[i]->SetIndex(i);

	if (!m_GroundNode)
		m_GroundNode = LookupGroundNode();

	m_Matrix.Resize(m_Nodes.size() - 1);
	FinalizeMatrixSize();
}


eNode* Circuit::LookupGroundNode()
{
	if (m_Nodes.empty())
		return nullptr;

	u64 MaxCountConnectedPins = 0;			// Ground node is often the one with the largest number of connected elements
	eNode* groundNode = nullptr;
	{
		for (auto& node : m_Nodes)
		{
			if (node->GetNumConnectedPins() > MaxCountConnectedPins)
			{
				MaxCountConnectedPins = node->GetNumConnectedPins();
				groundNode = node.get();
			}
		}

		if (MaxCountConnectedPins >= 5) // 5 or more should be enough
			return groundNode;
	}


	eVoltageSource* voltSource = nullptr;		// Looking for a voltage source with the highest number of connectd pins on its negative output
	MaxCountConnectedPins = 0;

	for (auto& elem : m_Elements)
	{
		if (eVoltageSource* source = dynamic_cast<eVoltageSource*>(elem.get()))
		{
			if (eNode* node = source->GetNegativePin()->GetConnectedNode())
			{
				if (node->GetNumConnectedPins() > MaxCountConnectedPins)
				{
					MaxCountConnectedPins = node->GetNumConnectedPins();
					voltSource = source;
				}
			}
		}
	}

	if (voltSource)
		groundNode = voltSource->GetNegativePin()->GetConnectedNode();

	return groundNode;
}


void Circuit::AdjustVoltages(eNode* ToDesiredGround)
{
	if (!ToDesiredGround || ToDesiredGround == m_GroundNode)
		return;

	double offset = ToDesiredGround->GetVoltage();
	offset = -offset;

	for (auto& node : m_Nodes)
	{
		node->SetVoltage(node->GetVoltage() + offset);
	}
}


u64 Circuit::GetNodeMtxIndex(eNode* node)
{
	u64 nodeIdx = node->GetIndex();
	u64 gndIdx = m_GroundNode->GetIndex();

	return (nodeIdx > gndIdx) ? nodeIdx - 1 : nodeIdx;
}


void Circuit::Solve()
{
	m_Matrix.Solve();
	for (auto& node : m_Nodes)
	{
		if (node.get() == m_GroundNode)
		{
			node->SetVoltage(0.0);
		}
		else
		{
			node->SetVoltage(m_Matrix.GetVoltage(GetNodeMtxIndex(node.get())));
		}
	}
}


void Circuit::FinalizeMatrixSize()
{
	for (auto& element : m_Elements)
	{
		element->InitMatrix(m_Matrix);
	}
}


Circuit::ResultsType Circuit::Simulate(double totalTime, double dt)
{
	ResultsType results;
	if (!m_GroundNode)
		m_GroundNode = LookupGroundNode();

	u64 steps = u64(totalTime / dt);
	std::ofstream file("results.txt");
	file << "Steps: " << steps << '\n';
	file << "Total Time: " << totalTime << '\n';
	file << "Dt: " << dt << '\n';


	for (u64 i = 0; i < steps; ++i)
	{
		file << std::format("Step: {}  Time: {:.7f}\n", i, m_CurrTime);

		m_Matrix.Clear();

		for (auto& element : m_Elements)
		{
			element->Stamp(m_Matrix, m_GroundNode, dt);
		}
		

		m_Matrix.Solve();
		for (auto& node : m_Nodes)
		{
			if (node.get() == m_GroundNode)
			{
				node->SetVoltage(0.0);
			}
			else
			{
				node->SetVoltage(m_Matrix.GetVoltage(GetNodeMtxIndex(node.get())));
			}
		}

		for (auto& element : m_Elements)
		{
			element->Update(m_Matrix, dt);
		}

		m_Matrix.Print(file);
		m_CurrTime += dt;
		for (auto& node : m_Nodes)
		{
			auto idx = node->GetIndex();
			results[idx].emplace_back(m_CurrTime, node->GetVoltage());

			file << "Node " << idx << " : " << node->GetVoltage() << '\n';
		}
		file << "-----------------------------------\n";
		
		if (m_CurrTime > 0.25)
		{
			static auto once = [&]()
				{ 
					if (eButton* button = static_cast<eButton*>(m_Elements.back().get()))
						if (button->GetType() == ty_Button)
							button->Release();
					
					return 1;
				}();
		}
	}

	file.close();
	return results;
}




void Circuit::Test1()
{
	// (v1)---n0--(R1)---n1-----(p)---n2---(R2)----
	//  |                        |                |
	//  |                        |                |
	//  |                        |                |
	//  -------------gnd---------------------------

	eNode* n0 = CreateNode();
	eNode* n1 = CreateNode();
	eNode* n2 = CreateNode();
	eNode* gnd_node = CreateNode();

	ePotentiometer* p = AddElement<ePotentiometer>(*this, 10, 0.5);
	eResistor* r1 = AddElement<eResistor>(5.0);
	eResistor* r2 = AddElement<eResistor>(5.0);
	eVoltageSource* v1 = AddElement<eVoltageSource>(10.0f);

	v1->GetPositivePin()->ConnectToNode(n0);
	v1->GetNegativePin()->ConnectToNode(gnd_node);

	r1->GetEpin(0)->ConnectToNode(n0);
	r1->GetEpin(1)->ConnectToNode(n1);

	p->GetLeftPin()->ConnectToNode(n1);
	p->ConnectOutputPinToNode(n2);
	p->GetRightPin()->ConnectToNode(gnd_node);

	r2->GetEpin(0)->ConnectToNode(n2);
	r2->GetEpin(1)->ConnectToNode(gnd_node);

	m_GroundNode = gnd_node;
	FinalizeMatrixSize();
	StampElements();
	Solve();

	std::cout << "R1 Current : " << r1->GetCurrent() << std::endl;
	std::cout << "R2 Current : " << r2->GetCurrent() << std::endl;

	for (auto& node : m_Nodes)
	{
		std::cout << "Node " << node->GetIndex() << " : " << node->GetVoltage() << std::endl;
	}

	m_Matrix.Print(std::cout);
}


void Circuit::Test3()
{
	//  -------(n2)----(R3)---(n3)-----|
	//  |       |              |       |
	// (R1)     |              |      (R4)
	//  |       |             -|       |
	// (n1)    (R2)           (V2)    (n6)
	//  |+      |             +|       |
	// (V1)     |              |      (R5)
	//  |-      |              |       |
	//  |       |              |       |
	//  |------(n4)---------------------

	// 
	// (n7)------(_r6)----(n8)----(_r7)----(n9)
	// 

	eVoltageSource* v1 = AddElement<eVoltageSource>(2.5);
	eVoltageSource* v2 = AddElement<eVoltageSource>(7.0);
	eResistor* r1 = AddElement<eResistor>(1.0);
	eResistor* r2 = AddElement<eResistor>(2.0);
	eResistor* r3 = AddElement<eResistor>(3.0);
	eResistor* r4 = AddElement<eResistor>(4.0);
	eResistor* r5 = AddElement<eResistor>(5.0);

	eResistor* _r6 = AddElement<eResistor>(5.0);
	eResistor* _r7 = AddElement<eResistor>(5.0);

	eNode* n1 = CreateNode();
	eNode* n2 = CreateNode();
	eNode* n3 = CreateNode();
	eNode* n4 = CreateNode();
	eNode* n6 = CreateNode();
	eNode* n7 = CreateNode();
	eNode* n8 = CreateNode();
	eNode* n9 = CreateNode();

	v1->GetPositivePin()->ConnectToNode(n1);
	v1->GetNegativePin()->ConnectToNode(n4);

	r1->GetEpin(0)->ConnectToNode(n1);
	r1->GetEpin(1)->ConnectToNode(n2);

	r2->GetEpin(1)->ConnectToNode(n2);
	r2->GetEpin(0)->ConnectToNode(n4);

	r3->GetEpin(0)->ConnectToNode(n2);
	r3->GetEpin(1)->ConnectToNode(n3);

	r4->GetEpin(0)->ConnectToNode(n3);
	r4->GetEpin(1)->ConnectToNode(n6);

	v2->GetPositivePin()->ConnectToNode(n4);
	v2->GetNegativePin()->ConnectToNode(n3);

	r4->GetEpin(0)->ConnectToNode(n3);
	r4->GetEpin(1)->ConnectToNode(n6);

	r5->GetEpin(0)->ConnectToNode(n6);
	r5->GetEpin(1)->ConnectToNode(n4);

	_r6->GetEpin(0)->ConnectToNode(n7);
	_r6->GetEpin(1)->ConnectToNode(n8);

	_r7->GetEpin(0)->ConnectToNode(n8);
	_r7->GetEpin(1)->ConnectToNode(n9);


	std::cout << r5->GetEpin(0)->IsConnectedToNode() << std::endl;
	std::cout << r5->GetEpin(1)->IsConnectedToNode() << std::endl;

	FinalizeMatrixSize();
	StampElements();
	Solve();

	std::cout << "R1 Current : " << r1->GetCurrent() << std::endl;
	std::cout << "R2 Current : " << r2->GetCurrent() << std::endl;
	std::cout << "R3 Current : " << r3->GetCurrent() << std::endl;
	std::cout << "R4 Current : " << r4->GetCurrent() << std::endl;
	std::cout << "R5 Current : " << r5->GetCurrent() << std::endl;

	std::cout << "R6 Current : " << _r6->GetCurrent() << std::endl;
	std::cout << "R7 Current : " << _r7->GetCurrent() << std::endl;

	for (auto& node : m_Nodes)
	{
		std::cout << "Node " << node->GetIndex() << " : " << node->GetVoltage() << std::endl;
	}

	m_Matrix.Print(std::cout);
}


Circuit::ResultsType Circuit::Test4(double totalTime)
{
	// p - Potentiometer
	// 
	//            d1
	//  n0--------|>|-----n1-----------
	//  |                 |           |
	// (Vs)             (C1)          (R1)
	//  |                 |           |
	//  ------------------n2------------
	// 

	eNode* n0 = CreateNode();
	eNode* n1 = CreateNode();
	eNode* n2 = CreateNode();


	eVoltageSource* Vs = AddElement<eVoltageSource>(5.0, 10); // 5V, 10Hz
	eDiode* d1 = AddElement<eDiode>();
	eResistor* r1 = AddElement<eResistor>(100);
	eCapacitor* c1 = AddElement<eCapacitor>(0.001); // 1mF

	Vs->GetPositivePin()->ConnectToNode(n0);
	Vs->GetNegativePin()->ConnectToNode(n2);

	d1->GetAnodePin()->ConnectToNode(n0);
	d1->GetCathodePin()->ConnectToNode(n1);

	r1->GetEpin(0)->ConnectToNode(n1);
	r1->GetEpin(1)->ConnectToNode(n2);

	c1->GetEpin(0)->ConnectToNode(n1);
	c1->GetEpin(1)->ConnectToNode(n2);

	m_GroundNode = n2;

	double dt = 0.0001;
	FinalizeMatrixSize();
	ResultsType results = Simulate(totalTime, dt);

	return results;
}


Circuit::ResultsType Circuit::Test5(double totalTime)
{
	//  -------------------
	//  |           d1    |		 d2
	// (Vs)     ---|>|----n1-----|>|----
	//  |       |                      |
	//  |       n0--------(R1)---------n2
	//  |       |                      |
	//  |       |---------(C1)---------|
	//  |       |                      |
	//  |       ---|>|----n3-----|>|----
	//  |           d3    |		  d4
	//  -------------------


	eNode* n0 = CreateNode();
	eNode* n1 = CreateNode();
	eNode* n2 = CreateNode();
	eNode* n3 = CreateNode();

	eVoltageSource* Vs = AddElement<eVoltageSource>(5, 10); // 5V, 10Hz
	eDiode* d1 = AddElement<eDiode>();
	eDiode* d2 = AddElement<eDiode>();
	eDiode* d3 = AddElement<eDiode>();
	eDiode* d4 = AddElement<eDiode>();

	eResistor* r1 = AddElement<eResistor>(100);
	eCapacitor* c1 = AddElement<eCapacitor>(0.001); // 1mF 

	Vs->GetPositivePin()->ConnectToNode(n1);
	Vs->GetNegativePin()->ConnectToNode(n3);

	d1->GetAnodePin()->ConnectToNode(n0);
	d1->GetCathodePin()->ConnectToNode(n1);

	d2->GetAnodePin()->ConnectToNode(n1);
	d2->GetCathodePin()->ConnectToNode(n2);

	d3->GetAnodePin()->ConnectToNode(n0);
	d3->GetCathodePin()->ConnectToNode(n3);

	d4->GetAnodePin()->ConnectToNode(n3);
	d4->GetCathodePin()->ConnectToNode(n2);

	r1->GetEpin(0)->ConnectToNode(n0);
	r1->GetEpin(1)->ConnectToNode(n2);

	c1->GetEpin(0)->ConnectToNode(n0);
	c1->GetEpin(1)->ConnectToNode(n2);

	m_GroundNode = n0;

	double dt = 0.000005;
	FinalizeMatrixSize();
	ResultsType results = Simulate(totalTime, dt);

	return results;
}


Circuit::ResultsType Circuit::Test6(double totalTime)
{
	// n3----(vs)----n0----(R1)----n1---(L1)----n2----(C1)----n3

	eNode* n0 = CreateNode();
	eNode* n1 = CreateNode();
	eNode* n2 = CreateNode();
	eNode* n3 = CreateNode();

	eVoltageSource* vs = AddElement<eVoltageSource>(5.0); // 5V DC
	eResistor* r1 = AddElement<eResistor>(10);// 10 Ohm
	eInductor* l1 = AddElement<eInductor>(1); // 1H
	eCapacitor* c1 = AddElement<eCapacitor>(0.000010); // 10uF

	vs->GetPositivePin()->ConnectToNode(n0);

	r1->GetEpin(0)->ConnectToNode(n0);
	r1->GetEpin(1)->ConnectToNode(n1);

	l1->GetEpin(0)->ConnectToNode(n1);
	l1->GetEpin(1)->ConnectToNode(n2);

	c1->GetEpin(0)->ConnectToNode(n2);
	c1->GetEpin(1)->ConnectToNode(n3);

	vs->GetNegativePin()->ConnectToNode(n3);

	FinalizeMatrixSize();

	double dt = 0.00005;
	return Simulate(totalTime, dt);
}


Circuit::ResultsType Circuit::Test7(double totalTime)
{ 
	//  |-(vs)---n1---(r0)---n2---(btn)---|
	//  |                                 |
	//  n0-------------(l1)---------------n3

	eNode* n0 = CreateNode();
	eNode* n1 = CreateNode();
	eNode* n2 = CreateNode();
	eNode* n3 = CreateNode();

	eVoltageSource* vs = AddElement<eVoltageSource>(5); // 5V DC
	eResistor* r0 = AddElement<eResistor>(100);// 10 Ohm
	eInductor* l1 = AddElement<eInductor>(0.2); // 1H
	eButton* button = AddElement<eButton>(eButton::NormalOpen);
	button->Press();

	vs->GetNegativePin()->ConnectToNode(n0);
	vs->GetPositivePin()->ConnectToNode(n1);

	r0->GetEpin(0)->ConnectToNode(n1);
	r0->GetEpin(1)->ConnectToNode(n2);

	button->GetEpin(0)->ConnectToNode(n2);
	button->GetEpin(1)->ConnectToNode(n3);

	l1->GetEpin(0)->ConnectToNode(n0);
	l1->GetEpin(1)->ConnectToNode(n3);

	m_GroundNode = n0;

	FinalizeMatrixSize();
	double dt = 0.00005;
	return Simulate(totalTime, dt);
}


Circuit::ResultsType Circuit::Test8(double totalTime)
{	              
	//  ----n0---   t ---n2----
	//  |        ) ║ (        |
	// (vs)      ) ║ (       (r)
	//  |        ) ║ (        |
	//  ----n1---     ----n3---

	eNode* n0 = CreateNode();
	eNode* n1 = CreateNode();
	eNode* n2 = CreateNode();
	eNode* n3 = CreateNode();

	eVoltageSource* vs = AddElement<eVoltageSource>(5, 20);
	eResistor* r = AddElement<eResistor>(100);
	eTransformer* t = AddElement<eTransformer>(1.0, 5.0);

	vs->GetPositivePin()->ConnectToNode(n0);
	vs->GetNegativePin()->ConnectToNode(n1);
	
	t->GetPrimaryPin1()->ConnectToNode(n0);
	t->GetPrimaryPin2()->ConnectToNode(n1);
	t->GetSecondaryPin1()->ConnectToNode(n2);
	t->GetSecondaryPin2()->ConnectToNode(n3);

	r->GetEpin(0)->ConnectToNode(n2);
	r->GetEpin(1)->ConnectToNode(n3);

	m_GroundNode = n1;

	FinalizeMatrixSize();
	double dt = 0.00005;
	return Simulate(totalTime, dt);
}



void CircuitEditor::SnapToGrid(sf::Vector2f& end, sf::Vector2f& start)
{
	sf::Vector2f dir = end - start;
	float degAngle = dir.angle().asDegrees();
	float degSnappedAngle = std::round(degAngle / 45.f) * 45.f;
	float radSnapped = sf::degrees(degSnappedAngle).asRadians();

	dir = sf::Vector2f(std::cos(radSnapped), std::sin(radSnapped)) * dir.length();
	end = start + dir;
}


void DrawCircle(sf::Vector2f position, float radius, sf::Color color = sf::Color::Black)
{
	sf::CircleShape circle(radius);
	circle.setFillColor(color);
	circle.setOrigin({ radius, radius });
	circle.setPosition(position);
	dlDrawList::getWindow()->draw(circle);
}



std::optional<CircuitEditor::myOptInfo> CircuitEditor::GetClosestElementPinPos(sf::Vector2f mousePos)
{
	for (auto& elem : m_DrawableCircuit->m_DrawableElements)
	{
		sf::Vector2f elemPos = elem->GetPosition();

		for (const auto& LocalPinPos : elem->GetLocalPinsPositions())
		{
			sf::Vector2f pinPos = elemPos + LocalPinPos; 
			float dist = (mousePos - pinPos).length();
			if (dist < 20.0f)
				return myOptInfo{ pinPos, elem->GetPinIndexFromLocalPosition(LocalPinPos), elem.get()};
		}
	}
	return std::nullopt;
}


sf::Vector2f CircuitEditor::GetClosestPointOnSegment(sf::Vector2f point, sf::Vector2f segStart, sf::Vector2f segEnd)
{
	sf::Vector2f segDir = segEnd - segStart;
	float segLength = segDir.length();

	if (segLength == 0)
		return segStart;

	sf::Vector2f pointDir = point - segStart;
	float t = std::max(0.f, std::min(1.f, (pointDir.dot(segDir)) / (segLength * segLength)));

	return segStart + segDir * t;
}


CircuitEditor::ClosestPointInfo CircuitEditor::FindClosestPointToWire(Wire& wire, sf::Vector2f mousePos, float maxDistance)
{
	auto& segments = *wire.GetSegments();

	if (segments.empty())
		return {};

	auto [minIt, maxIt] = std::minmax_element(segments.begin(), segments.end(),
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

	ClosestPointInfo result{};
	if (minDistance <= maxDistance)
	{
		result.position = closestPoint;
		result.distance = minDistance;
		result.valid = true;
	}
	return result;
}


std::pair<bool, std::weak_ptr<Wire>> CircuitEditor::CheckForConnectionWithWire(const sf::Vector2f& MousePos, sf::Vector2f& pos)
{
	for (auto& wire : m_DrawableCircuit->m_AllWires)
	{
		if (wire.get() == m_EditableWire)
			continue;

		ClosestPointInfo info = FindClosestPointToWire(*wire.get(), MousePos);
		if (info.valid)
		{
			pos = info.position;
			return { true, std::weak_ptr<Wire>(wire) };
		}
	}
	return { false, {} };
}


void CircuitEditor::DrawUI()
{
	ImGui::Begin("Editor");

	if (ImGui::CollapsingHeader("Wires"))
	{
		bool InEditMode = m_EditableWire != nullptr;
		if (InEditMode)
		{
			ImGui::Text("Editing Wire");
		}
		else
		{
			auto& wires = m_DrawableCircuit->m_AllWires;

			if (ImGui::Button("Add Wire"))
				wires.push_back(std::make_shared<Wire>());
			
			ImGui::Separator();

			if (ImGui::BeginTable("Wires", 3, ImGuiTableFlags_Borders))
			{
				auto toRemove = wires.end();
				for (size_t i = 0; i < wires.size(); i++)
				{
					Wire& wire = *wires[i];

					ImGui::TableNextRow();
					ImGui::TableNextColumn();

					if (ImGui::TreeNode(vfmt("Wire {}", i)))
					{
						for (size_t j = 0; j < wire.GetSegments()->size(); j++)
						{
							auto& segment = wire.GetSegments()->at(j);
							ImGui::Separator();
							ImGui::Text(vfmt("Segment {}", j));
							ImGui::InputFloat2(vfmt("Start##{}{}", i, j), &segment.vStart.x, "%.5f");
							ImGui::InputFloat2(vfmt("End##{}{}", i, j), &segment.vEnd.x, "%.5f");
						}
					
						ImGui::TreePop();
					}

					ImGui::TableNextColumn();
					if (ImGui::Button(vfmt("Edit## {}", i)))
					{
						m_EditableWire = &wire;
						m_EditableWire->SetDrawEndDot(false);
						m_EditableWire->SetDrawStartDot(false);
						m_WireEditOnStart = true;
					}
					if (ImGui::IsItemHovered())
						wire.SetColor(sf::Color::Red * sf::Color(255, 255, 255, 100));
					else
						wire.SetColor(sf::Color::Black);

					ImGui::TableNextColumn();
					if (ImGui::Button(vfmt("Remove##{}", i)))
					{
						toRemove = wires.begin() + i;
						if (m_EditableWire == &wire)
							m_EditableWire = nullptr;
					}

				}

				if (toRemove != wires.end())
					m_DrawableCircuit->RemoveWire(toRemove->get());

				ImGui::EndTable();
			}
		}
	}

	if (ImGui::CollapsingHeader("Elements"))
	{
		if (ImGui::Button("Make Resistor"))
			m_DrawableCircuit->AddElement<eResistor, Resistor>(5.0);

		auto& DrawableElements = m_DrawableCircuit->m_DrawableElements;

		for (size_t i = 0; i < DrawableElements.size(); i++)
		{
			eDrawableBase* elem = DrawableElements[i].get();
			eElement* electricElem = m_DrawableCircuit->m_DrawableToElement.at(elem);
			if (ImGui::TreeNode(vfmt("Element {} {}", i , electricElem->GetTypeName())))
			{
				elem->UIParams(electricElem);
				ImGui::TreePop();
			}
		}
	}


	ImGui::End();
	
	HandleWireEditing();
}


void CircuitEditor::HandleWireEditing()
{
	if (!m_EditableWire)
		return;


	sf::Vector2f MousePos = g_SFMLRenderer.GetWorldMousePos();
	auto* segments = m_EditableWire->GetSegments();


	if (segments->size() == 0)
	{
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			segments->push_back({ MousePos, MousePos });
		
		m_WireEditOnStart = false;
	}
	else
	{
		auto* lastWire = &segments->back();
		if (m_WireEditOnStart)
		{
			segments->push_back({ lastWire->vEnd, MousePos });
			lastWire = &segments->back();
			m_WireEditOnStart = false;
		}

		lastWire->vEnd = MousePos;

		sf::Vector2f& start = lastWire->vStart;
		sf::Vector2f& end = lastWire->vEnd;


		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift))
			SnapToGrid(end, start);

		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
		{
			segments->push_back({ end, MousePos });
		}
	}


	if (ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_Escape, false))
	{
		if (segments->size() != 0)
			segments->pop_back();

		if (segments->size() == 0)
		{
			m_EditableWire->SetDrawStartDot(false);
			m_EditableWire->SetDrawEndDot(false);
			m_EditableWire = nullptr;
			return;
		}
		

		std::optional closestPinPosEnd = GetClosestElementPinPos(segments->back().vEnd);
		std::optional closestPinPosStart = GetClosestElementPinPos(segments->front().vStart);

		if (closestPinPosEnd) // Wire end is connected to a pin
		{
			auto& [pos, pinIndex, elem] = *closestPinPosEnd;

			segments->back().vEnd = pos;
			m_EditableWire->SetDrawEndDot(false);
			m_EditableWire->SetConnectionTypeAtEnd(Wire::ConnectionType::ToPin);
			m_EditableWire->GetDataAtEnd() = Wire::DataToPin{ elem, pinIndex };
		}
		else if (auto [ConnectedToWire, wirePtr] = CheckForConnectionWithWire(segments->back().vEnd, segments->back().vEnd); ConnectedToWire) // Wire end is connected to another wire
		{
			m_EditableWire->SetDrawEndDot(ConnectedToWire);
			m_EditableWire->SetConnectionTypeAtEnd(Wire::ConnectionType::ToWire);
			m_EditableWire->GetDataAtEnd() = Wire::DataToWire{ wirePtr };
		}
		else
		{
			// Wire end is not connected to anything
			m_EditableWire->SetConnectionTypeAtEnd(Wire::ConnectionType::ToNothing);
			m_EditableWire->SetDrawEndDot(false);
			m_EditableWire->GetDataAtEnd() = Wire::DataToNothing{};
		}
		

		//Same as above but for the start
		if (closestPinPosStart) // Wire start is connected to a pin
		{
			auto& [pos, pinIndex, elem] = *closestPinPosStart;

			segments->front().vStart = pos;
			m_EditableWire->SetDrawStartDot(false);
			m_EditableWire->SetConnectionTypeAtStart(Wire::ConnectionType::ToPin);
			m_EditableWire->GetDataAtStart() = Wire::DataToPin{ elem, pinIndex };
		}
		else if (auto [ConnectedToWire, wirePtr] = CheckForConnectionWithWire(segments->front().vStart, segments->front().vStart); ConnectedToWire) // Wire start is connected to another wire
		{
			m_EditableWire->SetDrawStartDot(true);
			m_EditableWire->SetConnectionTypeAtStart(Wire::ConnectionType::ToWire);
			m_EditableWire->GetDataAtStart() = Wire::DataToWire{ wirePtr };
		}
		else
		{
			// Wire start is not connected to anything
			m_EditableWire->SetDrawStartDot(false);
			m_EditableWire->SetConnectionTypeAtStart(Wire::ConnectionType::ToNothing);
			m_EditableWire->GetDataAtStart() = Wire::DataToNothing{};
		}
		

		// TODO: make something to build m_WireGroups

		m_EditableWire = nullptr;
	}


	if (ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_Backspace, false))
	{
		if (segments->size() != 0)
		{
			segments->pop_back();
		}
	}
	
}




void Wire::Draw()
{
	for (size_t i = 0; i < m_Segments.size(); i++)
	{
		auto& segment = m_Segments[i];
		DrawThickLine(segment.vStart, segment.vEnd, m_WireThickness, m_WireColor);
		
		if (i == 0 && m_DrawStartDot)
			DrawCircle(segment.vStart, m_WireThickness * 1.5f, sf::Color::Black);

		if (i == m_Segments.size() - 1 && m_DrawEndDot)
			DrawCircle(segment.vEnd, m_WireThickness * 1.5f, sf::Color::Black);
	}
}


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




void DrawableCircuit::Draw()
{
	for (auto& wire : m_AllWires)
	{
		wire->Draw();
	}

	for (auto& drawable : m_DrawableElements)
	{
		drawable->Draw();
	}
}


void DrawableCircuit::RemoveWire(Wire* wire)
{
	auto it = std::ranges::find_if(m_AllWires, [wire](auto& w)
		{
			return w.get() == wire;
		});

	if (it != m_AllWires.end())
		m_AllWires.erase(it);
}


void DrawableCircuit::RemoveElement(eDrawableBase* element)
{
	auto it = std::ranges::find_if(m_DrawableElements, [element](const UPtrDrawableBaseTy& e) { return e.get() == element; });

	if (it != m_DrawableElements.end())
		m_DrawableElements.erase(it);

	m_Circuit->RemoveElement(m_DrawableToElement.at(element));
}


void DrawableCircuit::BuildWiresGroups()
{
	m_WireGroups.clear();

}



Resistor::Resistor()
	: eDrawableBase("assets\\resistor.png")
{
	GetTexture().setSmooth(true);
	m_EpinPositions =
	{
		{ 0.0f, 184.0f}, 
		{ float(m_sprite.getTextureRect().size.x), 184.0f }
	};
}


void Resistor::Draw()
{
	dlDrawList::getWindow()->draw(m_sprite);
}


sf::Vector2f Resistor::GetEpinPos(int n)
{
	return n == 0 ? sf::Vector2f(0.0f, 193.0f) : sf::Vector2f(m_sprite.getTextureRect().size.x, 193.0f);
}


void Resistor::UIParams(eElement* elem)
{
	eResistor* resistor = static_cast<eResistor*>(elem);
	float resistance = resistor->GetResistance();
	if (ImGui::InputFloat("Resistance", &resistance))
	{
		resistor->SetResistance(resistance);
	}
	sf::Vector2f pos = m_sprite.getPosition();
	ImGui::DragFloat2("Position", &pos.x, 1.0f);
	m_sprite.setPosition(pos);
}


int Resistor::GetPinIndexFromLocalPosition(sf::Vector2f pos)
{
	return pos == m_EpinPositions[0] ? 0 : 1;
}


// TODO...
////  th1(Ui + Render)   th2(Simulation)
////   .                  |
////   .			      |
////   . 	              |
////   .  send buffer0    |
////   | <----------------|
////   |	              |
////   | ---------------->| Setup sim time on this iteration
////   .                  |    + swap data buffers
////   | draw frame       |        
////   |                  |
////   |                  |
////   . wait             |
////   .                  |
////   .                  |
//
//
//class Simulation
//{
//	void StartSim();
//	void StopSim();
//	void Reset();
//	void Update();
//
//	void RunCircuit();
//	void SolveCircuit();
//};
//
