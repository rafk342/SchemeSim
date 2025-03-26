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

    WidgetBase(const std::string& path);
    void loadImageFromFile(const std::string& path);
    bool is_hovered();

    sf::Texture&    GetTexture();
    sf::Sprite&     GetSprite();
   
    void SetPosition(const sf::Vector2f& pos);
};
