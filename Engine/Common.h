#pragma once

#include <cstdio>
#include <cstdarg>
#include <cstddef>
#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#ifdef DEBUG
#	include <iostream>
#	include <fstream>
#endif

#include "Exceptions.h"

namespace DE {
	namespace Core {
		inline bool IsKeyDown(int key) {
			return GetAsyncKeyState(key) & 0x8000;
		}

		template <typename T> class EqualityPredicate {
			public:
				EqualityPredicate() = delete;
				static bool Examine(const T &tar, const T &rhs) {
					return tar == rhs;
				}
		};

#ifdef DEBUG
#	define ShowMessage _tprintf
#else
#	define ShowMessage(...)
#endif

#ifdef DEBUG
		int GetDumpID();
#endif

		template <typename T> class IsClass {
			protected:
				template <typename U> static char Validate(int U::*);
				template <typename U> static long long Validate(...);
			public:
				const static bool Result = sizeof(Validate<T>(nullptr)) == sizeof(char);
		};

		template <typename U, typename V> class IsSameClass {
			public:
				const static bool Result = false;
		};
		template <typename T> class IsSameClass<T, T> {
			public:
				const static bool Result = true;
		};

		template <typename CandidateDerived, typename CandidateBase> class IsBaseOf {
			private:
				struct Helper {
					operator CandidateBase*() const;
					operator CandidateDerived*();
				};
				template <typename T> static char Validate(CandidateDerived*, T);
				static long long Validate(CandidateBase*, int);
			public:
				const static bool Result = sizeof(Validate(Helper(), 0)) == sizeof(char);
		};
		template <typename Type> class IsBaseOf<Type, Type> {
			public:
				const static bool Result = true;
		};

#ifdef DEBUG
		template <size_t Size> struct MemoryMarker {
			public:
				MemoryMarker() = default;
				explicit constexpr MemoryMarker(const char *str) : Mark(*str), SubMarker(*str ? str + 1 : str) {
				}

				char Mark = '#';
				MemoryMarker<Size - 1> SubMarker;
		};
		template <> struct MemoryMarker<0> {
			public:
				MemoryMarker() = default;
				explicit constexpr MemoryMarker(const char*) {
				}
		};
#endif

#define StaticAssert static_assert
	}
}
