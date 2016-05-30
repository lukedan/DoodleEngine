#pragma once

#include "BinarySearchTree.h"

namespace DE {
	namespace Core {
		namespace Collections {
			template <typename KeyType, typename ValueType, class Comparer = KeyValuePairComparer<KeyType, ValueType>> class Dictionary
				: protected BST<KeyValuePair<KeyType, ValueType>, Comparer>
			{
				public:
					typedef KeyValuePair<KeyType, ValueType> Pair;
					typedef BSTNode<Pair, Comparer> Node;

					virtual ~Dictionary() {
					}

					const Node *GetFirstPair() const {
						return Base::Minimum();
					}
					const Node *GetLastPair() const {
						return Base::Maximum();
					}

					void ForEachPair(const std::function<bool(const KeyValuePair<KeyType, ValueType> &pair)> &func) const {
						Base::ForEach([&](const Node *n) {
							return func(n->Value());
						});
					}
					void ForEachPairReversed(const std::function<bool(const KeyValuePair<KeyType, ValueType> &pair)> &func) const {
						Base::ForEachReversed([&](const Node *n) {
							return func(n->Value());
						});
					}

					ValueType &operator [](const KeyType &key) {
						typename Base::Node *n = Base::Find(Pair(key)); // TODO eliminate the construction of ValueType
						if (n) {
							return n->Value().Value();
						}
						return Base::InsertLeft(Pair(key, ValueType()))->Value().Value();
					}
					const ValueType &operator [](const KeyType &key) const {
						return GetValue(key);
					}

					const ValueType &GetValue(const KeyType &key) const {
						const Node *n = Base::Find(Pair(key));
						if (n == nullptr) {
							throw InvalidOperationException(_TEXT("the value does not exist"));
						}
						return n->Value().Value();
					}
					ValueType &GetValue(const KeyType &key) {
						Node *n = Base::Find(key);
						if (n == nullptr) {
							throw InvalidOperationException(_TEXT("the value does not exist"));
						}
						return n->Value().Value();
					}
					void SetValue(const KeyType &key, const ValueType &value) {
						Node *n = Base::Find(Pair(key));
						if (n) {
							n->Value().Value() = value;
						} else {
							Base::InsertLeft(Pair(key, value));
						}
					}
					void DeleteValue(const KeyType &key) {
						typename Base::Node *n = Base::Find(Pair(key));
						if (n == nullptr) {
							throw InvalidOperationException(_TEXT("the value does not exist"));
						}
						Base::Delete(n);
					}
					bool ContainsKey(const KeyType &key) const {
						return Base::Find(Pair(key)) != nullptr;
					}

					size_t PairCount() const {
						return Base::Count();
					}

					using BST<Pair, Comparer>::Clear;
				private:
					typedef BST<Pair, Comparer> Base;
			};
		}
	}
}
