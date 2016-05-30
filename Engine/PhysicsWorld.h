#pragma once

#include "ParticleBody.h"
#include "Segment.h"
#include "ObjectAllocator.h"
#include "List.h"
#include "Vector2.h"
#include "Event.h"

namespace DE {
	namespace Physics {
		class ParticleBody;
		class Segment;
		struct CollideInfo {
			public:
				CollideInfo() = default;
				CollideInfo(const Core::Math::Vector2 &p, const ParticleBody *body, const Segment *segment) : pos(p), bd(body), seg(segment) {
				}

				const Core::Math::Vector2 &GetPosition() const {
					return pos;
				}
				const ParticleBody *GetBody() const {
					return bd;
				}
				const Segment *GetSegment() const {
					return seg;
				}
			private:
				Core::Math::Vector2 pos;
				const ParticleBody *bd = nullptr;
				const Segment *seg = nullptr;
		};
		class World {
			public:
				World() : hp(0.5) {
					cosq = 2.0 * g.Length() * hp;
				}
				World(const Core::Math::Vector2 &gravity) : g(gravity), hp(0.5) {
					cosq = 2.0 * g.Length() * hp;
				}
				~World();

				ParticleBody *CreateParticleBody();
				void DestroyParticleBody(ParticleBody*);
				Segment *CreateSegment();
				void DestroySegment(Segment*);

				void Update(double);

				const Core::Collections::List<ParticleBody*> &ParticleBodies() const {
					return pars;
				}
				const Core::Collections::List<Segment*> &Segments() const {
					return segs;
				}

				Core::Math::Vector2 &Gravity() {
					return g;
				}
				const Core::Math::Vector2 &Gravity() const {
					return g;
				}

				Core::Event<CollideInfo> Collide;
			private:
				Core::Math::Vector2 g;
				Core::Collections::List<ParticleBody*> pars;
				Core::Collections::List<Segment*> segs;
				double hp, cosq;
		};
	}
}