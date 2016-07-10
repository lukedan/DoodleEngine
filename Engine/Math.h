#pragma once

#include <cstddef>

#include "Vector2.h"
#include "List.h"

namespace DE {
	namespace Core {
		template <typename T> class DefaultComparer {
			public:
				static int Compare(const T &lhs, const T &rhs) {
					return (lhs == rhs ? 0 : (lhs > rhs ? 1 : -1));
				}
		};

		namespace Math {
			inline int Sign(double val) {
				if (val < -Epsilon) {
					return -1;
				}
				if (val > Epsilon) {
					return 1;
				}
				return 0;
			}
			inline bool EqualToZero(double val) {
				return Sign(val) == 0;
			}

			unsigned HighestBit(unsigned long);
			template <typename T> inline T &Max(T &a, T &b) {
				return (a > b) ? a : b;
			}
			template <typename T> constexpr inline T Max(const T &a, const T &b) {
				return (a > b) ? a : b;
			}
			template <typename T> inline T &Min(T &a, T &b) {
				return (a > b) ? b : a;
			}
			template <typename T> constexpr inline T Min(const T &a, const T &b) {
				return (a > b) ? b : a;
			}
			template <typename T> constexpr inline T Abs(const T &v) {
				return (v < 0 ? -v : v);
			}
			template <typename T> constexpr inline T Square(const T &v) {
				return v * v;
			}
			template <typename T> inline void Swap(T &a, T &b) {
				T t = a;
				a = b;
				b = t;
			}
			template <typename T> constexpr inline T Clamp(const T &v, const T &min, const T &max) {
				return Max(min, Min(max, v));
			}

			template <typename T> class IsSignedIntegerType {
				public:
					static const bool Result = false;
			};
			template <> class IsSignedIntegerType<signed char> {
				public:
					static const bool Result = true;
			};
			template <> class IsSignedIntegerType<signed short> {
				public:
					static const bool Result = true;
			};
			template <> class IsSignedIntegerType<signed long> {
				public:
					static const bool Result = true;
			};
			template <> class IsSignedIntegerType<signed long long> {
				public:
					static const bool Result = true;
			};

			template <
				typename C, typename T, typename Comparer = DefaultComparer<T>
			> inline void UnstableSortRange(C &a, int s, int e) {
				if (s >= e) {
					return;
				}
				int i = s - 1;
				for (int j = s; j < e; j++) {
					if (Comparer::Compare(a[j], a[e]) < 0) {
						Swap(a[++i], a[j]);
					}
				}
				Swap(a[i + 1], a[e]);
				UnstableSortRange<C, T, Comparer>(a, s, i);
				UnstableSortRange<C, T, Comparer>(a, i + 2, e);
			}
			template <
				typename C, typename T, typename Comparer = DefaultComparer<T>
			> inline void UnstableSort(C &a, size_t count) {
				UnstableSortRange<C, T, Comparer>(a, 0, count - 1);
			}
			template <
				typename C, typename T, typename Comparer = DefaultComparer<T>
			> inline void UnstableSort(C &a) {
				UnstableSortRange<C, T, Comparer>(a, 0, a.Count() - 1);
			}
			template <typename T, typename Comparer = DefaultComparer<T>> inline void UnstableSort(T *arr, size_t count) {
				UnstableSortRange<T*, T, Comparer>(arr, 0, count - 1);
			}

			#define DE_MATH_DEFVARS				\
				if (count == 0) {				\
					return 0;					\
				}								\
				size_t s = 0, e = count - 1
			#define DE_MATH_LOCATE(N, A, B)		\
				while (e > s + 1) {				\
					size_t m = (s + e)>>1;	\
					if (arr[m] N target) {		\
						A = m;					\
					} else {					\
						B = m;					\
					}							\
				}
			#define DE_MATH_CHECKRETURN(N, F, L)	\
				if (arr[F] N target) {			\
					return F;					\
				} else if (arr[L] N target) {	\
					return L;					\
				}								\
				return -1
			#define DE_MATH_LOCATEBIGGER DE_MATH_LOCATE(>, e, s)
			#define DE_MATH_LOCATESMALLER DE_MATH_LOCATE(<, s, e)
			template <typename T> size_t BinaryFindFirstEqual(const T &target, const T *arr, size_t count) {
				DE_MATH_DEFVARS;
				DE_MATH_LOCATESMALLER;
				DE_MATH_CHECKRETURN(==, s, e);
			}
			template <typename T> size_t BinaryFindLastEqual(const T &target, const T *arr, size_t count) {
				DE_MATH_DEFVARS;
				DE_MATH_LOCATEBIGGER;
				DE_MATH_CHECKRETURN(==, e, s);
			}
			template <typename T> size_t BinaryFindGreaterThan(const T &target, const T *arr, size_t count) {
				DE_MATH_DEFVARS;
				DE_MATH_LOCATEBIGGER;
				DE_MATH_CHECKRETURN(>, s, e);
			}
			template <typename T> size_t BinaryFindSmallerThan(const T &target, const T *arr, size_t count) {
				DE_MATH_DEFVARS;
				DE_MATH_LOCATESMALLER;
				DE_MATH_CHECKRETURN(<, e, s);
			}
			template <typename T> size_t BinaryFindLowerBound(const T &target, const T *arr, size_t count) {
				DE_MATH_DEFVARS;
				DE_MATH_LOCATEBIGGER;
				DE_MATH_CHECKRETURN(>=, s, e);
			}
			template <typename T> size_t BinaryFindUpperBound(const T &target, const T *arr, size_t count) {
				DE_MATH_DEFVARS;
				DE_MATH_LOCATESMALLER;
				DE_MATH_CHECKRETURN(<=, e, s);
			}
			#undef DE_MATH_DEFVARS
			#undef DE_MATH_LOCATE
			#undef DE_MATH_CHECKRETURN
			#undef DE_MATH_LOCATEBIGGER
			#undef DE_MATH_LOCATESMALLER

			struct Vector2;
			enum class IntersectionType { // TODO add numerical pivots
				None,
				Full,
				Edge
			};
			IntersectionType SegmentsIntersect(
				const Vector2&, const Vector2&,
				const Vector2&, const Vector2&
			);
			IntersectionType SegmentsIntersect(
				const Vector2&, const Vector2&,
				const Vector2&, const Vector2&, Vector2&
			);

			IntersectionType LineSegmentIntersect(
				const Vector2&, const Vector2&,
				const Vector2&, const Vector2&
			);
			IntersectionType LineSegmentIntersect(
				const Vector2&, const Vector2&,
				const Vector2&, const Vector2&, Vector2&
			);

			IntersectionType RaySegmentIntersect(
				const Vector2&, const Vector2&,
				const Vector2&, const Vector2&
			);
			IntersectionType RaySegmentIntersect(
				const Vector2&, const Vector2&,
				const Vector2&, const Vector2&, Vector2&
			);

			IntersectionType RayLineIntersect(
				const Vector2&, const Vector2&,
				const Vector2&, const Vector2&
			);
			IntersectionType RayLineIntersect(
				const Vector2&, const Vector2&,
				const Vector2&, const Vector2&, Vector2&
			);

			IntersectionType CircleLineIntersect(
				const Vector2&, double,
				const Vector2&, const Vector2&
			);
			IntersectionType CircleLineIntersect(
				const Vector2&, double,
				const Vector2&, const Vector2&,
				Vector2&, Vector2&
			);

			IntersectionType CircleSegmentIntersect(
				const Vector2&, double,
				const Vector2&, const Vector2&
			);
			IntersectionType CircleSegmentIntersect(
				const Vector2&, double,
				const Vector2&, const Vector2&,
				size_t&, Vector2&, Vector2&
			);

			IntersectionType PolygonPointIntersect(const Core::Collections::List<Vector2>&, const Vector2&);


			void Projection(const Vector2&, const Vector2&, Vector2&, Vector2&);
			Vector2 ProjectionX(const Vector2&, const Vector2&);
			Vector2 ProjectionY(const Vector2&, const Vector2&);


			double AngleSin(const Vector2&, const Vector2&);
			double AngleSinSquared(const Vector2&, const Vector2&);
			double AngleCos(const Vector2&, const Vector2&);
			double AngleCosSquared(const Vector2&, const Vector2&);
			double AngleTan(const Vector2&, const Vector2&);
			double HalfAngleTan(const Vector2&, const Vector2&);
			double DoubleAngleTan(const Vector2&, const Vector2&);


			Vector2 Mirror(const Vector2&, const Vector2&, const Vector2&);


			double PointLineDistance(const Vector2&, const Vector2&, const Vector2&);
			double PointLineDistanceSquared(const Vector2&, const Vector2&, const Vector2&);
			double PointSegmentDistance(const Vector2&, const Vector2&, const Vector2&);
			double PointSegmentDistanceSquared(const Vector2&, const Vector2&, const Vector2&);
		}
	}
}
