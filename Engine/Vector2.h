#pragma once

#include <cmath>

#include "Common.h"

namespace DE {
	namespace Core {
		namespace Math {
			const double
				Pi = 3.1415926535,
				SqrtTwo = 1.4142135624,
				OneOverSqrtTwo = 0.7071067812,
				Epsilon = 1e-6;
		    struct Vector2 {
				double X = 0.0, Y = 0.0;

				constexpr Vector2() = default;
				constexpr Vector2(double x, double y) : X(x), Y(y) {
				}

				double Length() const {
					return sqrt(X * X + Y * Y);
				}
				double LengthSquared() const {
					return X * X + Y * Y;
				}
				void SetLength(double newLen) {
					double per = sqrt(X * X + Y * Y);
					if (per < Epsilon) {
						X = newLen;
						Y = 0.0;
					} else {
						per = newLen / per;
						X *= per;
						Y *= per;
					}
				}

				double Angle() const {
                    return atan2(Y, X);
				}
				void SetAngle(double angle) {
				    double l = Length();
				    X = sin(angle) * l;
				    Y = cos(angle) * l;
				}
				void RotateClockwise(double angle) {
				    double ca = cos(angle), sa = sin(angle), nx = X * ca - Y * sa;
				    Y = X * sa + Y * ca;
				    X = nx;
				}

				void RotateRight90() {
					double t = -X;
					X = Y;
					Y = t;
				}
				void RotateLeft90() {
					double t = X;
					X = -Y;
					Y = t;
				}

				friend Vector2 operator +(const Vector2 &lhs, const Vector2 &rhs) {
					return Vector2(lhs.X + rhs.X, lhs.Y + rhs.Y);
				}
				Vector2 &operator +=(const Vector2 &rhs) {
					X += rhs.X;
					Y += rhs.Y;
					return *this;
				}

				Vector2 operator -() const {
					return Vector2(-X, -Y);
				}
				friend Vector2 operator -(const Vector2 &lhs, const Vector2 &rhs) {
					return Vector2(lhs.X - rhs.X, lhs.Y - rhs.Y);
				}
				Vector2 &operator -=(const Vector2 &rhs) {
					X -= rhs.X;
					Y -= rhs.Y;
					return *this;
				}

				friend Vector2 operator *(const Vector2 &lhs, double rhs) {
					return Vector2(rhs * lhs.X, rhs * lhs.Y);
				}
				friend Vector2 operator *(double lhs, const Vector2 &rhs) {
					return Vector2(lhs * rhs.X, lhs * rhs.Y);
				}
				Vector2 &operator *=(double rhs) {
					X *= rhs;
					Y *= rhs;
					return *this;
				}

				friend Vector2 operator /(const Vector2 &lhs, double rhs) {
					return Vector2(lhs.X / rhs, lhs.Y / rhs);
				}
				Vector2 &operator /=(double rhs) {
					X /= rhs;
					Y /= rhs;
					return *this;
				}

				static double Dot(const Vector2 &lhs, const Vector2 &rhs) {
					return lhs.X * rhs.X + lhs.Y * rhs.Y;
				}
				static double Cross(const Vector2 &lhs, const Vector2 &rhs) {
					return lhs.X * rhs.Y - lhs.Y * rhs.X;
				}
			};
		}
	}
}
