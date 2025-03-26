#include "components.h"
#include "circuit.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 											Pin


ePin::~ePin() { ReleaseNode(); }

bool ePin::IsConnectedToNode() { return (m_Enode != nullptr); }
bool ePin::HasParentElement() { return (m_Element != nullptr); }
void ePin::SetParentElement(eElement* element) { m_Element = element; }
eElement* ePin::GetParentElement() { return m_Element; }
eNode* ePin::GetConnectedNode() { return m_Enode; }


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


eNode::eNode(size_t index)
	: m_NodeIndex(index)
{ }

eNode::~eNode()
{
	for (ePin* pin : m_ePins)
		pin->ReleaseNode();

	m_ePins.clear();
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
	case ty_Contact:		return "Contact";
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

	size_t idx1 = node1->GetIndex();
	size_t idx2 = node2->GetIndex();
	size_t GndIdx = GndNode->GetIndex();

	size_t i = (idx1 > GndIdx) ? idx1 - 1 : idx1;
	size_t j = (idx2 > GndIdx) ? idx2 - 1 : idx2;

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
		m_Current = (v1 - v2) / m_Resistance;
		return m_Current;
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
{
	m_MiddleNode = circuit.CreateNode(); // Circuit owns and manages all the nodes

	m_R1.GetEpin(1)->ConnectToNode(m_MiddleNode);
	m_R2.GetEpin(0)->ConnectToNode(m_MiddleNode);

	SetTotalResistance(TotalResistance);
	SetSliderPosition(SliderPosition);
}


void ePotentiometer::Stamp(CircuitMtx& mtx, eNode* GndNode, double dt)
{
	m_R1.Stamp(mtx, GndNode, dt);
	m_R2.Stamp(mtx, GndNode, dt);
}



ePin* ePotentiometer::GetEpin(int num)
{
	if (num >= 3)
		return nullptr;

	return num == 0 ? m_R1.GetEpin(0) : num == 1 ? m_R1.GetEpin(1) : num == 2 ? m_R2.GetEpin(1) : nullptr;
}


void ePotentiometer::SetSliderPosition(double position)
{
	if (position < 0.0) position = 0.0;
	if (position > 1.0) position = 1.0;

	m_SliderPosition = position;

	const double min = 1e-9;
	double r1 = m_TotalResistance * m_SliderPosition;
	double r2 = m_TotalResistance * (1.0 - m_SliderPosition);

	m_R1.SetResistance(r1 < min ? min : r1);
	m_R2.SetResistance(r2 < min ? min : r2);
}

double ePotentiometer::GetSliderPosition() const { return m_SliderPosition; }

double ePotentiometer::GetTotalResistance() const { return m_TotalResistance; }

void ePotentiometer::SetTotalResistance(double resistance)
{
	m_TotalResistance = resistance;
	m_R1.SetResistance(m_TotalResistance * m_SliderPosition);
	m_R2.SetResistance(m_TotalResistance * (1.0 - m_SliderPosition));
}

void ePotentiometer::ConnectOutputPinToNode(eNode* node)
{
	m_R1.GetEpin(1)->ConnectToNode(node);
	m_R2.GetEpin(0)->ConnectToNode(node);
}

void ePotentiometer::ReleaseNodeFromOutputPin()
{
	m_R1.GetEpin(1)->ConnectToNode(m_MiddleNode);
	m_R2.GetEpin(0)->ConnectToNode(m_MiddleNode);
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
	size_t numNodes = mtx.GetNumNodes();
	m_eqIndex = numNodes;
	mtx.Resize(numNodes + 1);
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

	size_t idx1 = n1->GetIndex();
	size_t idx2 = n2->GetIndex();
	size_t gndIdx = GndNode->GetIndex();
	size_t eqIdx = m_eqIndex;

	size_t i = (idx1 > gndIdx) ? idx1 - 1 : idx1;
	size_t j = (idx2 > gndIdx) ? idx2 - 1 : idx2;

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

	size_t idx1 = node1->GetIndex();
	size_t idx2 = node2->GetIndex();
	size_t GndIdx = GndNode->GetIndex();

	size_t i = (idx1 > GndIdx) ? idx1 - 1 : idx1;
	size_t j = (idx2 > GndIdx) ? idx2 - 1 : idx2;

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
	if (m_ePins[0].IsConnectedToNode() && m_ePins[1].IsConnectedToNode())
	{
		double v1 = m_ePins[0].GetVoltage();
		double v2 = m_ePins[1].GetVoltage();
		m_PrevVoltage = v1 - v2;
	}
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 											Diode


#if DIODE_VERSION == 1

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

	// https://stemformulas.com/formulas/shockley-diode-model/
	// // Shokley diode model 
	// 
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

	size_t idx1 = node1->GetIndex();
	size_t idx2 = node2->GetIndex();
	size_t GndIdx = GndNode->GetIndex();

	size_t i = (idx1 > GndIdx) ? idx1 - 1 : idx1;
	size_t j = (idx2 > GndIdx) ? idx2 - 1 : idx2;

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


#elif DIODE_VERSION == 2


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

	size_t idx1 = node1->GetIndex();
	size_t idx2 = node2->GetIndex();
	size_t GndIdx = GndNode->GetIndex();

	size_t i = (idx1 > GndIdx) ? idx1 - 1 : idx1;
	size_t j = (idx2 > GndIdx) ? idx2 - 1 : idx2;

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
	Vd = LimitVoltageStep(Vd, m_LastVd);
	m_LastVd = Vd;
	double Gmin = m_Is * 0.01; 

	if (Vd >= 0 || m_ZVoltage == 0)  	// regular diode or forward-biased zener
	{
		double expVd = std::exp(Vd * m_Vdcoef);
		m_G = m_Vdcoef * m_Is * expVd + Gmin;
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


double eDiode::LimitVoltageStep(double Vnew, double Vold)
{
	double arg;
	double origVnew = Vnew;

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

#elif DIODE_VERSION == 3

#endif


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

	size_t idx1 = node1->GetIndex();
	size_t idx2 = node2->GetIndex();
	size_t GndIdx = GndNode->GetIndex();
	size_t currentIdx = m_CurrenIndex;

	size_t i = (idx1 > GndIdx) ? idx1 - 1 : idx1;
	size_t j = (idx2 > GndIdx) ? idx2 - 1 : idx2;


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
		if (pin0.IsConnectedToNode() && pin1.IsConnectedToNode())
		{
			m_prevCurrent = mtx.GetSolution()(m_CurrenIndex);
		}
	}
}


void eInductor::InitMatrix(CircuitMtx& mtx)
{
	size_t numNodes = mtx.GetNumNodes();
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
// 											eSwitchBase


eSwitchBase::~eSwitchBase()
{
	m_switches.clear();
}


void eSwitchBase::InitMatrix(CircuitMtx& mtx)
{
	size_t CountPins = m_switches.size() * 2;
	size_t numNodes = mtx.GetNumNodes();
	mtx.Resize(numNodes + CountPins);
}


void eSwitchBase::Stamp(CircuitMtx& mtx, eNode* GndNode, double dt)
{
	for (eResistor& sw : m_switches)
	{
		if (sw.GetEpin(0)->IsConnectedToNode() && sw.GetEpin(1)->IsConnectedToNode())
			sw.Stamp(mtx, GndNode, dt);
	}
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 											eButton


eButton::eButton(NormalState_e NormState)
{
	m_NormState = NormState;
	SetupSwitches();
}

void eButton::SetupSwitches()
{
	m_switches.push_back(eResistor(m_OnResistance));
	SetState(ButtonState_e::Released);
}


void eButton::SetState(int state)
{
	m_State = ButtonState_e(state);

	if (m_NormState == NormalOpen)
		m_switches[0].SetResistance((state == ButtonState_e::Pressed) ? m_OffResistance : m_OnResistance);
	else
		m_switches[0].SetResistance((state == ButtonState_e::Pressed) ? m_OnResistance : m_OffResistance);
}


ePin* eButton::GetEpin(int num)
{
	if (num >= m_switches.size() * 2)
		return nullptr;

	return m_switches[0].GetEpin(num);
}



