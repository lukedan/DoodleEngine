#pragma once

#include "Rectangle.h"
#include "InputElement.h"
#include "ObjectAllocator.h"
#include "Renderer.h"

namespace DE {
	namespace UI {
		class Control;
		class World : public Core::Input::InputElement {
				friend class Control;
			public:
				World() : InputElement() {
				}
				explicit World(Core::Window *f) : World() {
					SetFather(f);
				}
				~World() {
					SetFather(nullptr);
				}

				Core::Math::Rectangle GetBounds() const {
					return _bound;
				}
				void SetBounds(const Core::Math::Rectangle&);

				void SetFather(Core::Window*);
				const Core::Window* GetFather() const {
					return _father;
				}

				void SetChild(Control*);
				const Control *GetChild() const {
					return _child;
				}
				Control *GetChild() {
					return _child;
				}

				virtual void SetFocus(Control*);

				virtual void Update(double);
				virtual void Render(Graphics::Renderer&);

				virtual void OnKeyDown(const Core::Input::KeyInfo&) override;
				virtual void OnKeyUp(const Core::Input::KeyInfo&) override;
				virtual bool OnMouseDown(const Core::Input::MouseButtonInfo&) override;
				virtual void OnMouseUp(const Core::Input::MouseButtonInfo&) override;
				virtual void OnMouseMove(const Core::Input::MouseMoveInfo&) override;
				virtual void OnMouseEnter(const Core::Info&) override;
				virtual void OnMouseLeave(const Core::Info&) override;
				virtual void OnMouseHover(const Core::Input::MouseButtonInfo&) override;
				virtual bool OnMouseScroll(const Core::Input::MouseScrollInfo&) override;
				virtual void OnText(const Core::Input::TextInfo&) override;
				virtual void OnGotFocus(const Core::Info&) override;
				virtual void OnLostFocus(const Core::Info&) override;
				virtual void OnSetCursor(const Core::Info&);

				virtual Core::Input::Cursor GetCursor() const override;

				Control *FocusedControl() {
					return _focus;
				}
				const Control *FocusedControl() const {
					return _focus;
				}
				bool Focused() const {
					return _focused;
				}

				Core::Event<Core::Info> FocusChanged;
			private:
				struct ListenerAttachments {
					ListenerAttachments(World&, Core::Window&);

					Core::AutomaticEventHandlerToken<Core::Input::KeyInfo>
						KeyDownListener, KeyUpListener;
					Core::AutomaticEventHandlerToken<Core::Input::MouseButtonInfo>
						MouseDownListener, MouseUpListener, MouseHoverListener;
					Core::AutomaticEventHandlerToken<Core::Input::MouseMoveInfo>
						MouseMoveListener;
					Core::AutomaticEventHandlerToken<Core::Input::MouseScrollInfo>
						MouseScrollListener;
					Core::AutomaticEventHandlerToken<Core::Input::TextInfo>
						TextListener;
					Core::AutomaticEventHandlerToken<Core::Info>
						MouseEnterListener, MouseLeaveListener, GotFocusListener, LostFocusListener, SetCursorListener;
				};

				Core::Math::Rectangle _bound;
				Control *_child = nullptr, *_focus = nullptr;
				Core::Window *_father = nullptr;
				bool _focused = false;
				ListenerAttachments *_listeners = nullptr;
		};
	}
}
