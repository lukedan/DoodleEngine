#pragma once

#include "ObjectAllocator.h"

namespace DE {
	namespace Core {
		namespace Collections {
			template <typename T> class LinkedListNode {
				public:
					LinkedListNode() : _prev(nullptr), _next(nullptr) {
					}
					explicit LinkedListNode(const T &d) : _data(d), _prev(nullptr), _next(nullptr) {
					}

					T &Data() {
						return _data;
					}
					const T &Data() const {
						return _data;
					}

					LinkedListNode<T> *&Previous() {
						return _prev;
					}
					const LinkedListNode<T> *Previous() const {
						return _prev;
					}
					LinkedListNode<T> *&Next() {
						return _next;
					}
					const LinkedListNode<T> *Next() const {
						return _next;
					}
				private:
					T _data;
					LinkedListNode<T> *_prev, *_next;
			};
			template <typename T, class Allocator = GlobalAllocator> class LinkedList {
				public:
					LinkedList() : _head(nullptr), _tail(nullptr) {
					}

					void InsertFirst(LinkedListNode<T> *n) {
						if (n == nullptr) {
							throw InvalidArgumentException(_TEXT("the node is null"));
						}
						if (n->Previous() != nullptr || n->Next() != nullptr) {
							throw InvalidOperationException(_TEXT("the node is already in another linked list"));
						}
						n->Next() = _head;
						if (_head) {
							_head->Previous() = n;
						} else {
							_tail = n;
						}
						_head = n;
					}
					void InsertFirst(const T &obj) {
						InsertFirst(new (Allocator::Allocate(sizeof(Node))) Node(obj));
					}
					void InsertLast(LinkedListNode<T> *n) {
						if (n == nullptr) {
							throw InvalidArgumentException(_TEXT("the node is null"));
						}
						if (n->Previous() != nullptr || n->Next() != nullptr) {
							throw InvalidOperationException(_TEXT("the node is already in another linked list"));
						}
						n->Previous() = _tail;
						if (_tail) {
							_tail->Next() = n;
						} else {
							_head = n;
						}
						_tail = n;
					}
					void InsertLast(const T &obj) {
						InsertLast(new (Allocator::Allocate(sizeof(Node))) Node(obj));
					}
					void InsertAfter(LinkedListNode<T> *n, LinkedListNode<T> *target) {
						if (target == nullptr) {
							throw InvalidArgumentException(_TEXT("the node to insert is null"));
						}
						if (target->Previous() != nullptr || target->Next() != nullptr) {
							throw InvalidOperationException(_TEXT("the node is already in another linked list"));
						}
						if (n == nullptr) {
							throw InvalidArgumentException(_TEXT("cannot be inserted after a null node"));
						}
						if (n->Next()) {
							n->Next()->Previous() = target;
						}
						target->Next() = n->Next();
						n->Next() = target;
						target->Previous() = n;
						if (n == _tail) {
							_tail = target;
						}
					}
					void InsertBefore(LinkedListNode<T> *n, LinkedListNode<T> *target) {
						if (target == nullptr) {
							throw InvalidArgumentException(_TEXT("the node to insert is null"));
						}
						if (target->Previous() != nullptr || target->Next() != nullptr) {
							throw InvalidOperationException(_TEXT("the node is already in another linked list"));
						}
						if (n == nullptr) {
							throw InvalidArgumentException(_TEXT("cannot be inserted after a null node"));
						}
						if (n->Previous()) {
							n->Previous()->Next() = target;
						}
						target->Previous() = n->Previous();
						n->Previous() = target;
						target->Next() = n;
						if (n == _head) {
							_head = target;
						}
					}
					void Remove(LinkedListNode<T> *node) {
						if (node == nullptr) {
							throw InvalidArgumentException(_TEXT("removing a null node"));
						}
						if (node == _head) {
							_head = node->Next();
						}
						if (node == _tail) {
							_tail = node->Previous();
						}
						if (node->Next()) {
							node->Next()->Previous() = node->Previous();
						}
						if (node->Previous()) {
							node->Previous()->Next() = node->Next();
						}
						node->Previous() = node->Next() = nullptr;
					}

					LinkedListNode<T> *First() {
						return _head;
					}
					const LinkedListNode<T> *First() const {
						return _head;
					}
					LinkedListNode<T> *Last() {
						return _tail;
					}
					const LinkedListNode<T> *Last() const {
						return _tail;
					}

					void Clear() {
						_head = _tail = nullptr;
					}
				private:
					typedef LinkedListNode<T> Node;
					Node *_head, *_tail;
			};
		}
	}
}
