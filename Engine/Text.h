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
			enum class LineWrapType {
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
					virtual Core::Math::Vector2 GetCaretSize(size_t) const = 0;
					virtual Core::Math::Vector2 GetCaretSize(size_t, double) const = 0;
					virtual Core::Math::Rectangle GetCaretInfo(size_t caret) const {
						Core::Math::Vector2 pos = GetCaretPosition(caret), sz = GetCaretSize(caret);
						return Core::Math::Rectangle(pos.X, pos.Y, sz.X, sz.Y);
					}
					virtual Core::Math::Rectangle GetCaretInfo(size_t caret, double baseline) const {
						Core::Math::Vector2 pos = GetCaretPosition(caret, baseline), sz = GetCaretSize(caret, baseline);
						return Core::Math::Rectangle(pos.X, pos.Y, sz.X, sz.Y);
					}

					virtual size_t GetLineOfCaret(size_t) const = 0;
					virtual size_t GetLineOfCaret(size_t, double) const = 0;
					virtual size_t GetLineNumber() const = 0;

					virtual void GetLineBeginning(size_t, size_t&, double&) const = 0;
					virtual void GetLineCursorEnding(size_t, size_t&, double&) const = 0;
					virtual void GetLineEnding(size_t, size_t&, double&) const = 0;
					virtual double GetLineTop(size_t) const = 0;
					virtual double GetLineHeight(size_t) const = 0;
					virtual double GetLineBottom(size_t line) const {
						return GetLineTop(line) + GetLineHeight(line);
					}

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
					Core::Math::Rectangle Clip, Padding {-4.0, -4.0, 8.0, 8.0};
					Core::Color TextColor;

					void CacheFormat();

					Core::Math::Vector2 GetSize() const override {
						if (FormatCached) {
							return FormatCache.Size + Padding.Size();
						} else {
							BasicTextFormatCache t;
							DoCache(*this, t);
							return t.Size + Padding.Size();
						}
					}

					void Render(Renderer&) const override;

					Core::Collections::List<Core::Math::Rectangle> GetSelectionRegion(size_t, size_t) const override;

					void HitTest(const Core::Math::Vector2&, size_t&, size_t&) const override;
					size_t HitTestForCaret(const Core::Math::Vector2 &pos) const override {
						size_t over, caret;
						HitTest(pos, over, caret);
						return caret;
					}
					size_t HitTestForChar(const Core::Math::Vector2 &pos) const override {
						size_t over, caret;
						HitTest(pos, over, caret);
						return over;
					}

					size_t GetLineOfCaret(size_t) const override;
					size_t GetLineOfCaret(size_t, double) const override;
					size_t GetLineNumber() const override;
					Core::Math::Vector2 GetRelativeCaretPosition(size_t, double) const;
					Core::Math::Vector2 GetRelativeCaretPosition(size_t) const;
					Core::Math::Vector2 GetCaretPosition(size_t caret) const override {
						return GetRelativeCaretPosition(caret) + LayoutRectangle.TopLeft();
					}
					Core::Math::Vector2 GetCaretPosition(size_t caret, double baseline) const override {
						return GetRelativeCaretPosition(caret, baseline) + LayoutRectangle.TopLeft();
					}
					Core::Math::Vector2 GetCaretSize(size_t caret) const override {
						if (Font) {
							return Core::Math::Vector2(Font->GetData(caret < Content.Length() ? Content[caret] : _TEXT('\n')).Advance, Font->GetHeight()) * Scale;
						}
						return Core::Math::Vector2();
					}
					Core::Math::Vector2 GetCaretSize(size_t, double) const override;
					Core::Math::Rectangle GetCaretInfo(size_t, double) const override;

					void GetLineBeginning(size_t, size_t&, double&) const override;
					void GetLineCursorEnding(size_t, size_t&, double&) const override;
					void GetLineEnding(size_t, size_t&, double&) const override;
					double GetLineTop(size_t) const override;
					double GetLineHeight(size_t) const override {
						return Font->GetHeight() * Scale;
					}
					double GetLineBottom(size_t) const override;
				private:
					static void DoCache(const BasicText&, BasicTextFormatCache&);
					static void DoRender(const BasicTextFormatCache&, const BasicText&, Renderer&);
					static Core::Collections::List<Core::Math::Rectangle> DoGetSelectionRegion(const BasicTextFormatCache&, const BasicText&, size_t, size_t);
					static void DoHitTest(const BasicTextFormatCache&, const BasicText&, const Core::Math::Vector2&, size_t&, size_t&);
					static Core::Math::Vector2 DoGetRelativeCaretPosition(
						const BasicTextFormatCache &cache,
						const BasicText &text,
						size_t caret,
						bool useBaseline,
						double baselinePos
					) {
						return DoGetRelativeCaretPositionImpl(
							cache, text, caret,
							(useBaseline ? DoGetLineOfCaret(cache, text, caret, baselinePos) : DoGetLineOfCaret(cache, text, caret))
						);
					}
					static Core::Math::Vector2 DoGetRelativeCaretPositionImpl(const BasicTextFormatCache&, const BasicText&, size_t, size_t);
					static Core::Math::Vector2 DoGetCaretSize(const BasicTextFormatCache &cache, const BasicText &txt, size_t caret, double baseline) {
						return DoGetCaretSizeImpl(cache, txt, caret, DoGetLineOfCaret(cache, txt, caret, baseline));
					}
					static Core::Math::Vector2 DoGetCaretSizeImpl(const BasicTextFormatCache&, const BasicText&, size_t, size_t);
					static Core::Math::Rectangle DoGetCaretInfoWithBaseline(const BasicTextFormatCache &cache, const BasicText &txt, size_t caret, double baseline) {
						size_t line = DoGetLineOfCaret(cache, txt, caret, baseline);
						Core::Math::Vector2
							pos = DoGetRelativeCaretPositionImpl(cache, txt, caret, line) + txt.LayoutRectangle.TopLeft(),
							sz = DoGetCaretSizeImpl(cache, txt, caret, line);
						return Core::Math::Rectangle(pos.X, pos.Y, sz.X, sz.Y);
					}
					static size_t DoGetLineOfCaret(const BasicTextFormatCache&, const BasicText&, size_t, double);
					static size_t DoGetLineOfCaret(const BasicTextFormatCache&, const BasicText&, size_t);
					static size_t DoGetLineNumber(const BasicTextFormatCache&, const BasicText&);
					static void DoGetLineBeginning(const BasicTextFormatCache&, const BasicText&, size_t, size_t&, double&);
					static void DoGetLineCursorEnding(const BasicTextFormatCache&, const BasicText&, size_t, size_t&, double&);
					static void DoGetLineEnding(const BasicTextFormatCache&, const BasicText&, size_t, size_t&, double&);
					static double DoGetLineTop(const BasicTextFormatCache&, const BasicText&, size_t);
					static double DoGetLineBottom(const BasicTextFormatCache&, const BasicText&, size_t);
			};

			class StreamedRichText : public Text {
				public:
					enum class ChangeType {
						Invalid,
						Scale,
						Font,
						Color,
						LocalVerticalPosition
					};
					union ChangeParameters {
						double NewScale;
						double LocalVerticalPosition;
						const Font *NewFont;
						struct {
							unsigned char R, G, B, A;
						} NewColor;
					};
					struct ChangeInfo {
						ChangeInfo() = default;
						ChangeInfo(size_t pos, ChangeType type) : Position(pos), Type(type) {
						}

						size_t Position = 0;
						ChangeType Type = ChangeType::Invalid;
						ChangeParameters Parameters;
					};

					struct TextFormatInfo {
						double Scale = 1.0, LocalVerticalPosition = 0.5;
						const Font *Font = nullptr;
						Core::Color Color;

						void ApplyChange(const ChangeInfo &change) {
							switch (change.Type) {
								case ChangeType::Scale: {
									Scale = change.Parameters.NewScale;
									break;
								}
								case ChangeType::Color: {
									Color = Core::Color(
										change.Parameters.NewColor.R,
										change.Parameters.NewColor.G,
										change.Parameters.NewColor.B,
										change.Parameters.NewColor.A
									);
									break;
								}
								case ChangeType::Font: {
									Font = change.Parameters.NewFont;
									break;
								}
								case ChangeType::LocalVerticalPosition: {
									LocalVerticalPosition = change.Parameters.LocalVerticalPosition;
									break;
								}
								default: {
									break;
								}
							}
						}
					};

					struct StreamedRichTextFormatCache {
						Core::Collections::List<double> LineLengths, LineHeights;
						Core::Collections::List<size_t> LineBreaks, LineEndChangeIDs;
						Core::Collections::List<TextFormatInfo> LineEndFormat;
						Core::Math::Vector2 Size;
					};

					Core::String Content;
					Core::Math::Rectangle LayoutRectangle, Padding {-4.0, -4.0, 8.0, 8.0};
					Core::Collections::List<ChangeInfo> Changes;
					LineWrapType WrapType = LineWrapType::NoWrap;
					HorizontalTextAlignment HorizontalAlignment = HorizontalTextAlignment::Left;
					VerticalTextAlignment VerticalAlignment = VerticalTextAlignment::Top;
					bool FormatCached = false;
					StreamedRichTextFormatCache CachedFormat;

					StreamedRichText &AppendScaleChange(double scale) {
						ChangeInfo ci(Content.Length(), ChangeType::Scale);
						ci.Parameters.NewScale = scale;
						Changes.PushBack(ci);
						return *this;
					}
					StreamedRichText &AppendFontChange(const Font *font) {
						ChangeInfo ci(Content.Length(), ChangeType::Font);
						ci.Parameters.NewFont = font;
						Changes.PushBack(ci);
						return *this;
					}
					StreamedRichText &AppendColorChange(const Core::Color &c) {
						ChangeInfo ci(Content.Length(), ChangeType::Color);
						ci.Parameters.NewColor.A = c.A;
						ci.Parameters.NewColor.R = c.R;
						ci.Parameters.NewColor.G = c.G;
						ci.Parameters.NewColor.B = c.B;
						Changes.PushBack(ci);
						return *this;
					}
					StreamedRichText &AppendLocalVerticalPositionChange(double vpos) {
						ChangeInfo ci(Content.Length(), ChangeType::LocalVerticalPosition);
						ci.Parameters.LocalVerticalPosition = vpos;
						Changes.PushBack(ci);
						return *this;
					}
					StreamedRichText &AppendText(const Core::String &text) {
						Content += text;
						return *this;
					}
					StreamedRichText &operator <<(const Core::String &text) {
						Content += text;
						return *this;
					}
					StreamedRichText &operator <<(const Font *fnt) {
						return AppendFontChange(fnt);
					}
					StreamedRichText &operator <<(const Core::Color &c) {
						return AppendColorChange(c);
					}

					virtual void CacheFormat() {
						DoCache(*this, CachedFormat);
						FormatCached = true;
					}
					Core::Math::Vector2 GetSize() const override;

					Core::Collections::List<Core::Math::Rectangle> GetSelectionRegion(size_t, size_t) const override;

					void HitTest(const Core::Math::Vector2&, size_t&, size_t&) const override;
					size_t HitTestForCaret(const Core::Math::Vector2 &pos) const override {
						size_t over, caret;
						HitTest(pos, over, caret);
						return caret;
					}
					size_t HitTestForChar(const Core::Math::Vector2 &pos) const override {
						size_t over, caret;
						HitTest(pos, over, caret);
						return over;
					}

					Core::Math::Vector2 GetCaretPosition(size_t caret) const override {
						return GetCaretInfo(caret).TopLeft();
					}
					Core::Math::Vector2 GetCaretPosition(size_t caret, double baseline) const override { // with baseline information
						return GetCaretInfo(caret, baseline).TopLeft();
					}
					Core::Math::Vector2 GetCaretSize(size_t caret) const override {
						return GetCaretInfo(caret).Size();
					}
					Core::Math::Vector2 GetCaretSize(size_t caret, double baseline) const override {
						return GetCaretInfo(caret, baseline).Size();
					}
					Core::Math::Rectangle GetCaretInfo(size_t) const override;
					Core::Math::Rectangle GetCaretInfo(size_t, double) const override;

					size_t GetLineOfCaret(size_t) const override;
					size_t GetLineOfCaret(size_t, double) const override;
					size_t GetLineNumber() const override;

					void GetLineBeginning(size_t, size_t&, double&) const override;
					void GetLineCursorEnding(size_t, size_t&, double&) const override;
					void GetLineEnding(size_t, size_t&, double&) const override;
					double GetLineTop(size_t) const override;
					double GetLineHeight(size_t) const override;

					void Render(Renderer&) const override;
				protected:
					static void DoCache(const StreamedRichText&, StreamedRichTextFormatCache&);
					static Core::Math::Vector2 DoGetSize(const StreamedRichTextFormatCache&, const StreamedRichText&);
					static void DoRender(const StreamedRichTextFormatCache&, const StreamedRichText&, Renderer&);
					static Core::Collections::List<Core::Math::Rectangle> DoGetSelectionRegion(const StreamedRichTextFormatCache&, const StreamedRichText&, size_t, size_t);
					static void DoHitTest(const StreamedRichTextFormatCache&, const StreamedRichText&, const Core::Math::Vector2&, size_t&, size_t&);
					static Core::Math::Rectangle DoGetCaretInfo(const StreamedRichTextFormatCache &cache, const StreamedRichText &txt, size_t caret) {
						return DoGetCaretInfoImpl(cache, txt, caret, DoGetLineOfCaret(cache, txt, caret));
					}
					static Core::Math::Rectangle DoGetCaretInfo(const StreamedRichTextFormatCache &cache, const StreamedRichText &txt, size_t caret, double baseline) {
						return DoGetCaretInfoImpl(cache, txt, caret, DoGetLineOfCaret(cache, txt, caret, baseline));
					}
					static Core::Math::Rectangle DoGetCaretInfoImpl(const StreamedRichTextFormatCache&, const StreamedRichText&, size_t, size_t);
					static size_t DoGetLineOfCaret(const StreamedRichTextFormatCache&, const StreamedRichText&, size_t, double);
					static size_t DoGetLineOfCaret(const StreamedRichTextFormatCache&, const StreamedRichText&, size_t);
					static size_t DoGetLineNumber(const StreamedRichTextFormatCache&, const StreamedRichText&);
					static void DoGetLineBeginning(const StreamedRichTextFormatCache&, const StreamedRichText&, size_t, size_t&, double&);
					static void DoGetLineCursorEnding(const StreamedRichTextFormatCache&, const StreamedRichText&, size_t, size_t&, double&);
					static void DoGetLineEnding(const StreamedRichTextFormatCache&, const StreamedRichText&, size_t, size_t&, double&);
					static double DoGetLineTop(const StreamedRichTextFormatCache&, const StreamedRichText&, size_t);
					static double DoGetLineHeight(const StreamedRichTextFormatCache&, const StreamedRichText&, size_t);
			};
			namespace TextFormatStreaming {
				struct NewScale {
					explicit NewScale(double d) : Scale(d) {
					}
					NewScale(const NewScale&) = delete;
					friend StreamedRichText &operator <<(StreamedRichText &txt, const NewScale &sc) {
						return txt.AppendScaleChange(sc.Scale);
					}
					double Scale;
				};
				struct NewFont {
					explicit NewFont(const Font *fnt) : Font(fnt) {
					}
					NewFont(const NewFont&) = delete;
					friend StreamedRichText &operator <<(StreamedRichText &txt, const NewFont &sc) {
						return txt.AppendFontChange(sc.Font);
					}
					const Font *Font;
				};
				struct NewColor {
					explicit NewColor(const Core::Color &c) : Color(c) {
					}
					NewColor(const NewColor&) = delete;
					friend StreamedRichText &operator <<(StreamedRichText &txt, const NewColor &sc) {
						return txt.AppendColorChange(sc.Color);
					}
					Core::Color Color;
				};
				struct NewLocalVerticalPosition {
					explicit NewLocalVerticalPosition(double v) : LocalVerticalPosition(v) {
					}
					NewLocalVerticalPosition(const NewLocalVerticalPosition&) = delete;
					friend StreamedRichText &operator <<(StreamedRichText &txt, const NewLocalVerticalPosition &lvp) {
						return txt.AppendLocalVerticalPositionChange(lvp.LocalVerticalPosition);
					}
					double LocalVerticalPosition;
				};
			}
		}
	}
}
