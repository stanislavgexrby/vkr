#pragma once

namespace syngt {
namespace graphics {

constexpr int NS_Radius = 10;
constexpr int NS_NullTerminal = 2;
constexpr int NS_Down = 2;
constexpr int NullTerminalRadius = 2;

constexpr int HorizontalSkipFromBorder = 5;
constexpr int VerticalSkipFromBorder = 5;
constexpr int VerticalSkipForName = 20;

enum DrawObjectType {
    ctNilObject = -1,
    ctDrawObjectPoint = 0,
    ctDrawObjectExtendedPoint = 1,
    ctDrawObjectFirst = 2,
    ctDrawObjectLast = 3,
    ctDrawObjectTerminal = 4,
    ctDrawObjectNonTerminal = 5,
    ctDrawObjectMacro = 6
};

}
}