#pragma once

#include <deque>
#include <functional>

#include "base/SFMLRenderer.h"
#include "vendor/SFML/Graphics.hpp"
#include "common/sm_assert.h"

class dlDrawList
{
    static inline std::vector<std::function<void()>> dlDrawCommands;
public:

    inline static sf::RenderWindow* getWindow()
    {
        return g_SFMLRenderer.get_sfWindow();
    }

    inline static void DrawInvoke(const std::function<void()>& cmd)
    {
        dlDrawCommands.emplace_back(cmd);
    }

    inline static void Execute()
    {
        if (dlDrawCommands.empty())
            return;

		for (auto& call : dlDrawCommands)
			call();
    
        dlDrawCommands.clear();
    }
};


