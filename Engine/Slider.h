#pragma once

#include "ScrollBar.h"

namespace DE {
	namespace UI {
		class SliderBase : public PanelBase { // TODO override OnMouseScroll
			public:
				SliderBase() : PanelBase() {
					_button.Focusable() = false;
					_button.Bounded() = true;
					_button.DragMove += [&](const DragInfo &info) {
						OnDragMove(info);
					};
				}
				virtual ~SliderBase() {
					_disposing = true;
				}

				LayoutDirection GetLayoutDirection() const {
					return _dir;
				}
				virtual void SetLayoutDirection(LayoutDirection dir) {
					_dir = dir;
					ResetSliderButtonLayout();
				}

				double GetSliderButtonSize() const {
					return _sliderSize;
				}
				virtual void SetSliderButtonSize(double newSize) {
					if (newSize > GetSliderSize()) {
						throw Core::OverflowException(_TEXT("size overflow"));
					}
					_sliderSize = newSize;
					ResetSliderButtonLayout();
				}

				double GetValue() const {
					return _val;
				}
				virtual void SetValue(double val) {
					if (val < 0.0) {
						throw Core::UnderflowException(_TEXT("value underflow"));
					}
					if (val > _maxV) {
						throw Core::OverflowException(_TEXT("value overflow"));
					}
					_val = val;
					ResetSliderButtonLayout();
					ValueChanged(Core::Info());
				}
				virtual void JumpScroll(double delta) {
					_val += delta * _pageDelta;
					if (_val < 0.0) {
						_val = 0.0;
					}
					if (_val > _maxV) {
						_val = _maxV;
					}
					ResetSliderButtonLayout();
					ValueChanged(Core::Info());
				}

				double GetMaxValue() const {
					return _maxV;
				}
				virtual void SetMaxValue(double maxV) {
					if (_val > maxV) {
						throw Core::OverflowException(_TEXT("value overflow"));
					}
					_maxV = maxV;
					ResetSliderButtonLayout();
				}

				virtual void SetSliderParameters(double maxV, double val) {
					if (val < 0.0) {
						throw Core::UnderflowException(_TEXT("value underflow"));
					}
					if (val > maxV) {
						throw Core::OverflowException(_TEXT("value overflow"));
					}
					_val = val;
					_maxV = maxV;
					ResetSliderButtonLayout();
					ValueChanged(Core::Info());
				}

				virtual bool HitTest(const Core::Math::Vector2 &pos) const override {
					return Control::HitTest(pos);
				}

				Graphics::Brush *&IndicatorBrush() {
					return _indicatorBrush;
				}
				Graphics::Brush *const &IndicatorBrush() const {
					return _indicatorBrush;
				}
				Graphics::Pen *&IndicatorPen() {
					return _indicatorPen;
				}
				Graphics::Pen *const &IndicatorPen() const {
					return _indicatorPen;
				}
				double &IndicatorSize() {
					return _indicatorSize;
				}
				const double &IndicatorSize() const {
					return _indicatorSize;
				}

				bool &JumpToClickPosition() {
					return _jumpClick;
				}
				const bool &JumpToClickPosition() const {
					return _jumpClick;
				}
				double &JumpDelta() {
					return _pageDelta;
				}
				const double &JumpDelta() const {
					return _pageDelta;
				}

				Core::Event<Core::Info> ValueChanged;
			protected:
				SimpleScrollBarButton _button;
				LayoutDirection _dir = LayoutDirection::Horizontal;
				double _maxV = 1.0, _sliderSize = 10.0, _val = 0.0, _indicatorSize = 2.0, _pageDelta = 0.2;
				Graphics::Brush *_indicatorBrush = nullptr;
				Graphics::Pen *_indicatorPen = nullptr;
				bool _jumpClick = false;

				virtual void Initialize() override {
					PanelBase::Initialize();
					_col.Insert(_button);
				}

				virtual void Render(Graphics::Renderer &r) override {
					Core::Math::Rectangle indicator = GetIndicator();
					if (_indicatorBrush) {
						_indicatorBrush->FillRect(indicator, r);
					}
					if (_indicatorPen) {
						_indicatorPen->DrawRectangle(indicator, r);
					}
					PanelBase::Render(r);
				}

				virtual void OnDragMove(const DragInfo &info) {
					_val = _maxV * (GetSliderButtonPosition() / (GetSliderSize() - _sliderSize));
					ValueChanged(Core::Info());
				}

				virtual bool OnMouseDown(const Core::Input::MouseButtonInfo &info) override {
					bool res = PanelBase::OnMouseDown(info);
					if (!_button.IsMouseOver()) {
						if (_jumpClick) {
							double posV = (
								_dir == LayoutDirection::Horizontal ?
								(info.Position->X - _actualLayout.Left - _sliderSize * 0.5) / (actualSize.Width - _sliderSize) :
								(info.Position->Y - _actualLayout.Top - _sliderSize * 0.5) / (actualSize.Height - _sliderSize)
							) * _maxV;
							if (posV < 0.0) {
								posV = 0.0;
							}
							if (posV > _maxV) {
								posV = _maxV;
							}
							SetValue(posV);
							_button.StartDrag();
						} else {
							if (
								_dir == LayoutDirection::Horizontal ?
								info.Position->X > _button.GetActualLayout().CenterX() :
								info.Position->Y > _button.GetActualLayout().CenterY()
							) {
								JumpScroll(1.0);
							} else {
								JumpScroll(-1.0);
							}
						}
					}
					return res;
				}

				virtual Core::Math::Rectangle GetIndicator() const {
					return (
						_dir == LayoutDirection::Horizontal ?
						Core::Math::Rectangle(
							_actualLayout.Left + _sliderSize * 0.5,
							_actualLayout.CenterY() - _indicatorSize * 0.5,
							GetSliderSize() - _sliderSize,
							_indicatorSize
						) :
						Core::Math::Rectangle(
							_actualLayout.CenterX() - _indicatorSize * 0.5,
							_actualLayout.Top + _sliderSize * 0.5,
							_indicatorSize,
							GetSliderSize() - _sliderSize
						)
					);
				}
				virtual double GetSliderSize() const {
					return (
						_dir == LayoutDirection::Horizontal ?
						actualSize.Width :
						actualSize.Height
					);
				}
				virtual Size GetCompleteSliderButtonSize() const {
					return (
						_dir == LayoutDirection::Horizontal ?
						Size(_sliderSize, actualSize.Height) :
						Size(actualSize.Width, _sliderSize)
					);
				}
				virtual Thickness GetSliderButtonMargins() const {
					return (
						_dir == LayoutDirection::Horizontal ?
						Thickness((actualSize.Width - _sliderSize) * _val / _maxV, 0.0, 0.0, 0.0) :
						Thickness(0.0, (actualSize.Width - _sliderSize) * _val / _maxV, 0.0, 0.0)
					);
				}
				virtual double GetSliderButtonPosition() const {
					return (
						_dir == LayoutDirection::Horizontal ?
						_button.GetMargins().Left :
						_button.GetMargins().Top
					);
				}
				virtual void ResetSliderButtonLayout() {
					_button.SetSize(GetCompleteSliderButtonSize());
					_button.SetMargins(GetSliderButtonMargins());
				}
				virtual void FinishLayoutChange() override {
					_button.Bounds() = Core::Math::Rectangle(_actualLayout.Size());
					ResetSliderButtonLayout();
					PanelBase::FinishLayoutChange();
				}
		};
	}
}
