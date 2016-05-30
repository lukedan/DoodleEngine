#pragma once

#include "Vector2.h"
#include "Math.h"

namespace DE {
	namespace Core {
		namespace Math {
			struct Rectangle {
				constexpr Rectangle() : Left(0.0), Top(0.0), Right(0.0), Bottom(0.0) {
				}
				constexpr Rectangle(const Vector2 &size) : Left(0.0), Top(0.0), Right(size.X), Bottom(size.Y) {
				}
				constexpr Rectangle(double x, double y, double w, double h) : Left(x), Top(y), Right(x + w), Bottom(y + h) {
				}
				constexpr Rectangle(const Vector2 &lefttop, const Vector2 &rightbottom) : Left(lefttop.X), Top(lefttop.Y),
					Right(rightbottom.X), Bottom(rightbottom.Y) {
				}

				double Left, Top, Right, Bottom;

				double Width() const {
					return Right - Left;
				}
				double Height() const {
					return Bottom - Top;
				}

				Vector2 Size() const {
					return Vector2(Width(), Height());
				}
				double Area() const {
					return Width() * Height();
				}

                Vector2 TopLeft() const {
                	return Vector2(Left, Top);
                }
                Vector2 TopRight() const {
                	return Vector2(Right, Top);
                }
                Vector2 BottomLeft() const {
                	return Vector2(Left, Bottom);
                }
                Vector2 BottomRight() const {
                	return Vector2(Right, Bottom);
                }
                Vector2 Center() const {
                	return Vector2((Left + Right) * 0.5, (Top + Bottom) * 0.5);
                }
                double CenterX() const {
                	return (Left + Right) * 0.5;
                }
                double CenterY() const {
                	return (Top + Bottom) * 0.5;
                }

                void Translate(const Vector2 &offset) {
                	Left += offset.X;
                	Right += offset.X;
                	Top += offset.Y;
                	Bottom += offset.Y;
                }
                void Scale(const Vector2 &center, double scale) {
                	Top = center.Y + (Top - center.Y) * scale;
                	Bottom = center.Y + (Bottom - center.Y) * scale;
                	Left = center.X + (Left - center.X) * scale;
                	Right = center.X + (Right - center.X) * scale;
                }
                void Scale(const Rectangle &identity, const Rectangle &target) {
                	double nl, nt, wb = Width() / identity.Width(), hb = Height() / identity.Height();
                	nl = Right + wb * (target.Left - identity.Right);
                	Right = Left + wb * (target.Right - identity.Left);
                	nt = Bottom + hb * (target.Top - identity.Bottom);
                	Bottom = Top + hb * (target.Bottom - identity.Top);
                	Left = nl;
                	Top = nt;
                }

                bool Contains(const Rectangle &r) const {
                	return r.Left >= Left && r.Right <= Right && r.Top >= Top && r.Bottom <= Bottom;
                }

				static Rectangle Union(const Rectangle &a, const Rectangle &b) {
					Rectangle r;
					r.Left = Min(a.Left, b.Left);
					r.Top = Min(a.Top, b.Top);
					r.Right = Max(a.Right, b.Right);
					r.Bottom = Max(a.Bottom, b.Bottom);
					return r;
				}
				static IntersectionType Intersect(const Rectangle &a, const Rectangle &b, Rectangle &result) {
					result.Left = Max(a.Left, b.Left);
					result.Top = Max(a.Top, b.Top);
					result.Right = Min(a.Right, b.Right);
					result.Bottom = Min(a.Bottom, b.Bottom);
					if (result.Right < result.Left || result.Bottom < result.Top) {
						result.Left = result.Top = result.Right = result.Bottom = 0.0;
						return IntersectionType::None;
					}
					if (result.Right > result.Left && result.Bottom > result.Top) {
						return IntersectionType::Full;
					}
					return IntersectionType::Edge;
				}
				static IntersectionType Intersect(const Rectangle &a, const Rectangle &b) {
					if (a.Bottom < b.Top || a.Top > b.Bottom || a.Right < b.Left || a.Left > b.Right) {
						return IntersectionType::None;
					}
					return IntersectionType::Full; // FIXME! Obviously Wrong!
				}
				static IntersectionType Intersect(const Rectangle &rect, const Vector2 &point) {
					if (rect.Bottom < point.Y || rect.Top > point.Y || rect.Right < point.X || rect.Left > point.X) {
						return IntersectionType::None;
					}
					if (rect.Bottom > point.Y && rect.Top < point.Y && rect.Right > point.X && rect.Left < point.X) {
						return IntersectionType::Full;
					}
					return IntersectionType::Edge;
				}
			};
		}
	}
}
