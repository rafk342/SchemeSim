#pragma once
#include <cmath>
#include <stacktrace>
#include <iostream>
#include "vendor/SFML/Graphics.hpp"
#include "common/vfmt.h"
#include "vendor/Cougar/FixedSizeAllocator.h"

#define PI 3.14159265358979l

namespace Utils
{
    void InvertColors(sf::Image& image);
    void InvertTexture(sf::Texture& texture);
    std::optional<sf::IntRect> CalcTextureRect(const sf::Texture& texture);

    std::vector<std::string>    SplitString    (const std::string& input, const std::string& delimiters, uint16_t expected_vec_size = 16);
    std::string                 TrimString    (const std::string& str);
    
    void printStackTrace();
}



namespace math
{
    inline double NormalizeValue(double a, double b, double x)
    {
        return (x - a) / (b - a);
    }

    inline double mapRange(double value, double oldMin, double oldMax, double newMin, double newMax)
    {
        value = std::clamp(value, oldMin, oldMax);
		double normalized = NormalizeValue(oldMin, oldMax, value);
        return newMin + normalized * (newMax - newMin);
    }

    inline double easeInOutQuad(double t)
    {
        return t < 0.5 ? 2 * t * t : -1 + (4 - 2 * t) * t;
    }

    inline double easeInOutSine(double t)
    {
        return (1 - std::cos(t * PI)) / 2;
    }

    inline double easeInOutCubic(double t)
    {
        return t < 0.5 ? 4 * t * t * t : 1 + (--t) * (2 * (--t)) * (2 * t);
    }

    inline double sign(double x)
    {
		return x == 0.0 ? 0.0 : x / std::abs(x);
    }
}