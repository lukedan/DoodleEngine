#pragma once

#include "Button.h"

namespace DE {
	namespace UI {
		enum class CheckBoxState {
			Unchecked = 0,
			Checked = 1,
			HalfChecked = 2
		};
		enum class CheckBoxAlignment {
			TopLeft,
			MiddleLeft,
			BottomLeft
		};
		enum class CheckBoxType {
			Button,
			Box
		};

		struct CheckBoxStateChangeInfo {
			public:
				CheckBoxStateChangeInfo(CheckBoxState oldState, CheckBoxState newState) :
					OldState(oldState), NewState(newState)
				{
				}

				Core::ReferenceProperty<CheckBoxState, Core::PropertyType::ReadOnly> OldState, NewState;
		};
		template <typename T/* = Graphics::TextRendering::BasicText*/> class CheckBoxBase : public ButtonBase<T> {
				friend class World;
			public:
				const static Size DefaultBoxSize;

				CheckBoxBase() : ButtonBase<T>() {
					this->_content.HorizontalAlignment = Graphics::TextRendering::HorizontalTextAlignment::Left;
				}

				CheckBoxState &State() {
					return _checkState;
				}
				const CheckBoxState &State() const {
					return _checkState;
				}

				Size GetCheckBoxSize() const {
					return _boxSize;
				}
				void SetCheckBoxSize(const Size &newSize) {
					_boxSize = newSize;
					OnBoxSizeChanged();
				}

				CheckBoxAlignment GetBoxAlignment() const {
					return _align;
				}
				void SetBoxAlignment(CheckBoxAlignment align) {
					_align = align;
					OnBoxAlignmentChanged();
				}

				bool &ThreeState() {
					return _threeState;
				}
				const bool &ThreeState() const {
					return _threeState;
				}

				virtual void FitContent() override {
					Core::Math::Vector2 cSize = this->_content.GetSize();
					if (_type == CheckBoxType::Box) {
						this->SetSize(Size(cSize.X + _boxSize.Width, Core::Math::Max(cSize.Y, _boxSize.Height)));
					} else {
						this->SetSize(Size(cSize));
					}
				}

				Core::Event<CheckBoxStateChangeInfo> StateChanged;
			protected:
				Core::GetSetProperty<CheckBoxType>
					Type = Core::GetSetProperty<CheckBoxType>(
						[this](CheckBoxType newType) {
							_type = newType;
							if (_type == CheckBoxType::Button) {
								if (_checkState == CheckBoxState::HalfChecked) {
									_checkState = CheckBoxState::Checked;
									OnStateChanged(CheckBoxStateChangeInfo(CheckBoxState::HalfChecked, _checkState));
								}
								this->_content.LayoutRectangle.Left = this->_actualLayout.Left;
								this->_content.HorizontalAlignment = Graphics::TextRendering::HorizontalTextAlignment::Center;
							} else {
								CalculateBoxRegion();
								this->_content.LayoutRectangle.Left = this->_actualLayout.Left + _boxSize.Width;
								this->_content.HorizontalAlignment = Graphics::TextRendering::HorizontalTextAlignment::Left;
							}
						}, [this]() {
							return _type;
						}
					);

				CheckBoxState _checkState = CheckBoxState::Unchecked;
				Size _boxSize = DefaultBoxSize;
				CheckBoxAlignment _align = CheckBoxAlignment::MiddleLeft;
				Core::Math::Rectangle _boxRegion;
				bool _threeState = false;
				CheckBoxType _type = CheckBoxType::Box;

				virtual void FinishLayoutChange() override {
					ButtonBase<T>::FinishLayoutChange();
					CalculateBoxRegion();
					if (_type == CheckBoxType::Box) {
						this->_content.LayoutRectangle.Left += _boxSize.Width;
					}
				}
				virtual void OnClick(const Core::Info&) override {
					switch (_checkState) {
						case CheckBoxState::Checked: {
							_checkState = CheckBoxState::Unchecked;
							OnStateChanged(CheckBoxStateChangeInfo(CheckBoxState::Checked, _checkState));
							break;
						}
						case CheckBoxState::Unchecked: {
							if (_type == CheckBoxType::Button) {
								_checkState = CheckBoxState::Checked;
							} else {
								_checkState = (_threeState ? CheckBoxState::HalfChecked : CheckBoxState::Checked);
							}
							OnStateChanged(CheckBoxStateChangeInfo(CheckBoxState::Unchecked, _checkState));
							break;
						}
						case CheckBoxState::HalfChecked: {
							_checkState = CheckBoxState::Checked;
							OnStateChanged(CheckBoxStateChangeInfo(CheckBoxState::HalfChecked, _checkState));
							break;
						}
					}
				}
				virtual void OnStateChanged(const CheckBoxStateChangeInfo &info) {
					StateChanged(info);
				}
				virtual void CalculateBoxRegion() {
					_boxRegion.Left = this->_actualLayout.Left;
					_boxRegion.Right = _boxRegion.Left + _boxSize.Width;
					switch (_align) {
						case CheckBoxAlignment::TopLeft: {
							_boxRegion.Top = this->_actualLayout.Top;
							break;
						}
						case CheckBoxAlignment::MiddleLeft: {
							_boxRegion.Top = (this->_actualLayout.Top + this->_actualLayout.Bottom - _boxSize.Height) * 0.5;
							break;
						}
						case CheckBoxAlignment::BottomLeft: {
							_boxRegion.Top = this->_actualLayout.Bottom - _boxSize.Height;
							break;
						}
					}
					_boxRegion.Bottom = _boxRegion.Top + _boxSize.Height;
				}
				virtual void OnBoxSizeChanged() {
					CalculateBoxRegion();
				}
				virtual void OnBoxAlignmentChanged() {
					switch (_align) {
						case CheckBoxAlignment::TopLeft: {
							this->_content.VerticalAlignment = Graphics::TextRendering::VerticalTextAlignment::Bottom;
							break;
						}
						case CheckBoxAlignment::MiddleLeft: {
							this->_content.VerticalAlignment = Graphics::TextRendering::VerticalTextAlignment::Center;
							break;
						}
						case CheckBoxAlignment::BottomLeft: {
							this->_content.VerticalAlignment = Graphics::TextRendering::VerticalTextAlignment::Top;							break;
						}
					}
					CalculateBoxRegion();
				}
		};
		template <typename T> const Size CheckBoxBase<T>::DefaultBoxSize(10.0, 10.0);

		template <typename T/* = Graphics::TextRendering:BasicText*/> class SimpleCheckBox : public CheckBoxBase<T> {
				friend class World;
			public:
				const static Graphics::SolidBrush DefaultNormalBoxBrush, DefaultHoverBoxBrush, DefaultPressedBoxBrush, DefaultCheckedBoxBrush;
				const static Graphics::Pen DefaultCheckPen;
				const static double CheckSize;

				const Graphics::Brush *&NormalBrush() {
					return _freeBkg;
				}
				const Graphics::Brush *const &NormalBrush() const {
					return _freeBkg;
				}

				const Graphics::Brush *&HoverBrush() {
					return _overBkg;
				}
				const Graphics::Brush *const &HoverBrush() const {
					return _overBkg;
				}

				const Graphics::Brush *&PressedBrush() {
					return _downBkg;
				}
				const Graphics::Brush *const &PressedBrush() const {
					return _downBkg;
				}

				const Graphics::Brush *&CheckedBrush() {
					return _checkBkg;
				}
				const Graphics::Brush *const &CheckedBrush() const {
					return _checkBkg;
				}

				const Graphics::Pen *&CheckPen() {
					return _checkPen;
				}
				const Graphics::Pen *const &CheckPen() const {
					return _checkPen;
				}

				using CheckBoxBase<T>::Type;
			protected:
				const Graphics::Brush *_freeBkg = nullptr, *_overBkg = nullptr, *_downBkg = nullptr, *_checkBkg = nullptr;
				const Graphics::Pen *_checkPen = nullptr;

				virtual void Render(Graphics::Renderer &r) override {
					Core::Math::Rectangle fillRgn = (this->_type == CheckBoxType::Box ? this->_boxRegion : this->_actualLayout);
					if (
						(((int)this->_state & (int)ButtonState::KeyboardPressed)) ||
						((int)this->_state & (int)ButtonState::MousePressed) == (int)ButtonState::MousePressed
					) {
						DrawRect(_downBkg, DefaultPressedBoxBrush, r, fillRgn);
					} else if (!((int)this->_state & (int)ButtonState::MouseOver)) {
						if (this->_type == CheckBoxType::Box || this->_checkState != CheckBoxState::Checked) {
							DrawRect(_freeBkg, DefaultNormalBoxBrush, r, fillRgn);
						} else {
							DrawRect(_checkBkg, DefaultCheckedBoxBrush, r, fillRgn);
						}
					} else {
						DrawRect(_overBkg, DefaultHoverBoxBrush, r, fillRgn);
					}
					if (this->_type == CheckBoxType::Box) {
						Core::Collections::List<Core::Math::Vector2> ls;
						DE::Core::Math::Rectangle newRect = this->_boxRegion;
						newRect.Scale(newRect.Center(), CheckSize);
						if (this->_checkState != CheckBoxState::Unchecked) {
							ls.PushBack(newRect.TopLeft());
							ls.PushBack(newRect.BottomRight());
						}
						if (this->_checkState == CheckBoxState::Checked) {
							ls.PushBack(newRect.BottomLeft());
							ls.PushBack(newRect.TopRight());
						}
						if (_checkPen) {
							_checkPen->DrawLines(ls, r);
						} else {
							DefaultCheckPen.DrawLines(ls, r);
						}
					}
					CheckBoxBase<T>::Render(r);
				}
				void DrawRect(const Graphics::Brush *b, const Graphics::Brush &fallBack, Graphics::Renderer &r, const Core::Math::Rectangle &rect) {
					if (b) {
						b->FillRect(rect, r);
					} else {
						fallBack.FillRect(rect, r);
					}
				}
		};
		template <typename T> const Graphics::SolidBrush SimpleCheckBox<T>::DefaultNormalBoxBrush(Core::Color(180, 180, 180, 255));
		template <typename T> const Graphics::SolidBrush SimpleCheckBox<T>::DefaultHoverBoxBrush(Core::Color(230, 230, 230, 255));
		template <typename T> const Graphics::SolidBrush SimpleCheckBox<T>::DefaultPressedBoxBrush(Core::Color(130, 130, 130, 255));
		template <typename T> const Graphics::SolidBrush SimpleCheckBox<T>::DefaultCheckedBoxBrush(Core::Color(80, 80, 80, 255));
		template <typename T> const Graphics::Pen SimpleCheckBox<T>::DefaultCheckPen(Core::Color(0, 0, 0, 255), 2.5);
		template <typename T> const double SimpleCheckBox<T>::CheckSize = 0.8;
	}
}
