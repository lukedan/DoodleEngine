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
		template <typename T> struct ComboBoxSelectionChangedInfo;
		// TODO make ComboBox inherit from Button or ButtonBase
		template <typename T = Graphics::TextRendering::BasicText> class SimpleComboBox : public SimpleButton<T> { // TODO: restore the FitContentHeight() calls
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

						T &Content() {
							return _btn.Content();
						}
						const T &Content() const {
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
						SimpleButton<T> _btn;
						SimpleComboBox &_father;
						void FitScrollView() {
							_btn.SetSize(Size(_father.GetActualLayout().Width(), _btn.GetSize().Height));
						}
				};
				friend class Item;

				SimpleComboBox() : _arrowColor(0, 0, 0, 255) {
					this->_content.HorizontalAlignment = Graphics::TextRendering::HorizontalTextAlignment::Left;
					_box.SetAnchor(Anchor::TopLeft);
					_box.SetChild(&_pnl);
					_box.SetVisibility(Visibility::Ignored);
					_box.SetHorizontalScrollBarVisibility(ScrollBarVisibility::Hidden);
					_box.LostFocus += [&](const Core::Info &info) {
						if (!this->_disposing && IsSelectionCancelled()) {
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
				SimpleComboBox(ControlCollection &col) : SimpleComboBox() {
					_dropCol = &col;
				}
				virtual ~SimpleComboBox() {
					this->_disposing = true;
					_items.ForEach([&](Item *item) {
						item->~Item();
						Core::GlobalAllocator::Free(item);
						return true;
					});
				}

				ControlCollection *GetDropDownPanelFather() {
					return _dropCol;
				}
				const ControlCollection *GetDropDownPanelFather() const {
					return _dropCol;
				}
				void SetDropDownPanelFather(ControlCollection *col) {
					if (!this->Initialized()) {
						Initialize();
					}
					if (_dropCol) {
						_dropCol->Delete(_box);
					}
					_dropCol = col;
					if (_dropCol) {
						_dropCol->Insert(_box);
						ResetDropPanelPosition();
						ResetItemsSize();
					}
				}
				void SetDropPanelZIndex(int zIndex) {
					if (!this->Initialized()) {
						Initialize();
					}
					if (_dropCol) {
						_dropCol->SetZIndex(_box, zIndex);
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
				void SetSelectedItem(Item *item) {
					if (item && &(item->_father) != this) {
						throw Core::InvalidOperationException(_TEXT("the item doesn't belong to this ComboBox"));
					}
					if (_selection) {
						SetDefaultBrushes(_selection->_btn);
					}
					_selection = item;
					if (_selection) {
						SetSelectedBrushes(_selection->_btn);
						this->_content = _selection->Content();
						this->_content.LayoutRectangle = this->_actualLayout;
					} else {
						this->_content.Content = _TEXT("");
					}
					OnSelectionChanged(ComboBoxSelectionChangedInfo<T>(item));
				}
				Item *GetSelectedItem() const {
					return _selection;
				}
				size_t GetItemIndex(const Item &item) {
					return _items.FindFirst(const_cast<Item*>(&item));
				}

				void FitLargestItem() {
					double maxw = 0.0, maxh = 0.0;
					_items.ForEach([&](Item *item) {
						Core::Math::Vector2 sz = item->Content().GetSize();
						maxw = Core::Math::Max(maxw, sz.X);
						maxh = Core::Math::Max(maxh, sz.Y);
						return true;
					});
					double diffw = maxw - _box.GetVisibleRange().Width() + this->GetActualSize().Width;
					maxw += 4.0 * ArrowSize;
					this->SetSize(Size(Core::Math::Max(diffw, maxw), maxh));
				}

				Core::Event<ComboBoxSelectionChangedInfo<T>> SelectionChanged;
			protected:
				SimpleScrollView _box;
				WrapPanel _pnl;
				ControlCollection *_dropCol = nullptr;
				Item *_selection = nullptr;
				Core::Collections::List<Item*> _items;
				Core::Color _arrowColor;
				Core::Input::MouseScrollBuffer _scrollBuf;

				virtual void Render(Graphics::Renderer &r) override {
					SimpleButton<T>::Render(r);
					Core::Collections::List<Graphics::Vertex> tri;
					Core::Math::Rectangle rect = this->_actualLayout;
					rect.Left = rect.Right - ArrowSize * 3.0;
					rect.Right = rect.Left + ArrowSize * 2.0;
					rect.Top = (rect.Top + rect.Bottom - ArrowSize) * 0.5;
					rect.Bottom = rect.Top + ArrowSize;
					tri.PushBack(Graphics::Vertex(rect.TopLeft(), _arrowColor));
					tri.PushBack(Graphics::Vertex(rect.TopRight(), _arrowColor));
					tri.PushBack(Graphics::Vertex(Core::Math::Vector2((rect.Left + rect.Right) * 0.5, rect.Bottom), _arrowColor));
					r.BindTexture(Graphics::TextureID());
					r.DrawVertices(tri, Graphics::RenderMode::Triangles);
				}

				virtual void Initialize() override {
					SimpleButton<T>::Initialize();
					if (_dropCol && _box.GetFather() == nullptr) {
						_dropCol->Insert(_box);
					}
				}

				virtual void FinishLayoutChange() override {
					SimpleButton<T>::FinishLayoutChange();
					ResetDropPanelPosition();
					ResetItemsSize();
				}
				virtual void ResetDropPanelPosition() {
					if (_dropCol) {
						Core::Math::Vector2 tl = this->_actualLayout.BottomLeft();
						tl -= _dropCol->GetFather().GetActualLayout().TopLeft();
						_box.SetMargins(Thickness(tl.X, tl.Y, 0.0, 0.0));
					}
				}
				virtual void ResetItemsSize() {
					_items.ForEach([&](Item *item) {
						item->FitScrollView();
						return true;
					});
					_pnl.FitContent();
					if (_dropCol) {
						double maxsz = _dropCol->GetFather().GetActualLayout().Bottom - this->_actualLayout.Bottom;
						_box.SetSize(Size(this->_actualLayout.Width(), Core::Math::Min(maxsz, _pnl.GetSize().Height)));
					}
				}
				virtual void OnClick(const Core::Info &info) override {
					SimpleButton<T>::OnClick(info);
					if (_dropCol) {
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
						this->GetWorld()->SetFocus(&_box);
					}
				}
				virtual void OnSelectionChanged(const ComboBoxSelectionChangedInfo<T> &info) {
					SelectionChanged(info);
				}
				virtual void OnItemClicked(Item &item) {
					_box.SetVisibility(Visibility::Ignored);
					SetSelectedItem(&item);
					this->GetWorld()->SetFocus(this);
				}

				virtual void OnLostFocus(const Core::Info &info) override {
					SimpleButton<T>::OnLostFocus(info);
					if (IsSelectionCancelled()) {
						OnSelectionCancelled(info);
					}
				}
				virtual bool IsSelectionCancelled() const {
					return this->GetWorld()->FocusedControl() != this && this->GetWorld()->FocusedControl() != &_box;
				}
				virtual void OnSelectionCancelled(const Core::Info&) {
					_box.SetVisibility(Visibility::Ignored);
				}
				virtual bool OnMouseScroll(const Core::Input::MouseScrollInfo &info) override {
					Core::Input::InputElement::OnMouseScroll(info);
					if (_box.GetVisibility() == Visibility::Visible) {
						this->GetWorld()->SetFocus(&_box);
					} else {
						_scrollBuf.OnScroll(info.Delta);
					}
					return true;
				}
				virtual void SetDefaultBrushes(SimpleButton<T> &btn) {
					btn.NormalBrush = &DefaultNormalBrush;
					btn.HoverBrush = &DefaultHoverBrush;
					btn.PressedBrush = &DefaultPressedBrush;
				}
				virtual void SetSelectedBrushes(SimpleButton<T> &btn) {
					btn.NormalBrush = btn.HoverBrush = btn.PressedBrush = &DefaultSelectedBrush;
				}
		};
		template <typename T> const Graphics::SolidBrush SimpleComboBox<T>::DefaultNormalBrush(Core::Color(255, 255, 255, 255));
		template <typename T> const Graphics::SolidBrush SimpleComboBox<T>::DefaultHoverBrush(Core::Color(150, 150, 150, 255));
		template <typename T> const Graphics::SolidBrush SimpleComboBox<T>::DefaultPressedBrush(Core::Color(100, 100, 100, 255));
		template <typename T> const Graphics::SolidBrush SimpleComboBox<T>::DefaultSelectedBrush(Core::Color(100, 150, 255, 255));
		template <typename T> struct ComboBoxSelectionChangedInfo {
			public:
				ComboBoxSelectionChangedInfo(typename SimpleComboBox<T>::Item *item) : NewSelection(item) {
				}

				Core::ReferenceProperty<typename SimpleComboBox<T>::Item*, Core::PropertyType::ReadOnly> NewSelection;
		};
	}
}
