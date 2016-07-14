#pragma once

//#define TEST_KERNEL_LIST

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

					List() = default;
					List(const T &obj, size_t count) : List() {
						PushBack(obj, count);
					}
					virtual ~List() {
					}

					void PushBack(const T &obj, size_t count = 1) {
						size_t oc = Count();
						IncreaseCount(count);
						for (T *cur = _data->GetArray() + oc, *tar = cur + count; cur != tar; ++cur) {
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
						if (count == 0) {
							return;
						}
						size_t oc = Count();
						IncreaseCount(count);
						T *cur = _data->GetArray() + oc;
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
						if (Count() == 0) {
							throw InvalidOperationException(_TEXT("the List is empty"));
						}
#endif
						CheckShrink();
						if (DirectMemoryAccess) {
							--_data->Count;
							return _data->GetArray()[_data->Count];
						}
						T result = _data->GetArray()[--_data->Count];
						(_data->GetArray() + _data->Count)->~T();
						return result;
					}

					void Swap(size_t a, size_t b) {
#ifdef STRICT_RUNTIME_CHECK
						if (a >= Count() || b >= Count()) {
							throw InvalidArgumentException(_TEXT("index overflow"));
						}
#endif
						OnChanged();
						if (DirectMemoryAccess) {
							T *tmp = (T*)GlobalAllocator::Allocate(sizeof(T)), *aad = _data->GetArray() + a, *bad = _data->GetArray() + b;
							memcpy(tmp, aad, sizeof(T));
							memcpy(aad, bad, sizeof(T));
							memcpy(bad, tmp, sizeof(T));
							GlobalAllocator::Free(tmp);
						} else {
							T &aad = _data->GetArray()[a], &bad = _data->GetArray()[b], tempObj = aad;
							aad = bad;
							bad = tempObj;
						}
					}
					void SwapToBack(size_t index) {
						Swap(index, _data->Count - 1);
					}

					T &At(size_t index) {
#ifdef STRICT_RUNTIME_CHECK
						if (index >= Count()) {
							throw InvalidArgumentException(_TEXT("index overflow"));
						}
#endif
						OnChanged();
						return _data->GetArray()[index];
					}
					const T &At(size_t index) const {
#ifdef STRICT_RUNTIME_CHECK
						if (index >= Count()) {
							throw InvalidArgumentException(_TEXT("index overflow"));
						}
#endif
						return _data->GetArray()[index];
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
						return At(_data->Count - 1);
					}
					const T &Last() const {
						return At(_data->Count - 1);
					}

					void Remove(size_t start, size_t count = 1) {
						if (count == 0) {
							return;
						}
#ifdef STRICT_RUNTIME_CHECK
						if (start + count > Count()) {
							throw InvalidArgumentException(_TEXT("index overflow"));
						}
#endif
						CheckShrink();
						if (DirectMemoryAccess) {
							size_t sz = (_data->Count - start - count) * sizeof(T);
							if (sz > 0) {
								T *buf = (T*)GlobalAllocator::Allocate(sz);
								memcpy(buf, _data->GetArray() + start + count, sz);
								memcpy(_data->GetArray() + start, buf, sz);
								GlobalAllocator::Free(buf);
							}
						} else {
							T *cur = _data->GetArray() + start;
							for (T *tar = cur + count, *fin = _data->GetArray() + _data->Count; tar != fin; ++cur, ++tar) {
								(*cur) = (*tar);
							}
							for (T *tar = _data->GetArray() + _data->Count; cur != tar; ++cur) {
								cur->~T();
							}
						}
						_data->Count -= count;
					}
					bool TryRemoveFirst(const T &obj) {
						size_t index = FindFirst(obj);
						if (index >= _data->Count) {
							return false;
						}
						Remove(index);
						return true;
					}
					bool TryRemoveLast(const T &obj) {
						size_t index = FindLast(obj);
						if (index >= _data->Count) {
							return false;
						}
						Remove(index);
						return true;
					}

					void Insert(size_t index, const T *objs, size_t count) {
						if (count == 0) {
							return;
						}
						size_t oc = Count();
						IncreaseCount(count);
						if (DirectMemoryAccess) {
							size_t sz = (oc - index) * sizeof(T);
							if (sz > 0) {
								T *buf = (T*)GlobalAllocator::Allocate(sz);
								memcpy(buf, _data->GetArray() + index, sz);
								memcpy(_data->GetArray() + index + count, buf, sz);
								GlobalAllocator::Free(buf);
								memcpy(_data->GetArray() + index, objs, sizeof(T) * count);
							}
						} else {
							T *add = _data->GetArray() + index + count, *spl = _data->GetArray() + oc;
							for (T *cur = _data->GetArray() + _data->Count, *src = spl; cur != add; ) {
								if ((--cur) < spl) {
									(*cur) = *(--src);
								} else {
									new (cur) T(*(--src));
								}
							}
							T *cur = _data->GetArray() + index;
							for (const T *src = objs; cur != add; ++cur, ++src) {
								if (cur < spl) {
									(*cur) = (*src);
								} else {
									new (cur) T(*src);
								}
							}
						}
					}
					void Insert(size_t index, const T &obj) {
						Insert(index, &obj, 1);
					}
					void Insert(size_t index, const List<T, DirectMemoryAccess> &objs) {
						Insert(*objs, index, objs.Count());
					}

					template <typename Predicate = EqualityPredicate<T>> bool Contains(const T &target) const {
						if (Count() == 0) {
							return false;
						}
						for (const T *cur = _data->GetArray(), *fin = cur + _data->Count; cur != fin; ++cur) {
							if (Predicate::Examine(target, *cur)) {
								return true;
							}
						}
						return false;
					}
					template <typename Predicate = EqualityPredicate<T>> size_t FindFirst(const T &target) const {
						if (Count() == 0) {
							return 0;
						}
						size_t result = 0;
						const T *fin = _data->GetArray() + _data->Count;
						for (const T *cur = _data->GetArray(); cur != fin; ++cur, ++result) {
							if (Predicate::Examine(target, *cur)) {
								break;
							}
						}
						return result;
					}
					template <typename Predicate = EqualityPredicate<T>> size_t FindLast(const T &target) const {
						if (Count() == 0) {
							return 0;
						}
						size_t result = _data->Count;
						const T *fin = _data->GetArray();
						for (const T *cur = _data->GetArray() + _data->Count; cur != fin; --result) {
							if (Predicate::Examine(target, *(--cur))) {
								return --result;
							}
						}
						return _data->Count;
					}

					void ForEach(const std::function<bool(T&)> &func) {
#ifdef STRICT_RUNTIME_CHECK
						++_inFE;
#endif
						OnChanged();
						if (_data) {
							T *fin = _data->GetArray() + _data->Count;
							for (T *cur = _data->GetArray(); cur != fin; ++cur) {
								if (!func(*cur)) {
									break;
								}
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
						if (_data) {
							const T *fin = _data->GetArray() + _data->Count;
							for (const T *cur = _data->GetArray(); cur != fin; ++cur) {
								if (!func(*cur)) {
									break;
								}
							}
						}
#ifdef STRICT_RUNTIME_CHECK
						--_inFE;
#endif
					}

					List SubSequence(size_t start) const {
						return SubSequence(start, _data->Count - start);
					}
					List SubSequence(size_t start, size_t count) const {
						if (count == 0) {
							return List();
						}
#ifdef STRICT_RUNTIME_CHECK
						if (start + count > Count()) {
							throw InvalidArgumentException(_TEXT("index overflow"));
						}
#endif
						List result;
						result.PushBackRange(_data->GetArray() + start, count);
						return result;
					}

					void Clear() {
						_data = nullptr;
					}

					T *operator *() {
						if (_data) {
							return _data->GetArray();
						}
						return nullptr;
					}
					const T *operator *() const {
						if (_data) {
							return _data->GetArray();
						}
						return nullptr;
					}

					friend bool operator ==(const List &lhs, const List &rhs) {
						if (lhs.Count() != rhs.Count()) {
							return false;
						}
						if (lhs.Count() == 0) {
							return true;
						}
						if (lhs._data->GetArray() == rhs._data->GetArray()) {
							return true;
						}
						if (DirectMemoryAccess) {
							return memcmp(lhs._data->GetArray(), rhs._data->GetArray(), sizeof(T) * lhs._data->Count) == 0;
						}
						for (const T *lc = lhs._data->GetArray(), *le = lc + lhs._data->Count, *rc = rhs._data->GetArray(); lc != le; ++lc, ++rc) {
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
						if (lhs.Count() == 0) {
							return false;
						}
						if (rhs.Count() == 0) {
							return true;
						}
						for (
							const T *lc = lhs._data->GetArray(), *le = lc + lhs._data->Count, *rc = rhs._data->GetArray(), *re = rc + rhs._data->Count;
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
						if (_data) {
							for (size_t i = 0; i * 2 + 1 < _data->Count; ++i) {
								Swap(i, _data->Count - i - 1);
							}
						}
					}

					size_t Count() const {
						if (_data) {
							return _data->Count;
						}
						return 0;
					}
					size_t Capicy() const {
						if (_data) {
							return _data->Capicy;
						}
						return 0;
					}
				private:
					void OnChanged() {
						if (_data.Count() > 1) {
							ChangeCapicy(_data->Capicy);
						}
					}
					void CheckShrink() {
#ifdef STRICT_RUNTIME_CHECK
						if (_inFE > 0) {
							throw InvalidOperationException(_TEXT("the list is currently being enumerated, cannot change its content"));
						}
#endif
						if (_data->Capicy > MinCapicy && _data->Count < (_data->Capicy >> 2)) {
							size_t newCap = _data->Capicy >> 1;
							while (newCap > MinCapicy && _data->Count < (newCap >> 2)) {
								newCap >>= 1;
							}
							ChangeCapicy(newCap);
						} else {
							OnChanged();
						}
					}
					void IncreaseCount(size_t delta) {
#ifdef STRICT_RUNTIME_CHECK
						if (_inFE > 0) {
							throw InvalidOperationException(_TEXT("the list is currently being enumerated, cannot change its content"));
						}
#endif
						size_t newC = Count() + delta;
						if (newC > Capicy()) {
							size_t newCap = (_data ? _data->Capicy << 1 : MinCapicy);
							while (newCap < newC) {
								newCap <<= 1;
							}
							ChangeCapicy(newCap);
						} else {
							OnChanged();
						}
						_data->Count += delta;
					}
					void ChangeCapicy(size_t to) {
						_ListData *nd = CreateListData(to);
						if (_data) {
							nd->Count = _data->Count;
							if (DirectMemoryAccess) {
								memcpy(nd->GetArray(), _data->GetArray(), sizeof(T) * _data->Count);
							} else {
								T *lst = _data->GetArray() + _data->Count;
								for (T *cur = nd->GetArray() + _data->Count; cur != nd->GetArray(); ) {
									new (--cur) T(*(--lst));
								}
							}
						}
						_data = MakeShared(nd);
					}

					struct _ListData {
						_ListData() = default;
						_ListData(size_t cap) : Capicy(cap) {
						}
						~_ListData() {
							if (!DirectMemoryAccess) {
								for (
									T *cur = GetArray(), *end = cur + Count;
									cur != end;
									++cur
								) {
									cur->~T();
								}
							}
						}

						size_t Count = 0, Capicy = MinCapicy;
#ifdef DEBUG
						Core::MemoryMarker<30> _marker{"ListData inner object"};
#endif

						T *GetArray() {
							return reinterpret_cast<T*>(this + 1);
						}
					};
					inline static size_t GetListDataSize(size_t capicy) {
						return sizeof(_ListData) + sizeof(T) * capicy;
					}
					inline static size_t GetListDataSize(const _ListData *data) {
						return GetListDataSize(data->Capicy);
					}
					inline static _ListData *CreateListData(size_t capicy) {
						return new (GlobalAllocator::Allocate(GetListDataSize(capicy))) _ListData(capicy);
					}

					SharedPointer<_ListData> _data;
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
