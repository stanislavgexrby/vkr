#pragma once

namespace syngt {
namespace graphics {

// Размеры стрелок
constexpr int SpaceLength = 5;
constexpr int MinArrowLength = 2 * SpaceLength;
constexpr int SpikeLength = 7;
constexpr int SpikeWidth = 3;
constexpr int TextHeight = 15;

// Типы стрелок
enum ArrowType {
    ctArrow = 0,
    ctSemanticArrow = 1
};

}
}