#pragma once

#include "List.h"
#include "PhysicsWorld.h"
#include "Segment.h"

namespace DE {
	namespace Physics {
		class ParticleBody {
				friend class World;
			public:
				ParticleBody(const ParticleBody&) = delete;
				ParticleBody &operator =(const ParticleBody&) = delete;

				void SetPositions(const Core::Collections::List<Core::Math::Vector2> &ps) {
				    slpt = 0.0;
				    col = nullptr;
					poss = ps;
				}
				const Core::Collections::List<Core::Math::Vector2> &GetPositions() const {
					return poss;
				}
				void SetPosition(const Core::Math::Vector2 &p) {
					pos = p;
					col = nullptr;
					slpt = 0.0;
				}
				const Core::Math::Vector2 &GetPosition() const {
					return pos;
				}
				void SetSpeed(const Core::Math::Vector2 &s) {
					spd = s;
					slpt = 0.0;
				}
				const Core::Math::Vector2 &GetSpeed() const {
					return spd;
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

				bool Sleeping() const {
					return dslp && slpt > 1.0;
				}
				bool DoSleep() const {
					return dslp;
				}
				void SetSleep(bool ability) {
					if (!ability) {
						slpt = 0.0;
					}
					dslp = ability;
				}

				const Segment *Attached() const {
					return col;
				}
			private:
				ParticleBody() = default;

				Core::Collections::List<Core::Math::Vector2> poss;
				Core::Math::Vector2 pos, spd;
				Segment *col = nullptr;
				int clp = 0;
				double rest = 0.0, fric = 0.0, slpt = 0.0;
				bool dslp = true;
		};
	}
}
