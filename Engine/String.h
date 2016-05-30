#pragma once

#include <sstream>
#include <locale>

#include "ObjectAllocator.h"
#include "Common.h"
#include "Math.h"
#include "List.h"

namespace DE {
	namespace Core {
		template <typename Char> class StringBase : protected Collections::List<Char> {
			public:
				StringBase() : Base() {
					Base::PushBack(0);
				}
				StringBase(const Char *src) : Base() {
					Base::PushBackRange(src, GetLength(src));
					Base::PushBack(0);
				}
				StringBase(Char c, size_t repeat = 1) : Base(c, repeat) {
					Base::PushBack(0);
				}
				virtual ~StringBase() {
				}

				StringBase<Char> &operator +=(const StringBase<Char> &rhs) {
					Base::PopBack();
					Base::PushBackRange(rhs);
					return *this;
				}
				StringBase<Char> &operator +=(const Char *rhs) {
					Base::PopBack();
					Base::PushBackRange(rhs, GetLength(rhs) + 1);
					return *this;
				}
				StringBase<Char> &operator +=(const Char &c) {
					Base::Last() = c;
					Base::PushBack(0);
					return *this;
				}
				explicit operator Char*() {
					return **this;
				}
				friend StringBase<Char> operator +(const StringBase<Char> &lhs, const StringBase<Char> &rhs) {
					StringBase<Char> tmp(lhs);
					return tmp += rhs;
				}
				friend StringBase<Char> operator +(const StringBase<Char> &lhs, const Char *rhs) {
					StringBase<Char> tmp(lhs);
					return tmp += rhs;
				}
				friend StringBase<Char> operator +(const Char *lhs, const StringBase<Char> &rhs) {
					StringBase<Char> tmp(lhs);
					return tmp += rhs;
				}
				friend StringBase<Char> operator +(const StringBase<Char> &lhs, const Char &rhs) {
					StringBase<Char> tmp(lhs);
					return tmp += rhs;
				}
				friend StringBase<Char> operator +(const Char &lhs, const StringBase<Char> &rhs) {
					StringBase<Char> tmp(lhs);
					return tmp += rhs;
				}

				using Collections::List<Char>::operator*;

				Char &At(size_t index) {
					if (index >= Base::Count() - 1) {
						throw OverflowException(_TEXT("index overflow"));
					}
					return Base::At(index);
				}
				const Char &At(size_t index) const {
					if (index >= Base::Count() - 1) {
						throw OverflowException(_TEXT("index overflow"));
					}
					return Base::At(index);
				}
				Char &operator [](size_t index) {
					return At(index);
				}
				const Char &operator [](size_t index) const {
					return At(index);
				}

				StringBase<Char> SubString(size_t start) const {
					return StringBase<Char>(Base::SubSequence(start));
				}
				StringBase<Char> SubString(size_t start, size_t count) const {
					if (start + count >= Base::Count()) {
						throw OverflowException(_TEXT("index overflow"));
					}
					StringBase<Char> res(Base::SubSequence(start, count));
					res.PushBack(0);
					return res;
				}

				bool EndsWith(const StringBase<Char> &suffix) const {
					if (suffix.Base::Count() > Base::Count()) {
						return false;
					}
					for (
						const Char *thisC = (**this) + Base::Count() - suffix.Base::Count(), *sufC = *suffix, *sufE = sufC + suffix.Length();
						sufC != sufE;
						++thisC, ++sufC
					) {
						if ((*thisC) != (*sufC)) {
							return false;
						}
					}
					return true;
				}
				bool BeginsWith(const StringBase<Char> &prefix) const {
					if (prefix.Base::Count() > Base::Count()) {
						return false;
					}
					for (
						const Char *thisC = **this, *preC = *prefix, *preE = preC + prefix.Length();
						preC != preE;
						++thisC, ++preC
					) {
						if ((*thisC) != (*preC)) {
							return false;
						}
					}
					return true;
				}
				StringBase<Char> ToUpper() const {
					StringBase<Char> result;
					const std::ctype<Char> &facet = std::use_facet<std::ctype<Char>>(std::locale());
					for (const Char *beg = **this, *end = beg + Length(); beg != end; ++beg) {
						result += facet.toupper(*beg);
					}
					return result;
				}
				StringBase<Char> ToLower() const {
					StringBase<Char> result;
					const std::ctype<Char> &facet = std::use_facet<std::ctype<Char>>(std::locale());
					for (const Char *beg = **this, *end = beg + Length(); beg != end; ++beg) {
						result += facet.tolower(*beg);
					}
					return result;
				}

				size_t Length() const {
					return Base::Count() - 1;
				}
				bool Empty() const {
					return Base::Count() < 2;
				}

				void Remove(size_t index, size_t count = 1) {
					if (index + count >= Base::Count()) {
						throw OverflowException(_TEXT("index overflow"));
					}
					Base::Remove(index, count);
				}
				void Insert(size_t index, Char c) {
					if (index >= Base::Count()) {
						throw OverflowException(_TEXT("index overflow"));
					}
					Base::Insert(index, c);
				}
				void Insert(size_t index, const StringBase<Char> &str) {
					if (index >= Base::Count()) {
						throw OverflowException(_TEXT("index overflow"));
					}
					Base::Insert(index, *str, str.Length());
				}
				void Insert(size_t index, const Char *chars) {
					if (index >= Base::Count()) {
						throw OverflowException(_TEXT("index overflow"));
					}
					Base::Insert(index, chars, GetLength(chars));
				}

				void Swap(size_t id1, size_t id2) {
					TCHAR &t1 = Base::At(id1), &t2 = Base::At(id2), t = t1;
					t1 = t2;
					t2 = t;
				}

				inline static size_t GetLength(const Char *str) {
					if (str == nullptr) {
						return 0;
					}
					size_t l = 0;
					for (; *str; ++str, ++l) {
					}
					return l;
				}

				friend bool operator ==(const StringBase &lhs, const StringBase &rhs) {
					return static_cast<const Base&>(lhs) == static_cast<const Base&>(rhs);
				}
				friend bool operator !=(const StringBase &lhs, const StringBase &rhs) {
					return static_cast<const Base&>(lhs) != static_cast<const Base&>(rhs);
				}
				friend bool operator <(const StringBase &lhs, const StringBase &rhs) {
					return static_cast<const Base&>(lhs) < static_cast<const Base&>(rhs);
				}
				friend bool operator >(const StringBase &lhs, const StringBase &rhs) {
					return static_cast<const Base&>(lhs) > static_cast<const Base&>(rhs);
				}
				friend bool operator <=(const StringBase &lhs, const StringBase &rhs) {
					return static_cast<const Base&>(lhs) <= static_cast<const Base&>(rhs);
				}
				friend bool operator >=(const StringBase &lhs, const StringBase &rhs) {
					return static_cast<const Base&>(lhs) >= static_cast<const Base&>(rhs);
				}
			protected:
				typedef Collections::List<Char> Base;

				explicit StringBase<Char>(const Base &src) : Base(src) {
				}
		};
		typedef StringBase<char> AsciiString;
		typedef StringBase<wchar_t> WideString;
		typedef StringBase<TCHAR> String;

		template <typename T, typename Char = TCHAR> StringBase<Char> ToString(const T &x) {
			std::basic_stringstream<Char> ss;
			ss<<x;
			return StringBase<Char>(ss.str().c_str());
		}
		template <typename T, typename Char = TCHAR> T Extract(const StringBase<Char> &str) {
			std::basic_stringstream<Char> ss(*str);
			T result;
			if (!static_cast<bool>(ss>>result)) {
				throw InvalidArgumentException(_TEXT("extraction failed"));
			}
			return result;
		}
		template <typename T, typename Char = TCHAR> bool TryExtract(const StringBase<Char> &str, T &res) {
			std::basic_stringstream<Char> ss(*str);
			return static_cast<bool>(ss>>res);
		}
	}
}
