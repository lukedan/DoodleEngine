#pragma once

#include "..\CoreWrap.h"
#include "Font.h"

namespace DE {
	namespace Graphics {
		namespace TextRendering {
			struct TextFormatCache {
				TextFormatCache() = default;
				Core::Collections::List<size_t> LineBreaks;
				Core::Collections::List<double> LineLengths;
				Core::Math::Vector2 Size;
			};
			enum class HorizontalTextAlignment {
				Left,
				Center,
				Right
			};
			enum class VerticalTextAlignment {
				Top,
				Center,
				Bottom
			};
			struct Text { // FIXME controls in DE::UI conflicts with the Padding here (to be validated)
				public:
					Text() = default;
					Text(const Core::String &str, Font *font = nullptr, double maxW = 0.0) : Content(str), MaxWidth(maxW), Font(font) {
					}

					Core::String Content;
					double MaxWidth = 0.0, Scale = 1.0;
					TextFormatCache FormatCache;
					bool FormatCached = false;
					const TextRendering::Font *Font = nullptr;
					Core::Math::Rectangle LayoutRectangle;
					HorizontalTextAlignment HorizontalAlignment = HorizontalTextAlignment::Left;
					VerticalTextAlignment VerticalAlignment = VerticalTextAlignment::Top;
					bool RoundToInteger = true, UseClip = false;
					Core::Math::Rectangle Clip, Padding = Core::Math::Rectangle(-2.0, -2.0, 4.0, 4.0);
					Core::Color TextColor;

					void CacheFormat();

					Core::Math::Vector2 GetSize() const {
						if (FormatCached) {
							return FormatCache.Size + Core::Math::Vector2(Padding.Width(), Padding.Height());
						} else {
							TextFormatCache t;
							DoCache(Content, t, Font, MaxWidth, Scale);
							return t.Size + Core::Math::Vector2(Padding.Width(), Padding.Height());
						}
					}

					friend Renderer &operator <<(Renderer &r, const Text &txt) {
						txt.Render(r);
						return r;
					}
					void Render(Renderer&) const;

					 // NOTE all positions are absolute, unless stated otherwise
					Core::Collections::List<Core::Math::Rectangle> GetSelectionRegion(size_t, size_t) const;
					void HitTest(const Core::Math::Vector2&, size_t&, size_t&) const;
					size_t HitTestForCaret(const Core::Math::Vector2 &pos) const {
						size_t over, caret;
						HitTest(pos, over, caret);
						return caret;
					}
					size_t HitTestForChar(const Core::Math::Vector2 &pos) const {
						size_t over, caret;
						HitTest(pos, over, caret);
						return over;
					}
					Core::Math::Vector2 GetRelativeCaretPosition(size_t) const;
					Core::Math::Vector2 GetCaretPosition(size_t caret) const {
						return GetRelativeCaretPosition(caret) + LayoutRectangle.TopLeft();
					}
				private:
					static void DoCache(const Core::String&, TextFormatCache&, const TextRendering::Font*, double, double);
					static void RenderText(Renderer&, const Text&, const TextFormatCache&, bool);
					static Core::Collections::List<Core::Math::Rectangle> GetSelectionRegion(const Text&, size_t, size_t, const TextFormatCache&);
					static void HitTest(const Text&, const Core::Math::Vector2&, size_t&, size_t&, const TextFormatCache&);
					static Core::Math::Vector2 GetRelativeCaretPosition(const Text&, size_t, const TextFormatCache&);
			};
		}
	}
}
