#pragma once

#ifdef DEBUG
#	include <iostream>
#endif

#include "Rectangle.h"
#include "InputElement.h"
#include "Thickness.h"
#include "Size.h"
#include "UIWorld.h"
#include "Renderer.h"
#include "BrushAndPen.h"

namespace DE {
	namespace UI {
		enum class Visibility {
			Visible = 3,
			Invisible = 1,
			Ignored = 0,
			Ghost = 2
		};
		enum class LayoutDirection {
			Horizontal,
			Vertical
		};

		class World;
		class ControlCollection;
		class PanelBase;
		class Control : public Core::Input::InputElement {
				friend class World;
				friend class ControlCollection;
				friend class PanelBase;
				friend class WrapPanelBase;
				friend class ScrollViewBase;
			public:
				Control() = default;
				Control(const Control&) = delete;
				Control &operator =(const Control&) = delete;
				virtual ~Control();

				virtual Anchor GetAnchor() const {
					return static_cast<Anchor>(_anchor);
				}
				virtual void SetAnchor(Anchor anchor) {
					_anchor = anchor;
					ResetLayout();
				}
				virtual void AnchorTo(Anchor);
				virtual void DeanchorFrom(Anchor);

				virtual const Core::Math::Rectangle &GetActualLayout() const {
					return _actualLayout;
				}

				virtual const Thickness &GetMargins() const {
					return _margin;
				}
				virtual const Thickness &GetActualMargins() const {
					return _actualMargin;
				}
				virtual void SetMargins(const Thickness &newm) {
					_margin = newm;
					ResetLayout();
				}
				virtual const Size &GetSize() const {
					return _size;
				}
				virtual Size GetActualSize() const {
					return Size(_actualLayout.Width(), _actualLayout.Height());
				}
				virtual void SetSize(const Size &s) {
					_size = s;
					ResetLayout();
				}

				virtual Visibility GetVisibility() const {
					return _vis;
				}
				virtual void SetVisibility(Visibility v) {
					if (_vis != v) {
						_vis = v;
						if (v == Visibility::Ignored) {
							if (_world != nullptr && _world->FocusedControl() == this) {
								_world->SetFocus(nullptr);
							}
							_over = false;
							_keys = 0;
						}
						ResetLayout();
					}
				}

				virtual bool &Focusable() {
					return _focusable;
				}
				virtual const bool &Focusable() const {
					return _focusable;
				}

				virtual bool Focused() const;

				virtual PanelBase *GetFather() {
					return _father;
				}
				virtual const PanelBase *GetFather() const {
					return _father;
				}

				virtual World *GetWorld() {
					return _world;
				}
				virtual const World *GetWorld() const {
					return _world;
				}

				virtual int GetZIndex() const {
					return _zIndex;
				}

				virtual const Graphics::Brush *GetDefaultBackground() const {
					return nullptr;
				}
				virtual const Graphics::Brush *const &Background() const {
					return _background;
				}
				virtual const Graphics::Brush *&Background() {
					return _background;
				}

				virtual const Graphics::Pen *GetDefaultBorder() const {
					return nullptr;
				}
				virtual const Graphics::Pen *const &Border() const {
					return _border;
				}
				virtual const Graphics::Pen *&Border() {
					return _border;
				}

				virtual void SetCursor(const Core::Input::Cursor &cursor) override {
					_useDefaultCursor = false;
					Core::Input::InputElement::SetCursor(cursor);
				}
				virtual void RestoreDefaultCursor() {
					_useDefaultCursor = true;
					OnCursorChanged(Core::Info());
				}
				virtual Core::Input::Cursor GetCursor() const override {
					return (_useDefaultCursor ? GetDefaultCursor() : _cursor);
				}
				virtual Core::Input::Cursor GetDefaultCursor() const {
					return Core::Input::Cursor();
				}

#ifdef DEBUG
				virtual void DumpData(std::ostream &out, Core::Collections::List<bool> &hnl) {
					for (size_t i = 0; i + 1 < hnl.Count(); ++i) {
						out<<(hnl[i] ? " | " : "   ");
					}
					out<<" +- "<<typeid(*this).name()<<" "<<this<<"\n";
					DumpDataBasicProperties(out, hnl);
				}
				virtual void DumpDataBasicProperties(std::ostream &out, Core::Collections::List<bool> &hnl) {
					DumpDataProperty(out, hnl, "world", _world);
					DumpDataProperty(out, hnl, "father", _father);
					DumpDataProperty(out, hnl, "zindex", _zIndex);
					DE::Core::AsciiString str =
						"Left=" +
						Core::ToString<double, char>(_actualLayout.Left) +
						" Top=" +
						Core::ToString<double, char>(_actualLayout.Top) +
						" Right=" +
						Core::ToString<double, char>(_actualLayout.Right) +
						" Bottom=" +
						Core::ToString<double, char>(_actualLayout.Bottom);
					DumpDataProperty(out, hnl, "layout", *str);
					if (Name) {
						DumpDataProperty(out, hnl, "description", Name);
					}
				}
				template <typename U, typename V> void DumpDataProperty(std::ostream &out, Core::Collections::List<bool> &hnl, const U &name, const V &value) {
					for (size_t i = 0; i < hnl.Count(); ++i) {
						out<<(hnl[i] ? " | " : "   ");
					}
					out<<" > "<<name<<"\t"<<value<<"\n";
				}
#endif

				const static Graphics::Pen FocusBorderPen;
				const static double FocusBorderWidth;
			protected:
				static void SolveArrangement(
					bool, bool, double,
					double, double, double,
					double&, double&, double&
				);

				virtual void BeginRendering(Graphics::Renderer&);
				virtual void Render(Graphics::Renderer&);
				virtual void EndRendering(Graphics::Renderer&);
				virtual void Update(double) = 0;

				virtual void Initialize() {
					_inited = true;
				}

				virtual bool HitTest(const Core::Math::Vector2 &pos) const {
					if (_vis == Visibility::Ignored || _vis == Visibility::Ghost) {
						return false;
					}
					return Core::Math::Rectangle::Intersect(_actualLayout, pos) != Core::Math::IntersectionType::None;
				}
				virtual void ResetVerticalLayout();
				virtual void ResetHorizontalLayout();
				virtual void ResetLayout();
				virtual void FinishLayoutChange();

				virtual bool OnMouseDown(const Core::Input::MouseButtonInfo&) override;
				virtual bool OnMouseScroll(const Core::Input::MouseScrollInfo&) override;

				virtual void OnWorldChanged(const Core::Info&) {
				}

				bool Initialized() const {
					return _inited;
				}

				inline static void FillRectWithFallback(
					Graphics::Renderer &r,
					const Graphics::Brush *b,
					const Graphics::Brush *fallback,
					const Core::Math::Rectangle &rect
				) {
					if (b) {
						b->FillRect(rect, r);
					} else if (fallback) {
						fallback->FillRect(rect, r);
					}
				}
				inline static void DrawLinesWithFallback(
					Graphics::Renderer &r,
					const Graphics::Pen *p,
					const Graphics::Pen *fallback,
					const Core::Collections::List<Core::Math::Vector2> &lines
				) {
					if (p) {
						p->DrawLines(lines, r);
					} else if (fallback) {
						fallback->DrawLines(lines, r);
					}
				}
				inline static void DrawLineStripWithFallback(
					Graphics::Renderer &r,
					const Graphics::Pen *p,
					const Graphics::Pen *fallback,
					const Core::Collections::List<Core::Math::Vector2> &lineStrip
				) {
					if (p) {
						p->DrawLineStrip(lineStrip, r);
					} else if (fallback) {
						fallback->DrawLineStrip(lineStrip, r);
					}
				}

				Anchor _anchor = Anchor::TopLeft;
				PanelBase *_father = nullptr;
				Core::Math::Rectangle _actualLayout;
				Thickness _margin, _actualMargin;
				Size _size, actualSize;
				int _zIndex = 0;
				Visibility _vis = Visibility::Visible;
				bool _focusable = true, _useDefaultCursor = true, _disposing = false; // modify this when necessary
				const Graphics::Brush *_background = nullptr;
				const Graphics::Pen *_border = nullptr;
			private:
				bool _inited = false;
				World *_world = nullptr;

				virtual void SetWorld(World*);
#ifdef DEBUG
			public:
				const char *Name = nullptr;
#endif
		};
	}
}
