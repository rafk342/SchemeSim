#pragma once
#include <cmath>
#include "vendor/SFML/Graphics.hpp"

#define PI 3.14159265358979l

namespace Utils
{
    inline void InvertColors(sf::Image& image) {
        for (unsigned y = 0; y < image.getSize().y; ++y) {
            for (unsigned x = 0; x < image.getSize().x; ++x) {
                sf::Color color = image.getPixel(x, y);
                color.r = 255 - color.r;
                color.g = 255 - color.g;
                color.b = 255 - color.b;
                image.setPixel(x, y, color);
            }
        }
    }

    inline void InvertTexture(sf::Texture& texture)
    {
        sf::Image image = texture.copyToImage();
        InvertColors(image);
        texture.update(image);
    }

	inline std::optional<sf::IntRect> CalcTextureRect(const sf::Texture& texture)
    {
        sf::Image image = texture.copyToImage();
        sf::Vector2u size = image.getSize();

        unsigned int left = size.x;
        unsigned int top = size.y;
        unsigned int right = 0;
        unsigned int bottom = 0;

        for (unsigned int y = 0; y < size.y; ++y) {
            for (unsigned int x = 0; x < size.x; ++x) {
                sf::Color pixel = image.getPixel(x, y);
                if (pixel.a != 0) { 
                    if (x < left) left = x;
                    if (y < top) top = y;
                    if (x > right) right = x;
                    if (y > bottom) bottom = y;
                }
            }
        }

        if (left < right && top < bottom) {
            sf::IntRect rect(left, top, right - left + 1, bottom - top + 1);
			return rect;
        } else {
			return std::nullopt;
        }
    }

    std::vector<std::string>    split_string    (const std::string& input, const std::string& delimiters, uint16_t expected_vec_size = 16);
    std::string                 strip_string    (const std::string& str);
}

namespace math
{
    inline float vGetEulerAngleBetween(const sf::Vector2f& p1, const sf::Vector2f& p2)
    {
        return std::atan2(p2.y - p1.y, p2.x - p1.x) * 180.0f / PI;
    }

    inline float vGetLength(const sf::Vector2f& p1, const sf::Vector2f& p2)
    {
        return std::sqrt(std::pow(p2.x - p1.x, 2) + std::pow(p2.y - p1.y, 2));
    }

    inline double NormalizeValue(double a, double b, double x)
    {
        return (x - a) / (b - a);
    }

    inline double mapRange(double value, double oldMin, double oldMax, double newMin, double newMax)
    {
        value = std::clamp(value, oldMin, oldMax);
        double normalized = (value - oldMin) / (oldMax - oldMin);
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
}