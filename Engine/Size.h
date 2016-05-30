#pragma once

#include "Vector2.h"

namespace DE {
	namespace UI {
		struct Size {
			constexpr Size() = default;
			constexpr Size(double w, double h) : Width(w), Height(h) {
			}
			constexpr Size(const Core::Math::Vector2 &vec) : Width(vec.X), Height(vec.Y) {
			}

			operator Core::Math::Vector2() const {
				return Core::Math::Vector2(Width, Height);
			}

			double Width = 0.0, Height = 0.0;
		};
	}
}