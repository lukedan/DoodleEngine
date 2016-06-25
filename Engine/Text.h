#pragma once

#include "..\CoreWrap.h"
#include "Font.h"

namespace DE {
	namespace Graphics {
		namespace TextRendering {
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
			enum class LineWrapType { // TODO implement multiple line wrapping types
				NoWrap,
				WrapWords,
				WrapWordsNoOverflow,
				Wrap
			};

			class Text {
				public:
					virtual ~Text() {
					}

					virtual Core::Math::Vector2 GetSize() const = 0;

					virtual Core::Collections::List<Core::Math::Rectangle> GetSelectionRegion(size_t, size_t) const = 0;

					virtual size_t HitTestForCaret(const Core::Math::Vector2&) const = 0;
					virtual size_t HitTestForChar(const Core::Math::Vector2&) const = 0;
					virtual void HitTest(const Core::Math::Vector2&, size_t&, size_t&) const = 0;

					virtual Core::Math::Vector2 GetCaretPosition(size_t) const = 0;
					virtual Core::Math::Vector2 GetCaretPosition(size_t, double) const = 0; // with baseline information
					virtual double GetCaretHeight(size_t) const = 0;
					virtual void GetCaretInfo(size_t, Core::Math::Vector2&, double&) const = 0;
					virtual void GetCaretInfo(size_t, double, Core::Math::Vector2&, double&) const = 0;
					virtual double GetLineBegin(size_t) const = 0;
					virtual double GetLineEnd(size_t) const = 0;
					virtual size_t GetLineOfCaret(size_t) const = 0;
					virtual size_t GetLineOfCaret(size_t, double) const = 0;

					virtual size_t CaretLineUp(size_t, double) const = 0; // TODO deprecated
					virtual size_t CaretLineDown(size_t, double) const = 0;

					virtual void Render(Renderer&) const = 0;
			};

			class BasicText : public Text {
				public:
					struct BasicTextFormatCache {
						BasicTextFormatCache() = default;
						Core::Collections::List<size_t> LineBreaks;
						Core::Collections::List<double> LineLengths;
						Core::Math::Vector2 Size;
					};

					LineWrapType WrapType = LineWrapType::NoWrap;
					Core::String Content;
					double Scale = 1.0;
					BasicTextFormatCache FormatCache;
					bool FormatCached = false;
					const TextRendering::Font *Font = nullptr;
					Core::Math::Rectangle LayoutRectangle;
					HorizontalTextAlignment HorizontalAlignment = HorizontalTextAlignment::Left;
					VerticalTextAlignment VerticalAlignment = VerticalTextAlignment::Top;
					bool RoundToInteger = true, UseClip = false;
					Core::Math::Rectangle Clip, Padding {-2.0, -2.0, 4.0, 4.0};
					Core::Color TextColor;

					void CacheFormat();

					Core::Math::Vector2 GetSize() const override {
						if (FormatCached) {
							return FormatCache.Size + Core::Math::Vector2(Padding.Width(), Padding.Height());
						} else {
							BasicTextFormatCache t;
							DoCache(Content, t, Font, WrapType, LayoutRectangle.Width(), Scale);
							return t.Size + Core::Math::Vector2(Padding.Width(), Padding.Height());
						}
					}

					friend Renderer &operator <<(Renderer &r, const BasicText &txt) {
						txt.Render(r);
						return r;
					}
					void Render(Renderer&) const override;

					Core::Collections::List<Core::Math::Rectangle> GetSelectionRegion(size_t, size_t) const override;

					void HitTest(const Core::Math::Vector2&, size_t&, size_t&) const override;
					size_t HitTestForCaret(const Core::Math::Vector2 &pos) const override {
						size_t over, caret;
						HitTest(pos, over, caret);
						return caret;
					}
					size_t HitTestForChar(const Core::Math::Vector2 &pos) const {
						size_t over, caret;
						HitTest(pos, over, caret);
						return over;
					}

					void GetCaretInfo(size_t caret, Core::Math::Vector2 &pos, double &height) const override {
						if (Font) {
							pos = GetCaretPosition(caret);
							height = Font->GetHeight() * Scale;
						} else {
							pos = LayoutRectangle.TopLeft();
							height = 0.0;
						}
					}
					void GetCaretInfo(size_t caret, double baseline, Core::Math::Vector2 &pos, double &height) const override {
						if (Font) {
							pos = GetCaretPosition(caret, baseline);
							height = Font->GetHeight() * Scale;
						} else {
							pos = LayoutRectangle.TopLeft();
							height = 0.0;
						}
					}
					double GetLineBegin(size_t) const override;
					double GetLineEnd(size_t) const override;
					size_t GetLineOfCaret(size_t) const override;
					size_t GetLineOfCaret(size_t, double) const override;
					Core::Math::Vector2 GetRelativeCaretPosition(size_t, double) const;
					Core::Math::Vector2 GetRelativeCaretPosition(size_t) const;
					Core::Math::Vector2 GetCaretPosition(size_t caret) const override {
						return GetRelativeCaretPosition(caret) + LayoutRectangle.TopLeft();
					}
					Core::Math::Vector2 GetCaretPosition(size_t caret, double baseline) const override {
						return GetRelativeCaretPosition(caret, baseline) + LayoutRectangle.TopLeft();
					}
					double GetCaretHeight(size_t caret) const override {
						if (Font) {
							return Font->GetHeight() * Scale;
						}
						return 0.0;
					}
					size_t CaretLineUp(size_t, double) const override; // TODO deprecated
					size_t CaretLineDown(size_t, double) const override;
				private:
					static void DoCache(const Core::String&, BasicTextFormatCache&, const TextRendering::Font*, LineWrapType, double, double);
					static void DoRender(Renderer&, const BasicText&, const BasicTextFormatCache&, bool);
					static Core::Collections::List<Core::Math::Rectangle> DoGetSelectionRegion(const BasicText&, size_t, size_t, const BasicTextFormatCache&);
					static void DoHitTest(const BasicText&, const Core::Math::Vector2&, size_t&, size_t&, const BasicTextFormatCache&);
					static size_t DoCaretVerticalMove(const BasicText&, size_t, size_t, double, double, const BasicTextFormatCache&);
					static Core::Math::Vector2 DoGetRelativeCaretPosition(const BasicText&, size_t, bool, double, const BasicTextFormatCache&);
					static size_t DoGetLineOfCaret(const BasicText&, size_t, double, const BasicTextFormatCache&);
					static size_t DoGetLineOfCaret(const BasicText&, size_t, const BasicTextFormatCache&);
					static double DoGetLineBegin(const BasicText&, size_t, const BasicTextFormatCache&);
					static double DoGetLineEnd(const BasicText&, size_t, const BasicTextFormatCache&);
			};

			enum class StreamedRichTextParamChange {
				Color,
				Font,
				Scale
			};
			class StreamedRichText : public Text {
				public:
				protected:
			};
		}
	}
}
