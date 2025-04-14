#include "Helpers.h"

void Utils::InvertColors(sf::Image& image)
{
    for (unsigned y = 0; y < image.getSize().y; ++y)
    {
        for (unsigned x = 0; x < image.getSize().x; ++x)
        {
            sf::Color color = image.getPixel(sf::Vector2u(x, y));
            color.r = 255 - color.r;
            color.g = 255 - color.g;
            color.b = 255 - color.b;
            image.setPixel({ x, y }, color);
        }
    }
}


void Utils::InvertTexture(sf::Texture& texture)
{
    sf::Image image = texture.copyToImage();
    InvertColors(image);
    texture.update(image);
}


std::optional<sf::IntRect> Utils::CalcTextureRect(const sf::Texture& texture)
{
    sf::Image image = texture.copyToImage();
    sf::Vector2u size = image.getSize();

    unsigned int left = size.x;
    unsigned int top = size.y;
    unsigned int right = 0;
    unsigned int bottom = 0;

    for (unsigned int y = 0; y < size.y; ++y)
    {
        for (unsigned int x = 0; x < size.x; ++x)
        {
            sf::Color pixel = image.getPixel({ x, y });
            if (pixel.a != 0)
            {
                if (x < left) left = x;
                if (y < top) top = y;
                if (x > right) right = x;
                if (y > bottom) bottom = y;
            }
        }
    }

    if (left < right && top < bottom)
    {
        sf::IntRect rect(sf::Vector2i(left, top), sf::Vector2i(right - left + 1, bottom - top + 1));
        return rect;
    }
    else
    {
        return std::nullopt;
    }
}


std::vector<std::string> Utils::SplitString(const std::string& input, const std::string& delimiters, uint16_t expected_vec_size)
{
    std::vector<std::string> elements;
    std::size_t start = 0, end = 0;
    elements.reserve(expected_vec_size);

    while ((end = input.find_first_of(delimiters, start)) != std::string::npos) {
        if (end != start) {
            elements.push_back(input.substr(start, end - start));
        }
        start = end + 1;
    }

    if (start < input.size()) {
        elements.push_back(input.substr(start));
    }

    return elements;
}


std::string Utils::TrimString(const std::string& str)
{
    if (str.empty())
        return str;

    size_t start_pos = 0;
    size_t end_pos = str.size() - 1;

    while (start_pos < str.size() && std::isspace(str[start_pos]))
        ++start_pos;

    while (end_pos > start_pos && std::isspace(str[end_pos]))
        --end_pos;

    return str.substr(start_pos, end_pos - start_pos + 1);
}


void Utils::printStackTrace()
{
    char buffer[5000];
    for (auto& frame : std::stacktrace::current())
    {
        std::memset(buffer, 0, std::size(buffer));
        auto r = std::format_to_n(  buffer, 
                                    std::size(buffer), 
                                    " {:<80}| Line: {:<15}| File: {:<15}\n", 
                                    frame.description(), frame.source_line(), frame.source_file() );

        *r.out = '\0';
        std::cout << buffer;
    }
}