#include "components.h"
#include "circuit.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 											Pin


ePin::~ePin() { ReleaseNode(); }

bool		ePin::IsConnectedToNode()						{ return (m_Enode != nullptr); }
bool		ePin::HasParentElement()						{ return (m_Element != nullptr); }
void		ePin::SetParentElement(eElement* element)		{ m_Element = element; }
eElement*	ePin::GetParentElement()						{ return m_Element; }
eNode*		ePin::GetConnectedNode()						{ return m_Enode; }


void ePin::ConnectToNode(eNode* enode)
{
	if (m_Enode)
		m_Enode->RemoveEpin(this);

	m_Enode = enode;
	if (m_Enode)
		m_Enode->AddEpin(this);
}


void ePin::ReleaseNode()
{
	if (m_Enode)
		m_Enode->RemoveEpin(this);

	m_Enode = nullptr;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 											Node


eNode::eNode(u64 index)
	: m_NodeIndex(index)
{ }


eNode::~eNode()
{
	ReleaseAllPins();
}


void eNode::AddEpin(ePin* epin)
{
	if (!m_ePins.contains(epin))
		m_ePins.insert(epin);
}


void eNode::RemoveEpin(ePin* epin)
{
	if (m_ePins.contains(epin))
		m_ePins.erase(epin);
}

void eNode::ReleaseAllPins()
{
	for (ePin* pin : m_ePins)
		pin->ReleaseNode();
	m_ePins.clear();
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 											Element


eElement::~eElement()
{
	for (ePin& pin : m_ePins)
	{
		if (pin.IsConnectedToNode())
			pin.ReleaseNode();
	}
}


void eElement::SetNumEpins(int n)
{
	m_ePins.resize(n);

	for (ePin& pin : m_ePins)
		pin.SetParentElement(this); // Set parent for each pin created by this element
}


ePin* eElement::GetEpin(int num)
{
	if (num >= m_ePins.size())
		return nullptr;

	return &m_ePins[num];
}


const char* eElement::GetTypeName()
{
	switch (GetType())
	{
	case ty_Resistor:		return "Resistor";
	case ty_Capacitor:		return "Capacitor";
	case ty_Inductor:		return "Inductor";
	case ty_VoltageSource:	return "Voltage Source";
	case ty_Diode:			return "Diode";
	case ty_Potentiometer:	return "Potentiometer";
	case ty_Button:			return "Button";
	case ty_Transformer:    return "Transformer";
	case ty_RelayContact:   return "Relay Contact";
	default:				return "Unknown";
	}
}


void eElement::ReleaseConnectedNodes()
{
	for (ePin& pin : m_ePins)
	{
		if (pin.IsConnectedToNode())
			pin.ReleaseNode();
	}
}


void eElement::SetEpin(int num, ePin pin)
{
	if (num >= m_ePins.size())
		return;

	if (m_ePins[num].IsConnectedToNode())
		m_ePins[num].ReleaseNode();

	m_ePins[num] = pin;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 											Resistor


eResistor::eResistor(double resistance)
	: m_Resistance(resistance)
{
	SetNumEpins(2);
}


void eResistor::Stamp(CircuitMtx& mtx, eNode* GndNode, double dt)
{
	// Node i           Node j
	// *--------(R)--------*
	//        
	//       i   j 
	// i |   G  -G |
	// j |  -G   G |
	// G = 1 / R
	
	// Node i          
	// *--------(R)-----(GND)
	// 
	//     i
	// i | G |
	// G = 1 / R

	eNode* node1 = GetEpin(0)->GetConnectedNode();
	eNode* node2 = GetEpin(1)->GetConnectedNode();

	if (!node1 || !node2 || node1 == node2)
		return;

	if (m_Resistance == 0.0)
		m_Resistance = 1e-9;

	double G = 1.0 / m_Resistance;

	Eigen::MatrixXd& A = mtx.GetMatrix();
	Eigen::VectorXd& b = mtx.GetVector();

	u64 idx1 = node1->GetIndex();
	u64 idx2 = node2->GetIndex();
	u64 GndIdx = GndNode->GetIndex();

	u64 i = (idx1 > GndIdx) ? idx1 - 1 : idx1;
	u64 j = (idx2 > GndIdx) ? idx2 - 1 : idx2;

	if (node1 != GndNode && node2 != GndNode)
	{
		A(i, i) += G;
		A(j, j) += G;
		A(j, i) -= G;
		A(i, j) -= G;
	}
	else if (node1 == GndNode && node2 != GndNode)
	{
		A(j, j) += G;
	}
	else if (node2 == GndNode && node1 != GndNode)
	{
		A(i, i) += G;
	}
}


double eResistor::GetCurrent()
{
	if (m_ePins[0].IsConnectedToNode() && m_ePins[1].IsConnectedToNode())
	{
		double v1 = m_ePins[0].GetVoltage();
		double v2 = m_ePins[1].GetVoltage();
		double vd = v1 - v2;

		double current = vd / m_Resistance;
		
		//if (IsAlmostEqual(v1, v2, 0.0001))
		//	return current;

		//if (vd > 0.0)
		//	return current;
		//else
		//	return -current;

		return current;
	}
	return 0.0;
}


double eResistor::GetVoltDrop()
{
	if (m_ePins[0].IsConnectedToNode() && m_ePins[1].IsConnectedToNode())
	{
		double v1 = m_ePins[0].GetVoltage();
		double v2 = m_ePins[1].GetVoltage();
		return v1 - v2;
	}
	return 0.0;
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 											Potentiometer




ePotentiometer::ePotentiometer(Circuit& circuit, double TotalResistance, double SliderPosition)
	: m_R1(1.0)
	, m_R2(1.0)
	, m_RWire(1e-6)
	, m_Circuit(&circuit)
{
	m_MiddleNode = circuit.CreateNode(); // Circuit owns and manages all the nodes

	m_R1.GetEpin(1)->ConnectToNode(m_MiddleNode);
	m_R2.GetEpin(0)->ConnectToNode(m_MiddleNode);
	m_RWire.GetEpin(0)->ConnectToNode(m_MiddleNode);

	SetTotalResistance(TotalResistance);
	SetSliderPosition(SliderPosition);
}

ePotentiometer::~ePotentiometer()
{
	m_Circuit->RemoveNode(m_MiddleNode);
}


void ePotentiometer::Stamp(CircuitMtx& mtx, eNode* GndNode, double dt)
{
	m_R1.Stamp(mtx, GndNode, dt);
	m_R2.Stamp(mtx, GndNode, dt);
	m_RWire.Stamp(mtx, GndNode, dt);
}


ePin* ePotentiometer::GetEpin(int num)
{
	if (num >= 3)
		return nullptr;

	return  num == 0 ? m_R1.GetEpin(0) : 
			num == 1 ? m_R1.GetEpin(1) : 
			num == 2 ? m_RWire.GetEpin(1) : nullptr;
}


void ePotentiometer::SetSliderPosition(double position)
{
	if (position < 0.0) position = 0.0;
	if (position > 1.0) position = 1.0;

	m_SliderPosition = position;

	double min = 1e-9;
	double r1 = m_TotalResistance * m_SliderPosition;
	double r2 = m_TotalResistance * (1.0 - m_SliderPosition);

	m_R1.SetResistance(r1 < min ? min : r1);
	m_R2.SetResistance(r2 < min ? min : r2);
}


double	ePotentiometer::GetSliderPosition() const	{ return m_SliderPosition; }
double	ePotentiometer::GetTotalResistance() const	{ return m_TotalResistance; }

ePin*	ePotentiometer::GetLeftPin()				{ return m_R1.GetEpin(0); }
ePin*	ePotentiometer::GetRightPin()				{ return m_R2.GetEpin(1); }
ePin*	ePotentiometer::GetMiddlePin()				{ return m_RWire.GetEpin(1); }


void ePotentiometer::SetTotalResistance(double resistance)
{
	m_TotalResistance = resistance;
	m_R1.SetResistance(m_TotalResistance * m_SliderPosition);
	m_R2.SetResistance(m_TotalResistance * (1.0 - m_SliderPosition));
}


double ePotentiometer::GetOutputVoltage()
{
	return m_R1.GetEpin(1)->GetConnectedNode()->GetVoltage();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 											Voltage Source


eVoltageSource::eVoltageSource(double amplitude, double frequency, double phase)
	: m_Amplitude(amplitude)
	, m_Frequency(frequency)
	, m_Phase(phase)
	, m_Time(0.0) 
{
	SetNumEpins(2);
}


double eVoltageSource::GetVoltage(double t) const
{
	if (m_Frequency == 0.0)
	{
		return m_Amplitude;
	}
	else 
	{
		return m_Amplitude * std::sin(2.0 * M_PI * m_Frequency * t + m_Phase);
	}
}


void eVoltageSource::InitMatrix(CircuitMtx& mtx)
{
	u64 numNodes = mtx.GetNumNodes();
	m_eqIndex = numNodes;
	mtx.Resize(numNodes + 1);
}

double eVoltageSource::GetCurrent()
{
	return m_Current;
}

double eVoltageSource::GetVoltDrop()
{
	if (m_ePins[0].IsConnectedToNode() && m_ePins[1].IsConnectedToNode())
	{
		double v1 = GetPositivePin()->GetVoltage();
		double v2 = GetNegativePin()->GetVoltage();
		return v1 - v2;
	}
	return 0.0;
}


void eVoltageSource::Stamp(CircuitMtx& mtx, eNode* GndNode, double dt)
{
	// Node i           Node j
	// *------(Vs)------*
	//      +     -
	// 
	//        i   j   IVs
	// i  |   0   0   1  |
	// j  |   0   0  -1  |
	// IVs|   1  -1   0  | b(IVs) = Vsource

	// Node i          
	// *------(Vs)-----(GND)
	//      +     -
	// 
	//       i   IVs
	// i  |  0   1  |
	// IVs|  1   0  | b(IVs) = Vsource

	eNode* n1 = GetEpin(0)->GetConnectedNode();
	eNode* n2 = GetEpin(1)->GetConnectedNode();
	if (!n1 || !n2)
		return;

	Eigen::MatrixXd& A = mtx.GetMatrix();
	Eigen::VectorXd& b = mtx.GetVector();

	u64 idx1 = n1->GetIndex();
	u64 idx2 = n2->GetIndex();
	u64 gndIdx = GndNode->GetIndex();
	u64 eqIdx = m_eqIndex;

	u64 i = (idx1 > gndIdx) ? idx1 - 1 : idx1;
	u64 j = (idx2 > gndIdx) ? idx2 - 1 : idx2;

	if (n1 != GndNode && n2 != GndNode) 
	{
		A(eqIdx, i) = 1.0;
		A(eqIdx, j) = -1.0;
		A(i, eqIdx) = 1.0;
		A(j, eqIdx) = -1.0;
	}
	else if (n1 == GndNode && n2 != GndNode)
	{
		A(eqIdx, j) = -1.0;
		A(j, eqIdx) = -1.0;
	}
	else if (n2 == GndNode && n1 != GndNode) 
	{
		A(eqIdx, i) = 1.0;
		A(i, eqIdx) = 1.0;
	}

	b(eqIdx) = GetVoltage(m_Time);
}


void eVoltageSource::Update(CircuitMtx& mtx, double dt)
{
	m_Time += dt;
	m_Current = mtx.GetSolution()(m_eqIndex);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 											Capacitor


eCapacitor::eCapacitor(double capacitance)
	: m_Capacitance(capacitance)
{
	SetNumEpins(2);
}


void eCapacitor::Stamp(CircuitMtx& mtx, eNode* GndNode, double dt)
{
	// Node i           Node j
	// *--------(C)--------*
	//        
	//       i   j 
	// i |   G  -G |  b(i) += Ieq
	// j |  -G   G |  b(j) -= Ieq
	// G = 2C/Δt, 
	// Ieq = G * V(t-Δt)

	// Node i          
	// *--------(C)-----(GND)
	// 
	//     i
	// i | G |  b(i) += Ieq
	//

	eNode* node1 = GetEpin(0)->GetConnectedNode();
	eNode* node2 = GetEpin(1)->GetConnectedNode();

	if (!node1 || !node2 || node1 == node2)
		return;

	double G = 2.0 * m_Capacitance / dt;
	double prevVoltage = m_PrevVoltage;

	Eigen::MatrixXd& A = mtx.GetMatrix();
	Eigen::VectorXd& b = mtx.GetVector();

	u64 idx1 = node1->GetIndex();
	u64 idx2 = node2->GetIndex();
	u64 GndIdx = GndNode->GetIndex();

	u64 i = (idx1 > GndIdx) ? idx1 - 1 : idx1;
	u64 j = (idx2 > GndIdx) ? idx2 - 1 : idx2;

	if (node1 != GndNode && node2 != GndNode)
	{
		A(i, i) += G;
		A(j, j) += G;
		A(i, j) -= G;
		A(j, i) -= G;

		double Ieq = G * prevVoltage;
		b(i) += Ieq;
		b(j) -= Ieq;
	}
	else if (node1 == GndNode && node2 != GndNode)
	{
		A(j, j) += G;
		b(j) -= G * prevVoltage;
	}
	else if (node2 == GndNode && node1 != GndNode)
	{
		A(i, i) += G;
		b(i) += G * prevVoltage;
	}
}


void eCapacitor::Update(CircuitMtx& mtx, double dt)
{
	double v_now = GetVoltDrop();
	m_Current = m_Capacitance * (v_now - m_PrevVoltage) / dt;
	m_PrevVoltage = v_now;
}

void eCapacitor::Reset()
{
	m_PrevVoltage = 0.0;
	m_Current = 0.0;
}

double eCapacitor::GetCurrent()
{
	return m_Current;
}

double eCapacitor::GetVoltDrop()
{
	if (m_ePins[0].IsConnectedToNode() && m_ePins[1].IsConnectedToNode())
	{
		double v1 = m_ePins[0].GetVoltage();
		double v2 = m_ePins[1].GetVoltage();
		return v1 - v2;
	}
	return 0.0;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 											Diode


#if DIODE_VER == 1

eDiode::eDiode(double Vt, double Is)
	: m_Vt(Vt)
	, m_Is(Is)
{
	SetNumEpins(2);
}


void eDiode::Stamp(CircuitMtx& mtx, eNode* GndNode, double dt)
{
	// Node i         Node j
	// *-------|>|--------*
	//        
	//       i  j
	// i  |  G -G |   b(i) -= Ieq
	// j  | -G  G |   b(j) += Ieq  
	//  // Conductance G = dI/dV
	// 

	// Node i          
	// *----|>|----(GND)
	// 
	//     i  
	// i | G |  b(i) -= Ieq  

	// Shokley diode model  
	// G = (Is / Vt) * exp(Vd / Vt)  // conductivity
	// Ieq = (Is * (exp(Vd / Vt) - 1)) - (G * Vd)  // equivalent current source

	// Is - saturation current
	// Vt - thermal voltage
	// Vd - voltage drop

	eNode* node1 = GetAnodePin()->GetConnectedNode();
	eNode* node2 = GetCathodePin()->GetConnectedNode();
	if (!node1 || !node2 || node1 == node2)
		return;

	Eigen::MatrixXd& A = mtx.GetMatrix();
	Eigen::VectorXd& b = mtx.GetVector();

	u64 idx1 = node1->GetIndex();
	u64 idx2 = node2->GetIndex();
	u64 GndIdx = GndNode->GetIndex();

	u64 i = (idx1 > GndIdx) ? idx1 - 1 : idx1;
	u64 j = (idx2 > GndIdx) ? idx2 - 1 : idx2;

	double Vd = GetVoltDrop();
	double G = (m_Is / m_Vt) * std::exp(Vd / m_Vt);
	m_G = std::max(G, 1e-12);
	double Ieq = (m_Is * (std::exp(Vd / m_Vt) - 1.0)) - (m_G * Vd);
	m_Ieq = Ieq;

	if (node1 != GndNode && node2 != GndNode)
	{
		A(i, i) += G;
		A(j, j) += G;
		A(i, j) -= G;
		A(j, i) -= G;
		b(i) -= Ieq;
		b(j) += Ieq;
	}
	else if (node1 == GndNode && node2 != GndNode)
	{
		A(j, j) += G;
		b(j) += Ieq;
	}
	else if (node2 == GndNode && node1 != GndNode)
	{
		A(i, i) += G;
		b(i) -= Ieq;
	}
}


void eDiode::Update(CircuitMtx& mtx, double dt)
{
	double Vd = GetVoltDrop();
	m_G = std::max((m_Is / m_Vt) * std::exp(Vd / m_Vt), 1e-12);
	m_Ieq = (m_Is * (std::exp(Vd / m_Vt) - 1.0)) - (m_G * Vd);
}


double eDiode::GetVoltDrop()
{
	ePin* anode = GetAnodePin();
	ePin* cathode = GetCathodePin();

	if (anode->IsConnectedToNode() && cathode->IsConnectedToNode())
	{
		double v1 = anode->GetVoltage();
		double v2 = cathode->GetVoltage();
		return v1 - v2;
	}
	return 0.0;
}


#elif DIODE_VER == 2


eDiode::eDiode(double Vt, double Is, double ZVoltage)
	: m_Vt(Vt)
	, m_Is(Is)
	, m_ZVoltage(ZVoltage)
	, m_LastVd(0)
	, m_G(0)
	, m_Ieq(0)
{
	SetNumEpins(2);
	SetupCriticalVoltages();
}


void eDiode::Stamp(CircuitMtx& mtx, eNode* GndNode, double dt)
{
	eNode* node1 = GetAnodePin()->GetConnectedNode();
	eNode* node2 = GetCathodePin()->GetConnectedNode();
	if (!node1 || !node2 || node1 == node2)
		return;

	Eigen::MatrixXd& A = mtx.GetMatrix();
	Eigen::VectorXd& b = mtx.GetVector();

	u64 idx1 = node1->GetIndex();
	u64 idx2 = node2->GetIndex();
	u64 GndIdx = GndNode->GetIndex();

	u64 i = (idx1 > GndIdx) ? idx1 - 1 : idx1;
	u64 j = (idx2 > GndIdx) ? idx2 - 1 : idx2;

	if (node1 != GndNode && node2 != GndNode) 
	{
		A(i, i) += m_G;
		A(j, j) += m_G;
		A(i, j) -= m_G;
		A(j, i) -= m_G;
		b(i) -= m_Ieq;
		b(j) += m_Ieq;
	}
	else if (node1 == GndNode && node2 != GndNode)
	{
		A(j, j) += m_G;
		b(j) += m_Ieq;
	}
	else if (node2 == GndNode && node1 != GndNode)
	{
		A(i, i) += m_G;
		b(i) -= m_Ieq;
	}
}


void eDiode::Update(CircuitMtx& mtx, double dt)
{
	double Vd = GetVoltDrop();
	Vd = LimitVoltStep(Vd, m_LastVd);
	m_LastVd = Vd;
	double Gmin = m_Is * 0.01; 


	if (Vd >= 0 || m_ZVoltage == 0)  	// regular diode or forward-biased zener
	{
		double expVd = std::exp(Vd * m_Vdcoef);
		//m_G = m_Vdcoef * m_Is * expVd + Gmin;
		m_G = 0.7 * m_G + 0.3 * (m_Vdcoef * m_Is * expVd + Gmin);
		m_Ieq = (expVd - 1) * m_Is - m_G * Vd;
	}
	else
	{
		double expFwd = std::exp(Vd * m_Vdcoef);
		double expRev = std::exp((-Vd - m_ZOffset) * m_Vzcoef);
		m_G = m_Is * (m_Vdcoef * expFwd + m_Vzcoef * expRev) + Gmin;
		m_Ieq = m_Is * (expFwd - expRev - 1) - m_G * Vd;
	}
}


double eDiode::GetVoltDrop()
{
	ePin* anode = GetAnodePin();
	ePin* cathode = GetCathodePin();
	if (anode->IsConnectedToNode() && cathode->IsConnectedToNode()) 
	{
		double v1 = anode->GetVoltage();
		double v2 = cathode->GetVoltage();
		return v1 - v2;
	}
	return 0.0;
}


void eDiode::SetupCriticalVoltages()
{
	m_Vscale = m_Vt;
	m_Vdcoef = 1.0 / m_Vscale;
	m_Vzcoef = 1.0 / m_Vt;
	m_Vcrit = m_Vscale * std::log(m_Vscale / (std::sqrt(2) * m_Is));
	m_Vzcrit = m_Vt * std::log(m_Vt / (std::sqrt(2) * m_Is));
	if (m_ZVoltage == 0)
	{
		m_ZOffset = 0;
	}
	else
	{
		double i = -0.005;	// 5mA offset at Zener voltage
		m_ZOffset = m_ZVoltage - std::log(-(1 + i / m_Is)) / m_Vzcoef;
	}
}


double eDiode::LimitVoltStep(double Vnew, double Vold)
{
	double arg;

	if (Vnew > m_Vcrit && std::abs(Vnew - Vold) > (m_Vscale + m_Vscale))	// Forward bias limiting
	{
		if (Vold > 0)
		{
			arg = 1 + (Vnew - Vold) / m_Vscale;
			if (arg > 0)
				Vnew = Vold + m_Vscale * std::log(arg);
			else
				Vnew = m_Vcrit;
		}
		else
		{
			Vnew = m_Vscale * std::log(Vnew / m_Vscale);
		}
	}
	else if (Vnew < 0 && m_ZOffset != 0) // Zener breakdown limiting
	{
		Vnew = -Vnew - m_ZOffset;
		Vold = -Vold - m_ZOffset;

		if (Vnew > m_Vzcrit && std::abs(Vnew - Vold) > (m_Vt + m_Vt))
		{
			if (Vold > 0)
			{
				arg = 1 + (Vnew - Vold) / m_Vt;
				if (arg > 0)
					Vnew = Vold + m_Vt * std::log(arg);
				else
					Vnew = m_Vzcrit;

			}
			else
			{
				Vnew = m_Vt * std::log(Vnew / m_Vt);
			}
		}
		Vnew = -(Vnew + m_ZOffset);
	}
	return Vnew;
}

#elif DIODE_VER == 3

#endif


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 											eDiodeBridge



eDiodeBridge::eDiodeBridge(Circuit& circuit)
	: m_Wires{ { 1e-3 },{ 1e-3 },{ 1e-3 },{ 1e-3 } }
	, m_InnerNodes{ circuit.CreateNode(), circuit.CreateNode(), circuit.CreateNode(), circuit.CreateNode() }
	, m_Circuit(&circuit)
{
	auto& d0 = m_Diodes[0];
	auto& d1 = m_Diodes[1];
	auto& d2 = m_Diodes[2];
	auto& d3 = m_Diodes[3];

	auto& wire0 = m_Wires[0];
	auto& wire1 = m_Wires[1];
	auto& wire2 = m_Wires[2];
	auto& wire3 = m_Wires[3];

	auto& n0 = m_InnerNodes[0];
	auto& n1 = m_InnerNodes[1];
	auto& n2 = m_InnerNodes[2];
	auto& n3 = m_InnerNodes[3];

	wire0.GetEpin(1)->ConnectToNode(n0);
	wire1.GetEpin(1)->ConnectToNode(n1);
	wire2.GetEpin(1)->ConnectToNode(n2);
	wire3.GetEpin(1)->ConnectToNode(n3);

	d0.GetCathodePin()->ConnectToNode(n0);
	d0.GetAnodePin()->ConnectToNode(n2);
	d2.GetAnodePin()->ConnectToNode(n2);
	d2.GetCathodePin()->ConnectToNode(n1);

	d1.GetAnodePin()->ConnectToNode(n0);
	d1.GetCathodePin()->ConnectToNode(n3);
	d3.GetCathodePin()->ConnectToNode(n3);
	d3.GetAnodePin()->ConnectToNode(n1);


}

eDiodeBridge::~eDiodeBridge()
{
	for (auto& n : m_InnerNodes)
	{
		m_Circuit->RemoveNode(n);
	}
}

void eDiodeBridge::Stamp(CircuitMtx& mtx, eNode* GndNode, double dt)
{
	for (size_t i = 0; i < 4; i++)
	{
		m_Diodes[i].Stamp(mtx, GndNode, dt);
		m_Wires[i].Stamp(mtx, GndNode, dt);
	}
}

void eDiodeBridge::Update(CircuitMtx& mtx, double dt)
{
	for (auto& d : m_Diodes)
	{
		d.Update(mtx, dt);
	}
}

void eDiodeBridge::Reset()
{
	std::ranges::for_each(m_Diodes, std::mem_fn(&eDiode::Reset));
}

ePin* eDiodeBridge::GetEpin(int num)
{
	auto& wire0 = m_Wires[0];
	auto& wire1 = m_Wires[1];
	auto& wire2 = m_Wires[2];
	auto& wire3 = m_Wires[3];

	PinIndex n = PinIndex(num);
	switch (n)
	{
	case eDiodeBridge::In0: return wire0.GetEpin(0);
	case eDiodeBridge::In1: return wire1.GetEpin(0);
	case eDiodeBridge::OutMinus: return wire2.GetEpin(0);
	case eDiodeBridge::OutPlus: return wire3.GetEpin(0);
	default:
		break;
	}
	SM_ASSERT(false, vfmt("eDiodeBridge::GetEpin({}) -> Invalid pin index", num));
	std::unreachable();
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 											Inductor


eInductor::eInductor(double inductance)
	: m_Inductance(inductance)
{
	SetNumEpins(2);
}


void eInductor::Stamp(CircuitMtx& mtx, eNode* GndNode, double dt)
{
	// Node i           Node j
	// *--------(L)--------*
	//        
	//        i   j   IL
	// i  |   0   0   1    |
	// j  |   0   0  -1    |
	// IL |   1  -1  -L/Δt | b(IL) -= (L/Δt) * I(t-Δt)

	// Node i          
	// *--------(L)-----(GND)
	// 
	//       i   IL
	// i  |  0   1     |
	// IL |  1   -L/Δt | b(IL) -= (L/Δt) * I(t-Δt)

	eNode* node1 = GetEpin(0)->GetConnectedNode();
	eNode* node2 = GetEpin(1)->GetConnectedNode();

	if (!node1 || !node2 || node1 == node2)
		return;

	Eigen::MatrixXd& A = mtx.GetMatrix();
	Eigen::VectorXd& b = mtx.GetVector();

	u64 idx1 = node1->GetIndex();
	u64 idx2 = node2->GetIndex();
	u64 GndIdx = GndNode->GetIndex();
	u64 currentIdx = m_CurrenIndex;

	u64 i = (idx1 > GndIdx) ? idx1 - 1 : idx1;
	u64 j = (idx2 > GndIdx) ? idx2 - 1 : idx2;


	if (node1 != GndNode)
	{
		A(i, currentIdx) += 1.0;
		A(currentIdx, i) += 1.0;
	}
	if (node2 != GndNode)
	{
		A(j, currentIdx) -= 1.0;
		A(currentIdx, j) -= 1.0;
	}

	A(currentIdx, currentIdx) -= m_Inductance / dt;			// dI/dt = (I(t) - I(t-Δt))/Δt
	b(currentIdx) -= (m_Inductance / dt) * m_prevCurrent;	// b(IL) -= (L/Δt) * I(t-Δt)
}


void eInductor::Update(CircuitMtx& mtx, double dt)
{
	ePin& pin0 = m_ePins[0];
	ePin& pin1 = m_ePins[1];

	if (pin0.IsConnectedToNode() && pin1.IsConnectedToNode())
	{
		m_prevCurrent = mtx.GetSolution()(m_CurrenIndex);
	}
}


void eInductor::InitMatrix(CircuitMtx& mtx)
{
	u64 numNodes = mtx.GetNumNodes();
	m_CurrenIndex = numNodes;
	mtx.Resize(numNodes + 1);
}


double eInductor::GetVoltDrop()
{
	ePin& pin0 = m_ePins[0];
	ePin& pin1 = m_ePins[1];

	if (pin0.IsConnectedToNode() && pin1.IsConnectedToNode())
	{
		double v1 = pin0.GetVoltage();
		double v2 = pin1.GetVoltage();
		return v1 - v2;
	}
	return 0.0;
}


double eInductor::GetCurrent()
{
	return m_prevCurrent;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 											eTransformer


eTransformer::eTransformer(double Inductance1, double ratio) 
	: eTransformer(Inductance1, Inductance1 * std::pow(ratio, 2), 0.997)
{ }

eTransformer::eTransformer(double Inductance1, double Inductance2, double transformCoef)
	: m_L1(Inductance1)
	, m_L2(Inductance2)
	, m_CouplCoef(transformCoef)
	, m_I1(0.0)
	, m_I2(0.0)
{   //
 	// 0 ───┐   ┌─── 2
	//     *) │ (*
	// 	    ) │ ( 
	//	    ) │ ( 
	// 1 ───┘   └─── 3

	SetNumEpins(4);

	m_CouplCoef = std::clamp(m_CouplCoef, 0.0, 1.0);
	m_L1 = std::clamp(m_L1, 1e-6, 1e6);
	m_L2 = std::clamp(m_L2, 1e-6, 1e6);
}


inline void eTransformer::InitMatrix(CircuitMtx& mtx)
{
	u64 numNodes = mtx.GetNumNodes();
	m_I1_Idx = numNodes;
	m_I2_Idx = numNodes + 1;
	mtx.Resize(numNodes + 2);
}


void eTransformer::Stamp(CircuitMtx& mtx, eNode* GndNode, double dt)
{
	eNode* n1 = GetEpin(0)->GetConnectedNode();
	eNode* n2 = GetEpin(1)->GetConnectedNode();
	eNode* n3 = GetEpin(2)->GetConnectedNode();
	eNode* n4 = GetEpin(3)->GetConnectedNode();

	if (!n1 || !n2 || !n3 || !n4)
		return;

	Eigen::MatrixXd& A = mtx.GetMatrix();
	Eigen::VectorXd& b = mtx.GetVector();
	u64 gndIdx = GndNode->GetIndex();

	auto GetNodeIndex = [gndIdx](eNode* node) -> u64
		{
			u64 idx = node->GetIndex();
			return (idx > gndIdx) ? idx - 1 : idx;
		};
	
	u64 i1 = GetNodeIndex(n1);
	u64 i2 = GetNodeIndex(n2);
	u64 i3 = GetNodeIndex(n3);
	u64 i4 = GetNodeIndex(n4);

	double L1 = m_L1;
	double L2 = m_L2;
	double M = m_CouplCoef * std::sqrt(L1 * L2);


	if (n1 != GndNode) 
	{
		A(i1, m_I1_Idx) += 1.0;
		A(m_I1_Idx, i1) += 1.0;
	}
	if (n2 != GndNode) 
	{
		A(i2, m_I1_Idx) -= 1.0;
		A(m_I1_Idx, i2) -= 1.0;
	}


	if (n3 != GndNode) 
	{
		A(i3, m_I2_Idx) +=  1.0;
		A(m_I2_Idx, i3) +=  1.0;
	}
	if (n4 != GndNode)
	{
		A(i4, m_I2_Idx) -=  1.0;
		A(m_I2_Idx, i4) -=  1.0;
	}

	A(m_I1_Idx, m_I1_Idx) -= L1 / dt;
	A(m_I1_Idx, m_I2_Idx) -= M / dt;
	A(m_I2_Idx, m_I1_Idx) -= M / dt;
	A(m_I2_Idx, m_I2_Idx) -= L2 / dt;

	b(m_I1_Idx) -= (L1 * m_I1 + M * m_I2) / dt;
	b(m_I2_Idx) -= (M * m_I1 + L2 * m_I2) / dt;
}


void eTransformer::Update(CircuitMtx& mtx, double dt)
{
	m_I1 = mtx.GetSolution()(m_I1_Idx);
	m_I2 = mtx.GetSolution()(m_I2_Idx);
	
	double v1 = GetEpin(0)->GetVoltage() - GetEpin(1)->GetVoltage();
	double v2 = GetEpin(2)->GetVoltage() - GetEpin(3)->GetVoltage();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 											eButton


eButton::eButton(NormalState NormState)
	: m_R(1.0)
{
	m_NormState = NormState;
	SetState(ButtonState::Released);
}

void eButton::Stamp(CircuitMtx& mtx, eNode* GndNode, double dt)
{
	m_R.Stamp(mtx, GndNode, dt);
}

void eButton::Update(CircuitMtx& mtx, double dt)
{
}

void eButton::SetNormalState(NormalState state)
{
	m_NormState = state;
	SetState(m_State);
}


void eButton::SetState(ButtonState state)
{
	m_State = state;

	if (m_NormState == NormalOpen)
		m_R.SetResistance((state == ButtonState::Pressed) ? OnResistance : OffResistance);
	else
		m_R.SetResistance((state == ButtonState::Pressed) ? OffResistance : OnResistance);
}


ePin* eButton::GetEpin(int num)
{
	return (num == 0) ? m_R.GetEpin(0) : m_R.GetEpin(1);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 											eRelayContactsGroup


eRelayContactsGroup::eRelayContactsGroup(Circuit& circuit)
	: m_R11(1.0f)
	, m_R12(1.0f)
	, m_R13(1.0f)
	, m_MiddleNode(circuit.CreateNode())
	, m_State(n11_n12)
	, m_Circuit(&circuit)
{
	m_R11.GetEpin(1)->ConnectToNode(m_MiddleNode);
	m_R12.GetEpin(0)->ConnectToNode(m_MiddleNode);
	m_R13.GetEpin(0)->ConnectToNode(m_MiddleNode);
	SetState(m_State);
}


eRelayContactsGroup::~eRelayContactsGroup()
{
	m_Circuit->RemoveNode(m_MiddleNode);
}


void eRelayContactsGroup::Stamp(CircuitMtx& mtx, eNode* GndNode, double dt)
{
	m_R11.Stamp(mtx, GndNode, dt);
	m_R12.Stamp(mtx, GndNode, dt);
	m_R13.Stamp(mtx, GndNode, dt);
}


ePin* eRelayContactsGroup::GetEpin(int num)
{
	ePin* pin = nullptr;
	switch (RelayContact(num))
	{
	case eRelayContactsGroup::N11: pin = m_R11.GetEpin(0); break;
	case eRelayContactsGroup::N12: pin = m_R12.GetEpin(1); break;
	case eRelayContactsGroup::N13: pin = m_R13.GetEpin(1); break;
	default:
		break;
	}
	return pin;
}


eRelayContactsGroup::State eRelayContactsGroup::GetState()
{ 
	return m_State; 
}

void eRelayContactsGroup::SetCoilName(const std::string& name)
{
	m_CoilName = name;
	m_HashName = std::hash<std::string>()(name);
}

void eRelayContactsGroup::SetState(State state)
{
	if (state == n11_n12)
	{
		m_R11.SetResistance(minResistance);
		m_R12.SetResistance(minResistance);
		m_R13.SetResistance(maxResistance);
	}
	else
	{
		m_R11.SetResistance(minResistance);
		m_R12.SetResistance(maxResistance);
		m_R13.SetResistance(minResistance);
	}
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 											eCoil


eCoil::eCoil(double Inductance, double ReleaseDelay)
	: eInductor(Inductance)
	, m_ReleaseDelay(ReleaseDelay)
{ }


void eCoil::Update(CircuitMtx& mtx, double dt)
{
	ePin& pin0 = m_ePins[0];
	ePin& pin1 = m_ePins[1];

	if (!pin0.IsConnectedToNode() && !pin1.IsConnectedToNode())
		return;

	double current = mtx.GetSolution()(m_CurrenIndex);		
	m_prevCurrent = current;

	if (std::abs(current) > m_CurrThreshold)
	{
		m_IsActive = true;
		m_InactiveCoilTimer = 0.0;
	}
	else if (m_IsActive)
	{
		m_InactiveCoilTimer += dt;
		if (m_InactiveCoilTimer > m_ReleaseDelay)
		{
			m_IsActive = false;
			m_InactiveCoilTimer = 0.0;
		}
	}
}

void eCoil::SetName(const std::string& name)
{
	m_Name = name;
	m_HashName = std::hash<std::string>()(name);
}


#if vComposeCoil
eCoilWithRectifier::eCoilWithRectifier(Circuit& circ, double Inductance, double ReleaseDelay)
	: r(10)
	, wire(1e-3)
	, m_InnerNodes{ circ.CreateNode(), circ.CreateNode() }
	, m_Circuit(&circ)
{ 
	// --wire--n0----|<|----n1-----r----
	//          |           |
	//          |----l------|
	
	eNode* n0 = m_InnerNodes[0];
	eNode* n1 = m_InnerNodes[1];
	l = m_Circuit->AddElement<eCoil>(Inductance, ReleaseDelay);
	
	d.GetCathodePin()->ConnectToNode(n0);
	wire.GetEpin(0)->ConnectToNode(n0);
	l->GetEpin(0)->ConnectToNode(n0);

	d.GetAnodePin()->ConnectToNode(n1);
	r.GetEpin(0)->ConnectToNode(n1);
	l->GetEpin(1)->ConnectToNode(n1);
}

eCoilWithRectifier::~eCoilWithRectifier()
{
	m_Circuit->RemoveElement(l);
	m_Circuit->RemoveNode(m_InnerNodes[0]);
	m_Circuit->RemoveNode(m_InnerNodes[1]);
}

void eCoilWithRectifier::Stamp(CircuitMtx& mtx, eNode* GndNode, double dt)
{
	l->Stamp(mtx, GndNode, dt);
	d.Stamp(mtx, GndNode, dt);
	r.Stamp(mtx, GndNode, dt);
	wire.Stamp(mtx, GndNode, dt);
}

void eCoilWithRectifier::Update(CircuitMtx& mtx, double dt)
{
	l->Update(mtx, dt);
	d.Update(mtx, dt);
	r.Update(mtx, dt);
	wire.Update(mtx, dt);
}

void eCoilWithRectifier::InitMatrix(CircuitMtx& mtx)
{
	l->InitMatrix(mtx);
	d.InitMatrix(mtx);
	r.InitMatrix(mtx);
	wire.InitMatrix(mtx);
}


void eCoilWithRectifier::Reset()
{
	l->Reset();
	d.Reset();
	r.Reset();
	wire.Reset();
}

ePin* eCoilWithRectifier::GetEpin(int num)
{
	return num == 0 ? wire.GetEpin(1) : r.GetEpin(1);
}

double eCoilWithRectifier::GetCurrent()
{
	return l->GetCurrent();
}

double eCoilWithRectifier::GetVoltDrop()
{
	return l->GetVoltDrop();
}

#else
eCoilWithRectifier::eCoilWithRectifier(Circuit& circ, double Inductance, double ReleaseDelay)
	: /*eCoil*/eInductor(Inductance/*, ReleaseDelay*/)
	, m_InnerNodes{ circ.CreateNode(), circ.CreateNode() }
	, m_Circuit(&circ)
{
#if 0
	// --wire--n0----|<|----n1-----r----
	//          |           |
	//          |----l------|
#endif


	// --wire--n0----l------n1-----r----
	eNode* n0 = m_InnerNodes[0];
	eNode* n1 = m_InnerNodes[1];

	wire = m_Circuit->AddElement<eResistor>(1e-3);
	r = m_Circuit->AddElement<eResistor>(5);
	
	wire->GetEpin(0)->ConnectToNode(n0);
	eInductor::GetEpin(0)->ConnectToNode(n0);

	r->GetEpin(1)->ConnectToNode(n1);
	eInductor::GetEpin(1)->ConnectToNode(n1);

	//d.GetAnodePin()->ConnectToNode(n1);
	//d.GetCathodePin()->ConnectToNode(n0);
}

eCoilWithRectifier::~eCoilWithRectifier()
{
	m_Circuit->RemoveElement(r);
	m_Circuit->RemoveElement(wire);
	m_Circuit->RemoveNode(m_InnerNodes[0]);
	m_Circuit->RemoveNode(m_InnerNodes[1]);
}

ePin* eCoilWithRectifier::GetEpin(int num)
{
	return num == 0 ? r->GetEpin(0) : wire->GetEpin(1);
}


#endif