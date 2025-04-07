#pragma once

#include <deque>
#include <functional>

#include "core/SFMLRenderer.h"
#include "vendor/SFML/Graphics.hpp"
#include "common/sm_assert.h"

class dlDrawList
{
    static inline std::vector<std::function<void()>> dlList;

public:

    inline static sf::RenderWindow* getWindow()
    {
        return gSFMLRenderer.GetSfWindow();
    }

    inline static void DrawInvoke(const std::function<void()>& cmd)
    {
        dlList.emplace_back(cmd);
    }

    inline static void Execute()
    {
        if (dlList.empty())
            return;

		for (auto& call : dlList)
			call();
    
        dlList.clear();
    }
};
