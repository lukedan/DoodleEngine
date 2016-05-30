#pragma once

//#define TEST_KERNEL_LIST
#ifdef DEBUG
#	define STRICT_RUNTIME_CHECK
#endif

#include <cstdlib>
#include <cstring>
#include <functional>

#include "Common.h"
#include "ObjectAllocator.h"
#include "Exceptions.h"
#include "ReferenceCounter.h"

#ifdef TEST_KERNEL_LIST
	#include <vector>
#endif

namespace DE {
	namespace Core {
		namespace Collections {
			// NOTE list.PushBack(list.Last()) IS A TRAP!
#ifndef TEST_KERNEL_LIST
			template <typename T, bool DirectMemoryAccess = !IsClass<T>::Result> class List {
				public:
					constexpr static size_t MinCapicy = 5;

					List() :
						_arr(
							(T*)GlobalAllocator::Allocate(sizeof(T) * MinCapicy),
							GetMyAutoFreeFunction()
						)
					{
					}
					List(const T &obj, size_t count) : List() {
						PushBack(obj, count);
					}
					List(const List<T, DirectMemoryAccess> &src) : _count(src._count), _capicy(src._capicy), _arr(src._arr) {
						_arr.AutoFreeFunction() = GetMyAutoFreeFunction();
					}
					List<T, DirectMemoryAccess> &operator =(const List<T, DirectMemoryAccess> &src) {
						if (this == &src) {
							return *this;
						}
						_arr = src._arr;
						_count = src._count;
						_capicy = src._capicy;
						_arr.AutoFreeFunction() = GetMyAutoFreeFunction();
						return *this;
					}
					virtual ~List() {
#ifdef STRICT_RUNTIME_CHECK
						if (!_arr.AutoFreeFunction()) {
							throw InvalidOperationException(_TEXT("bug here"));
						}
#endif
					}

					void PushBack(const T &obj, size_t count = 1) {
						size_t oc = _count;
						IncreaseCount(count);
						for (T *cur = _arr + oc, *tar = cur + count; cur != tar; ++cur) {
							if (DirectMemoryAccess) {
								memcpy(cur, &obj, sizeof(T));
							} else {
								new (cur) T(obj);
							}
						}
					}
					void PushBackRange(const List<T, DirectMemoryAccess> &range) {
						PushBackRange(*range, range.Count());
					}
					void PushBackRange(const T *range, size_t count) {
						size_t oc = _count;
						IncreaseCount(count);
						T *cur = _arr + oc;
						if (DirectMemoryAccess) {
							memcpy(cur, range, sizeof(T) * count);
						} else {
							for (const T *tar = cur + count, *rc = range; cur != tar; ++cur, ++rc) {
								new (cur) T(*rc);
							}
						}
					}
					static List<T, DirectMemoryAccess> Concat(
						const List<T, DirectMemoryAccess> &lhs,
						const List<T, DirectMemoryAccess> &rhs
					) {
						List<T, DirectMemoryAccess> result = lhs;
						result.PushBackRange(rhs);
						return result;
					}

					T PopBack() {
#ifdef STRICT_RUNTIME_CHECK
						if (_count == 0) {
							throw InvalidOperationException(_TEXT("the List is empty"));
						}
#endif
						if (DirectMemoryAccess) {
							DecreaseCount(1);
							return _arr[_count];
						}
						T result = _arr[_count - 1];
						(_arr + (_count - 1))->~T();
						DecreaseCount(1);
						return result;
					}

					void Swap(size_t a, size_t b) {
#ifdef STRICT_RUNTIME_CHECK
						if (a >= _count || b >= _count) {
							throw InvalidArgumentException(_TEXT("index overflow"));
						}
#endif
						OnChanged();
						if (DirectMemoryAccess) {
							T *tmp = (T*)GlobalAllocator::Allocate(sizeof(T)), *aad = _arr + a, *bad = _arr + b;
							memcpy(tmp, aad, sizeof(T));
							memcpy(aad, bad, sizeof(T));
							memcpy(bad, tmp, sizeof(T));
							GlobalAllocator::Free(tmp);
						} else {
							T &aad = _arr[a], &bad = _arr[b], tempObj = aad;
							aad = bad;
							bad = tempObj;
						}
					}
					void SwapToBack(size_t index) {
						Swap(index, _count - 1);
					}

					T &At(size_t index) {
#ifdef STRICT_RUNTIME_CHECK
						if (index >= _count) {
							throw InvalidArgumentException(_TEXT("index overflow"));
						}
#endif
						OnChanged();
						return _arr[index];
					}
					const T &At(size_t index) const {
#ifdef STRICT_RUNTIME_CHECK
						if (index >= _count) {
							throw InvalidArgumentException(_TEXT("index overflow"));
						}
#endif
						return _arr[index];
					}
					T &operator [](size_t index) {
						return At(index);
					}
					const T &operator [](size_t index) const {
						return At(index);
					}
					T &First() {
						return At(0);
					}
					const T &First() const {
						return At(0);
					}
					T &Last() {
						return At(_count - 1);
					}
					const T &Last() const {
						return At(_count - 1);
					}

					void Remove(size_t start, size_t count = 1) {
#ifdef STRICT_RUNTIME_CHECK
						if (start + count > _count) {
							throw InvalidArgumentException(_TEXT("index overflow"));
						}
#endif
						OnChanged();
						if (DirectMemoryAccess) {
							size_t sz = (_count - start - count) * sizeof(T);
							if (sz > 0) {
								T *buf = (T*)GlobalAllocator::Allocate(sz);
								memcpy(buf, _arr + start + count, sz);
								memcpy(_arr + start, buf, sz);
								GlobalAllocator::Free(buf);
							}
						} else {
							T *cur = _arr + start;
							for (T *tar = cur + count, *fin = _arr + _count; tar != fin; ++cur, ++tar) {
								(*cur) = (*tar);
							}
							for (T *tar = _arr + _count; cur != tar; ++cur) {
								cur->~T();
							}
						}
						DecreaseCount(count);
					}
					bool TryRemoveFirst(const T &obj) {
						size_t index = FindFirst(obj);
						if (index >= _count) {
							return false;
						}
						Remove(index);
						return true;
					}
					bool TryRemoveLast(const T &obj) {
						size_t index = FindLast(obj);
						if (index >= _count) {
							return false;
						}
						Remove(index);
						return true;
					}

					void Insert(size_t index, const T &obj) {
						Insert(index, &obj, 1);
					}
					void Insert(size_t index, const T *objs, size_t count) {
						size_t oc = _count;
						IncreaseCount(count);
						if (DirectMemoryAccess) {
							size_t sz = (oc - index) * sizeof(T);
							if (sz > 0) {
								T *buf = (T*)GlobalAllocator::Allocate(sz);
								memcpy(buf, _arr + index, sz);
								memcpy(_arr + index + count, buf, sz);
								GlobalAllocator::Free(buf);
								memcpy(_arr + index, objs, sizeof(T) * count);
							}
						} else {
							T *add = _arr + index + count, *spl = _arr + oc;
							for (T *cur = _arr + _count, *src = spl; cur != add; ) {
								if ((--cur) < spl) {
									(*cur) = *(--src);
								} else {
									new (cur) T(*(--src));
								}
							}
							T *cur = _arr + index;
							for (const T *src = objs; cur != add; ++cur, ++src) {
								if (cur < spl) {
									(*cur) = (*src);
								} else {
									new (cur) T(*src);
								}
							}
						}
					}
					void Insert(size_t index, const List<T, DirectMemoryAccess> &objs) {
						Insert(*objs, index, objs.Count());
					}

					template <typename Predicate = EqualityPredicate<T>> bool Contains(const T &target) const {
						for (const T *cur = _arr, *fin = cur + _count; cur != fin; ++cur) {
							if (Predicate::Examine(target, *cur)) {
								return true;
							}
						}
						return false;
					}
					template <typename Predicate = EqualityPredicate<T>> size_t FindFirst(const T &target) const {
						size_t result = 0;
						const T *fin = _arr + _count;
						for (const T *cur = _arr; cur != fin; ++cur, ++result) {
							if (Predicate::Examine(target, *cur)) {
								break;
							}
						}
						return result;
					}
					template <typename Predicate = EqualityPredicate<T>> size_t FindLast(const T &target) const {
						size_t result = _count;
						const T *fin = _arr;
						for (const T *cur = _arr + _count; cur != fin; --result) {
							if (Predicate::Examine(target, *(--cur))) {
								return --result;
							}
						}
						return _count;
					}

					void ForEach(const std::function<bool(T&)> &func) {
#ifdef STRICT_RUNTIME_CHECK
						++_inFE;
#endif
						T *fin = _arr + _count;
						for (T *cur = _arr; cur != fin; ++cur) {
							if (!func(*cur)) {
								break;
							}
						}
#ifdef STRICT_RUNTIME_CHECK
						--_inFE;
#endif
					}
					void ForEach(const std::function<bool(const T&)> &func) const {
#ifdef STRICT_RUNTIME_CHECK
						++_inFE;
#endif
						const T *fin = _arr + _count;
						for (const T *cur = _arr; cur != fin; ++cur) {
							if (!func(*cur)) {
								break;
							}
						}
#ifdef STRICT_RUNTIME_CHECK
						--_inFE;
#endif
					}

					List<T, DirectMemoryAccess> SubSequence(size_t start) const {
						return SubSequence(start, _count - start);
					}
					List<T, DirectMemoryAccess> SubSequence(size_t start, size_t count) const {
#ifdef STRICT_RUNTIME_CHECK
						if (start > _count || start + count > _count) {
							throw InvalidArgumentException(_TEXT("index overflow"));
						}
#endif
						List<T, DirectMemoryAccess> result;
						result.PushBackRange(_arr + start, count);
						return result;
					}

					void Clear() {
						_arr = (T*)GlobalAllocator::Allocate(sizeof(T) * MinCapicy);
						_capicy = MinCapicy;
						_count = 0;
					}

					T *operator *() {
						return _arr;
					}
					const T *operator *() const {
						return _arr;
					}

					friend bool operator ==(const List &lhs, const List &rhs) {
						if (lhs._count != rhs._count) {
							return false;
						}
						if (static_cast<const T*>(lhs._arr) == static_cast<const T*>(rhs._arr)) {
							return true;
						}
						if (DirectMemoryAccess) {
							return memcmp(lhs._arr, rhs._arr, sizeof(T) * lhs._count) == 0;
						}
						for (const T *lc = lhs._arr, *le = lc + lhs._count, *rc = rhs._arr; lc != le; ++lc, ++rc) {
							if ((*lc) != (*rc)) {
								return false;
							}
						}
						return true;
					}
					friend bool operator !=(const List &lhs, const List &rhs) {
						return !(lhs == rhs);
					}

					friend bool operator >(const List &lhs, const List &rhs) {
						for (
							const T *lc = lhs._arr, *le = lc + lhs._count, *rc = rhs._arr, *re = rc + rhs._count;
							lc != le && rc != re;
							++lc, ++rc
						) {
							if (DirectMemoryAccess) {
								int mcr = memcmp(lc, rc, sizeof(T));
								if (mcr > 0) {
									return true;
								} else if (mcr < 0) {
									return false;
								}
							} else {
								if ((*lc) > (*rc)) {
									return true;
								} else if ((*lc) < (*rc)) {
									return false;
								}
							}
						}
						return false;
					}
					friend bool operator <(const List &lhs, const List &rhs) {
						return rhs > lhs;
					}
					friend bool operator >=(const List &lhs, const List &rhs) {
						return!(rhs > lhs);
					}
					friend bool operator <=(const List &lhs, const List &rhs) {
						return !(lhs > rhs);
					}

					void Reverse() {
						for (size_t i = 0; i * 2 + 1 < _count; ++i) {
							Swap(i, _count - i - 1);
						}
					}

					size_t Count() const {
						return _count;
					}
					size_t Capicy() const {
						return _capicy;
					}
				private:
					void OnChanged() {
						if (_arr.Count() > 1) {
							ChangeCapicy(_capicy);
						}
					}
					void DecreaseCount(size_t delta) {
#ifdef STRICT_RUNTIME_CHECK
						if (_inFE > 0) {
							throw InvalidOperationException(_TEXT("the list is currently being enumerated, cannot change its content"));
						}
#endif
						if (_capicy > MinCapicy && _count < (_capicy>>2)) {
							size_t newCap = _capicy>>1;
							while (newCap > MinCapicy && _count < (newCap>>2)) {
								newCap >>= 1;
							}
							ChangeCapicy(newCap);
						} else {
							OnChanged();
						}
						_count -= delta;
					}
					void IncreaseCount(size_t delta) {
#ifdef STRICT_RUNTIME_CHECK
						if (_inFE > 0) {
							throw InvalidOperationException(_TEXT("the list is currently being enumerated, cannot change its content"));
						}
#endif
						size_t newC = _count + delta;
						if (newC > _capicy) {
							size_t newCap = _capicy<<1;
							while (newCap < newC) {
								newCap <<= 1;
							}
							ChangeCapicy(newCap);
						} else {
							OnChanged();
						}
						_count += delta;
					}
					void ChangeCapicy(size_t to) {
						_capicy = to;
						T *newArr = (T*)GlobalAllocator::Allocate(sizeof(T) * _capicy);
						if (DirectMemoryAccess) {
							memcpy(newArr, _arr, sizeof(T) * _count);
						} else {
							T *lst = _arr + _count;
							for (T *cur = newArr + _count; cur != newArr; ) {
								new (--cur) T(*(--lst));
							}
						}
						_arr = newArr;
					}
					std::function<void(T*)> GetMyAutoFreeFunction() const {
						return [this](T *ptr) {
							if (ptr) {
								if (!DirectMemoryAccess) {
									for (T *cur = ptr + _count; cur != ptr; ) {
										(--cur)->~T();
									}
								}
								GlobalAllocator::Free(ptr);
							}
						};
					}

					size_t _count = 0, _capicy = MinCapicy;
					SharedPointer<T> _arr;
#ifdef STRICT_RUNTIME_CHECK
					mutable size_t _inFE = 0;
#endif
			};
#else
			template <typename T> class TypeWrapper {
				public:
					typedef T Type;
					typedef T RealType;
					typedef std::vector<T> ContainerType;
			};
			struct WrappedBoolean {
				public:
					WrappedBoolean() = default;
					WrappedBoolean(bool v) : Value(v) {
					}

					bool Value = false;

					operator bool&() {
						return Value;
					}
					operator const bool&() const {
						return Value;
					}
			};
			template <> class TypeWrapper<bool> {
				public:
					typedef WrappedBoolean Type;
					typedef bool RealType;
					typedef std::vector<WrappedBoolean> ContainerType;
			};
			template <typename T, bool DirectMemoryAccess = !IsClass<T>::Result> class List {
				public:
					List() {
					}
					List(const typename TypeWrapper<T>::Type &obj, size_t count) : _vec(count, obj) {
					}
					virtual ~List() {
					}

					void PushBack(const typename TypeWrapper<T>::Type &obj) {
						_vec.push_back(obj);
					}
					void PushBack(const typename TypeWrapper<T>::Type &obj, size_t count) {
						for (size_t i = 0; i < count; ++i) {
							_vec.push_back(obj);
						}
					}
					void PushBackRange(const List<typename TypeWrapper<T>::Type, DirectMemoryAccess> &rhs) {
						PushBackRange(*rhs, rhs.Count());
					}
					void PushBackRange(const typename TypeWrapper<T>::Type *rhs, size_t count) {
						for (size_t i = 0; i < count; ++i) {
							_vec.push_back(rhs[i]);
						}
					}
					static List<typename TypeWrapper<T>::Type, DirectMemoryAccess> Concat(
						const List<typename TypeWrapper<T>::Type, DirectMemoryAccess> &lhs,
						const List<typename TypeWrapper<T>::Type, DirectMemoryAccess> &rhs
					) {
						List<typename TypeWrapper<T>::Type, DirectMemoryAccess> result = lhs;
						for (size_t i = 0; i < rhs.Count(); ++i) {
							result.PushBack(rhs[i]);
						}
						return result;
					}
					typename TypeWrapper<T>::Type PopBack() {
						typename TypeWrapper<T>::Type tmp = _vec[_vec.size() - 1];
						_vec.pop_back();
						return tmp;
					}
					void Swap(size_t a, size_t b) {
						typename TypeWrapper<T>::Type tmp = _vec[a];
						_vec[a] = _vec[b];
						_vec[b] = tmp;
					}
					void SwapToBack(size_t index) {
						Swap(index, _vec.size() - 1);
					}

					typename TypeWrapper<T>::Type &operator [](size_t index) {
						return _vec[index];
					}
					const typename TypeWrapper<T>::Type &operator [](size_t index) const {
						return _vec[index];
					}
					typename TypeWrapper<T>::Type &At(size_t index) {
						return _vec[index];
					}
					const typename TypeWrapper<T>::Type &At(size_t index) const {
						return _vec[index];
					}
					typename TypeWrapper<T>::Type &First() {
						return _vec[0];
					}
					const typename TypeWrapper<T>::Type &First() const {
						return _vec[0];
					}
					typename TypeWrapper<T>::Type &Last() {
						return _vec[_vec.size() - 1];
					}
					const typename TypeWrapper<T>::Type &Last() const {
						return _vec[_vec.size() - 1];
					}

					bool Contains(const typename TypeWrapper<T>::Type &obj) const {
						for (size_t i = 0; i < _vec.size(); ++i) {
							if (_vec[i] == obj) {
								return true;
							}
						}
						return false;
					}
					template <typename Predicate = EqualityPredicate<T>> size_t FindFirst(const T &obj) const {
						for (size_t i = 0; i < _vec.size(); ++i) {
							if (Predicate::Examine(obj, _vec[i])) {
								return i;
							}
						}
						return _vec.size();
					}
					template <typename Predicate = EqualityPredicate<T>> size_t FindLast(const T &obj) const {
						size_t i = _vec.size();
						do {
							--i;
							if (Predicate::Examine(obj, _vec[i])) {
								return i;
							}
						} while (i > 0);
						return _vec.size();
					}
					bool TryRemoveFirst(const typename TypeWrapper<T>::Type &obj) {
						size_t f = FindFirst(obj);
						if (f >= _vec.size()) {
							return false;
						}
						Remove(f);
						return true;
					}
					bool TryRemoveLast(const typename TypeWrapper<T>::Type &obj) {
						size_t f = FindLast(obj);
						if (f >= _vec.size()) {
							return false;
						}
						Remove(f);
						return true;
					}
					void Remove(size_t start, size_t count = 1) {
						for (size_t i = 0; i < count; ++i) {
							_vec.erase(_vec.begin() + start);
						}
					}
					void Insert(size_t position, const typename TypeWrapper<T>::Type &obj) {
						Insert(position, &obj, 1);
					}
					void Insert(size_t position, const typename TypeWrapper<T>::Type *objs, size_t count) {
						for (size_t i = position, j = 0; j < count; ++j) {
							_vec.insert(_vec.begin() + i + j, objs[j]);
						}
					}
					template <bool DirectMemAccess> void Insert(size_t position, const List<typename TypeWrapper<T>::Type, DirectMemAccess> &list) {
						Insert(position, *list, list.Count());
					}
					void Clear() {
						_vec.clear();
					}

					List<typename TypeWrapper<T>::Type, DirectMemoryAccess> SubSequence(size_t start) const {
						std::vector<typename TypeWrapper<T>::Type> res(_vec.begin() + start, _vec.end());
						List<typename TypeWrapper<T>::Type, DirectMemoryAccess> reslist;
						reslist._vec = res;
						return reslist;
					}
					List<typename TypeWrapper<T>::Type, DirectMemoryAccess> SubSequence(size_t start, size_t count) const {
						std::vector<typename TypeWrapper<T>::Type> res(_vec.begin() + start, _vec.begin() + start + count);
						List<typename TypeWrapper<T>::Type, DirectMemoryAccess> reslist;
						reslist._vec = res;
						return reslist;
					}

					void ForEach(const std::function<bool(const typename TypeWrapper<T>::RealType&)> &func) const {
						for (size_t i = 0; i < _vec.size(); ++i) {
							if (!func((const typename TypeWrapper<T>::RealType&)_vec[i])) {
								return;
							}
						}
					}
					void ForEach(const std::function<bool(typename TypeWrapper<T>::RealType&)> &func) {
						for (size_t i = 0; i < _vec.size(); ++i) {
							if (!func((typename TypeWrapper<T>::RealType&)_vec[i])) {
								return;
							}
						}
					}

					bool operator ==(
						const List<typename TypeWrapper<T>::Type, DirectMemoryAccess> &rhs
					) const {
						return _vec == rhs._vec;
					}
					bool operator !=(
						const List<typename TypeWrapper<T>::Type, DirectMemoryAccess> &rhs
					) const {
						return _vec != rhs._vec;
					}
					bool operator >(
						const List<typename TypeWrapper<T>::Type, DirectMemoryAccess> &rhs
					) const {
						return _vec > rhs._vec;
					}
					bool operator <(
						const List<typename TypeWrapper<T>::Type, DirectMemoryAccess> &rhs
					) const {
						return _vec < rhs._vec;
					}
					bool operator >=(
						const List<typename TypeWrapper<T>::Type, DirectMemoryAccess> &rhs
					) const {
						return _vec >= rhs._vec;
					}
					bool operator <=(
						const List<typename TypeWrapper<T>::Type, DirectMemoryAccess> &rhs
					) const {
						return _vec <= rhs._vec;
					}

					size_t Capicy() const {
						return _vec.capacity();
					}
					size_t Count() const {
						return _vec.size();
					}

					const typename TypeWrapper<T>::Type *operator *() const {
						return &_vec[0];
					}

					constexpr static size_t MinCapicy = 5;
				private:
					typename TypeWrapper<T>::ContainerType _vec;
			};
#endif
		}
	}
}
