#include "Control.h"

#ifdef DEBUG
#	include <iostream>
#endif

#include "List.h"
#include "Panel.h"

namespace DE {
	namespace UI {
		using namespace Core;
		using namespace Core::Math;
		using namespace Core::Collections;
		using namespace Core::Input;
		using namespace Graphics;
		using namespace Graphics::RenderingContexts;

		const Pen Control::FocusBorderPen(Color(100, 255, 100, 255));
		const double Control::FocusBorderWidth = 4.0;

		Control::~Control() {
			_disposing = true;
			if (_world) {
				if (_world->FocusedControl() == this) {
					_world->_focus = nullptr;
				}
				if (_world->GetChild() == this) {
					_world->_child = nullptr;
				}
			}
			if (_father) {
				_father->_col.Delete(*this);
			}
		}

		bool Control::Focused() const {
			return _focusable && _world && _world->Focused() && _world->FocusedControl() == this;
		}

		void Control::SolveArrangement(
			bool aSide, bool bSide, double tot,
			double ma, double mb, double w,
			double &ares, double &bres, double &wr
		) {
			if (aSide && bSide) {
				ares = ma;
				bres = mb;
				wr = tot - ma - mb;
			} else if (aSide || bSide) {
				wr = w;
				if (aSide) {
					ares = ma;
					bres = tot - w - ma;
				} else {
					bres = mb;
					ares = tot - w - mb;
				}
			} else {
				double t = ma / (ma + mb), tt = tot - w;
				wr = w;
				ares = tt * t;
				bres = tt - ares;
			}
		}

		void Control::AnchorTo(Anchor anc) {
			if ((~(int)_anchor) & (int)anc) {
				_anchor = (Anchor)((int)_anchor | (int)anc);
				ResetLayout();
			}
		}
		void Control::DeanchorFrom(Anchor anc) {
			if ((int)_anchor & (int)anc) {
				_anchor = (Anchor)((int)_anchor & (~(int)anc));
				ResetLayout();
			}
		}

		void Control::BeginRendering(Graphics::Renderer &r) {
			r.PushRectangularClip(_actualLayout);
			// bottom layer
			const Graphics::Brush *bkg = (_background ? _background : GetDefaultBackground());
			if (bkg) {
				bkg->FillRect(_actualLayout, r);
			}
		}
		void Control::Render(Graphics::Renderer &r) {
		}
		void Control::EndRendering(Graphics::Renderer &r) {
			// top layer
			const Graphics::Pen *border = (_border ? _border : GetDefaultBorder());
			if (border) {
				border->DrawRectangle(_actualLayout, r);
			}
			if (Focused()) {
				Math::Rectangle rect = _actualLayout;
				rect.Top += FocusBorderWidth;
				rect.Bottom -= FocusBorderWidth;
				rect.Left += FocusBorderWidth;
				rect.Right -= FocusBorderWidth;
				FocusBorderPen.DrawRectangle(rect, r);
			}
			// cleanup
			r.PopRectangularClip();
		}
		void Control::ResetVerticalLayout() {
			if (_world) {
				const Rectangle &rect = (_father ? _father->_actualLayout : _world->GetBounds());
				SolveArrangement(
					(int)_anchor & (int)Anchor::Top, (int)_anchor & (int)Anchor::Bottom, rect.Height(),
					_margin.Top, _margin.Bottom, _size.Height,
					_actualMargin.Top, _actualMargin.Bottom, actualSize.Height);
				_actualLayout.Top = rect.Top + _actualMargin.Top;
				_actualLayout.Bottom = _actualLayout.Top + actualSize.Height;
			}
		}
		void Control::ResetHorizontalLayout() {
			if (_world) {
				const Rectangle &rect = (_father ? _father->_actualLayout : _world->GetBounds());
				SolveArrangement(
					(int)_anchor & (int)Anchor::Left, (int)_anchor & (int)Anchor::Right, rect.Width(),
					_margin.Left, _margin.Right, _size.Width,
					_actualMargin.Left, _actualMargin.Right, actualSize.Width);
				_actualLayout.Left = rect.Left + _actualMargin.Left;
				_actualLayout.Right = _actualLayout.Left + actualSize.Width;
			}
		}
		void Control::ResetLayout() {
			if (_world) {
				if (_father && (!_father->_disposing && _father->OverrideChildrenLayout())) {
					_father->ResetChildrenLayout();
				} else {
					ResetVerticalLayout();
					ResetHorizontalLayout();
					FinishLayoutChange();
				}
			}
		}
		void Control::FinishLayoutChange() {
		    _relPos = _world->GetRelativeMousePosition();
			topLeft = _actualLayout.TopLeft();
			bool overNow = HitTest(_relPos) && _world->IsMouseOver();
			if (_over && !overNow) {
				OnMouseLeave(Info());
			} else if (!_over && overNow) {
			    OnMouseEnter(Info());
			}
			_over = overNow;
			_relPos -= topLeft;
		}

		bool Control::OnMouseDown(const MouseButtonInfo &info) {
			InputElement::OnMouseDown(info);
			if (_focusable && info.ContainsKey(SystemKey::LeftMouse)) {
				_world->SetFocus(this);
				return true;
			}
			return false;
		}
		bool Control::OnMouseScroll(const Core::Input::MouseScrollInfo &info) {
			InputElement::OnMouseScroll(info);
			return false;
		}

		void Control::SetWorld(World *w) {
			if (!_inited) {
				Initialize();
			}
			if (_world != nullptr && _world->FocusedControl() == this) {
				_world->SetFocus(nullptr);
			}
			_world = w;
			OnWorldChanged(Core::Info());
		}
	}
}
