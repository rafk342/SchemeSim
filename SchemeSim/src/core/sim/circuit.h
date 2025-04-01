#pragma once
#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <ranges>
#include <fstream>
#include <unordered_map>

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
	std::vector<sf::Vector2f> m_EpinPositions;

public:

	eDrawableBase(const std::string& path) : WidgetBase(path) 
	{ }

	virtual void					Draw							() = 0;
	virtual sf::Vector2f			GetEpinPos(int n) = 0; // int can be enum, should be the same as in derived eElement class
	virtual void					UIParams						(eElement* elem) = 0;
	std::vector<sf::Vector2f>&		GetLocalPinsPositions			() { return m_EpinPositions; }
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
		eDrawableBase* element;
		int pinIndex;
	};
	struct DataToWire
	{
		std::weak_ptr<Wire> wire;
	};
	struct DataToNothing
	{ };
private:
	
	using VariantDataTy = std::variant<DataToPin, DataToWire, DataToNothing>;

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

public:
	Wire() = default;

	void						Draw();

	void						Clear()												{ m_Segments.clear(); }
	std::vector<Segment>*		GetSegments()										{ return &m_Segments; }
	void						SetDrawStartDot(bool draw)							{ m_DrawStartDot = draw; }
	void						SetDrawEndDot(bool draw)							{ m_DrawEndDot = draw; }
	void						SetColor(sf::Color color)							{ m_WireColor = color; }
	void						SetConnectionTypeAtStart(ConnectionType type)		{ m_ConnectionTypeAtStart = type; }
	void						SetConnectionTypeAtEnd(ConnectionType type)			{ m_ConnectionTypeAtEnd = type; }
	ConnectionType				GetConnectionTypeAtStart()							{ return m_ConnectionTypeAtStart; }
	ConnectionType				GetConnectionTypeAtEnd()							{ return m_ConnectionTypeAtEnd; }
	VariantDataTy&				GetDataAtStart()									{ return m_DataAtStart; }
	VariantDataTy&				GetDataAtEnd()										{ return m_DataAtEnd; }
};



class DrawableCircuit
{
	friend class CircuitEditor;
	using UPtrDrawableBaseTy = std::unique_ptr<eDrawableBase>;


	Circuit* m_Circuit = nullptr;

	struct WiresGroup
	{
		eNode* node = nullptr;						// Node that simulates this group
		std::vector<std::weak_ptr<Wire>> wires;			// Wires in this group
	};

	std::vector<WiresGroup>					m_WireGroups;
	std::deque<std::shared_ptr<Wire>>		m_AllWires;
	std::deque<UPtrDrawableBaseTy>			m_DrawableElements;

	std::unordered_map<eDrawableBase*, eElement*> m_DrawableToElement;



public:

	DrawableCircuit(Circuit& circuit) : m_Circuit(&circuit) { }
	
	Circuit*		GetCircuit() { return m_Circuit; }
	void			Draw();
	void			RemoveWire(Wire* wire);
	void			RemoveElement(eDrawableBase* element);
	void			BuildWiresGroups();

	template <typename SimTy, typename DrawTy>
	void AddElement(auto&&... args) 
	{
		eElement* elem = m_Circuit->AddElement<SimTy>(std::forward<decltype(args)>(args)...);
		UPtrDrawableBaseTy drawable = std::make_unique<DrawTy>();
		m_DrawableToElement[drawable.get()] = elem;
		m_DrawableElements.push_back(std::move(drawable));
	}
};


class CircuitEditor
{
	DrawableCircuit* m_DrawableCircuit = nullptr;
	Wire* m_EditableWire = nullptr;
	bool m_WireEditOnStart = false;

	struct ClosestPointInfo
	{
		sf::Vector2f position;
		float distance;
		bool valid = false;
	};

	struct myOptInfo
	{
		sf::Vector2f ClosestPinPos;
		int PinIndex;
		eDrawableBase* DrawableElement;
	};

	std::optional<myOptInfo> GetClosestElementPinPos(sf::Vector2f mousePos);

	sf::Vector2f						 GetClosestPointOnSegment(sf::Vector2f point, sf::Vector2f segStart, sf::Vector2f segEnd);
	ClosestPointInfo					 FindClosestPointToWire(Wire& wire, sf::Vector2f mousePos, float maxDistance = 20.f);
	std::pair<bool, std::weak_ptr<Wire>> CheckForConnectionWithWire(const sf::Vector2f& MousePos, sf::Vector2f& end);
	void								 SnapToGrid(sf::Vector2f& end, sf::Vector2f& start);

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

