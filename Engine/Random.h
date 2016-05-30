#pragma once

#include "Common.h"
#include "Stopwatch.h"

namespace DE {
	namespace Core {
		struct Random {
			public:
				Random() : _curV(Stopwatch::GetTime()) {
				}
				Random(unsigned int seed) : _curV((long)seed) {
				}

				void SetSeed(unsigned int seed) {
					_curV = (long)seed;
				}

                int Next() {
                	return (((_curV = _curV * 214013L + 2531011L)>>16) & MaxValue);
                }
                int NextBetween(int a, int b) { // [a, b)
                	if (b < a) {
						throw InvalidArgumentException(_TEXT("the region is invalid"));
                	}
                	if (a == b) {
						return a;
                	}
                	double i = Next() / (double)(MaxValue + 1);
                	return a + (b - a) * i;
                }
                double NextDouble() { // [0.0, 1.0)
                	return Next() / (double)(MaxValue + 1);
                }
                float NextFloat() { // [0.0f, 1.0f)
                	return Next() / (float)(MaxValue + 1);
                }
                Math::Vector2 NextDirection() {
                    double ang = NextDouble() * Math::Pi * 2;
                    return Math::Vector2(sin(ang), cos(ang));
                }
                Math::Vector2 NextInCircle(double radius = 1.0) {
                	Math::Vector2 tres = NextDirection();
                	double x = NextDouble();
                	if (x < NextDouble()) {
                		x = 1.0 - x;
                	}
                	return tres * x * radius;
                }

				const static long MaxValue = 0x7FFF;
			private:
				unsigned long _curV = 1L;
		};
	}
}
