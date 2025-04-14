#include "core/SFMLRenderer.h"
#include "WidgetsBase.h"

WidgetBase::WidgetBase(const std::string& path)
	: m_sprite(m_texture)
{
	if (path.empty())
		return;
    loadImageFromFile(path);
}

void WidgetBase::loadImageFromFile(const std::string& path)
{
    SM_ASSERT(m_texture.loadFromFile(path), std::format("::WidgetBase() Couldn't load image from the given path : {}", path));
    if (!m_texture.generateMipmap())
		std::cout << "Failed to generate mipmaps to texture: " << path << '\n';
    m_sprite.setTexture(m_texture, true);
}

bool WidgetBase::IsHovered()
{
	return IsHovered(m_sprite);
}

bool WidgetBase::IsHovered(sf::Sprite& sprite)
{
	return sprite.getGlobalBounds().contains(gSFMLRenderer.GetWorldMousePos());
}

sf::Texture&     WidgetBase::GetTexture()                   { return m_texture; }
sf::Sprite&      WidgetBase::GetSprite()                    { return m_sprite; }
void             WidgetBase::SetPosition(sf::Vector2f pos)  { m_sprite.setPosition(pos); }
void             WidgetBase::SetColor(sf::Color color)      { m_sprite.setColor(color);  }
