#pragma once

#include "Common.h"
#include "Vector2.h"
#include "Color.h"
#include "BrushAndPen.h"

namespace DE {
	namespace Graphics {
		struct GradientStop {
			double Progress;
			Core::Color Color;
		};
		class LinearGradientBrush : public Brush {
			public:
				Core::Math::Vector2 &Start() {
					return _s;
				}
				const Core::Math::Vector2 &Start() const {
					return _s;
				}

				Core::Math::Vector2 &End() {
					return _e;
				}
				const Core::Math::Vector2 &End() const {
					return _e;
				}

				Core::Collections::List<GradientStop, true> &GradientStops() {
					return _stops;
				}
				const Core::Collections::List<GradientStop, true> &GradientStops() const {
					return _stops;
				}

				void FillRect(const Core::Math::Rectangle &rect, Renderer &r) const {
					if (_stops.Count() == 0) {
						return;
					}
					Core::Math::Vector2 sedir = _e - _s;
					double tlp, trp, blp, brp, lsq = sedir.LengthSquared();
					tlp = Core::Math::Vector2::Dot(rect.TopLeft() - _s, sedir) / lsq;
					trp = Core::Math::Vector2::Dot(rect.TopRight() - _s, sedir) / lsq;
					blp = Core::Math::Vector2::Dot(rect.BottomLeft() - _s, sedir) / lsq;
					brp = Core::Math::Vector2::Dot(rect.BottomRight() - _s, sedir) / lsq;
				}
			protected:
				Core::Math::Vector2 _s, _e;
				Core::Collections::List<GradientStop, true> _stops;

				Core::Color At(double prog) const {
					if (_stops.Count() == 0) {
						return Core::Color();
					}
					if (prog <= 0.0) {
						return _stops.First();
					}
					if (prog >= 1.0) {
						return _stops.Last();
					}
					for (size_t i = 1; i < _stops.Count(); ++i) {
						if (_stops[i].Progress >= prog && _stops[i - 1].Progress <= prog) {
							double gp = _stops[i].Progress - _stops[i - 1].Progress;
							return Core::Color::Blend(
								_stops[i - 1].Color,
								(prog - _stops[i - 1].Progress) / gp,
								_stops[i].Color,
								(_stops[i].Progress - prog) / gp
							);
						}
					}
					return Core::Color();
				}

				inline static Core::Math::Vector2 LineXAALIntersect(double y, const Core::Math::Vector2 &pol, const Core::Math::Vector2 &pdr) {
					return pol + pdr * (y - pol.Y) / pdr.Y;
				}
				inline static Core::Math::Vector2 LineYAALIntersect(double x, const Core::Math::Vector2 &pol, const Core::Math::Vector2 &pdr) {
					return pol + pdr * (x - pol.X) / pdr.X;
				}
				inline static bool RayXAALIntersect(double y, const Core::Math::Vector2 &vo, const Core::Math::Vector2 &vd, Core::Math::Vector2 &res) {
					if ((vo.Y - y) * vd.Y > 0.0) {
						return false;
					}
					res = vo + vd * (y - vo.Y) / vd.Y;
					return true;
				}
				inline static bool RayYAALIntersect(double x, const Core::Math::Vector2 &vo, const Core::Math::Vector2 &vd, Core::Math::Vector2 &res) {
					if ((vo.X - x) * vd.X > 0.0) {
						return false;
					}
					res = vo + vd * (x - vo.X) / vd.X;
					return true;
				}

				struct Triangle {
					Triangle() = default;
					Triangle(const Vertex &v1, const Vertex &v2, const Vertex &v3) : V1(v1), V2(v2), V3(v3) {
					}

					Vertex V1, V2, V3;
				};
				enum class NodeType {
					Left,
					Middle,
					Right
				};
				inline static NodeType GetNodeType(const Core::Math::Vector2 &node, const Core::Math::Rectangle &rect) {
					if (node.X < rect.Left) {
						return NodeType::Left;
					}
					return (node.X < rect.Right ? NodeType::Middle : NodeType::Right);
				}
				inline static void CutTriangle(
					const Vertex &xaal,
					const Vertex &xaar,
					const Vertex &uq,
					const Core::Math::Rectangle &rect,
					Core::Collections::List<Triangle> &res
				) {
					double xmax = Core::Math::Max(xaar.Position.X, uq.Position.X), xmin = Core::Math::Min(xaal.Position.X, uq.Position.X);
					if (xmax < rect.Left || xmin > rect.Right) {
						return;
					}
				}

				void DrawColorStrip(
					const Core::Math::Vector2 &s,
					const Core::Math::Vector2 &e,
					const Core::Color &sc,
					const Core::Color &ec,
					const Core::Math::Rectangle &rect,
					Core::Collections::List<Vertex> vxs
				) const {
					Core::Math::Vector2 ldr = e - s, prpldr = ldr, ts = s, te = e;
					Core::Math::Rectangle trect = rect;
					prpldr.RotateLeft90();
					bool xyInv = ldr.Y < ldr.X;
					if (xyInv) {
						Core::Math::Swap(ldr.X, ldr.Y);
						Core::Math::Swap(prpldr.X, prpldr.Y);
						Core::Math::Swap(ts.X, ts.Y);
						Core::Math::Swap(te.X, te.Y);
						Core::Math::Swap(trect.Top, trect.Left);
						Core::Math::Swap(trect.Right, trect.Bottom);
					}
					Core::Math::Vector2
						ts = LineXAALIntersect(trect.Top, ts, prpldr),
						te = LineXAALIntersect(trect.Top, te, prpldr),
						bs = LineXAALIntersect(trect.Bottom, ts, prpldr),
						be = LineXAALIntersect(trect.Bottom, te, prpldr);
				}
		};
	}
}