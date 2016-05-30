#pragma once

#ifdef DEBUG
#	include <fstream>
#endif

#include "Panel.h"
#include "ScrollBar.h"

namespace DE {
	namespace UI {
		enum class ScrollBarVisibility {
			Hidden,
			Visible,
			Auto
		};
		class ScrollViewBase : public PanelBase {
			public:
				ScrollViewBase() : PanelBase() {
					_overrideChildrenLayout = true;

					_vert.SetAnchor(Anchor::RightDock);
					_vert.SetMargins(Thickness());
					_vert.SetSize(Size(ScrollBarBase::DefaultWidth, 0.0));
					_vert.SetLayoutDirection(LayoutDirection::Vertical);
					_vert.Scroll += [this](const ScrollInfo&) {
						ResetChildLayout();
					};

					_hori.SetAnchor(Anchor::BottomDock);
					_hori.SetMargins(Thickness());
					_hori.SetSize(Size(0.0, ScrollBarBase::DefaultWidth));
					_hori.SetLayoutDirection(LayoutDirection::Horizontal);
					_hori.Scroll += [this](const ScrollInfo&) {
						ResetChildLayout();
					};

					_tokBkg.BrushColor() = Core::Color(255, 255, 255, 0);

					_tok.SetAnchor(Anchor::BottomRight);
					_tok.SetMargins(Thickness());
					_tok.Focusable() = false;
					_tok.SetSize(Size(ScrollBarBase::DefaultWidth, ScrollBarBase::DefaultWidth));
					_tok.SetVisibility(Visibility::Ignored);
					_tok.Background() = &_tokBkg;
#ifdef DEBUG
					_vert.Name = "a vertical scroll bar in a scroll view";
					_hori.Name = "a horizontal scroll bar in a scroll view";
					_tok.Name = "a background token in a scroll view";
#endif
				}
				virtual ~ScrollViewBase() {
					_ssbs = true;
				}

			protected:
				virtual void Initialize() override {
					PanelBase::Initialize();
					_col.Insert(_vert);
					_col.Insert(_hori);
					_col.Insert(_tok);
				}

				void SetChild(Control *con) {
					if (!Initialized()) {
						Initialize();
					}
					if (_child) {
						_col.Delete(*_child);
					}
					_child = con;
					if (con) {
						_col.Insert(*con);
						_col.SetZIndex(_tok, con->GetZIndex());
						_col.SetZIndex(_vert, con->GetZIndex());
						_col.SetZIndex(_hori, con->GetZIndex());
					}
					ResetScrollBars();
				}
				Control *GetChild() {
					return _child;
				}
				const Control *GetChild() const {
					return _child;
				}

				const Core::Math::Rectangle &GetVisibleRange() const {
					return _visibleRgn;
				}

				ScrollBarVisibility GetHorizontalScrollBarVisibility() const {
					return _horVis;
				}
				void SetHorizontalScrollBarVisibility(ScrollBarVisibility visibility) {
					_horVis = visibility;
					ResetScrollBars();
				}
				ScrollBarVisibility GetVerticalScrollBarVisibility() const {
					return _verVis;
				}
				void SetVerticalScrollBarVisibility(ScrollBarVisibility visibility) {
					_verVis = visibility;
					ResetScrollBars();
				}

				double GetHorizontalScrollBarValue() const {
					return _hori.GetValue();
				}
				void SetHorizontalScrollBarValue(double value) {
					_hori.SetValue(value);
				}
				double GetVerticalScrollBarValue() const {
					return _vert.GetValue();
				}
				void SetVerticalScrollBarValue(double value) {
					_vert.SetValue(value);
				}

				double GetHorizontalScrollBarHeight() const {
					return _hori.GetSize().Height;
				}
				void SetHorizontalScrollBarHeight(double height) {
					_hori.SetSize(Size(_hori.GetSize().Width, height));
				}
				double GetVerticalScrollBarWidth() const {
					return _vert.GetSize().Width;
				}
				void SetVerticalScrollBarWidth(double width) {
					_vert.SetSize(Size(width, _vert.GetSize().Height));
				}

				void FinishLayoutChange() override {
					PanelBase::FinishLayoutChange();
					ResetScrollBars();
				}

				void MakePointInView(const Core::Math::Vector2 &vec) {
					Control *c = GetChild();
					if (c) {
						Core::Math::Rectangle rect;
						Core::Math::Vector2 vpos = _visibleRgn.TopLeft() - c->GetActualLayout().TopLeft();
						rect.Left = vpos.X;
						rect.Top = vpos.Y;
						rect.Right = rect.Left + _visibleRgn.Width();
						rect.Bottom = rect.Top + _visibleRgn.Height();
						if (vec.X < rect.Left) {
							_hori.SetValue(vec.X);
						} else if (vec.X > rect.Right) {
							_hori.SetValue(vec.X - _hori.GetViewRange());
						}
						if (vec.Y < rect.Top) {
							_vert.SetValue(vec.Y);
						} else if (vec.Y > rect.Bottom) {
							_vert.SetValue(vec.Y - _vert.GetViewRange());
						}
					}
				}

				virtual void ResetChildLayout() {
					if (_world) {
						Control *con = GetChild();
						if (con) {
							if (con->_father == this) {
								con->_actualLayout.Top = _actualLayout.Top - _vert.GetValue();
								con->_actualLayout.Bottom = con->_actualLayout.Top + con->GetSize().Height;
								con->_actualLayout.Left = _actualLayout.Left - _hori.GetValue();
								con->_actualLayout.Right = con->_actualLayout.Left + con->GetSize().Width;
								con->FinishLayoutChange();
							} else {
								_child = nullptr;
							}
						}
					}
				}
				virtual void ResetChildrenLayout() override {
					if (!_ssbs) {
						ResetScrollBars();
					}
					Control *child = GetChild();
					for (decltype(_col)::Node *cur = _col.First(); cur; cur = cur->Next()) {
						Control *c = cur->Value();
						if (c != child) {
							c->ResetVerticalLayout();
							c->ResetHorizontalLayout();
							c->FinishLayoutChange();
						}
					}
					ResetChildLayout();
				}

				virtual void Render(Graphics::Renderer &r) override {
					Control::Render(r);
					Control *c = GetChild();
					if (c && c->GetVisibility() == Visibility::Visible) {
						r.PushRectangularClip(_visibleRgn);
						RenderChild(r, c);
						r.PopRectangularClip();
					}
					if (_hori.GetFather() == this && _hori.GetVisibility() == Visibility::Visible) {
						static_cast<Control*>(&_hori)->BeginRendering(r);
						static_cast<Control*>(&_hori)->Render(r);
						static_cast<Control*>(&_hori)->EndRendering(r);
					}
					if (_vert.GetFather() == this && _vert.GetVisibility() == Visibility::Visible) {
						static_cast<Control*>(&_vert)->BeginRendering(r);
						static_cast<Control*>(&_vert)->Render(r);
						static_cast<Control*>(&_vert)->EndRendering(r);
					}
				}
				virtual void RenderChild(Graphics::Renderer &r, Control *c) {
					c->BeginRendering(r);
					c->Render(r);
					c->EndRendering(r);
				}

				virtual void ResetScrollBars() {
					_ssbs = true;
					_visibleRgn = _actualLayout;
					Control *c = GetChild();
					if (c) {
						bool goon = true;
						while (goon) {
							goon = false;
							if (ResetScrollBar(
								_horVis,
								_hori,
								_visibleRgn.Bottom,
								_actualLayout.Bottom,
								_hori.GetSize().Height,
								_visibleRgn.Width(),
								c->GetSize().Width
							)) {
								goon = true;
							}
							if (ResetScrollBar(
								_verVis,
								_vert,
								_visibleRgn.Right,
								_actualLayout.Right,
								_vert.GetSize().Width,
								_visibleRgn.Height(),
								c->GetSize().Height
							)) {
								goon = true;
							}
						}
						if (_vert.GetVisibility() == Visibility::Visible && _hori.GetVisibility() == Visibility::Visible) {
							_vert.SetMargins(Thickness(0.0, 0.0, 0.0, _hori.GetSize().Height));
							_hori.SetMargins(Thickness(0.0, 0.0, _vert.GetSize().Width, 0.0));
							_tok.SetVisibility(Visibility::Visible);
							_tok.SetSize(Size(_vert.GetSize().Width, _hori.GetSize().Height));
						} else {
							_vert.SetMargins(Thickness());
							_hori.SetMargins(Thickness());
							_tok.SetVisibility(Visibility::Ignored);
						}
						double rgn, newv;
						rgn = Core::Math::Max(_visibleRgn.Width(), c->GetSize().Width);
						newv = Core::Math::Min(_hori.GetValue(), rgn - _visibleRgn.Width());
						_hori.SetScrollBarProperties(rgn, _visibleRgn.Width(), newv);
						_hori.PageDelta() = _visibleRgn.Width();
						rgn = Core::Math::Max(_visibleRgn.Height(), c->GetSize().Height);
						newv = Core::Math::Min(_vert.GetValue(), rgn - _visibleRgn.Height());
						_vert.SetScrollBarProperties(rgn, _visibleRgn.Height(), newv);
						_vert.PageDelta() = _visibleRgn.Height();
					}
					_ssbs = false;
				}
				virtual bool ResetScrollBar(
					ScrollBarVisibility visibility,
					ScrollBarBase &bar,
					double &target,
					double ttv,
					double barW,
					double vrv,
					double csv
				) {
					switch (visibility) {
						case ScrollBarVisibility::Hidden: {
							bar.SetVisibility(Visibility::Ignored);
							target = ttv;
							return false;
						}
						case ScrollBarVisibility::Visible: {
							bar.SetVisibility(Visibility::Visible);
							target = ttv - barW;
							return false;
						}
						case ScrollBarVisibility::Auto: {
							if (vrv < csv) {
								target = ttv - barW;
								if (bar.GetVisibility() != Visibility::Visible) {
									bar.SetVisibility(Visibility::Visible);
									return true;
								}
								return false;
							}
							target = ttv;
							if (bar.GetVisibility() != Visibility::Ignored) {
								bar.SetVisibility(Visibility::Ignored);
								return true;
							}
							return false;
						}
					}
					return false;
				}

				virtual bool OnMouseScroll(const Core::Input::MouseScrollInfo &info) override {
					InputElement::OnMouseScroll(info);
					if (_child && _child->HitTest(info.Position) && _child->OnMouseScroll(info)) {
						return true;
					}
					return _vert.HandleMouseScroll(info);
				}

				ScrollBarBase _vert, _hori;
				Panel _tok;
				Graphics::SolidBrush _tokBkg;
				ScrollBarVisibility _verVis = ScrollBarVisibility::Auto, _horVis = ScrollBarVisibility::Auto;
				Core::Math::Rectangle _visibleRgn;
				Control *_child = nullptr;
				bool _ssbs = false;
		};
		class SimpleScrollView : public ScrollViewBase {
			public:
				using ScrollViewBase::SetChild;
				using ScrollViewBase::GetChild;
				using ScrollViewBase::GetVisibleRange;
				using ScrollViewBase::GetHorizontalScrollBarVisibility;
				using ScrollViewBase::SetHorizontalScrollBarVisibility;
				using ScrollViewBase::GetVerticalScrollBarVisibility;
				using ScrollViewBase::SetVerticalScrollBarVisibility;
				using ScrollViewBase::GetHorizontalScrollBarValue;
				using ScrollViewBase::SetHorizontalScrollBarValue;
				using ScrollViewBase::GetVerticalScrollBarValue;
				using ScrollViewBase::SetVerticalScrollBarValue;

				using ScrollViewBase::GetHorizontalScrollBarHeight;
				using ScrollViewBase::SetHorizontalScrollBarHeight;
				using ScrollViewBase::GetVerticalScrollBarWidth;
				using ScrollViewBase::SetVerticalScrollBarWidth;

				using ScrollViewBase::MakePointInView;
		};
	}
}
