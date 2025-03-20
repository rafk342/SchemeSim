#include "SFMLRenderer.h"
#include "WidgetsBase.h"

WidgetsBase::WidgetsBase(const std::string& path)
{
    loadImageFromFile(path);
}

void WidgetsBase::loadImageFromFile(const std::string& path)
{
    SM_ASSERT(m_texture.loadFromFile(path), std::format("::WidgetsBase() Couldn't load image from the given path : {}", path));
    m_texture.generateMipmap();
    m_sprite.setTexture(m_texture);
}

bool WidgetsBase::is_hovered()
{
    return m_sprite.getGlobalBounds().contains(g_SFMLRenderer.GetWorldMousePos());
}

sf::Texture&     WidgetsBase::GetTexture()   { return m_texture; }
sf::Sprite&      WidgetsBase::GetSprite()    { return m_sprite; }

void WidgetsBase::SetPosition(const sf::Vector2f& pos) { m_sprite.setPosition(pos); }


