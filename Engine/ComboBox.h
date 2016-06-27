#pragma once

#include "Control.h"
#include "ContentControl.h"
#include "Button.h"
#include "Panel.h"
#include "ScrollView.h"
#include "WrapPanel.h"
#include "Label.h"

namespace DE {
	namespace UI {
		struct ComboBoxSelectionChangedInfo;
		// TODO make ComboBox inherit from Button or ButtonBase
		class SimpleComboBox : public SimpleButton { // TODO: restore the FitContentHeight() calls
			public:
				const static Graphics::SolidBrush DefaultNormalBrush, DefaultHoverBrush, DefaultPressedBrush, DefaultSelectedBrush;
				constexpr static double ArrowSize = 5.0;
				class Item {
						friend class SimpleComboBox;
					public:
						Item(SimpleComboBox &base) : _father(base) {
							_btn.Content().HorizontalAlignment = Graphics::TextRendering::HorizontalTextAlignment::Left;
							_father._pnl.Children().Insert(_btn);
							FitScrollView();
							_father._pnl.FitContent();
							_btn.Focusable() = false;
							_father.SetDefaultBrushes(_btn);
							_btn.Click += [this](const Core::Info&) {
								_father.OnItemClicked(*this);
							};
						}
						Item(const Item&) = delete;
						Item &operator =(const Item&) = delete;
						~Item() {
							if (!_father._disposing) {
								_father._pnl.Children().Delete(_btn);
								_father._pnl.FitContent();
							}
						}

						Graphics::TextRendering::BasicText &Content() {
							return _btn.Content();
						}
						const Graphics::TextRendering::BasicText &Content() const {
							return _btn.Content();
						}

						void FitContent() {
							_btn.SetSize(Size(_father._box.GetSize().Width, _btn.Content().GetSize().Y));
							_father._pnl.FitContent();
						}

						SimpleComboBox &GetFather() {
							return _father;
						}
						const SimpleComboBox &GetFather() const {
							return _father;
						}
					private:
						SimpleButton _btn;
						SimpleComboBox &_father;
						void FitScrollView() {
							_btn.SetSize(Size(_father.GetActualLayout().Width(), _btn.GetSize().Height));
						}
				};
				friend class Item;

				SimpleComboBox() : _arrowColor(0, 0, 0, 255) {
					Content().HorizontalAlignment = Graphics::TextRendering::HorizontalTextAlignment::Left;
					_box.SetAnchor(Anchor::TopLeft);
					_box.SetChild(&_pnl);
					_box.SetVisibility(Visibility::Ignored);
					_box.SetHorizontalScrollBarVisibility(ScrollBarVisibility::Hidden);
					_box.LostFocus += [&](const Core::Info &info) {
						if (!_disposing && IsSelectionCancelled()) {
							OnSelectionCancelled(info);
						}
					};
					_pnl.Focusable() = false;
					_pnl.SetLayoutDirection(LayoutDirection::Vertical);

					_scrollBuf.OnPivotHit += [&](const Core::Input::BufferPivotHitInfo &info) {
						size_t index = _items.FindFirst(_selection), tindex = index - info.Delta;
						if (info.Delta > 0) {
							if (index == 0) {
								return;
							}
							if (index < static_cast<size_t>(info.Delta)) {
								tindex = 0;
							}
						} else {
							if (index + 1 >= _items.Count()) {
								return;
							}
							if (tindex >= _items.Count()) {
								if (_items.Count() > 0) {
									tindex = _items.Count() - 1;
								} else {
									tindex = 0;
								}
							}
						}
						SetSelectedItem(tindex);
					};
#ifdef DEBUG
					_box.Name = "a scroll view of a combo box";
					_pnl.Name = "a popup panel of a combo box";
#endif
				}
				SimpleComboBox(Panel &pnl) : SimpleComboBox() {
					_dropPnl = &pnl;
				}
				virtual ~SimpleComboBox() {
					_disposing = true;
					_items.ForEach([&](Item *item) {
						item->~Item();
						Core::GlobalAllocator::Free(item);
						return true;
					});
				}

				Panel *GetDropDownPanel() {
					return _dropPnl;
				}
				const Panel *GetDropDownPanel() const {
					return _dropPnl;
				}
				void SetDropDownPanel(Panel *pnl) {
					if (!Initialized()) {
						Initialize();
					}
					if (_dropPnl) {
						_dropPnl->Children().Delete(_box);
					}
					_dropPnl = pnl;
					if (_dropPnl) {
						_dropPnl->Children().Insert(_box);
						ResetItemsSize();
					}
				}

				Item &InsertItem() {
					Item *item = new (Core::GlobalAllocator::Allocate(sizeof(Item))) Item(*this);
					_items.PushBack(item);
					return *item;
				}
				void DeleteItem(Item *item) {
					size_t x = _items.FindFirst(item);
					if (&(item->_father) != this || x >= _items.Count()) {
						throw Core::InvalidArgumentException(_TEXT("the Item doesn't belong to this ComboBox"));
					}
					if (_selection == item) {
						_selection = nullptr;
					}
					_items.Remove(x);
					item->~Item();
					Core::GlobalAllocator::Free(item);
				}
				void ClearItems() {
					_items.ForEach([&](Item *item) {
						item->~Item();
						Core::GlobalAllocator::Free(item);
						return true;
					});
					_items.Clear();
				}

				Core::Color &ArrowColor() {
					return _arrowColor;
				}
				const Core::Color &ArrowColor() const {
					return _arrowColor;
				}

				const Core::Collections::List<Item*> &Items() const {
					return _items;
				}
				Item &GetItem(size_t index) {
					return *_items[index];
				}
				const Item &GetItem(size_t index) const {
					return *_items[index];
				}

				void SetSelectedItem(size_t index) {
					SetSelectedItem(_items[index]);
				}
				void SetSelectedItem(Item*);
				Item *GetSelectedItem() const {
					return _selection;
				}
				size_t GetItemIndex(const Item &item) {
					return _items.FindFirst(const_cast<Item*>(&item));
				}

				Core::Event<ComboBoxSelectionChangedInfo> SelectionChanged;
			protected:
				SimpleScrollView _box;
				WrapPanel _pnl;
				Panel *_dropPnl = nullptr;
				Item *_selection = nullptr;
				Core::Collections::List<Item*> _items;
				Core::Color _arrowColor;
				Core::Input::MouseScrollBuffer _scrollBuf;

				virtual void Render(Graphics::Renderer &r) override {
					SimpleButton::Render(r);
					Core::Collections::List<Graphics::Vertex> tri;
					Core::Math::Rectangle rect = _actualLayout;
					rect.Left = rect.Right - ArrowSize * 3.0;
					rect.Right = rect.Left + ArrowSize * 2.0;
					rect.Top = (rect.Top + rect.Bottom - ArrowSize) * 0.5;
					rect.Bottom = rect.Top + ArrowSize;
					tri.PushBack(Graphics::Vertex(rect.TopLeft(), _arrowColor));
					tri.PushBack(Graphics::Vertex(rect.TopRight(), _arrowColor));
					tri.PushBack(Graphics::Vertex(Core::Math::Vector2((rect.Left + rect.Right) * 0.5, rect.Bottom), _arrowColor));
					r.DrawVertices(tri, Graphics::RenderMode::Triangles);
				}

				virtual void Initialize() override {
					SimpleButton::Initialize();
					if (_dropPnl && _box.GetFather() == nullptr) {
						_dropPnl->Children().Insert(_box);
					}
				}

				virtual void FinishLayoutChange() override {
					SimpleButton::FinishLayoutChange();
					if (_dropPnl) {
						Core::Math::Vector2 tl = _actualLayout.BottomLeft();
						tl -= _dropPnl->GetActualLayout().TopLeft();
						_box.SetMargins(Thickness(tl.X, tl.Y, 0.0, 0.0));
					}
					ResetItemsSize();
				}
				virtual void ResetItemsSize() {
					_items.ForEach([&](Item *item) {
						item->FitScrollView();
						return true;
					});
					_pnl.FitContent();
					if (_dropPnl) {
						double maxsz = _dropPnl->GetActualLayout().Bottom - _actualLayout.Bottom;
						_box.SetSize(Size(GetActualLayout().Width(), Core::Math::Min(maxsz, _pnl.GetSize().Height)));
					}
				}
				virtual void OnClick(const Core::Info &info) override {
					SimpleButton::OnClick(info);
					if (_dropPnl) {
						_box.SetHorizontalScrollBarValue(0.0);
						_box.SetVisibility(Visibility::Visible);
						_box.SetVerticalScrollBarValue(0.0);
						ResetItemsSize();
						if (_selection) {
							_box.MakePointInView(
								_selection->_btn.GetActualLayout().BottomLeft() -
								_pnl.GetActualLayout().TopLeft()
							);
						}
						GetWorld()->SetFocus(&_box);
					}
				}
				virtual void OnSelectionChanged(const ComboBoxSelectionChangedInfo&);
				virtual void OnItemClicked(Item &item) {
					_box.SetVisibility(Visibility::Ignored);
					SetSelectedItem(&item);
					GetWorld()->SetFocus(this);
				}

				virtual void OnLostFocus(const Core::Info &info) override {
					SimpleButton::OnLostFocus(info);
					if (IsSelectionCancelled()) {
						OnSelectionCancelled(info);
					}
				}
				virtual bool IsSelectionCancelled() const {
					return GetWorld()->FocusedControl() != this && GetWorld()->FocusedControl() != &_box;
				}
				virtual void OnSelectionCancelled(const Core::Info &info) {
					_box.SetVisibility(Visibility::Ignored);
				}
				virtual bool OnMouseScroll(const Core::Input::MouseScrollInfo &info) override {
					InputElement::OnMouseScroll(info);
					if (_box.GetVisibility() == Visibility::Visible) {
						GetWorld()->SetFocus(&_box);
					} else {
						_scrollBuf.OnScroll(info.Delta);
					}
					return true;
				}
				virtual void SetDefaultBrushes(SimpleButton &btn) {
					btn.NormalBrush = &DefaultNormalBrush;
					btn.HoverBrush = &DefaultHoverBrush;
					btn.PressedBrush = &DefaultPressedBrush;
				}
				virtual void SetSelectedBrushes(SimpleButton &btn) {
					btn.NormalBrush = btn.HoverBrush = btn.PressedBrush = &DefaultSelectedBrush;
				}
		};
		struct ComboBoxSelectionChangedInfo {
			public:
				ComboBoxSelectionChangedInfo(SimpleComboBox::Item *item) : NewSelection(item) {
				}

				Core::ReferenceProperty<SimpleComboBox::Item*, Core::PropertyType::ReadOnly> NewSelection;
		};
	}
}
