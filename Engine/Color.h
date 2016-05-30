#pragma once

namespace DE {
	namespace Core {
		struct Color {
			constexpr Color() = default;
			constexpr Color(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
				: R(r), G(g), B(b), A(a) {
			}
			static Color FromFloats(float r, float g, float b, float a) {
			    return Color(r * 255.0f, g * 255.0f, b * 255.0f, a * 255.0f);
			}
			static Color FromDoubles(double r, double g, double b, double a) {
			    return Color(r * 255.0, g * 255.0, b * 255.0, a * 255.0);
			}

			static Color Blend(const Color &c1, double a1, const Color &c2, double a2) {
				double
					sa1 = a1 * (c1.A / 255.0), sr1 = c1.R / 255.0, sg1 = c1.G / 255.0, sb1 = c1.B / 255.0,
					sa2 = a2 * (c2.A / 255.0), sr2 = c2.R / 255.0, sg2 = c2.G / 255.0, sb2 = c2.B / 255.0;
				return FromDoubles(a1 * sr1 + a2 * sr2, a1 * sg1 + a2 * sg2, a1 * sb1 + a2 * sb2, sa1 + sa2);
			}

			unsigned char R = 255, G = 255, B = 255, A = 255;

			double DoubleR() const {
				return R / 255.0;
			}
			double DoubleG() const {
				return G / 255.0;
			}
			double DoubleB() const {
				return B / 255.0;
			}
			double DoubleA() const {
				return A / 255.0;
			}

			double FloatR() const {
				return R / 255.0f;
			}
			double FloatG() const {
				return G / 255.0f;
			}
			double FloatB() const {
				return B / 255.0f;
			}
			double FloatA() const {
				return A / 255.0f;
			}
		};
	}
}
