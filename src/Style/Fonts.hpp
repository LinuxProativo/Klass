/**
 * @file Fonts.hpp
 * @brief Header for system font discovery mechanisms.
 */

#ifndef FONTS_HPP
#define FONTS_HPP

#include <QFont>

/**
 * @class Fonts
 * @brief Manages system font discovery by parsing configuration files.
 */
class Fonts {
public:
    [[nodiscard]] static QFont getSystemFont();
};

#endif
