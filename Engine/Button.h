#pragma once

#include "ContentControl.h"
#include "Text.h"

namespace DE {
	namespace UI {
		enum class ButtonState {
			Normal = 0,
			MouseOver = 1,
			MouseDown = 2,
			MousePressed = MouseOver | MouseDown,
			KeyboardPressed = 4
		};
		enum class ButtonClickMode {
			OnMouseDown,
			OnMouseUp
		};
		class ButtonBase : public ContentControl {
			public:
				ButtonBase() : ContentControl() {
					_content.HorizontalAlignment = Graphics::TextRendering::HorizontalTextAlignment::Center;
					_content.VerticalAlignment = Graphics::TextRendering::VerticalTextAlignment::Center;
				}
				virtual ~ButtonBase() {
				}
			protected:
				ButtonState _state = ButtonState::Normal;
				ButtonClickMode _mode = ButtonClickMode::OnMouseUp;

				virtual void Update(double) override {
					if (!IsMouseOver() && ((int)_state & (int)ButtonState::MouseDown) && !Core::IsKeyDown(VK_LBUTTON)) {
						_state = (ButtonState)((int)_state & (~(int)ButtonState::MouseDown));
					}
				}

				virtual void OnClick(const Core::Info&) = 0;
				virtual bool OnMouseDown(const Core::Input::MouseButtonInfo &info) override {
					bool res = ContentControl::OnMouseDown(info);
					if (info.Button == Core::Input::MouseButton::Left) {
						_state = (ButtonState)((int)_state | (int)ButtonState::MouseDown);
						if (_mode == ButtonClickMode::OnMouseDown) {
							OnClick(Core::Info());
						}
					}
					return res;
				}
				virtual void OnMouseUp(const Core::Input::MouseButtonInfo &info) override {
					ContentControl::OnMouseUp(info);
					if (info.Button == Core::Input::MouseButton::Left && ((int)_state & (int)ButtonState::MouseDown)) {
						_state = (ButtonState)((int)_state & (~(int)ButtonState::MouseDown));
						if (_mode == ButtonClickMode::OnMouseUp) {
							OnClick(Core::Info());
						}
					}
				}
				virtual void OnMouseEnter(const Core::Info &info) override {
					ContentControl::OnMouseEnter(info);
					_state = (ButtonState)((int)_state | (int)ButtonState::MouseOver);
				}
				virtual void OnMouseLeave(const Core::Info &info) override {
					ContentControl::OnMouseLeave(info);
					_state = (ButtonState)((int)_state & (~(int)ButtonState::MouseOver));
				}
				virtual void OnKeyDown(const Core::Input::KeyInfo &info) override {
					ContentControl::OnKeyDown(info);
					if (info.GetKey() == VK_SPACE) {
						_state = (ButtonState)((int)_state | (int)ButtonState::KeyboardPressed);
					}
				}
				virtual void OnKeyUp(const Core::Input::KeyInfo &info) override {
					ContentControl::OnKeyUp(info);
					if (info.GetKey() == VK_SPACE) {
						_state = (ButtonState)((int)_state & (~(int)ButtonState::KeyboardPressed));
						OnClick(Core::Info());
					}
				}
				virtual void OnLostFocus(const Core::Info &info) override {
					ContentControl::OnLostFocus(info);
					_state = ButtonState::Normal;
				}
		};
		class Button : public ButtonBase {
			public:
				Button() : ButtonBase() {
				}

				virtual const Graphics::Brush *GetDefaultNormalBrush() const = 0;
				virtual const Graphics::Brush *GetDefaultPressedBrush() const = 0;
				virtual const Graphics::Brush *GetDefaultHoverBrush() const = 0;

//				const Graphics::Brush *&NormalBrush() {
//					if (!Initialized()) {
//						Initialize();
//					}
//					return _freeBrush;
//				}
//				const Graphics::Brush *const &NormalBrush() const {
//					if (!Initialized()) {
//						Initialize();
//					}
//					return _freeBrush;
//				}
//				const Graphics::Brush *&PressedBrush() {
//					if (!Initialized()) {
//						Initialize();
//					}
//					return _downBrush;
//				}
//				const Graphics::Brush *const &PressedBrush() const {
//					if (!Initialized()) {
//						Initialize();
//					}
//					return _downBrush;
//				}
//				const Graphics::Brush *&HoverBrush() {
//					if (!Initialized()) {
//						Initialize();
//					}
//					return _mOverBrush;
//				}
//				const Graphics::Brush *const &HoverBrush() const {
//					return _mOverBrush;
//				}

				Core::Event<Core::Info> Click;

				Core::GetSetProperty<const Graphics::Brush*>
					NormalBrush = Core::GetSetProperty<const Graphics::Brush*>([this](const Graphics::Brush *brush) {
						if (!Initialized()) {
							Initialize();
						}
						_freeBrush = brush;
					}, [this]() {
						if (!Initialized()) {
							Initialize();
						}
						return _freeBrush;
					}), HoverBrush = Core::GetSetProperty<const Graphics::Brush*>([this](const Graphics::Brush *brush) {
						if (!Initialized()) {
							Initialize();
						}
						_mOverBrush = brush;
					}, [this]() {
						if (!Initialized()) {
							Initialize();
						}
						return _mOverBrush;
					}), PressedBrush = Core::GetSetProperty<const Graphics::Brush*>([this](const Graphics::Brush *brush) {
						if (!Initialized()) {
							Initialize();
						}
						_downBrush = brush;
					}, [this]() {
						if (!Initialized()) {
							Initialize();
						}
						return _downBrush;
					});
				Core::GetSetProperty<ButtonClickMode> ClickMode = Core::GetSetProperty<ButtonClickMode>(
					[this](ButtonClickMode c) {
						_mode = c;
					}, [this]() {
						return _mode;
					}
				);
			protected:
				const Graphics::Brush *_freeBrush = nullptr, *_mOverBrush = nullptr, *_downBrush = nullptr;

				virtual void OnClick(const Core::Info &info) {
					Click(info);
				}

				void Initialize() override {
					ButtonBase::Initialize();
					_freeBrush = GetDefaultNormalBrush();
					_mOverBrush = GetDefaultHoverBrush();
					_downBrush = GetDefaultPressedBrush();
				}

				void Render(Graphics::Renderer &r) override {
					if (
						(((int)_state & (int)ButtonState::KeyboardPressed)) ||
						((int)_state & (int)ButtonState::MousePressed) == (int)ButtonState::MousePressed
					) {
						FillRectWithFallback(_actualLayout, _downBrush, GetDefaultPressedBrush(), r);
					} else if (!((int)_state & (int)ButtonState::MouseOver)) {
						FillRectWithFallback(_actualLayout, _freeBrush, GetDefaultNormalBrush(), r);
					} else {
						FillRectWithFallback(_actualLayout, _mOverBrush, GetDefaultHoverBrush(), r);
					}
					ButtonBase::Render(r);
				}
				inline static void FillRectWithFallback(
					const DE::Core::Math::Rectangle &rect,
					const Graphics::Brush *b,
					const Graphics::Brush *fallBack,
					Graphics::Renderer &r
				) {
					if (b) {
						b->FillRect(rect, r);
					} else if (fallBack) {
						fallBack->FillRect(rect, r);
					}
				}
		};
		class SimpleButton : public Button {
			public:
				SimpleButton() : Button() {
				}

				const Graphics::Brush *GetDefaultNormalBrush() const override {
					return &DefaultNormalBrush;
				}
				const Graphics::Brush *GetDefaultHoverBrush() const override {
					return &DefaultHoverBrush;
				}
				const Graphics::Brush *GetDefaultPressedBrush() const override {
					return &DefaultPressedBrush;
				}

				const static Graphics::SolidBrush
					DefaultNormalBrush,
					DefaultHoverBrush,
					DefaultPressedBrush;
			protected:
		};
	}
}
