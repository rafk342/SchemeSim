#pragma once
#include <iostream>
#include <vector>
#include "circuit.h"
#include <string>


inline void drawText(const auto& text, float x, float y, int charSize, sf::Color color = sf::Color::Black)
{
	sf::Text t(gSFMLRenderer.GetFont(), text, charSize);
	t.setFillColor(color);
	t.setPosition({ x, y });
	t.setString(text);
	dlDrawList::getWindow()->draw(t);
};


inline void DrawCircle(sf::Vector2f position, float radius, sf::Color color = sf::Color::Black)
{
	sf::CircleShape circle(radius);
	circle.setFillColor(color);
	circle.setOrigin({ radius, radius });
	circle.setPosition(position);
	dlDrawList::getWindow()->draw(circle);
}


inline sf::Color lerpColor(sf::Color color1, sf::Color color2, float t)
{
	return sf::Color( std::lerp(color1.r, color2.r, t),
					  std::lerp(color1.g, color2.g, t),
					  std::lerp(color1.b, color2.b, t),
					  std::lerp(color1.a, color2.a, t) );
}


class eDrawableBase;
class DrawableCircuit;
class CircuitEditor;
class Wire;
class ConnectionDot;


class ConnectionDot
{
	using PinIndex = int;

	std::list<std::pair<std::weak_ptr<eDrawableBase>, PinIndex>> m_ConnectedElements;
	sf::Vector2f m_Position;
	bool m_ToElemPin = false;

	void UpdateVisibility();
public:
	ConnectionDot(sf::Vector2f pos);

	bool IsHoveredInUi = false;

	void			Draw();
	void			Connect(std::weak_ptr<eDrawableBase> element, int pinIndex);
	void			Release(std::weak_ptr<eDrawableBase> element);
	bool			contains(std::weak_ptr<eDrawableBase> element) const;
	void			SetPosition(sf::Vector2f pos)			{ m_Position = pos; }
	sf::Vector2f	GetPosition() const						{ return m_Position; }
	void			UpdateConnectedElementsPositions();
	void			CleanupFromExpiredElements();
	u64				GetNumConnectedElements() const			{ return m_ConnectedElements.size(); }
	auto&			GetConnectedElements()					{ return m_ConnectedElements; }
	bool			HasConnectionWithElem() const			{ return m_ToElemPin; }
};



enum DrawableType
{
	ResistorType,
	BatteryType,
	Unknown = -1,
};

class eDrawableBase : public WidgetBase, public std::enable_shared_from_this<eDrawableBase>
{
protected:

	std::vector<sf::Vector2f> m_PinPositions;
	sf::Angle m_Rotation{};

public:

	eDrawableBase(const std::string& path) 
		: WidgetBase(path) 
		, m_Rotation(sf::degrees(0))
	{ }

	virtual void					Draw() = 0;
	// int can be enum, should be the same as in derived eElement class
	virtual sf::Vector2f			GetLocalPinPosition(int n)						{ return m_PinPositions[n]; } 
	virtual sf::Vector2f			GetGlobalPinPosition(int n)						{ return GetPosition() + GetLocalPinPosition(n); }
	virtual void					UIParams(eElement* elem) = 0;
	virtual int						GetPinIndexFromLocalPosition(sf::Vector2f pos) = 0;
	virtual void					SetRotation(float degrees);
	virtual u64 					GetNumPins() const								{ return m_PinPositions.size(); }
	sf::Angle						GetRotation() const								{ return m_Rotation; }
	//bool							IsWire() { return dynamic_cast<Wire*>(this) != nullptr; }
	virtual bool					IsWire() const { return false; }
	virtual DrawableType 			GetType() const = 0;

	template <typename T> T* As() { return /*dynamic_cast*/static_cast<T*>(this); }
};



class Resistor : public eDrawableBase
{
public:
	Resistor();
	virtual void					Draw() override;
	virtual sf::Vector2f			GetLocalPinPosition(int n) override;
	virtual void					UIParams(eElement* elem) override;
	virtual int						GetPinIndexFromLocalPosition(sf::Vector2f pos) override;
	virtual DrawableType			GetType() const override { return DrawableType::ResistorType; }
};


class Battery : public eDrawableBase
{
public:
	Battery();
	virtual void					Draw() override;
	virtual sf::Vector2f			GetLocalPinPosition(int n) override;
	virtual void					UIParams(eElement* elem) override;
	virtual int						GetPinIndexFromLocalPosition(sf::Vector2f pos) override;
	virtual DrawableType			GetType() const override { return DrawableType::BatteryType; }
};




class Wire : public eDrawableBase
{
public:

	static inline float WireThickness = 24.0f;
	
	struct Segment
	{
		sf::Vector2f vStart;
		sf::Vector2f vEnd;

		Segment(sf::Vector2f start, sf::Vector2f end) 
			: vStart(start), vEnd(end) 
		{ }

		double length() const
		{
			return (vStart - vEnd).length();
		}
	};

	enum PinPoint { Start, End };

private:
	
	//u64									m_Id = -1;
	std::deque<Segment>					m_Segments;
	//sf::Color							m_WireColor = sf::Color::Black;
	std::weak_ptr<ConnectionDot>		m_StartDot;
	std::weak_ptr<ConnectionDot>		m_EndDot;
	sf::Color							m_StartColor = sf::Color::Black;
	sf::Color							m_EndColor = sf::Color::Black;

	void DrawThickLine(sf::Vector2f start, sf::Vector2f end, float thickness, sf::Color color);
	void DrawThickLineWithGradient(sf::Vector2f start, sf::Vector2f end, float thickness, sf::Color color1, sf::Color color2);
public:

	//bool IsHoveredInUi = false

	Wire();
	virtual void					Draw() override;
	virtual sf::Vector2f			GetGlobalPinPosition(int n) override;
	virtual sf::Vector2f			GetLocalPinPosition(int n) override;
	virtual void					UIParams(eElement* elem) override {}
	virtual int						GetPinIndexFromLocalPosition(sf::Vector2f pos) override;
	virtual bool 					IsWire() const override { return true; }
	virtual DrawableType			GetType() const override { return DrawableType::Unknown; }
	auto&							GetSegments() { return m_Segments; }
	std::shared_ptr<eDrawableBase>	SplitSelf(DrawableCircuit& circuit, u64 SegmentIndex, sf::Vector2f SplitPoint);
	void							UpdateColor(DrawableCircuit& circuit);

	template<PinPoint StartOrEnd> void ConnectToDotAt(std::shared_ptr<ConnectionDot> dot);
	template<PinPoint StartOrEnd> void DisconnectFromDotAt();
};

template void Wire::ConnectToDotAt<Wire::Start>(std::shared_ptr<ConnectionDot>);
template void Wire::ConnectToDotAt<Wire::End>(std::shared_ptr<ConnectionDot>);
template void Wire::DisconnectFromDotAt<Wire::Start>();
template void Wire::DisconnectFromDotAt<Wire::End>();



class DrawableCircuit
{
	friend class CircuitEditor;
	friend class CircuitParser;

	Circuit* m_Circuit = nullptr;
																  
	std::deque<std::shared_ptr<eDrawableBase>>					  m_DrawableElements;
	std::unordered_map<eDrawableBase*, eElement*>				  m_DrawableToElement;
	std::unordered_map<std::shared_ptr<ConnectionDot>, eNode*>	  m_Connections;

	std::optional<std::shared_ptr<ConnectionDot>> SplitWire(std::shared_ptr<eDrawableBase> wire, u64 SegmentIndex, sf::Vector2f SplitPoint);
	void SyncWithCircuit();

public:

	DrawableCircuit(Circuit& circuit) 
		: m_Circuit(&circuit) 
	{ }
	
	void			Draw();
	Circuit*		GetCircuit() { return m_Circuit; }
	void			RemoveElement(eDrawableBase* element);
	void			RemoveElement(u64 index);
	void			CleanupConnections();
	std::shared_ptr<ConnectionDot> CreateConnectionDot(sf::Vector2f pos);
	void			UpdateWireColors();

	auto AddResistor()	{ return AddElement<eResistor, Resistor>(5.0); }
	auto AddBattery()	{ return AddElement<eVoltageSource, Battery>(5.0); }
	auto AddWire()		{ return AddElement<eResistor, Wire>(0.000001); }

	template <typename SimTy, typename DrawTy>
	auto AddElement(auto&&... args)
	{
		eElement* elem = m_Circuit->AddElement<SimTy>(std::forward<decltype(args)>(args)...);
		std::shared_ptr<eDrawableBase> drawable = std::make_shared<DrawTy>();
		m_DrawableToElement[drawable.get()] = elem;
		m_DrawableElements.push_back(drawable);
		return drawable;
	}

	eElement* GetElecticElement(eDrawableBase* drawable)
	{
		auto it = m_DrawableToElement.find(drawable);
		if (it != m_DrawableToElement.end())
			return it->second;
		return nullptr;
	}

	eNode* GetElecticNode(std::shared_ptr<ConnectionDot> dot)
	{
		auto it = m_Connections.find(dot);
		if (it != m_Connections.end())
			return it->second;
		return nullptr;
	}
};



class CircuitParser
{
	CircuitEditor* m_editor = nullptr;

	__forceinline void WriteVec2f(std::ostream& os, const sf::Vector2f& v)		{ os << *(u32*)(&v.x) << ' ' << *(u32*)(&v.y) << '\n'; }
	__forceinline void WriteFloat(std::ostream& os, float f)					{ os << *(u32*)(&f) << '\n'; }
	__forceinline void WriteInt(std::ostream& os, int n)						{ os << n << '\n'; }

	__forceinline sf::Vector2f ReadVec2f(const std::string& line)
	{
		u32 n[2]{};
		sscanf_s(line.c_str(), "%u %u", &n[0], &n[1]);
		return { *(float*)(&n[0]) , *(float*)(&n[1]) };
	}

	__forceinline float ReadFloat(const std::string& line) {
		u32 n = 0;
		sscanf_s(line.c_str(), "%u", &n);
		return *(float*)(&n);
	}

	__forceinline int ReadInt(const std::string& line) {
		u32 n = 0;
		sscanf_s(line.c_str(), "%u", &n);
		return *(int*)(&n);
	}

public:
	CircuitParser(CircuitEditor* editor) : m_editor(editor) { }
	void LoadFromFile(const std::filesystem::path& path);
	void SaveToFile(const std::filesystem::path& path);
};


class CircuitEditor
{
	friend class CircuitParser;

	DrawableCircuit*	m_DrawableCircuit	= nullptr;
	Wire*				m_EditableWire		= nullptr;
	bool				m_WireEditOnStart	= false;
	eDrawableBase*		m_DraggableElement	= nullptr;
	CircuitParser		m_Parser;

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
	std::optional< std::pair<std::shared_ptr<eDrawableBase>,u64> >	SearchForConnectionWithOtherWire(const sf::Vector2f& MousePos, sf::Vector2f& OutEnd, std::shared_ptr<eDrawableBase> selfWire);


	void UpdateConnectionDots();

	template<Wire::PinPoint StartOrEnd>
	void UpdateConnectionData(u64 WireIndex, sf::Vector2f& point);

public:

	CircuitEditor(DrawableCircuit& circuit);
	void DrawUI();
	void HandleDraggableElem(sf::Vector2f mousePos, sf::Vector2f DragOffset);
	void HandleWireEditing();
	void UpdateAllWireConnections();
};

template void CircuitEditor::UpdateConnectionData<Wire::Start>(u64, sf::Vector2f&);
template void CircuitEditor::UpdateConnectionData<Wire::End>(u64, sf::Vector2f&);
