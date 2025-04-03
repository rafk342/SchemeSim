#pragma once
#include <iostream>
#include <print>
#include "SFML/Graphics.hpp"

//#include "scheme/Scheme.h"

class SFMLRenderer
{
    static inline SFMLRenderer* self = nullptr;
    std::unique_ptr<sf::RenderWindow> m_Window;
    sf::View                    m_view;
    sf::Font                    m_font;
    sf::Clock                   m_Clock;
    float                       m_frameTime = 0;
    float                       m_fps = 0;
    sf::Vector2f                m_DeltaMouse;
    
    SFMLRenderer() = default;
    void HandleEvents();

public:

    static SFMLRenderer*        Create();
    static SFMLRenderer*        Get();
    static void                 Destroy();

    SFMLRenderer*               Init();
    SFMLRenderer*               OnRender();
    
    sf::RenderWindow*           GetSfWindow();
    sf::View*                   GEtSfView();
    sf::Font&                   GetFont();
    sf::Vector2f                GetDeltaMouse();
    sf::Vector2f                GetWorldMousePos();
};

#define g_SFMLRenderer (*SFMLRenderer::Get())

