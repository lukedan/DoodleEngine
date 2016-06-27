#pragma once

#include "Control.h"
#include "SortedList.h"

namespace DE {
	namespace UI {
		enum class ChangeType {
			Add,
			Remove,
			Modify
		};
		template <typename T> struct CollectionChangeInfo {
			public:
				CollectionChangeInfo() {
				}
				CollectionChangeInfo(T i, ChangeType t) : item(i), type(t) {
				}

				T GetItem() const {
					return item;
				}
				ChangeType GetType() const {
					return type;
				}
			private:
				T item;
				ChangeType type = ChangeType::Add;
		};

		class World;
		class Control;
		class PanelBase;
		class WrapPanelBase;
		class ControlCollection {
				friend class PanelBase;
				friend class WrapPanelBase;
				friend class ScrollViewBase;
			public:
				class ControlZIndexComparer {
					public:
						static int Compare(const Control *lhs, const Control *rhs) {
							return Core::DefaultComparer<int>::Compare(lhs->_zIndex, rhs->_zIndex);
						}
				};
				typedef Core::Collections::BSTNode<Control*, ControlZIndexComparer> Node;

				void Insert(Control&);
				void Delete(Control&);
				void SetZIndex(Control&, int);

				const Node *First() const {
					return _cons.First();
				}
				const Node *Last() const {
					return _cons.Last();
				}

				size_t Count() const {
					return _cons.Count();
				}

				World *GetWorld() {
					return _world;
				}
				const World *GetWorld() const {
					return _world;
				}

				void ForEach(const std::function<bool(Control*)> &func) {
					_cons.ForEach(func);
				}
				void ForEach(const std::function<bool(const Control*)> &func) const {
					_cons.ForEach(func);
				}
				void ForEachReversed(const std::function<bool(Control*)> &func) {
					_cons.ForEachReversed(func);
				}
				void ForEachReversed(const std::function<bool(const Control*)> &func) const {
					_cons.ForEachReversed(func);
				}
			private:
				Node *First() {
					return _cons.First();
				}
				Node *Last() {
					return _cons.Last();
				}

				explicit ControlCollection(PanelBase *c) : _father(c) {
				}
				~ControlCollection() {
					for (decltype(_cons)::Node *n = _cons.First(); n; n = n->Next()) {
						n->Value()->_father = nullptr;
						n->Value()->SetWorld(nullptr);
					}
				}

				PanelBase *const _father;
				World *_world = nullptr;
                Core::Collections::SortedList<Control*, ControlZIndexComparer> _cons;

                void SetWorld(World *w) {
                	_world = w;
                	for (decltype(_cons)::Node *n = _cons.First(); n; n = n->Next()) {
                		n->Value()->SetWorld(_world);
                	}
                }
		};
	}
}
