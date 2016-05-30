#pragma once

#include "Label.h"
#include "ScrollView.h"
#include "BrushAndPen.h"

namespace DE {
	namespace UI {
		class TextBox : public ScrollViewBase {
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

				Graphics::TextRendering::Text &Text() {
					return _lbl.Content();
				}
				const Graphics::TextRendering::Text &Text() const {
					return _lbl.Content();
				}
				void SetText(const Core::String &text) {
					_lbl.Content().Content = text;
					_lbl.FitText();
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
					if (_lbl.Content().Font) {
						Core::Math::Vector2 caret = _lbl.Content().GetRelativeCaretPosition(_caret);
						MakePointInView(caret + Core::Math::Vector2(0.0, _lbl.Content().Font->GetHeight() * _lbl.Content().Scale));
						MakePointInView(caret);
						if (!_insert) {
							MakePointInView(caret + Core::Math::Vector2(GetOverwriteModeCaretWidth(), 0.0));
						}
						_blink = 0.0;
					}
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

				virtual bool IsMultiLine() const {
					return _multiLine;
				}
				virtual void SetMultiLine(bool v) {
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
						_lbl.FitText();
						SetCaretPositionInfo(tarC, CaretMoveType::SetBaseLineAndCancelSelection);
						OnTextChanged(Core::Info());
					}
				}

				virtual void SetDesiredVisibleLine(size_t lines) {
					SetSize(Size(GetSize().Width, _lbl.Content().Padding.Height() + lines * _lbl.Content().Font->GetHeight()));
				}

				using ScrollViewBase::SetHorizontalScrollBarVisibility;
				using ScrollViewBase::SetVerticalScrollBarVisibility;

				Core::Event<Core::Info> TextChanged;
			protected:
				size_t _caret = 0, _selectS = 0, _selectE = 0;
				Label _lbl;
				const Graphics::Pen *_caretPen = nullptr;
				const Graphics::Brush *_selectionBrush = nullptr;
				double _blink = 0.0, _period = DefaultCursorPeriod, _cbase = 0.0;
				bool _selecting = false, _readOnly = false, _insert = true, _multiLine = true;

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

				virtual void SetCaretPositionInfo(size_t newPosition, CaretMoveType type) {
					_caret = newPosition;
					if ((int)type & (int)CaretMoveType::SetBaseLine) {
						_cbase = _lbl.Content().GetRelativeCaretPosition(_caret).X;
					}
					if ((int)type & (int)CaretMoveType::CancelSelection) {
						_selectS = _selectE = _caret;
					} else if ((int)type & (int)CaretMoveType::ExtendSelection) {
						_selectE = _caret;
					}
					MakeCaretInView();
				}
				virtual void Update(double dt) override {
					ScrollViewBase::Update(dt);
					_blink += dt;
					while (_blink > _period) {
						_blink -= _period;
					}
					if (_selecting) {
						if (GetWorld()) {
							size_t caretP = _lbl.Content().HitTestForCaret(GetWorld()->GetRelativeMousePosition() + GetWorld()->GetBounds().TopLeft());
							SetCaretPositionInfo(caretP, CaretMoveType::SetBaseLineAndExtendSelection);
						}
						if (!Core::IsKeyDown(VK_LBUTTON)) {
							_selecting = false;
						}
					}
				}
				virtual void RenderChild(Graphics::Renderer &r, Control *c) override {
					ScrollViewBase::RenderChild(r, c);
					if (_lbl.Content().Font) {
						if (Focused() && _blink < _period * 0.5) {
							Core::Collections::List<Core::Math::Vector2> caret;
							Core::Math::Vector2 delta;
							double dy = _lbl.Content().Font->GetHeight() * _lbl.Content().Scale;
							caret.PushBack(_lbl.Content().GetCaretPosition(_caret) + Core::Math::Vector2(0.0, dy));
							if (_insert) {
								delta.Y = -dy;
							} else {
								delta.X = GetOverwriteModeCaretWidth();
							}
							caret.PushBack(caret.First() + delta);
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
				}
				double GetOverwriteModeCaretWidth() const {
					double rawResult;
					if (_caret >= _lbl.Content().Content.Length()) {
						rawResult = _lbl.Content().Font->GetData(_TEXT('\n')).Advance;
					} else {
						rawResult = _lbl.Content().Font->GetData(_lbl.Content().Content[_caret]).Advance;
					}
					return rawResult * _lbl.Content().Scale;
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
					_lbl.FitText();
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
							case Core::Input::MouseButton::Right: {
								// nothing to do
								break;
							}
							case Core::Input::MouseButton::Middle: {
								// nothing to do either, just to disable the stupid errors
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
							if (_lbl.Content().Font) {
								Core::Math::Vector2 cpos = _lbl.Content().GetRelativeCaretPosition(_caret);
								cpos.Y -= _lbl.Content().Font->GetHeight() * _lbl.Content().Scale * 0.5;
								if (cpos.Y > 0.0) {
									cpos.X = _cbase;
									SetCaretPositionInfo(
										_lbl.Content().HitTestForCaret(cpos + _lbl.Content().LayoutRectangle.TopLeft()),
										GetMoveTypeWithShift(false)
									);
								}
							}
							break;
						}
						case VK_DOWN: {
							if (_lbl.Content().Font) {
								Core::Math::Vector2 cpos = _lbl.Content().GetRelativeCaretPosition(_caret);
								cpos.Y += _lbl.Content().Font->GetHeight() * _lbl.Content().Scale * 1.5;
								if (cpos.Y < _lbl.Content().GetSize().Y) {
									cpos.X = _cbase;
									SetCaretPositionInfo(
										_lbl.Content().HitTestForCaret(cpos + _lbl.Content().LayoutRectangle.TopLeft()),
										GetMoveTypeWithShift(false)
									);
								}
							}
							break;
						}
						case VK_DELETE: {
							if (!DeleteSelectedBlock() && !_readOnly) {
								if (_caret < _lbl.Content().Content.Length()) {
									_lbl.Content().Content.Remove(_caret);
									_lbl.FitText();
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
						// TODO pageup and pagedown not found
						case VK_HOME: {
							if (_caret == 0) {
								break;
							}
							_lbl.Content().CacheFormat();
							_lbl.Content().FormatCached = false;
							size_t targetID = Core::Math::BinaryFindUpperBound(
								_caret - 1,
								*(_lbl.Content().FormatCache.LineBreaks),
								_lbl.Content().FormatCache.LineBreaks.Count()
							), targetPos = (
								targetID >= _lbl.Content().FormatCache.LineBreaks.Count() ?
								0 :
								_lbl.Content().FormatCache.LineBreaks[targetID] + 1
							);
							SetCaretPositionInfo(targetPos, GetMoveTypeWithShift(true));
							break;
						}
						case VK_END: {
							_lbl.Content().CacheFormat();
							_lbl.Content().FormatCached = false;
							size_t targetID = Core::Math::BinaryFindLowerBound(
								_caret,
								*(_lbl.Content().FormatCache.LineBreaks),
								_lbl.Content().FormatCache.LineBreaks.Count()
							), targetPos = (
								targetID >= _lbl.Content().FormatCache.LineBreaks.Count() ?
								_lbl.Content().Content.Length() :
								_lbl.Content().FormatCache.LineBreaks[targetID]
							);
							SetCaretPositionInfo(targetPos, GetMoveTypeWithShift(true));
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
							if (_lbl.Content().Font && _lbl.Content().Font->HasData(realChar)) {
								if (_insert) {
									_lbl.Content().Content.Insert(_caret, realChar);
								} else {
									if (_caret < _lbl.Content().Content.Length() && _lbl.Content().Content[_caret] != _TEXT('\n')) {
										_lbl.Content().Content[_caret] = realChar;
									} else {
										_lbl.Content().Content.Insert(_caret, realChar);
									}
								}
							} else {
								changed = false;
							}
							break;
						}
					}
					if (changed) {
						_lbl.FitText();
						SetCaretPositionInfo(target, CaretMoveType::SetBaseLineAndCancelSelection);
						OnTextChanged(Core::Info());
					}
				}
				virtual void OnTextChanged(const Core::Info &info) {
					TextChanged(info);
				}
		};
	}
}
