#pragma once

#include "Panel.h"

namespace DE {
	namespace UI {
		class WrapPanelBase : public PanelBase {
			public:
				WrapPanelBase() : PanelBase() {
				    _overrideChildrenLayout = true;
				}
				virtual ~WrapPanelBase() {
				}

				void FitContent() {
					if (_ldir == LayoutDirection::Horizontal) {
						SetSize(Size(GetTotalStretch(), GetMaxSpan()));
					} else {
						SetSize(Size(GetMaxSpan(), GetTotalStretch()));
					}
				}
				void FitContentStretch() {
					if (_ldir == LayoutDirection::Horizontal) {
						SetSize(Size(GetTotalStretch(), GetSize().Height));
					} else {
						SetSize(Size(GetSize().Width, GetTotalStretch()));
					}
				}

				LayoutDirection GetLayoutDirection() const {
					return _ldir;
				}
				void SetLayoutDirection(LayoutDirection dir) {
					_ldir = dir;
					if (GetWorld() != nullptr) {
						ResetChildrenLayout();
					}
				}
			protected:
				LayoutDirection _ldir = LayoutDirection::Horizontal;

				virtual void ResetChildrenLayout() override {
					if (_ldir == LayoutDirection::Horizontal) {
						double totO = _actualLayout.Left;
						_col.ForEach([&](Control *c) {
							if (c->_vis == Visibility::Ignored) {
								return true;
							}
							c->ResetVerticalLayout();
							c->actualSize.Width = c->_size.Width;
							c->_actualLayout.Left = c->_actualMargin.Left = (totO += c->_margin.Left);
							totO += c->_size.Width;
							c->_actualMargin.Right = _actualLayout.Width() - totO;
							c->_actualLayout.Right = totO;
							totO += c->_margin.Right;
							c->_relPos += c->topLeft;
							c->topLeft = c->_actualLayout.TopLeft();
							c->_relPos -= c->topLeft;
							c->FinishLayoutChange();
							return true;
						});
					} else {
						double totO = _actualLayout.Top;
						_col.ForEach([&](Control *c) {
							if (c->_vis == Visibility::Ignored) {
								return true;
							}
							c->ResetHorizontalLayout();
							c->actualSize.Height = c->_size.Height;
							c->_actualLayout.Top = c->_actualMargin.Top = (totO += c->_margin.Top);
							totO += c->_size.Height;
							c->_actualMargin.Bottom = _actualLayout.Height() - totO;
							c->_actualLayout.Bottom = totO;
							totO += c->_margin.Bottom;
							c->_relPos += c->topLeft;
							c->topLeft = c->_actualLayout.TopLeft();
							c->_relPos -= c->topLeft;
							c->FinishLayoutChange();
							return true;
						});
					}
				}
				virtual double GetTotalStretch() const {
					double totS = 0.0;
					if (_ldir == LayoutDirection::Horizontal) {
						_col.ForEach([&](const Control *c) {
							if (c->_vis == Visibility::Ignored) {
								return true;
							}
							totS += c->_margin.Left + c->_size.Width + c->_margin.Right;
							return true;
						});
					} else {
						_col.ForEach([&](const Control *c) {
							if (c->_vis == Visibility::Ignored) {
								return true;
							}
							totS += c->_margin.Top + c->_size.Height + c->_margin.Bottom;
							return true;
						});
					}
					return totS;
				}
				virtual double GetMaxSpan() const {
					double maxS = 0.0;
					if (_ldir == LayoutDirection::Horizontal) {
						_col.ForEach([&](const Control *c) {
							double h = 0.0;
							if ((int)c->GetAnchor() & (int)Anchor::Top) {
								h += c->GetMargins().Top;
							}
							if ((int)c->GetAnchor() & (int)Anchor::Bottom) {
								h += c->GetMargins().Bottom;
							}
							if ((Anchor)((int)c->GetAnchor() & (int)Anchor::StretchVertically) != Anchor::StretchVertically) {
								h += c->GetSize().Height;
							}
							if (h > maxS) {
								maxS = h;
							}
							return true;
						});
					} else {
						_col.ForEach([&](const Control *c) {
							double w = 0.0;
							if ((int)c->GetAnchor() & (int)Anchor::Left) {
								w += c->GetMargins().Left;
							}
							if ((int)c->GetAnchor() & (int)Anchor::Right) {
								w += c->GetMargins().Right;
							}
							if ((Anchor)((int)c->GetAnchor() & (int)Anchor::StretchHorizontally) != Anchor::StretchHorizontally) {
								w += c->GetSize().Width;
							}
							if (w > maxS) {
								maxS = w;
							}
							return true;
						});
					}
					return maxS;
				}
		};
		class WrapPanel : public WrapPanelBase {
                friend class World;
		    public:
                virtual ~WrapPanel() {
                }

                ControlCollection &Children() {
					return _col;
				}
		};
	}
}
