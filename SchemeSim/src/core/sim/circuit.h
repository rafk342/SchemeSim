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
class WiresConnectionDot;

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
public:
	static inline float WireThickness = 24.0f;


	struct Segment
	{
		sf::Vector2f vStart;
		sf::Vector2f vEnd;

		Segment(sf::Vector2f start, sf::Vector2f end) : vStart(start), vEnd(end) { }
	};
	
	enum ConnectionPointType { Start, End };
	enum ConnectionType { ToWireDot, ToPin, ToNothing };
	
	struct DataToPin
	{
		std::weak_ptr<eDrawableBase> element;
		int pinIndex;
	};
	struct DataToWireDot
	{
		std::weak_ptr<WiresConnectionDot> dot;
	};
	struct DataToNothing
	{ };
	using VariantDataTy = std::variant<DataToPin, DataToWireDot, DataToNothing>;

private:

	u64							m_Id = -1;
	//eNode*					m_Node = nullptr;
	std::deque<Segment>			m_Segments;
	sf::Color					m_WireColor = sf::Color::Black;
	//bool						m_DrawStartDot = false;
	//bool						m_DrawEndDot = false;
	ConnectionType				m_ConnectionTypeAtStart = ToNothing;
	ConnectionType				m_ConnectionTypeAtEnd = ToNothing;
	VariantDataTy				m_DataAtStart = DataToNothing{};
	VariantDataTy				m_DataAtEnd = DataToNothing{};
	std::weak_ptr<WiresConnectionDot> m_ConnectionDotAtStart;
	std::weak_ptr<WiresConnectionDot> m_ConnectionDotAtEnd;
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
	
	//void							SetNode(eNode* node) { m_Node = node; }
	//eNode*							GetNode() const { return m_Node; }
	void							Draw();
	void							Clear()												{ m_Segments.clear(); }
	std::deque<Segment>*			GetSegments()										{ return &m_Segments; }
	//void							SetDrawStartDot(bool draw)							{ m_DrawStartDot = draw; }
	//void							SetDrawEndDot(bool draw)							{ m_DrawEndDot = draw; }
	void							SetColor(sf::Color color)							{ m_WireColor = color; }
	sf::Color						GetColor() const									{ return m_WireColor; }
	ConnectionType					GetConnectionTypeAtStart()							{ return m_ConnectionTypeAtStart; }
	ConnectionType					GetConnectionTypeAtEnd()							{ return m_ConnectionTypeAtEnd; }
	void							SetConnectionTypeAtStart(ConnectionType type)		{ m_ConnectionTypeAtStart = type; }
	void							SetConnectionTypeAtEnd(ConnectionType type)			{ m_ConnectionTypeAtEnd = type; }
	VariantDataTy&					DataAtStart()										{ return m_DataAtStart; }
	VariantDataTy&					DataAtEnd()											{ return m_DataAtEnd; }
	bool							IsInGroup() const									{ return m_IsInGroup; }
	void							SetIsInGroup(bool val)								{ m_IsInGroup = val; }
	u64								GetID() const										{ return m_Id; }
	std::shared_ptr<Wire>			SplitSelf(u64 SegmentIndex, sf::Vector2f SplitPoint);
	
	template<ConnectionPointType PointType>
	void SetConnectionType(ConnectionType type) { if (PointType == Start) m_ConnectionTypeAtStart = type; else m_ConnectionTypeAtEnd = type; }
	
	template<ConnectionPointType PointType>
	ConnectionType GetConnectionType() { if (PointType == Start) return m_ConnectionTypeAtStart; else return m_ConnectionTypeAtEnd; }

	template<ConnectionPointType PointType>
	VariantDataTy& DataAt() { if (PointType == Start) return m_DataAtStart; else return m_DataAtEnd; }

	template<ConnectionPointType PointType>
	std::weak_ptr<WiresConnectionDot> GetConnectionDot() { if (PointType == Start) return m_ConnectionDotAtStart; else return m_ConnectionDotAtEnd; }
	
	template<ConnectionPointType PointType>
	void SetConnectionDot(std::weak_ptr<WiresConnectionDot> dot);

	template<ConnectionPointType PointType>
	void ReleaseConnectionDot();
};


class WiresConnectionDot
{
	std::list< std::pair<std::weak_ptr<Wire>, Wire::ConnectionPointType> > m_ConnectedWires;
	sf::Vector2f m_Position;
public:
	WiresConnectionDot(sf::Vector2f pos) : m_Position(pos) { }
	bool contains(std::weak_ptr<Wire> wire) const									{ return std::ranges::find_if(m_ConnectedWires, [wire](const auto& pair) { return pair.first.lock() == wire.lock(); }) != m_ConnectedWires.end(); }
	void AddWire(std::weak_ptr<Wire> wire, Wire::ConnectionPointType PointType)		{ if (!contains(wire)) m_ConnectedWires.push_back({ wire, PointType }); }
	void RemoveWire(std::weak_ptr<Wire> wire)										{ std::erase_if(m_ConnectedWires, [wire](const auto& pair) { return pair.first.lock() == wire.lock(); }); }
	void RemoveWire(Wire* wire)														{  std::erase_if(m_ConnectedWires, [wire](const auto& pair) { if (auto p = pair.first.lock()) { return p.get() == wire; } else return false; }); }
	auto begin()																	{ return m_ConnectedWires.begin(); }
	auto end()																		{ return m_ConnectedWires.end(); }
	auto GetWiresCount() const														{ return m_ConnectedWires.size(); }
	void CleanupExpiredWireRefs()													{ std::erase_if(m_ConnectedWires, [](const auto& pair) { return pair.first.expired(); }); }
	void SetPosition(sf::Vector2f pos)												{ m_Position = pos; }
	sf::Vector2f GetPosition() const												{ return m_Position; }
	std::list< std::pair<std::weak_ptr<Wire>, Wire::ConnectionPointType> >& GetConnectedWires() { return m_ConnectedWires; }
	
	void Draw();
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
		auto size() const								{ return wires.size(); }
	};

	std::deque<std::shared_ptr<Wire>>					m_AllWires;
	std::unordered_map<eNode*,WiresGroup>				m_WireGroups;
	std::deque<std::shared_ptr<WiresConnectionDot>>		m_WiresConnectionDots;

	std::deque<std::shared_ptr<eDrawableBase>>			m_DrawableElements;
	std::unordered_map<eDrawableBase*, eElement*>		m_DrawableToElement;

	eNode*			LookupNodeByWire(Wire* wire);
	void			SyncWithCircuit();

	std::pair<std::weak_ptr<Wire>, std::weak_ptr<Wire>>	SplitWire(std::shared_ptr<Wire> wire, u64 SegmentIndex, sf::Vector2f SplitPoint);

public:

	DrawableCircuit(Circuit& circuit) : m_Circuit(&circuit) { }
	
	void			Draw();
	Circuit*		GetCircuit() { return m_Circuit; }
	void			RemoveWire(Wire* wire);
	void			RemoveElement(eDrawableBase* element);

	std::shared_ptr<WiresConnectionDot>	CreateWiresConnectionDot(sf::Vector2f pos);

	template <typename SimTy, typename DrawTy>
	void AddElement(auto&&... args);
};




class CircuitEditor
{
	DrawableCircuit* m_DrawableCircuit = nullptr;
	Wire* m_EditableWire = nullptr;
	bool m_WireEditOnStart = false;

	struct ClosestWirePointInfo
	{
		sf::Vector2f position;
		size_t segmentIndex;
		float distance;
	};

	struct ClosestPinData
	{
		sf::Vector2f ClosestPinPos;
		int PinIndex;
		std::weak_ptr<eDrawableBase> DrawableElement;
	};

	std::optional<ClosestPinData>							GetClosestElementPinInfo(sf::Vector2f mousePos);
	sf::Vector2f											GetClosestPointOnSegment(sf::Vector2f point, sf::Vector2f segStart, sf::Vector2f segEnd);
	std::optional<ClosestWirePointInfo>						FindClosestPointToWire(Wire& wire, sf::Vector2f mousePos, float maxDistance = 20.f);
	std::optional<std::pair<std::weak_ptr<Wire>, size_t>>	CheckForConnectionWithOtherWire(const sf::Vector2f& MousePos, sf::Vector2f& OutEnd, std::shared_ptr<Wire> selfWire);
	std::optional<std::weak_ptr<WiresConnectionDot>>		GetClosestWireConnectionDot(sf::Vector2f& OutPos);
	void													SnapToGrid(sf::Vector2f& end, sf::Vector2f& start);
	void													InitWireData(std::shared_ptr<Wire> wire);
	void													CleanupWireDots();
	void													InitAllWiresData();
public:

	CircuitEditor(DrawableCircuit& circuit) : m_DrawableCircuit(&circuit) { }
	
	void DrawUI();
	void HandleWireEditing();
	
	template<Wire::ConnectionPointType myEndTy>
	void UpdateConnectionData(std::shared_ptr<Wire> wire, sf::Vector2f& point);
};

template void CircuitEditor::UpdateConnectionData<Wire::ConnectionPointType::Start>(std::shared_ptr<Wire>, sf::Vector2f&);
template void CircuitEditor::UpdateConnectionData<Wire::ConnectionPointType::End>(std::shared_ptr<Wire>, sf::Vector2f&);

inline void drawText(const auto& text, float x, float y, int charSize, sf::Color color = sf::Color::Black)
{
	sf::Text t(g_SFMLRenderer.GetFont(), text, charSize);
	t.setFillColor(color);
	t.setPosition({ x, y });
	t.setString(text);
	dlDrawList::getWindow()->draw(t);
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




template<typename SimTy, typename DrawTy>
inline void DrawableCircuit::AddElement(auto&&... args)
{
	eElement* elem = m_Circuit->AddElement<SimTy>(std::forward<decltype(args)>(args)...);
	std::shared_ptr<eDrawableBase> drawable = std::make_shared<DrawTy>();
	m_DrawableToElement[drawable.get()] = elem;
	m_DrawableElements.push_back(std::move(drawable));
}


template<Wire::ConnectionPointType PointType>
inline void Wire::SetConnectionDot(std::weak_ptr<WiresConnectionDot> dot)
{
	if (PointType == Start)
	{
		if (auto ptr = m_ConnectionDotAtStart.lock())
			ptr->RemoveWire(this);

		m_ConnectionDotAtStart = dot;
	}
	else
	{
		if (auto ptr = m_ConnectionDotAtEnd.lock())
			ptr->RemoveWire(this);

		m_ConnectionDotAtEnd = dot;
	}
}


template<Wire::ConnectionPointType PointType>
inline void Wire::ReleaseConnectionDot()
{
	if (PointType == Start)
	{
		if (auto dotPtr = m_ConnectionDotAtStart.lock())
			dotPtr->RemoveWire(this);

		m_ConnectionDotAtStart.reset();
	}
	else
	{
		if (auto dotPtr = m_ConnectionDotAtEnd.lock())
			dotPtr->RemoveWire(this);

		m_ConnectionDotAtEnd.reset();
	}
}