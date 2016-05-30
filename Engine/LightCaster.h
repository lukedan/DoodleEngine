#pragma once

#include "Math.h"
#include "Vector2.h"
#include "List.h"

namespace DE {
	namespace Utils {
		namespace LightCaster {
			struct Light {
				Core::Math::Vector2 Position;
				double Strength = 200.0;
			};
			struct Wall {
				Core::Math::Vector2 Node1, Node2;
				bool IsMirror = false;
			};
			enum class SplitType {
				FromSource,
				Reflected
			};
			struct CastResult {
				Core::Math::Vector2 SourcePoint1, SourcePoint2, TargetPoint1, TargetPoint2;
				double SourceStrength1, SourceStrength2, TargetStrength1, TargetStrength2;
				SplitType Type;
#ifdef DEBUG
				int Father = -1; // DEBUG
#endif
			};

			class Caster {
				public:
					Core::Collections::List<CastResult> Cast(const Light&, size_t = 50) const;

					Core::Collections::List<Wall> &Walls() {
						return _walls;
					}
					const Core::Collections::List<Wall> &Walls() const {
						return _walls;
					}
				private:
					class DirectionComparer {
						public:
							static int Compare(const Core::Math::Vector2 &lhs, const Core::Math::Vector2 &rhs) {
								// this is RIGHT because this is quick sort
								double dv = Core::Math::Vector2::Cross(lhs, rhs);
								return (dv > 0 ? 1 : (dv < 0 ? -1 : 0));
							}
					};
					Core::Collections::List<Wall> _walls;
			};
		}
	}
}
