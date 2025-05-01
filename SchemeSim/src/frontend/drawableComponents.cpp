#include "drawableCircuit.h"
#include "drawableComponents.h"


void eDrawableBase::SetRotation(float degrees)
{
	const sf::Angle NewRotation = sf::degrees(degrees);
	for (sf::Vector2f& pos : m_PinPositions)
	{
		sf::Vector2f initialPos = pos.rotatedBy(-m_Rotation);
		pos = initialPos.rotatedBy(NewRotation);
	}

	m_Rotation = NewRotation;
	m_sprite.setRotation(m_Rotation);
}


void eDrawableBase::Flip(flipAxis axis)
{
	for (auto& pos : m_PinPositions)
	{
		sf::Vector2f localPos = pos.rotatedBy(-m_Rotation);

		if (axis == flipAxis::X)
			localPos.x = -localPos.x;
		else
			localPos.y = -localPos.y;

		pos = localPos.rotatedBy(m_Rotation);
	}
	sf::Vector2f scale = m_sprite.getScale();

	m_sprite.setScale({
		axis == flipAxis::X ? -scale.x : scale.x,
		axis == flipAxis::Y ? -scale.y : scale.y });

	if (axis == flipAxis::X)
		m_IsFlipped_X = !m_IsFlipped_X;
	else
		m_IsFlipped_Y = !m_IsFlipped_Y;
}

int eDrawableBase::GetPinIndexFromLocalPosition(sf::Vector2f pos)
{
	for (size_t i = 0; i < m_PinPositions.size(); i++)
	{
		if (pos == m_PinPositions[i])
			return i;
	}
	return 0;
}

//----------------------------------------------------------------------------------------------------------------------------------------
//											Resistor
//
//----------------------------------------------------------------------------------------------------------------------------------------



Resistor::Resistor()
	: eDrawableBase("assets\\resistor2.png")
{
	GetTexture().setSmooth(true);
	m_PinPositions =
	{
		{ 0.0f, 58.0f },
		{ float(m_sprite.getTextureRect().size.x), 58.0f }
	};
}


void Resistor::UIParams(eElement* elem)
{
	eResistor* resistor = static_cast<eResistor*>(elem);
	double min = 1e-6;
	double max = 1e6;
	ImGui::DragScalar(vfmt("Resistance##{}", u64(this)), ImGuiDataType_Double, &resistor->m_Resistance, 0.01f, &min, &max, "%.6f Ohm");
	ImGui::Text("Current: %.5f A", resistor->GetCurrent());
}


std::string Resistor::Parser_WriteElementData(eElement* elem)
{
	eResistor* r = static_cast<eResistor*>(elem);
	return std::to_string(r->m_Resistance);
}


void Resistor::Parser_ReadElementData(eElement* elem, const std::string& data)
{
	eResistor* r = static_cast<eResistor*>(elem);
	r->m_Resistance = std::stod(data);
}

void			Resistor::Draw() { dlDrawList::getWindow()->draw(m_sprite); }


//----------------------------------------------------------------------------------------------------------------------------------------
//											Battery
//
//----------------------------------------------------------------------------------------------------------------------------------------


Battery::Battery()
	: eDrawableBase("assets\\battery.png")
{
	GetTexture().setSmooth(true);
	m_PinPositions =
	{
		{ float(m_sprite.getTextureRect().size.x), 131.0f }, // +
		{ 0.0f, 131.0f }, // -
	};
}


void Battery::UIParams(eElement* elem)
{
	eVoltageSource* vs = static_cast<eVoltageSource*>(elem);
	if (!vs)
		return;

	ImGui::DragScalar(vfmt("Voltage/Amplitude##{}", u64(this)), ImGuiDataType_Double, &vs->m_Amplitude, 0.1f, nullptr, nullptr, "%.2f V");
	ImGui::DragScalar(vfmt("Frequency##{}", u64(this)), ImGuiDataType_Double, &vs->m_Frequency, 0.1f, nullptr, nullptr, "%.2f Hz");
	ImGui::DragScalar(vfmt("Phase##{}", u64(this)), ImGuiDataType_Double, &vs->m_Phase, 0.1f, nullptr, nullptr, "%.2f rad");

	ImGui::Text("Current: %.5f A", vs->GetCurrent());
}


std::string Battery::Parser_WriteElementData(eElement* elem)
{
	eVoltageSource* vs = static_cast<eVoltageSource*>(elem);
	return std::format("{:.6f} {:.6f} {:.6f}", vs->m_Amplitude, vs->m_Frequency, vs->m_Phase);
}


void Battery::Parser_ReadElementData(eElement* elem, const std::string& data)
{
	eVoltageSource* vs = static_cast<eVoltageSource*>(elem);
	if (auto result = scn::scan<double, double, double>(data, "{} {} {}"))
	{
		auto [ampl, freq, phase] = result->values();
		vs->m_Amplitude = ampl;
		vs->m_Frequency = freq;
		vs->m_Phase = phase;
	}
}

void			Battery::Draw() { dlDrawList::getWindow()->draw(m_sprite); }


//----------------------------------------------------------------------------------------------------------------------------------------
//											Capacitor
//
//----------------------------------------------------------------------------------------------------------------------------------------


Capacitor::Capacitor()
	: eDrawableBase("assets\\capacitor.png")
{
	GetTexture().setSmooth(true);
	m_PinPositions =
	{
		{ float(m_sprite.getTextureRect().size.x), 100.0f },
		{ 0.0f, 100.0f },
	};
}


void Capacitor::Draw()
{
	dlDrawList::getWindow()->draw(m_sprite);
}


void Capacitor::UIParams(eElement* elem)
{
	eCapacitor* capacitor = static_cast<eCapacitor*>(elem);
	double min = 1e-6;
	double max = 1e6;
	ImGui::DragScalar("Capacitance", ImGuiDataType_Double, &capacitor->m_Capacitance, 0.00001f, &min, &max, "%.6f F");
}


std::string Capacitor::Parser_WriteElementData(eElement* elem)
{
	eCapacitor* capacitor = static_cast<eCapacitor*>(elem);
	return std::to_string(capacitor->m_Capacitance);
}


void Capacitor::Parser_ReadElementData(eElement* elem, const std::string& data)
{
	eCapacitor* capacitor = static_cast<eCapacitor*>(elem);
	capacitor->m_Capacitance = std::stod(data);
}


//----------------------------------------------------------------------------------------------------------------------------------------
//											Inductor
//
//----------------------------------------------------------------------------------------------------------------------------------------


Inductor::Inductor()
	: eDrawableBase("assets\\inductor2.png")
{
	GetTexture().setSmooth(true);
	m_PinPositions =
	{
		{ float(m_sprite.getTextureRect().size.x), 57.0f },
		{ 0.0f, 57.0f },
	};
}

void Inductor::Draw()
{
	dlDrawList::getWindow()->draw(m_sprite);
}

void Inductor::UIParams(eElement* elem)
{
	eInductor* inductor = static_cast<eInductor*>(elem);
	double min = 1e-6;
	double max = 1e6;
	ImGui::DragScalar("Inductance", ImGuiDataType_Double, &inductor->m_Inductance, 0.0001f, &min, &max, "%.6f H");
}

std::string Inductor::Parser_WriteElementData(eElement* elem)
{
	return std::to_string(static_cast<eInductor*>(elem)->m_Inductance);
}

void Inductor::Parser_ReadElementData(eElement* elem, const std::string& data)
{
	eInductor* inductor = static_cast<eInductor*>(elem);
	inductor->m_Inductance = std::stod(data);
}


//----------------------------------------------------------------------------------------------------------------------------------------
//											Diode
//
//----------------------------------------------------------------------------------------------------------------------------------------



Diode::Diode()
	: eDrawableBase("assets\\diode.png")
{
	GetTexture().setSmooth(true);
	m_PinPositions =
	{
		{ 0.0f, 64.0f },
		{ float(m_sprite.getTextureRect().size.x), 64.0f },
	};
}

void Diode::Draw()
{
	dlDrawList::getWindow()->draw(m_sprite);
}


void Diode::UIParams(eElement* elem)
{

}


std::string Diode::Parser_WriteElementData(eElement* elem)
{
	return "";
}

void Diode::Parser_ReadElementData(eElement* elem, const std::string& data)
{
}


//----------------------------------------------------------------------------------------------------------------------------------------
//											RelayContactsGroup
//
//----------------------------------------------------------------------------------------------------------------------------------------

#if 0 
RelayContactsGroup::RelayContactsGroup()
	: eDrawableBase("assets\\switch.png")
	, m_BaseSprite(m_texture)
	, m_SwitchSprite(m_texture)
{
	m_texture.setSmooth(true);

	float texSize_X = float(m_sprite.getTextureRect().size.x);
	float texSize_Y = float(m_sprite.getTextureRect().size.y);

	m_PinPositions =
	{
		{ 0.0f, 12.0f },
		{ texSize_X, 12.0f },
		{ 100.0f, 130.0f },
	};
	m_BaseSprite.setTextureRect(sf::IntRect({ 0, 0 }, { int(texSize_X), 60 }));
	m_SwitchSprite.setTextureRect(sf::IntRect({ 0, 70 }, { int(texSize_X), 20 }));

	m_SwitchSprite.setOrigin({ texSize_X - 12.0f, 10.0f });
	m_SwitchPos = { 250, 56 };
	m_SwitchSprite.setPosition(m_SwitchPos);
	m_SwitchSprite.setRotation(sf::degrees(-25.0f));
}
#else
RelayContactsGroup::RelayContactsGroup()
	: eDrawableBase("assets\\switch2.png")
	, m_BaseSprite(m_texture)
	, m_SwitchSprite(m_texture)
{
	m_texture.setSmooth(true);

	float texSize_X = float(m_sprite.getTextureRect().size.x);
	float texSize_Y = float(m_sprite.getTextureRect().size.y);

	m_PinPositions =
	{
		{ 12.0f, 12.0f },
		{ 212.0f, 12.0f },
		{ 12.0f, 155.0f },
	};
	m_BaseSprite.setTextureRect(sf::IntRect({ 0, 0 }, { 224, 60 }));
	m_SwitchSprite.setTextureRect(sf::IntRect({ 0, 70 }, { int(texSize_X), 20 }));

	m_SwitchSprite.setOrigin({ texSize_X - 12.0f, 10.0f });
	m_SwitchPos = { 212, 56 };
	m_SwitchSprite.setPosition(m_SwitchPos);
	m_SwitchSprite.setRotation(sf::degrees(-25.0f));
}

#endif

void RelayContactsGroup::SetPosition(sf::Vector2f pos)
{
	m_BaseSprite.setPosition(pos);
	m_SwitchSprite.setPosition(pos + m_SwitchPos);
}


sf::Vector2f RelayContactsGroup::GetPosition()
{
	return m_BaseSprite.getPosition();
}

void RelayContactsGroup::Update(DrawableCircuit& circ, eElement* elem)
{
	eRelayContactsGroup* eGroup = static_cast<eRelayContactsGroup*>(elem);

	if (m_Coil.expired()) // m_Coil is null
	{
		m_Coil.reset();
		if (!LookupCoil(eGroup, circ)) // couldn't find any coil
		{
			eGroup->SetState(m_NormalState); // set to default state
			SetStateToDraw(m_NormalState);
			return;
		}
	}

	eCoil* pCoil = static_cast<eCoil*>(circ.GetAssociatedElectricElement(m_Coil.lock().get()));
	if (pCoil->GetHashName() != eGroup->GetCoilHashName())   // Coil isn't null but its name is different from the one in eGroup
	{
		m_Coil.reset();
		if (!LookupCoil(eGroup, circ))
		{
			eGroup->SetState(m_NormalState);
			SetStateToDraw(m_NormalState);
			return;
		}
	}

	if (pCoil->IsActive())
	{
		switch (m_NormalState)
		{
		case eRelayContactsGroup::n11_n12:
			SetStateToDraw(eRelayContactsGroup::State::n11_n13);
			eGroup->SetState(eRelayContactsGroup::State::n11_n13);
			break;
		case eRelayContactsGroup::n11_n13:
			SetStateToDraw(eRelayContactsGroup::State::n11_n12);
			eGroup->SetState(eRelayContactsGroup::State::n11_n12);
			break;
		default:
			break;
		}
	}
	else
	{
		eGroup->SetState(m_NormalState);
		SetStateToDraw(m_NormalState);
	}
}


void RelayContactsGroup::SetRotation(float degrees)
{
	const sf::Angle NewRotation = sf::degrees(degrees);
	for (sf::Vector2f& pos : m_PinPositions)
	{
		sf::Vector2f initialPos = pos.rotatedBy(-m_Rotation);
		pos = initialPos.rotatedBy(NewRotation);
	}

	m_BaseSprite.setRotation(NewRotation);
	m_SwitchPos = m_SwitchPos.rotatedBy(-m_Rotation).rotatedBy(NewRotation);
	m_Rotation = NewRotation;
}


void RelayContactsGroup::Flip(flipAxis axis)
{
	for (auto& pos : m_PinPositions)
	{
		sf::Vector2f localPos = pos.rotatedBy(-m_Rotation);

		if (axis == flipAxis::X)
			localPos.x = -localPos.x;
		else
			localPos.y = -localPos.y;

		pos = localPos.rotatedBy(m_Rotation);
	}

	sf::Vector2f baseScale = m_BaseSprite.getScale();
	m_BaseSprite.setScale({
		axis == flipAxis::X ? -baseScale.x : baseScale.x,
		axis == flipAxis::Y ? -baseScale.y : baseScale.y });

	sf::Vector2f buttonScale = m_SwitchSprite.getScale();
	m_SwitchSprite.setScale({
		axis == flipAxis::X ? -buttonScale.x : buttonScale.x,
		axis == flipAxis::Y ? -buttonScale.y : buttonScale.y });


	if (axis == flipAxis::X)
	{
		m_IsFlipped_X = !m_IsFlipped_X;
		m_SwitchPos = m_SwitchPos.rotatedBy(-m_Rotation);
		m_SwitchPos.x = -m_SwitchPos.x;
		m_SwitchPos = m_SwitchPos.rotatedBy(m_Rotation);
	}
	else
	{
		m_IsFlipped_Y = !m_IsFlipped_Y;
		m_SwitchPos = m_SwitchPos.rotatedBy(-m_Rotation);
		m_SwitchPos.y = -m_SwitchPos.y;
		m_SwitchPos = m_SwitchPos.rotatedBy(m_Rotation);
	}
}


bool RelayContactsGroup::LookupCoil(eRelayContactsGroup* my_eContacts, DrawableCircuit& circ)
{
	auto& drawableElements = circ.GetDrawableElements();

	auto it = std::ranges::find_if(drawableElements,					// eGroup name should match the coil name
		[&](const std::shared_ptr<eDrawableBase>& otherDrawable) 
		{
			if (otherDrawable->IsCoil())
			{
				eCoil* someCoil = static_cast<eCoil*>(circ.GetAssociatedElectricElement(otherDrawable.get()));
				return my_eContacts->GetCoilHashName() == someCoil->GetHashName();
			}
			return false;
		});


	if (it != drawableElements.end())
		m_Coil = *it;
	else
		return false;
}


void RelayContactsGroup::Draw()
{
	switch (m_stateToDraw)
	{
	case eRelayContactsGroup::n11_n12:
		m_SwitchSprite.setRotation(m_Rotation + sf::degrees(0.0f));
		break;
	case eRelayContactsGroup::n11_n13:
		{
			float sign = m_IsFlipped_Y ? -1.0f : 1.0f;
			m_SwitchSprite.setRotation(m_Rotation + sign * sf::degrees(m_IsFlipped_X ? 25.0f : -25.0f));
		}
		break;
	default:
		break;
	}

	dlDrawList::getWindow()->draw(m_BaseSprite);
	dlDrawList::getWindow()->draw(m_SwitchSprite);

	for (auto& pos : m_PinPositions)
	{
		DrawCircle(m_BaseSprite.getPosition() + pos, 5.0f, sf::Color::Red);
	}
}



void RelayContactsGroup::UIParams(eElement* elem)
{
	eRelayContactsGroup* contact = static_cast<decltype(contact)>(elem);

	if (ImGui::InputText("Coil name", m_UiBuff, std::size(m_UiBuff)))
	{
		contact->SetCoilName(m_UiBuff);
	}
	if (ImGui::Combo("Normal state", (int*)&m_NormalState, "n11_n12\0n11_n13\0"))
	{

	}
}


std::string RelayContactsGroup::Parser_WriteElementData(eElement* elem) 
{ 
	eRelayContactsGroup* c = static_cast<decltype(c)>(elem);
	auto& name = c->GetCoilName();
	return std::format("{} {} {}", name.empty() ? "-" : name, c->GetCoilHashName(), u64(m_NormalState));
}

void RelayContactsGroup::Parser_ReadElementData(eElement* elem, const std::string& data)
{
	eRelayContactsGroup* c = static_cast<decltype(c)>(elem);
	std::istringstream iss(data);
	iss >> c->m_CoilName >> c->m_HashName >> *(int*)(&m_NormalState);
	std::copy(c->m_CoilName.begin(), c->m_CoilName.end(), std::begin(m_UiBuff));
}
bool RelayContactsGroup::IsHovered() { return WidgetBase::IsHovered(m_BaseSprite); }


//----------------------------------------------------------------------------------------------------------------------------------------
//											Coil
//
//----------------------------------------------------------------------------------------------------------------------------------------


Coil::Coil(const std::string& path)
	: eDrawableBase(path)
{ }

void Coil::Draw()
{
	dlDrawList::getWindow()->draw(m_sprite);
}

void Coil::UIParams(eElement* elem)
{
	eCoil* coil = static_cast<eCoil*>(elem);
	if (ImGui::InputText("Coil name", m_UiBuff, std::size(m_UiBuff)))
		coil->SetName(m_UiBuff);
	double min = 0.00001;
	double max = 100.0;

	ImGui::DragScalar("Inductance", ImGuiDataType_Double, &coil->m_l.m_Inductance, 0.00001f, &min, &max, "%.6f H");
	ImGui::DragScalar(vfmt("Resistance##{}", u64(this)), ImGuiDataType_Double, &coil->m_l.m_R, 0.01f, &min, &max, "%.5f Ohm");
	ImGui::DragScalar(vfmt("Release delay##{}", u64(this)), ImGuiDataType_Double, &coil->m_ReleaseDelay, 0.01f, &min, &max, "%.5f s");
	ImGui::DragScalar(vfmt("Current threshold##{}", u64(this)), ImGuiDataType_Double, &coil->m_CurrThreshold, 0.001f, &min, &max, "%.5f A");
	ImGui::Text("Is active: %s", coil->IsActive() ? "true" : "false");
	ImGui::Text("Inactive timer: %.5f", coil->m_InactiveCoilTimer);
}

std::string Coil::Parser_WriteElementData(eElement* elem)
{
	eCoil* coil = static_cast<eCoil*>(elem);
	return std::format("{} {} {:.6f} {:.6f} {:.6f} {:.6f}",
		coil->m_Name.empty() ? "-" : coil->m_Name,
		coil->m_HashName,
		coil->m_ReleaseDelay,
		coil->m_CurrThreshold,
		coil->m_l.m_R, 
		coil->m_l.m_Inductance );
}

void Coil::Parser_ReadElementData(eElement* elem, const std::string& data)
{
	eCoil* coil = static_cast<eCoil*>(elem);
	//if (auto result = scn::scan<std::string, u64, double, double, double>(data, "{} {} {} {} {}"))
	//{
	//	auto [name, hashName, releaseDelay, currThreshold, induct] = result->values();
	//	coil->m_Name = name;
	//	coil->m_HashName = hashName;
	//	coil->m_ReleaseDelay = releaseDelay;
	//	coil->m_CurrThreshold = currThreshold;
	//	coil->m_l.SetInductance(induct);
	//}

	std::istringstream iss(data);
	iss >> coil->m_Name 					 		// coil->m_Name.empty() ? "-" : coil->m_Name,
		>> coil->m_HashName 				 		// coil->m_HashName,
		>> coil->m_ReleaseDelay 			 		// coil->m_ReleaseDelay,
		>> coil->m_CurrThreshold			 		// coil->m_CurrThreshold,
		>> coil->m_l.m_R 					 		// coil->m_l.m_R,
		>> coil->m_l.m_Inductance;			 		// coil->m_l.m_Inductance );

	std::copy(coil->m_Name.begin(), coil->m_Name.end(), std::begin(m_UiBuff));
}

void Coil::Update(DrawableCircuit& circ, eElement* elem)
{
	//eCoil* coil = static_cast<eCoil*>(elem);

}



//----------------------------------------------------------------------------------------------------------------------------------------
//											NeutralRelayCoil
//
//----------------------------------------------------------------------------------------------------------------------------------------


NeutralRelayCoil::NeutralRelayCoil()
	: Coil("assets\\coilNeutral.png")
{
	GetTexture().setSmooth(true);
	m_PinPositions =
	{
		{ 0.0f, 148.0f },
		{ float(m_sprite.getTextureRect().size.x), 148.0f },
	};
}


NeutralRelayCoil3RelyabilityClass::NeutralRelayCoil3RelyabilityClass()
	: Coil("assets\\coilNeutral3.png")
{
	GetTexture().setSmooth(true);
	m_PinPositions =
	{
		{ 0.0f, 135.0f },
		{ float(m_sprite.getTextureRect().size.x), 135.0f },
	};
}


//----------------------------------------------------------------------------------------------------------------------------------------
//											NeutralRelayCoil with Switch Off Delay
//
//----------------------------------------------------------------------------------------------------------------------------------------


NeutralRelayCoilWithSwitchOffDelay::NeutralRelayCoilWithSwitchOffDelay()
	: Coil("assets\\coilNeutralSwitchOffDelay.png")
{
	GetTexture().setSmooth(true);
	m_PinPositions =
	{
		{ 0.0f, 148.0f },
		{ float(m_sprite.getTextureRect().size.x), 148.0f },
	};
}

NeutralRelayCoilWithSwitchOffDelay3RelyabilityClass::NeutralRelayCoilWithSwitchOffDelay3RelyabilityClass()
	: Coil("assets\\coilNeutral3SwitchOffDelay.png")
{
	GetTexture().setSmooth(true);
	m_PinPositions =
	{
		{ 0.0f, 135.0f },
		{ float(m_sprite.getTextureRect().size.x), 135.0f },
	};
}

//----------------------------------------------------------------------------------------------------------------------------------------
//											NeutralRelayCoil with rectifier
//
//----------------------------------------------------------------------------------------------------------------------------------------


NeutralRelayCoilWithRectifier::NeutralRelayCoilWithRectifier()
	: Coil("assets\\coilNeutralWithRectifier.png")
{
	GetTexture().setSmooth(true);
	m_PinPositions =
	{
		{ 0.0f, 148.0f },
		{ float(m_sprite.getTextureRect().size.x), 148.0f },
	};
}

void NeutralRelayCoilWithRectifier::UIParams(eElement* elem)
{
	eCoilWithRectifier* coil = static_cast<eCoilWithRectifier*>(elem);
	Coil::UIParams(elem);

	double min = 1e-6;
	double max = 1e6;
	ImGui::DragScalar(vfmt("R##{}", u64(this)), ImGuiDataType_Double, &coil->m_r.m_Resistance, 0.01f, &min, &max, "%.4f");
	//auto diodeParams = [&](eDiode& d)
	//	{
	//		ImGui::Separator();
	//		bool edited = false;
	//		edited = ImGui::DragScalar(vfmt("Thermal voltage##{}", u64(&d)), ImGuiDataType_Double, &d.m_Vt, 0.00001f, &min, &max, "%.6f");
	//		edited = ImGui::DragScalar(vfmt("Saturation current##{}", u64(&d)), ImGuiDataType_Double, &d.m_Is, 0.00001f, &min, &max, "%.6f");
	//		edited = ImGui::DragScalar(vfmt("Zener breakdown voltage##{}", u64(&d)), ImGuiDataType_Double, &d.m_ZVoltage, 0.00001f, &min, &max, "%.6f");
	//		edited = ImGui::DragScalar(vfmt("Offset for Zener breakdown exponential##{}", u64(&d)), ImGuiDataType_Double, &d.m_ZOffset, 0.00001f, &min, &max, "%.6f");
	//		edited = ImGui::DragScalar(vfmt("Critical voltage for limiting exponential growth##{}", u64(&d)), ImGuiDataType_Double, &d.m_Vcrit, 0.00001f, &min, &max, "%.6f");
	//		edited = ImGui::DragScalar(vfmt("Critical voltage for Zener breakdown limiting##{}", u64(&d)), ImGuiDataType_Double, &d.m_Vzcrit, 0.00001f, &min, &max, "%.6f");
	//		edited = ImGui::DragScalar(vfmt("scale voltage##{}", u64(&d)), ImGuiDataType_Double, &d.m_Vscale, 0.00001f, &min, &max, "%.6f");
	//		edited = ImGui::DragScalar(vfmt("1 / Vscale##{}", u64(&d)), ImGuiDataType_Double, &d.m_Vdcoef, 0.00001f, &min, &max, "%.6f");
	//		edited = ImGui::DragScalar(vfmt("1 / Vt for Zener breakdown##{}", u64(&d)), ImGuiDataType_Double, &d.m_Vzcoef, 0.00001f, &min, &max, "%.6f");
	//		d.SetupCriticalVoltages();
	//	};

	//diodeParams(coil->m_db.m_Diodes[0]);
	//diodeParams(coil->m_db.m_Diodes[1]);
	//diodeParams(coil->m_db.m_Diodes[2]);
	//diodeParams(coil->m_db.m_Diodes[3]);
}

std::string NeutralRelayCoilWithRectifier::Parser_WriteElementData(eElement* elem)
{
	eCoilWithRectifier* coil = static_cast<eCoilWithRectifier*>(elem);
	return std::format("{} {} {:.6f} {:.6f} {:.6f} {:.6f}",
		coil->m_Name.empty() ? "-" : coil->m_Name,
		coil->m_HashName,
		coil->m_ReleaseDelay,
		coil->m_CurrThreshold,
		coil->m_l.GetInductance(),
		coil->m_r.GetResistance() );
}

void NeutralRelayCoilWithRectifier::Parser_ReadElementData(eElement* elem, const std::string& data)
{
	eCoilWithRectifier* coil = static_cast<eCoilWithRectifier*>(elem);
	//if (auto result = scn::scan<u64, double, double, double, double>(data, "{} {} {} {} {}"))
	//{
	//	auto [hashName, releaseDelay, currThreshold, l, r] = result->values();
	//	coil->m_HashName = hashName;
	//	coil->m_ReleaseDelay = releaseDelay;
	//	coil->m_CurrThreshold = currThreshold;
	//	coil->m_l.SetInductance(l);
	//	coil->m_r.SetResistance(r);
	//}
	std::istringstream iss(data);
	iss >> coil->m_Name 
		>> coil->m_HashName 
		>> coil->m_ReleaseDelay 
		>> coil->m_CurrThreshold 
		>> coil->m_l.m_Inductance 
		>> coil->m_r.m_Resistance;
	std::copy(coil->m_Name.begin(), coil->m_Name.end(), std::begin(m_UiBuff));
}



//----------------------------------------------------------------------------------------------------------------------------------------
//											Transformer
//
//----------------------------------------------------------------------------------------------------------------------------------------

Transformer::Transformer()
	: eDrawableBase("assets\\transformer1.png")
{
	GetTexture().setSmooth(true);
	m_PinPositions =
	{
		{ 12.0f, 0.0f }, // 0
		{ 12.0f, float(m_sprite.getTextureRect().size.y) }, // 1
		{ float(m_sprite.getTextureRect().size.x) - 12.0f, 0.0f }, // 2
		{ float(m_sprite.getTextureRect().size.x) - 12.0f, float(m_sprite.getTextureRect().size.y) }, // 3
	};
}


void Transformer::Draw()
{
	dlDrawList::getWindow()->draw(m_sprite);
}


void Transformer::UIParams(eElement* elem)
{
	eTransformer* t = static_cast<eTransformer*>(elem);
	double r = t->GetRatio();
	double min = 1e-6;
	double max = 1e6;
	ImGui::DragScalar(vfmt("L1##{}", u64(this)), ImGuiDataType_Double, &t->m_L1, 0.00001f, &min, &max, "%.6f H");
	ImGui::DragScalar(vfmt("L2##{}", u64(this)), ImGuiDataType_Double, &t->m_L2, 0.00001f, &min, &max, "%.6f H");
	ImGui::DragScalar(vfmt("Coupling coefficient##{}", u64(this)), ImGuiDataType_Double, &t->m_CouplCoef, 0.00001f, &min, &max, "%.6f");
	if (ImGui::DragScalar(vfmt("Ratio##{}", u64(this)), ImGuiDataType_Double, &r, 0.001f, &min, &max, "%.6f"))
	{
		t->SetRatio(r);
	}
}


std::string Transformer::Parser_WriteElementData(eElement* elem)
{
	eTransformer* t = static_cast<eTransformer*>(elem);
	return std::format("{:.6f} {:.6f} {:.6f}", t->m_L1, t->m_L2, t->m_CouplCoef);
}


void Transformer::Parser_ReadElementData(eElement* elem, const std::string& data)
{
	eTransformer* t = static_cast<eTransformer*>(elem);
	if (auto result = scn::scan<double, double, double>(data, "{} {} {}"))
	{
		auto [L1, L2, CouplCoef] = result->values();
		t->m_L1 = L1;
		t->m_L2 = L2;
		t->m_CouplCoef = CouplCoef;
	}
}

//----------------------------------------------------------------------------------------------------------------------------------------
//											DiodeBridge
//
//----------------------------------------------------------------------------------------------------------------------------------------



DiodeBridge::DiodeBridge()
	: eDrawableBase("assets\\diodeBridge.png")
{
	GetTexture().setSmooth(true);
	m_PinPositions =
	{
		{ 160.0f, 10.0f  }, // in0
		{ 160.0f, 310.0f }, // in1
		{ 10.0f, 160.0f  } , // out2
		{ 310.0f, 160.0f }, // out3
	};
}

void DiodeBridge::Draw()
{
	dlDrawList::getWindow()->draw(m_sprite);
}

TransformerWithMiddlePin::TransformerWithMiddlePin()
	: eDrawableBase("assets\\transformer3.png")
{
	GetTexture().setSmooth(true);
	m_PinPositions =
	{
		{ 12.0f, 0.0f }, // primary start
		{ 12.0f, float(m_sprite.getTextureRect().size.y) }, // primary end
		{ float(m_sprite.getTextureRect().size.x) - 12.0f, 0.0f }, // secondary start
		{ float(m_sprite.getTextureRect().size.x) - 12.0f, float(m_sprite.getTextureRect().size.y) }, // secondary end
		{ float(m_sprite.getTextureRect().size.x) - 12.0f, float(m_sprite.getTextureRect().size.y) / 2.0f }, // middle
	};
}	


//----------------------------------------------------------------------------------------------------------------------------------------
//											TransformerWithMiddlePin
//
//----------------------------------------------------------------------------------------------------------------------------------------


void TransformerWithMiddlePin::Draw()
{
	dlDrawList::getWindow()->draw(m_sprite);
}

void TransformerWithMiddlePin::UIParams(eElement* elem)
{
	SimTy* t = static_cast<SimTy*>(elem);

	double min = 1e-6;
	double max = 1e6;

	auto TransformerParams = [&](eTransformer* t)
		{
			double r = t->GetRatio();
			ImGui::DragScalar(vfmt("L1##{}", u64(t)), ImGuiDataType_Double, &t->m_L1, 0.00001f, &min, &max, "%.6f H");
			ImGui::DragScalar(vfmt("L2##{}", u64(t)), ImGuiDataType_Double, &t->m_L2, 0.00001f, &min, &max, "%.6f H");
			ImGui::DragScalar(vfmt("Coupling coefficient##{}", u64(t)), ImGuiDataType_Double, &t->m_CouplCoef, 0.00001f, &min, &max, "%.6f");
			ImGui::DragScalar(vfmt("R1##{}", u64(t)), ImGuiDataType_Double, &t->m_R1, 0.001f, &min, &max, "%.6f Ohm");
			ImGui::DragScalar(vfmt("R2##{}", u64(t)), ImGuiDataType_Double, &t->m_R2, 0.001f, &min, &max, "%.6f Ohm");
			if (ImGui::DragScalar(vfmt("Ratio##{}", u64(t)), ImGuiDataType_Double, &r, 0.001f, &min, &max, "%.6f"))
			{
				t->SetRatio(r);
			}
		};

	TransformerParams(&t->m_T[0]);
	ImGui::Separator();
	TransformerParams(&t->m_T[1]);
}

std::string TransformerWithMiddlePin::Parser_WriteElementData(eElement* elem)
{
	SimTy* t = static_cast<SimTy*>(elem);
	return std::format("{:.6f} {:.6f} {:.6f} {:.6f} {:.6f} {:.6f}",
		t->m_T[0].m_L1, t->m_T[0].m_L2, t->m_T[0].m_CouplCoef,
		t->m_T[1].m_L1, t->m_T[1].m_L2, t->m_T[1].m_CouplCoef);
	
}

void TransformerWithMiddlePin::Parser_ReadElementData(eElement* elem, const std::string& data)
{
	SimTy* t = static_cast<SimTy*>(elem);
	if (auto result = scn::scan<double, double, double, double, double, double>(data, "{} {} {} {} {} {}"))
	{
		auto [L01, L02, k0, L11, L12, k1] = result->values();
		t->m_T[0].m_L1 = L01;
		t->m_T[0].m_L2 = L02;
		t->m_T[0].m_CouplCoef = k0;

		t->m_T[1].m_L1 = L11;
		t->m_T[1].m_L2 = L12;
		t->m_T[1].m_CouplCoef = k1;
	}

}

KPTSH::KPTSH()
	: eDrawableBase("assets\\kptsh.png")
	, m_groupOffsets
	{
		{ 286.0f, 300.0f - 12.0f},
		{ 286.0f, 650.0f - 12.0f},
		{ 286.0f, 1000.0f - 12.0f },
	}
{
	GetTexture().setSmooth(true);
	m_PinPositions = 
	{
		{ 0.0f, 300.0f  }, { float(m_sprite.getTextureRect().size.x), 300.0f },
		{ 0.0f, 650.0f  }, { float(m_sprite.getTextureRect().size.x), 650.0f },
		{ 0.0f, 1000.0f }, { float(m_sprite.getTextureRect().size.x), 1000.0f },
		{ 0.0f, 1500.0f }, { float(m_sprite.getTextureRect().size.x), 1500.0f },
	};
	SetPosition(GetPosition());
}


void KPTSH::Draw()
{
	dlDrawList::getWindow()->draw(m_sprite);
	m_Groups[0].Draw();
	m_Groups[1].Draw();
	m_Groups[2].Draw();

	//for (auto& pos : m_PinPositions)
	//{
	//	DrawCircle(m_sprite.getPosition() + pos, 10.0f, sf::Color::Red);
	//}
}

std::string KPTSH::Parser_WriteElementData(eElement* elem)
{
	SimTy* kptsh = static_cast<SimTy*>(elem);
	return std::to_string(kptsh->GetType());
}

void KPTSH::Parser_ReadElementData(eElement* elem, const std::string& data)
{
	SimTy* kptsh = static_cast<SimTy*>(elem);
	kptsh->SetType(std::stoi(data));
}

void KPTSH::UIParams(eElement* element)
{
	SimTy* kptsh = static_cast<SimTy*>(element);
	const char* items[] = { "TYPE 5", "TYPE 7", };

	static int selected = kptsh->m_Type == 5 ? 0 : 1;
	if (ImGui::Combo("Type", &selected, items, std::size(items)))
	{
		selected == 0 ? kptsh->SetType(5) : kptsh->SetType(7);
	}

	ImGui::Text("Z: %s", kptsh->m_Z.GetState() == eRelayContactsGroup::State::n11_n12 ? "n11_n12" : "n11_n13");
	ImGui::Text("J: %s", kptsh->m_J.GetState() == eRelayContactsGroup::State::n11_n12 ? "n11_n12" : "n11_n13");
	ImGui::Text("KJ: %s", kptsh->m_KJ.GetState() == eRelayContactsGroup::State::n11_n12 ? "n11_n12" : "n11_n13");
	
	ImGui::Text("t: %.5f", kptsh->m_Timer);
}

void KPTSH::SetPosition(sf::Vector2f pos)
{
	m_sprite.setPosition(pos);
	m_Groups[0].SetPosition(pos + m_groupOffsets[0]);
	m_Groups[1].SetPosition(pos + m_groupOffsets[1]);
	m_Groups[2].SetPosition(pos + m_groupOffsets[2]);
}

void KPTSH::Update(DrawableCircuit& circ, eElement* elem)
{
	SimTy* kptsh = static_cast<SimTy*>(elem);
	m_Groups[0].SetStateToDraw(kptsh->m_Z.GetState());
	m_Groups[1].SetStateToDraw(kptsh->m_J.GetState());
	m_Groups[2].SetStateToDraw(kptsh->m_KJ.GetState());
}

ZBF::ZBF()
	: eDrawableBase("assets\\zbf.png")
{
	GetTexture().setSmooth(true);
	m_PinPositions =
	{
		{ 100.0f, 0.0f }, 		
		{ 100.0f, float(m_sprite.getTextureRect().size.y) },
		{ 400.0f, 0.0f }, 		
		{ 400.0f, float(m_sprite.getTextureRect().size.y) },
	};
}

void ZBF::Draw()
{
	dlDrawList::getWindow()->draw(m_sprite);
}
