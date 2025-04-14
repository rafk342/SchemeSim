#pragma once
#include <iostream>
#include <vector>
#include "circuit.h"
#include <string>
#include <scn/scan.h>
#include "helpers/Helpers.h"
#include "implot.h"


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

	void			UpdateVisibility();
	void			UpdateConnectedElementsPositions();
public:

	ConnectionDot(sf::Vector2f pos);

	bool IsHoveredInUi = false;

	void			Draw();
	void			Connect(std::weak_ptr<eDrawableBase> element, int pinIndex);
	void			Release(std::weak_ptr<eDrawableBase> element);
	bool			contains(std::weak_ptr<eDrawableBase> element) const;
	void			SetPosition(sf::Vector2f pos);
	sf::Vector2f	GetPosition() const						{ return m_Position; }
	void			CleanupFromExpiredElements();
	u64				GetNumConnectedElements() const			{ return m_ConnectedElements.size(); }
	auto&			GetConnectedElements()					{ return m_ConnectedElements; }
	bool			HasConnectionWithElem() const			{ return m_ToElemPin; }
};



enum DrawableType
{
	DrawableResistorType,
	DrawableBatteryType,
	DrawableCapacitorType,
	DrawableInductorType,
	DrawableDiodeType,
	DrawableButtonType,
	DrawableRelayContactType,
	DrawableRelayCoilType,
	
	DrawableUnknownType = -1,
};

enum class flipAxis { X, Y };

class eDrawableBase : public WidgetBase, public std::enable_shared_from_this<eDrawableBase>
{	
protected:

	std::vector<sf::Vector2f> m_PinPositions;
	sf::Angle m_Rotation{};
	bool m_IsFlipped_X = false;
	bool m_IsFlipped_Y = false;

public:

	eDrawableBase(const std::string& path) 
		: WidgetBase(path) 
		, m_Rotation(sf::degrees(0))
	{ }

	virtual void					Draw() = 0;
	// int can be enum, should be the same as in derived eElement class
	virtual sf::Vector2f			GetLocalPinPosition(int n)									{ return m_PinPositions[n]; } 
	virtual sf::Vector2f			GetGlobalPinPosition(int n)									{ return GetPosition() + GetLocalPinPosition(n); }
	virtual void					UIParams(eElement* elem) = 0;
	virtual int						GetPinIndexFromLocalPosition(sf::Vector2f pos) = 0;
	virtual void					SetRotation(float degrees);
	virtual u64 					GetNumPins() const											{ return m_PinPositions.size(); }
	sf::Angle						GetRotation() const											{ return m_Rotation; }
	virtual void					Flip(flipAxis axis);
	//bool							IsWire()													{ return dynamic_cast<Wire*>(this) != nullptr; }
	virtual bool					IsWire() const												{ return false; }
	virtual DrawableType 			GetType() const = 0;
	virtual std::string				Parser_WriteElementData(eElement* )							{ return ""; }
	virtual void					Parser_ReadElementData(eElement*, const std::string&)		{ }
	virtual void					Update(DrawableCircuit&, eElement*)							{ }
	std::shared_ptr<eDrawableBase>	GetSharedInstance()											{ return shared_from_this(); }

	bool IsFlippedOverX() const { return m_IsFlipped_X; }
	bool IsFlippedOverY() const { return m_IsFlipped_Y; }

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
	virtual DrawableType			GetType() const override { return DrawableType::DrawableResistorType; }
	virtual std::string				Parser_WriteElementData(eElement* elem) override;
	virtual void					Parser_ReadElementData(eElement* elem, const std::string& data) override;
};

class Battery : public eDrawableBase
{
public:
	Battery();
	virtual void					Draw() override;
	virtual sf::Vector2f			GetLocalPinPosition(int n) override;
	virtual void					UIParams(eElement* elem) override;
	virtual int						GetPinIndexFromLocalPosition(sf::Vector2f pos) override;
	virtual DrawableType			GetType() const override { return DrawableType::DrawableBatteryType; }
	virtual std::string				Parser_WriteElementData(eElement* elem) override;
	virtual void					Parser_ReadElementData(eElement* elem, const std::string& data) override;
}; 

class Capacitor : public eDrawableBase
{
public:
	Capacitor();
	virtual void					Draw() override;
	virtual sf::Vector2f			GetLocalPinPosition(int n) override;
	virtual void					UIParams(eElement* elem) override;
	virtual int						GetPinIndexFromLocalPosition(sf::Vector2f pos) override;
	virtual DrawableType			GetType() const override { return DrawableType::DrawableCapacitorType; }
	virtual std::string				Parser_WriteElementData(eElement* elem) override;
	virtual void					Parser_ReadElementData(eElement* elem, const std::string& data) override;
};

class Inductor : public eDrawableBase
{
public:
	Inductor();
	virtual void					Draw() override;
	virtual sf::Vector2f			GetLocalPinPosition(int n) override;
	virtual void					UIParams(eElement* elem) override;
	virtual int						GetPinIndexFromLocalPosition(sf::Vector2f pos) override;
	virtual DrawableType			GetType() const override { return DrawableType::DrawableInductorType; }
	virtual std::string				Parser_WriteElementData(eElement* elem) override;
	virtual void					Parser_ReadElementData(eElement* elem, const std::string& data) override;
};

class Diode : public eDrawableBase
{
public:
	Diode();
	virtual void					Draw() override;
	virtual sf::Vector2f			GetLocalPinPosition(int n) override;
	virtual void					UIParams(eElement* elem) override;
	virtual int						GetPinIndexFromLocalPosition(sf::Vector2f pos) override;
	virtual DrawableType			GetType() const override { return DrawableType::DrawableDiodeType; }
	virtual std::string				Parser_WriteElementData(eElement* elem) override;
	virtual void					Parser_ReadElementData(eElement* elem, const std::string& data) override;
};

class RelayContactsGroup : public eDrawableBase
{
	sf::Sprite m_BaseSprite;
	sf::Sprite m_ButtonSprite;
	sf::Vector2f m_ButtonPos;

	eRelayContactsGroup::State m_stateToDraw = eRelayContactsGroup::State::n11_n12;
	std::weak_ptr<eDrawableBase> m_Coil;

	char m_UiBuff[0x80]{};

public:

	RelayContactsGroup();
	virtual void					Draw() override;
	virtual sf::Vector2f			GetLocalPinPosition(int n) override;
	virtual void					UIParams(eElement* elem) override;
	virtual int						GetPinIndexFromLocalPosition(sf::Vector2f pos) override;
	virtual DrawableType			GetType() const override { return DrawableType::DrawableButtonType; }
	virtual std::string				Parser_WriteElementData(eElement* elem) override;
	virtual void					Parser_ReadElementData(eElement* elem, const std::string& data) override;
	virtual bool					IsHovered() override;
	virtual void					SetPosition(sf::Vector2f pos) override;
	virtual sf::Vector2f			GetPosition() override;
	virtual void					Update(DrawableCircuit& circ, eElement* elem) override;
	virtual void					SetRotation(float degrees) override;
	virtual void					Flip(flipAxis axis) override;
	
	bool LookupCoil(eRelayContactsGroup* my_eContact, DrawableCircuit& circ);
};

class NeutralRelayCoil : public eDrawableBase
{
	char m_UiBuff[0x80]{};

public:
	NeutralRelayCoil();
	virtual void					Draw() override;
	virtual sf::Vector2f			GetLocalPinPosition(int n) override;
	virtual void					UIParams(eElement* elem) override;
	virtual int						GetPinIndexFromLocalPosition(sf::Vector2f pos) override;
	virtual DrawableType			GetType() const override { return DrawableType::DrawableRelayCoilType; }
	virtual std::string				Parser_WriteElementData(eElement* elem) override;
	virtual void					Parser_ReadElementData(eElement* elem, const std::string& data) override;
	virtual void					Update(DrawableCircuit& circ, eElement* elem) override;
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
		f128 curcount = 0.0f;

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
	f128								m_curcount = 0.0f;


	void DrawThickLine(sf::Vector2f start, sf::Vector2f end, float thickness, sf::Color color);
	void DrawThickLineWithGradient(sf::Vector2f start, sf::Vector2f end, float thickness, sf::Color color1, sf::Color color2);

public:

	bool IsHoveredInUi = false;

	Wire();
	virtual void					Draw() override;
	virtual sf::Vector2f			GetGlobalPinPosition(int n) override;
	virtual sf::Vector2f			GetLocalPinPosition(int n) override;
	virtual void					UIParams(eElement* elem) override {}
	virtual int						GetPinIndexFromLocalPosition(sf::Vector2f pos) override;
	virtual bool 					IsWire() const override { return true; }
	virtual DrawableType			GetType() const override { return DrawableType::DrawableUnknownType; }
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

	auto AddResistor()	{ return AddElement<eResistor, Resistor>(5.0); }
	auto AddBattery()	{ return AddElement<eVoltageSource, Battery>(5.0); }
	auto AddWire()		{ return AddElement<eResistor, Wire>(1e-2); }
	auto AddCapacitor() { return AddElement<eCapacitor, Capacitor>(0.00001); }
	auto AddInductor()  { return AddElement<eInductor, Inductor>(0.5); }
	auto AddDiode()		{ return AddElement<eDiode, Diode>(); }
	auto AddButton()	{ return AddElement<eRelayContactsGroup, RelayContactsGroup>(*m_Circuit); }
	auto AddNeutralRelayCoil() { return AddElement<eCoil, NeutralRelayCoil>(0.2, 0.0); }

	template <typename SimTy, typename DrawTy>
	auto AddElement(auto&&... args)
	{
		eElement* elem = m_Circuit->AddElement<SimTy>(std::forward<decltype(args)>(args)...);
		std::shared_ptr<eDrawableBase> drawable = std::make_shared<DrawTy>();
		m_DrawableToElement[drawable.get()] = elem;
		m_DrawableElements.push_back(drawable);
		SyncWithCircuit();
		return drawable;
	}

	eElement* GetElecticElementFromDrawable(eDrawableBase* drawable);
	eNode* GetElecticNode(std::shared_ptr<ConnectionDot> dot);
	auto& GetDrawableElements() { return m_DrawableElements; }
};


class CircuitParser
{
	CircuitEditor* m_editor = nullptr;

	__forceinline void WriteVec2f(std::ostream& os, const sf::Vector2f& v)	{ os << vfmt("{:.6f} {:.6f}\n", v.x, v.y); }
	__forceinline void WriteFloat(std::ostream& os, float f)				{ os << vfmt("{:.6f}\n", f); }
	__forceinline void WriteInt(std::ostream& os, int n)					{ os << n << '\n'; }

	__forceinline sf::Vector2f ReadVec2f(const std::string& line)
	{
		if (auto result = scn::scan<float, float>(line, "{} {}"))
		{
			auto [a, b] = result->values();
			return { a, b };
		} 
		else
		{
			return { 0.0f, 0.0f };
		}
	}
	__forceinline float ReadFloat(const std::string& line)
	{
		if (auto result = scn::scan<float>(line, "{}"))
			return result->value();
		else
			return 0.0f;
	}
	__forceinline int ReadInt(const std::string& line) 
	{
		if (auto result = scn::scan<int>(line, "{}"))
			return result->value();
		else
			return 0;
	}

public:

	CircuitParser(CircuitEditor* editor) : m_editor(editor) { }
	void LoadFromFile(const std::filesystem::path& path);
	void SaveToFile(const std::filesystem::path& path);
};


struct ScrollingBuffer 
{
	int MaxSize;
	int Offset;
	ImVector<ImVec2> Data;
	float lastSampleX;
	float sampleInterval;

	ScrollingBuffer(int max_size = 2000, float display_interval = 1.0f)
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
	std::weak_ptr<eDrawableBase> GetDrawable() { return m_Drawable; }
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
	std::optional<std::pair<std::shared_ptr<eDrawableBase>,u64>> SearchForConnectionWithOtherWire(const sf::Vector2f& MousePos, sf::Vector2f& OutEnd, std::shared_ptr<eDrawableBase> selfWire);
	
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










