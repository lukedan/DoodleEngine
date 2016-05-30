#pragma once

#include "PhysicsWorld.h"
#include "Vector2.h"

namespace DE {
	namespace Physics {
		class Segment {
				friend class World;
			public:
				Segment(const Segment&) = delete;
				Segment &operator =(const Segment&) = delete;

				Core::Math::Vector2 &Node1() {
					return p1;
				}
				const Core::Math::Vector2 &Node1() const {
					return p1;
				}
				Core::Math::Vector2 &Node2() {
					return p2;
				}
				const Core::Math::Vector2 &Node2() const {
					return p2;
				}

				double &Restitution() {
					return rest;
				}
				const double Restitution() const {
					return rest;
				}
				double &Friction() {
					return fric;
				}
				const double Friction() const {
					return fric;
				}
			private:
				Segment() = default;

				Core::Math::Vector2 p1, p2;
				double rest = 0.0, fric = 0.0;
		};
	}
}