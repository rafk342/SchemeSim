#pragma once
#include <iostream>
#include <vector>
#include "core/sim/circuit.h"
#include <string>
#include <scn/scan.h>
#include "helpers/Helpers.h"
#include "implot.h"
#include "drawableComponents.h"
#include "parser.h"
#include "drawableCircuit.h"


class eDrawableBase;
class DrawableCircuit;
class CircuitEditor;
class Wire;
class ConnectionDot;
class CircuitParser;


class ConnectionDot
{
	using PinIndex = int;

	std::list<std::pair<std::weak_ptr<eDrawableBase>, PinIndex>> m_ConnectedElements;
	sf::Vector2f m_Position;
	bool m_ToElemPin = false;

	void			UpdateVisibility();
	void			UpdateConnectedElementsPositions();
public:

	ConnectionDot(sf::Vector2f pos);

	bool IsHoveredInUi = false;

	void			Draw();
	void			Connect(std::weak_ptr<eDrawableBase> element, int pinIndex);
	void			Release(std::weak_ptr<eDrawableBase> element);
	bool			contains(std::weak_ptr<eDrawableBase> element);
	void			SetPosition(sf::Vector2f pos);
	sf::Vector2f	GetPosition() const						{ return m_Position; }
	void			CleanupFromExpiredElements();
	u64				GetNumConnectedElements() const			{ return m_ConnectedElements.size(); }
	auto&			GetConnectedElements()					{ return m_ConnectedElements; }
	bool			HasConnectionWithAnyElem() const			{ return m_ToElemPin; }
};


class Wire : public eDrawableBase
{
public:

	static inline float			WireThickness = 24.0f;
	static inline sf::Color		PositiveVoltColor = sf::Color(0, 194, 93, 255);
	static inline sf::Color		NegativeVoltColor = sf::Color(210, 42, 42, 255);
	static inline sf::Color		CurrentDotsColor = sf::Color::Yellow;
	static inline float			CurrentSpeedScalar = 10.0f;

	struct Segment
	{
		sf::Vector2f vStart;
		sf::Vector2f vEnd;
		double curcount = 0.0f;

		Segment(sf::Vector2f start, sf::Vector2f end) 
			: vStart(start), vEnd(end) 
		{ }

		double length() const { return (vStart - vEnd).length(); }
	};

	enum PinPoint { Start, End };

private:

	//u64									m_Id = -1;
	std::deque<Segment>					m_Segments;
	std::weak_ptr<ConnectionDot>		m_StartDot;
	std::weak_ptr<ConnectionDot>		m_EndDot;
	sf::Color							m_StartColor = sf::Color::Black;
	sf::Color							m_EndColor = sf::Color::Black;
	double								m_curcount = 0.0f;


	void DrawThickLine(sf::Vector2f start, sf::Vector2f end, float thickness, sf::Color color);
	void DrawThickLineWithGradient(sf::Vector2f start, sf::Vector2f end, float thickness, sf::Color color1, sf::Color color2);

public:

	using SimTy = eResistor;

	bool IsHoveredInUi = false;

	Wire();
	virtual void					Draw() override;
	virtual sf::Vector2f			GetGlobalPinPosition(int n) override;
	virtual sf::Vector2f			GetLocalPinPosition(int n) override;
	virtual void					UIParams(eElement* elem) override {}
	virtual int						GetPinIndexFromLocalPosition(sf::Vector2f pos) override;
	virtual bool 					IsWire() const override { return true; }
	virtual DrawableType			GetType() const override { return DrawableType::DRAWABLE_UNKNOWN; }
	virtual bool					CanHaveOscilloscope() const override { return false; }

	auto&							GetSegments() { return m_Segments; }
	std::shared_ptr<eDrawableBase>	SplitSelf(DrawableCircuit& circuit, u64 SegmentIndex, sf::Vector2f SplitPoint);
	void							UpdateColor(DrawableCircuit& circuit);
	void							DrawCurrentDots(eElement* elem);

	template<PinPoint StartOrEnd> void ConnectToDotAt(std::shared_ptr<ConnectionDot> dot);
	template<PinPoint StartOrEnd> void DisconnectFromDotAt();
};

template void Wire::ConnectToDotAt<Wire::Start>(std::shared_ptr<ConnectionDot>);
template void Wire::ConnectToDotAt<Wire::End>(std::shared_ptr<ConnectionDot>);
template void Wire::DisconnectFromDotAt<Wire::Start>();
template void Wire::DisconnectFromDotAt<Wire::End>();



struct ScrollingBuffer
{
	int MaxSize;
	int Offset;
	ImVector<ImVec2> Data;
	float lastSampleX;
	float sampleInterval;

	ScrollingBuffer(int max_size = 20'000, float display_interval = 1.0f)
	{
		lastSampleX = 0.0f;
		MaxSize = max_size;
		Offset = 0;
		sampleInterval = display_interval / max_size;
		Data.reserve(MaxSize);
	}

	void AddPoint(float x, float y)
	{
		if (x < lastSampleX + sampleInterval)
			return;

		lastSampleX = x;
		if (Data.size() < MaxSize)
			Data.push_back(ImVec2(x, y));
		else
		{
			Data[Offset] = ImVec2(x, y);
			Offset = (Offset + 1) % MaxSize;
		}
	}

	void Erase()
	{
		if (Data.size() > 0)
		{
			Data.shrink(0);
			Offset = 0;
			lastSampleX = 0.0f;
		}
	}
};


class Oscilloscope
{
	using OwnerListTy = std::list<std::shared_ptr<Oscilloscope>>;
	OwnerListTy* m_OwnerList = nullptr;
	OwnerListTy::iterator m_selfIt;

	std::weak_ptr<eDrawableBase> m_Drawable;
	ScrollingBuffer m_VoltData;
	ScrollingBuffer m_CurrentData;
	bool m_ShowVoltage;
	bool m_ShowCurrent;

public:

	Oscilloscope(std::shared_ptr<eDrawableBase> drawable);
	~Oscilloscope();
	void Init(OwnerListTy* owner, OwnerListTy::iterator it);
	void DrawPlot();
	void AddVoltData(float time, float voltage);
	void AddCurrentData(float time, float current);
	void Reset();
	OwnerListTy::iterator GetSelfIt() { return m_selfIt; }
};


class DrawableCircuit
{
	friend class CircuitEditor;
	friend class CircuitParser;

	Circuit* m_Circuit = nullptr;
																  
	std::deque<std::shared_ptr<eDrawableBase>>					  m_DrawableElements;
	std::unordered_map<eDrawableBase*, eElement*>				  m_DrawableToElement;
	std::unordered_map<std::shared_ptr<ConnectionDot>, eNode*>	  m_Connections;

	std::shared_ptr<ConnectionDot> SplitWire(std::shared_ptr<eDrawableBase> wire, u64 SegmentIndex, sf::Vector2f SplitPoint);
	void SyncWithCircuit();

public:

	void Destroy();

	DrawableCircuit(Circuit& circuit) 
		: m_Circuit(&circuit) 
	{ }
	
	void			Draw(float frameTime);
	Circuit*		GetCircuit() { return m_Circuit; }
	void			RemoveElement(eDrawableBase* element);
	void			RemoveElement(u64 index);
	void			CleanupConnections();
	std::shared_ptr<ConnectionDot> CreateConnectionDot(sf::Vector2f pos);
	void			UpdateWireColors();

	auto AddResistor()		{ return AddElement<Resistor>(5.0); }
	auto AddBattery()		{ return AddElement<Battery>(5.0); }
	auto AddWire()			{ return AddElement<Wire>(1e-2); }
	auto AddCapacitor()		{ return AddElement<Capacitor>(0.00001); }
	auto AddInductor()		{ return AddElement<Inductor>(0.5); }
	auto AddDiode()			{ return AddElement<Diode>(); }
	auto AddButton()		{ return AddElement<RelayContactsGroup>(*m_Circuit); }
	auto AddTransformer()	{ return AddElement<Transformer>(0.5, 0.3); }
	auto AddDiodeBridge()	{ return AddElement<DiodeBridge>(*m_Circuit); }
	
	auto AddNeutralRelayCoil()					{ return AddElement<NeutralRelayCoil>(0.2, 0.0); }
	auto AddNeutralRelayCoil3Class()			{ return AddElement<NeutralRelayCoil3RelyabilityClass>(0.2, 0.0); }
	auto AddNeutralRelayCoilWithDelay()			{ return AddElement<NeutralRelayCoilWithSwitchOffDelay>(0.2, 0.5); }
	auto AddNeutralRelayCoilWithDelay3Class()	{ return AddElement<NeutralRelayCoilWithSwitchOffDelay3RelyabilityClass>(0.2, 0.5); }
	auto AddNeutralRelayCoilWithDiode()			{ return AddElement<NeutralRelayCoilWithRectifier>(*m_Circuit, 0.2, 0.0); }

	template <typename DrawTy>
	auto AddElement(auto&&... args)
	{
		eElement* elem = m_Circuit->AddElement<typename DrawTy::SimTy>(std::forward<decltype(args)>(args)...);
		std::shared_ptr<eDrawableBase> drawable = std::make_shared<DrawTy>();
		m_DrawableToElement[drawable.get()] = elem;
		m_DrawableElements.push_back(drawable);
		SyncWithCircuit();
		return drawable;
	}

	eElement* GetAssociatedElectricElement(eDrawableBase* drawable);
	eNode* GetElecticNode(std::shared_ptr<ConnectionDot> dot);
	auto& GetDrawableElements() { return m_DrawableElements; }
};




class CircuitEditor
{
	friend class CircuitParser;

	DrawableCircuit*	m_DrawableCircuit	= nullptr;
	Wire*				m_EditableWire		= nullptr;
	bool				m_WireEditOnStart	= false;
	eDrawableBase*		m_DraggableElement	= nullptr;
	CircuitParser		m_Parser;

	std::list<std::shared_ptr<Oscilloscope>> m_Oscilloscopes;
	std::unordered_map<eDrawableBase*, std::weak_ptr<Oscilloscope>> m_DrawableToOscilloscope;

	struct ClosestWirePointInfo
	{
		sf::Vector2f position;
		u64 segmentIndex;
		float distance;
	};
	struct ClosestPinData
	{
		sf::Vector2f PinPos;
		int PinIndex;
		std::weak_ptr<eDrawableBase> DrawableElement;
	};

	sf::Vector2f SnapToGrid(sf::Vector2f pos, float GridScale = 50.0f);
	
	std::optional<std::shared_ptr<ConnectionDot>>	GetClosestConnectionDot(sf::Vector2f& OutPos);
	std::optional<ClosestPinData>					GetClosestElementPinInfo(sf::Vector2f pos);
	std::optional<ClosestWirePointInfo>				GetClosestPointOnWire(Wire& wire, sf::Vector2f mousePos, float maxDistance = 20.f);
	std::optional<std::pair<std::shared_ptr<eDrawableBase>,u64>> SearchForNearestWire(const sf::Vector2f& MousePos, sf::Vector2f& OutEnd, std::shared_ptr<eDrawableBase> selfWire);
	
	void HandleDraggableElem(sf::Vector2f mousePos, sf::Vector2f DragOffset);
	void HandleWireEditing();
	void UpdateAllWireConnections();
	void UpdateConnectionDots();

	template<Wire::PinPoint StartOrEnd>
	void UpdateConnectionData(u64 WireIndex, sf::Vector2f& point);

	void AddOscilloscope(std::shared_ptr<eDrawableBase> drawable);
	void ResetOscilloscopes();
	void RemoveOscilloscope(std::shared_ptr<eDrawableBase> drawable);

public:

	CircuitEditor(DrawableCircuit& circuit);
	void DrawUI();
};

template void CircuitEditor::UpdateConnectionData<Wire::Start>(u64, sf::Vector2f&);
template void CircuitEditor::UpdateConnectionData<Wire::End>(u64, sf::Vector2f&);










