#include "UIWorld.h"

#include "Control.h"

namespace DE {
	namespace UI {
		using namespace Core;
		using namespace Core::Math;
		using namespace Core::Input;
		using namespace Graphics;

		void World::SetBounds(const Math::Rectangle &bounds) {
			_bound = bounds;
			topLeft = bounds.TopLeft();
			if (_child) {
				_child->ResetLayout();
			}
		}
		void World::SetChild(Control *con) {
			if (con == _child) {
				return;
			}
			if (con) {
				if (con->GetWorld() != nullptr) {
					throw InvalidOperationException(_TEXT("this control belongs to another world"));
				}
			}
			if (_focus) {
				_focus->OnLostFocus(Info());
				_focus = nullptr;
			}
			if (_child) {
				_child->SetWorld(nullptr);
			}
			_child = con;
			if (_child) {
				_child->SetWorld(this);
				_child->ResetLayout();
			}
		}
		void World::SetFather(Window *el) {
			if (_father) {
				_listeners->~ListenerAttachments();
				Core::GlobalAllocator::Free(_listeners);
				--_father->CursorOverrideCount();
			}
			_father = el;
			if (_father) {
				_listeners = new (GlobalAllocator::Allocate(sizeof(ListenerAttachments))) ListenerAttachments(*this, *_father);
				++_father->CursorOverrideCount();
				OnMouseMove(MouseMoveInfo(_father->GetRelativeMousePosition(), SystemKey::None));
			}
		}

		void World::Update(double dt) {
			if (_child) {
				_child->Update(dt);
			}
		}
		void World::Render(Renderer &r) {
			if (_child && (_child->_vis == Visibility::Visible || _child->_vis == Visibility::Ghost)) {
				_child->BeginRendering(r);
				_child->Render(r);
				_child->EndRendering(r);
			}
		}

		void World::OnKeyDown(const KeyInfo &info) {
			InputElement::OnKeyDown(info);
			if (_focus) {
				_focus->OnKeyDown(info);
			}
		}
		void World::OnKeyUp(const KeyInfo &info) {
			InputElement::OnKeyUp(info);
			if (_focus) {
				_focus->OnKeyUp(info);
			}
		}
		bool World::OnMouseDown(const MouseButtonInfo &info) {
			InputElement::OnMouseDown(info);
			if (_child && _child->IsMouseOver()) {
				return _child->OnMouseDown(info);
			}
			return false;
		}
		void World::OnMouseUp(const MouseButtonInfo &info) {
			InputElement::OnMouseUp(info);
			if (_child) {
				_child->OnMouseUp(info);
			}
		}
		void World::OnMouseMove(const MouseMoveInfo &info) {
			InputElement::OnMouseMove(info);
			if (_child) {
				if (_child->HitTest(info.Position)) {
					_child->OnMouseMove(info);
				} else if (_child->IsMouseOver()) {
					_child->OnMouseLeave(Info());
				}
			}
		}
		void World::OnMouseEnter(const Info &info) {
			InputElement::OnMouseEnter(info);
		}
		void World::OnMouseLeave(const Info &info) {
			InputElement::OnMouseLeave(info);
			if (_child && _child->IsMouseOver()) {
				_child->OnMouseLeave(info);
			}
		}
		void World::OnMouseHover(const MouseButtonInfo &info) {
			InputElement::OnMouseHover(info);
			if (_child && _child->IsMouseOver()) {
				_child->OnMouseHover(info);
			}
		}
		bool World::OnMouseScroll(const MouseScrollInfo &info) {
			InputElement::OnMouseScroll(info);
			Control *c = _child;
			if (_child) {
				if (_child->HitTest(info.Position)) {
					return c->OnMouseScroll(info);
				}
			}
			return false;
		}
		void World::OnText(const TextInfo &info) {
			InputElement::OnText(info);
			if (_focus) {
				_focus->OnText(info);
			}
		}
		void World::OnGotFocus(const Info &info) {
			InputElement::OnGotFocus(info);
			_focused = true;
			if (_focus) {
				_focus->OnGotFocus(info);
			}
		}
		void World::OnLostFocus(const Info &info) {
			InputElement::OnLostFocus(info);
			_focused = false;
			if (_focus) {
				_focus->OnLostFocus(info);
			}
		}
		void World::OnSetCursor(const Core::Info&) {
			if (IsMouseOver()) {
				GetCursor().Set();
			}
		}

		Core::Input::Cursor World::GetCursor() const {
			if (_child && _child->IsMouseOver()) {
				return _child->GetCursor();
			}
			return Core::Input::InputElement::GetCursor();
		}

		void World::SetFocus(Control *con) {
			if (con && con->GetWorld() != this) {
				throw InvalidOperationException(_TEXT("the control doesn't belong to this world"));
			}
			if (con == _focus) {
				return;
			}
			Control *oldFocus = _focus;
			bool canGetFocus = (con && con->Focusable());
			for (const Control *c = con; canGetFocus && c != nullptr; c = reinterpret_cast<const Control*>(c->GetFather())) {
				if (c->GetVisibility() == Visibility::Ignored) {
					canGetFocus = false;
				}
			}
			if (canGetFocus) {
				_focus = con;
			} else {
				_focus = nullptr;
			}
			if (oldFocus) {
				oldFocus->OnLostFocus(Info());
			}
			if (canGetFocus) {
				con->OnGotFocus(Info());
			}
			FocusChanged(Info());
		}

		World::ListenerAttachments::ListenerAttachments(World &w, Core::Window &wnd) :
			KeyDownListener(wnd.KeyDown += [&](const Core::Input::KeyInfo &info) { w.OnKeyDown(info); }),
			KeyUpListener(wnd.KeyUp += [&](const Core::Input::KeyInfo &info) { w.OnKeyUp(info); }),
			MouseDownListener(wnd.MouseDown += [&](const Core::Input::MouseButtonInfo &info) { w.OnMouseDown(info); }),
			MouseUpListener(wnd.MouseUp += [&](const Core::Input::MouseButtonInfo &info) { w.OnMouseUp(info); }),
			MouseHoverListener(wnd.MouseHover += [&](const Core::Input::MouseButtonInfo &info) { w.OnMouseHover(info); }),
			MouseMoveListener(wnd.MouseMove += [&](const Core::Input::MouseMoveInfo &info) { w.OnMouseMove(info); }),
			MouseScrollListener(wnd.MouseScroll += [&](const Core::Input::MouseScrollInfo &info) { w.OnMouseScroll(info); }),
			TextListener(wnd.KeyboardText += [&](const Core::Input::TextInfo &info) { w.OnText(info); }),
			MouseEnterListener(wnd.MouseEnter += [&](const Core::Info &info) { w.OnMouseEnter(info); }),
			MouseLeaveListener(wnd.MouseLeave += [&](const Core::Info &info) { w.OnMouseLeave(info); }),
			GotFocusListener(wnd.GotFocus += [&](const Core::Info &info) { w.OnGotFocus(info); }),
			LostFocusListener(wnd.LostFocus += [&](const Core::Info &info) { w.OnLostFocus(info); }),
			SetCursorListener(wnd.OnSetCursor += [&](const Core::Info &info) { w.OnSetCursor(info); })
		{
		}
	}
}
