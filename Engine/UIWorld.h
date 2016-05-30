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
				Core::Math::Rectangle _bound;
				Control *_child = nullptr, *_focus = nullptr;
				Core::Window *_father = nullptr;
				bool _focused = false;

				Core::Event<Core::Input::KeyInfo>::Handler
					keyDownL = [this](const Core::Input::KeyInfo &info) { OnKeyDown(info); },
					keyUpL = [this](const Core::Input::KeyInfo &info) { OnKeyUp(info); };
				Core::Event<Core::Input::MouseButtonInfo>::Handler
					mouseDownL = [this](const Core::Input::MouseButtonInfo &info) { OnMouseDown(info); },
					mouseUpL = [this](const Core::Input::MouseButtonInfo &info) { OnMouseUp(info); },
					mouseHoverL = [this](const Core::Input::MouseButtonInfo &info) { OnMouseHover(info); };
				Core::Event<Core::Input::MouseMoveInfo>::Handler
					mouseMoveL = [this](const Core::Input::MouseMoveInfo &info) { OnMouseMove(info); };
				Core::Event<Core::Info>::Handler
					mouseEnterL = [this](const Core::Info &info) { OnMouseEnter(info); },
					mouseLeaveL = [this](const Core::Info &info) { OnMouseLeave(info); },
					gotFocusL = [this](const Core::Info &info) { OnGotFocus(info); },
					lostFocusL = [this](const Core::Info &info) { OnLostFocus(info); },
					onSetCursorL = [this](const Core::Info &info) { OnSetCursor(info); };
				Core::Event<Core::Input::MouseScrollInfo>::Handler
					mouseScrollL = [this](const Core::Input::MouseScrollInfo &info) { OnMouseScroll(info); };
				Core::Event<Core::Input::TextInfo>::Handler
					textL = [this](const Core::Input::TextInfo &info) { OnText(info); };
		};
	}
}
