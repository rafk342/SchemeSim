#pragma once
#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <ranges>
#include <fstream>
#include <unordered_map>
#include <unordered_set>

#include "components.h"
#include "imgui.h"
#include "frontend/Widgets/WidgetsBase.h"
#include "common/vfmt.h"
#include "core/DrawList.h"



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

	void									Reset									();
	void									ResetElements							();	
	eNode*									CreateNode								();
	void									RemoveNode								(eNode* node);
	void									RemoveElement							(eElement* element);
	void									Connect									(ePin* pin, eNode* node);
	void									StampElements							();
	void									RebuildMatrix							();
	eNode*									LookupGroundNode						();
	void									AdjustVoltages							(eNode* ToDesiredGround);
	void									Solve									();
	void									FinalizeMatrixSize						();
	void									CleanupFromNodes						();
	eNode*									MergeNodes								(eNode* node1, eNode* node2);

	using NodeIndexTy = int;
	using TimeTy = double;
	using VoltageTy = double;

	using ResultsType = std::map<NodeIndexTy, std::vector<std::pair<TimeTy, VoltageTy>>>;

	ResultsType								Simulate(double totalTime, double dt);
	
	void									Test1();
	void									Test3();
	ResultsType								Test4(double totalTime = 0.25);
	ResultsType								Test5(double totalTime = 0.25);
	ResultsType								Test6(double totalTime = 0.5);
	ResultsType								Test7(double totalTime = 0.5);
	ResultsType								Test8(double totalTime = 0.5);


	std::vector<UPtrElementTy>& 			GetElements()							{ return m_Elements; }
};




class CircuitEditor;
class DrawableCircuit;
class Wire;


class eDrawableBase : public WidgetBase
{
protected:
	std::vector<sf::Vector2f> m_PinPositions;

public:

	eDrawableBase(const std::string& path) : WidgetBase(path) 
	{ }

	virtual void					Draw								() = 0;
	virtual sf::Vector2f			GetEpinPos							(int n) = 0; // int can be enum, should be the same as in derived eElement class
	virtual void					UIParams							(eElement* elem) = 0;
	std::vector<sf::Vector2f>&		GetLocalPinsPositions				()							{ return m_PinPositions; }
	virtual int						GetPinIndexFromLocalPosition		(sf::Vector2f pos) = 0;
};


class Resistor : public eDrawableBase
{
public:
	Resistor();
	virtual void					Draw() override;
	virtual sf::Vector2f			GetEpinPos(int n) override;
	virtual void					UIParams(eElement* elem) override;
	virtual int						GetPinIndexFromLocalPosition(sf::Vector2f pos) override;
};





class Wire
{
	static inline float m_WireThickness = 24.0f;

public:

	struct Segment
	{
		sf::Vector2f vStart;
		sf::Vector2f vEnd;

		Segment(sf::Vector2f start, sf::Vector2f end) : vStart(start), vEnd(end) {}
	};

	enum ConnectionType { ToWire, ToPin, ToNothing };
	struct DataToPin
	{
		std::weak_ptr<eDrawableBase> element;
		int pinIndex;
	};
	struct DataToWire
	{
		std::weak_ptr<Wire> wire;
	};
	struct DataToNothing
	{ };
	using VariantDataTy = std::variant<DataToPin, DataToWire, DataToNothing>;

private:

	u64							m_Id = -1;
	//eNode*						m_Node = nullptr;
	std::vector<Segment>		m_Segments;
	sf::Color					m_WireColor = sf::Color::Black;
	bool						m_DrawStartDot = false;
	bool						m_DrawEndDot = false;
	ConnectionType				m_ConnectionTypeAtStart = ToNothing;
	ConnectionType				m_ConnectionTypeAtEnd = ToNothing;
	VariantDataTy				m_DataAtStart;
	VariantDataTy				m_DataAtEnd;
	bool						m_IsInGroup = false;

	void DrawThickLine(sf::Vector2f start, sf::Vector2f end, float thickness, sf::Color color);

	static inline u64 counter = 0;
	static inline u64 NumInstances = 0;
public:

	bool IsHoveredInMainList = false;
	bool IsHoveredInGroupList = false;

	Wire()	
	{
		m_Id = counter++;
		NumInstances++;
	}
	~Wire()
	{
		NumInstances--;
		if (NumInstances == 0)
			counter = 0;
	}
	
	//void						SetNode(eNode* node) { m_Node = node; }
	//eNode*						GetNode() const { return m_Node; }
	void						Draw();
	void						Clear()												{ m_Segments.clear(); }
	std::vector<Segment>*		GetSegments()										{ return &m_Segments; }
	void						SetDrawStartDot(bool draw)							{ m_DrawStartDot = draw; }
	void						SetDrawEndDot(bool draw)							{ m_DrawEndDot = draw; }
	void						SetColor(sf::Color color)							{ m_WireColor = color; }
	sf::Color					GetColor() const									{ return m_WireColor; }
	ConnectionType				GetConnectionTypeAtStart()							{ return m_ConnectionTypeAtStart; }
	ConnectionType				GetConnectionTypeAtEnd()							{ return m_ConnectionTypeAtEnd; }
	void						SetConnectionTypeAtStart(ConnectionType type)		{ m_ConnectionTypeAtStart = type; }
	void						SetConnectionTypeAtEnd(ConnectionType type)			{ m_ConnectionTypeAtEnd = type; }
	VariantDataTy&				DataAtStart()										{ return m_DataAtStart; }
	VariantDataTy&				DataAtEnd()											{ return m_DataAtEnd; }
	bool						IsInGroup() const									{ return m_IsInGroup; }
	void						SetIsInGroup(bool val)								{ m_IsInGroup = val; }
	u64							GetID() const										{ return m_Id; }
};



class DrawableCircuit
{
	friend class CircuitEditor;

	Circuit* m_Circuit = nullptr;

	class WiresGroup
	{
		std::list<std::weak_ptr<Wire>> wires;			// Wires in this group
	public:

		bool contains(std::weak_ptr<Wire> wire) const	{ return std::ranges::find_if(wires, [wire](const auto& w) { return w.lock() == wire.lock(); }) != wires.end(); }
		void insert(std::weak_ptr<Wire> wire)			{ if (!contains(wire)) wires.push_back(wire); } 
		void CleanUpFromExiredWires()					{ std::erase_if(wires, [](const std::weak_ptr<Wire>& wire) { return wire.expired(); }); } 
		void clear()									{ wires.clear(); } 
		auto begin()									{ return wires.begin(); }
		auto end()										{ return wires.end(); }
		auto erase(auto it)								{ return wires.erase(it); }
	};

	std::vector<std::shared_ptr<Wire>>				m_AllWires;
	std::unordered_map<eNode*,WiresGroup>			m_WireGroups;
	std::vector<std::shared_ptr<eDrawableBase>>		m_DrawableElements;
	std::unordered_map<eDrawableBase*, eElement*>	m_DrawableToElement;

	eNode*			LookupNodeByWire(Wire* wire);
	void			SyncWithCircuit();

public:

	DrawableCircuit(Circuit& circuit) : m_Circuit(&circuit) { }
	
	void			Draw();
	Circuit*		GetCircuit() { return m_Circuit; }
	void			RemoveWire(Wire* wire);

	template <typename SimTy, typename DrawTy>
	void AddElement(auto&&... args) 
	{
		eElement* elem = m_Circuit->AddElement<SimTy>(std::forward<decltype(args)>(args)...);
		std::shared_ptr<eDrawableBase> drawable = std::make_shared<DrawTy>();
		m_DrawableToElement[drawable.get()] = elem;
		m_DrawableElements.push_back(std::move(drawable));
	}
	void			RemoveElement(eDrawableBase* element);
};


class CircuitEditor
{
	DrawableCircuit* m_DrawableCircuit = nullptr;
	Wire* m_EditableWire = nullptr;
	bool m_WireEditOnStart = false;

	struct ClosestWirePointInfo
	{
		sf::Vector2f position;
		float distance;
		bool valid = false;
	};

	struct ClosestPinData
	{
		sf::Vector2f ClosestPinPos;
		int PinIndex;
		std::weak_ptr<eDrawableBase> DrawableElement;
	};

	std::optional<ClosestPinData>			GetClosestElementPinInfo(sf::Vector2f mousePos);
	sf::Vector2f							GetClosestPointOnSegment(sf::Vector2f point, sf::Vector2f segStart, sf::Vector2f segEnd);
	ClosestWirePointInfo					FindClosestPointToWire(Wire& wire, sf::Vector2f mousePos, float maxDistance = 20.f);
	std::optional<std::weak_ptr<Wire>>		CheckForConnectionWithOtherWire(const sf::Vector2f& MousePos, sf::Vector2f& end, Wire* selfWire);
	void									SnapToGrid(sf::Vector2f& end, sf::Vector2f& start);
	void									InitWireData(Wire* wire);
	void									SplitWire(Wire* wire, u64 segmentIndex, sf::Vector2f point);

public:

	CircuitEditor(DrawableCircuit& circuit) : m_DrawableCircuit(&circuit) { }
	
	void DrawUI();
	void HandleWireEditing();
};




class CircuitRef : WidgetBase
{
public:
	CircuitRef()
		: WidgetBase("assets\\circuitRef.png")
	{
		GetTexture().setSmooth(true);
	}

	void Draw()
	{
		dlDrawList::getWindow()->draw(m_sprite);
	}
};

