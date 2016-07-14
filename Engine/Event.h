#pragma once

#include <functional>

#include "List.h"
#include "LinkedList.h"
#include "ObjectAllocator.h"

namespace DE {
	namespace Core {
		template <typename T> class Event;
		template <typename T> class AutomaticEventHandlerToken;
		template <typename T> struct TemporaryEventHandlerToken {
				friend class Event<T>;
				friend class AutomaticEventHandlerToken<T>;
			protected:
				explicit TemporaryEventHandlerToken(typename Collections::LinkedList<typename Event<T>::HandlerInfo>::Node *node) : _data(node) {
				}

				typename Collections::LinkedList<typename Event<T>::HandlerInfo>::Node *_data = nullptr;
		};
		template <typename T> struct AutomaticEventHandlerToken {
				friend class Event<T>;
			public:
				AutomaticEventHandlerToken() = default;
				AutomaticEventHandlerToken(const TemporaryEventHandlerToken<T> &tok) {
					SetPointer(tok);
				}
				AutomaticEventHandlerToken &operator =(const TemporaryEventHandlerToken<T> &tok) {
					CheckPointer();
					SetPointer(tok);
					return *this;
				}
				~AutomaticEventHandlerToken() {
					CheckPointer();
				}
			protected:
				SharedPointer<typename Collections::LinkedList<typename Event<T>::HandlerInfo>::Node> _pointer;

				void SetPointer(const TemporaryEventHandlerToken<T> &tok) {
					if (tok._data) {
						if (tok._data->Data().Pointer) {
							_pointer = tok._data->Data().Pointer;
						} else {
							_pointer = tok._data->Data().Pointer = tok._data;
						}
					} else {
						_pointer = nullptr;
					}
				}
				void CheckPointer() {
					if (_pointer && _pointer.Count() == 2) {
						_pointer->Data().AttachedTo->RemoveHandler(_pointer);
					}
				}
		};
        template <typename T> class Event {
        		friend class TemporaryEventHandlerToken<T>;
        		friend class AutomaticEventHandlerToken<T>;
        	public:
        		typedef std::function<void(const T&)> Handler;

        		Event() : _hands() {
        		}
        		Event(const Event&) = delete;
        		Event &operator =(const Event&) = delete;

        		TemporaryEventHandlerToken<T> AddHandler(const Handler &hand) {
        			HandlerInfo info(hand, *this);
					return TemporaryEventHandlerToken<T>(_hands.InsertLast(info));
				}
        		TemporaryEventHandlerToken<T> operator +=(const Handler &hand) {
					return AddHandler(hand);
				}

				void RemoveHandler(const AutomaticEventHandlerToken<T> &tok) {
					RemoveHandler(tok._pointer);
				}
				void operator -=(const AutomaticEventHandlerToken<T> &tok) {
					RemoveHandler(tok._pointer);
				}
				void RemoveHandler(const TemporaryEventHandlerToken<T> &tok) {
					RemoveHandler(tok._data);
				}
				void operator -=(const TemporaryEventHandlerToken<T> &tok) {
					RemoveHandler(tok._data);
				}

				void operator ()(const T &info) { // TODO
					for (typename decltype(_hands)::Node *n = _hands.First(); n; n = n->Next()) {
						n->Data().Func(info);
					}
				}

				operator bool() const {
					return _hands.First() != nullptr;
				}
			private:
				struct HandlerInfo {
					HandlerInfo() = default;
					HandlerInfo(const Handler &hand, Event &event) : Func(hand), AttachedTo(&event) {
					}
					~HandlerInfo() {
						Pointer.SetSharedPointer(nullptr);
					}

					Handler Func;
					SharedPointer<typename Collections::LinkedList<HandlerInfo>::Node> Pointer;
					Event *AttachedTo = nullptr;
				};

				void RemoveHandler(typename Collections::LinkedList<HandlerInfo>::Node *node) {
#ifdef STRICT_RUNTIME_CHECK
					if (node->Data().AttachedTo != this) {
						throw InvalidArgumentException(_TEXT("the node doesn't belong to this event"));
					}
#endif
					_hands.Delete(node);
				}

				Collections::LinkedList<HandlerInfo> _hands;
        };
	}
}
