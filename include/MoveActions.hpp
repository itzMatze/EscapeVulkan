#pragma once
#include <cstdint>

namespace MoveActionFlags
{
    using type = uint32_t;
    enum : uint32_t
    {
        NoMove =           0,
        RotateUp =         1 << 0,
        RotateDown =       1 << 1,
        RotateLeft =       1 << 2,
        RotateRight =      1 << 3,
        RollLeft =         1 << 4,
        RollRight =        1 << 5,
        MoveForward =      1 << 6,
        MoveBackward =     1 << 7,
        MoveLeft =         1 << 8,
        MoveRight =        1 << 9,
        MoveUp =           1 << 10,
        MoveDown =         1 << 11,
        ActionCount = 13
    };
}

