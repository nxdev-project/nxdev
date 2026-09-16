#pragma once

#include <nxdev/types.hpp>
#include <string>
#include <string_view>
#include <cstdint>

namespace nxdev::ui {

/**
 * @brief Floating point RGBA color normalized in [0.0, 1.0].
 */
struct Color {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;

    constexpr Color() noexcept = default;
    constexpr Color(float red, float green, float blue, float alpha = 1.0f) noexcept
        : r(red), g(green), b(blue), a(alpha) {}

    static constexpr Color from_rgba(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255) noexcept {
        return Color(red / 255.0f, green / 255.0f, blue / 255.0f, alpha / 255.0f);
    }

    static constexpr Color from_hex(uint32_t hex) noexcept {
        return from_rgba(
            static_cast<uint8_t>((hex >> 24) & 0xFF),
            static_cast<uint8_t>((hex >> 16) & 0xFF),
            static_cast<uint8_t>((hex >> 8) & 0xFF),
            static_cast<uint8_t>(hex & 0xFF)
        );
    }

    static constexpr Color from_rgb_hex(uint32_t hex) noexcept {
        return from_rgba(
            static_cast<uint8_t>((hex >> 16) & 0xFF),
            static_cast<uint8_t>((hex >> 8) & 0xFF),
            static_cast<uint8_t>(hex & 0xFF),
            255
        );
    }

    constexpr bool operator==(const Color& other) const noexcept = default;

    // Common color constants
    static constexpr Color White() noexcept { return Color(1.0f, 1.0f, 1.0f, 1.0f); }
    static constexpr Color Black() noexcept { return Color(0.0f, 0.0f, 0.0f, 1.0f); }
    static constexpr Color Transparent() noexcept { return Color(0.0f, 0.0f, 0.0f, 0.0f); }
    static constexpr Color Red() noexcept { return Color(1.0f, 0.0f, 0.0f, 1.0f); }
    static constexpr Color Green() noexcept { return Color(0.0f, 1.0f, 0.0f, 1.0f); }
    static constexpr Color Blue() noexcept { return Color(0.0f, 0.0f, 1.0f, 1.0f); }
};

/**
 * @brief 2D Point.
 */
struct Point {
    float x = 0.0f;
    float y = 0.0f;

    constexpr Point() noexcept = default;
    constexpr Point(float px, float py) noexcept : x(px), y(py) {}
    constexpr bool operator==(const Point& other) const noexcept = default;
};

/**
 * @brief 2D Size.
 */
struct Size {
    float width = 0.0f;
    float height = 0.0f;

    constexpr Size() noexcept = default;
    constexpr Size(float w, float h) noexcept : width(w), height(h) {}
    constexpr bool operator==(const Size& other) const noexcept = default;
};

/**
 * @brief 2D Rectangle.
 */
struct Rect {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    constexpr Rect() noexcept = default;
    constexpr Rect(float rx, float ry, float rw, float rh) noexcept
        : x(rx), y(ry), width(rw), height(rh) {}
    constexpr bool operator==(const Rect& other) const noexcept = default;
};

/**
 * @brief Margin / padding insets.
 */
struct Insets {
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;
    float left = 0.0f;

    constexpr Insets() noexcept = default;
    constexpr Insets(float all) noexcept : top(all), right(all), bottom(all), left(all) {}
    constexpr Insets(float vertical, float horizontal) noexcept
        : top(vertical), right(horizontal), bottom(vertical), left(horizontal) {}
    constexpr Insets(float t, float r, float b, float l) noexcept
        : top(t), right(r), bottom(b), left(l) {}
    constexpr bool operator==(const Insets& other) const noexcept = default;
};

/**
 * @brief Layout flow direction for containers.
 */
enum class Direction {
    Row,
    Column
};

/**
 * @brief Flexbox alignment options.
 */
enum class Align {
    Start,
    Center,
    End,
    Stretch,
    SpaceBetween,
    SpaceAround,
    SpaceEvenly
};

/**
 * @brief Controller and navigation action types.
 */
enum class Action {
    Confirm,
    Back,
    Menu,
    Context,
    Left,
    Right,
    Up,
    Down,
    Alt1,
    Alt2
};

/**
 * @brief UI Theme appearance.
 */
enum class ThemeVariant {
    Auto,
    Light,
    Dark
};

/**
 * @brief Horizontal text alignment.
 */
enum class TextAlignment {
    Left,
    Center,
    Right
};

/**
 * @brief Image scaling and fitting mode.
 */
enum class ImageScaleMode {
    Fit,
    Fill,
    Crop,
    Center
};

} // namespace nxdev::ui
