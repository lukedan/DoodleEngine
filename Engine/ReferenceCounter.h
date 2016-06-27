#pragma once

#include <functional>

#include "Common.h"
#include "ObjectAllocator.h"

namespace DE {
	namespace Core {
		struct ReferenceCounter {
			public:
				ReferenceCounter() : _val(new (GlobalAllocator::Allocate(sizeof(size_t))) size_t(1)) {
				}
				ReferenceCounter(const ReferenceCounter &src) : _val(src._val) {
					++(*_val);
				}
				ReferenceCounter &operator =(const ReferenceCounter &rhs) {
					if (this == &rhs) {
						return *this;
					}
					if ((--(*_val)) == 0) {
						GlobalAllocator::Free(_val);
					}
					_val = rhs._val;
					++(*_val);
					return *this;
				}
				~ReferenceCounter() {
					if ((--(*_val)) == 0) {
						GlobalAllocator::Free(_val);
					}
				}

				void ManuallyIncrease(size_t val = 1) {
					(*_val) += val;
				}
				void ManuallyDecrease(size_t val = 1) {
					if (val >= (*_val)) {
						throw InvalidOperationException(_TEXT("decreasing too much"));
					}
					(*_val) -= val;
				}

				size_t Count() const {
					return (*_val);
				}
			private:
				size_t *_val;
		};
		struct SharedPointerData {
			template <typename T> SharedPointerData(T *ptr = nullptr) : Pointer(ptr) {
			}
			template <typename T> SharedPointerData(T *ptr, const std::function<void(T*)> &func) : Pointer(ptr), FreeFunc(func ? [func](void *ptr) {
				func(static_cast<T*>(ptr));
			} : std::function<void(void*)>()) {
			}
			~SharedPointerData() {
				if (FreeFunc) {
					FreeFunc(Pointer);
				}
			}

			template <typename T> T *GetPointerAs() const {
				return static_cast<T*>(Pointer);
			}

			template <typename T> void SetFreeFunc(const std::function<void(T*)> &func) {
				if (func) {
					FreeFunc = [func](void *ptr) {
						func(static_cast<T*>(ptr));
					};
				} else {
					FreeFunc = nullptr;
				}
			}

			void *Pointer = nullptr;
			std::function<void(void*)> FreeFunc;
#ifdef DEBUG
			MemoryMarker<30> _marker {"SharedPointer inner data"};
#endif
		};
		template <typename T> struct SharedPointer {
			public:
				SharedPointer(T *ptr = nullptr) : _data(new (GlobalAllocator::Allocate(sizeof(SharedPointerData))) SharedPointerData(ptr)) {
				}
				SharedPointer(T *ptr, const std::function<void(T*)> &freeFunc) : _data(
					new (GlobalAllocator::Allocate(sizeof(SharedPointerData))) SharedPointerData(ptr, freeFunc)
				) {
				}
				template <typename U> SharedPointer &operator =(U *ptr) {
					SetOwnPointer(ptr);
					return *this;
				}
				SharedPointer &operator =(const SharedPointer &src) {
					if (this == &src) {
						return *this;
					}
					SetOwnPointer(src);
					return *this;
				}
				~SharedPointer() {
					OnRefRemoved();
				}

				void SetSharedPointer(const SharedPointer &ptr) {
					throw InvalidOperationException(_TEXT("operation not supported"));
				}
				void SetSharedPointer(T *ptr) {
					_data->Pointer = ptr;
				}
				void SetOwnPointer(const SharedPointer &ptr) {
					if (ptr._data == _data) {
						return;
					}
					OnRefRemoved();
					_refc = ptr._refc;
					_data = ptr._data;
				}
				void SetOwnPointer(T *ptr) {
					OnRefRemoved();
					_refc = ReferenceCounter();
					_data = new (GlobalAllocator::Allocate(sizeof(SharedPointerData))) SharedPointerData(ptr);
				}
				void SetSharedFreeFunc(const std::function<void(T*)> &func) {
					_data->SetFreeFunc(func);
				}
				std::function<void(void*)> GetSharedFreeFunc() const {
					return _data->FreeFunc;
				}

				T &GetObject() const {
					if (_data->Pointer) {
						return *_data->GetPointerAs<T>();
					}
					throw InvalidOperationException(_TEXT("dereferencing nullptr"));
				}
				T &operator *() const {
					return GetObject();
				}
				T *GetPointer() const {
					return _data->GetPointerAs<T>();
				}
				operator T*() const {
					return GetPointer();
				}
				T *operator ->() const {
					return GetPointer();
				}

				size_t Count() const {
					return _refc.Count();
				}
				void ManuallyIncrease(size_t val = 1) {
					_refc.ManuallyIncrease(val);
				}
				void ManuallyDecrease(size_t val = 1) {
					_refc.ManuallyDecrease(val);
				}

				template <typename U> SharedPointer<U> CastTo() const {
					return SharedPointer<U>(_refc, _data);
				}
				template <typename U> explicit operator SharedPointer<U>() const {
					return CastTo<T>();
				}
			protected:
				ReferenceCounter _refc;
				SharedPointerData *_data;

				SharedPointer(const ReferenceCounter &refc, SharedPointerData *data) : _refc(refc), _data(data) {
				}

				void OnRefRemoved() {
					if (_refc.Count() == 1) {
						_data->~SharedPointerData();
						GlobalAllocator::Free(_data);
					}
				}
		};
		// function below about shared objects all use DE::Core::GlobalAllocator
		template <typename T, typename ...Args> inline SharedPointer<T> CreateSharedObject(Args&&... args) {
			T *obj = new (GlobalAllocator::Allocate(sizeof(T))) T(std::forward<Args>(args)...);
			return SharedPointer<T>(obj, [](T *ptr) {
				ptr->~T();
				GlobalAllocator::Free(ptr);
			});
		}
		template <typename T, typename RealType, typename ...Args> inline SharedPointer<T> CreateSharedObjectAs(Args&&... args) {
			T *obj = static_cast<T*>(new (GlobalAllocator::Allocate(sizeof(RealType))) RealType(std::forward<Args>(args)...));
			return SharedPointer<T>(obj, [](T *ptr) {
				ptr->~T();
				GlobalAllocator::Free(ptr);
			});
		}
		template <typename T> inline SharedPointer<T> MakeShared(T *obj) {
			return SharedPointer<T>(obj, [](T *ptr) {
				ptr->~T();
				GlobalAllocator::Free(ptr);
			});
		}
	}
}
