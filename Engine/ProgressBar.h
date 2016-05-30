#pragma once

#include "Control.h"

namespace DE {
	namespace UI {
		class SimpleProgressBar : public Control {
				friend class World;
			public:
				SimpleProgressBar() : Control() {
					_focusable = false;
				}
				virtual ~SimpleProgressBar() {
				}

				double GetProgress() const {
					return _progress;
				}
				void SetProgress(double p) {
					if (p < 0.0) {
						throw Core::UnderflowException(_TEXT("progress underflow"));
					}
					if (p > _maxProgress) {
						throw Core::OverflowException(_TEXT("progress overflow"));
					}
					_progress = p;
					ResetLayout();
				}

				double GetMaxProgress() const {
					return _maxProgress;
				}
				void SetMaxProgress(double maxProg) {
					if (maxProg < _progress) {
						throw Core::OverflowException(_TEXT("progress overflow"));
					}
					_maxProgress = maxProg;
					ResetLayout();
				}

				const Graphics::Brush *&FinishedBrush() {
					return _finishedBrush;
				}
				const Graphics::Brush *const &FinishedBrush() const {
					return _finishedBrush;
				}
				const Graphics::Brush *&UnfinishedBrush() {
					return _unfinishedBrush;
				}
				const Graphics::Brush *const &UnfinishedBrush() const {
					return _unfinishedBrush;
				}

				LayoutDirection GetLayoutDirection() const {
					return _dir;
				}
				void SetLayoutDirection(LayoutDirection newDir) {
					_dir = newDir;
					ResetLayout();
				}

				const static Graphics::SolidBrush DefaultFinishedBrush, DefaultUnfinishedBrush;
			protected:
				double _progress = 0.0, _maxProgress = 1.0;
				const Graphics::Brush *_finishedBrush = nullptr, *_unfinishedBrush = nullptr;
				Core::Math::Rectangle _finR, _unfR;
				LayoutDirection _dir = LayoutDirection::Horizontal;

				virtual void FinishLayoutChange() {
					Control::FinishLayoutChange();
					_finR.Left = _actualLayout.Left;
					_finR.Bottom = _actualLayout.Bottom;
					_unfR.Right = _actualLayout.Right;
					_unfR.Top = _actualLayout.Top;
					if (_dir == LayoutDirection::Horizontal) {
						_finR.Top = _actualLayout.Top;
						_unfR.Bottom = _actualLayout.Bottom;
						_unfR.Left = _finR.Right = _finR.Left + (_progress / _maxProgress) * _actualLayout.Width();
					} else {
						_finR.Right = _actualLayout.Right;
						_unfR.Left = _actualLayout.Left;
						_unfR.Bottom = _finR.Top = _finR.Bottom - (_progress / _maxProgress) * _actualLayout.Height();
					}
				}

				virtual void Update(double) {
				}
				virtual void Render(Graphics::Renderer &r) {
					Control::Render(r);
					DrawRect(r, _finR, _finishedBrush, DefaultFinishedBrush);
					DrawRect(r, _unfR, _unfinishedBrush, DefaultUnfinishedBrush);
				}

				void DrawRect(
					Graphics::Renderer &r,
					const Core::Math::Rectangle &rect,
					const Graphics::Brush *brush,
					const Graphics::Brush &fallBack
				) {
					if (brush) {
						brush->FillRect(rect, r);
					} else {
						fallBack.FillRect(rect, r);
					}
				}
		};
	}
}