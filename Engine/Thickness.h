#pragma once

namespace DE {
	namespace UI {
		enum class Anchor {
			None = 0,
			Left = 1,
			Top = 2,
			TopLeft = 3,
			Right = 4,
			StretchHorizontally = 5,
			TopRight = 6,
			TopDock = 7,
			Bottom = 8,
			BottomLeft = 9,
			StretchVertically = 10,
			LeftDock = 11,
			BottomRight = 12,
			BottomDock = 13,
			RightDock = 14,
			All = 15
		};
		struct Thickness {
			constexpr Thickness() : Left(0.0), Top(0.0), Right(0.0), Bottom(0.0) {
			}
			explicit constexpr Thickness(double uniLen) : Left(uniLen), Top(uniLen), Right(uniLen), Bottom(uniLen) {
			}
			constexpr Thickness(double l, double t, double r, double b) : Left(l), Top(t), Right(r), Bottom(b) {
			}

			double Left, Top, Right, Bottom;
		};
	}
}
