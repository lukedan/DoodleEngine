#include "LightCaster.h"

#include "Queue.h"

namespace DE {
	namespace Utils {
		namespace LightCaster {
			using namespace Core;
			using namespace Core::Math;
			using namespace Core::Collections;

			struct CastTempNode {
				const Wall *_wallPtr;
				Vector2 _source, _dir1, _dir2, _isc1, _isc2, _pow;
				double _sd1, _sd2;
#ifdef DEBUG
				int _src = -1; // DEBUG
#endif
			};
			List<CastResult> Caster::Cast(const Light &light, size_t split) const {
				List<Vector2> poss; // RELATIVE positions
				for (size_t i = 0; i < _walls.Count(); ++i) { // wall breaks
					const Wall &curWall = _walls[i];
					size_t jtN = 0;
					Vector2 js[2];
					if (CircleSegmentIntersect(light.Position, light.Strength, curWall.Node1, curWall.Node2, jtN, js[0], js[1]) != IntersectionType::Full) {
						continue;
					}
					poss.PushBack(curWall.Node1 - light.Position);
					poss.PushBack(curWall.Node2 - light.Position);
					for (size_t j = 0; j < jtN; ++j) {
						poss.PushBack(js[j] - light.Position);
					}
					for (size_t j = 0; j < i; ++j) {
						Vector2 jt;
						if (SegmentsIntersect(
							curWall.Node1, curWall.Node2, _walls[j].Node1, _walls[j].Node2, jt
						) != IntersectionType::None) {
							if ((jt - light.Position).LengthSquared() <= light.Strength * light.Strength) {
								poss.PushBack(jt - light.Position);
							}
						}
					}
				}
				double angle = 2.0 * Pi / split;
				Vector2 rotVec(cos(angle), sin(angle));
				poss.PushBack(Vector2(light.Strength, 0.0));
				for (size_t i = 1; i < split; ++i) { // least fixed parts
					Vector2 last = poss.Last();
					poss.PushBack(Vector2(last.X * rotVec.X - last.Y * rotVec.Y, last.X * rotVec.Y + last.Y * rotVec.X));
				}
				UnstableSort<List<Vector2>, Vector2, DirectionComparer>(poss);
				size_t rpP = 0;
				for (size_t i = 1; i < poss.Count(); ++i) {
					if (AngleSinSquared(poss[rpP], poss[i]) >= 1e-6) {
						poss[++rpP] = poss[i];
					}
				}
				++rpP;
				poss.Remove(rpP, poss.Count() - rpP);
				List<CastResult> results;
				Queue<CastTempNode> mirrorCast;
				for (size_t i = 0; i < poss.Count(); ++i) {
					CastResult curResult;
					curResult.Type = SplitType::FromSource;
					curResult.SourcePoint1 = light.Position;
					curResult.SourceStrength1 = curResult.SourceStrength2 = 1.0;
					size_t j = i + 1;
					if (j == poss.Count()) {
						j = 0;
					}
					const Vector2 &posi = poss[i], posj = poss[j];
					Vector2 mid = posi + posj, hitwallp;
					size_t nearW = _walls.Count();
					double minLSq = light.Strength * light.Strength;
					for (size_t k = 0; k < _walls.Count(); ++k) {
						Vector2 jt;
						if (RaySegmentIntersect(light.Position, mid, _walls[k].Node1, _walls[k].Node2, jt) != IntersectionType::None) {
							double lenSq = (jt - light.Position).LengthSquared();
							if (lenSq <= minLSq) {
								minLSq = lenSq;
								nearW = k;
								hitwallp = jt;
							}
						}
					}
					if (nearW == _walls.Count()) {
						Vector2 v = posi;
						v.SetLength(light.Strength);
						curResult.TargetPoint1 = v + light.Position;
						curResult.TargetStrength1 = 0.0;
						v = posj;
						v.SetLength(light.Strength);
						curResult.TargetPoint2 = v + light.Position;
						curResult.TargetStrength2 = 0.0;
					} else {
						Vector2 jt;
						const Wall &curWall = _walls[nearW];
						if (RayLineIntersect(light.Position, posi, curWall.Node1, curWall.Node2 - curWall.Node1, jt) != IntersectionType::None) {
							curResult.TargetPoint1 = jt;
							curResult.TargetStrength1 = Max(0.0, 1.0 - sqrt((jt - light.Position).LengthSquared() / (light.Strength * light.Strength)));
						} else {
							throw SystemException(_TEXT("floating point error"));
						}
						if (RayLineIntersect(light.Position, posj, curWall.Node1, curWall.Node2 - curWall.Node1, jt) != IntersectionType::None) {
							curResult.TargetPoint2 = jt;
							curResult.TargetStrength2 = Max(
								0.0,
								1.0 - sqrt((jt - light.Position).LengthSquared() / (light.Strength * light.Strength))
							);
						} else {
							throw SystemException(_TEXT("floating point error"));
						}
						if (curWall.IsMirror) {
							CastTempNode n;
							n._wallPtr = &curWall;
							n._source = light.Position;
							n._dir1 = posi;
							n._dir2 = posj;
							n._isc1 = curResult.TargetPoint1;
							n._isc2 = curResult.TargetPoint2;
							n._sd1 = curResult.TargetStrength1;
							n._sd2 = curResult.TargetStrength2;
#ifdef DEBUG
							n._src = results.Count();
#endif
							n._pow = hitwallp;
							mirrorCast.PushTail(n);
						}
					}
					results.PushBack(curResult);
				}

				// cast mirrors
				while (!mirrorCast.Empty()) {
					CastTempNode n = mirrorCast.PopHead();
					const Wall &hitWall = *(n._wallPtr);
					Vector2
						wallDir12 = hitWall.Node2 - hitWall.Node1,
						mirrorOri = Mirror(n._source, hitWall.Node1, wallDir12),
						mirrorDir1 = n._isc1 - mirrorOri,
						mirrorDir2 = n._isc2 - mirrorOri;
					List<const Wall*> valWals;
					List<Vector2> relBreaks;
					_walls.ForEach([&](const Wall &curWall) { // filter the walls
						if ((&curWall) == (&hitWall)) {
							return true;
						}
						double
							detd1w1 = Vector2::Cross(mirrorDir1, curWall.Node1 - mirrorOri),
							detd1w2 = Vector2::Cross(mirrorDir1, curWall.Node2 - mirrorOri),
							detd2w1 = Vector2::Cross(mirrorDir2, curWall.Node1 - mirrorOri),
							detd2w2 = Vector2::Cross(mirrorDir2, curWall.Node2 - mirrorOri);
						bool
							xBorder = false, // whether the wall is crossing the outer border of the circular region
							xD1 = detd1w1 * detd1w2 <= 0.0, // whether the current wall crosses the first direction of light
							xD2 = detd2w1 * detd2w2 <= 0.0; // whether the current wall crosses the second direction of light
						if ((!xD1) && (!xD2)) { // crosses no border
							if (detd1w1 * detd2w1 > 0.0) { // out of range - 1st node is not between the two directions
								return true;
							}
							// the wall is either completely lighten by the beam, or on the wrong side
							Vector2 isect;
							if (SegmentsIntersect(
								curWall.Node1, mirrorOri, hitWall.Node1, hitWall.Node2, isect
							) == IntersectionType::None) { // the wall is on the wrong side!
								return true;
							}
							// NOTE one more test can be performed
							Vector2 rel1 = curWall.Node1 - mirrorOri, rel2 = curWall.Node2 - mirrorOri;
							bool
								out1 = rel1.LengthSquared() > Square(light.Strength),
								out2 = rel2.LengthSquared() > Square(light.Strength);
							if (out1 && out2) { // the wall is too faraway
								return true;
							}
							if (!out1) {
								relBreaks.PushBack(rel1);
							}
							if (!out2) {
								relBreaks.PushBack(rel2);
							}
							xBorder = (out1 != out2);
						} else if (xD1 && xD2) { // the wall either blocks the beam entirely, or is on the wrong side
							Vector2 midpt = (n._isc1 + n._isc2) * 0.5, testInter;
							if (RayLineIntersect(
								midpt, midpt - mirrorOri, curWall.Node1, curWall.Node1 - curWall.Node2, testInter
							) == IntersectionType::None) { // see if the wall is on the right side
								return true;
							}
							Vector2 isect1, isect2;
							if (RaySegmentIntersect(
								mirrorOri, mirrorDir1, curWall.Node1, curWall.Node2, isect1
							) == IntersectionType::None) {
								// NOTE a floating point error has occurred
								return true;
							}
							if (RaySegmentIntersect(
								mirrorOri, mirrorDir2, curWall.Node1, curWall.Node2, isect2
							) == IntersectionType::None) {
								// NOTE a floating point error has occurred
								return true;
							}
							bool
								out1 = (isect1 - mirrorOri).LengthSquared() > Square(light.Strength),
								out2 = (isect2 - mirrorOri).LengthSquared() > Square(light.Strength);
							if (out1 && out2) { // too faraway
								return true;
							}
							xBorder = (out1 != out2);
						} else { // cross an edge
							Vector2
								testNode = (detd1w1 * detd2w1 > detd1w2 * detd2w2 ? curWall.Node2 : curWall.Node1),
								testDir = (xD1 ? mirrorDir1 : mirrorDir2), dirIs;
							if (RayLineIntersect(
								mirrorOri, testDir, curWall.Node1, curWall.Node1 - curWall.Node2, dirIs
							) == IntersectionType::None) { // wrong side!
								return true;
							}
							if (LineSegmentIntersect(
								hitWall.Node1, hitWall.Node1 - hitWall.Node2, (dirIs + testNode) * 0.5, mirrorOri
							) == IntersectionType::None) { // wrong side!
								return true;
							}

							bool
								nodeInRange = (testNode - mirrorOri).LengthSquared() < Square(light.Strength),
								dirInRange = (dirIs - mirrorOri).LengthSquared() < Square(light.Strength);
							if ((!nodeInRange) && (!dirInRange)) {
								return true;
							}
							if (nodeInRange) {
								relBreaks.PushBack(testNode - mirrorOri);
							}
							xBorder = (nodeInRange != dirInRange);
						}
						if (SegmentsIntersect(
							0.5 * (n._isc1 + n._isc2), mirrorOri, curWall.Node1, curWall.Node2
						) != IntersectionType::None) { // solves the bug when the beam is just overlapping with the wall
							return true;
						}
						valWals.PushBack(&curWall);
						if (xBorder) {
							Vector2 d1 = mirrorDir1, d2 = mirrorDir2, isect;
							d1.SetLength(light.Strength);
							d2.SetLength(light.Strength);
							if (SegmentsIntersect(
								d1 + mirrorOri, d2 + mirrorOri, curWall.Node1, curWall.Node2, isect
							) == IntersectionType::None) {
								// NOTE a floating point error has occurred
								return true;
							}
							relBreaks.PushBack(isect - mirrorOri);
						}
						return true;
					});
					for (size_t i = 0; i < valWals.Count(); ++i) {
						const Wall &walli = *(valWals[i]);
						for (size_t j = i + 1; j < valWals.Count(); ++j) {
							const Wall &wallj = *(valWals[j]);
							Vector2 isect;
							if (SegmentsIntersect(walli.Node1, walli.Node2, wallj.Node1, wallj.Node2, isect) != IntersectionType::None) {
								Vector2 relDir = isect - mirrorOri;
								if (Vector2::Cross(relDir, mirrorDir1) * Vector2::Cross(relDir, mirrorDir2) > 0.0) {
									continue;
								}
								if (relDir.LengthSquared() > Square(light.Strength)) {
									continue;
								}
								if (SegmentsIntersect(hitWall.Node1, hitWall.Node2, isect, mirrorOri) == IntersectionType::None) {
									continue;
								}
								relBreaks.PushBack(relDir);
							}
						}
					}
					relBreaks.PushBack(mirrorDir1);
					relBreaks.PushBack(mirrorDir2);
					UnstableSort<List<Vector2>, Vector2, DirectionComparer>(relBreaks);
					size_t rpP = 0;
					for (size_t i = 1; i < relBreaks.Count(); ++i) {
						if (AngleSinSquared(relBreaks[rpP], relBreaks[i]) >= 1e-6) { // NOTE magic number
							relBreaks[++rpP] = relBreaks[i];
						}
					}
					++rpP;
					relBreaks.Remove(rpP, relBreaks.Count() - rpP);
					Vector2 lastdir = relBreaks[0], lasthitp;
					RayLineIntersect(mirrorOri, lastdir, hitWall.Node1, wallDir12, lasthitp);
					for (size_t i = 1; i < relBreaks.Count(); ++i) {
						CastResult curResult;
						Vector2 curdir = relBreaks[i], dir = lastdir + curdir, curhitp;
						RayLineIntersect(mirrorOri, curdir, hitWall.Node1, wallDir12, curhitp);
						curResult.Type = SplitType::Reflected;
						curResult.SourcePoint1 = lasthitp;
						curResult.SourcePoint2 = curhitp;
						curResult.SourceStrength1 = Max(0.0, 1.0 - (lasthitp - mirrorOri).Length() / light.Strength);
						curResult.SourceStrength2 = Max(0.0, 1.0 - (curhitp - mirrorOri).Length() / light.Strength);
						Vector2 hitwallp;
						const Wall *mdw = nullptr;
						double msqd;
						valWals.ForEach([&](const Wall *cw) {
							Vector2 jt;
							if (RaySegmentIntersect(mirrorOri, dir, cw->Node1, cw->Node2, jt) != IntersectionType::None) {
								double csqd = (jt - mirrorOri).LengthSquared();
								if (csqd > Square(light.Strength)) {
									return true;
								}
								if (mdw == nullptr || csqd < msqd) {
									mdw = cw;
									msqd = csqd;
									hitwallp = jt;
								}
							}
							return true;
						});
						bool valid = true;
						if (mdw == nullptr) { // didn't hit anything
							curResult.TargetPoint1 = lastdir;
							curResult.TargetPoint2 = curdir;
							curResult.TargetPoint1.SetLength(light.Strength);
							curResult.TargetPoint2.SetLength(light.Strength);
							curResult.TargetPoint1 += mirrorOri;
							curResult.TargetPoint2 += mirrorOri;
							curResult.TargetStrength1 = curResult.TargetStrength2 = 0.0;
						} else {
							if (RayLineIntersect(
								mirrorOri, lastdir, mdw->Node1, mdw->Node2 - mdw->Node1, curResult.TargetPoint1
							) == IntersectionType::None) {
								// NOTE a floating point error has occurred
								valid = false;
							}
							if (valid && RayLineIntersect(
								mirrorOri, curdir, mdw->Node1, mdw->Node2 - mdw->Node1, curResult.TargetPoint2
							) == IntersectionType::None) {
								// NOTE a floating point error has occurred
								valid = false;
							}
							if (valid) {
								curResult.TargetStrength1 = Max(0.0, 1.0 - (curResult.TargetPoint1 - mirrorOri).Length() / light.Strength);
								curResult.TargetStrength2 = Max(0.0, 1.0 - (curResult.TargetPoint2 - mirrorOri).Length() / light.Strength);
								if (mdw->IsMirror) {
									CastTempNode n;
									n._wallPtr = mdw;
									n._source = mirrorOri;
									n._dir1 = lastdir;
									n._dir2 = curdir;
									n._isc1 = curResult.TargetPoint1;
									n._isc2 = curResult.TargetPoint2;
									n._sd1 = curResult.TargetStrength1;
									n._sd2 = curResult.TargetStrength2;
#ifdef DEBUG
									n._src = results.Count();
#endif
									n._pow = hitwallp;
									mirrorCast.PushTail(n);
								}
							}
						}
						if (valid) {
#ifdef DEBUG
							curResult.Father = n._src;
#endif
							results.PushBack(curResult);
						}
						lastdir = curdir;
						lasthitp = curhitp;
					}
				}
				return results;
			}
		}
	}
}
