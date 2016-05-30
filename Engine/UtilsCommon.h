#pragma once

namespace DE {
	namespace Utils {
		enum class Direction : unsigned {
			Left = 0x1,
			Up = 0x2,
			Right = 0x4,
			Down = 0x8
		};
		inline Direction Reverse(Direction d) {
			switch (d) {
				case Direction::Left: {
					return Direction::Right;
				}
				case Direction::Up: {
					return Direction::Down;
				}
				case Direction::Down: {
					return Direction::Up;
				}
				case Direction::Right: {
					return Direction::Left;
				}
			}
			throw Core::InvalidOperationException(_TEXT("invalid direction"));
		}
		inline Direction TurnRight(Direction d) {
			unsigned x = (unsigned)d;
            x <<= 1;
            if (x & (~0xF)) {
				x &= 0xF;
				x |= 0x1;
            }
            return (Direction)x;
		}
		inline Direction TurnLeft(Direction d) {
			unsigned x = (unsigned)d;
			if (x & 0x1) {
				x |= 0x10;
			}
			x >>= 1;
			return (Direction)x;
		}
	}
}