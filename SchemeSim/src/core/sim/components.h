#pragma once
#include <iostream>
#include <vector>
#include <map>
#include <set>
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
	double	GetVoltage() const { return m_Voltage; }
	void	SetVoltage(double Volt) { m_Voltage = IsAlmostEqual(Volt, 0.0) ? 0.0 : Volt; }
	u64		GetIndex() const { return m_NodeIndex; }
	u64		GetNumConnectedPins() const { return m_ePins.size(); }
	void    SetIndex(u64 index) { m_NodeIndex = index; }
	std::set<ePin*>& GetPins() { return m_ePins; }
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


enum ElementType_e : u8
{
	ty_Resistor,
	ty_Capacitor,
	ty_Inductor,
	ty_VoltageSource,
	ty_Diode,
	ty_Potentiometer,
	ty_RelayContact,
	ty_Coil,
	ty_Button,
	ty_Transformer,
	ty_DiodeBridge,
	ty_Unknown = 255,
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
	virtual void InitMatrix(CircuitMtx& mtx) { }						// Called before matrix is assembled
	virtual void Reset() { }
	virtual ePin* GetEpin(int num);
	virtual u64 GetNumEpins()					{ return m_ePins.size(); }
	virtual ElementType_e GetType()				{ return ty_Unknown; }
	virtual double GetCurrent()					{ return 0.0; }
	virtual double GetVoltDrop()				{ return 0.0; }

	const char* GetTypeName();
	void ReleaseConnectedNodes();
};


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


class eResistor : public eElement
{
	friend class Resistor;

	double m_Resistance;

public:

	eResistor(double resistance);
	virtual void Stamp(CircuitMtx& mtx, eNode* GndNode, double dt) override;
	virtual ElementType_e GetType() override { return ty_Resistor; }
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
	virtual ElementType_e GetType() override { return ty_Potentiometer; }
	virtual u64 GetNumEpins() override { return 3; }

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
	virtual ElementType_e GetType() override { return ty_VoltageSource; }
	virtual void Reset() override { m_Time = 0.0; }
	virtual u64 GetNumEpins() override { return 2; }
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
	virtual ElementType_e GetType() override { return ty_Capacitor; }
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
	virtual ElementType_e GetType() override { return ty_Diode; }
	
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
	virtual ElementType_e GetType() override { return ty_DiodeBridge; }
	virtual u64 GetNumEpins() override { return 4; }
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

	double m_Inductance;			// L, H
	double m_prevCurrent = 0.0;

	u64 m_CurrenIndex = 0;

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

	virtual void Reset() override { m_prevCurrent = 0.0; }
};


class eRelayContactsGroup : public eElement
{
	friend class RelayContactsGroup;

public:

	enum State { n11_n12, n11_n13, };
	enum RelayContact { N11, N12, N13, ContactsCount, };

protected:

	static inline const double maxResistance = 1e9;
	static inline const double minResistance = 1e-6;

	// --(r11)----(r12)---
	//         |
	//         |-(r13)--
	//   

	//     11     12
	// -----______-------
	//      
	//           |_______
	//             13

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
	virtual ElementType_e	GetType() override										{ return ty_RelayContact; }
	virtual u64				GetNumEpins() override									{ return 3; }

	void					SetState(State state);
	State					GetState();
	void					SetCoilName(const std::string& name);
	const std::string&		GetCoilName() const										{ return m_CoilName; }
	u64						GetCoilHashName()										{ return m_HashName; }
};


class eCoil : public eInductor
{
	friend class Coil;
	friend class NeutralRelayCoilWithRectifier;
	friend class eCoilWithRectifier;

	std::string m_Name;
	u64		m_HashName = 0;
	double	m_ReleaseDelay; // in seconds
	double  m_CurrThreshold = 0.015;
	double  m_InactiveCoilTimer = 0.0;
	bool	m_IsActive = false;

public:

	eCoil(double Inductance, double ReleaseDelay = 0.0);
	virtual void Update(CircuitMtx& mtx, double dt) override;
	virtual ElementType_e GetType() override { return ty_Coil; }
	
	bool IsActive() { return m_IsActive; }
	void SetName(const std::string& name);
	const std::string& GetName() { return m_Name; }
	u64	GetHashName() { return m_HashName; }
};


class eCoilWithRectifier : public eElement
{
	friend class NeutralRelayCoilWithRectifier;
	eInductor l;
	eDiode d;
	eResistor r;
	eResistor wire;
	eNode* m_InnerNodes[2];
	Circuit* m_Circuit;

public:

	eCoilWithRectifier(Circuit& circ, double Inductance, double ReleaseDelay = 0.0);
	~eCoilWithRectifier();

	virtual void Stamp(CircuitMtx& mtx, eNode* GndNode, double dt) override;
	virtual void Update(CircuitMtx& mtx, double dt) override;
	virtual void InitMatrix(CircuitMtx& mtx) override;
	virtual void Reset() override;
	virtual ePin* GetEpin(int num) override;
	virtual double GetCurrent() override;
	virtual double GetVoltDrop() override;
};


class eTransformer : public eElement
{
	friend class Transformer;

	double m_L1;
	double m_L2;
	double m_CouplCoef;
	double m_I1 = 0.0;
	double m_I2 = 0.0;

	u64 m_I1_Idx = -1;
	u64 m_I2_Idx = -1;

public:

	eTransformer(double Inductance1, double ratio);
	eTransformer(double Inductance1, double Inductance2, double transformCoef);
	virtual void InitMatrix(CircuitMtx& mtx) override;
	virtual void Stamp(CircuitMtx& mtx, eNode* GndNode, double dt) override;
	virtual void Update(CircuitMtx& mtx, double dt) override;

	virtual ElementType_e GetType() override { return ty_Transformer; }
	virtual u64 GetNumEpins() override { return 4; }

	ePin* GetPrimaryPin1()		{ return &m_ePins[0]; }
	ePin* GetPrimaryPin2()		{ return &m_ePins[1]; }
	ePin* GetSecondaryPin1()	{ return &m_ePins[2]; }
	ePin* GetSecondaryPin2()	{ return &m_ePins[3]; }

	virtual void Reset() override { m_I1 = 0.0; m_I2 = 0.0; }
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
	virtual ElementType_e GetType() override { return ty_Button; }
	virtual void Stamp(CircuitMtx& mtx, eNode* GndNode, double dt) override;
	virtual void Update(CircuitMtx& mtx, double dt) override;

	void SetNormalState(NormalState state);
	void Press()	{ SetState(Pressed); }
	void Release()	{ SetState(Released); }
};













