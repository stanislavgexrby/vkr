#pragma once

namespace syngt {
namespace graphics {

constexpr int NS_Radius = 10;
constexpr int NS_NullTerminal = 2;
constexpr int NS_Down = 2;
constexpr int HorizontalSkipFromBorder = 5;
constexpr int VerticalSkipFromBorder = 5;
constexpr int VerticalSkipForName = 20;
constexpr int NullTerminalRadius = 2;
// Visual half-height of drawn elements (terminals/non-terminals) in pixels.
// Rendering draws these elements from center-ElementHalfHeight to center+ElementHalfHeight.
constexpr int ElementHalfHeight = NS_Radius * 2; // 20px
// Gap between bottom of one branch and top of the next branch's element.
// Must be > ElementHalfHeight so rows don't overlap.
constexpr int VerticalSpace = ElementHalfHeight + 7; // 27px

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