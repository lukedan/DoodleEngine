#include "Text.h"

namespace DE {
	namespace Graphics {
		namespace TextRendering {
			using namespace Core;
			using namespace Core::Math;
			using namespace Core::Collections;
			using namespace RenderingContexts;

			// local functions
			double _GetLineBegin(double lineLen, double layoutLeft, double layoutWidth, double lPad, double rPad, HorizontalTextAlignment align) {
				switch (align) {
					case HorizontalTextAlignment::Center: {
						return layoutLeft + 0.5 * (layoutWidth - lineLen - lPad - rPad);
					}
					case HorizontalTextAlignment::Right: {
						return layoutLeft + layoutWidth - lineLen - rPad;
					}
					default: {
						return layoutLeft - lPad;
					}
				}
			}
			double _GetLineBegin(double lineLen, const BasicText &txt) {
				return _GetLineBegin(
					lineLen, txt.LayoutRectangle.Left, txt.LayoutRectangle.Width(), txt.Padding.Left, txt.Padding.Right, txt.HorizontalAlignment
				);
			}
			double _GetRelativeLineBegin(double lineLen, const BasicText &txt) {
				return _GetLineBegin(
					lineLen, 0.0, txt.LayoutRectangle.Width(), txt.Padding.Left, txt.Padding.Right, txt.HorizontalAlignment
				);
			}
			double _GetLineEnd(double lineLen, double layoutLeft, double layoutWidth, double lPad, double rPad, HorizontalTextAlignment align) {
				switch (align) {
					case HorizontalTextAlignment::Center: {
						return layoutLeft + 0.5 * (layoutWidth + lineLen - lPad - rPad);
					}
					case HorizontalTextAlignment::Right: {
						return layoutLeft + layoutWidth - rPad;
					}
					default: {
						return layoutLeft + lineLen - lPad;
					}
				}
			}
			double _GetLineEnd(double lineLen, const BasicText &txt) {
				return _GetLineEnd(
					lineLen, txt.LayoutRectangle.Left, txt.LayoutRectangle.Width(), txt.Padding.Left, txt.Padding.Right, txt.HorizontalAlignment
				);
			}
			double _GetRelativeLineEnd(double lineLen, const BasicText &txt) {
				return _GetLineEnd(
					lineLen, 0.0, txt.LayoutRectangle.Width(), txt.Padding.Left, txt.Padding.Right, txt.HorizontalAlignment
				);
			}
			double _GetLayoutTop(double vertLen, double layoutTop, double layoutHeight, double tPad, double bPad, VerticalTextAlignment align) {
				switch (align) {
					case VerticalTextAlignment::Center: {
						return layoutTop + 0.5 * (layoutHeight - vertLen - tPad - bPad);
					}
					case VerticalTextAlignment::Bottom: {
						return layoutTop + layoutHeight - vertLen - bPad;
					}
					default: {
						return layoutTop - tPad;
					}
				}
			}
			double _GetLayoutTop(const BasicText::BasicTextFormatCache &cc, const BasicText &txt) {
				return _GetLayoutTop(
					cc.Size.Y, txt.LayoutRectangle.Top, txt.LayoutRectangle.Height(), txt.Padding.Top, txt.Padding.Bottom, txt.VerticalAlignment
				);
			}
			double _GetRelativeLayoutTop(const BasicText::BasicTextFormatCache &cc, const BasicText &txt) {
				return _GetLayoutTop(
					cc.Size.Y, 0.0, txt.LayoutRectangle.Height(), txt.Padding.Top, txt.Padding.Bottom, txt.VerticalAlignment
				);
			}

			void _AddRectangleToListWithClip(List<Math::Rectangle> &list, const Math::Rectangle &r, const Math::Rectangle &clip, bool useClip) {
				if (useClip) {
					Math::Rectangle isect;
					if (Math::Rectangle::Intersect(clip, r, isect) != IntersectionType::None) {
						list.PushBack(isect);
					}
				} else {
					list.PushBack(r);
				}
			}

			void BasicText::DoCache(
				const BasicText &txt,
				BasicTextFormatCache &cache
			) {
				cache.LineBreaks.Clear();
				cache.LineLengths.Clear();
				cache.Size = Vector2();
				if (txt.Font == nullptr) {
					return;
				}
				bool hasBreakable = false;
				size_t lastBreakable = 0;
				double curLineLen = 0.0, lastBreakableLen = 0.0;
				CharData curData;
				for (size_t i = 0; i < txt.Content.Length(); ++i) {
					curData = txt.Font->GetData(txt.Content[i]);
					curLineLen += curData.Advance * txt.Scale;
					if (curLineLen + txt.Padding.Width() > txt.LayoutRectangle.Width() && i != (cache.LineBreaks.Count() == 0 ? 0 : cache.LineBreaks.Last() + 1)) {
						if (!(txt.WrapType == LineWrapType::WrapWords && !hasBreakable) && txt.WrapType != LineWrapType::NoWrap) {
							// if the current line can be wrapped
							if (txt.WrapType == LineWrapType::Wrap || (txt.WrapType == LineWrapType::WrapWordsNoOverflow && !hasBreakable)) {
								// coerce wrapping
								if (txt.Content[i] == _TEXT('\n')) {
									lastBreakable = i - 2;
									lastBreakableLen = curLineLen - (curData.Advance + txt.Font->GetData(txt.Content[i - 1]).Advance) * txt.Scale;
								} else {
									lastBreakable = i - 1;
									lastBreakableLen = curLineLen - curData.Advance * txt.Scale;
								}
							}
							cache.LineBreaks.PushBack(lastBreakable);
							cache.LineLengths.PushBack(lastBreakableLen);
							if (lastBreakableLen > cache.Size.X) {
								cache.Size.X = lastBreakableLen;
							}
							hasBreakable = false;
							curLineLen = 0.0;
							i = lastBreakable;
							continue;
						}
					}
					switch (txt.Content[i]) {
						case _TEXT('\n'): {
							cache.LineBreaks.PushBack(i);
							cache.LineLengths.PushBack(curLineLen);
							if (curLineLen > cache.Size.X) {
								cache.Size.X = curLineLen;
							}
							hasBreakable = false;
							curLineLen = 0.0;
							break;
						}
						default: {
							TCHAR curc = txt.Content[i];
							if (!(
								(curc >= _TEXT('A') && curc <= _TEXT('Z')) ||
								(curc >= _TEXT('a') && curc <= _TEXT('z')) // TODO substitute this with a formal 'is a breakable char' condition
							)) {
								hasBreakable = true;
								lastBreakable = i;
								lastBreakableLen = curLineLen;
							}
							break;
						}
					}
				}
				cache.LineLengths.PushBack(curLineLen);
				if (curLineLen > cache.Size.X) {
					cache.Size.X = curLineLen;
				}
				cache.Size.Y = cache.LineLengths.Count() * txt.Font->GetHeight() * txt.Scale;
			}
			void BasicText::DoRender(const BasicTextFormatCache &cc, const BasicText &txt, Renderer &r, bool roundToInt) {
				if (r.GetContext() == nullptr || txt.Font == nullptr || txt.Content.Empty() || cc.LineLengths.Count() == 0) {
					return;
				}
				size_t curB = 0;
				Vector2 pos(
					_GetLineBegin(cc.LineLengths[0], txt),
					_GetLayoutTop(cc, txt)
				);
				pos.X = _GetLineBegin(cc.LineLengths[0], txt);
				List<Vertex> vs;
				size_t lstpg = txt.Font->GetTextureInfo(txt.Content[0]).Page;
				for (size_t i = 0; i < txt.Content.Length(); ++i) { // TODO: improve performance
					const CharData &data = txt.Font->GetData(txt.Content[i]);
					AtlasTexture ctex = txt.Font->GetTextureInfo(data.Character);
					Vector2 aRPos = pos;
					if (roundToInt) {
						aRPos = Vector2(round(pos.X), round(pos.Y));
					}
					bool drd = true;
					Math::Rectangle charRect = data.Placement, realUV = ctex.UVRect;
					charRect.Scale(Vector2(), txt.Scale);
					charRect.Translate(aRPos);
					if (txt.UseClip) {
						Math::Rectangle isectRect;
						if (Math::Rectangle::Intersect(txt.Clip, charRect, isectRect) == IntersectionType::Full) {
							realUV.Scale(charRect, isectRect);
							charRect = isectRect;
						} else {
							drd = false;
						}
					}
					if (drd) {
						if (ctex.Page != lstpg) {
							r<<txt.Font->GetTexture(lstpg);
							r.DrawVertices(vs, RenderMode::Triangles);
							lstpg = ctex.Page;
							vs.Clear();
						}
						vs.PushBack(Vertex(charRect.TopLeft(), txt.TextColor, realUV.TopLeft()));
						vs.PushBack(Vertex(charRect.TopRight(), txt.TextColor, realUV.TopRight()));
						vs.PushBack(Vertex(charRect.BottomLeft(), txt.TextColor, realUV.BottomLeft()));
						vs.PushBack(Vertex(charRect.TopRight(), txt.TextColor, realUV.TopRight()));
						vs.PushBack(Vertex(charRect.BottomLeft(), txt.TextColor, realUV.BottomLeft()));
						vs.PushBack(Vertex(charRect.BottomRight(), txt.TextColor, realUV.BottomRight()));
					}
                    pos.X += data.Advance * txt.Scale;
					if (curB < cc.LineBreaks.Count() && cc.LineBreaks[curB] == i) {
						++curB;
						pos.X = _GetLineBegin(cc.LineLengths[curB], txt);
						pos.Y += txt.Font->GetHeight() * txt.Scale;
					}
				}
				if (vs.Count() > 0) {
					r<<txt.Font->GetTexture(lstpg);
					r.DrawVertices(vs, RenderMode::Triangles);
				}
				r<<Texture();
			}
			List<Math::Rectangle> BasicText::DoGetSelectionRegion(const BasicTextFormatCache &cc, const BasicText &txt, size_t start, size_t end) {
				if (start >= end) {
					return List<Math::Rectangle>();
				}
				size_t sl = DoGetLineOfCaret(cc, txt, start), el = DoGetLineOfCaret(cc, txt, end - 1);
				double
					lineHeight = txt.Font->GetHeight() * txt.Scale,
					top = _GetLayoutTop(cc, txt),
					sll = _GetLineBegin(cc.LineLengths[sl], txt),
					ell = sll;
				size_t
					slc = (sl > 0 ? cc.LineBreaks[sl - 1] + 1 : 0),
					elc = slc;
				if (sl != el) {
					ell = _GetLineBegin(cc.LineLengths[el], txt);
					elc = (el > 0 ? cc.LineBreaks[el - 1] + 1 : 0);
				}
				for (; slc < start; ++slc) {
					sll += txt.Font->GetData(txt.Content[slc]).Advance * txt.Scale;
				}
				for (; elc < end; ++elc) {
					ell += txt.Font->GetData(txt.Content[elc]).Advance * txt.Scale;
				}
				List<Math::Rectangle> result;
				if (sl == el) {
					_AddRectangleToListWithClip(result, Math::Rectangle(sll, top + lineHeight * sl, ell - sll, lineHeight), txt.Clip, txt.UseClip);
				} else {
					_AddRectangleToListWithClip(result, Math::Rectangle(
						sll,
						top += lineHeight * sl,
						_GetLineEnd(cc.LineLengths[sl], txt) - sll,
						lineHeight
					), txt.Clip, txt.UseClip);
					for (size_t x = sl + 1; x < el; ++x) {
						_AddRectangleToListWithClip(result, Math::Rectangle(
							_GetLineBegin(cc.LineLengths[x], txt),
							top += lineHeight,
							cc.LineLengths[x],
							lineHeight
						), txt.Clip, txt.UseClip);
					}
					double l = _GetLineBegin(cc.LineLengths[el], txt);
					_AddRectangleToListWithClip(result, Math::Rectangle(
						l,
						top + lineHeight,
						ell - l,
						lineHeight
					), txt.Clip, txt.UseClip);
				}
				return result;
			}
			void BasicText::DoHitTest(const BasicTextFormatCache &cc, const BasicText &txt, const Vector2 &pos, size_t &over, size_t &caret) {
				double top = _GetLayoutTop(cc, txt);
				if (pos.Y <= top) {
					over = caret = 0;
					return;
				}
				size_t
					line = static_cast<size_t>(floor((pos.Y - top) / (txt.Font->GetHeight() * txt.Scale)));
				if (line > cc.LineBreaks.Count()) {
					over = caret = txt.Content.Length();
					return;
				}
				size_t
					lbeg = (line > 0 ? cc.LineBreaks[line - 1] + 1 : 0),
					lend = (line < cc.LineBreaks.Count() ? cc.LineBreaks[line] + 1 : txt.Content.Length());
				if (lbeg == lend) { // this is the last line, empty
					over = (caret = txt.Content.Length()) - 1;
					return;
				}
				double sx = _GetLineBegin(cc.LineLengths[line], txt);
				if (pos.X <= sx) {
					over = caret = lbeg;
					return;
				}
				for (; lbeg < lend; ++lbeg) {
					TCHAR c = txt.Content[lbeg];
					double adv = txt.Font->GetData(c).Advance * txt.Scale;
					if (pos.X <= sx + adv) {
						over = lbeg;
						caret = (c != _TEXT('\n') && pos.X > sx + 0.5 * adv ? lbeg + 1 : lbeg);
						return;
					}
					sx += adv;
				}
				if (txt.Content[lend - 1] == _TEXT('\n')) {
					over = caret = lend - 1;
				} else {
					over = (caret = lend) - 1;
				}
			}
			Vector2 BasicText::DoGetRelativeCaretPosition(
				const BasicTextFormatCache &cache,
				const BasicText &text,
				size_t caret,
				bool useBaseline,
				double baselinePos
			) {
				size_t line = (useBaseline ? DoGetLineOfCaret(cache, text, caret, baselinePos) : DoGetLineOfCaret(cache, text, caret));
				Vector2 pos(0.0, line * text.Font->GetHeight()); // no need to multiply Scale, for it's done later
				for (size_t ls = (line == 0 ? 0 : cache.LineBreaks[line - 1] + 1); ls < caret; ++ls) {
					pos.X += text.Font->GetData(text.Content[ls]).Advance;
				}
				return Vector2(
					_GetRelativeLineBegin(cache.LineLengths[line], text),
					_GetRelativeLayoutTop(cache, text)
				) + pos * text.Scale;
			}
			size_t BasicText::DoGetLineOfCaret(
				const BasicTextFormatCache &cache,
				const BasicText &text,
				size_t caret,
				double baselinePos
			) {
				size_t line = 0;
				bool end = false;
				TCHAR endchar = _TEXT('\n');
				cache.LineBreaks.ForEach([&](size_t breakpos) {
					if (breakpos < caret) {
						++line;
						if (breakpos + 1 == caret) {
							end = true;
							endchar = text.Content[breakpos];
						}
						return true;
					}
					return false;
				});
				if (end && endchar != _TEXT('\n')) { // very suspicious
					double midpvt = cache.LineLengths[line - 1];
					midpvt = _GetLineBegin(midpvt, text) + 0.5 * midpvt;
					if (baselinePos > midpvt) {
						--line;
					}
				}
				return line;
			}
			size_t BasicText::DoGetLineOfCaret(const BasicTextFormatCache &cache, const BasicText &text, size_t caret) {
				size_t line = 0;
				cache.LineBreaks.ForEach([&](size_t pos) {
					if (pos < caret) {
						++line;
						return true;
					}
					return false;
				});
				return line;
			}
			size_t BasicText::DoGetLineNumber(const BasicTextFormatCache &cache, const BasicText &text) {
				return cache.LineLengths.Count();
			}
			void BasicText::DoGetLineBeginning(const BasicTextFormatCache &cache, const BasicText &text, size_t line, size_t &caret, double &pos) {
				if (line == 0) {
					caret = 0;
					pos = _GetLineBegin(cache.LineLengths[0], text);
				} else {
					caret = cache.LineBreaks[line - 1] + 1;
					pos = _GetLineBegin(cache.LineLengths[line], text);
				}
			}
			void BasicText::DoGetLineCursorEnding(const BasicTextFormatCache &cache, const BasicText &text, size_t line, size_t &caret, double &pos) {
				if (line >= cache.LineBreaks.Count()) {
					caret = text.Content.Length();
					pos = _GetLineEnd(cache.LineLengths.Last(), text);
				} else {
					size_t lb = cache.LineBreaks[line];
					if (text.Content[lb] == _TEXT('\n')) {
						caret = lb;
						pos = _GetLineEnd(cache.LineLengths[line], text) - text.Font->GetData(_TEXT('\n')).Advance * text.Scale;
					} else {
						caret = lb + 1;
						pos = _GetLineEnd(cache.LineLengths[line], text);
					}
				}
			}
			void BasicText::DoGetLineEnding(const BasicTextFormatCache &cache, const BasicText &text, size_t line, size_t &caret, double &pos) {
				if (line >= cache.LineBreaks.Count()) {
					caret = text.Content.Length();
					pos = _GetLineEnd(cache.LineLengths.Last(), text);
				} else {
					caret = cache.LineBreaks[line] + 1;
					pos = _GetLineEnd(cache.LineLengths[line], text);
				}
			}
			double BasicText::DoGetLineTop(const BasicTextFormatCache &cache, const BasicText &text, size_t line) {
				if (line >= cache.LineLengths.Count()) {
					throw DE::Core::OverflowException(_TEXT("the line number is too large"));
				}
				return line * text.Font->GetHeight() * text.Scale + _GetLayoutTop(cache, text);
			}
			double BasicText::DoGetLineBottom(const BasicTextFormatCache &cache, const BasicText &text, size_t line) {
				if (line >= cache.LineLengths.Count()) {
					throw DE::Core::OverflowException(_TEXT("the line number is too large"));
				}
				return (line + 1) * text.Font->GetHeight() * text.Scale + _GetLayoutTop(cache, text);
			}

			void BasicText::CacheFormat() {
				DoCache(*this, FormatCache);
				FormatCached = true;
			}

#define BASICTEXT_NEEDCACHE_FUNC_IMPL_BASE(FUNC, PARAMS...)   \
	if (Font) {                                               \
		if (FormatCached) {                                   \
			FUNC(FormatCache, PARAMS);                        \
		} else {                                              \
			BasicTextFormatCache cc;                          \
			DoCache(*this, cc);                               \
			FUNC(cc, PARAMS);                                 \
		}                                                     \
	}                                                         \

#define BASICTEXT_NEEDCACHE_FUNC_IMPL(FUNC, PARAMS...) BASICTEXT_NEEDCACHE_FUNC_IMPL_BASE(FUNC, *this, PARAMS)
#define BASICTEXT_NEEDCACHE_FUNC_IMPL_NOPARAM(FUNC) BASICTEXT_NEEDCACHE_FUNC_IMPL_BASE(FUNC, *this)
			void BasicText::Render(Renderer &r) const {
				BASICTEXT_NEEDCACHE_FUNC_IMPL(DoRender, r, RoundToInteger);
			}
			Core::Collections::List<Core::Math::Rectangle> BasicText::GetSelectionRegion(size_t start, size_t end) const {
				if (start > Content.Length() || end > Content.Length() || start > end) {
					throw InvalidArgumentException(_TEXT("index overflow"));
				}
				BASICTEXT_NEEDCACHE_FUNC_IMPL(return DoGetSelectionRegion, start, end);
				return Core::Collections::List<Core::Math::Rectangle>();
			}
			size_t BasicText::GetLineOfCaret(size_t caret) const {
				BASICTEXT_NEEDCACHE_FUNC_IMPL(return DoGetLineOfCaret, caret);
				return 0;
			}
			size_t BasicText::GetLineOfCaret(size_t caret, double baseline) const {
				BASICTEXT_NEEDCACHE_FUNC_IMPL(return DoGetLineOfCaret, caret, baseline);
				return 0;
			}
			size_t BasicText::GetLineNumber() const {
				BASICTEXT_NEEDCACHE_FUNC_IMPL_NOPARAM(return DoGetLineNumber);
				return 0;
			}
			void BasicText::GetLineBeginning(size_t line, size_t &caret, double &pos) const {
				BASICTEXT_NEEDCACHE_FUNC_IMPL(DoGetLineBeginning, line, caret, pos);
			}
			void BasicText::GetLineCursorEnding(size_t line, size_t &caret, double &pos) const {
				BASICTEXT_NEEDCACHE_FUNC_IMPL(DoGetLineCursorEnding, line, caret, pos);
			}
			void BasicText::GetLineEnding(size_t line, size_t &caret, double &pos) const {
				BASICTEXT_NEEDCACHE_FUNC_IMPL(DoGetLineEnding, line, caret, pos);
			}
			void BasicText::HitTest(const Vector2 &pos, size_t &over, size_t &caret) const {
				BASICTEXT_NEEDCACHE_FUNC_IMPL(DoHitTest, pos, over, caret);
			}
			Vector2 BasicText::GetRelativeCaretPosition(size_t caret) const {
				if (caret > Content.Length()) {
					throw InvalidArgumentException(_TEXT("caret index overflow"));
				}
				BASICTEXT_NEEDCACHE_FUNC_IMPL(return DoGetRelativeCaretPosition, caret, false, 0.0);
				return Vector2();
			}
			Vector2 BasicText::GetRelativeCaretPosition(size_t caret, double baseline) const {
				if (caret > Content.Length()) {
					throw InvalidArgumentException(_TEXT("caret index overflow"));
				}
				BASICTEXT_NEEDCACHE_FUNC_IMPL(return DoGetRelativeCaretPosition, caret, true, baseline);
				return Vector2();
			}
			double BasicText::GetLineTop(size_t line) const {
				BASICTEXT_NEEDCACHE_FUNC_IMPL(return DoGetLineTop, line);
				return LayoutRectangle.Top;
			}
			double BasicText::GetLineBottom(size_t line) const {
				BASICTEXT_NEEDCACHE_FUNC_IMPL(return DoGetLineBottom, line);
				return LayoutRectangle.Top;
			}
#undef BASICTEXT_NEEDCACHE_FUNC_IMPL_BASE
#undef BASICTEXT_NEEDCACHE_FUNC_IMPL
#undef BASICTEXT_NEEDCACHE_FUNC_IMPL_NOPARAM
		}
	}
}
