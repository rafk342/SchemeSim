#pragma once

#include <thread>
#include <functional>
#include <print>

#include "vendor/SFML/Graphics.hpp"
#include "common/sm_assert.h"


class WidgetBase
{
protected:

    sf::Texture m_texture;
    sf::Sprite  m_sprite;

public:

    static bool IsHovered(sf::Sprite& sprite);
    
    WidgetBase(const std::string& path);
	virtual ~WidgetBase() = default;
 
    void                    loadImageFromFile(const std::string& path);
    void                    SetColor(sf::Color color);
    sf::Texture&            GetTexture();
    sf::Sprite&             GetSprite();
   
    virtual bool            IsHovered();
    virtual void            SetPosition(sf::Vector2f pos);
	virtual sf::Vector2f    GetPosition() { return m_sprite.getPosition(); }
};
