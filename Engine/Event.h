#pragma once

#include <functional>

#include "List.h"
#include "LinkedList.h"
#include "ObjectAllocator.h"

namespace DE {
	namespace Core {
		// TODO: ugly code to remember memory address. however, that seems to be the only way
        template <typename T> class Event {
        	public:
        		typedef std::function<void(const T&)> Handler;

        		Event() : _hands() {
        		}

				void AddHandler(const Handler &hand) {
					_hands.PushBack(_FunctionRec(hand));
				}
        		Event<T> &operator +=(const Handler &hand) {
					AddHandler(hand);
					return *this;
				}
				bool RemoveHandler(const Handler &hand) {
					size_t targ = 0;
					_hands.ForEach(
						[&targ, &hand](const _FunctionRec &chand) {
							if (chand.Pointer == &hand) {
								return false;
							}
							++targ;
							return true;
						}
					);
					if (targ == _hands.Count()) {
						return false;
					}
					_hands.SwapToBack(targ);
					_hands.PopBack();
					return true;
				}
				Event<T> &operator -=(const Handler &hand) {
					if (!RemoveHandler(hand)) {
						throw InvalidArgumentException(_TEXT("the handler is not attached to this event"));
					}
					return *this;
				}

				void operator ()(const T &info) {
					for (size_t i = 0; i < _hands.Count(); ++i) {
                        _hands[i].Function(info);
					}
				}

				size_t Count() const {
					return _hands.Count();
				}
				operator bool() const {
					return _hands.Count() > 0;
				}
			private:
				struct _FunctionRec {
					explicit _FunctionRec(const Handler &hand) : Function(hand), Pointer(&hand) {
					}

					Handler Function;
					const Handler *Pointer;
				};

				Collections::List<_FunctionRec> _hands;
        };
	}
}
