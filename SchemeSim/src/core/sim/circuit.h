#pragma once
#include <iostream>
#include <vector>
#include <map>
#include <set>
//#define  EIGEN_DONT_PARALLELIZE

#include <ranges>

#include <fstream>

#include "components.h"


class Circuit
{
	using UPtrNodeTy = std::unique_ptr<eNode>;
	using UPtrElementTy = std::unique_ptr<eElement>;


	std::vector<UPtrNodeTy>					m_Nodes;
	std::vector<UPtrNodeTy>					m_UnusedNodes;

	std::vector<UPtrElementTy>				m_Elements;
	std::vector<UPtrElementTy>				m_UnusedElements;

	CircuitMtx								m_Matrix;
	eNode*									m_GroundNode = nullptr;

	double									m_CurrTime = 0.0;

public:

	Circuit() = default;
	
	template <typename T, typename... Args> 
	T* AddElement(Args&&... args)
	{
		m_Elements.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
		return static_cast<T*>(m_Elements.back().get());
	}

	void									Reset									();
	eNode*									CreateNode								();
	eVoltageSource*							AddVoltageSource						(double voltage);
	eResistor*								AddResistor								(double resistance);
	void									RemoveElement							(eElement* element);
	void									Connect									(ePin* pin, eNode* node);
	void									AssembleMatrix							();
	eNode*									LookupGroundNode						();
	void									AdjustVoltages							(eNode* ToDesiredGround);
	size_t									GetNodeMtxIndex							(eNode* node);
	void									Solve									();
	void									FinalizeMatrixSize						();



	using NodeIndexTy = int;
	using TimeTy = double;
	using VoltageTy = double;

	using ResultsType = std::map<NodeIndexTy, std::vector<std::pair<TimeTy, VoltageTy>>>;

	ResultsType								Simulate(double totalTime, double dt);
	
	void									Test1();
	void									Test2();
	void									Test3();
	ResultsType								Test4(double totalTime = 0.25);
	ResultsType								Test5(double totalTime = 0.25);
	ResultsType								Test6(double totalTime = 0.5);


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
