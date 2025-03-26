#include "circuit.h"

void Circuit::Reset()
{
	m_Elements.clear();
	m_UnusedElements.clear();

	m_Nodes.clear();
	m_UnusedNodes.clear();

	m_Matrix.Reset();
	m_GroundNode = nullptr;
}

eNode* Circuit::CreateNode()
{
	size_t index = m_Nodes.size();
	m_Nodes.emplace_back(std::make_unique<eNode>(index));

	int n = m_Nodes.size() - 1;
	if (n > 0)
		m_Matrix.Resize(n); // n-1 (Without the ref gnd)

	return m_Nodes.back().get();
}

eVoltageSource* Circuit::AddVoltageSource(double voltage)
{
	return AddElement<eVoltageSource>(voltage);
}

eResistor* Circuit::AddResistor(double resistance)
{
	return AddElement<eResistor>(resistance);
}

void Circuit::RemoveElement(eElement* element)
{
	auto it = std::ranges::find_if(m_Elements, [element](const UPtrElementTy& e)
		{
			return e.get() == element;
		});

	if (it != m_Elements.end())
		m_Elements.erase(it);			// Pins will be released from connected nodes in destructor
}

void Circuit::Connect(ePin* pin, eNode* node)
{
	if (pin && node)
		pin->ConnectToNode(node);
}

void Circuit::AssembleMatrix()
{
	m_GroundNode = LookupGroundNode();

	for (auto& element : m_Elements)
	{
		element->Stamp(m_Matrix, m_GroundNode, 0.0f);
	}
}

eNode* Circuit::LookupGroundNode()
{
	if (m_Nodes.empty())
		return nullptr;

	size_t MaxCountConnectedPins = 0;			// Ground node is often the one with the largest number of connected elements
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

		if (MaxCountConnectedPins >= 4)
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
		double newVoltage = node->GetVoltage() + offset;
		node->SetVoltage(newVoltage);
	}
}

size_t Circuit::GetNodeMtxIndex(eNode* node)
{
	size_t nodeIdx = node->GetIndex();
	size_t gndIdx = m_GroundNode->GetIndex();

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

	eVoltageSource* v1 = AddVoltageSource(2.5);
	eVoltageSource* v2 = AddVoltageSource(7.0);
	eResistor* r1 = AddResistor(1.0);
	eResistor* r2 = AddResistor(2.0);
	eResistor* r3 = AddResistor(3.0);
	eResistor* r4 = AddResistor(4.0);
	eResistor* r5 = AddResistor(5.0);

	eResistor* _r6 = AddResistor(5.0);
	eResistor* _r7 = AddResistor(5.0);

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
	AssembleMatrix();
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

Circuit::ResultsType Circuit::Simulate(double totalTime, double dt)
{
	ResultsType results;
	if (!m_GroundNode)
		m_GroundNode = LookupGroundNode();

	size_t steps = static_cast<size_t>(totalTime / dt);
	std::ofstream file("results.txt");
	file << "Steps: " << steps << '\n';
	file << "Total Time: " << totalTime << '\n';
	file << "Dt: " << dt << '\n';


	for (size_t i = 0; i < steps; ++i)
	{
		file << std::format("Step: {}  Time: {:.7f}\n", i, m_CurrTime);

		m_Matrix.Clear();

		for (auto& element : m_Elements)
		{
			element->Stamp(m_Matrix, m_GroundNode, dt);
		}
		file << "Matrix A:\n" << m_Matrix.GetMatrix() << "\n";
		file << "Vector b:\n" << m_Matrix.GetVector() << "\n";

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

			//if (eDiode* diode = dynamic_cast<eDiode*>(element.get()))
			//{
			//	file << "Diode " << diode->GetName() << " Vd: " << diode->GetVoltDrop() << " G: " << diode->GetG() << " Ieq: " << diode->GetIeq() << '\n';
			//}
		}

		m_CurrTime += dt;
		for (auto& node : m_Nodes)
		{
			auto idx = node->GetIndex();
			results[idx].emplace_back(m_CurrTime, node->GetVoltage());

			file << "Node " << idx << " : " << node->GetVoltage() << '\n';
		}
		file << "-----------------------------------\n";

		static bool bSetSlider = false;
		if (m_CurrTime > 0.25 && !bSetSlider)
		{
			bSetSlider = true;
			if (m_Elements.size() > 4 && m_Elements[4]->GetType() == ty_Potentiometer)
				static_cast<ePotentiometer*>(m_Elements[4].get())->SetSliderPosition(0.1);
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

	ePotentiometer* p = AddElement<ePotentiometer>(*this, 10, 1.0f);
	eVoltageSource* v1 = AddVoltageSource(10.0f);

	eResistor* r1 = AddResistor(5.0);
	eResistor* r2 = AddResistor(5.0);

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
	AssembleMatrix();
	Solve();

	std::cout << "R1 Current : " << r1->GetCurrent() << std::endl;
	std::cout << "R2 Current : " << r2->GetCurrent() << std::endl;

	for (auto& node : m_Nodes)
	{
		std::cout << "Node " << node->GetIndex() << " : " << node->GetVoltage() << std::endl;
	}

	m_Matrix.Print(std::cout);
}

void Circuit::Test2()
{
	// (v1)----(n1)---R1-------------------GND
	//          |
	//          |-----R2----(n2)-----R3----GND

	eNode* n1 = CreateNode();
	eNode* n2 = CreateNode();

	eNode* gnd_node = CreateNode();

	eVoltageSource* v1 = AddVoltageSource(5.0);
	eResistor* r1 = AddResistor(0.5f);
	eResistor* r2 = AddResistor(20.0);
	eResistor* r3 = AddResistor(3.0);

	eResistor* r4 = AddResistor(0.5);
	eResistor* r5 = AddResistor(0.6);
	eResistor* r6 = AddResistor(0.4);

	v1->GetPositivePin()->ConnectToNode(n1);
	v1->GetNegativePin()->ConnectToNode(gnd_node);

	r1->GetEpin(0)->ConnectToNode(n1);
	r1->GetEpin(1)->ConnectToNode(gnd_node);

	r2->GetEpin(0)->ConnectToNode(n1);
	r2->GetEpin(1)->ConnectToNode(n2);

	r3->GetEpin(0)->ConnectToNode(n2);
	r3->GetEpin(1)->ConnectToNode(gnd_node);

	FinalizeMatrixSize();
	AssembleMatrix();
	Solve();

	std::cout << "R1 Current : " << r1->GetCurrent() << std::endl;
	std::cout << "R2 Current : " << r2->GetCurrent() << std::endl;
	std::cout << "R3 Current : " << r3->GetCurrent() << std::endl;
	std::cout << "R4 Current : " << r4->GetCurrent() << std::endl;
	std::cout << "R5 Current : " << r5->GetCurrent() << std::endl;

	for (auto& node : m_Nodes)
	{
		std::cout << "Node " << node->GetIndex() << " : " << node->GetVoltage() << std::endl;
	}

	m_Matrix.Print(std::cout);
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

	double dt = 0.001;
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

	double dt = 0.001;
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