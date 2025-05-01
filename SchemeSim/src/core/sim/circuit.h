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
#include <thread>

#include "components.h"
#include "imgui.h"
#include "frontend/Widgets/WidgetsBase.h"
#include "common/vfmt.h"
#include "core/DrawList.h"
#include "helpers/Helpers.h"


class Circuit
{
protected:

	using UPtrNodeTy = std::unique_ptr<eNode>;
	using UPtrElementTy = std::unique_ptr<eElement>;


	std::vector<UPtrNodeTy>					m_Nodes;
	std::vector<UPtrElementTy>				m_Elements;

	CircuitMtx								m_Matrix;
	eNode*									m_GroundNode = nullptr;

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
	void									StampElements(double dt);
	void									RebuildMatrix();
	eNode*									LookupGroundNode();
	void									AdjustVoltages(eNode* ToDesiredGround);
	void									Solve(bool decompose = true);
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
	ResultsType								Test2(double totalTime= 0.25);
	ResultsType								Test3(double totalTime= 0.25);
	ResultsType								Test4(double totalTime = 0.25);
	ResultsType								Test5(double totalTime = 0.25);
	ResultsType								Test6(double totalTime = 0.5);
	ResultsType								Test7(double totalTime = 0.5);
	ResultsType								Test8(double totalTime = 0.5);


	std::vector<UPtrElementTy>& 			GetElements()							{ return m_Elements; }
	auto& 									GetNodes()								{ return m_Nodes; }
	CircuitMtx& 							GetMatrix()								{ return m_Matrix; }
};

class DrawableCircuit;
class Oscilloscope;



enum SimState
{
	SIM_STOPPED,
	SIM_RUNNING,
	SIM_ON_START,
	SIM_PAUSED,
};

class Simulation
{
	static inline Circuit*		sm_Circuit = nullptr;
	static inline double		sm_CircTime = 0.0;
	static inline double		sm_RealTime = 0.0;
	static inline SimState 		sm_SimState = SIM_PAUSED;
	static inline float			sm_SimSpeed = 0.3f;

	static inline std::unordered_map<eElement*, std::weak_ptr<Oscilloscope>> sm_ElemToOscilloscope;

public:

	static void		Init(Circuit* circuit)										{ sm_Circuit = circuit; }
	static void		Shutdown()													{ }
	static void		Simulate(double frameTime, const double step = 0.0005);
	static void		UpdateOscilloscopes(double t);
	static SimState	GetState() 													{ return sm_SimState; }
	static void		SetState(SimState state)									{ sm_SimState = state; }
	static double&	CircTime() 													{ return sm_CircTime; }
	static float&	SimSpeed() 													{ return sm_SimSpeed; }
	
	static void		RegisterOscilloscope(eElement* element, std::shared_ptr<Oscilloscope> oscilloscope);
	static std::shared_ptr<Oscilloscope> GetOscilloscope(eElement* element);
};
