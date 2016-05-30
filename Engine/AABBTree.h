#pragma once

#include "Common.h"
#include "Math.h"
#include "Rectangle.h"
#include "List.h"

namespace DE {
	namespace Core {
		namespace Collections {
			struct AABBNode {
					friend class AABBTree;
				public:
					typedef Math::Rectangle AABB;

					AABBNode() = default;
					AABBNode(const AABB &c) : Region(c) {
					}

					AABBNode *Father = nullptr, *Left = nullptr, *Right = nullptr;
					void *Tag = nullptr;
					AABB Region;
				protected:
					size_t _maxDep = 1;
			};
			class AABBTree {
				public:
					typedef Math::Rectangle AABB;

					AABBTree() = default;
					AABBTree(const AABBTree &rhs) : _root(rhs._root ? CopyTree(rhs._root) : nullptr) {
					}
					AABBTree &operator =(const AABBTree &rhs) {
						if (this == &rhs) {
							return *this;
						}
						Clear();
						if (rhs._root) {
							_root = CopyTree(rhs._root);
						}
						return *this;
					}
					~AABBTree() {
						Clear();
					}

					AABBNode *Insert(const AABB &ab) {
						AABBNode *nd = new (GlobalAllocator::Allocate(sizeof(AABBNode))) AABBNode(ab);
						Insert(nd);
						return nd;
					}
					void Insert(AABBNode *nd) {
						if (_root == nullptr) {
							_root = nd;
							_root->Father = nullptr;
							return;
						}
						AABBNode *cur = _root;
						while (cur->Left && cur->Right) {
							double lv = GetCombinedEval(nd, cur->Left), rv = GetCombinedEval(nd, cur->Right);
							cur->Region = CombineAABB(cur->Region, nd->Region);
							cur = (lv > rv ? cur->Right : cur->Left);
						}
						AABBNode *tmp = new (GlobalAllocator::Allocate(sizeof(AABBNode))) AABBNode(CombineAABB(nd->Region, cur->Region));
						tmp->Father = cur->Father;
						if (tmp->Father) {
							(cur == cur->Father->Left ? cur->Father->Left : cur->Father->Right) = tmp;
						} else {
							_root = tmp;
						}
						tmp->Left = cur;
						cur->Father = tmp;
						tmp->Right = nd;
						nd->Father = tmp;
						FixupBottomUp(tmp);
					}
					void Remove(AABBNode *abn) {
						if (abn->Father == nullptr) {
							_root = nullptr;
						} else {
							AABBNode *other = (abn == abn->Father->Left ? abn->Father->Right : abn->Father->Left);
							if (abn->Father->Father == nullptr) {
								_root = other;
								other->Father = nullptr;
							} else {
								other->Father = abn->Father->Father;
								(abn->Father == other->Father->Left ? other->Father->Left : other->Father->Right) = other;
								for (AABBNode *cur = other->Father; cur; cur = cur->Father) {
									cur->Region = CombineAABB(cur->Left->Region, cur->Right->Region);
								}
								FixupBottomUp(other->Father);
							}
							abn->Father->~AABBNode();
							GlobalAllocator::Free(abn->Father);
						}
					}
					void Delete(AABBNode *abn) {
						Remove(abn);
						abn->~AABBNode();
						GlobalAllocator::Free(abn);
					}
					void QueryRegion(const AABB &rgn, const std::function<bool(AABBNode*)> &callback) {
						List<AABBNode*> stack;
						if (_root) {
							if (AABB::Intersect(_root->Region, rgn) != Math::IntersectionType::None) {
								stack.PushBack(_root);
							}
						}
						while (stack.Count() > 0) {
							AABBNode *cur = stack.PopBack();
							if (cur->Left == nullptr) {
								if (!callback(cur)) {
									return;
								}
							} else {
								if (AABB::Intersect(cur->Left->Region, rgn) != Math::IntersectionType::None) {
									stack.PushBack(cur->Left);
								}
								if (AABB::Intersect(cur->Right->Region, rgn) != Math::IntersectionType::None) {
									stack.PushBack(cur->Right);
								}
							}
						}
					}
					void ForEach(const std::function<bool(const AABBNode*)> &callback) const {
						List<const AABBNode*> stack;
						if (_root) {
							stack.PushBack(_root);
						}
						while (stack.Count() > 0) {
							const AABBNode *cur = stack.PopBack();
							if (cur->Left == nullptr) {
								if (!callback(cur)) {
									return;
								}
							} else {
								stack.PushBack(cur->Left);
								stack.PushBack(cur->Right);
							}
						}
					}
					void MoveAABB(AABBNode *node, const AABB &newRgn) {
						Remove(node);
						node->Region = newRgn;
						Insert(node);
					}

					inline static double GetCombinedEval(const AABBNode *ln, const AABBNode *rn) {
						return GetEval(CombineAABB(ln->Region, rn->Region));
					}
					inline static double GetEval(const AABB &ab) {
						return 2.0 * (ab.Width() + ab.Height());
					}
					inline static AABB CombineAABB(const AABB &a, const AABB &b) {
						AABB result;
						result.Left = Math::Min(a.Left, b.Left);
						result.Top = Math::Min(a.Top, b.Top);
						result.Right = Math::Max(a.Right, b.Right);
						result.Bottom = Math::Max(a.Bottom, b.Bottom);
						return result;
					}

					void Clear() {
						if (_root) {
							List<AABBNode*> stack;
							stack.PushBack(_root);
							while (stack.Count() > 0) {
								AABBNode *cur = stack.PopBack();
								if (cur->Left) {
									stack.PushBack(cur->Left);
									stack.PushBack(cur->Right);
								}
								cur->~AABBNode();
								GlobalAllocator::Free(cur);
							}
							_root = nullptr;
						}
					}

					void Validate(AABBNode *nd) {
						if (nd == _root) {
							if (nd->Father != nullptr) {
								throw SystemException(_TEXT("tree structure invalid"));
							}
						}
						if (nd->Left == nullptr) {
							if (nd->Right != nullptr) {
								throw SystemException(_TEXT("tree structure invalid"));
							}
							return;
						}
						if (nd->Left->Father != nd) {
							throw SystemException(_TEXT("tree structure invalid"));
						}
						if (nd->Right->Father != nd) {
							throw SystemException(_TEXT("tree structure invalid"));
						}
						if (nd->_maxDep != Math::Max(nd->Left->_maxDep, nd->Right->_maxDep) + 1) {
							throw SystemException(_TEXT("tree structure invalid"));
						}
						Validate(nd->Left);
						Validate(nd->Right);
					}
				protected:
					AABBNode *_root = nullptr;

					inline static AABBNode *CopyTree(const AABBNode *nd) {
						AABBNode *newnd = new (GlobalAllocator::Allocate(sizeof(AABBNode))) AABBNode(*nd);
						if (nd->Left) {
							newnd->Left = CopyTree(nd->Left);
							newnd->Left->Father = newnd;
							newnd->Right = CopyTree(nd->Right);
							newnd->Right->Father = newnd;
						}
						return newnd;
					}

					void FixupBottomUp(AABBNode *nd) {
						AABBNode *cur = nd;
						if (cur->Left == nullptr) {
							cur->_maxDep = 1;
							cur = cur->Father;
						}
						for (; cur; cur = cur->Father) {
							if (cur->Left->_maxDep + 1 < cur->Right->_maxDep) {
								RotateLeft(cur);
								cur = cur->Father;
							} else if (cur->Right->_maxDep + 1 < cur->Left->_maxDep) {
								RotateRight(cur);
								cur = cur->Father;
							} else {
								cur->_maxDep = Math::Max(cur->Left->_maxDep, cur->Right->_maxDep) + 1;
							}
						}
						Validate(_root);
					}
					void RotateLeft(AABBNode *rt) {
						if (rt == nullptr || rt->Right == nullptr) {
							return;
						}
						rt->Right->Father = rt->Father;
						if (rt->Father) {
							(rt == rt->Father->Left ? rt->Father->Left : rt->Father->Right) = rt->Right;
						} else {
							_root = rt->Right;
						}
						rt->Father = rt->Right;
						rt->Right = rt->Right->Left;
						rt->Father->Left = rt;
						rt->Father->Region = rt->Region;
						if (rt->Right) {
							rt->Right->Father = rt;
							rt->Region = CombineAABB(rt->Left->Region, rt->Right->Region);
							rt->_maxDep = Math::Max(rt->Left->_maxDep, rt->Right->_maxDep) + 1;
						}
						rt->Father->_maxDep = Math::Max(rt->_maxDep, rt->Father->Right->_maxDep) + 1;
					}
					void RotateRight(AABBNode *rt) {
						if (rt == nullptr || rt->Left == nullptr) {
							return;
						}
						rt->Left->Father = rt->Father;
						if (rt->Father) {
							(rt == rt->Father->Left ? rt->Father->Left : rt->Father->Right) = rt->Left;
						} else {
							_root = rt->Left;
						}
						rt->Father = rt->Left;
						rt->Left = rt->Father->Right;
						rt->Father->Right = rt;
						rt->Father->Region = rt->Region;
						if (rt->Left) {
							rt->Left->Father = rt;
							rt->Region = CombineAABB(rt->Left->Region, rt->Right->Region);
							rt->_maxDep = Math::Max(rt->Left->_maxDep, rt->Right->_maxDep) + 1;
						}
						rt->Father->_maxDep = Math::Max(rt->_maxDep, rt->Father->Left->_maxDep) + 1;
					}
			};
		}
	}
}
