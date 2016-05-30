#pragma once

#include <typeinfo>

#include "BinarySearchTree.h"
#include "String.h"

namespace DE {
	namespace Core {
		enum class PropertyType {
			ReadWrite,
			WriteOnly,
			ReadOnly
		};
		template <typename T, PropertyType Type = PropertyType::ReadWrite, bool DirectMemoryAccess = !IsClass<T>::Result> class ReferenceProperty {
			public:
				ReferenceProperty() {
					DefaultInit(_Helper<IsClass<T>::Result>());
				}
				constexpr ReferenceProperty(const T &v) : _val(v) {
				}
				virtual ~ReferenceProperty() {
				}

				T &Get() {
					StaticAssert(Type != PropertyType::WriteOnly, "cannot get a write-only property");
					return _val;
				}
				const T &Get() const {
					StaticAssert(Type != PropertyType::WriteOnly, "cannot get a write-only property");
					return _val;
				}
				operator T&() {
					return _val;
				}
				operator const T&() const {
					return _val;
				}
				T &operator *() {
					return Get();
				}
				const T &operator *() const {
					return Get();
				}
				T *operator ->() {
					return &Get();
				}
				const T *operator ->() const {
					return &Get();
				}
				void Set(const T &obj) {
					StaticAssert(Type != PropertyType::ReadOnly, "cannot set a read-only property");
					if (DirectMemoryAccess) {
						memcpy(&_val, &obj, sizeof(T));
					} else {
						_val = obj;
					}
				}
				template <typename U> U As() const {
					return (U)_val;
				}
				ReferenceProperty<T, Type, DirectMemoryAccess> &operator =(const T &obj) {
					Set(obj);
					return *this;
				}
			protected:
				T _val;

				template <bool V> class _Helper {
				};
				void DefaultInit(const _Helper<true>&) {
				}
				void DefaultInit(const _Helper<false>&) {
					_val = static_cast<T>(0);
				}
		};
		template <typename T, PropertyType Type = PropertyType::ReadWrite> class GetSetProperty {
			public:
				typedef std::function<void(const T&)> Setter;
				typedef std::function<T()> Getter;

				constexpr explicit GetSetProperty(const Setter &setter) : _setter(setter) {
					StaticAssert(Type == PropertyType::WriteOnly, "the property must be write-only");
				}
				constexpr explicit GetSetProperty(const Getter &getter) : _getter(getter) {
					StaticAssert(Type == PropertyType::ReadOnly, "the property must be read-only");
				}
				constexpr GetSetProperty(const Setter &setter, const Getter &getter) : _setter(setter), _getter(getter) {
					StaticAssert(Type == PropertyType::ReadWrite, "the property must be readable and writable");
				}
				constexpr GetSetProperty(const GetSetProperty<T, Type>&) = default;
				GetSetProperty &operator =(const GetSetProperty &rhs) {
					Set(rhs.Get());
					return *this;
				}
				virtual ~GetSetProperty() {
				}

				T Get() const {
					StaticAssert(Type != PropertyType::WriteOnly, "cannot get a write-only property");
					return _getter();
				}
				operator T() const {
					return Get();
				}
				T operator ()() {
					return Get();
				}
				void Set(const T &obj) {
					StaticAssert(Type != PropertyType::ReadOnly, "cannot set a read-only property");
					_setter(obj);
				}
				GetSetProperty<T, Type> &operator =(const T &obj) {
					Set(obj);
					return *this;
				}
			protected:
				Setter _setter = nullptr;
				Getter _getter = nullptr;
		};
	}
}
