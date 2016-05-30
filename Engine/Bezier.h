#pragma once

#include "Vector2.h"
#include "List.h"

namespace DE {
	namespace Utils {
		class Bezier {
			public:
				Bezier() = default;
				Bezier(
					const Core::Math::Vector2 &node1, const Core::Math::Vector2 &node2,
					const Core::Math::Vector2 &ctrl1, const Core::Math::Vector2 &ctrl2
				) : _s1(node1), _s2(node2), _c1(ctrl1), _c2(ctrl2) {
				}

				Core::Math::Vector2 operator [](double stat) const {
					return At(stat);
				}
				Core::Math::Vector2 At(double stat) const {
					double oms = 1.0 - stat;
					return
						Core::Math::Square(stat) * (stat * _s2 + 3 * oms * _c2) +
						Core::Math::Square(oms) * (3 * stat * _c1 + oms * _s1);
				}

				Core::Collections::List<Core::Math::Vector2> GetLine(size_t split) const {
					if (split < 1) {
						throw Core::InvalidArgumentException(_TEXT("invalid part count"));
					}
					double step = 1.0 / split;
					--split;
					Core::Collections::List<Core::Math::Vector2> vs;
					vs.PushBack(_s1);
					double cur = step;
					for (size_t i = 0; i < split; ++i, cur += step) {
						vs.PushBack(At(cur));
					}
					vs.PushBack(_s2);
					return vs;
				}

				const Core::Math::Vector2 &Node1() const {
					return _s1;
				}
				Core::Math::Vector2 &Node1() {
					return _s1;
				}

				const Core::Math::Vector2 &Node2() const {
					return _s2;
				}
				Core::Math::Vector2 &Node2() {
					return _s2;
				}

				const Core::Math::Vector2 &Control1() const {
					return _c1;
				}
				Core::Math::Vector2 &Control1() {
					return _c1;
				}

				const Core::Math::Vector2 &Control2() const {
					return _c2;
				}
				Core::Math::Vector2 &Control2() {
					return _c2;
				}
			private:
				Core::Math::Vector2 _s1, _s2, _c1, _c2;
		};
	}
}