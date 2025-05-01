#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <scn/scan.h>
#include "imgui.h"

#include "Widgets/WidgetsBase.h"
#include "core/sim/components.h"
#include "core/DrawList.h"

class eElement;
class CircuitEditor;
class DrawableCircuit;
class ConnectionDot;
class Wire;


__forceinline void drawText(const auto& text, float x, float y, int charSize, sf::Color color = sf::Color::Black)
{
	sf::Text t(gSFMLRenderer.GetFont(), text, charSize);
	t.setFillColor(color);
	t.setPosition({ x, y });
	t.setString(text);
	dlDrawList::getWindow()->draw(t);
};

__forceinline void DrawCircle(sf::Vector2f position, float radius, sf::Color color = sf::Color::Black)
{
	sf::CircleShape circle(radius);
	circle.setFillColor(color);
	circle.setOrigin({ radius, radius });
	circle.setPosition(position);
	dlDrawList::getWindow()->draw(circle);
}

__forceinline sf::Color lerpColor(sf::Color color1, sf::Color color2, float t)
{
	return sf::Color(std::lerp(color1.r, color2.r, t),
					 std::lerp(color1.g, color2.g, t),
					 std::lerp(color1.b, color2.b, t),
					 std::lerp(color1.a, color2.a, t));
}


enum DrawableType
{
	DRAWABLE_RESISTOR,
	DRAWABLE_BATTERY,
	DRAWABLE_CAPACITOR,
	DRAWABLE_INDUCTOR,
	DRAWABLE_DIODE,
	DRAWABLE_RELAY_CONTACT_GROUP,
	DRAWABLE_RELAY_COIL_NEUTRAL,
	DRAWABLE_RELAY_COIL_NEUTRAL_THIRD_RELYABILITY_CLASS,
	DRAWABLE_RELAY_COIL_NEUTRAL_WITH_SWITCH_OFF_DELAY,
	DRAWABLE_RELAY_COIL_NEUTRAL_WITH_SWITCH_OFF_DELAY_THIRD_RELYABILITY_CLASS,
	DRAWABLE_RELAY_COIL_NEUTRAL_WITH_RECTIFIER,
	DRAWABLE_DIODE_BRIDGE,
	DRAWABLE_TRANSFORMER,
	DRAWABLE_TRANSFORMER_WITH_MIDDLE_PIN,
	DRAWABLE_KPTSH,
	DRAWABLE_ZBF,
	
	DRAWABLE_UNKNOWN = -1,
};

enum class flipAxis { X, Y };

class eDrawableBase : public WidgetBase, public std::enable_shared_from_this<eDrawableBase>
{	
protected:

	std::vector<sf::Vector2f> m_PinPositions;
	sf::Angle m_Rotation{};
	bool m_IsFlipped_X = false;
	bool m_IsFlipped_Y = false;
	bool m_Hide = false;

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
	virtual int						GetPinIndexFromLocalPosition(sf::Vector2f pos);
	virtual void					SetRotation(float degrees);
	virtual u64 					GetNumPins() const											{ return m_PinPositions.size(); }
	sf::Angle						GetRotation() const											{ return m_Rotation; }
	virtual void					Flip(flipAxis axis);
	//bool							IsWire()													{ return dynamic_cast<Wire*>(this) != nullptr; }
	virtual bool					IsWire() const												{ return false; }
	virtual DrawableType 			GetType() const = 0;
	virtual std::string				Parser_WriteElementData(eElement*)							{ return ""; }
	virtual void					Parser_ReadElementData(eElement*, const std::string&)		{ }
	virtual void					Update(DrawableCircuit&, eElement*)							{ }
	std::shared_ptr<eDrawableBase>	GetSharedInstance()											{ return shared_from_this(); }
	virtual bool					CanHaveOscilloscope() const = 0;
	virtual const char*				GetTypeName() const = 0;

	bool IsFlippedOverX() const { return m_IsFlipped_X; }
	bool IsFlippedOverY() const { return m_IsFlipped_Y; }
	bool& Hide() { return m_Hide; }

	template <typename T> T* As() { return /*dynamic_cast*/static_cast<T*>(this); }
	
	__forceinline bool IsCoil()
	{
		DrawableType coilTypes[] = {
			DRAWABLE_RELAY_COIL_NEUTRAL,
			DRAWABLE_RELAY_COIL_NEUTRAL_THIRD_RELYABILITY_CLASS,
			DRAWABLE_RELAY_COIL_NEUTRAL_WITH_SWITCH_OFF_DELAY,
			DRAWABLE_RELAY_COIL_NEUTRAL_WITH_SWITCH_OFF_DELAY_THIRD_RELYABILITY_CLASS,
			DRAWABLE_RELAY_COIL_NEUTRAL_WITH_RECTIFIER,
		};

		return std::ranges::find(coilTypes, GetType()) != std::end(coilTypes);
	}
};


class Resistor : public eDrawableBase
{
public:
	using SimTy = eResistor;

	Resistor();
	virtual void					Draw() override;
	virtual void					UIParams(eElement* elem) override;
	virtual DrawableType			GetType() const override { return DrawableType::DRAWABLE_RESISTOR; }
	virtual std::string				Parser_WriteElementData(eElement* elem) override;
	virtual void					Parser_ReadElementData(eElement* elem, const std::string& data) override;
	virtual bool					CanHaveOscilloscope() const override { return true; }
	virtual const char*				GetTypeName() const override { return "Resistor"; }
};


class Battery : public eDrawableBase
{
public:
	using SimTy = eVoltageSource;

	Battery();
	virtual void					Draw() override;
	virtual void					UIParams(eElement* elem) override;
	virtual DrawableType			GetType() const override { return DrawableType::DRAWABLE_BATTERY; }
	virtual std::string				Parser_WriteElementData(eElement* elem) override;
	virtual void					Parser_ReadElementData(eElement* elem, const std::string& data) override;
	virtual bool					CanHaveOscilloscope() const override { return true; }
	virtual const char*				GetTypeName() const override { return "Battery"; }
}; 


class Capacitor : public eDrawableBase
{
public:
	using SimTy = eCapacitor;

	Capacitor();
	virtual void					Draw() override;
	virtual void					UIParams(eElement* elem) override;
	virtual DrawableType			GetType() const override { return DrawableType::DRAWABLE_CAPACITOR; }
	virtual std::string				Parser_WriteElementData(eElement* elem) override;
	virtual void					Parser_ReadElementData(eElement* elem, const std::string& data) override;
	virtual bool					CanHaveOscilloscope() const override { return true; }
	virtual const char*				GetTypeName() const override { return "Capacitor"; }
};


class Inductor : public eDrawableBase
{
public:
	using SimTy = eInductor;

	Inductor();
	virtual void					Draw() override;
	virtual void					UIParams(eElement* elem) override;
	virtual DrawableType			GetType() const override { return DrawableType::DRAWABLE_INDUCTOR; }
	virtual std::string				Parser_WriteElementData(eElement* elem) override;
	virtual void					Parser_ReadElementData(eElement* elem, const std::string& data) override;
	virtual bool					CanHaveOscilloscope() const override { return true; }
	virtual const char*				GetTypeName() const override { return "Inductor"; }
};


class Diode : public eDrawableBase
{
public:
	using SimTy = eDiode;

	Diode();
	virtual void					Draw() override;
	virtual void					UIParams(eElement* elem) override;
	virtual DrawableType			GetType() const override { return DrawableType::DRAWABLE_DIODE; }
	virtual std::string				Parser_WriteElementData(eElement* elem) override;
	virtual void					Parser_ReadElementData(eElement* elem, const std::string& data) override;
	virtual bool					CanHaveOscilloscope() const override { return true; }
	virtual const char*				GetTypeName() const override { return "Diode"; }
};


class Transformer : public eDrawableBase
{
public:
	using SimTy = eTransformer;

	Transformer();
	virtual void					Draw() override;
	virtual void					UIParams(eElement* elem) override;
	virtual DrawableType			GetType() const override { return DrawableType::DRAWABLE_TRANSFORMER; }
	virtual std::string				Parser_WriteElementData(eElement* elem) override;
	virtual void					Parser_ReadElementData(eElement* elem, const std::string& data) override;
	virtual bool					CanHaveOscilloscope() const override { return false; }
	virtual const char*				GetTypeName() const override { return "Transformer"; }
};


class TransformerWithMiddlePin : public eDrawableBase
{
public:
	using SimTy = eTransformerWithMiddlePin;
	TransformerWithMiddlePin();
	virtual void					Draw() override;
	virtual void					UIParams(eElement* elem) override;
	virtual DrawableType			GetType() const override { return DrawableType::DRAWABLE_TRANSFORMER_WITH_MIDDLE_PIN; }
	virtual std::string				Parser_WriteElementData(eElement* elem) override;
	virtual void					Parser_ReadElementData(eElement* elem, const std::string& data) override;
	virtual bool					CanHaveOscilloscope() const override { return false; }
	virtual const char*				GetTypeName() const override { return "Transformer with Middle Tap"; }
};


class RelayContactsGroup : public eDrawableBase
{
	enum NormalState { NormalOpen, NormalClosed, };

	sf::Sprite m_BaseSprite;
	sf::Sprite m_SwitchSprite;
	sf::Vector2f m_SwitchPos;

	eRelayContactsGroup::State m_NormalState = eRelayContactsGroup::State::n11_n12;
	eRelayContactsGroup::State m_stateToDraw = eRelayContactsGroup::State::n11_n12;
	std::weak_ptr<eDrawableBase> m_Coil;

	char m_UiBuff[0x80]{};

public:
	using SimTy = eRelayContactsGroup;

	RelayContactsGroup();
	virtual void					Draw() override;
	virtual void					UIParams(eElement* elem) override;
	virtual DrawableType			GetType() const override { return DrawableType::DRAWABLE_RELAY_CONTACT_GROUP; }
	virtual bool					IsHovered() override;
	virtual void					SetPosition(sf::Vector2f pos) override;
	virtual sf::Vector2f			GetPosition() override;
	virtual void					Update(DrawableCircuit& circ, eElement* elem) override;
	virtual void					SetRotation(float degrees) override;
	virtual void					Flip(flipAxis axis) override;
	virtual bool					CanHaveOscilloscope() const override { return false; }
	virtual const char*				GetTypeName() const override { return "Relay Contacts Group"; }

	virtual std::string				Parser_WriteElementData(eElement* elem) override;
	virtual void					Parser_ReadElementData(eElement* elem, const std::string& data) override;

	bool							LookupCoil(eRelayContactsGroup* my_eContact, DrawableCircuit& circ);
	void 							SetStateToDraw(eRelayContactsGroup::State state) { m_stateToDraw = state; }
};


class Coil : public eDrawableBase
{
protected:
	char m_UiBuff[0x80]{};

public:
	using SimTy = eCoil;

	Coil(const std::string& path);

	virtual void					Draw() override;
	virtual void					UIParams(eElement* elem) override;
	virtual std::string				Parser_WriteElementData(eElement* elem) override;
	virtual void					Parser_ReadElementData(eElement* elem, const std::string& data) override;
	virtual void					Update(DrawableCircuit& circ, eElement* elem) override;
	virtual bool					CanHaveOscilloscope() const override { return true; }
	virtual const char*				GetTypeName() const override { return "Coil"; }
};


class NeutralRelayCoil : public Coil
{
public:
	NeutralRelayCoil();
	virtual DrawableType GetType() const override { return DrawableType::DRAWABLE_RELAY_COIL_NEUTRAL; }
	virtual const char* GetTypeName() const override { return "Neutral Relay Coil"; }
};


class NeutralRelayCoilWithSwitchOffDelay : public Coil
{
public:
	NeutralRelayCoilWithSwitchOffDelay();
	virtual DrawableType GetType() const override { return DrawableType::DRAWABLE_RELAY_COIL_NEUTRAL_WITH_SWITCH_OFF_DELAY; }
	virtual const char* GetTypeName() const override { return "Neutral Relay Coil with Switch Off Delay"; }
};


class NeutralRelayCoil3RelyabilityClass : public Coil
{
public:
	NeutralRelayCoil3RelyabilityClass();
	virtual DrawableType GetType() const override { return DrawableType::DRAWABLE_RELAY_COIL_NEUTRAL_THIRD_RELYABILITY_CLASS; }
	virtual const char* GetTypeName() const override { return "Neutral Relay Coil 3rd Relyability Class"; }
};


class NeutralRelayCoilWithSwitchOffDelay3RelyabilityClass : public Coil
{
public:
	NeutralRelayCoilWithSwitchOffDelay3RelyabilityClass();
	virtual DrawableType GetType() const override { return DrawableType::DRAWABLE_RELAY_COIL_NEUTRAL_WITH_SWITCH_OFF_DELAY_THIRD_RELYABILITY_CLASS; }
	virtual const char* GetTypeName() const override { return "Neutral Relay Coil with Switch Off Delay 3rd Relyability Class"; }
};


class NeutralRelayCoilWithRectifier : public Coil
{
	char m_UiBuff[0x80]{};
public:
	using SimTy = eCoilWithRectifier;

	NeutralRelayCoilWithRectifier();

	virtual void					UIParams(eElement* elem) override;
	virtual DrawableType			GetType() const override { return DrawableType::DRAWABLE_RELAY_COIL_NEUTRAL_WITH_RECTIFIER; }
	virtual std::string				Parser_WriteElementData(eElement* elem) override;
	virtual void					Parser_ReadElementData(eElement* elem, const std::string& data) override;
	virtual const char*				GetTypeName() const override { return "Neutral Relay Coil with Rectifier"; }
};


class DiodeBridge : public eDrawableBase
{
public:
	using SimTy = eDiodeBridge;
	DiodeBridge();
	virtual void					Draw() override;
	virtual DrawableType			GetType() const override { return DrawableType::DRAWABLE_DIODE_BRIDGE; }
	virtual std::string				Parser_WriteElementData(eElement* elem) override { return ""; }
	virtual void					Parser_ReadElementData(eElement* elem, const std::string& data) override { }
	virtual bool					CanHaveOscilloscope() const override { return false; }
	virtual void					UIParams(eElement* element) override { }
	virtual const char*				GetTypeName() const override { return "Diode Bridge"; }
};


class KPTSH : public eDrawableBase
{
	RelayContactsGroup m_Groups[3];
	sf::Vector2f m_groupOffsets[3];
public:
	using SimTy = eKPTSH;
	KPTSH();
	virtual void					Draw() override;
	virtual DrawableType			GetType() const override { return DrawableType::DRAWABLE_KPTSH; }
	virtual std::string				Parser_WriteElementData(eElement* elem) override;
	virtual void					Parser_ReadElementData(eElement* elem, const std::string& data) override;
	virtual bool					CanHaveOscilloscope() const override { return false; }
	virtual void					UIParams(eElement* element) override;
	virtual const char*				GetTypeName() const override { return "KPTSH"; }
	virtual void 					SetRotation(float degrees) override { }
	virtual void					Flip(flipAxis axis) override { }
	virtual void 					SetPosition(sf::Vector2f pos) override;
	virtual void					Update(DrawableCircuit& circ, eElement* elem) override;
};

class ZBF : public eDrawableBase
{
public:
	using SimTy = eZBF;
	ZBF();
	virtual void 					Draw() override;
	virtual DrawableType			GetType() const override { return DrawableType::DRAWABLE_ZBF; }
	virtual const char*				GetTypeName() const override { return "ZBF"; }
	virtual bool					CanHaveOscilloscope() const override { return false; }
	virtual void					UIParams(eElement* element) override { }
};