#pragma once
#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <ranges>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <deque>
#include <queue>

#include "components.h"
#include "imgui.h"
#include "frontend/Widgets/WidgetsBase.h"
#include "common/vfmt.h"
#include "core/DrawList.h"
#include "helpers/Helpers.h"
#pragma warning(disable: 2397)


class Circuit
{
protected:

	using UPtrNodeTy = std::unique_ptr<eNode>;
	using UPtrElementTy = std::unique_ptr<eElement>;


	std::vector<UPtrNodeTy>					m_Nodes;
	std::vector<UPtrElementTy>				m_Elements;

	CircuitMtx								m_Matrix;
	eNode*									m_GroundNode = nullptr;

	double									m_CurrTime = 0.0;


	u64										GetNodeMtxIndex							(eNode* node);

public:

	Circuit() = default;
	
	template <typename T, typename... Args> 
	T* AddElement(Args&&... args)
	{
		m_Elements.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
		return static_cast<T*>(m_Elements.back().get());
	}

	void									Reset();
	void									ResetElements();	
	eNode*									CreateNode();
	void									RemoveNode(eNode* node);
	void									RemoveElement(eElement* element);
	void									Connect(ePin* pin, eNode* node);
	void									StampElements();
	void									RebuildMatrix();
	eNode*									LookupGroundNode();
	void									AdjustVoltages(eNode* ToDesiredGround);
	void									Solve();
	void									FinalizeMatrixSize();
	void									CleanupFromNodes();
	eNode*									MergeNodes(eNode* node1, eNode* node2);
	void									SetGroundNode(eNode* node)							{ m_GroundNode = node; }
	void									UpdateElements(double dt);
	using NodeIndexTy = int;
	using TimeTy = double;
	using VoltageTy = double;

	using ResultsType = std::map<NodeIndexTy, std::vector<std::pair<TimeTy, VoltageTy>>>;

	ResultsType								Simulate(double totalTime, double dt);
	
	void									Test1();
	ResultsType								Test3(double totalTime= 0.25);
	ResultsType								Test4(double totalTime = 0.25);
	ResultsType								Test5(double totalTime = 0.25);
	ResultsType								Test6(double totalTime = 0.5);
	ResultsType								Test7(double totalTime = 0.5);
	ResultsType								Test8(double totalTime = 0.5);


	std::vector<UPtrElementTy>& 			GetElements()							{ return m_Elements; }
	CircuitMtx& 							GetMatrix()								{ return m_Matrix; }
};
