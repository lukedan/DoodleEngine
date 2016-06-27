#pragma once

#include "Control.h"
#include "ControlCollection.h"

namespace DE {
	namespace UI {
		class ControlCollection;
		class PanelBase : public Control {
				friend class World;
				friend class ControlCollection;
				friend void Control::ResetLayout();
				friend Control::~Control();
            public:
				PanelBase() : Control(), _col(this) {
				}
				~PanelBase() {
					_disposing = true;
				}

				bool OverrideChildrenLayout() const {
				    return _overrideChildrenLayout;
				}

				virtual Core::Input::Cursor GetCursor() const override {
					Core::Math::Vector2 pos = _relPos + topLeft;
					for (const decltype(_col)::Node *n = _col.Last(); n != nullptr; n = n->Previous()) {
						if (n->Value()->HitTest(pos)) {
							return n->Value()->GetCursor();
						}
					}
					return Control::GetCursor();
				}

				virtual void SetVisibility(Visibility vis) override {
					if (vis == Visibility::Ignored && IsAnyChildFocused()) {
						GetWorld()->SetFocus(nullptr);
					}
					Control::SetVisibility(vis);
				}
			protected:
				virtual void Update(double delta) override {
					_col.ForEach([&](Control *c) {
						c->Update(delta);
						return true;
					});
				}
				virtual void Render(Graphics::Renderer &r) override {
					Control::Render(r);
					_col.ForEach([&](Control *c) {
						if (c->_vis == Visibility::Visible || c->_vis == Visibility::Ghost) {
							c->BeginRendering(r);
							c->Render(r);
							c->EndRendering(r);
						}
						return true;
					});
				}
				virtual void FinishLayoutChange() override {
				    Control::FinishLayoutChange();
				    ResetChildrenLayout();
				}
				virtual void ResetChildrenLayout() {
					_col.ForEach([&](Control *c) {
						c->ResetLayout();
						return true;
					});
				}

				virtual bool IsAnyChildFocused() const {
					if (GetWorld() != nullptr) {
						for (const Control *c = GetWorld()->FocusedControl(); c != nullptr; c = c->GetFather()) {
							if (c == this) {
								return true;
							}
						}
					}
					return false;
				}

				virtual bool HitTest(const Core::Math::Vector2 &pos) const override {
					if (_vis == Visibility::Ignored || _vis == Visibility::Ghost) {
						return false;
					}
					if (_background || GetDefaultBackground()) {
						return Control::HitTest(pos);
					}
					if (!Control::HitTest(pos)) {
						return false;
					}
					bool result = false;
					_col.ForEachReversed([&](const Control *c) {
						if (c->HitTest(pos)) {
							result = true;
							return false;
						}
						return true;
					});
					return result;
				}

				virtual bool OnMouseDown(const Core::Input::MouseButtonInfo &info) override {
					InputElement::OnMouseDown(info);
					bool res = false;
					_col.ForEachReversed([&](Control *c) {
						if (c->HitTest(info.Position)) {
							res = c->OnMouseDown(info);
							return false;
						}
						return true;
					});
					if (!res && _focusable && info.ContainsKey(Core::Input::SystemKey::LeftMouse)) {
						_world->SetFocus(this);
						return true;
					}
					return res;
				}
				virtual void OnMouseUp(const Core::Input::MouseButtonInfo &info) override {
					Control::OnMouseUp(info);
					_col.ForEachReversed([&](Control *c) {
						if (c->IsMouseOver()) {
							c->OnMouseUp(info);
							return false;
						}
						return true;
					});
				}
				virtual void OnMouseLeave(const Core::Info &info) override {
					Control::OnMouseLeave(info);
					_col.ForEach([&](Control *c) {
						if (c->IsMouseOver()) {
							c->OnMouseLeave(info);
						}
						return true;
					});
				}
				virtual void OnMouseMove(const Core::Input::MouseMoveInfo &info) override {
					Control::OnMouseMove(info);
					bool handled = false;
					_col.ForEachReversed([&](Control *c) {
						if (c->HitTest(info.Position)) {
							if (handled) {
								if (c->IsMouseOver()) {
									c->OnMouseLeave(Core::Info());
								}
							} else {
								c->OnMouseMove(info);
								handled = true;
							}
						} else if (c->IsMouseOver()) {
							c->OnMouseLeave(Core::Info());
						}
						return true;
					});
				}
				virtual bool OnMouseScroll(const Core::Input::MouseScrollInfo &info) override {
					InputElement::OnMouseScroll(info);
					bool result = false;
					_col.ForEachReversed([&](Control *c) {
						if (c->HitTest(info.Position)) {
							result = c->OnMouseScroll(info);
							return false;
						}
						return true;
					});
					return result;
				}

				virtual void OnChildrenChanged(const CollectionChangeInfo<Control*>&) {
				}

#ifdef DEBUG
                virtual void DumpData(std::ostream &out, Core::Collections::List<bool> &hnl) override {
                	for (size_t i = 0; i + 1 < hnl.Count(); ++i) {
                		out<<(hnl[i] ? " | " : "   ");
                	}
                	out<<" +- "<<typeid(*this).name()<<" "<<this<<"\n";
                	DumpDataBasicProperties(out, hnl);
                	hnl.PushBack(true);
					for (decltype(_col)::Node *n = _col.First(); n; n = n->Next()) {
						if (!(n->Next())) {
							hnl.Last() = false;
						}
						n->Value()->DumpData(out, hnl);
					}
                	hnl.PopBack();
                }
#endif

				ControlCollection _col;
				bool _overrideChildrenLayout = false;
			private:
				virtual void SetWorld(World *w) override {
					Control::SetWorld(w);
					_col.SetWorld(w);
				}
		};
		class Panel : public PanelBase {
				friend class World;
			public:
				ControlCollection &Children() {
					return _col;
				}
		};
	}
}
