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

    inline double easeInOutQuad(double x)
    {
        return x < 0.5 ? 8.0 * std::pow(x, 4.0) : 1.0 - std::pow(-2.0 * x + 2.0, 4.0) / 2.0;
    }

    inline double easeInOutSine(double t)
    {
        return (1.0 - std::cos(t * PI)) / 2.0;
    }

    inline double easeInOutCubic(double x) 
    {
        return x < 0.5 ? 4.0 * std::pow(x, 3.0) : 1.0 - std::pow(-2.0 * x + 2.0, 3.0) / 2.0;
    }

    inline double sign(double x)
    {
		return x == 0.0 ? 0.0 : x / std::abs(x);
    }
}