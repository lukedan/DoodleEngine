#pragma once

#include "ObjectAllocator.h"
#include "Common.h"

namespace DE {
	namespace Core {
		namespace Collections {
			template <typename T, size_t ChunkSize = 100, bool DirectMemoryAccess = !IsClass<T>::Result> class Queue {
					// +---------------+ next +-+ next +-+ next +---------------+
					// |     _head     | ---> | | ---> | | ---> |     _tail     |
					// | unused | used | <--- | | <--- | | <--- | used | unused |
					// +---------------+ prev +-+ prev +-+ prev +---------------+
					// _headPtr ^                          _pastTailPtr ^
					//
					// - or -
					//
					// +------------------------+
					// |     _head == _tail     |
					// | unused | used | unused |
					// +------------------------+
					// _headPtr ^       ^ _pastTailPtr
				public:
					Queue() {
						StaticAssert(ChunkSize > 0, "chunk size must not be zero");
					}
					Queue(const Queue<T, ChunkSize, DirectMemoryAccess> &src) {
						CopyContentFrom(src);
					}
					Queue &operator =(const Queue<T, ChunkSize, DirectMemoryAccess> &src) {
						if (this == &src) {
							return *this;
						}
						Clear();
						CopyContentFrom(src);
						return *this;
					}
					~Queue() {
						Clear();
					}

					void PushHead(const T &obj) {
						T *position = nullptr;
						if (_head == nullptr) { // no objects here, checks are omitted
							_head = _tail = new (GlobalAllocator::Allocate(sizeof(Chunk))) Chunk();
							_headPtr = ChunkSize - 1;
							_pastTailPtr = ChunkSize;
							position = _head->Objects + _headPtr;
						} else if (_headPtr == 0) { // the first chunk is full, create new chunk
							Chunk *newChunk = new (GlobalAllocator::Allocate(sizeof(Chunk))) Chunk();
							_head->Previous = newChunk;
							newChunk->Next = _head;
							_headPtr = ChunkSize - 1;
							_head = newChunk;
							position = _head->Objects + _headPtr;
						} else { // the first chunk is not full
							--_headPtr;
							position = _head->Objects + _headPtr;
						}
						if (DirectMemoryAccess) {
							memcpy(position, &obj, sizeof(T));
						} else {
							new (position) T(obj);
						}
						++_count;
					}
					void PushTail(const T &obj) {
						T *position = nullptr;
						if (_tail == nullptr) { // no objects here, checks are omitted
							_head = _tail = new (GlobalAllocator::Allocate(sizeof(Chunk))) Chunk();
							_headPtr = ChunkSize - 1;
							_pastTailPtr = ChunkSize;
							position = _tail->Objects + _headPtr;
						} else if (_pastTailPtr == ChunkSize) { // the last chunk is full, create new chunk
							Chunk *newChunk = new (GlobalAllocator::Allocate(sizeof(Chunk))) Chunk();
							_tail->Next = newChunk;
							newChunk->Previous = _tail;
							_pastTailPtr = 1;
							_tail = newChunk;
							position = _tail->Objects;
						} else { // the last chunk is not full
							position = _tail->Objects + _pastTailPtr;
							++_pastTailPtr;
						}
						if (DirectMemoryAccess) {
							memcpy(position, &obj, sizeof(T));
						} else {
							new (position) T(obj);
						}
						++_count;
					}
					T PopHead() {
						if (_count == 0) { // no objects here, checks are omitted
							throw InvalidOperationException(_TEXT("the queue is empty"));
						}
						T obj = *(_head->Objects + _headPtr);
						if (!DirectMemoryAccess) {
							(_head->Objects + _headPtr)->~T();
						}
						if ((++_headPtr) == ChunkSize) { // the chunk is empty and should be deleted
							Chunk *next = _head->Next;
							_headPtr = 0;
							_head->~Chunk();
							GlobalAllocator::Free(_head);
							if (next != nullptr) {
								next->Previous = nullptr;
								_head = next;
							} else { // checks are omitted
								_head = _tail = nullptr;
							}
						}
						--_count;
						return obj;
					}
					T PopTail() {
						if (_count == 0) { // no objects here, checks are omitted
							throw InvalidOperationException(_TEXT("the queue is empty"));
						}
						T obj = *(_tail->Objects + (--_pastTailPtr));
						if (!DirectMemoryAccess) {
							(_tail->Objects + _pastTailPtr)->~T();
						}
						if (_pastTailPtr == 0) { // the chunk is empty and should be deleted
							Chunk *prev = _tail->Previous;
							_pastTailPtr = ChunkSize;
							_tail->~Chunk();
							GlobalAllocator::Free(_tail);
							if (prev != nullptr) {
								prev->Next = nullptr;
								_tail = prev;
							} else { // checks are omitted
								_head = _tail = nullptr;
							}
						}
						--_count;
						return obj;
					}

					T &PeekHead() {
						if (_count == 0) {
							throw InvalidOperationException(_TEXT("the queue is empty"));
						}
						return *(_head->Objects + _headPtr);
					}
					const T &PeekHead() const {
						if (_count == 0) {
							throw InvalidOperationException(_TEXT("the queue is empty"));
						}
						return *(_head->Objects + _headPtr);
					}
					T &PeekTail() {
						if (_count == 0) {
							throw InvalidOperationException(_TEXT("the queue is empty"));
						}
						return *(_tail->Objects + (_pastTailPtr - 1));
					}
					const T &PeekTail() const {
						if (_count == 0) {
							throw InvalidOperationException(_TEXT("the queue is empty"));
						}
						return *(_tail->Objects + (_pastTailPtr - 1));
					}

					void Clear() {
						if (_count == 0) {
							if (_head != nullptr) {
								_head->~Chunk();
								GlobalAllocator::Free(_head);
								_head = _tail = nullptr;
							}
							return;
						}
						_count = 0;
						if (_head == _tail) {
							if (!DirectMemoryAccess) {
								while (_pastTailPtr > _headPtr) {
									(_tail->Objects + (--_pastTailPtr))->~T();
								}
							}
							_tail->~Chunk();
							GlobalAllocator::Free(_tail);
							_head = _tail = nullptr;
							return;
						}
						if (!DirectMemoryAccess) {
							while (_pastTailPtr > 0) {
								(_tail->Objects + (--_pastTailPtr))->~T();
							}
						}
						Chunk *prev = _tail->Previous;
						_tail->~Chunk();
						GlobalAllocator::Free(_tail);
						for (Chunk *ck = prev; ck != _head; ck = prev) {
							prev = ck->Previous;
							if (!DirectMemoryAccess) {
								for (_pastTailPtr = ChunkSize; _pastTailPtr > 0; ) {
									(ck->Objects + (--_pastTailPtr))->~T();
								}
							}
							ck->~Chunk();
							GlobalAllocator::Free(ck);
						}
						if (!DirectMemoryAccess) {
							for (; _headPtr < ChunkSize; ++_headPtr) {
								(_head->Objects + _headPtr)->~T();
							}
						}
						_head->~Chunk();
						GlobalAllocator::Free(_head);
						_head = _tail = nullptr;
					}

					void ForEachHeadToTail(const std::function<bool(T&)> &op) {
						if (_head == nullptr) {
							return;
						}
						if (_head == _tail) {
							T *end = _head->Objects + _pastTailPtr;
							for (T *cur = _head->Objects + _headPtr; cur != end; ++cur) {
								if (!op(*cur)) {
									return;
								}
							}
							return;
						}
						T *end = _head->Objects + ChunkSize;
						for (T *cur = _head->Objects + _headPtr; cur != end; ++cur) {
							if (!op(*cur)) {
								return;
							}
						}
						for (Chunk *curC = _head->Next; curC != _tail; curC = curC->Next) {
							end = curC->Objects + ChunkSize;
							for (T *cur = curC->Objects; cur != end; ++cur) {
								if (!op(*cur)) {
									return;
								}
							}
						}
						end = _tail->Objects + _pastTailPtr;
						for (T *cur = _tail->Objects; cur != end; ++cur) {
							if (!op(*cur)) {
								return;
							}
						}
					}
					void ForEachHeadToTail(const std::function<bool(const T&)> &op) const {
						if (_head == nullptr) {
							return;
						}
						if (_head == _tail) {
							const T *end = _head->Objects + _pastTailPtr;
							for (const T *cur = _head->Objects + _headPtr; cur != end; ++cur) {
								if (!op(*cur)) {
									return;
								}
							}
							return;
						}
						const T *end = _head->Objects + ChunkSize;
						for (const T *cur = _head->Objects + _headPtr; cur != end; ++cur) {
							if (!op(*cur)) {
								return;
							}
						}
						for (Chunk *curC = _head->Next; curC != _tail; curC = curC->Next) {
							end = curC->Objects + ChunkSize;
							for (const T *cur = curC->Objects; cur != end; ++cur) {
								if (!op(*cur)) {
									return;
								}
							}
						}
						end = _tail->Objects + _pastTailPtr;
						for (const T *cur = _tail->Objects; cur != end; ++cur) {
							if (!op(*cur)) {
								return;
							}
						}
					}
					void ForEachTailToHead(const std::function<bool(T&)> &op) {
						if (_tail == nullptr) {
							return;
						}
						if (_head == _tail) {
							T *end = _tail->Objects + _headPtr;
							for (T *cur = _tail->Objects + _pastTailPtr; cur != end; ) {
								if (!op(*(--cur))) {
									return;
								}
							}
							return;
						}
						T *end = _tail->Objects;
						for (T *cur = _tail->Objects + _pastTailPtr; cur != end; ) {
							if (!op(*(--cur))) {
								return;
							}
						}
						for (Chunk *curC = _tail->Previous; curC != _head; curC = curC->Previous) {
							end = curC->Objects;
							for (T *cur = curC->Objects + ChunkSize; cur != end; ) {
								if (!op(*(--cur))) {
									return;
								}
							}
						}
						end = _head->Objects + _headPtr;
						for (T *cur = _head->Objects + ChunkSize; cur != end; ) {
							if (!op(*(--cur))) {
								return;
							}
						}
					}
					void ForEachTailToHead(const std::function<bool(const T&)> &op) const {
						if (_tail == nullptr) {
							return;
						}
						if (_head == _tail) {
							const T *end = _tail->Objects + _headPtr;
							for (const T *cur = _tail->Objects + _pastTailPtr; cur != end; ) {
								if (!op(*(--cur))) {
									return;
								}
							}
							return;
						}
						const T *end = _tail->Objects;
						for (const T *cur = _tail->Objects + _pastTailPtr; cur != end; ) {
							if (!op(*(--cur))) {
								return;
							}
						}
						for (Chunk *curC = _tail->Previous; curC != _head; curC = curC->Previous) {
							end = curC->Objects;
							for (const T *cur = curC->Objects + ChunkSize; cur != end; ) {
								if (!op(*(--cur))) {
									return;
								}
							}
						}
						end = _head->Objects + _headPtr;
						for (const T *cur = _head->Objects + ChunkSize; cur != end; ) {
							if (!op(*(--cur))) {
								return;
							}
						}
					}

					size_t Count() const {
						return _count;
					}
					bool Empty() const {
						return _count == 0;
					}
				protected:
					struct Chunk {
						Chunk() : Objects((T*)GlobalAllocator::Allocate(sizeof(T) * ChunkSize)) {
						}
						~Chunk() {
							GlobalAllocator::Free(Objects);
						}

						T *Objects;
						Chunk *Next = nullptr, *Previous = nullptr;
					};

					void CopyContentFrom(const Queue<T, ChunkSize, DirectMemoryAccess> &src) {
						if (src._count == 0) {
							return;
						}
						_count = src._count;
						if (src._head == src._tail) {
							_head = _tail = new (GlobalAllocator::Allocate(sizeof(Chunk))) Chunk();
							if (DirectMemoryAccess) {
								memcpy(_head->Objects, src._head->Objects, sizeof(T) * ChunkSize);
								_headPtr = src._headPtr;
								_pastTailPtr = src._pastTailPtr;
							} else {
								for (_headPtr = _pastTailPtr = src._pastTailPtr; _headPtr > src._headPtr; ) {
									--_headPtr;
									new (_head->Objects + _headPtr) T(*(src._head->Objects + _headPtr));
								}
							}
							return;
						}
						_tail = new (GlobalAllocator::Allocate(sizeof(Chunk))) Chunk();
						if (DirectMemoryAccess) {
							memcpy(_tail->Objects, src._tail->Objects, sizeof(T) * ChunkSize);
							_headPtr = src._headPtr;
							_pastTailPtr = src._pastTailPtr;
						} else {
							for (_pastTailPtr = 0; _pastTailPtr < src._pastTailPtr; ++_pastTailPtr) {
								new (_tail->Objects + _pastTailPtr) T(*(src._tail->Objects + _pastTailPtr));
							}
						}
						Chunk *cur = _tail;
						for (Chunk *srcCur = src._tail->Previous; srcCur; srcCur = srcCur->Previous) {
							cur->Previous = new (GlobalAllocator::Allocate(sizeof(Chunk))) Chunk();
							cur->Previous->Next = cur;
							cur = cur->Previous;
							if (DirectMemoryAccess) {
								memcpy(cur->Objects, srcCur->Objects, sizeof(T) * ChunkSize);
							} else {
								if (srcCur == src._head) {
									for (_headPtr = ChunkSize; _headPtr > src._headPtr; ) {
										--_headPtr;
										new (cur->Objects + _headPtr) T(*(srcCur->Objects + _headPtr));
									}
								} else {
									for (_headPtr = 0; _headPtr < ChunkSize; ++_headPtr) {
										new (cur->Objects + _headPtr) T(*(srcCur->Objects + _headPtr));
									}
								}
							}
						}
						_head = cur;
					}

					Chunk *_head = nullptr, *_tail = nullptr;
					size_t _headPtr = 0, _pastTailPtr = 0, _count = 0;
			};
		}
	}
}
