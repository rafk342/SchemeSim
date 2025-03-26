#pragma once
#include <iostream>
#include <vector>
#include <map>
#include <set>
#define _USE_MATH_DEFINES
#include <math.h>
#include "matrix.h"

#define DIODE_VERSION 2


class ePin;
class eNode;
class eElement;
class Circuit;
class CircuitMtx;


template<typename T>
inline bool IsAlmostEqual(T a, T b, T epsilon = 1e-9) { return std::abs(a - b) < epsilon; }  


class eNode
{
	std::set<ePin*> m_ePins;

	double m_TotalCurr = 0.0;
	double m_Voltage = 0.0;

	size_t m_NodeIndex;

public:

	eNode(size_t index);
	~eNode();

	void	AddEpin(ePin* epin);
	void	RemoveEpin(ePin* epin);
	double	GetVoltage() const { return m_Voltage; }
	void	SetVoltage(double Volt) { m_Voltage = IsAlmostEqual(Volt, 0.0) ? 0.0 : Volt; }
	size_t	GetIndex() const { return m_NodeIndex; }
	size_t	GetNumConnectedPins() const { return m_ePins.size(); }
	void    SetIndex(size_t index) { m_NodeIndex = index; }
};



class ePin
{
	eNode* m_Enode = nullptr;				// Node connected to this pin
	eElement* m_Element = nullptr;			// Element that owns this pin

public:

	ePin() = default;
	~ePin();

	bool		HasParentElement();
	void		SetParentElement(eElement* element);
	eElement*	GetParentElement();
	bool		IsConnectedToNode();
	void		ConnectToNode(eNode* enode);
	eNode*		GetConnectedNode();
	void		ReleaseNode();
	double		GetVoltage() const { return m_Enode ? m_Enode->GetVoltage() : 0.0; }

};


enum ElementType_e : u8
{
	ty_Resistor,
	ty_Capacitor,
	ty_Inductor,
	ty_VoltageSource,
	ty_Diode,
	ty_Potentiometer,
	ty_Button,
	ty_Contact,
	ty_Unknown
};


class eElement
{
protected:

	std::vector<ePin> m_ePins;

	virtual void SetNumEpins(int n) final;
	void SetEpin(int num, ePin pin);

public:

	eElement() = default;
	virtual ~eElement();

	virtual void Stamp(CircuitMtx& mtx, eNode* GndNode, double dt) { }
	virtual void Update(CircuitMtx& mtx, double dt) { }					// Called after matrix was solved
	virtual void InitMatrix(CircuitMtx& mtx) { }						// Called before matrix will be assembled
	virtual ePin* GetEpin(int num);
	virtual size_t GetNumEpins()		{ return m_ePins.size(); }
	virtual ElementType_e GetType()		{ return ty_Unknown; }
	
	const char* GetTypeName();
	void ReleaseConnectedNodes();
};


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


class eResistor : public eElement
{
	double m_Resistance;
	double m_Current = 0.0;

public:

	eResistor(double resistance);
	virtual void Stamp(CircuitMtx& mtx, eNode* GndNode, double dt) override;
	virtual ElementType_e GetType() override { return ty_Resistor; }

	double GetCurrent();
	double GetVoltDrop();
	double GetResistance() { return m_Resistance; }
	void SetResistance(double resistance) { m_Resistance = resistance; }
};


class ePotentiometer : public eElement
{
	eResistor m_R1;
	eResistor m_R2;
	double m_TotalResistance;
	double m_SliderPosition; // 0.0 - 1.0
	eNode* m_MiddleNode; // This is here for the case when we didn't connect output pin to any node

public:

	ePotentiometer(Circuit& circuit, double TotalResistance, double SliderPosition = 0.5);

	virtual void Stamp(CircuitMtx& mtx, eNode* GndNode, double dt) override;
	virtual ePin* GetEpin(int num) override;
	virtual ElementType_e GetType() override { return ty_Potentiometer; }

	void SetSliderPosition(double position);
	double GetSliderPosition() const;
	double GetTotalResistance() const;
	void SetTotalResistance(double resistance);

	ePin* GetLeftPin()				{ return m_R1.GetEpin(0); }
	ePin* GetRightPin()				{ return m_R2.GetEpin(1); }
	eNode* GetOutputNode()			{ return m_MiddleNode; }
	
	void ConnectOutputPinToNode(eNode* node);
	void ReleaseNodeFromOutputPin();
	double GetOutputVoltage();
};


class eVoltageSource : public eElement
{
	double m_Amplitude;		// (for DC = volt, for AC = max volt)
	double m_Frequency;		// (0 for DC)
	double m_Phase;			// (0 for DC)
	double m_Time;

	size_t m_eqIndex = 0;

public:

	eVoltageSource(double amplitude, double frequency = 0.0, double phase = 0.0);
	double GetVoltage(double t) const;

	virtual void Stamp(CircuitMtx& mtx, eNode* GndNode, double dt) override;
	virtual void Update(CircuitMtx& mtx, double dt) override;
	virtual void InitMatrix(CircuitMtx& mtx) override;
	virtual ElementType_e GetType() override { return ty_VoltageSource; }

	ePin* GetPositivePin() { return &m_ePins[0]; }
	ePin* GetNegativePin() { return &m_ePins[1]; }

};




class eCapacitor : public eElement
{
	double m_Capacitance;			// C, F
	double m_PrevVoltage = 0.0;

public:

	eCapacitor(double capacitance);
	virtual void Stamp(CircuitMtx& mtx, eNode* GndNode, double dt) override;
	virtual void Update(CircuitMtx& mtx, double dt) override;
	virtual ElementType_e GetType() override { return ty_Capacitor; }

};




#if DIODE_VERSION == 1

class eDiode : public eElement
{
	double m_Vt;  // Thermal voltage (~0.025V at room temp)
	double m_Is;  // Saturation current (really small value)

	double m_G;
	double m_Ieq;

public:

	eDiode(double Vt = 0.025, double Is = 1e-12);

	virtual void Stamp(CircuitMtx& mtx, eNode* GndNode, double dt) override;
	virtual void Update(CircuitMtx& mtx, double dt) override;

	double GetVoltDrop();

	ePin* GetAnodePin() { return &m_ePins[0]; }
	ePin* GetCathodePin() { return &m_ePins[1]; }

	double GetG() { return m_G; }
	double GetIeq() { return m_Ieq; }
};


#elif DIODE_VERSION == 2


class eDiode : public eElement
{
	// https://github.com/sharpie7/circuitjs1/blob/master/src/com/lushprojects/circuitjs1/client/Diode.java

	double m_Vt;		// Thermal voltage (~0.025V at room temp)
	double m_Is;		// Saturation current (should be really small value)
	double m_ZVoltage;	// Zener breakdown voltage (0 if not used)
	double m_ZOffset;	// Offset for Zener breakdown exponential
	double m_Vcrit;		// Critical voltage for limiting exponential growth
	double m_Vzcrit;	// Critical voltage for Zener breakdown limiting
	double m_Vscale;	// Voltage scale (same as Vt in most cases)
	double m_Vdcoef;	// 1 / Vscale for speed
	double m_Vzcoef;	// 1 / Vt for Zener breakdown
	double m_LastVd;	// Last voltage drop for convergence checks
	double m_G;			// Equivalent conductance
	double m_Ieq;		// Equivalent current source


	void SetupCriticalVoltages();
	double LimitVoltageStep(double Vnew, double Vold);

public:

	eDiode(double Vt = 0.025, double Is = 1e-12, double ZVoltage = 0);
	
	virtual void Stamp(CircuitMtx& mtx, eNode* GndNode, double dt) override;
	virtual void Update(CircuitMtx& mtx, double dt) override;
	virtual ElementType_e GetType() override { return ty_Diode; }
	
	double GetVoltDrop();
	ePin* GetAnodePin() { return &m_ePins[0]; }
	ePin* GetCathodePin() { return &m_ePins[1]; }
};


#elif DIODE_VERSION == 3

class eDiode : public eElement
{

};


#endif // DIODE_VERSION



class eInductor : public eElement
{
	double m_Inductance;			// L, H
	double m_prevCurrent = 0.0;

	size_t m_CurrenIndex = 0;

public:

	eInductor(double inductance);

	virtual void Stamp(CircuitMtx& mtx, eNode* GndNode, double dt) override;
	virtual void Update(CircuitMtx& mtx, double dt) override;
	virtual void InitMatrix(CircuitMtx& mtx) override;
	virtual ElementType_e GetType() override { return ty_Inductor; }

	double GetInductance() { return m_Inductance; }
	void SetInductance(double inductance) { m_Inductance = inductance; }
	double GetCurrent();
	double GetVoltDrop();

};



class eSwitchBase : public eElement
{
public:
	enum NormalState_e : u8
	{
		NormalOpen,
		NormalClosed
	};
protected:

	std::vector<eResistor> m_switches; // the easiest way to impl switches
	double m_OnResistance = 1e-3;
	double m_OffResistance = 1e9;
	NormalState_e m_NormState = NormalOpen;

public:

	eSwitchBase() = default;
	~eSwitchBase();

	virtual void SetupSwitches() = 0;
	virtual void SetState(int state) = 0; // !! int should be enum from derived class, same with GetEpin(n)

	virtual void InitMatrix(CircuitMtx& mtx) override;
	virtual void Stamp(CircuitMtx& mtx, eNode* GndNode, double dt) override;

	void SetNormalState(NormalState_e state) { m_NormState = state; }
};


class eButton : public eSwitchBase
{
public:
	enum ButtonState_e : int
	{
		Pressed = 1,
		Released = 0,
	};

private:
	ButtonState_e m_State;

public:

	eButton(NormalState_e state);

	virtual void SetupSwitches() override;
	virtual void SetState(int state) override;
	virtual ePin* GetEpin(int num) override;
	virtual size_t GetNumEpins() override { return 2; }
	virtual ElementType_e GetType() override { return ty_Button; }

	void Press() { SetState(Pressed); }
	void Release() { SetState(Released); }
};
