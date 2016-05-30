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
		template <typename T> struct SharedPointer {
			public:
				SharedPointer(T *ptr = nullptr) : _ptr((T**)GlobalAllocator::Allocate(sizeof(T**))) {
					(*_ptr) = ptr;
				}
				SharedPointer(T *ptr, const std::function<void(T*)> &freeFunc) :
					_ptr((T**)GlobalAllocator::Allocate(sizeof(T*))), _freeFunc(freeFunc)
				{
					(*_ptr) = ptr;
				}
				SharedPointer(const SharedPointer<T>&) = default;
				SharedPointer<T> &operator =(T *ptr) {
					SetOwnPointer(ptr);
					return *this;
				}
				SharedPointer<T> &operator =(const SharedPointer<T> &src) {
					if (this == &src) {
						return *this;
					}
					SetOwnPointer(src);
					return *this;
				}
				~SharedPointer() {
					if (_counter.Count() == 1) {
						if (_freeFunc) {
							_freeFunc(*_ptr);
						}
						GlobalAllocator::Free(_ptr);
					}
				}

				T &operator *() {
					if (!(*_ptr)) {
						throw InvalidOperationException(_TEXT("trying to dereference nullptr"));
					}
					return **_ptr;
				}
				const T &operator *() const {
					if (!(*_ptr)) {
						throw InvalidOperationException(_TEXT("trying to dereference nullptr"));
					}
					return **_ptr;
				}

				T *operator ->() {
					return *_ptr;
				}
				const T *operator ->() const {
					return *_ptr;
				}

				operator T*() {
					return *_ptr;
				}
				operator const T*() const {
					return *_ptr;
				}

				T &GetObject() {
					if (!(*_ptr)) {
						throw InvalidOperationException(_TEXT("trying to dereference nullptr"));
					}
					return **_ptr;
				}
				const T &GetObject() const {
					if (!(*_ptr)) {
						throw InvalidOperationException(_TEXT("trying to dereference nullptr"));
					}
					return **_ptr;
				}
				T *GetPointer() {
					return *_ptr;
				}
				const T *GetPointer() const {
					return *_ptr;
				}
				void SetSharedPointer(const SharedPointer<T> &ptr) {
					throw InvalidOperationException(_TEXT("operation not supported"));
				}
				void SetSharedPointer(T *ptr) {
					(*_ptr) = ptr;
				}
				void SetOwnPointer(const SharedPointer<T> &ptr) {
					if (_counter.Count() == 1) {
						if (_freeFunc) {
							_freeFunc(*_ptr);
						}
						GlobalAllocator::Free(_ptr);
					}
					_counter = ptr._counter;
					_freeFunc = ptr._freeFunc;
					_ptr = ptr._ptr;
				}
				void SetOwnPointer(T *ptr) {
					if (_counter.Count() == 1) {
						if (_freeFunc) {
							_freeFunc(*_ptr);
						}
						GlobalAllocator::Free(_ptr);
					}
					_counter = ReferenceCounter();
					_ptr = (T**)GlobalAllocator::Allocate(sizeof(T**));
					(*_ptr) = ptr;
				}

				void ManuallyIncrease(size_t val) {
					_counter.ManuallyIncrease(val);
				}
				void ManuallyDecrease(size_t val) {
					_counter.ManuallyDecrease(val);
				}

				size_t Count() const {
					return _counter.Count();
				}

				std::function<void(T*)> &AutoFreeFunction() {
					return _freeFunc;
				}
				const std::function<void(T*)> &AutoFreeFunction() const {
					return _freeFunc;
				}
			private:
				ReferenceCounter _counter;
				T **_ptr = nullptr;
				std::function<void(T*)> _freeFunc;
		};
	}
}
