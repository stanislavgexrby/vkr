#pragma once

namespace syngt {
namespace graphics {

// Размеры элементов
constexpr int NS_Radius = 10;
constexpr int NS_NullTerminal = 2;
constexpr int NS_Down = 2;
constexpr int HorizontalSkipFromBorder = 5;
constexpr int VerticalSkipFromBorder = 5;
constexpr int VerticalSkipForName = 20;
constexpr int NullTerminalRadius = 2;
constexpr int VerticalSpace = NS_Radius + 7;

// Типы DrawObject
constexpr int ctNilObject = -1;
constexpr int ctDrawObjectPoint = 0;
constexpr int ctDrawObjectExtendedPoint = 1;
constexpr int ctDrawObjectFirst = 2;
constexpr int ctDrawObjectLast = 3;
constexpr int ctDrawObjectTerminal = 4;
constexpr int ctDrawObjectNonTerminal = 5;
constexpr int ctDrawObjectMacro = 6;

} // namespace graphics
} // namespace syngt