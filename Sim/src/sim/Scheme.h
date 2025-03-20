#pragma once
#include <iostream>
#include <vector>
#include <set>
#include "vendor/Eigen/Dense"



class ePin;
class eNode;
class eElement;
class CircuitMtx;


class eNode
{
	std::set<ePin*> m_ePins;
	
	double m_TotalCurr = 0.0;
	double m_Voltage = 0.0;
	
	size_t m_NodeIndex;
public:

	eNode(size_t index)
		: m_NodeIndex(index) 
	{ }

	void AddEpin(ePin* epin);
	void RemoveEpin(ePin* epin);
	void clear();

	double GetVoltage() const { return m_Voltage; }
	void   SetVoltage(double Volt) { m_Voltage = Volt; }
	
	size_t GetIndex() const { return m_NodeIndex; }
};


class ePin
{
	eNode* m_Enode = nullptr;     // Node connected to this pin
	eElement* m_Element = nullptr; // Element connected to this pin // Element should be always be here

public:
	ePin() = default;
	ePin(eElement* parent);
	~ePin();

	bool		HasParentElement();
	void		SetParentElement(eElement* element);
	eElement*	GetParentElement();

	bool		IsConnectedToNode();
	void		ConnectToNode(eNode* enode);
	eNode*		GetConnectedNode();
	void		ReleaseNode();

	double GetVoltage() const { return m_Enode ? m_Enode->GetVoltage() : 0.0; }

};


class eElement
{
protected:
	std::vector<ePin> m_ePins;
	double m_step;

	virtual void SetNumEpins(int n);
	void SetEpin(int num, ePin pin);

public:

	virtual ~eElement();

	virtual void Initialize() { }
	virtual void Stamp(CircuitMtx& mtx, eNode* GndNode) { }

	virtual ePin* GetNextPin(ePin* pin);
	ePin* GetEpin(int num);
	void ReleaseConnectedNodes();

	void SetStep(double step);
	double GetStep();
	
private:

};


class eResistor : public eElement
{
	double m_Resistance;
	double m_Current = 0.0;

public:

	eResistor(double resistance);
	virtual void Stamp(CircuitMtx& mtx, eNode* GndNode) override;

	double GetCurrent()
	{
		if (m_ePins[0].IsConnectedToNode() && m_ePins[1].IsConnectedToNode())
		{
			double v1 = m_ePins[0].GetVoltage();
			double v2 = m_ePins[1].GetVoltage();
			m_Current = (v1 - v2) / m_Resistance;
			return m_Current;
		}

		return 0.0;
	}

	double GetVoltageLoss()
	{
		if (m_ePins[0].IsConnectedToNode() && m_ePins[1].IsConnectedToNode())
		{
			double v1 = m_ePins[0].GetVoltage();
			double v2 = m_ePins[1].GetVoltage();
			return v1 - v2;
		}
		return 0.0;
	}

	double GetResistance() { return m_Resistance; }

	void SetResistance(double resistance) { m_Resistance = resistance; }
};


class eVoltageSource : public eElement
{
	double m_Voltage;

public:

	eVoltageSource(double voltage);
	virtual void Stamp(CircuitMtx& mtx, eNode* GndNode) override;
	ePin* GetPositivePin() { return &m_ePins[0]; }
	ePin* GetNegativePin() { return &m_ePins[1]; }
};



class CircuitMtx
{
	u64 m_NumNodes = 0;

	Eigen::MatrixXd A; // Circuit matrix
	Eigen::VectorXd x; // Solution
	Eigen::VectorXd b; // Ñurrents

public:
	
	CircuitMtx()
	{
		Reset();
	}

	// Ax = b
	void Solve() 
	{
		x = A.colPivHouseholderQr().solve(b);
	}

	double GetVoltage(int node) {
		return x(node);
	}

	Eigen::MatrixXd& GetMatrix() { return A; }
	Eigen::VectorXd& GetVector() { return b; }
	Eigen::VectorXd& GetSolution() { return x; }

	void Clear()
	{
		A = Eigen::MatrixXd::Zero(m_NumNodes, m_NumNodes);
		x = Eigen::VectorXd::Zero(m_NumNodes);
		b = Eigen::VectorXd::Zero(m_NumNodes);
	}
	
	void Reset()
	{
		m_NumNodes = 0;
		A = Eigen::MatrixXd::Zero(m_NumNodes, m_NumNodes);
		x = Eigen::VectorXd::Zero(m_NumNodes);
		b = Eigen::VectorXd::Zero(m_NumNodes);
	}

	void Resize(u64 numTotal)
	{
		if (numTotal == m_NumNodes)
			return;

		Eigen::MatrixXd newA = Eigen::MatrixXd::Zero(numTotal, numTotal);
		Eigen::VectorXd newX = Eigen::VectorXd::Zero(numTotal);
		Eigen::VectorXd newB = Eigen::VectorXd::Zero(numTotal);

		int minSize = std::min(m_NumNodes, numTotal);
		newA.block(0, 0, minSize, minSize) = A.block(0, 0, minSize, minSize);
		newX.head(minSize) = x.head(minSize);
		newB.head(minSize) = b.head(minSize);

		A = std::move(newA);
		x = std::move(newX);
		b = std::move(newB);

		m_NumNodes = numTotal;
	}

	u64 GetNumNodes() { return m_NumNodes; }

	void PrintMatrix()
	{
		for (int i = 0; i < A.rows(); i++)
		{
			std::string row = "";
			for (int j = 0; j < A.cols(); j++)
			{
				row += std::format("{:.3f}\t", (A(i, j)));
			}

			row += " | " + std::format("{:.3f}", x(i));
			row += " | " + std::format("{:.3f}",b(i));
			std::cout << row << std::endl;
		}
	}
};



class Circuit
{
	using UniquePtrNodeTy = std::unique_ptr<eNode>;
	using UniquePtrElementTy = std::unique_ptr<eElement>;

	
	std::vector<UniquePtrNodeTy> m_Nodes;
	std::vector<UniquePtrNodeTy> m_UnusedNodes;
	
	std::vector<UniquePtrElementTy> m_Elements;
	std::vector<UniquePtrElementTy> m_UnusedElements;

	CircuitMtx m_Matrix;
	eNode* m_GroundNode = nullptr;

public:

	Circuit() = default;

	void Reset()
	{
		m_Elements.clear();
		m_Nodes.clear();
		m_Matrix.Reset();
		m_GroundNode = nullptr;
	}

	eNode* CreateNode()
	{
		size_t index = m_Nodes.size();
		m_Nodes.push_back(std::make_unique<eNode>(index));
		if (!m_GroundNode)
			m_GroundNode = m_Nodes.back().get(); // Assume the first node is the ground

		m_Matrix.Resize(m_Nodes.size() - 1); // Resize without the ground node
		return m_Nodes.back().get();
	}

	template <typename T, typename... Args>
	T* AddElement(Args&&... args)
	{
		m_Elements.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
		return static_cast<T*>(m_Elements.back().get());
	}

	eVoltageSource* AddVoltageSource(double voltage)
	{
		return AddElement<eVoltageSource>(voltage);
	}

	eResistor* AddResistor(double resistance)
	{
		return AddElement<eResistor>(resistance);
	}

	void CreateNodeBetween(eElement* element1, eElement* element2, int pinElement_1, int pinElement_2)
	{
		ePin* pin1 = element1->GetEpin(pinElement_1);
		ePin* pin2 = element2->GetEpin(pinElement_2);
		if (pin1 && pin2)
		{
			eNode* node = CreateNode();
			Connect(pin1, node);
			Connect(pin2, node);
		}
	}

	void RemoveElement(eElement* element)
	{
		auto it = std::ranges::find_if(m_Elements, [element](const auto& e)
			{ 
				return e.get() == element; 
			});

		if (it != m_Elements.end())
			m_Elements.erase(it); // Pins will be released from connected nodes in destruct
	}

	void Connect(ePin* pin, eNode* node)
	{
		if (pin && node)
			pin->ConnectToNode(node);
	}

	void AssembleMatrix()
	{
		m_Matrix.Clear();
		for (auto& element : m_Elements)
		{
			element->Stamp(m_Matrix, m_GroundNode);
		}
	}

	eNode* LookupGroundNode()
	{
		if (m_Nodes.empty())
			return nullptr;

		for (auto& element : m_Elements)
		{
			if (auto* voltageSource = dynamic_cast<eVoltageSource*>(element.get()))
			{
				eNode* negativeNode = voltageSource->GetNegativePin()->GetConnectedNode();
				if (negativeNode)
					return negativeNode;
			}
		}

		// AC case
		eNode* minVoltNode = m_Nodes[0].get();
		double minVoltage = std::abs(minVoltNode->GetVoltage());

		for (auto& node : m_Nodes)
		{
			double nodeVoltage = std::abs(node->GetVoltage());
			if (nodeVoltage < minVoltage)
			{
				minVoltage = nodeVoltage;
				minVoltNode = node.get();
			}
		}

		return minVoltNode;
	}

	void AdjustVoltages(eNode* ToDesiredGround)
	{
		if (!ToDesiredGround)
			return;

		double offset = ToDesiredGround->GetVoltage();
		offset = -offset;

		for (auto& node : m_Nodes)
		{
			double newVoltage = node->GetVoltage() + offset;
			node->SetVoltage(newVoltage);
		}
	}
	
	size_t GetNodeIndex(eNode* node)
	{
		return node->GetIndex() > m_GroundNode->GetIndex() ? node->GetIndex() - 1 : node->GetIndex();
	}

	void Solve()
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
				size_t idx = (node->GetIndex() > m_GroundNode->GetIndex()) ? node->GetIndex() - 1 : node->GetIndex();
				node->SetVoltage(m_Matrix.GetVoltage(idx));
			}
		}
	}

	void Test1()
	{
		// (v1) ---(n1)---R1---(n2)---R2----GND

		eNode* n1 = CreateNode();
		eNode* n2 = CreateNode();
		eNode* gnd_node = CreateNode();

		eVoltageSource* v1 = AddVoltageSource(5.0);
		eResistor* r1 = AddResistor(1.0);
		eResistor* r2 = AddResistor(2.0);

		v1->GetPositivePin()->ConnectToNode(n1);
		v1->GetNegativePin()->ConnectToNode(gnd_node);

		r1->GetEpin(0)->ConnectToNode(n1);
		r1->GetEpin(1)->ConnectToNode(n2);

		r2->GetEpin(0)->ConnectToNode(n2);
		r2->GetEpin(1)->ConnectToNode(gnd_node);

		AssembleMatrix();
		Solve();
		AdjustVoltages(LookupGroundNode());

		std::cout << "R1 Current : " << r1->GetCurrent() << std::endl;
		std::cout << "R2 Current : " << r2->GetCurrent() << std::endl;

		for (auto& node : m_Nodes)
		{
			std::cout << "Node " << node->GetIndex() << " : " << node->GetVoltage() << std::endl;
		}

		m_Matrix.PrintMatrix();
	}

	void Test2()
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

		v1->GetPositivePin()->ConnectToNode(n1);
		v1->GetNegativePin()->ConnectToNode(gnd_node);

		r1->GetEpin(0)->ConnectToNode(n1);
		r1->GetEpin(1)->ConnectToNode(gnd_node);

		r2->GetEpin(0)->ConnectToNode(n1);
		r2->GetEpin(1)->ConnectToNode(n2);

		r3->GetEpin(0)->ConnectToNode(n2);
		r3->GetEpin(1)->ConnectToNode(gnd_node);

		AssembleMatrix();
		Solve();
		AdjustVoltages(LookupGroundNode());
		std::cout << "R1 Current : " << r1->GetCurrent() << std::endl;
		std::cout << "R2 Current : " << r2->GetCurrent() << std::endl;
		std::cout << "R3 Current : " << r3->GetCurrent() << std::endl;

		for (auto& node : m_Nodes)
		{
			std::cout << "Node " << node->GetIndex() << " : " << node->GetVoltage() << std::endl;
		}

		m_Matrix.PrintMatrix();
	}
};


class Simulation
{
	void StartSim();
	void StopSim();
	void Reset();
	void Update();

	void RunCircuit();
	void SolveCircuit();
	void SolveMatrix();
};

