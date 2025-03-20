#include "Scheme.h"


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 											Pin



ePin::ePin(eElement* parent) { m_Element = parent; }
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


void eNode::clear()
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
		pin.SetParentElement(this); // Set parent this for each pin created by this element
}


ePin* eElement::GetNextPin(ePin* pin)
{
	for (int i = 0; i < m_ePins.size(); i++)
	{
		if (&m_ePins[i] == pin)
		{
			if (i + 1 < m_ePins.size())
				return &m_ePins[i + 1];
			else
				return nullptr;
		}
	}
	return nullptr;
}


ePin* eElement::GetEpin(int num)
{
	if (num >= m_ePins.size())
		return nullptr;

	return &m_ePins[num];
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

	m_ePins[num] = pin;
}


void eElement::SetStep(double step)
{
	m_step = step;
}


double eElement::GetStep()
{
	return m_step;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 											Resistor



eResistor::eResistor(double resistance)
	: m_Resistance(resistance)
{
	SetNumEpins(2);
}


void eResistor::Stamp(CircuitMtx& mtx, eNode* GndNode)
{
	// Node i           Node j
	// *--------(R)--------*
	//        
	//       i   j 
	// i |   G  -G |
	// j |  -G   G |

	// Node i          
	// *--------(R)-----(GND)
	// 
	//     i
	// i | G |
	// 

	eNode* node1 = GetEpin(0)->GetConnectedNode();
	eNode* node2 = GetEpin(1)->GetConnectedNode();

	if (!node1 || !node2 || node1 == node2)
		return;

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
	else
	{

	}
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 											Voltage Source



eVoltageSource::eVoltageSource(double voltage)
{
	m_Voltage = voltage;
	SetNumEpins(2);
}

void eVoltageSource::Stamp(CircuitMtx& mtx, eNode* GndNode)
{
	// Node i           Node j
 	// *--------(v)--------*
	//		 i   j   new
	// i   |          1 |    |   |
	// j   |         -1 |    |   |
	// new | 1   -1   0 |    | E |
	//                      RHS Vec

	// Node i
	// *--------(v)--------(Gnd) 
	//       i   .  new 
	// i   |         1 |    |   |
	// .   |         0 |    |   |
	// new | 1   0   0 |    | E |
    //

	eNode* n1 = GetPositivePin()->GetConnectedNode();
	eNode* n2 = GetNegativePin()->GetConnectedNode();

	if (!n1 || !n2)
		return;

	size_t numNodes = mtx.GetNumNodes();
	size_t eqIdx = numNodes;

	mtx.Resize(numNodes + 1);

	Eigen::MatrixXd& A = mtx.GetMatrix();
	Eigen::VectorXd& b = mtx.GetVector();

	size_t idx1 = n1->GetIndex();
	size_t idx2 = n2->GetIndex();
	size_t gndIdx = GndNode->GetIndex();

	size_t i = (idx1 > gndIdx) ? idx1 - 1 : idx1;
	size_t j = (idx2 > gndIdx) ? idx2 - 1 : idx2;

	if (n1 != GndNode && n2 != GndNode)
	{
		A(eqIdx, i) = 1;
		A(eqIdx, j) = -1;
		A(i, eqIdx) = 1;
		A(j, eqIdx) = -1;
	}
	else if (n1 == GndNode && n2 != GndNode)
	{
		A(eqIdx, j) = -1;
		A(j, eqIdx) = -1;
	}
	else if (n2 == GndNode && n1 != GndNode)
	{
		A(eqIdx, i) = 1;
		A(i, eqIdx) = 1;
	}
	else
	{

	}

	b(eqIdx) = m_Voltage;
}

