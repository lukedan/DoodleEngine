#pragma once

#include "ObjectAllocator.h"

namespace DE {
	namespace Core {
		namespace Collections {
			template <typename T> class LinkedList;
			template <typename T> class LinkedListNode {
					friend class LinkedList<T>;
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

					LinkedListNode *Previous() {
						return _prev;
					}
					const LinkedListNode *Previous() const {
						return _prev;
					}
					LinkedListNode *Next() {
						return _next;
					}
					const LinkedListNode *Next() const {
						return _next;
					}
				private:
					T _data;
					LinkedListNode *_prev, *_next;
			};
			template <typename T> class LinkedList {
				public:
					typedef LinkedListNode<T> Node;

					LinkedList() = default;
					LinkedList(const LinkedList &src) {
						CopyContentFrom(src);
					}
					LinkedList &operator =(const LinkedList &rhs) {
						if (this == &rhs) {
							return *this;
						}
						DeleteAllNodes();
						CopyContentFrom(rhs);
						return *this;
					}
					~LinkedList() {
						DeleteAllNodes();
					}

					Node *InsertFirst(const T &obj) {
						Node *n = new (GlobalAllocator::Allocate(sizeof(Node))) Node(obj);
						InsertFirst(n);
						return n;
					}
					Node *InsertLast(const T &obj) {
						Node *n = new (GlobalAllocator::Allocate(sizeof(Node))) Node(obj);
						InsertLast(n);
						return n;
					}
					Node *InsertAfter(LinkedListNode<T> *node, const T &obj) {
						Node *n = new (GlobalAllocator::Allocate(sizeof(Node))) Node(obj);
						if (node) {
							InsertAfter(node, n);
						} else {
							InsertFirst(n);
						}
						return n;
					}
					Node *InsertBefore(LinkedListNode<T> *node, const T &obj) {
						Node *n = new (GlobalAllocator::Allocate(sizeof(Node))) Node(obj);
						if (node) {
							InsertBefore(node, n);
						} else {
							InsertLast(n);
						}
						return n;
					}
					void Delete(LinkedListNode<T> *node) {
						if (node == nullptr) {
							throw InvalidArgumentException(_TEXT("removing a null node"));
						}
						(node->_next ? node->_next->_prev : _tail) = node->_prev;
						(node->_prev ? node->_prev->_next : _head) = node->_next;
						node->_prev = node->_next = nullptr;
						node->~Node();
						GlobalAllocator::Free(node);
					}

					Node *First() {
						return _head;
					}
					const Node *First() const {
						return _head;
					}
					Node *Last() {
						return _tail;
					}
					const Node *Last() const {
						return _tail;
					}

					void Clear() {
						DeleteAllNodes();
						_head = _tail = nullptr;
					}
				protected:
					Node *_head = nullptr, *_tail = nullptr;

					void InsertFirst(LinkedListNode<T> *n) {
						n->_next = _head;
						(_head ? _head->_prev : _tail) = n;
						_head = n;
					}
					void InsertLast(LinkedListNode<T> *n) {
						n->_prev = _tail;
						(_tail ? _tail->_next : _head) = n;
						_tail = n;
					}
					void InsertAfter(LinkedListNode<T> *n, LinkedListNode<T> *target) {
						(n->_next ? n->_next->_prev : _tail) = target;
						target->_next = n->_next;
						n->_next = target;
						target->_prev = n;
					}
					void InsertBefore(LinkedListNode<T> *n, LinkedListNode<T> *target) {
						(n->_prev ? n->_prev->_next : _head) = target;
						target->_prev = n->_prev;
						n->_prev = target;
						target->_next = n;
					}

					void DeleteAllNodes() {
						Node *next = _head;
						for (Node *cur = _head; cur; cur = next) {
							next = cur->_next;
							cur->~Node();
							GlobalAllocator::Free(cur);
						}
					}
					void CopyContentFrom(const LinkedList &src) {
						if (src._head) {
							_head = new (GlobalAllocator::Allocate(sizeof(Node))) Node(src._head->_data);
							Node *last = _head;
							for (Node *cur = src._head->_next; cur; cur = cur->_next) {
								Node *dup = new (GlobalAllocator::Allocate(sizeof(Node))) Node(cur->_data);
								last->_next = dup;
								dup->_prev = last;
								last = dup;
							}
							_tail = last;
						} else {
							_head = _tail = nullptr;
						}
					}
			};
		}
	}
}
