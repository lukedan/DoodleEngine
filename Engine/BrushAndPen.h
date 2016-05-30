#pragma once

#include "Rectangle.h"
#include "Renderer.h"
#include "Rectangle.h"

namespace DE {
	namespace Graphics {
		class Brush {
				friend class Renderer;
			public:
				virtual ~Brush() {
				}

				virtual void FillRect(const Core::Math::Rectangle&, Renderer&) const = 0;
		};
		class SolidBrush : public Brush {
			public:
				SolidBrush() = default;
				explicit SolidBrush(const Core::Color &c) : _color(c) {
				}
				virtual ~SolidBrush() {
				}

				Core::Color &BrushColor() {
					return _color;
				}
				const Core::Color &BrushColor() const {
					return _color;
				}

				virtual void FillRect(const Core::Math::Rectangle &rect, Renderer &r) const override {
					Core::Collections::List<Vertex> v;
					v.PushBack(Vertex(rect.TopLeft(), _color));
					v.PushBack(Vertex(rect.BottomLeft(), _color));
					v.PushBack(Vertex(rect.TopRight(), _color));
					v.PushBack(Vertex(rect.TopRight(), _color));
					v.PushBack(Vertex(rect.BottomLeft(), _color));
					v.PushBack(Vertex(rect.BottomRight(), _color));
					r<<Texture();
					r.DrawVertices(v, RenderMode::Triangles);
				}
			protected:
				Core::Color _color;
		};
		class TextureBrush : public Brush {
			public:
				TextureBrush() = default;
				explicit TextureBrush(const Texture &t) : _tex(t) {
				}

				Texture &BrushTexture() {
					return _tex;
				}
				const Texture &BrushTexture() const {
					return _tex;
				}

				Core::Color &BrushColor() {
					return _color;
				}
				const Core::Color &BrushColor() const {
					return _color;
				}

				TextureWrap &HorizontalWrap() {
					return _hor;
				}
				const TextureWrap &HorizontalWrap() const {
					return _hor;
				}
				TextureWrap &VerticalWrap() {
					return _ver;
				}
				const TextureWrap &VerticalWrap() const {
					return _ver;
				}

				const Core::Math::Rectangle &UVRegion() const {
					return _rgn;
				}
				Core::Math::Rectangle &UVRegion() {
					return _rgn;
				}

				virtual void FillRect(const Core::Math::Rectangle &rect, Renderer &renderer) const override {
					renderer<<_tex;
					renderer.SetHorizontalTextureWrap(_hor).SetVerticalTextureWrap(_ver);
					Core::Collections::List<Vertex> vs;
					vs.PushBack(Vertex(rect.TopLeft(), _color, _rgn.TopLeft()));
					vs.PushBack(Vertex(rect.TopRight(), _color, _rgn.TopRight()));
					vs.PushBack(Vertex(rect.BottomLeft(), _color, _rgn.BottomLeft()));
					vs.PushBack(Vertex(rect.TopRight(), _color, _rgn.TopRight()));
					vs.PushBack(Vertex(rect.BottomLeft(), _color, _rgn.BottomLeft()));
					vs.PushBack(Vertex(rect.BottomRight(), _color, _rgn.BottomRight()));
					renderer.DrawVertices(vs, RenderMode::Triangles);
				}
			protected:
				Texture _tex;
				Core::Color _color;
				TextureWrap
					_hor = TextureWrap::None,
					_ver = TextureWrap::None;
				Core::Math::Rectangle _rgn;
		};

		class Pen {
				friend class Renderer;
			public:
				Pen() = default;
				explicit Pen(const Core::Color &color) : _color(color) {
				}
				Pen(const Core::Color &color, double thickness) : _color(color), _thickness(thickness) {
				}
				virtual ~Pen() {
				}

				Core::Color &PenColor() {
					return _color;
				}
				const Core::Color &PenColor() const {
					return _color;
				}

				double &Thickness() {
					return _thickness;
				}
				const double &Thickness() const {
					return _thickness;
				}

				virtual void DrawLines(const Core::Collections::List<Core::Math::Vector2> &vxs, Renderer &r) const {
					Core::Collections::List<Vertex> ps;
					vxs.ForEach([&ps, this](const Core::Math::Vector2 &v) {
						ps.PushBack(Vertex(v, _color));
						return true;
					});
					r<<Texture();
					r.SetLineWidth(_thickness);
					r.DrawVertices(ps, RenderMode::Lines);
				}
				virtual void DrawLineStrip(const Core::Collections::List<Core::Math::Vector2> &vxs, Renderer &r) const {
					Core::Collections::List<Vertex> ps;
					vxs.ForEach([&ps, this](const Core::Math::Vector2 &v) {
						ps.PushBack(Vertex(v, _color));
						return true;
					});
					r<<Texture();
					r.SetLineWidth(_thickness);
					r.DrawVertices(ps, RenderMode::LineStrip);
				}
				virtual void DrawRectangle(const Core::Math::Rectangle &rect, Renderer &r) const {
					Core::Collections::List<Vertex> vs;
					vs.PushBack(Vertex(rect.TopLeft(), _color));
					vs.PushBack(Vertex(rect.TopRight(), _color));
					vs.PushBack(Vertex(rect.BottomRight(), _color));
					vs.PushBack(Vertex(rect.BottomLeft(), _color));
					vs.PushBack(Vertex(rect.TopLeft(), _color));
					r<<Texture();
					r.SetLineWidth(_thickness);
					r.DrawVertices(vs, RenderMode::LineStrip);
				}
			protected:
				Core::Color _color;
				double _thickness = 1.0;
		};
	}
}
