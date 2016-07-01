#pragma once

#include "Label.h"
#include "ScrollView.h"
#include "BrushAndPen.h"

namespace DE {
	namespace UI {
		// testing
		template <typename T = Graphics::TextRendering::BasicText> class TextBox : public ScrollViewBase {
			protected:
				enum class CaretMoveType {
					None = 0,
					SetBaseLine = 1,
					CancelSelection = 2,
					SetBaseLineAndCancelSelection = 3,
					ExtendSelection = 4,
					SetBaseLineAndExtendSelection = 5
				};
			public:
				const static Graphics::Pen DefaultCaretPen;
				const static Graphics::SolidBrush DefaultSelectionBrush, DefaultTextBoxBackground;
				const static Graphics::Pen DefaultTextBoxBorder;
				constexpr static double DefaultCursorPeriod = 1.0;

				TextBox() {
					_lbl.SetCursor(Core::Input::Cursor(Core::Input::DefaultCursorType::IBeam));
				}
				~TextBox() {
					_disposing = true;
				}

				T &Text() {
					return _lbl.Content();
				}
				const T &Text() const {
					return _lbl.Content();
				}
				void SetText(const Core::String &text) {
					_lbl.Content().Content = text;
					_lbl.FitContent();
					SetCaretPositionInfo(0, CaretMoveType::SetBaseLineAndCancelSelection);
					MakePointInView(Core::Math::Vector2());
					OnTextChanged(Core::Info());
				}

				const Graphics::Pen *&CaretPen() {
					return _caretPen;
				}
				const Graphics::Pen *const &CaretPen() const {
					return _caretPen;
				}

				const Graphics::Brush *&SelectionBrush() {
					return _selectionBrush;
				}
				const Graphics::Brush *const &SelectionBrush() const {
					return _selectionBrush;
				}

				virtual void MakeCaretInView() {
					Core::Math::Rectangle caret = _lbl.Content().GetCaretInfo(_caret, _cbase + _lbl.Content().LayoutRectangle.Left);
					caret.Translate(-_lbl.GetActualLayout().TopLeft());
					if (_insert) {
						MakePointInView(caret.BottomLeft());
					} else {
						MakePointInView(caret.BottomRight());
					}
					MakePointInView(caret.TopLeft());
					_blink = 0.0;
				}
				virtual void MoveCaret(size_t newPosition) {
					if (newPosition > _lbl.Content().Content.Length()) {
						throw Core::InvalidArgumentException(_TEXT("index overflow"));
					}
					SetCaretPositionInfo(newPosition, CaretMoveType::SetBaseLineAndCancelSelection);
				}
				size_t GetCaretPosition() const {
					return _caret;
				}

				bool &ReadOnly() {
					return _readOnly;
				}
				const bool &ReadOnly() const {
					return _readOnly;
				}

				virtual Core::Input::Cursor GetDefaultCursor() const override {
					return Core::Input::Cursor(Core::Input::DefaultCursorType::IBeam);
				}

				virtual const Graphics::Brush *GetDefaultBackground() const override {
					return &DefaultTextBoxBackground;
				}
				virtual const Graphics::Pen *GetDefaultBorder() const override {
					return &DefaultTextBoxBorder;
				}

				void SetDesiredVisibleLine(size_t lines) {
					SetSize(Size(GetSize().Width, _lbl.Content().Padding.Height() + lines * _lbl.Content().Font->GetHeight()));
				}

				using ScrollViewBase::SetHorizontalScrollBarVisibility;
				using ScrollViewBase::SetVerticalScrollBarVisibility;

				Core::Event<Core::Info> TextChanged;

				Core::GetSetProperty<bool> MultiLine {
					[this](bool v) {
						_multiLine = v;
						if (!_multiLine) {
							size_t tarC = _caret;
							Core::String newContent = _TEXT("");
							for (size_t i = 0; i < _lbl.Content().Content.Length(); ++i) {
								TCHAR curChar = _lbl.Content().Content[i];
								if (curChar == _TEXT('\n')) {
									if (tarC > i) {
										--tarC;
									}
								} else {
									newContent += curChar;
								}
							}
							_lbl.Content().Content = newContent;
							_lbl.FitContent();
							SetCaretPositionInfo(tarC, CaretMoveType::SetBaseLineAndCancelSelection);
							OnTextChanged(Core::Info());
						}
					}, [this]() {
						return _multiLine;
					}
				};
				Core::GetSetProperty<bool> WrapText {
					[this](bool v) {
						if (_wrapText != v) {
							_wrapText = v;
							if (!_wrapText) {
								_lbl.Content().WrapType = Graphics::TextRendering::LineWrapType::NoWrap;
								_lbl.FitContent();
							} else {
								_lbl.Content().WrapType = Graphics::TextRendering::LineWrapType::WrapWordsNoOverflow;
							}
							ResetLayout();
						}
					},
					[this]() {
						return _wrapText;
					}
				};
			protected:
				class LabelWrapper : public Label<T> {
					friend class TextBox;
				};

				size_t _caret = 0, _selectS = 0, _selectE = 0;
				LabelWrapper _lbl;
				const Graphics::Pen *_caretPen = nullptr;
				const Graphics::Brush *_selectionBrush = nullptr;
				double _blink = 0.0, _period = DefaultCursorPeriod, _cbase = 0.0;
				bool _selecting = false, _readOnly = false, _insert = true, _multiLine = true, _wrapText = false;

				CaretMoveType GetMoveTypeWithShift(bool setBaseLine) const {
					return (CaretMoveType)(
						(int)(setBaseLine ? CaretMoveType::SetBaseLine : CaretMoveType::None) |
						(int)(Core::IsKeyDown(VK_SHIFT) ? CaretMoveType::ExtendSelection : CaretMoveType::CancelSelection)
					);
				}

				virtual void Initialize() override {
					ScrollViewBase::Initialize();
					SetChild(&_lbl);
				}

				virtual void SetCaretPositionInfo(size_t newPosition, CaretMoveType type, double baseline) {
					_caret = newPosition;
					if ((int)type & (int)CaretMoveType::SetBaseLine) {
						_cbase = baseline;
					}
					if ((int)type & (int)CaretMoveType::CancelSelection) {
						_selectS = _selectE = _caret;
					} else if ((int)type & (int)CaretMoveType::ExtendSelection) {
						_selectE = _caret;
					}
					MakeCaretInView();
				}
				virtual void SetCaretPositionInfo(size_t newPosition, CaretMoveType type) {
					SetCaretPositionInfo(newPosition, type, _lbl.Content().GetCaretPosition(newPosition).X - _lbl.Content().LayoutRectangle.Left);
				}

				virtual void Update(double dt) override {
					ScrollViewBase::Update(dt);
					_blink += dt;
					while (_blink > _period) {
						_blink -= _period;
					}
					if (_selecting) {
						if (GetWorld()) {
							size_t caretP = _lbl.Content().HitTestForCaret(
								GetWorld()->GetRelativeMousePosition() + GetWorld()->GetBounds().TopLeft()
							);
							SetCaretPositionInfo(caretP, CaretMoveType::SetBaseLineAndExtendSelection);
						}
						if (!Core::IsKeyDown(VK_LBUTTON)) {
							_selecting = false;
						}
					}
				}
				virtual void RenderChild(Graphics::Renderer &r, Control *c) override {
					ScrollViewBase::RenderChild(r, c);
					if (Focused() && _blink < _period * 0.5) {
						Core::Collections::List<Core::Math::Vector2> caret;
						Core::Math::Rectangle crect = _lbl.Content().GetCaretInfo(_caret, _cbase + _lbl.Content().LayoutRectangle.Left);
						caret.PushBack(crect.BottomLeft());
						if (_insert) {
							caret.PushBack(crect.TopLeft());
						} else {
							caret.PushBack(crect.BottomRight());
							caret.PushBack(crect.BottomRight());
							caret.PushBack(crect.TopRight());
							caret.PushBack(crect.TopRight());
							caret.PushBack(crect.TopLeft());
							caret.PushBack(crect.TopLeft());
							caret.PushBack(crect.BottomLeft());
						}
						if (_caretPen) {
							_caretPen->DrawLines(caret, r);
						} else {
							DefaultCaretPen.DrawLines(caret, r);
						}
					}
					size_t rss = _selectS, rse = _selectE;
					if (rss > rse) {
						Core::Math::Swap(rss, rse);
					}
					const Graphics::Brush *brush = (_selectionBrush ? _selectionBrush : &DefaultSelectionBrush);
					_lbl.Content().GetSelectionRegion(rss, rse).ForEach([&](const Core::Math::Rectangle &rect) {
						brush->FillRect(rect, r);
						return true;
					});
				}

				void ResetChildrenLayout() override {
					if (!_ssbs) {
						if (_wrapText) { // make sure the width of the label doesn't exceed the width of visible area
							if (GetWorld()) {
								_lbl.Content().LayoutRectangle = _actualLayout;
								double y = _lbl.Content().GetSize().Y;
								if (y > _actualLayout.Height()) {
									_lbl.Content().LayoutRectangle.Right -= _vert.GetActualSize().Width;
									y = _lbl.Content().GetSize().Y;
								}
								_lbl._size = Size(_lbl.Content().LayoutRectangle.Width(), y);
							}
						}
						ScrollViewBase::ResetChildrenLayout();
					}
				}

				virtual bool DeleteSelectedBlock() {
					if (_readOnly || _selectS == _selectE) {
						return false;
					}
					size_t rrs = _selectS, rre = _selectE;
					if (rrs > rre) {
						Core::Math::Swap(rrs, rre);
					}
					_lbl.Content().Content.Remove(rrs, rre - rrs);
					_lbl.FitContent();
					SetCaretPositionInfo(rrs, CaretMoveType::SetBaseLineAndCancelSelection);
					OnTextChanged(Core::Info());
					return true;
				}
				virtual bool HitTest(const Core::Math::Vector2 &pos) const override {
					return Control::HitTest(pos);
				}

				virtual void OnGotFocus(const Core::Info &info) override {
					ScrollViewBase::OnGotFocus(info);
					_blink = 0.0;
				}
				virtual bool OnMouseDown(const Core::Input::MouseButtonInfo &info) override {
					bool res = ScrollViewBase::OnMouseDown(info);
					if (Core::Math::Rectangle::Intersect(_visibleRgn, *info.Position) != Core::Math::IntersectionType::None) {
						switch (*info.Button) {
							case Core::Input::MouseButton::Left: {
								size_t caretP = _lbl.Content().HitTestForCaret(info.Position);
								SetCaretPositionInfo(caretP, GetMoveTypeWithShift(true));
								_selecting = true;
								break;
							}
							case Core::Input::MouseButton::Right:
							case Core::Input::MouseButton::Middle: {
								// nothing to do, just to disable the stupid warnings
								break;
							}
						}
					}
					return res;
				}
				virtual void OnMouseMove(const Core::Input::MouseMoveInfo &info) override {
					ScrollViewBase::OnMouseMove(info);
					if (_selecting) {
						size_t caretP = _lbl.Content().HitTestForCaret(info.Position);
						_selectE = caretP;
					}
				}
				virtual void OnKeyDown(const Core::Input::KeyInfo &info) override {
					ScrollViewBase::OnKeyDown(info);
					_blink = 0.0;
					switch (info.GetKey()) {
						case VK_LEFT: {
							if (_caret > 0) {
								SetCaretPositionInfo(_caret - 1, GetMoveTypeWithShift(true));
							}
							break;
						}
						case VK_RIGHT: {
							if (_caret < _lbl.Content().Content.Length()) {
								SetCaretPositionInfo(_caret + 1, GetMoveTypeWithShift(true));
							}
							break;
						}
						case VK_UP: {
							size_t line = _lbl.Content().GetLineOfCaret(_caret, _cbase + _lbl.Content().LayoutRectangle.Left);
							if (line == 0) {
								break;
							}
							Core::Math::Vector2 pos(
								_cbase + _lbl.Content().LayoutRectangle.Left,
								_lbl.Content().GetLineTop(line) - _lbl.Content().GetLineHeight(line - 1) * 0.5
							);
							SetCaretPositionInfo(_lbl.Content().HitTestForCaret(pos), GetMoveTypeWithShift(false));
							break;
						}
						case VK_DOWN: {
							size_t line = _lbl.Content().GetLineOfCaret(_caret, _cbase + _lbl.Content().LayoutRectangle.Left);
							if (line >= _lbl.Content().GetLineNumber() - 1) {
								break;
							}
							Core::Math::Vector2 pos(
								_cbase + _lbl.Content().LayoutRectangle.Left,
								_lbl.Content().GetLineBottom(line) + _lbl.Content().GetLineHeight(line + 1) * 0.5
							);
							SetCaretPositionInfo(_lbl.Content().HitTestForCaret(pos), GetMoveTypeWithShift(false));
							break;
						}
						case VK_DELETE: {
							if (!DeleteSelectedBlock() && !_readOnly) {
								if (_caret < _lbl.Content().Content.Length()) {
									_lbl.Content().Content.Remove(_caret);
									_lbl.FitContent();
									SetCaretPositionInfo(_caret, CaretMoveType::SetBaseLineAndCancelSelection);
									OnTextChanged(Core::Info());
								}
							}
							break;
						}
						case VK_INSERT: {
							_insert = !_insert;
							break;
						}
						case VK_PRIOR: {
							Core::Math::Vector2 pos;
							size_t line = _lbl.Content().GetLineOfCaret(_caret, _cbase + _lbl.Content().LayoutRectangle.Left);
							pos.X = _cbase + _lbl.Content().LayoutRectangle.Left;
							pos.Y = _lbl.Content().GetLineTop(line) + 0.5 * _lbl.Content().GetLineHeight(line) - GetVisibleRange().Height();
							SetCaretPositionInfo(_lbl.Content().HitTestForCaret(pos), GetMoveTypeWithShift(false));
							break;
						}
						case VK_NEXT: {
							Core::Math::Vector2 pos;
							size_t line = _lbl.Content().GetLineOfCaret(_caret, _cbase + _lbl.Content().LayoutRectangle.Left);
							pos.X = _cbase + _lbl.Content().LayoutRectangle.Left;
							pos.Y = _lbl.Content().GetLineTop(line) + 0.5 * _lbl.Content().GetLineHeight(line) + GetVisibleRange().Height();
							SetCaretPositionInfo(_lbl.Content().HitTestForCaret(pos), GetMoveTypeWithShift(false));
							break;
						}
						case VK_HOME: {
							size_t nc;
							double nbase;
							_lbl.Content().GetLineBeginning(
								_lbl.Content().GetLineOfCaret(_caret, _cbase + _lbl.Content().LayoutRectangle.Left),
								nc, nbase
							);
							SetCaretPositionInfo(nc, GetMoveTypeWithShift(true), nbase - _lbl.Content().LayoutRectangle.Left);
							break;
						}
						case VK_END: {
							size_t nc;
							double nbase;
							_lbl.Content().GetLineCursorEnding(
								_lbl.Content().GetLineOfCaret(_caret, _cbase + _lbl.Content().LayoutRectangle.Left),
								nc, nbase
							);
							SetCaretPositionInfo(nc, GetMoveTypeWithShift(true), nbase - _lbl.Content().LayoutRectangle.Left);
							break;
						}
					}
				}
				virtual void OnText(const Core::Input::TextInfo &info) override {
					ScrollViewBase::OnText(info);
					if (_readOnly) {
						return;
					}
					bool changed = true, hadSelection = DeleteSelectedBlock();
					int target = _caret + 1;
					_blink = 0.0;
					switch (info.GetChar()) {
						case VK_BACK: {
							if (!hadSelection) {
								if (_caret > 0) {
									target = _caret - 1;
									_lbl.Content().Content.Remove(target);
								} else {
									changed = false;
								}
							} else {
								target = _caret;
							}
							break;
						}
						default: {
							TCHAR realChar = info.GetChar();
							if (realChar == _TEXT('\r')) {
								realChar = _TEXT('\n');
							}
							if (realChar == _TEXT('\n')) {
								if (!_multiLine) {
									changed = false;
									break; // terminate in advance
								}
							}
							if (_insert) {
								_lbl.Content().Content.Insert(_caret, realChar);
							} else {
								if (_caret < _lbl.Content().Content.Length() && _lbl.Content().Content[_caret] != _TEXT('\n')) {
									_lbl.Content().Content[_caret] = realChar;
								} else {
									_lbl.Content().Content.Insert(_caret, realChar);
								}
							}
							break;
						}
					}
					if (changed) {
						_lbl.FitContent();
						SetCaretPositionInfo(target, CaretMoveType::SetBaseLineAndCancelSelection);
						OnTextChanged(Core::Info());
					}
				}
				virtual void OnTextChanged(const Core::Info &info) {
					TextChanged(info);
				}
		};
		template <typename T> const Graphics::Pen TextBox<T>::DefaultCaretPen(Core::Color(0, 0, 0, 255), 1.0);
		template <typename T> const Graphics::SolidBrush TextBox<T>::DefaultSelectionBrush(Core::Color(100, 150, 200, 100));
		template <typename T> const Graphics::SolidBrush TextBox<T>::DefaultTextBoxBackground(Core::Color(255, 255, 255, 255));
		template <typename T> const Graphics::Pen TextBox<T>::DefaultTextBoxBorder(Core::Color(0, 0, 0, 255), 1.0);
	}
}
