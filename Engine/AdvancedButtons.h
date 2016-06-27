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
		class CheckBoxBase : public ButtonBase {
				friend class World;
			public:
				const static Size DefaultBoxSize;

				CheckBoxBase() : ButtonBase() {
					_content.HorizontalAlignment = Graphics::TextRendering::HorizontalTextAlignment::Left;
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
					Core::Math::Vector2 cSize = _content.GetSize();
					if (_type == CheckBoxType::Box) {
						SetSize(Size(cSize.X + _boxSize.Width, Core::Math::Max(cSize.Y, _boxSize.Height)));
					} else {
						SetSize(Size(cSize));
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
								_content.LayoutRectangle.Left = _actualLayout.Left;
								_content.HorizontalAlignment = Graphics::TextRendering::HorizontalTextAlignment::Center;
							} else {
								CalculateBoxRegion();
								_content.LayoutRectangle.Left = _actualLayout.Left + _boxSize.Width;
								_content.HorizontalAlignment = Graphics::TextRendering::HorizontalTextAlignment::Left;
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
					ButtonBase::FinishLayoutChange();
					CalculateBoxRegion();
					if (_type == CheckBoxType::Box) {
						_content.LayoutRectangle.Left += _boxSize.Width;
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
					_boxRegion.Left = _actualLayout.Left;
					_boxRegion.Right = _boxRegion.Left + _boxSize.Width;
					switch (_align) {
						case CheckBoxAlignment::TopLeft: {
							_boxRegion.Top = _actualLayout.Top;
							break;
						}
						case CheckBoxAlignment::MiddleLeft: {
							_boxRegion.Top = (_actualLayout.Top + _actualLayout.Bottom - _boxSize.Height) / 2.0;
							break;
						}
						case CheckBoxAlignment::BottomLeft: {
							_boxRegion.Top = _actualLayout.Bottom - _boxSize.Height;
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
							_content.VerticalAlignment = Graphics::TextRendering::VerticalTextAlignment::Bottom;
							break;
						}
						case CheckBoxAlignment::MiddleLeft: {
							_content.VerticalAlignment = Graphics::TextRendering::VerticalTextAlignment::Center;
							break;
						}
						case CheckBoxAlignment::BottomLeft: {
							_content.VerticalAlignment = Graphics::TextRendering::VerticalTextAlignment::Top;							break;
						}
					}
					CalculateBoxRegion();
				}
		};
		class SimpleCheckBox : public CheckBoxBase {
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

				using CheckBoxBase::Type;
			protected:
				const Graphics::Brush *_freeBkg = nullptr, *_overBkg = nullptr, *_downBkg = nullptr, *_checkBkg = nullptr;
				const Graphics::Pen *_checkPen = nullptr;

				virtual void Render(Graphics::Renderer &r) override {
					Core::Math::Rectangle fillRgn = (_type == CheckBoxType::Box ? _boxRegion : _actualLayout);
					if (
						(((int)_state & (int)ButtonState::KeyboardPressed)) ||
						((int)_state & (int)ButtonState::MousePressed) == (int)ButtonState::MousePressed
					) {
						DrawRect(_downBkg, DefaultPressedBoxBrush, r, fillRgn);
					} else if (!((int)_state & (int)ButtonState::MouseOver)) {
						if (_type == CheckBoxType::Box || _checkState != CheckBoxState::Checked) {
							DrawRect(_freeBkg, DefaultNormalBoxBrush, r, fillRgn);
						} else {
							DrawRect(_checkBkg, DefaultCheckedBoxBrush, r, fillRgn);
						}
					} else {
						DrawRect(_overBkg, DefaultHoverBoxBrush, r, fillRgn);
					}
					if (_type == CheckBoxType::Box) {
						Core::Collections::List<Core::Math::Vector2> ls;
						DE::Core::Math::Rectangle newRect = _boxRegion;
						newRect.Scale(newRect.Center(), CheckSize);
						if (_checkState != CheckBoxState::Unchecked) {
							ls.PushBack(newRect.TopLeft());
							ls.PushBack(newRect.BottomRight());
						}
						if (_checkState == CheckBoxState::Checked) {
							ls.PushBack(newRect.BottomLeft());
							ls.PushBack(newRect.TopRight());
						}
						if (_checkPen) {
							_checkPen->DrawLines(ls, r);
						} else {
							DefaultCheckPen.DrawLines(ls, r);
						}
					}
					CheckBoxBase::Render(r);
				}
				void DrawRect(const Graphics::Brush *b, const Graphics::Brush &fallBack, Graphics::Renderer &r, const Core::Math::Rectangle &rect) {
					if (b) {
						b->FillRect(rect, r);
					} else {
						fallBack.FillRect(rect, r);
					}
				}
		};
	}
}
