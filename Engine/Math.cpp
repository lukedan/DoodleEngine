#include "Math.h"

namespace DE {
	namespace Core {
		namespace Math {
			unsigned HighestBit(unsigned long val) {
				unsigned res = 0;
				unsigned long t;
				#define HB_RMOVE(X) 		\
					t = val>>(X);		\
					if (t) {			\
						res += (X);		\
						val = t;		\
					}
				HB_RMOVE(16);
				HB_RMOVE(8);
				HB_RMOVE(4);
				HB_RMOVE(2);
				HB_RMOVE(1);
				if (val) {
					return ++res;
				}
				return res;
				#undef HB_RMOVE
			}

			IntersectionType SegmentsIntersect(
				const Vector2 &p11, const Vector2 &p12,
				const Vector2 &p21, const Vector2 &p22
			) {
				Vector2
					d1 = p12 - p11,
					d2 = p22 - p21,
					v121 = p21 - p11,
					v122 = p22 - p11,
					v212 = p12 - p21;
				double
					r1t2 = Vector2::Cross(v121, d1) * Vector2::Cross(v122, d1),
					r2t1 = Vector2::Cross(-v121, d2) * Vector2::Cross(v212, d2);
				if (r1t2 > 0.0 || r2t1 > 0.0) {
					return IntersectionType::None;
				}
				if (r1t2 * r2t1 > 0.0) {
					return IntersectionType::Full;
				}
				return IntersectionType::Edge;
			}
			IntersectionType SegmentsIntersect(
				const Vector2 &p11, const Vector2 &p12,
				const Vector2 &p21, const Vector2 &p22, Vector2 &joint
			) {
				Vector2
					d1 = p12 - p11,
					d2 = p22 - p21,
					v121 = p21 - p11,
					v122 = p22 - p11,
					v212 = p12 - p21,
					tp21 = p21;
				double
					r1t21 = Vector2::Cross(d1, v121),
					r1t22 = Vector2::Cross(d1, v122),
					r1t2 = r1t21 * r1t22,
					r2t1 = Vector2::Cross(-v121, d2) * Vector2::Cross(v212, d2);
				if (r1t2 > 0.0 || r2t1 > 0.0) {
					return IntersectionType::None;
				}
				joint = tp21 + d2 * (r1t21 / (r1t21 - r1t22));
				if (r1t2 * r2t1 > 0.0) {
					return IntersectionType::Full;
				}
				return IntersectionType::Edge;
			}
			IntersectionType LineSegmentIntersect(
				const Vector2 &pol, const Vector2 &pdr,
				const Vector2 &p21, const Vector2 &p22
			) {
				double evidence = Vector2::Cross(p21 - pol, pdr) * Vector2::Cross(p22 - pol, pdr);
				if (evidence < 0.0) {
					return IntersectionType::Full;
				}
				if (evidence > 0.0) {
					return IntersectionType::None;
				}
				return IntersectionType::Edge;
			}
			IntersectionType LineSegmentIntersect(
				const Vector2 &pol, const Vector2 &pdr,
				const Vector2 &p21, const Vector2 &p22, Vector2 &joint
			) {
				double cr1 = Vector2::Cross(p21 - pol, pdr), cr2 = Vector2::Cross(p22 - pol, pdr);
				if (cr1 * cr2 > 0.0) {
					return IntersectionType::None;
				}
				joint = p21 + (p22 - p21) * (cr1 / (cr1 - cr2));
				if (cr1 * cr2 < 0.0) {
					return IntersectionType::Full;
				}
				return IntersectionType::Edge;
			}
			IntersectionType RaySegmentIntersect(
				const Vector2 &vo, const Vector2 &vd,
				const Vector2 &p1, const Vector2 &p2
			) {
				Vector2 st1 = p1 - vo, st2 = p2 - vo;
				double dst1 = Vector2::Cross(vd, st1), st2d = Vector2::Cross(st2, vd), dmul = dst1 * st2d;
				if (dmul < 0.0) {
					return IntersectionType::None;
				}
				double st1st2 = Vector2::Cross(st1, st2), dm1 = st1st2 * dst1, dm2 = st1st2 * st2d;
				if (dm1 > 0.0 || dm2 > 0.0) {
					return IntersectionType::None;
				}
				if (dm1 < 0.0 && dm2 < 0.0 && dmul > 0.0) {
					return IntersectionType::Full;
				}
				return IntersectionType::Edge;
			}
			IntersectionType RaySegmentIntersect(
				const Vector2 &vo, const Vector2 &vd,
				const Vector2 &p1, const Vector2 &p2, Vector2 &joint
			) {
				Vector2 st1 = p1 - vo, st2 = p2 - vo, sdr = p2 - p1;
				double dst1 = Vector2::Cross(st1, vd), dst2 = Vector2::Cross(st2, vd), dmul = dst1 * dst2;
				if (dmul > 0.0) {
					return IntersectionType::None;
				}
				Vector2 tj = p2 + sdr * (dst2 / (dst1 - dst2));
				double dj = Vector2::Dot(vd, tj - vo);
				if (dj < 0.0) {
					return IntersectionType::None;
				}
				joint = tj;
				if (dj > 0.0 && dmul < 0.0) {
					return IntersectionType::Full;
				}
				return IntersectionType::Edge;
			}

			IntersectionType RayLineIntersect(
				const Vector2 &vo, const Vector2 &vd,
				const Vector2 &pol, const Vector2 &pd
			) {
				Vector2 otPol = vo - pol;
				double dres = Vector2::Cross(vd, pd) * Vector2::Cross(otPol, pd);
				return (dres < 0.0 ? IntersectionType::Full : (dres > 0.0 ? IntersectionType::None : IntersectionType::Edge));
			}
			IntersectionType RayLineIntersect(
				const Vector2 &vo, const Vector2 &vd,
				const Vector2 &pol, const Vector2 &pd, Vector2 &joint
			) {
				Vector2 st1 = vo - pol, st2 = st1 - vd;
				double
					dst1 = Vector2::Cross(st1, pd),
					dst2 = Vector2::Cross(st2, pd),
					ddec = dst1 * Vector2::Cross(vd, pd);
				if (ddec > 0.0) {
					return IntersectionType::None;
				}
				joint = vo - vd * (dst1 / (dst1 - dst2));
				if (ddec < 0.0) {
					return IntersectionType::Full;
				}
				return IntersectionType::Edge;
			}

			IntersectionType CircleLineIntersect(
				const Vector2 &origin, double r,
				const Vector2 &pOL, const Vector2 &pDr
			) {
				double dist = PointLineDistanceSquared(origin, pOL, pDr);
				r *= r;
				return (dist > r ? IntersectionType::None : (dist < r ? IntersectionType::Full : IntersectionType::Edge));
			}
			IntersectionType CircleLineIntersect(
				const Vector2 &origin, double r,
				const Vector2 &pOL, const Vector2 &pDr,
				Vector2 &res1, Vector2 &res2
			) { // TODO: when pDr == 0 & improve performance
				double distSqr = PointLineDistanceSquared(origin, pOL, pDr);
				r *= r;
				if (distSqr > r) {
					return IntersectionType::None;
				}
				Vector2 pers = pDr, otol = pOL - origin;
				pers.RotateLeft90();
				Vector2 tos = ProjectionY(otol, pers) + origin;
				double tgl = sqrt(r - (tos - origin).LengthSquared());
				pers = pDr;
				pers.SetLength(tgl);
				res1 = tos + pers;
				res2 = tos - pers;
				return (distSqr < r ? IntersectionType::Full : IntersectionType::Edge);
			}

			IntersectionType CircleSegmentIntersect(
				const Vector2 &origin, double r,
				const Vector2 &p1, const Vector2 &p2
			) {
				double distSqr = PointSegmentDistanceSquared(origin, p1, p2);
				r *= r;
				return (distSqr > r ? IntersectionType::None : (distSqr < r ? IntersectionType::Full : IntersectionType::Edge));
			}
			IntersectionType CircleSegmentIntersect(
				const Vector2 &origin, double r,
				const Vector2 &p1, const Vector2 &p2,
				size_t &jtNum, Vector2 &j1, Vector2 &j2
			) {
				Vector2 pt1 = p1 - origin, pt2 = p2 - origin, pdir = p2 - p1;
				pdir.RotateRight90();
				double dv1 = Vector2::Cross(pdir, pt1), dv2 = Vector2::Cross(pdir, pt2), dissq, orr = r;
				r *= r;
				double ls1 = pt1.LengthSquared(), ls2 = pt2.LengthSquared();
				if (dv1 * dv2 >= 0.0) {
					double lm = Max(ls1, ls2);
					dissq = Min(ls1, ls2);
					jtNum = (dissq <= r && lm > r ? 1 : 0);
				} else {
					dissq = Vector2::Dot(pt2, pdir);
					dissq = (dissq * dissq) / pdir.LengthSquared();
					jtNum = (dissq > r ? 0 : (ls1 >= r ? 1 : 0) + (ls2 >= r ? 1 : 0));
				}
				if (jtNum > 0) {
					CircleLineIntersect(origin, orr, p1, p1 - p2, j1, j2); // TODO: improve performance(remove the call to CircleLineIntersect)
					if (Vector2::Dot(p1 - p2, j1 - p2) * Vector2::Dot(p2 - p1, j1 - p1) < 0.0) {
						j1 = j2;
					}
				}
				return (dissq > r ? IntersectionType::None : (dissq < r ? IntersectionType::Full : IntersectionType::Edge));
			}

			IntersectionType PolygonPointIntersection(const Collections::List<Vector2> &pol, const Vector2 &vs) { // FIXME: in error
				const Vector2 direction(1.0, 0.0);

				/*size_t count = 0;
				for (size_t i = 0; i < pol.Count(); ++i) {
					IntersectionType type = RaySegmentIntersect(vs, direction, pol[i], pol[(i + 1) % pol.Count()]);
					if (type == IntersectionType::Full) {
						++count;
					} else if (type == IntersectionType::Edge) {
						double
							x1 = Vector2::Cross(direction, pol[i] - vs),
							x2 = Vector2::Cross(direction, pol[(i + 2) % pol.Count()] - vs);
						if (x1 * x2 < -Epsilon) {
							++count;
						}
						++i;
					}
				}
				return (count % 2 == 1) ? IntersectionType::Full : IntersectionType::None;*/

				#define DE_MATH_POLYPTISECT_MOVENEXT	\
					last = cur;							\
					cur = next;							\
					++next;								\
					if (next == pol.Count()) {			\
						next = 0;						\
						con = 1;						\
					}
				size_t last = pol.Count() - 1, cur = 0, next = 1, isc = 0;
				Vector2 tv;
				int con = 2;
				do {
					if (con == 1) {
						--con;
					}
					while (EqualToZero(Vector2::Cross(pol[next] - pol[cur], direction))) {
						++next;
						if (next == pol.Count()) {
							next = 0;
							con = 1;
						}
					}
					IntersectionType type = RaySegmentIntersect(vs, direction, pol[last], pol[cur], tv);
					switch (type) {
						case IntersectionType::Full: {
							++isc;
							break;
						}
						case IntersectionType::Edge: {
							if ((tv - vs).LengthSquared() < Epsilon) {
								return IntersectionType::Edge;
							}
							Vector2 a = pol[last] - pol[cur], b = pol[next] - pol[cur];
							if (Vector2::Cross(direction, a) * Vector2::Cross(direction, b) < -Epsilon) {
								++isc;
							}
							DE_MATH_POLYPTISECT_MOVENEXT; // ignore the next segment
							break;
						}
						case IntersectionType::None: {
							break;
						}
					}
					DE_MATH_POLYPTISECT_MOVENEXT;
				} while (con);
				return (isc % 2 == 1) ? IntersectionType::Full : IntersectionType::None;
				#undef DE_MATH_POLYPTISECT_MOVENEXT
			}

			void Projection(const Vector2 &tar, const Vector2 &yaxis, Vector2 &rx, Vector2 &ry) {
				double yl = yaxis.LengthSquared();
				ry = yaxis * (Vector2::Dot(yaxis, tar) / yl);
				rx = tar - ry;
			}
			Vector2 ProjectionX(const Vector2 &tar, const Vector2 &yaxis) {
				return tar - yaxis * (Vector2::Dot(yaxis, tar) / yaxis.LengthSquared());
			}
			Vector2 ProjectionY(const Vector2 &tar, const Vector2 &yaxis) {
				return yaxis * (Vector2::Dot(yaxis, tar) / yaxis.LengthSquared());
			}

			double AngleSin(const Vector2 &a, const Vector2 &b) {
				double det = Vector2::Cross(a, b);
				return (
					det > 0 ?
					sqrt(det * det / (a.LengthSquared() * b.LengthSquared())) :
					-sqrt(det * det / (a.LengthSquared() * b.LengthSquared()))
				);
			}
			double AngleSinSquared(const Vector2 &a, const Vector2 &b) {
				double det = Vector2::Cross(a, b);
				det *= det;
				return det / (a.LengthSquared() * b.LengthSquared());
			}
			double AngleCos(const Vector2 &a, const Vector2 &b) {
				double dot = Vector2::Dot(a, b);
				return (
					dot > 0 ?
					sqrt(dot * dot / (a.LengthSquared() * b.LengthSquared())) :
					-sqrt(dot * dot / (a.LengthSquared() * b.LengthSquared()))
				);
			}
			double AngleCosSquared(const Vector2 &a, const Vector2 &b) {
				double dot = Vector2::Dot(a, b);
				dot *= dot;
				return dot / (a.LengthSquared() * b.LengthSquared());
			}
			double AngleTan(const Vector2 &a, const Vector2 &b) {
				return Vector2::Cross(a, b) / Vector2::Dot(a, b);
			}
			double HalfAngleTan(const Vector2 &a, const Vector2 &b) { // TODO: improve performance and see if it's right
				return (1 - AngleCos(a, b)) / AngleSin(a, b);
			}
			double DoubleAngleTan(const Vector2 &a, const Vector2 &b) {
				double angTan = AngleTan(a, b);
				return 2 * angTan / (1 - angTan * angTan);
			}

			Vector2 Mirror(const Vector2 &tarPt, const Vector2 &pOL, const Vector2 &pDr) {
				return tarPt - ProjectionX(tarPt - pOL, pDr) * 2.0;
			}

			double PointLineDistance(const Vector2 &pt, const Vector2 &ptOnLine, const Vector2 &lineDir) {
				Vector2 va = lineDir, vb = ptOnLine - pt;
				va.RotateRight90();
				return Abs(Vector2::Dot(va, vb) / va.Length());
			}
			double PointLineDistanceSquared(const Vector2 &pt, const Vector2 &ptOnLine, const Vector2 &lineDir) {
				Vector2 va = lineDir, vb = ptOnLine - pt;
				va.RotateRight90();
				double dotR = Vector2::Dot(va, vb);
				return (dotR * dotR) / va.LengthSquared();
			}
			double PointSegmentDistance(const Vector2 &pt, const Vector2 &p1, const Vector2 &p2) {
				Vector2 pt1 = p1 - pt, pt2 = p2 - pt, pdir = p2 - p1;
				pdir.RotateRight90();
				double dv1 = Vector2::Cross(pdir, pt1), dv2 = Vector2::Cross(pdir, pt2);
				if (dv1 * dv2 > 0.0) {
					return (pt1.LengthSquared() > pt2.LengthSquared() ? pt2.Length() : pt1.Length());
				} else {
					return Abs(Vector2::Dot(pt2, pdir) / pdir.Length());
				}
			}
			double PointSegmentDistanceSquared(const Vector2 &pt, const Vector2 &p1, const Vector2 &p2) {
				Vector2 pt1 = p1 - pt, pt2 = p2 - pt, pdir = p2 - p1;
				pdir.RotateRight90();
				double dv1 = Vector2::Cross(pdir, pt1), dv2 = Vector2::Cross(pdir, pt2);
				if (dv1 * dv2 > 0.0 || (p2 - p1).LengthSquared() <= Epsilon) {
					return Min(pt1.LengthSquared(), pt2.LengthSquared());
				} else {
					double dotR = Vector2::Dot(pt2, pdir);
					return dotR * dotR / pdir.LengthSquared();
				}
			}
		}
	}
}
