#pragma once

#include "BinarySearchTree.h"

namespace DE {
	namespace Core {
		namespace Collections {
			template <typename T, class Comparer = DefaultComparer<T>> class SortedList : protected BST<T, Comparer> {
				public:
					typedef BSTNode<T, Comparer> Node;

					~SortedList() {
					}

					Node *InsertLeft(const T &value) {
						return Base::InsertLeft(value);
					}
					Node *InsertRight(const T &value) {
						return Base::InsertRight(value);
					}
					void Delete(Node *node) {
						Base::Delete(node);
					}
					bool Delete(const T &value) {
						Node *target = Base::Find(value);
						Base::Delete(target);
						return target;
					}
					template <class Predicate> bool Delete(const T &value) {
						Node *targetNode = Base::template Find<Predicate>(value);
						if (targetNode) {
							Base::Delete(targetNode);
							return true;
						}
						return false;
					}

					size_t IndexOf(Node *node) {
						return Base::GetIndex(node);
					}
					size_t IndexOf(Node *node) const {
						return Base::GetIndex(node);
					}

					void ForEach(const std::function<bool(T&)> &func) {
						Base::ForEach([&](Node *node) {
							return func(node->Value());
						});
					}
					void ForEach(const std::function<bool(const T&)> &func) const {
						Base::ForEach([&](const Node *node) {
							return func(node->Value());
						});
					}
					void ForEachReversed(const std::function<bool(T&)> &func) {
						Base::ForEachReversed([&](Node *node) {
							return func(node->Value());
						});
					}
					void ForEachReversed(const std::function<bool(const T&)> &func) const {
						Base::ForEachReversed([&](const Node *node) {
							return func(node->Value());
						});
					}

					Node *Find(const T &value) {
						return Base::Find(value);
					}
					const Node *Find(const T &value) const {
						return Base::Find(value);
					}
					template <class Predicate> Node *Find(const T &value) {
						return Base::template Find<Predicate>(value);
					}
					template <class Predicate> const Node *Find(const T &value) const {
						return Base::template Find<Predicate>(value);
					}

					Node *First() {
						return Base::Minimum();
					}
					const Node *First() const {
						return Base::Minimum();
					}
					Node *Last() {
						return Base::Maximum();
					}
					const Node *Last() const {
						return Base::Maximum();
					}

					Node *operator [](size_t index) {
						return Base::At(index);
					}
					const Node *operator [](size_t index) const {
						return Base::At(index);
					}

					using BST<T, Comparer>::Clear;
					size_t Count() const {
						return Base::Count();
					}
				protected:
					typedef BST<T, Comparer> Base;
			};
		}
	}
}
