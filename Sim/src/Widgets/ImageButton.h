#pragma once

#include "vendor/SFML/Graphics.hpp"
#include "base/WidgetsBase.h"


class WidgetsBase;

class ImageButton : public WidgetsBase
{
protected:

    sf::IntRect InactiveSpriteRect{};
    sf::IntRect ActiveSpriteRect{};
    sf::Color hovered_color = { 212, 212, 212 };
    sf::Color unhovered_color = { 255, 255, 255 };
    bool was_pressed_over_the_button = false;
    bool was_pressed = false;
    int id = -1;  //Can be unused

    bool ButtonBehavior();

public:
    //using WidgetsBase::WidgetsBase;
    ImageButton(std::string path, bool genMips = true);

    void SetInactiveImageRectSprite(const sf::IntRect& rect);
    void SetActiveImageRectSprite(const sf::IntRect& rect);
    void SetHoveredTint(sf::Color col);
    void SetUnhoveredTint(sf::Color col);
    void SetId(int v);
    int  GetId();
    operator bool();
};



class TwoStatesButton : public ImageButton
{
    bool m_current_state = false;
    bool m_lock = false;
    bool ButtonBehavior();

public:
    using ImageButton::ImageButton;

    operator bool();
    bool getState();
    void SetTrueStateSpriteRect(const sf::IntRect& rect);
    void SetFalseStateSpriteRect(const sf::IntRect& rect);
    void lock();
    void unlock();
    bool isLocked();
    bool SetState(bool state);
};

