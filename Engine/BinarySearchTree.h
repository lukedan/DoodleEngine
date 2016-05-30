#pragma once

#include <tchar.h>
#include <functional>

#include "Common.h"
#include "Math.h"
#include "ObjectAllocator.h"
#include "List.h"

namespace DE {
	namespace Core {
		namespace Collections {
			template <typename KeyType, typename ValueType> struct KeyValuePair;
			template <typename KeyType, typename ValueType, class Comparer = DefaultComparer<KeyType>> class KeyValuePairComparer {
				public:
					static int Compare(const KeyValuePair<KeyType, ValueType> &lhs, const KeyValuePair<KeyType, ValueType> &rhs) {
						return Comparer::Compare(lhs.Key(), rhs.Key());
					}
			};
			template <typename KeyType, typename ValueType> struct KeyValuePair {
				public:
					KeyValuePair() = default;
					explicit KeyValuePair(const KeyType &key) : _key(key) {
					}
					KeyValuePair(const KeyType &key, const ValueType &value) : _key(key), _value(value) {
					}

					KeyType &Key() {
						return _key;
					}
					const KeyType &Key() const {
						return _key;
					}
					ValueType &Value() {
						return _value;
					}
					const ValueType &Value() const {
						return _value;
					}
				private:
					KeyType _key;
					ValueType _value;
			};

			template <typename T, class Comparer> class BST;
			template <typename T, class Comparer = DefaultComparer<T>> class BSTNode {
					friend class BST<T, Comparer>;
				public:
					BSTNode(const BSTNode&) = delete;
					BSTNode &operator =(const BSTNode&) = delete;

					const T &Value() const {
						return _val;
					}
					T &Value() {
						return _val;
					}

					BSTNode<T, Comparer> *Next() {
						if (_tree == nullptr) {
							throw InvalidOperationException(_TEXT("this node is not in a tree"));
						}
						return _tree->Next(this);
					}
					BSTNode<T, Comparer> *Previous() {
						if (_tree == nullptr) {
							throw InvalidOperationException(_TEXT("this node is not in a tree"));
						}
						return _tree->Previous(this);
					}
					const BSTNode<T, Comparer> *Next() const {
						if (_tree == nullptr) {
							throw InvalidOperationException(_TEXT("this node is not in a tree"));
						}
						return _tree->Next(this);
					}
					const BSTNode<T, Comparer> *Previous() const {
						if (_tree == nullptr) {
							throw InvalidOperationException(_TEXT("this node is not in a tree"));
						}
						return _tree->Previous(this);
					}
				private:
					explicit BSTNode(BST<T, Comparer> *t) : _tree(t) {
					}
					BSTNode(BST<T, Comparer> *t, const T &val) : _val(val), _tree(t) {
					}

					T _val;
					BSTNode<T, Comparer> *_left = nullptr, *_right = nullptr, *_father = nullptr;
					size_t _treeSize = 1;
					BST<T, Comparer> *const _tree;

					/*
					 *      X      LeftRotate     Y
					 *     / \   <------------   / \
					 *    Y   3  ------------>  1   X
					 *   / \      RightRotate      / \
					 *  1   2                     2   3
					 */
					void LeftRotate() {
						if (!_right) {
							return;
						}
						_right->_father = _father;
						if (_father) {
							(this == _father->_left ? _father->_left : _father->_right) = _right;
						}
						_father = _right;
						_right = _right->_left;
						_father->_left = this;
						if (_right) {
							_right->_father = this;
						}
						_father->_treeSize = _treeSize;
						_treeSize -= 1 + (_father->_right ? _father->_right->_treeSize : 0);
					}
					void RightRotate() {
						if (!_left) {
							return;
						}
						_left->_father = _father;
						if (_father) {
							(this == _father->_left ? _father->_left : _father->_right) = _left;
						}
						_father = _left;
						_left = _left->_right;
						_father->_right = this;
						if (_left) {
							_left->_father = this;
						}
						_father->_treeSize = _treeSize;
						_treeSize -= 1 + (_father->_left ? _father->_left->_treeSize : 0);
					}

					static void DisposeTree(BSTNode<T, Comparer> *node) {
						if (node) {
							List<BSTNode<T, Comparer>*> stk;
							stk.PushBack(node);
							while (stk.Count() > 0) {
								node = stk.PopBack();
								if (node->_left) {
									stk.PushBack(node->_left);
								}
								if (node->_right) {
									stk.PushBack(node->_right);
								}
								node->~BSTNode<T, Comparer>();
								GlobalAllocator::Free(node);
							}
						}
					}
					static void Dispose(BSTNode<T, Comparer> *node) {
						node->~BSTNode<T, Comparer>();
						GlobalAllocator::Free(node);
					}

					static BSTNode<T, Comparer> *Copy(BST<T, Comparer> *tree, const BSTNode<T, Comparer> *node, BSTNode<T, Comparer> *father = nullptr) {
						if (!node) {
							return nullptr;
						}
						BSTNode<T, Comparer> *result =
							new (GlobalAllocator::Allocate(sizeof(BSTNode<T, Comparer>))) BSTNode<T, Comparer>(tree, node->_val);
						result->_left = Copy(tree, node->_left, result);
						result->_right = Copy(tree, node->_right, result);
						result->_treeSize = node->_treeSize;
						result->_father = father;
						return result;
					}
			};
			template <typename T, class Comparer = DefaultComparer<T>> class BST { // actually splay tree
				public:
					typedef BSTNode<T, Comparer> Node;

					BST() = default;
					BST(const BST &src) : _root(Node::Copy(this, src._root)) {
					}
					BST &operator =(const BST &src) {
						if (this == &src) {
							return *this;
						}
						if (_root) {
							Node::DisposeTree(_root);
						}
						_root = Node::Copy(this, src._root);
						return *this;
					}
					virtual ~BST() {
						Node::DisposeTree(_root);
					}

					Node *Maximum() {
						Node *n = Maximum(_root);
						Splay(n);
						return n;
					}
					const Node *Maximum() const {
						return Maximum(_root);
					}
					Node *Minimum() {
						Node *n = Minimum(_root);
						Splay(n);
						return n;
					}
					const Node *Minimum() const {
						return Minimum(_root);
					}

					const Node *Next(const Node *n) const {
						if (n == nullptr) {
							throw InvalidArgumentException(_TEXT("the node is null"));
						}
						if (n->_right) {
							for (n = n->_right; n->_left; n = n->_left) {
							}
							return n;
						}
						Node *last = n->_father;
						for (; last && n == last->_right; n = last, last = last->_father) {
						}
						return last;
					}
					Node *Next(Node *n) {
						if (n == nullptr) {
							throw InvalidArgumentException(_TEXT("the node is null"));
						}
						if (n->_right) {
							for (n = n->_right; n->_left; n = n->_left) {
							}
							Splay(n);
							return n;
						}
						Node *last = n->_father;
						for (; last && n == last->_right; n = last, last = last->_father) {
						}
						Splay(last);
						return last;
					}
					const Node *Previous(const Node *n) const {
						if (n == nullptr) {
							throw InvalidArgumentException(_TEXT("the node is null"));
						}
						if (n->_left) {
							for (n = n->_left; n->_right; n = n->_right) {
							}
							return n;
						}
						Node *last = n->_father;
						for (; last && n == last->_left; n = last, last = last->_father) {
						}
						return last;
					}
					Node *Previous(Node *n) {
						if (n == nullptr) {
							throw InvalidArgumentException(_TEXT("the node is null"));
						}
						if (n->_left) {
							for (n = n->_left; n->_right; n = n->_right) {
							}
							Splay(n);
							return n;
						}
						Node *last = n->_father;
						for (; last && n == last->_left; n = last, last = last->_father) {
						}
						Splay(last);
						return last;
					}

					Node *InsertRight(const T &value) {
						Node *n = new (GlobalAllocator::Allocate(sizeof(Node))) Node(this, value);
						if (_root == nullptr) {
							_root = n;
							return n;
						}
						Node **t = nullptr;
						for (Node *cur = _root; cur; cur = *t) {
							n->_father = cur;
							++(cur->_treeSize);
							t = (Comparer::Compare(n->_val, cur->_val) < 0 ? &(cur->_left) : &(cur->_right));
						}
						(*t) = n;
						Splay(n);
						return n;
					}
					Node *InsertLeft(const T &value) {
						Node *n = new (GlobalAllocator::Allocate(sizeof(Node))) Node(this, value);
						if (_root == nullptr) {
							_root = n;
							return n;
						}
						Node **t = nullptr;
						for (Node *cur = _root; cur; cur = *t) {
							n->_father = cur;
							++(cur->_treeSize);
							t = (Comparer::Compare(n->_val, cur->_val) <= 0 ? &(cur->_left) : &(cur->_right));
						}
						(*t) = n;
						Splay(n);
						return n;
					}
					void Delete(Node *n) {
						if (n == nullptr) {
							throw InvalidOperationException(_TEXT("the node to delete is null"));
						}
						Splay(n);
						if (n->_left) {
							Node *lMax = Maximum(n->_left);
							Splay(lMax, n);
							lMax->_right = n->_right;
							if (lMax->_right) {
								lMax->_treeSize += lMax->_right->_treeSize;
								lMax->_right->_father = lMax;
							}
							_root = lMax;
							lMax->_father = nullptr;
						} else {
							_root = n->_right;
							if (_root) {
								_root->_father = nullptr;
							}
						}
						Node::Dispose(n);
					}

					Node *Find(const T &targetVal) {
						for (Node *n = _root; n; n = (Comparer::Compare(n->_val, targetVal) > 0 ? n->_left : n->_right)) {
							if (Comparer::Compare(n->_val, targetVal) == 0) {
								Splay(n);
								return n;
							}
						}
						return nullptr;
					}
					const Node *Find(const T &targetVal) const {
						int lval;
						for (const Node *n = _root; n; n = (lval > 0 ? n->_left : n->_right)) {
							lval = Comparer::Compare(n->_val, targetVal);
							if (lval == 0) {
								return n;
							}
						}
						return nullptr;
					}
					template <class Predicate> Node *Find(const T &targetVal) {
						int lval;
						for (Node *n = _root; n; n = (lval > 0 ? n->_left : n->_right)) {
							lval = Comparer::Compare(n->_val, targetVal);
							if (lval == 0) {
								for (; n->_left && Comparer::Compare(n->_left->_val, targetVal) == 0; n = n->_left) {
								}
								for (Node *cur = n; cur && Comparer::Compare(cur->_val, targetVal) == 0; cur = Next(cur)) {
                                    if (Predicate::Examine(targetVal, cur->_val)) {
										return cur;
                                    }
								}
								return nullptr;
							}
						}
						return nullptr;
					}
					template <class Predicate> const Node *Find(const T &targetVal) const {
						int lval;
						for (const Node *n = _root; n; n = (lval > 0 ? n->_left : n->_right)) {
							lval = Comparer::Compare(n->_val, targetVal);
							if (lval == 0) {
								for (; n->_left && Comparer::Compare(n->_left->_val, targetVal) == 0; n = n->_left) {
								}
								for (Node *cur = n; cur && Comparer::Compare(cur->_val, targetVal) == 0; cur = Next(cur)) {
                                    if (Predicate::Examine(targetVal, cur->_val)) {
										return cur;
                                    }
								}
								return nullptr;
							}
						}
						return nullptr;
					}

				protected:
					struct IterationStackRecord {
						IterationStackRecord() = default;
						explicit IterationStackRecord(Node *n) : CurrentNode(n) {
						}

						Node *CurrentNode = nullptr;
						bool Visited = false, LVisited = false, RVisited = false;
					};

				public:
					void ForEach(const std::function<bool(Node*)> &func) { // TODO: repeated code
						List<IterationStackRecord> stack;
						if (_root == nullptr) {
							return;
						}
						stack.PushBack(IterationStackRecord(_root));
						while (stack.Count() > 0) {
							IterationStackRecord &current = stack.Last(), *up = (stack.Count() > 1 ? &stack[stack.Count() - 2] : nullptr);
							if (current.Visited) {
								if (current.CurrentNode->_right == nullptr || current.RVisited) {
									stack.PopBack();
								} else {
									stack.PushBack(IterationStackRecord(current.CurrentNode->_right));
								}
							} else {
								if (current.CurrentNode->_left == nullptr || current.LVisited) {
									if (!func(current.CurrentNode)) {
										break;
									}
									current.Visited = true;
									if (up) {
										(current.CurrentNode == up->CurrentNode->_right ? up->RVisited : up->LVisited) = true;
									}
									if (current.CurrentNode->_right) {
										stack.PushBack(IterationStackRecord(current.CurrentNode->_right));
									}
								} else {
									stack.PushBack(IterationStackRecord(current.CurrentNode->_left));
								}
							}
						}
					}
					void ForEach(const std::function<bool(const Node*)> &func) const {
						List<IterationStackRecord> stack;
						if (_root == nullptr) {
							return;
						}
						stack.PushBack(IterationStackRecord(_root));
						while (stack.Count() > 0) {
							IterationStackRecord &current = stack.Last(), *up = (stack.Count() > 1 ? &stack[stack.Count() - 2] : nullptr);
							if (current.Visited) {
								if (current.CurrentNode->_right == nullptr || current.RVisited) {
									stack.PopBack();
								} else {
									stack.PushBack(IterationStackRecord(current.CurrentNode->_right));
								}
							} else {
								if (current.CurrentNode->_left == nullptr || current.LVisited) {
									if (!func(current.CurrentNode)) {
										break;
									}
									current.Visited = true;
									if (up) {
										(current.CurrentNode == up->CurrentNode->_right ? up->RVisited : up->LVisited) = true;
									}
									if (current.CurrentNode->_right) {
										stack.PushBack(IterationStackRecord(current.CurrentNode->_right));
									}
								} else {
									stack.PushBack(IterationStackRecord(current.CurrentNode->_left));
								}
							}
						}
					}
					void ForEachReversed(const std::function<bool(Node*)> &func) {
						List<IterationStackRecord> stack;
						if (_root == nullptr) {
							return;
						}
						stack.PushBack(IterationStackRecord(_root));
						while (stack.Count() > 0) {
							IterationStackRecord &current = stack.Last(), *up = (stack.Count() > 1 ? &stack[stack.Count() - 2] : nullptr);
							if (current.Visited) {
								if (current.CurrentNode->_left == nullptr || current.LVisited) {
									stack.PopBack();
								} else {
									stack.PushBack(IterationStackRecord(current.CurrentNode->_left));
								}
							} else {
								if (current.CurrentNode->_right == nullptr || current.RVisited) {
									if (!func(current.CurrentNode)) {
										break;
									}
									current.Visited = true;
									if (up) {
										(current.CurrentNode == up->CurrentNode->_right ? up->RVisited : up->LVisited) = true;
									}
									if (current.CurrentNode->_left) {
										stack.PushBack(IterationStackRecord(current.CurrentNode->_left));
									}
								} else {
									stack.PushBack(IterationStackRecord(current.CurrentNode->_right));
								}
							}
						}
					}
					void ForEachReversed(const std::function<bool(const Node*)> &func) const {
						List<IterationStackRecord> stack;
						if (_root == nullptr) {
							return;
						}
						stack.PushBack(IterationStackRecord(_root));
						while (stack.Count() > 0) {
							IterationStackRecord &current = stack.Last(), *up = (stack.Count() > 1 ? &stack[stack.Count() - 2] : nullptr);
							if (current.Visited) {
								if (current.CurrentNode->_left == nullptr || current.LVisited) {
									stack.PopBack();
								} else {
									stack.PushBack(IterationStackRecord(current.CurrentNode->_left));
								}
							} else {
								if (current.CurrentNode->_right == nullptr || current.RVisited) {
									if (!func(current.CurrentNode)) {
										break;
									}
									current.Visited = true;
									if (up) {
										(current.CurrentNode == up->CurrentNode->_right ? up->RVisited : up->LVisited) = true;
									}
									if (current.CurrentNode->_left) {
										stack.PushBack(IterationStackRecord(current.CurrentNode->_left));
									}
								} else {
									stack.PushBack(IterationStackRecord(current.CurrentNode->_right));
								}
							}
						}
					}

					Node *At(size_t index) {
						for (Node *cur = _root; cur; ) {
							size_t lsz = (cur->_left ? cur->_left->_treeSize : 0);
							if (index == lsz) {
								Splay(cur);
								return cur;
							}
							cur = (index > lsz ? cur->_right : cur->_left);
							if (index > lsz) {
								index -= lsz + 1;
							}
						}
						return nullptr;
					}
					const Node *At(size_t index) const {
						for (const Node *cur = _root; cur; ) {
							size_t lsz = (cur->_left ? cur->_left->_treeSize : 0);
							if (index == lsz) {
								return cur;
							}
							cur = (index > lsz ? cur->_right : cur->_left);
							if (index > lsz) {
								index -= lsz + 1;
							}
						}
						return nullptr;
					}
					size_t GetIndex(Node *n) {
						Splay(n);
						return (n->_left ? n->_left->_treeSize : 0);
					}
					size_t GetIndex(const Node *n) const {
						if (n == nullptr) {
							throw InvalidArgumentException(_TEXT("the node is null"));
						}
						size_t res = (n->_left ? n->_left->_treeSize : 0);
						for (const Node *cur = n->_father, *lst = n; cur; lst = cur, cur = cur->_father) {
							if (lst == cur->_right) {
								res += 1 + (cur->_left ? cur->_left->_treeSize : 0);
							}
						}
						return res;
					}

					void Clear() {
						Node::DisposeTree(_root);
						_root = nullptr;
					}
					size_t Count() const {
						return (_root ? _root->_treeSize : 0);
					}
				private:
					Node *_root = nullptr;

					void Splay(Node *target, Node *targetFather = nullptr) {
						if (!target) {
							return;
						}
						while (target->_father && target->_father != targetFather) {
							bool fl = (target == target->_father->_left);
							if (target->_father->_father) {
								bool ffl = (target->_father == target->_father->_father->_left);
								if (fl == ffl) {
									if (fl) {
										target->_father->_father->RightRotate();
										target->_father->RightRotate();
									} else {
										target->_father->_father->LeftRotate();
										target->_father->LeftRotate();
									}
								} else {
									if (fl) {
										target->_father->RightRotate();
										target->_father->LeftRotate();
									} else {
										target->_father->LeftRotate();
										target->_father->RightRotate();
									}
								}
							} else {
								if (fl) {
									target->_father->RightRotate();
								} else {
									target->_father->LeftRotate();
								}
							}
						}
						_root = target;
					}

					const Node *Maximum(const Node *targetRoot) const {
						for (; targetRoot && targetRoot->_right; targetRoot = targetRoot->_right) {
						}
						return targetRoot;
					}
					Node *Maximum(Node *targetRoot) {
						for (; targetRoot && targetRoot->_right; targetRoot = targetRoot->_right) {
						}
						return targetRoot;
					}
					const Node *Minimum(const Node *targetRoot) const {
						for (; targetRoot && targetRoot->_left; targetRoot = targetRoot->_left) {
						}
						return targetRoot;
					}
					Node *Minimum(Node *targetRoot) {
						for (; targetRoot && targetRoot->_left; targetRoot = targetRoot->_left) {
						}
						return targetRoot;
					}
			};
		}
	}
}
