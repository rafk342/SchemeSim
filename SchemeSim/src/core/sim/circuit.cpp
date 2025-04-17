//#include "drawableCircuit.h"
#include "frontend/drawableCircuit.h"

void Circuit::Reset()
{
	m_Elements.clear();
	m_Nodes.clear();
	m_Matrix.Reset();

	m_GroundNode = nullptr;
}


void Circuit::ResetElements()
{
	for (auto& elem : m_Elements)
	{
		if (elem.get())
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

	auto it = std::ranges::find_if(m_Nodes, [node](const UPtrNodeTy& n) { return n.get() == node; });

	if (it != m_Nodes.end())
		m_Nodes.erase(it);

	RebuildMatrix();
}


void Circuit::RemoveElement(eElement* element)
{
	if (!element)
		return;

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


void Circuit::StampElements(double dt)
{
	if (!m_GroundNode)
		m_GroundNode = LookupGroundNode();

	for (auto& element : m_Elements)
	{
		element->Stamp(m_Matrix, m_GroundNode, dt);
	}
}


void Circuit::RebuildMatrix()
{
	m_Matrix.Reset();

	ResetElements();

	for (size_t i = 0; i < m_Nodes.size(); ++i)
		m_Nodes[i]->SetIndex(i);

	if (!m_GroundNode)
		m_GroundNode = LookupGroundNode();
	
	if (!m_Nodes.empty())
		m_Matrix.Resize(m_Nodes.size() - 1);

	FinalizeMatrixSize();
}


eNode* Circuit::LookupGroundNode()
{
	if (m_Nodes.empty())
		return nullptr;
	
	u64 MaxCountConnectedPins = 0;			// Ground node is often the one with the largest number of connected elements
	eNode* groundNode = nullptr;
	//{
	//	for (auto& node : m_Nodes)
	//	{
	//		if (node->GetNumConnectedPins() > MaxCountConnectedPins)
	//		{
	//			MaxCountConnectedPins = node->GetNumConnectedPins();
	//			groundNode = node.get();
	//		}
	//	}

	//	if (MaxCountConnectedPins >= 5) // 5 or more should be enough
	//		return groundNode;
	//}


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

	if (!groundNode)
	{
		std::cout << "Circuit::LookupGroundNode() -> Failed to find gnd node" << std::endl;
		groundNode = m_Nodes[0].get();	// just take the first one
	}

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
		if (element.get())
			element->InitMatrix(m_Matrix);
	}
}

void Circuit::CleanupFromNodes()
{
	for (size_t i = m_Nodes.size(); i > 0; --i)
	{
		RemoveNode(m_Nodes[i - 1].get());
	}
}

eNode* Circuit::MergeNodes(eNode* node1, eNode* node2)
{
	if (!node1 || !node2)
		return nullptr;

	for (auto& pin : node2->GetPins())
	{
		pin->ConnectToNode(node1);
	}
	RemoveNode(node2);
	RebuildMatrix();
	return node1;
}

void Circuit::UpdateElements(double dt)
{
	for (auto& elem : m_Elements) 
		elem->Update(m_Matrix, dt);
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

	double Time = 0.0f;

	for (u64 i = 0; i < steps; ++i)
	{
		file << std::format("Step: {}  Time: {:.7f}\n", i, Time);

		m_Matrix.Clear();

		for (auto& element : m_Elements)
		{
			element->Stamp(m_Matrix, m_GroundNode, dt);
		}

		Solve();
		UpdateElements(dt);

		m_Matrix.Print(file);
		Time += dt;
		for (auto& node : m_Nodes)
		{
			auto idx = node->GetIndex();
			results[idx].emplace_back(Time, node->GetVoltage());

			file << "Node " << idx << " : " << node->GetVoltage() << '\n';
		}
		file << "-----------------------------------\n";
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
	p->GetMiddlePin()->ConnectToNode(n2);
	p->GetRightPin()->ConnectToNode(gnd_node);

	r2->GetEpin(0)->ConnectToNode(n2);
	r2->GetEpin(1)->ConnectToNode(gnd_node);

	m_GroundNode = gnd_node;
	FinalizeMatrixSize();
	StampElements(0.0f);
	Solve();

	std::cout << "R1 Current : " << r1->GetCurrent() << std::endl;
	std::cout << "R2 Current : " << r2->GetCurrent() << std::endl;

	for (auto& node : m_Nodes)
	{
		std::cout << "Node " << node->GetIndex() << " : " << node->GetVoltage() << std::endl;
	}

	m_Matrix.Print(std::cout);
}

Circuit::ResultsType Circuit::Test2(double totalTime)
{
	//  n5--vs--n0---wire1-----n1--r1--n2-----wire2------n3--c1--n4-----wire3-----n5


	eVoltageSource* vs = AddElement<eVoltageSource>(7.0);
	eResistor* r1 = AddElement<eResistor>(5000);
	eCapacitor* c1 = AddElement<eCapacitor>(1.0);

	eResistor* wire1 = AddElement<eResistor>(1e-6);
	eResistor* wire2 = AddElement<eResistor>(1e-6);
	eResistor* wire3 = AddElement<eResistor>(1e-6);

	eNode* n0 = CreateNode();
	eNode* n1 = CreateNode();
	eNode* n2 = CreateNode();
	eNode* n3 = CreateNode();
	eNode* n4 = CreateNode();
	eNode* n5 = CreateNode();

	vs->GetPositivePin()->ConnectToNode(n0);

	wire1->GetEpin(0)->ConnectToNode(n0);
	wire1->GetEpin(1)->ConnectToNode(n1);
	r1->GetEpin(0)->ConnectToNode(n1);
	r1->GetEpin(1)->ConnectToNode(n2);
	wire2->GetEpin(0)->ConnectToNode(n2);
	wire2->GetEpin(1)->ConnectToNode(n3);
	c1->GetEpin(0)->ConnectToNode(n3);
	c1->GetEpin(1)->ConnectToNode(n4);
	wire3->GetEpin(0)->ConnectToNode(n4);
	wire3->GetEpin(1)->ConnectToNode(n5);

	vs->GetNegativePin()->ConnectToNode(n5);

	double dt = 0.0005;
	FinalizeMatrixSize();
	ResultsType results = Simulate(totalTime, dt);

	return results;
}


Circuit::ResultsType Circuit::Test3(double totalTime)
{
	//  -------(n1)----(R3)---(n2)-----|
	//  |       |              |       |
	// (R1)     |              |      (R4)
	//  |       |             -|       |
	// (n0)    (R2)           (V2)    (n4)
	//  |+      |             +|       |
	// (V1)     |              |      (R5)
	//  |-      |              |       |
	//  |       |              |       |
	//  |------(n3)---------------------


	eVoltageSource* v1 = AddElement<eVoltageSource>(2.5);
	eVoltageSource* v2 = AddElement<eVoltageSource>(7.0);
	eResistor* r1 = AddElement<eResistor>(1.0);
	eResistor* r2 = AddElement<eResistor>(2.0);
	eResistor* r3 = AddElement<eResistor>(3.0);
	eResistor* r4 = AddElement<eResistor>(4.0);
	eResistor* r5 = AddElement<eResistor>(5.0);

	eNode* n0 = CreateNode();
	eNode* n1 = CreateNode();
	eNode* n2 = CreateNode();
	eNode* n3 = CreateNode();
	eNode* n4 = CreateNode();


	v1->GetPositivePin()->ConnectToNode(n0);
	v1->GetNegativePin()->ConnectToNode(n3);

	r1->GetEpin(0)->ConnectToNode(n0);
	r1->GetEpin(1)->ConnectToNode(n1);

	r2->GetEpin(1)->ConnectToNode(n1);
	r2->GetEpin(0)->ConnectToNode(n3);

	r3->GetEpin(0)->ConnectToNode(n1);
	r3->GetEpin(1)->ConnectToNode(n2);

	r4->GetEpin(0)->ConnectToNode(n2);
	r4->GetEpin(1)->ConnectToNode(n4);

	v2->GetPositivePin()->ConnectToNode(n3);
	v2->GetNegativePin()->ConnectToNode(n2);

	r4->GetEpin(0)->ConnectToNode(n2);
	r4->GetEpin(1)->ConnectToNode(n4);

	r5->GetEpin(0)->ConnectToNode(n4);
	r5->GetEpin(1)->ConnectToNode(n3);

	double dt = 0.005;
	FinalizeMatrixSize();
	return Simulate(totalTime, dt);
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
	////  |-(vs)---n1---(r0)---n2---(btn)---|
	////  |                                 |
	////  n0-------------(l1)---------------n3

	//eNode* n0 = CreateNode();
	//eNode* n1 = CreateNode();
	//eNode* n2 = CreateNode();
	//eNode* n3 = CreateNode();

	//eVoltageSource* vs = AddElement<eVoltageSource>(5);
	//eResistor* r0 = AddElement<eResistor>(100);
	//eInductor* l1 = AddElement<eInductor>(0.2);
	//eButton* button = AddElement<eButton>(eButton::NormalOpen);
	//button->Press();

	//vs->GetNegativePin()->ConnectToNode(n0);
	//vs->GetPositivePin()->ConnectToNode(n1);

	//r0->GetEpin(0)->ConnectToNode(n1);
	//r0->GetEpin(1)->ConnectToNode(n2);

	//button->GetEpin(0)->ConnectToNode(n2);
	//button->GetEpin(1)->ConnectToNode(n3);

	//l1->GetEpin(0)->ConnectToNode(n0);
	//l1->GetEpin(1)->ConnectToNode(n3);

	//m_GroundNode = n0;

	//FinalizeMatrixSize();
	//double dt = 0.00005;
	//return Simulate(totalTime, dt);
	return ResultsType();
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


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void Simulation::Simulate(double frameTime, const double step)
{
	if (!sm_Circuit)
		return;

	if (sm_SimState == SIM_STOPPED)
	{
		sm_Circuit->RebuildMatrix();
		sm_CircTime = 0.0;
		sm_RealTime = 0.0;
		sm_SimState = SIM_PAUSED;
		return;
	}

	if (sm_SimState == SIM_PAUSED)
		return;
	
	if (sm_SimState == SIM_ON_START)
	{
		sm_Circuit->RebuildMatrix();
		sm_CircTime = 0.0;
		sm_RealTime = 0.0;
		sm_SimState = SIM_RUNNING;
	}

	if (sm_SimState != SIM_RUNNING)
		return;

	u64 NumSteps = u64(frameTime * sm_SimSpeed / step);
	sm_RealTime += frameTime;
	if (NumSteps <= 0)
		NumSteps = 1;

	double startTime = sm_CircTime;
	sm_CircTime += NumSteps * step;

	if (sm_Circuit->GetElements().size() == 0)
		return;

	if (sm_Circuit->GetNodes().size() == 0)
		return;

#define LOG_DATA 1

#if LOG_DATA
	static std::ofstream file("sim_results.txt");
	file << std::format("Sim Time: {:.7f}  Real Time: {:.7f}\n", sm_CircTime, sm_RealTime);
	file << std::format("Num Steps: {}\n", NumSteps);
	file << std::format("Step: {}\n", step);
	file << "-----------------------------------\n";
#endif

	for (size_t i = 0; i < NumSteps; i++)
	{
		double tickTime = startTime + step * i;

		sm_Circuit->GetMatrix().Clear();
		sm_Circuit->StampElements(step);
		sm_Circuit->Solve();
		sm_Circuit->UpdateElements(step);
		UpdateOscilloscopes(tickTime);

#if LOG_DATA
		file << std::format("Step: {}\n", i);
		sm_Circuit->GetMatrix().Print(file);

		for (auto& node : sm_Circuit->GetNodes())
		{
			auto idx = node->GetIndex();
			file << std::format("Node {} : {:.7f}\n", idx, node->GetVoltage());
		}
#endif
	}

}

template<typename T, size_t size>
using stack_unordered_set = std::unordered_set<T, std::hash<T>, std::equal_to<T>, hmcgr::StackFirstFitAllocator<T, size>>;


void Simulation::UpdateOscilloscopes(double t)
{
	stack_unordered_set<eElement*, 300> existingElements;
	for (auto& elem : sm_Circuit->GetElements())
	{
		existingElements.insert(elem.get());
		if (auto osc = GetOscilloscope(elem.get()))
		{
			osc->AddCurrentData(t, elem->GetCurrent());
			osc->AddVoltData(t, elem->GetVoltDrop());
		}
	}

	for (auto it = sm_ElemToOscilloscope.begin(); it != sm_ElemToOscilloscope.end(); )
	{
		if (!existingElements.contains(it->first))
			it = sm_ElemToOscilloscope.erase(it);
		else
			++it;
	}
}


void Simulation::RegisterOscilloscope(eElement* element, std::shared_ptr<Oscilloscope> oscilloscope)
{
	sm_ElemToOscilloscope[element] = oscilloscope;
}

std::shared_ptr<Oscilloscope> Simulation::GetOscilloscope(eElement* element)
{
	auto it = sm_ElemToOscilloscope.find(element);
	if (it != sm_ElemToOscilloscope.end())
	{
		if (auto osc = it->second.lock())
			return osc;
		else
			sm_ElemToOscilloscope.erase(it);
	}
	return nullptr;
}