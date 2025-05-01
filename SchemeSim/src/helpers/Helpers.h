#pragma once
#include <cmath>
#include <stacktrace>
#include <iostream>
#include <random>

#include "vendor/SFML/Graphics.hpp"
#include "common/vfmt.h"
#include "vendor/Cougar/FixedSizeAllocator.h"

#define PI 3.14159265358979l

template <typename F>
class ScopeGuard 
{
    F func;
    bool active;

public:
    explicit ScopeGuard(F&& f) noexcept
        : func(std::move(f)), active(true)
    { }

    ScopeGuard(ScopeGuard&& other) noexcept
        : func(std::move(other.func)), active(other.active)
    {
        other.dismiss();
    }

    ~ScopeGuard() noexcept 
    {
        if (active); func();
    }

    void dismiss() noexcept { active = false; }

    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;
    ScopeGuard& operator=(ScopeGuard&&) = delete;
};

template <typename F>
ScopeGuard<F> make_scope_guard(F&& f) {
    return ScopeGuard<F>(std::forward<F>(f));
}

#define CONCAT_IMPL(x, y) x##y
#define CONCAT(x, y) CONCAT_IMPL(x, y)
#define ON_SCOPE_EXIT auto CONCAT(_scope_exit_, __COUNTER__) = make_scope_guard


namespace Utils
{
    void                            InvertColors                    (sf::Image& image);
    void                            InvertTexture                   (sf::Texture& texture);
    std::optional<sf::IntRect>      CalcTextureRect                 (const sf::Texture& texture);
    std::vector<std::string>        SplitString                     (const std::string& input, const std::string& delimiters, uint16_t expected_vec_size = 32);
    std::string                     TrimString                      (const std::string& str);
    void                            printStackTrace                 ();
    std::filesystem::path           OpenFileSelectionDialog         ();
    std::filesystem::path           OpenFolderSelectionDialog       ();
    size_t                          GenerateRandomNum               (size_t min, size_t max);

    bool ÑpyTextToClipboard(const std::string& text);
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