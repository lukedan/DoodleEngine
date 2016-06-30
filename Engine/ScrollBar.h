#pragma once

#include "Panel.h"
#include "Button.h"

namespace DE {
	namespace UI {
		enum class DragMode {
			Fixed,
			Remote,
			RemoteLocally
		};
		struct DragInfo {
			public:
				DragInfo() = default;
				DragInfo(
					const Core::Math::Vector2 &newPosition,
					const Core::Math::Vector2 &oldPosition
				) : newPos(newPosition), oldPos(oldPosition) {
				}

				const Core::Math::Vector2 &GetNewPosition() const {
					return newPos;
				}
				const Core::Math::Vector2 &GetOldPosition() const {
					return oldPos;
				}
			private:
				Core::Math::Vector2 newPos, oldPos;
		};
		class DraggableControlBase : public Control {
				friend class World;
			public:
				DraggableControlBase() : Control() {
					SetAnchor(Anchor::TopLeft);
					SetMargins(Thickness());
				}

				Core::Math::Vector2 GetPosition() const {
					return Core::Math::Vector2(_margin.Left, _margin.Top);
				}
				void SetPosition(const Core::Math::Vector2 &pos) {
					SetMargins(Thickness(pos.X, pos.Y, 0.0, 0.0));
				}

				void StartDrag() {
					dragging = true;
					dragOffset = GetRelativeMousePosition();
				}
				void StopDrag() {
					dragging = false;
				}
				bool IsDragging() const {
					return dragging;
				}

				bool &Bounded() {
					return bounded;
				}
				const bool &Bounded() const {
					return bounded;
				}
				Core::Math::Rectangle &Bounds() {
					return bounds;
				}
				const Core::Math::Rectangle &Bounds() const {
					return bounds;
				}
				DragMode &Mode() {
					return mode;
				}
				const DragMode &Mode() const {
					return mode;
				}

				Core::Event<DragInfo> DragMove;
			protected:
				bool dragging = false, bounded = false;
				Core::Math::Vector2 dragOffset;
				Core::Math::Rectangle bounds;
				DragMode mode = DragMode::Fixed;

				virtual void FinishLayoutChange() override {
					Control::FinishLayoutChange();
					if (_anchor != Anchor::TopLeft) {
						throw Core::InvalidArgumentException(_TEXT("the DraggableControl must be anchored to top left"));
					}
				}
				virtual void Update(double) override {
					if (dragging) {
						Core::Math::Vector2 targetPos, mouse = GetWorld()->GetRelativeMousePosition();
						if (_father) {
							mouse -= _father->GetActualLayout().TopLeft();
						}
						targetPos = mouse - dragOffset;
						if (bounded) {
							bool nc = false;
							Core::Math::Vector2 rightBottom = targetPos + GetSize();
							if (rightBottom.X > bounds.Right) {
								targetPos.X -= (rightBottom.X - bounds.Right);
								nc = true;
							}
							if (rightBottom.Y > bounds.Bottom) {
								targetPos.Y -= (rightBottom.Y - bounds.Bottom);
								nc = true;
							}
							if (targetPos.X < bounds.Left) {
								targetPos.X = bounds.Left;
								nc = true;
							}
							if (targetPos.Y < bounds.Top) {
								targetPos.Y = bounds.Top;
								nc = true;
							}
							if (nc) {
								switch (mode) {
									case DragMode::Fixed: {
										break;
									}
									case DragMode::Remote: {
										dragOffset = mouse - targetPos;
										break;
									}
									case DragMode::RemoteLocally: {
										if (IsMouseOver()) {
											dragOffset = mouse - targetPos;
										}
										break;
									}
								}
							}
						}
						Core::Math::Vector2 oldP = GetPosition();
						SetPosition(targetPos);
						DragMove(DragInfo(targetPos, oldP));
					}
				}
		};
		class SimpleScrollBarButton : public DraggableControlBase {
				friend class World;
			public:
				const Graphics::Brush *&NormalBrush() {
					return freeBrush;
				}
				const Graphics::Brush *const &NormalBrush() const {
					return freeBrush;
				}
				const Graphics::Brush *&PressedBrush() {
					return downBrush;
				}
				const Graphics::Brush *const &PressedBrush() const {
					return downBrush;
				}
				const Graphics::Brush *&HoverBrush() {
					return moverBrush;
				}
				const Graphics::Brush *const &HoverBrush() const {
					return moverBrush;
				}

				virtual bool OnMouseDown(const Core::Input::MouseButtonInfo &info) override {
					bool res = DraggableControlBase::OnMouseDown(info);
					if (info.Button == Core::Input::MouseButton::Left) {
						StartDrag();
					}
					return res;
				}

				virtual void Update(double dt) override {
					if (dragging && !Core::IsKeyDown(VK_LBUTTON)) {
						StopDrag();
					}
					DraggableControlBase::Update(dt);
				}
			protected:
				const Graphics::Brush *freeBrush = nullptr, *moverBrush = nullptr, *downBrush = nullptr;

				virtual void Render(Graphics::Renderer &r) override {
					if (dragging) {
						DrawRect(downBrush, SimpleButton<Graphics::TextRendering::BasicText>::DefaultPressedBrush, r);
					} else if (IsMouseOver()) {
						DrawRect(moverBrush, SimpleButton<Graphics::TextRendering::BasicText>::DefaultHoverBrush, r);
					} else {
						DrawRect(freeBrush, SimpleButton<Graphics::TextRendering::BasicText>::DefaultNormalBrush, r);
					}
				}

				void DrawRect(const Graphics::Brush *b, const Graphics::Brush &fallBack, Graphics::Renderer &r) {
					if (b) {
						b->FillRect(_actualLayout, r);
					} else {
						fallBack.FillRect(_actualLayout, r);
					}
				}
		};

		struct ScrollInfo {
			public:
				ScrollInfo() = default;
				ScrollInfo(double newV, double dt) : newVal(newV), delta(dt) {
				}

				double GetNewValue() const {
					return newVal;
				}
				double GetDelta() const {
					return delta;
				}
				double GetOldValue() const {
					return newVal - delta;
				}
			private:
				double newVal = 0.0, delta = 0.0;
		};
		class ScrollBarBase : public PanelBase {
				friend class World;
			public:
				ScrollBarBase() : PanelBase() {
					_focusable = false;

					_drag.Focusable() = false;
					_drag.Bounded() = true;
					_drag.DragMove += [this](const DragInfo &info) { OnDragMove(info); };

					_upB.Click += [this](const Core::Info &info) { OnPageUpButtonClicked(info); };
					_upB.Focusable() = false;
					_upB.NormalBrush = &DefaultPageButtonNormalBrush;
					_upB.PressedBrush = &DefaultPageButtonPressedBrush;
					_upB.HoverBrush = &DefaultPageButtonHoverBrush;

					_downB.Click += [this](const Core::Info &info) { OnPageDownButtonClicked(info); };
					_downB.Focusable() = false;
					_downB.NormalBrush = &DefaultPageButtonNormalBrush;
					_downB.PressedBrush = &DefaultPageButtonPressedBrush;
					_downB.HoverBrush = &DefaultPageButtonHoverBrush;
				}

				double GetValue() const {
					return _cur;
				}
				void SetValue(double v) {
					if (v < 0.0) {
						v = 0.0;
					}
					if (v + _range > _maxV) {
						v = _maxV - _range;
					}
					if (_dir == LayoutDirection::Vertical) {
						_drag.SetPosition(Core::Math::Vector2(0.0, _actualLayout.Height() * v / _maxV));
					} else {
						_drag.SetPosition(Core::Math::Vector2(_actualLayout.Width() * v / _maxV, 0.0));
					}
					_cur = v;
					ResetLayout();
				}
				double GetViewRange() const {
					return _range;
				}
				void SetViewRange(double r) {
					if (r <= 0.0) {
						throw Core::InvalidArgumentException(_TEXT("the range is empty"));
					}
					if (r > _maxV) {
						r = _maxV;
					}
					if (_cur + r > _maxV) {
						_cur = _maxV - r;
					}
					_range = r;
					ResetLayout();
				}
				double GetMaxValue() const {
					return _maxV;
				}
				void SetMaxValue(double val) {
					if (_range < val) {
						_range = val;
					}
					if (_cur + _range > val) {
						_cur = val - _range;
					}
					_maxV = val;
					ResetLayout();
				}
				void SetScrollBarProperties(double maxv, double range, double value) {
					if (value < -Core::Math::Epsilon) {
						throw Core::InvalidArgumentException(_TEXT("value underflow"));
					}
					if (value + range > maxv + Core::Math::Epsilon) {
						throw Core::InvalidArgumentException(_TEXT("value overflow"));
					}
					_maxV = maxv;
					_range = range;
					if (_dir == LayoutDirection::Vertical) {
						_drag.SetPosition(Core::Math::Vector2(0.0, _actualLayout.Height() * value / _maxV));
					} else {
						_drag.SetPosition(Core::Math::Vector2(_actualLayout.Width() * value / _maxV, 0.0));
					}
					_cur = value;
					ResetLayout();
				}

				LayoutDirection GetLayoutDirection() const {
					return _dir;
				}
				void SetLayoutDirection(LayoutDirection newDir) {
					_dir = newDir;
					ResetLayout();
				}

				double &PageDelta() {
					return _pageDelta;
				}
				const double &PageDelta() const {
					return _pageDelta;
				}

				double &StepDistanceRatio() {
					return _stepRatio;
				}
				const double &StepDistanceRatio() const {
					return _stepRatio;
				}

				void ScrollPage(int delta) {
					SetValue(Core::Math::Min(_maxV - _range, Core::Math::Max(0.0, _cur + _pageDelta * delta)));
					Scroll(ScrollInfo(_cur, _pageDelta));
				}
				void ScrollLine(int delta) {
					SetValue(Core::Math::Min(_maxV - _range, Core::Math::Max(0.0, _cur + _stepRatio * _pageDelta * delta)));
					Scroll(ScrollInfo(_cur, _pageDelta));
				}

				virtual bool OnMouseScroll(const Core::Input::MouseScrollInfo &info) override {
					return HandleMouseScroll(info);
				}
				virtual bool HandleMouseScroll(const Core::Input::MouseScrollInfo &info) {
					double ocur = _cur;
					_cur -= info.Delta * _pageDelta * _stepRatio;
					_cur = Core::Math::Min(_cur, _maxV - _range);
					_cur = Core::Math::Max(_cur, 0.0);
					double dt = _cur - ocur;
					if (Core::Math::Abs(dt) <= Core::Math::Epsilon) {
						return false;
					}
					SetValue(_cur);
					Scroll(ScrollInfo(_cur, dt));
					return true;
				}

				// TODO add properties
//				const Graphics::Brush *&UpPageButtonNormalBrush() {
//					return _upB.NormalBrush();
//				}
//				const Graphics::Brush *const &UpPageButtonNormalBrush() const {
//					return _upB.NormalBrush();
//				}
//				const Graphics::Brush *&UpPageButtonHoverBrush() {
//					return _upB.HoverBrush();
//				}
//				const Graphics::Brush *const &UpPageButtonHoverBrush() const {
//					return _upB.HoverBrush();
//				}
//				const Graphics::Brush *&UpPageButtonPressedBrush() {
//					return _upB.PressedBrush();
//				}
//				const Graphics::Brush *const &UpPageButtonPressedBrush() const {
//					return _upB.PressedBrush();
//				}
//
//				const Graphics::Brush *&DownPageButtonNormalBrush() {
//					return _downB.NormalBrush();
//				}
//				const Graphics::Brush *const &DownPageButtonNormalBrush() const {
//					return _downB.NormalBrush();
//				}
//				const Graphics::Brush *&DownPageButtonHoverBrush() {
//					return _downB.HoverBrush();
//				}
//				const Graphics::Brush *const &DownPageButtonHoverBrush() const {
//					return _downB.HoverBrush();
//				}
//				const Graphics::Brush *&DownPageButtonPressedBrush() {
//					return _downB.PressedBrush();
//				}
//				const Graphics::Brush *const &DownPageButtonPressedBrush() const {
//					return _downB.PressedBrush();
//				}

				const static Graphics::SolidBrush
					DefaultPageButtonNormalBrush,
					DefaultPageButtonHoverBrush,
					DefaultPageButtonPressedBrush;
				constexpr static double DefaultPageDelta = 0.2, DefaultStepDistanceRatio = 0.1, DefaultWidth = 15.0;

				Core::Event<ScrollInfo> Scroll;
			protected:
				SimpleScrollBarButton _drag;
				SimpleButton<Graphics::TextRendering::BasicText> _upB, _downB;
				double _cur = 0.0, _range = 0.2, _maxV = 1.0, _pageDelta = DefaultPageDelta, _stepRatio = DefaultStepDistanceRatio;
				LayoutDirection _dir = LayoutDirection::Vertical;

				virtual void Initialize() override {
					PanelBase::Initialize();
					_col.Insert(_drag);
					_col.Insert(_upB);
					_col.Insert(_downB);
				}

				virtual void OnPageUpButtonClicked(const Core::Info&) {
					ScrollPage(-1);
				}
				virtual void OnPageDownButtonClicked(const Core::Info&) {
					ScrollPage(1);
				}
				virtual void OnDragMove(const DragInfo &info) {
					double oc = _cur;
					_cur = (_dir == LayoutDirection::Vertical ?
						info.GetNewPosition().Y / _actualLayout.Height() :
						info.GetNewPosition().X / _actualLayout.Width()) * _maxV;
					Scroll(ScrollInfo(_cur, _cur - oc));
					ResetButtons();
				}

				virtual void FinishLayoutChange() override {
					if (_dir == LayoutDirection::Vertical) {
						_drag.SetSize(Size(_actualLayout.Width(), _actualLayout.Height() * (_range / _maxV)));
						_upB.SetAnchor(Anchor::TopDock);
						_downB.SetAnchor(Anchor::BottomDock);
					} else {
						_drag.SetSize(Size(_actualLayout.Width() * (_range / _maxV), _actualLayout.Height()));
						_upB.SetAnchor(Anchor::LeftDock);
						_downB.SetAnchor(Anchor::RightDock);
					}
					_drag.Bounds() = Core::Math::Rectangle(_actualLayout.BottomRight() - _actualLayout.TopLeft());
					ResetButtons();
					PanelBase::FinishLayoutChange();
				}

				void ResetButtons() {
					if (_dir == LayoutDirection::Vertical) {
						_upB.SetSize(Size(_actualLayout.Width(), _drag.GetActualMargins().Top));
						_downB.SetSize(Size(_actualLayout.Width(), _drag.GetActualMargins().Bottom));
					} else {
						_upB.SetSize(Size(_drag.GetActualMargins().Left, _actualLayout.Height()));
						_downB.SetSize(Size(_drag.GetActualMargins().Right, _actualLayout.Height()));
					}
				}
		};
	}
}
