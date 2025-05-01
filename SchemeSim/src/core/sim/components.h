#pragma once
#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <functional> 
#define _USE_MATH_DEFINES
#include <math.h>
#include "matrix.h"


class ePin;
class eNode;
class eElement;
class Circuit;
class CircuitMtx;


template<typename T>
inline bool IsAlmostEqual(T a, T b, T epsilon = 1e-9) { return std::abs(a - b) < epsilon; }  

template<typename T>
inline bool IsInRange(T value, T min, T max) { return value >= min && value <= max; }

class eNode
{
	std::set<ePin*> m_ePins;

	double m_TotalCurr = 0.0;
	double m_Voltage = 0.0;

	u64 m_NodeIndex;

public:

	eNode(u64 index);
	~eNode();

	void	AddEpin(ePin* epin);
	void	RemoveEpin(ePin* epin);
	double	GetVoltage() const					{ return m_Voltage; }
	void	SetVoltage(double Volt)				{ m_Voltage = IsAlmostEqual(Volt, 0.0) ? 0.0 : Volt; }
	u64		GetIndex() const					{ return m_NodeIndex; }
	u64		GetNumConnectedPins() const			{ return m_ePins.size(); }
	void    SetIndex(u64 index)					{ m_NodeIndex = index; }
	auto&	GetPins()							{ return m_ePins; }
	void	ReleaseAllPins();
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


//enum ElementType_e : u8
//{
//	eTYPE_RESISTOR,
//	eTYPE_CAPACITOR,
//	eTYPE_INDUCTOR,
//	eTYPE_VOLT_SOURCE,
//	eTYPE_DIODE,
//	eTYPE_POTENTIOMETER,
//	eTYPE_RELAY_SWITCH,
//	eTYPE_COIL,
//	eTYPE_BUTTON,
//	eTYPE_TRANSFORMER,
//	eTYPE_DIODE_BRIDGE,
//	
//	eTYPE_UNKNOWN = 255,
//};


class eElement
{
protected:

	std::vector<ePin>		m_ePins;

	virtual void			SetNumEpins(int n) final;
	void					SetEpin(int num, ePin pin);

public:

	eElement() = default;
	virtual ~eElement();

	virtual void			Stamp(CircuitMtx& mtx, eNode* GndNode, double dt) { }
	virtual void			Update(CircuitMtx& mtx, double dt)	{ }		// Called after matrix was solved
	virtual void			InitMatrix(CircuitMtx& mtx)			{ }		// Called before matrix is assembled
	virtual void			Reset()								{ }

	virtual ePin*			GetEpin(int num);
	//virtual ElementType_e	GetType()							{ return eTYPE_UNKNOWN; }
	virtual double			GetCurrent()						{ return 0.0; }
	virtual double			GetVoltDrop()						{ return 0.0; }
	virtual void			ReleaseConnectedNodes();

	//const char* GetTypeName();
};


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


class eResistor : public eElement
{
	friend class Resistor;
	friend class NeutralRelayCoilWithRectifier;

	double m_Resistance;

public:

	eResistor(double resistance);
	virtual void Stamp(CircuitMtx& mtx, eNode* GndNode, double dt) override;
	virtual double GetCurrent() override;
	virtual double GetVoltDrop() override;

	double GetResistance() { return m_Resistance; }
	void SetResistance(double resistance) { m_Resistance = resistance; }
};



class ePotentiometer : public eElement
{
	// --(r1)----(r2)---
	//         |
	//         |-(wire)--

	eResistor m_R1;
	eResistor m_R2;
	eResistor m_RWire;
	double m_TotalResistance;
	double m_SliderPosition; // 0.0 - 1.0
	eNode* m_MiddleNode;

	Circuit* m_Circuit;

public:

	ePotentiometer(Circuit& circuit, double TotalResistance, double SliderPosition = 0.5);
	~ePotentiometer();
	virtual void Stamp(CircuitMtx& mtx, eNode* GndNode, double dt) override;
	virtual ePin* GetEpin(int num) override;

	void		SetSliderPosition(double position);
	double		GetSliderPosition() const;
	double		GetTotalResistance() const;
	void		SetTotalResistance(double resistance);

	ePin*		GetLeftPin();
	ePin*		GetRightPin();
	ePin*		GetMiddlePin();
	
	double		GetOutputVoltage();
};



class eVoltageSource : public eElement
{
	friend class Battery;

	double m_Amplitude;		// (for DC = volt, for AC = max volt)
	double m_Frequency;		// (0 for DC)
	double m_Phase;			// (0 for DC)
	double m_Time;
	double m_Current = 0.0; 
	u64 m_eqIndex = 0;

public:

	eVoltageSource(double amplitude, double frequency = 0.0, double phase = 0.0);
	double GetVoltage(double t) const;

	virtual void Stamp(CircuitMtx& mtx, eNode* GndNode, double dt) override;
	virtual void Update(CircuitMtx& mtx, double dt) override;
	virtual void InitMatrix(CircuitMtx& mtx) override;
	virtual void Reset() override { m_Time = 0.0; }
	virtual double GetCurrent() override;
	virtual double GetVoltDrop() override;

	ePin* GetPositivePin() { return &m_ePins[0]; }
	ePin* GetNegativePin() { return &m_ePins[1]; }

};


class eCapacitor : public eElement
{
	friend class Capacitor;
	double m_Capacitance;			// C, F
	double m_PrevVoltage = 0.0;
	double m_Current = 0.0;
public:

	eCapacitor(double capacitance);
	virtual void Stamp(CircuitMtx& mtx, eNode* GndNode, double dt) override;
	virtual void Update(CircuitMtx& mtx, double dt) override;
	virtual void Reset() override;
	virtual double GetCurrent() override;
	virtual double GetVoltDrop() override;

};

#define DIODE_VER 2


#if DIODE_VER == 1

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


#elif DIODE_VER == 2

// https://github.com/sharpie7/circuitjs1/blob/master/src/com/lushprojects/circuitjs1/client/Diode.java

class eDiode : public eElement
{
	friend class Diode;
	friend class NeutralRelayCoilWithRectifier;

	double m_Vt;		// Thermal voltage (~0.025V at 27 C (300.15 K))
	double m_Is;		// Saturation current (should be really small value)
	double m_ZVoltage;	// Zener breakdown voltage (0 if not used)
	double m_ZOffset;	// Offset for Zener breakdown exponential
	double m_Vcrit;		// Critical voltage for limiting exponential growth
	double m_Vzcrit;	// Critical voltage for Zener breakdown limiting
	double m_Vscale;	// "scale voltage" - the voltage increase which will raise current by a factor of e.
	double m_Vdcoef;	// 1 / Vscale
	double m_Vzcoef;	// 1 / Vt for Zener breakdown
	double m_LastVd;	// Last voltage drop for convergence checks
	double m_G;			// Equivalent conductance
	double m_Ieq;		// Equivalent current source


	void SetupCriticalVoltages();
	double LimitVoltStep(double Vnew, double Vold);

public:

	eDiode(double Vt = 0.025865, double Is = 1e-12, double ZVoltage = 0);
	
	virtual void Stamp(CircuitMtx& mtx, eNode* GndNode, double dt) override;
	virtual void Update(CircuitMtx& mtx, double dt) override;
	
	double GetVoltDrop();
	ePin* GetAnodePin()		{ return &m_ePins[0]; }
	ePin* GetCathodePin()	{ return &m_ePins[1]; }

	virtual void Reset() override { m_LastVd = 0.0; }
};


#elif DIODE_VER == 3

class eDiode : public eElement
{

};


#endif // DIODE_VER


class eDiodeBridge : public eElement
{
	friend NeutralRelayCoilWithRectifier;

	//       in0 ----(wire0)-----
	//                   d0     |   d1
	//                -----|>|--n0--|>|------
	//                |                     |
	// out2---(wire2)-n2                    n3---(wire3)---out3
	//                |   d2          d3    |
	//                -----|>|--n1---|>|-----
	//                          |
	//        in1 ----(wire1)----

	friend class DiodeBridge;
	eDiode m_Diodes[4];
	eResistor m_Wires[4];
	eNode* m_InnerNodes[4];
	Circuit* m_Circuit;

public:

	enum PinIndex
	{
		In0 = 0,
		In1 = 1,
		OutMinus = 2,
		OutPlus = 3,
	};

	eDiodeBridge(Circuit& circuit);
	~eDiodeBridge();
	virtual void Stamp(CircuitMtx& mtx, eNode* GndNode, double dt) override;
	virtual void Update(CircuitMtx& mtx, double dt) override;
	virtual void Reset() override;
	virtual ePin* GetEpin(int num);

	ePin* GetIn0() { return m_Wires[0].GetEpin(0); }
	ePin* GetIn1() { return m_Wires[1].GetEpin(0); }
	ePin* GetOutMinus() { return m_Wires[2].GetEpin(0); }
	ePin* GetOutPlus() { return m_Wires[3].GetEpin(0); }
};


class eInductor : public eElement
{
protected:

	friend class Inductor;
	friend class Coil;
	friend class NeutralRelayCoilWithRectifier;

	double m_Inductance;			// L, H
	double m_prevCurrent = 0.0;
	double m_R = 1.0;

	u64 m_CurrenIndex = 0;

public:

	eInductor(double inductance);

	virtual void Stamp(CircuitMtx& mtx, eNode* GndNode, double dt) override;
	virtual void Update(CircuitMtx& mtx, double dt) override;
	virtual void InitMatrix(CircuitMtx& mtx) override;

	double GetInductance() { return m_Inductance; }
	void SetInductance(double inductance) { m_Inductance = inductance; }
	double GetCurrent();
	double GetVoltDrop();

	virtual void Reset() override { m_prevCurrent = 0.0; }
};


class eRelayContactsGroup : public eElement
{
	friend class RelayContactsGroup;

public:

	enum State { n11_n12, n11_n13, };
	enum RelayContact { N11, N12, N13, ContactsCount, };

protected:

	static inline const double maxResistance = 1e12;
	static inline const double minResistance = 1e-12;

	// --(r11)----(r12)---
	//         |
	// -(r13)--|
	//   

	//     11     12
	// -----______-------
	//      
	// _____|
	//     13

	std::string		m_CoilName;
	u64				m_HashName = 0;

	eResistor		m_R11;
	eResistor		m_R12;
	eResistor		m_R13;
	eNode*			m_MiddleNode;
	State			m_State;
	Circuit*		m_Circuit;


public:

	eRelayContactsGroup(Circuit& circuit);
	~eRelayContactsGroup();

	virtual void			Stamp(CircuitMtx& mtx, eNode* GndNode, double dt) override;
	virtual ePin*			GetEpin(int num) override;

	void					SetState(State state);
	State					GetState();
	void					SetCoilName(const std::string& name);
	const std::string&		GetCoilName() const										{ return m_CoilName; }
	u64						GetCoilHashName()										{ return m_HashName; }
};


class eCoil : public eElement
{
	friend class Coil;
	friend class NeutralRelayCoilWithRectifier;
	friend class eCoilWithRectifier;

	std::string		m_Name;
	u64				m_HashName = 0;
	double			m_ReleaseDelay; // in seconds
	double			m_CurrThreshold = 0.015;
	double			m_InactiveCoilTimer = 0.0;
	bool			m_IsActive = false;

protected:
	eInductor		m_l;

public:

	eCoil(double Inductance, double ReleaseDelay = 0.0);
	
	virtual void			Stamp(CircuitMtx& mtx, eNode* GndNode, double dt);
	virtual void			Update(CircuitMtx& mtx, double dt) override;
	virtual void			InitMatrix(CircuitMtx& mtx) override;
	virtual void			Reset() override;
	
	virtual ePin*			GetEpin(int num) override;
	virtual double			GetCurrent() override							{ return m_l.GetCurrent(); }
	virtual double			GetVoltDrop() override							{ return m_l.GetVoltDrop(); }
	
	bool					IsActive()										{ return m_IsActive; }
	void					SetName(const std::string& name);
	const std::string&		GetName()										{ return m_Name; }
	u64						GetHashName()									{ return m_HashName; }
};


class eCoilWithRectifier : public eCoil
{
	friend class NeutralRelayCoilWithRectifier;
	
	eDiodeBridge m_db;
	eResistor m_r;
	eNode* m_Nodes[3];
	Circuit* m_Circuit;

public:

	eCoilWithRectifier(Circuit& circ, double Inductance, double ReleaseDelay = 0.0);
	~eCoilWithRectifier();
	virtual ePin* GetEpin(int num) override;
	virtual void Stamp(CircuitMtx& mtx, eNode* GndNode, double dt) override;
	virtual void Update(CircuitMtx& mtx, double dt) override;
	virtual void Reset() override { m_InactiveCoilTimer = 0.0; m_IsActive = false; }
};


class eTransformer : public eElement
{
	friend class Transformer;
	friend class TransformerWithMiddlePin;

	double m_L1;
	double m_L2;
	double m_R1 = 1.0;
	double m_R2 = 1.0;
	double m_CouplCoef;
	double m_I1 = 0.0;
	double m_I2 = 0.0;

	u64 m_I1_Idx = -1;
	u64 m_I2_Idx = -1;

public:

	eTransformer(double L1, double ratio);
	eTransformer(double L1, double L2, double transformCoef);
	virtual void InitMatrix(CircuitMtx& mtx) override;
	virtual void Stamp(CircuitMtx& mtx, eNode* GndNode, double dt) override;
	virtual void Update(CircuitMtx& mtx, double dt) override;

	ePin* GetPrimaryPin1()		{ return &m_ePins[0]; }
	ePin* GetPrimaryPin2()		{ return &m_ePins[1]; }
	ePin* GetSecondaryPin1()	{ return &m_ePins[2]; }
	ePin* GetSecondaryPin2()	{ return &m_ePins[3]; }

	double GetRatio() const;
	void SetRatio(double ratio);
	double GetInductance1() const;
	double GetInductance2() const;
	void SetInductance1(double inductance);
	void SetInductance2(double inductance);
	
	virtual void Reset() override { m_I1 = 0.0; m_I2 = 0.0; }
};


class eTransformerWithMiddlePin : public eElement
{
	eTransformer m_T[2];
	eNode* m_Nodes[2];
	eResistor m_Wire;

	friend class TransformerWithMiddlePin;

public:
	eTransformerWithMiddlePin(Circuit& circ, double L1, double L2);
	virtual void InitMatrix(CircuitMtx& mtx) override;
	virtual void Stamp(CircuitMtx& mtx, eNode* GndNode, double dt) override;
	virtual void Update(CircuitMtx& mtx, double dt) override;
	virtual void Reset() override;
	virtual ePin* GetEpin(int num) override;
};


class eKPTSH : public eElement
{
	friend class KPTSH;

public:
	enum KPTSH_TYPE {
		_5 = 5, 
		_7 = 7, 
	};

private:

	static constexpr double KPTSH_5_DURATION = 1.6;
	static constexpr double KPTSH_7_DURATION = 1.86;

	eRelayContactsGroup m_Z;
	eRelayContactsGroup m_J;
	eRelayContactsGroup m_KJ;
	eResistor m_R;
	KPTSH_TYPE m_Type;
	double m_Timer = 0.0;

public:

	eKPTSH(Circuit& circ, int type);
	~eKPTSH();

	virtual ePin* GetEpin(int num) override;
	virtual void Stamp(CircuitMtx& mtx, eNode* GndNode, double dt) override;
	virtual void Update(CircuitMtx& mtx, double dt) override;
	virtual void Reset() override { m_Timer = 0.0; }
	int GetType() const { return m_Type; }
	void SetType(int type);
};


class eZBF : public eElement
{
	eResistor m_R[2];
public:

	eZBF();
	~eZBF();
	virtual ePin* GetEpin(int num) override;
	virtual void Stamp(CircuitMtx& mtx, eNode* GndNode, double dt) override;
};


class eButton : public eElement
{
public:
	enum ButtonState : int
	{
		Pressed = 1,
		Released = 0,
	};

	enum NormalState : u8
	{
		NormalOpen,
		NormalClosed,
	};

private:

	static const inline double OnResistance = 1e-3;
	static const inline double OffResistance = 1e9;

	ButtonState m_State;
	NormalState m_NormState;
	eResistor	m_R;

	void SetState(ButtonState state);

public:

	eButton(NormalState state);

	virtual ePin* GetEpin(int num) override;
	virtual void Stamp(CircuitMtx& mtx, eNode* GndNode, double dt) override;

	void SetNormalState(NormalState state);
	void Press()	{ SetState(Pressed); }
	void Release()	{ SetState(Released); }
};











