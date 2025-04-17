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



Resistor::Resistor()
	: eDrawableBase("assets\\resistor.png")
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
	ImGui::DragScalar(vfmt("Resistance##{}", u64(this)), ImGuiDataType_Double, &resistor->m_Resistance, 0.1f, nullptr, nullptr, "%.2f Ohm");
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
	ImGui::DragScalar("Capacitance", ImGuiDataType_Double, &capacitor->m_Capacitance, 0.00001f, nullptr, nullptr, "%.6f F");
}


std::string Capacitor::Parser_WriteElementData(eElement* elem)
{
	eCapacitor* capacitor = static_cast<eCapacitor*>(elem);
	std::string data = std::to_string(capacitor->m_Capacitance);
	return data;
}


void Capacitor::Parser_ReadElementData(eElement* elem, const std::string& data)
{
	eCapacitor* capacitor = static_cast<eCapacitor*>(elem);
	capacitor->m_Capacitance = std::stod(data);
}


//----------------------------------------------------------------------------------------------------------------------------------------
//											Inductor


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
	ImGui::DragScalar("Inductance", ImGuiDataType_Double, &inductor->m_Inductance, 0.00001f, nullptr, nullptr, "%.6f H");
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


RelayContactsGroup::RelayContactsGroup()
	: eDrawableBase("assets\\switch.png")
	, m_BaseSprite(m_texture)
	, m_ButtonSprite(m_texture)
{
	m_texture.setSmooth(true);

	m_PinPositions =
	{
		{ 0.0f, 12.0f },
		{ float(m_sprite.getTextureRect().size.x), 12.0f },
		{ 68.0f, 130.0f },
	};
	m_BaseSprite.setTextureRect(sf::IntRect({ 0, 0 }, { 300, 60 }));
	m_ButtonSprite.setTextureRect(sf::IntRect({ 0, 70 }, { 219, 48 }));

	m_ButtonSprite.setOrigin({ 195.0f, 24.0f });
	m_SwitchPos = { 235, 56 };
	m_ButtonSprite.setPosition(m_SwitchPos);
	m_ButtonSprite.setRotation(sf::degrees(-25.0f));
}


void RelayContactsGroup::SetPosition(sf::Vector2f pos)
{
	m_BaseSprite.setPosition(pos);
	m_ButtonSprite.setPosition(pos + m_SwitchPos);
}


sf::Vector2f RelayContactsGroup::GetPosition()
{
	return m_BaseSprite.getPosition();
}

void RelayContactsGroup::Update(DrawableCircuit& circ, eElement* elem)
{
	eRelayContactsGroup* eGroup = static_cast<eRelayContactsGroup*>(elem);

	if (m_Coil.expired())
	{
		m_Coil.reset();
		if (!LookupCoil(eGroup, circ))
			return;
	}

	eCoil* pCoil = static_cast<eCoil*>(circ.GetAssociatedElectricElement(m_Coil.lock().get()));
	if (pCoil->GetHashName() != eGroup->GetCoilHashName())
	{
		m_Coil.reset();
		if (!LookupCoil(eGroup, circ))
		{
			m_stateToDraw = eRelayContactsGroup::State::n11_n12;
			eGroup->SetState(eRelayContactsGroup::State::n11_n12);
			return;
		}
	}

	if (pCoil->IsActive())
	{
		eGroup->SetState(eRelayContactsGroup::State::n11_n13);
		m_stateToDraw = eRelayContactsGroup::State::n11_n13;
	}
	else
	{
		eGroup->SetState(eRelayContactsGroup::State::n11_n12);
		m_stateToDraw = eRelayContactsGroup::State::n11_n12;
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

	sf::Vector2f buttonScale = m_ButtonSprite.getScale();
	m_ButtonSprite.setScale({
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

	auto it = std::ranges::find_if(drawableElements,
		[&](const std::shared_ptr<eDrawableBase>& otherDrawable)
		{
			if (otherDrawable->GetType() == DrawableType::DRAWABLE_RELAY_COIL)
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
		m_ButtonSprite.setRotation(m_Rotation + sf::degrees(0.0f));
		break;
	case eRelayContactsGroup::n11_n13:
		m_ButtonSprite.setRotation(m_Rotation + sf::degrees(-25.0f));
		break;
	default:
		break;
	}

	dlDrawList::getWindow()->draw(m_BaseSprite);
	dlDrawList::getWindow()->draw(m_ButtonSprite);

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
}


std::string RelayContactsGroup::Parser_WriteElementData(eElement* elem) { return ""; }
void RelayContactsGroup::Parser_ReadElementData(eElement* elem, const std::string& data) {}

bool RelayContactsGroup::IsHovered() { return WidgetBase::IsHovered(m_BaseSprite); }


//----------------------------------------------------------------------------------------------------------------------------------------
//											Coil


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

	ImGui::DragScalar("Inductance", ImGuiDataType_Double, &coil->m_Inductance, 0.00001f, nullptr, nullptr, "%.6f H");

	double min = 0.0;
	double max = 10.0;
	ImGui::DragScalar("Release delay", ImGuiDataType_Double, &coil->m_ReleaseDelay, 0.01f, &min, &max, "%.5f s");
	ImGui::DragScalar("Current threshold", ImGuiDataType_Double, &coil->m_CurrThreshold, 0.001f, &min, &max, "%.5f A");
	ImGui::Text("Is active: %s", coil->IsActive() ? "true" : "false");
	ImGui::Text("Inactive timer: %.5f", coil->m_InactiveCoilTimer);
}

std::string Coil::Parser_WriteElementData(eElement* elem)
{
	eCoil* coil = static_cast<eCoil*>(elem);
	return std::format("{} {:.6f} {:.6f}",
		coil->m_HashName,
		coil->m_ReleaseDelay,
		coil->m_CurrThreshold);
}

void Coil::Parser_ReadElementData(eElement* elem, const std::string& data)
{
	eCoil* coil = static_cast<eCoil*>(elem);
	if (auto result = scn::scan<u64, double, double>(data, "{} {} {}"))
	{
		auto [hashName, releaseDelay, currThreshold] = result->values();
		coil->m_HashName = hashName;
		coil->m_ReleaseDelay = releaseDelay;
		coil->m_CurrThreshold = currThreshold;
	}
}

void Coil::Update(DrawableCircuit& circ, eElement* elem)
{
	//eCoil* coil = static_cast<eCoil*>(elem);

}



//----------------------------------------------------------------------------------------------------------------------------------------
//											NeutralRelayCoil


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
	eCoilWithRectifier* wrapper = static_cast<eCoilWithRectifier*>(elem);
	double r = wrapper->r.GetResistance();
	double min = 1e-6;
	double max = 1e6;
	ImGui::DragScalar(vfmt("R##{}", u64(this)), ImGuiDataType_Double, &r, 0.01f, &min, &max, "%.4f Ohm");
	wrapper->r.SetResistance(r);

#if vComposeCoil
	if (ImGui::InputText("Coil name", m_UiBuff, std::size(m_UiBuff)))
		wrapper->l->SetName(m_UiBuff);

	ImGui::DragScalar(vfmt("Inductance##{}", u64(this)), ImGuiDataType_Double, &wrapper->l->m_Inductance, 0.00001f, &min, &max, "%.6f H");
	ImGui::DragScalar(vfmt("Release delay##{}", u64(this)), ImGuiDataType_Double, &wrapper->l->m_ReleaseDelay, 0.01f, &min, &max, "%.5f s");
	ImGui::DragScalar(vfmt("Current threshold##{}", u64(this)), ImGuiDataType_Double, &wrapper->l->m_CurrThreshold, 0.001f, &min, &max, "%.5f A");
	ImGui::Text("Is active: %s", wrapper->l->IsActive() ? "true" : "false");
	ImGui::Text("Inactive timer: %.5f", wrapper->l->m_InactiveCoilTimer);
	ImGui::Text("Current: %.5f A", wrapper->l->GetCurrent());
#else
	//Coil::UIParams(elem);
#endif
}



//----------------------------------------------------------------------------------------------------------------------------------------
//											Transformer

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
	eTransformer* transformer = static_cast<eTransformer*>(elem);
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


