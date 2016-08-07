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
		template <typename T = Graphics::TextRendering::BasicText> class ButtonBase : public ContentControl<T> {
			public:
				ButtonBase() : ContentControl<T>() {
					this->_content.HorizontalAlignment = Graphics::TextRendering::HorizontalTextAlignment::Center;
					this->_content.VerticalAlignment = Graphics::TextRendering::VerticalTextAlignment::Center;
				}
			protected:
				ButtonState _state = ButtonState::Normal;
				ButtonClickMode _mode = ButtonClickMode::OnMouseUp;

				virtual void Update(double) override {
					if (!this->IsMouseOver() && ((int)_state & (int)ButtonState::MouseDown) && !Core::IsKeyDown(VK_LBUTTON)) {
						_state = (ButtonState)((int)_state & (~(int)ButtonState::MouseDown));
					}
				}

				virtual void OnClick(const Core::Info&) = 0;
				virtual bool OnMouseDown(const Core::Input::MouseButtonInfo &info) override {
					bool res = ContentControl<T>::OnMouseDown(info);
					if (info.Button == Core::Input::MouseButton::Left) {
						_state = (ButtonState)((int)_state | (int)ButtonState::MouseDown);
						if (_mode == ButtonClickMode::OnMouseDown) {
							OnClick(Core::Info());
						}
					}
					return res;
				}
				virtual void OnMouseUp(const Core::Input::MouseButtonInfo &info) override {
					ContentControl<T>::OnMouseUp(info);
					if (info.Button == Core::Input::MouseButton::Left && ((int)_state & (int)ButtonState::MouseDown)) {
						_state = (ButtonState)((int)_state & (~(int)ButtonState::MouseDown));
						if (_mode == ButtonClickMode::OnMouseUp) {
							OnClick(Core::Info());
						}
					}
				}
				virtual void OnMouseEnter(const Core::Info &info) override {
					ContentControl<T>::OnMouseEnter(info);
					_state = (ButtonState)((int)_state | (int)ButtonState::MouseOver);
				}
				virtual void OnMouseLeave(const Core::Info &info) override {
					ContentControl<T>::OnMouseLeave(info);
					_state = (ButtonState)((int)_state & (~(int)ButtonState::MouseOver));
				}
				virtual void OnKeyDown(const Core::Input::KeyInfo &info) override {
					ContentControl<T>::OnKeyDown(info);
					if (info.Key == VK_SPACE) {
						_state = (ButtonState)((int)_state | (int)ButtonState::KeyboardPressed);
					}
				}
				virtual void OnKeyUp(const Core::Input::KeyInfo &info) override {
					ContentControl<T>::OnKeyUp(info);
					if (info.Key == VK_SPACE) {
						_state = (ButtonState)((int)_state & (~(int)ButtonState::KeyboardPressed));
						OnClick(Core::Info());
					}
				}
				virtual void OnLostFocus(const Core::Info &info) override {
					ContentControl<T>::OnLostFocus(info);
					_state = ButtonState::Normal;
				}
		};
		template <typename T = Graphics::TextRendering::BasicText> class Button : public ButtonBase<T> {
			public:
				Button() : ButtonBase<T>() {
				}

				virtual const Graphics::Brush *GetDefaultNormalBrush() const {
					return nullptr;
				}
				virtual const Graphics::Brush *GetDefaultPressedBrush() const {
					return nullptr;
				}
				virtual const Graphics::Brush *GetDefaultHoverBrush() const {
					return nullptr;
				}

				Core::Event<Core::Info> Click;

				Core::ReferenceProperty<const Graphics::Brush*>
					NormalBrush {nullptr},
					HoverBrush {nullptr},
					PressedBrush {nullptr};
				Core::GetSetProperty<ButtonClickMode> ClickMode {
					[this](ButtonClickMode c) {
						this->_mode = c;
					}, [this]() {
						return this->_mode;
					}
				};
			protected:
				virtual void OnClick(const Core::Info &info) {
					Click(info);
				}
				void Render(Graphics::Renderer &r) override {
					if (
						(((int)this->_state & (int)ButtonState::KeyboardPressed)) ||
						((int)this->_state & (int)ButtonState::MousePressed) == (int)ButtonState::MousePressed
					) {
						Control::FillRectWithFallback(r, PressedBrush, GetDefaultPressedBrush(), this->_actualLayout);
					} else if (!((int)this->_state & (int)ButtonState::MouseOver)) {
						Control::FillRectWithFallback(r, NormalBrush, GetDefaultNormalBrush(), this->_actualLayout);
					} else {
						Control::FillRectWithFallback(r, HoverBrush, GetDefaultHoverBrush(), this->_actualLayout);
					}
					ButtonBase<T>::Render(r);
				}
		};
		template <typename T = Graphics::TextRendering::BasicText> class SimpleButton : public Button<T> {
			public:
				SimpleButton() : Button<T>() {
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
		};
		template <typename T> const Graphics::SolidBrush SimpleButton<T>::DefaultNormalBrush(Core::Color(180, 180, 180, 255));
		template <typename T> const Graphics::SolidBrush SimpleButton<T>::DefaultHoverBrush(Core::Color(230, 230, 230, 255));
		template <typename T> const Graphics::SolidBrush SimpleButton<T>::DefaultPressedBrush(Core::Color(130, 130, 130, 255));
	}
}
