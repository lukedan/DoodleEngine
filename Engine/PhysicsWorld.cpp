#include "PhysicsWorld.h"
#include "Math.h"

namespace DE {
	namespace Physics {
		using namespace Core;
		using namespace Core::Math;

		World::~World() {
			for (unsigned i = 0; i < pars.Count(); ++i) {
				pars[i]->~ParticleBody();
				Core::GlobalAllocator::Free(pars[i]);
			}
			for (unsigned i = 0; i < segs.Count(); ++i) {
				segs[i]->~Segment();
				Core::GlobalAllocator::Free(segs[i]);
			}
		}

		ParticleBody *World::CreateParticleBody() {
			ParticleBody *p = new (Core::GlobalAllocator::Allocate(sizeof(ParticleBody))) ParticleBody();
			pars.PushBack(p);
			return p;
		}
		void World::DestroyParticleBody(ParticleBody *p) {
			size_t id = pars.FindFirst(p);
			if (id < pars.Count()) {
				throw InvalidArgumentException(_TEXT("the particle to destroy doesn't belong to this world"));
			}
			pars.SwapToBack(id);
			pars.PopBack();
			p->~ParticleBody();
			Core::GlobalAllocator::Free(p);
		}
		Segment *World::CreateSegment() {
			Segment *s = new (Core::GlobalAllocator::Allocate(sizeof(Segment))) Segment();
			segs.PushBack(s);
			return s;
		}
		void World::DestroySegment(Segment *s) {
			size_t id = segs.FindFirst(s);
			if (id < pars.Count()) {
				throw InvalidArgumentException(_TEXT("the particle to destroy doesn't belong to this world"));
			}
			segs.SwapToBack(id);
			segs.PopBack();
			s->~Segment();
			Core::GlobalAllocator::Free(s);
		}
		void World::Update(double delta) {
			/* Original code
			for (unsigned i = 0; i < pars.Count(); ++i) {
				Particle &p = *(pars[i]);
				if (p.Sleeping()) {
					continue;
				}
				Vector2 np, x, y, a, offset;
				if (p.col) {
					Segment &s = *p.col;
					Projection(g * delta, s.p2 - s.p1, x, a);
					// calculate friction
					x *= sqrt(s.fric * p.fric);
					if (Vector2::Cross(x, p.spd) < 0) {
						x.RotateLeft90();
					} else {
						x.RotateRight90();
					}
					if (p.spd.LengthSquared() <= x.LengthSquared()) {
						x = -p.spd;
					}
					a += x;
					if (Vector2::Dot(p.pos - s.p1, s.p2 - s.p1) <= 0 ||
						Vector2::Dot(p.pos - s.p2, s.p1 - s.p2) <= 0) {
						p.col = nullptr;
						//ShowMessage("Detach!\n");
					}
				} else {
					a = g * delta;
				}
				Vector2 oo = delta * (p.spd + 0.5 * a);
				np = p.pos + oo;
				Segment *nearest = nullptr;
				for (int i = 0; i < 10; ++i) {
					nearest = nullptr;
					for (unsigned j = 0; j < segs.Count(); ++j) {
						Segment &s = *(segs[j]);
						if (Intersect(p.pos, np, s.p1, s.p2, np) != NoIntersection) {
							offset = np - p.pos;
							x = ProjectionX(offset, s.p2 - s.p1);
							double l = x.Length();
							offset *= 1 - hp / l;
							np = p.pos + offset;
							nearest = &s;
						}
					}
					if (nearest && nearest != p.col) {
						p.col = nullptr;
						Vector2 dir = nearest->p2 - nearest->p1;
						a *= sqrt(offset.LengthSquared() / oo.LengthSquared()); // fix energy
						Projection(p.spd, dir, x, y); // new speed
						a -= x;
						x *= Max(p.rest, nearest->rest);
						//ShowMessage("Bump! ACCL=%f %f\n", a.X, a.Y);
						if (x.LengthSquared() <= cosq &&
							Vector2::Cross(g, dir) * Vector2::Det(p.pos - nearest->p1, dir) <= 0) {
							//ShowMessage("Entach!\n");
							p.col = nearest;
						} else {
							a -= x;
						}
					} else if (nearest == nullptr) {
						break;
					}
					//ShowMessage("RAN TO %d\n", i);
				}
				if (p.col) {
					// make it straight
					p.spd = ProjectionY(p.spd + a, p.col->p2 - p.col->p1);
				} else {
					p.spd += a;
				}
				if (p.spd.LengthSquared() <= cosq) {
					p.slpt += delta;
				} else {
					p.slpt = 0.0;
				}
				p.pos = np;
			}
			*/
			for (size_t i = 0; i < pars.Count(); ++i) {
				ParticleBody &p = *(pars[i]);
				if (p.Sleeping()) {
					continue;
				}

				Vector2
					scdA, // acceleration scaled with time
					bumpDt, // offset delta after bumping
					originalDt; // original delta
				if (p.col != nullptr) { // Calculate acceleration : it's sliding on something!
					Segment &s = *p.col;
					Vector2 Ff;
					Projection(g * delta, s.p2 - s.p1, Ff, scdA);
					Ff *= sqrt(s.fric * p.fric); // Calculate friction
					if (Vector2::Cross(Ff, p.spd) < 0) {
						Ff.RotateLeft90();
					} else {
						Ff.RotateRight90();
					}
					if (p.spd.LengthSquared() <= Ff.LengthSquared()) {
						Ff = -p.spd;
					}
					scdA += Ff;
					ShowMessage(_TEXT("CLP=%d "), p.clp);
					double da = Vector2::Dot(p.poss[p.clp] - p.pos - s.p1, s.p2 - s.p1) <= 0, db = Vector2::Dot(p.poss[p.clp] + p.pos - s.p2, s.p1 - s.p2) <= 0;
					ShowMessage(_TEXT("Dot result:%f %f\n"), da, db);
					if ( // The thing has slided OUT OF the platform!
						Vector2::Dot(p.poss[p.clp] + p.pos - s.p1, s.p2 - s.p1) < 0.0 ||
						Vector2::Dot(p.poss[p.clp] + p.pos - s.p2, s.p1 - s.p2) < 0.0
					) {
						ShowMessage(_TEXT("Detach!\n"));
						p.col = nullptr;
					}
				} else {
					scdA = g * delta;
				}
				originalDt = bumpDt = (p.spd + 0.5 * scdA * delta) * delta;

				Segment *col = nullptr; // collided with col
				size_t cp; // index of col
				//for (size_t repeat = 0; repeat < 100; ++repeat) { // Repeat 10(?) times
					for (size_t j = 0; j < p.poss.Count(); ++j) { // Each node in the PointBody
						Vector2 acP = p.poss[j] + p.pos, np = acP + bumpDt;
						for (size_t k = 0; k < segs.Count(); ++k) { // Each segment
							Segment &s = *(segs[k]);
							Vector2 tmpBPos;
							if (SegmentsIntersect(s.p1, s.p2, acP, np, tmpBPos) != IntersectionType::None) {
								bumpDt = tmpBPos - acP;
								col = &s; // record collision data
								cp = j;
								Vector2 fxOfs = ProjectionX(bumpDt, s.p2 - s.p1); // add small gap
								//ShowMessage(_TEXT("Gap multiply:%f\n"), 1 - hp / fxOfs.Length());
								bumpDt *= 1 - hp / fxOfs.Length();
								np = acP + bumpDt;
							}
						}
					}
					if (col && col != p.col) { // bounced against something
						ShowMessage(_TEXT("Bump!%p\n"), col);
						p.col = nullptr; // clear collision data
						Vector2 dir = col->p2 - col->p1, x, y;
						//scdA *= sqrt(bumpDt.LengthSquared() / originalDt.LengthSquared()); // fix energy(?)
						Projection(p.spd, dir, x, y); // new speed
						scdA -= x; // clear speed
						x *= Max(p.rest, col->rest); // scale
						if ( // it didn't bounce up
							x.LengthSquared() <= cosq &&
							Vector2::Cross(g, dir) * Vector2::Cross(p.poss[cp] - col->p1, dir) <= 0
						) {
							ShowMessage(_TEXT("Entach!%d\n"), cp);
							p.col = col;
							p.clp = cp;
						} else {
							scdA -= x;
						}
						Collide(CollideInfo(p.poss[cp] + p.pos, &p, col));
					} else if (col == nullptr) {
						//break;
					}
				//}

				p.spd += scdA;
				if (p.col) { // make its speed straight
					p.spd = ProjectionY(p.spd, p.col->p2 - p.col->p1);
				}
				// see if it's sleeping
				if (p.spd.LengthSquared() <= cosq) { // not moving
					p.slpt += delta;
				} else { // moving
					p.slpt = 0.0;
				}
				p.pos += bumpDt;
			}
		}
	}
}
