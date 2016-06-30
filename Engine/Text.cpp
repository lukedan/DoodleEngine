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

			// auxiliary functions for BasicText
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

			void BasicText::DoCache(
				const BasicText &txt,
				BasicTextFormatCache &cache
			) {
#define BASICTEXT_SET_LASTBREAK_TO_CURRENT { lbw = curw; lbid = i; }
#define BASICTEXT_ON_NEWLINE(WIDTH) { cache.LineLengths.PushBack(WIDTH); if (WIDTH > cache.Size.X) { cache.Size.X = WIDTH; } }
				cache.LineBreaks.Clear();
				cache.LineLengths.Clear();
				cache.Size = Vector2();

				if (!txt.Font) {
					return;
				}
				double lbw = 0.0, curw = 0.0;
				bool hasBreakable = false, hasBreakableChar = false;
				size_t lbid = 0;
				for (size_t i = 0; i < txt.Content.Length(); ++i) {
					TCHAR curc = txt.Content[i];
					const CharData &cData = txt.Font->GetData(curc);
					curw += cData.Advance * txt.Scale;
					bool breaknow = curw > txt.LayoutRectangle.Width() - txt.Padding.Width() && hasBreakable;
					if (curc == _TEXT('\n') && !breaknow) {
						breaknow = true;
						BASICTEXT_SET_LASTBREAK_TO_CURRENT;
					}
					if (breaknow) {
						cache.LineBreaks.PushBack(lbid);
						BASICTEXT_ON_NEWLINE(lbw);
						// restore last break scene
						i = lbid;
						hasBreakable = hasBreakableChar = false;
						curw = 0.0;
						continue;
					}
					// curc != '\n'
					bool curBreakable = (txt.WrapType == LineWrapType::Wrap);
					if ( // TODO formality
						(txt.Content[i] >= _TEXT('a') && txt.Content[i] <= _TEXT('z')) ||
						(txt.Content[i] >= _TEXT('A') && txt.Content[i] <= _TEXT('Z'))
					) { // not a breakable character
						if (!hasBreakableChar && txt.WrapType == LineWrapType::WrapWordsNoOverflow) {
							curBreakable = true;
						}
					} else {
						if (txt.WrapType != LineWrapType::NoWrap) {
							curBreakable = true;
							hasBreakableChar = true;
						}
					}
					if (i + 1 < txt.Content.Length() && txt.Content[i + 1] == _TEXT('\n')) {
						curBreakable = false;
					}
					if (curBreakable) {
						hasBreakable = true;
						BASICTEXT_SET_LASTBREAK_TO_CURRENT;
					}
				}
				BASICTEXT_ON_NEWLINE(curw);
				cache.Size.Y = cache.LineLengths.Count() * txt.Font->GetHeight() * txt.Scale;
#undef BASICTEXT_SET_LASTBREAK_TO_CURRENT
#undef BASICTEXT_ON_NEWLINE
			}
			void BasicText::DoRender(const BasicTextFormatCache &cc, const BasicText &txt, Renderer &r) {
				if (txt.Content.Empty() || txt.Font == nullptr || cc.LineLengths.Count() == 0) {
					return;
				}
				size_t curB = 0;
				Vector2 pos(_GetLineBegin(cc.LineLengths[0], txt), _GetLayoutTop(cc, txt));
				List<Vertex> vs;
				size_t lstpg = txt.Font->GetTextureInfo(txt.Content[0]).Page;
				for (size_t i = 0; i < txt.Content.Length(); ++i) { // TODO: improve performance
					const CharData &data = txt.Font->GetData(txt.Content[i]);
					AtlasTexture ctex = txt.Font->GetTextureInfo(data.Character);
					Vector2 aRPos = pos;
					if (txt.RoundToInteger) {
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
							r.SetTexture(txt.Font->GetTexture(lstpg));
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
					r.SetTexture(txt.Font->GetTexture(lstpg));
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
				if (caret > text.Content.Length()) {
					throw InvalidArgumentException(_TEXT("caret index overflow"));
				}
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
				cache.LineBreaks.ForEach([&](size_t breakpos) {
					if (breakpos < caret) {
						++line;
						if (breakpos + 1 == caret) {
							end = true;
							return false;
						}
						return true;
					}
					return false;
				});
				if (end && text.Content[caret - 1] != _TEXT('\n')) { // very suspicious
					double midpvt = cache.LineLengths[line - 1];
					midpvt = _GetLineBegin(midpvt, text) + 0.5 * midpvt;
					if (baselinePos > midpvt) {
						--line;
					}
				}
				return line;
			}
			size_t BasicText::DoGetLineOfCaret(const BasicTextFormatCache &cache, const BasicText&, size_t caret) {
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
			size_t BasicText::DoGetLineNumber(const BasicTextFormatCache &cache, const BasicText&) {
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
			void BasicText::DoGetLineCursorEnding(const BasicTextFormatCache &cache, const BasicText &text, size_t line, size_t &caret, double &pos) { // FIXME bug here
				pos = _GetLineEnd(cache.LineLengths[line], text);
				caret = (line < cache.LineBreaks.Count() ? cache.LineBreaks[line] : text.Content.Length());
				if (line < cache.LineBreaks.Count() && text.Content[caret] == _TEXT('\n')) {
					pos -= text.Font->GetData(_TEXT('\n')).Advance * text.Scale;
				} else {
					++caret;
				}
			}
			void BasicText::DoGetLineEnding(const BasicTextFormatCache &cache, const BasicText &text, size_t line, size_t &caret, double &pos) {
				caret = (line >= cache.LineBreaks.Count() ? text.Content.Length() : cache.LineBreaks[line] + 1);
				pos = _GetLineEnd(cache.LineLengths[line], text);
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

#define BASICTEXT_NEEDCACHE_FUNC_IMPL_BASE(FUNC, ...)   \
	if (Font) {                                         \
		if (FormatCached) {                             \
			FUNC(FormatCache, __VA_ARGS__);             \
		} else {                                        \
			BasicTextFormatCache cc;                    \
			DoCache(*this, cc);                         \
			FUNC(cc, __VA_ARGS__);                      \
		}                                               \
	}                                                   \

#define BASICTEXT_NEEDCACHE_FUNC_IMPL(FUNC, ...) BASICTEXT_NEEDCACHE_FUNC_IMPL_BASE(FUNC, *this, __VA_ARGS__)
#define BASICTEXT_NEEDCACHE_FUNC_IMPL_NOPARAM(FUNC) BASICTEXT_NEEDCACHE_FUNC_IMPL_BASE(FUNC, *this)
			void BasicText::Render(Renderer &r) const {
				BASICTEXT_NEEDCACHE_FUNC_IMPL(DoRender, r);
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
				BASICTEXT_NEEDCACHE_FUNC_IMPL(return DoGetRelativeCaretPosition, caret, false, 0.0);
				return Vector2();
			}
			Vector2 BasicText::GetRelativeCaretPosition(size_t caret, double baseline) const {
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

			// auxiliary functions for StreamedRichText
			double _GetLineBegin(double lineLen, const StreamedRichText &txt) {
				return _GetLineBegin(
					lineLen, txt.LayoutRectangle.Left, txt.LayoutRectangle.Width(), txt.Padding.Left, txt.Padding.Right, txt.HorizontalAlignment
				);
			}
			double _GetRelativeLineBegin(double lineLen, const StreamedRichText &txt) {
				return _GetLineBegin(
					lineLen, 0.0, txt.LayoutRectangle.Width(), txt.Padding.Left, txt.Padding.Right, txt.HorizontalAlignment
				);
			}
			double _GetLineEnd(double lineLen, const StreamedRichText &txt) {
				return _GetLineEnd(
					lineLen, txt.LayoutRectangle.Left, txt.LayoutRectangle.Width(), txt.Padding.Left, txt.Padding.Right, txt.HorizontalAlignment
				);
			}
			double _GetRelativeLineEnd(double lineLen, const StreamedRichText &txt) {
				return _GetLineEnd(
					lineLen, 0.0, txt.LayoutRectangle.Width(), txt.Padding.Left, txt.Padding.Right, txt.HorizontalAlignment
				);
			}
			double _GetLayoutTop(const StreamedRichText::StreamedRichTextFormatCache &cc, const StreamedRichText &txt) {
				return _GetLayoutTop(
					cc.Size.Y, txt.LayoutRectangle.Top, txt.LayoutRectangle.Height(), txt.Padding.Top, txt.Padding.Bottom, txt.VerticalAlignment
				);
			}
			double _GetRelativeLayoutTop(const StreamedRichText::StreamedRichTextFormatCache &cc, const StreamedRichText &txt) {
				return _GetLayoutTop(
					cc.Size.Y, 0.0, txt.LayoutRectangle.Height(), txt.Padding.Top, txt.Padding.Bottom, txt.VerticalAlignment
				);
			}

			void StreamedRichText::DoCache(const StreamedRichText &txt, StreamedRichTextFormatCache &cache) { // idea: update lastBreak to make sure it's always valid
				cache.LineBreaks.Clear();
				cache.LineHeights.Clear();
				cache.LineLengths.Clear();
				cache.LineEndFormat.Clear();
				cache.LineEndChangeIDs.Clear();
				cache.Size = Vector2();

				TextFormatInfo curInfo, lastBreakInfo;
				double lbw = 0.0, lbh = 0.0, curw = 0.0, curh = 0.0, maxh = 0.0;
				bool hasBreakable = false, hasBreakableChar = false;
				size_t markID = 0, breakMarkID = 0, lbid = 0;
				for (size_t i = 0; i < txt.Content.Length(); ++i) {
					TCHAR curc = txt.Content[i];
					for (; markID < txt.Changes.Count() && txt.Changes[markID].Position == i; ++markID) {
						const ChangeInfo &ci = txt.Changes[markID];
						curInfo.ApplyChange(ci);
						if (ci.Type == ChangeType::Font) {
							if (curInfo.Font) {
								if ((curh = curInfo.Font->GetHeight()) > maxh) {
									maxh = curh;
								}
							} else {
								curh = 0.0;
							}
						}
					}
					if (!curInfo.Font) { // skip this char if no font specified
						continue;
					}
					const CharData &cData = curInfo.Font->GetData(curc);
					curw += cData.Advance * curInfo.Scale;
					bool breaknow = curw > txt.LayoutRectangle.Width() - txt.Padding.Width() && hasBreakable;
					if (curc == _TEXT('\n') && !breaknow) {
						breaknow = true;
						lastBreakInfo = curInfo;
						lbw = curw;
						lbh = maxh;
						breakMarkID = markID;
						lbid = i;
					}
					if (breaknow) {
						cache.LineBreaks.PushBack(lbid);
						cache.LineHeights.PushBack(lbh);
						cache.LineLengths.PushBack(lbw);
						cache.LineEndFormat.PushBack(lastBreakInfo);
						cache.LineEndChangeIDs.PushBack(breakMarkID);
						cache.Size.Y += lbh;
						if (lbw > cache.Size.X) {
							cache.Size.X = lbw;
						}
						// restore last break scene
						i = lbid;
						curInfo = lastBreakInfo;
						markID = breakMarkID;
						hasBreakable = hasBreakableChar = false;
						curw = 0.0;
						maxh = curh = (curInfo.Font ? curInfo.Font->GetHeight() : 0.0);
						continue;
					}
					// curc != '\n'
					bool curBreakable = (txt.WrapType == LineWrapType::Wrap);
					if (
						(txt.Content[i] >= _TEXT('a') && txt.Content[i] <= _TEXT('z')) ||
						(txt.Content[i] >= _TEXT('A') && txt.Content[i] <= _TEXT('Z'))
					) { // not a breakable character
						if (!hasBreakableChar && txt.WrapType == LineWrapType::WrapWordsNoOverflow) {
							curBreakable = true;
						}
					} else {
						if (txt.WrapType != LineWrapType::NoWrap) {
							curBreakable = true;
							hasBreakableChar = true;
						}
					}
					if (i + 1 < txt.Content.Length() && txt.Content[i + 1] == _TEXT('\n')) {
						curBreakable = false;
					}
					if (curBreakable) {
						hasBreakable = true;
						lastBreakInfo = curInfo;
						lbw = curw;
						lbh = maxh;
						breakMarkID = markID;
						lbid = i;
					}
				}
				cache.Size.Y += maxh;
				if (curw > cache.Size.X) {
					cache.Size.X = curw;
				}
				cache.LineHeights.PushBack(maxh);
				cache.LineLengths.PushBack(curw);
				cache.LineEndFormat.PushBack(curInfo);
			}
			Vector2 StreamedRichText::DoGetSize(const StreamedRichTextFormatCache &cache, const StreamedRichText &txt) {
				return cache.Size;
			}
			void StreamedRichText::DoRender(const StreamedRichTextFormatCache &cache, const StreamedRichText &txt, Renderer &r) {
				if (cache.LineLengths.Count() == 0) {
					return;
				}
				List<Vertex> vxs;
				size_t lastPage = 0, changeID = 0, curLine = 0;
				Vector2 topLeft(_GetLineBegin(cache.LineLengths[curLine], txt), _GetLayoutTop(cache, txt));
				TextFormatInfo ti;
				for (size_t i = 0; i < txt.Content.Length(); ++i) {
					while (changeID < txt.Changes.Count() && i == txt.Changes[changeID].Position) {
						if (txt.Changes[changeID].Type == ChangeType::Font) {
							if (ti.Font && vxs.Count() > 0) {
								r.SetTexture(ti.Font->GetTexture(lastPage));
								r.DrawVertices(vxs, RenderMode::Triangles);
								vxs.Clear();
							}
						}
						ti.ApplyChange(txt.Changes[changeID]);
					}
					if (ti.Font) {
						const CharData &cd = ti.Font->GetData(txt.Content[i]);
						const AtlasTexture &atex = ti.Font->GetTextureInfo(txt.Content[i]);
						if (atex.Page != lastPage) {
							if (vxs.Count() > 0) {
								r.SetTexture(ti.Font->GetTexture(lastPage));
								r.DrawVertices(vxs, RenderMode::Triangles);
								vxs.Clear();
							}
							lastPage = atex.Page;
						}
						Math::Rectangle rect = cd.Placement;
						Vector2 diff = topLeft;
						diff.Y += (cache.LineHeights[curLine] - ti.Font->GetHeight()) * ti.LocalVerticalPosition;
						rect.Scale(Vector2(), ti.Scale);
						rect.Translate(diff);
						vxs.PushBack(Vertex(rect.TopLeft(), ti.Color, atex.UVRect.TopLeft()));
						vxs.PushBack(Vertex(rect.TopRight(), ti.Color, atex.UVRect.TopRight()));
						vxs.PushBack(Vertex(rect.BottomLeft(), ti.Color, atex.UVRect.BottomLeft()));
						vxs.PushBack(Vertex(rect.TopRight(), ti.Color, atex.UVRect.TopRight()));
						vxs.PushBack(Vertex(rect.BottomRight(), ti.Color, atex.UVRect.BottomRight()));
						vxs.PushBack(Vertex(rect.BottomLeft(), ti.Color, atex.UVRect.BottomLeft()));
						topLeft.X += cd.Advance;
						if (cache.LineBreaks[curLine] == i) {
							topLeft.Y += cache.LineHeights[curLine];
							topLeft.X = _GetLineBegin(cache.LineLengths[++curLine], txt);
						}
					}
				}
				if (vxs.Count() > 0) {
					r.SetTexture(ti.Font->GetTexture(lastPage));
					r.DrawVertices(vxs, RenderMode::Triangles);
				}
			}
			List<Rectangle> StreamedRichText::DoGetSelectionRegion(
				const StreamedRichTextFormatCache &cache,
				const StreamedRichText &txt,
				size_t beg, size_t end
			) {
				// testing
				return List<Math::Rectangle>();
			}
			void StreamedRichText::DoHitTest(const StreamedRichTextFormatCache &cache, const StreamedRichText &txt, const Core::Math::Vector2 &pos, size_t &over, size_t &caret) {
				Vector2 relPos = pos;
				relPos.Y -= _GetLayoutTop(cache, txt);
				if (relPos.Y < 0.0) {
					over = caret = 0;
					return;
				}
				size_t line = 0;
				cache.LineHeights.ForEach([&](double h) {
					if ((h -= relPos.Y) < 0.0) {
						return false;
					}
					++line;
					return true;
				});
				if (line >= cache.LineLengths.Count()) {
					over = (caret = txt.Content.Length()) - 1;
					return;
				}
				TextFormatInfo info;
				size_t beg = 0, mark = 0, end = (line == cache.LineBreaks.Count() ? txt.Content.Length() : cache.LineBreaks[line] + 1);
				if (line > 0) {
					beg = cache.LineBreaks[line - 1] + 1;
					info = cache.LineEndFormat[line - 1];
					mark = cache.LineEndChangeIDs[line - 1];
				}
				if ((relPos.X -= _GetLineBegin(cache.LineLengths[line], txt)) < 0.0) {
					over = caret = beg;
					return;
				}
				for (size_t i = beg; i < end; ++i) {
					while (mark < txt.Changes.Count() && txt.Changes[mark].Position == i) {
						info.ApplyChange(txt.Changes[mark]);
					}
					if (info.Font) {
						double adv = info.Font->GetData(txt.Content[i]).Advance * info.Scale;
						if ((relPos.X -= adv) < 0.0) {
							over = i;
							caret = (txt.Content[i] != _TEXT('\n') && relPos.X > -0.5 * adv ? i + 1 : i);
							return;
						}
					}
				}
				if (txt.Content[end - 1] == _TEXT('\n')) {
					over = caret = end - 1;
				} else {
					over = (caret = end) - 1;
				}
			}
			void StreamedRichText::DoGetRelativeCaretPositionAndHeight(
				const StreamedRichTextFormatCache &cache,
				const StreamedRichText &txt,
				size_t caret,
				bool useBaseline,
				double baseline,
				Core::Math::Vector2 &pos,
				double &h
			) {
				size_t
					line = (useBaseline ? DoGetLineOfCaret(cache, txt, caret, baseline) : DoGetLineOfCaret(cache, txt, caret)),
					lb = 0,
					changeID = 0;
				TextFormatInfo ti;
				if (line > 0) {
					ti = cache.LineEndFormat[line - 1];
					lb = cache.LineBreaks[line - 1] + 1;
					changeID = cache.LineEndChangeIDs[line - 1];
				}
				pos.X = _GetLineBegin(cache.LineLengths[line], txt);
				for (size_t i = lb; i < caret; ++i) {
					while (changeID < txt.Changes.Count() && i == txt.Changes[changeID].Position) {
						ti.ApplyChange(txt.Changes[changeID]);
						++changeID;
					}
					if (ti.Font) {
						pos.X += ti.Font->GetData(txt.Content[i]).Advance * ti.Scale;
					}
				}
				if (lb == caret && lb < txt.Content.Length()) { // at the beginning of the line
					while (changeID < txt.Changes.Count() && lb == txt.Changes[changeID].Position) {
						ti.ApplyChange(txt.Changes[changeID]);
						++changeID;
					}
				}
				pos.Y = DoGetLineTop(cache, txt, line);
				if (ti.Font) {
					pos.Y += (cache.LineHeights[line] - (h = ti.Font->GetHeight() * ti.Scale)) * ti.LocalVerticalPosition;
				} else {
					h = 0.0;
				}
			}
			size_t StreamedRichText::DoGetLineOfCaret(const StreamedRichTextFormatCache &cache, const StreamedRichText &txt, size_t caret, double baseline) {
				size_t line = 0;
				bool end = false;
				cache.LineBreaks.ForEach([&](size_t bpos) {
					if (bpos < caret) {
						++line;
						if (bpos + 1 == caret) {
							end = true;
							return false;
						}
						return true;
					}
					return false;
				});
				if (end && txt.Content[caret - 1] != _TEXT('\n')) { // very suspicious
					double midpvt = cache.LineLengths[line - 1];
					midpvt = _GetLineBegin(midpvt, txt) + 0.5 * midpvt;
					if (baseline > midpvt) {
						--line;
					}
				}
				return line;
			}
			size_t StreamedRichText::DoGetLineOfCaret(const StreamedRichTextFormatCache &cache, const StreamedRichText &txt, size_t caret) {
				size_t line = 0;
				cache.LineBreaks.ForEach([&](size_t bpos) {
					if (bpos < caret) {
						++line;
						return true;
					}
					return false;
				});
				return line;
			}
			size_t StreamedRichText::DoGetLineNumber(const StreamedRichTextFormatCache &cache, const StreamedRichText &text) {
				return cache.LineLengths.Count();
			}
			void StreamedRichText::DoGetLineBeginning(const StreamedRichTextFormatCache &cache, const StreamedRichText &txt, size_t line, size_t &caret, double &baseline) {
				caret = (line > 0 ? cache.LineBreaks[line - 1] + 1 : 0);
				baseline = _GetLineBegin(cache.LineLengths[line], txt);
			}
			void StreamedRichText::DoGetLineCursorEnding(const StreamedRichTextFormatCache &cache, const StreamedRichText &txt, size_t line, size_t &caret, double &baseline) {
				caret = (line >= cache.LineBreaks.Count() ? txt.Content.Length() : cache.LineBreaks[line] + 1);
				baseline = _GetLineEnd(cache.LineLengths[line], txt);
				if (caret > 0 && txt.Content[caret - 1] == _TEXT('\n')) { // return at end of line
					--caret;
					const TextFormatInfo &endFormat = cache.LineEndFormat[line];
					baseline -= endFormat.Font->GetData(_TEXT('\n')).Advance * endFormat.Scale;
				}
			}
			void StreamedRichText::DoGetLineEnding(const StreamedRichTextFormatCache &cache, const StreamedRichText &txt, size_t line, size_t &caret, double &baseline) {
				caret = (line >= cache.LineBreaks.Count() ? txt.Content.Length() : cache.LineBreaks[line] + 1);
				baseline = _GetLineEnd(cache.LineLengths[line], txt);
			}
			double StreamedRichText::DoGetLineTop(const StreamedRichTextFormatCache &cache, const StreamedRichText &txt, size_t line) {
				double top = _GetLayoutTop(cache, txt);
				for (size_t i = 0; i < line; ++i) {
					top += cache.LineHeights[i];
				}
				return top;
			}
			double StreamedRichText::DoGetLineHeight(const StreamedRichTextFormatCache &cache, const StreamedRichText &txt, size_t line) {
				return cache.LineHeights[line];
			}

#define STREAMEDRICHTEXT_NEEDCACHE_FUNC_IMPL_BASE(FUNC, ...)    \
	if (FormatCached) {                                         \
		FUNC(CachedFormat, __VA_ARGS__);                        \
	} else {                                                    \
		StreamedRichTextFormatCache cache;                      \
		DoCache(*this, cache);                                  \
		FUNC(cache, __VA_ARGS__);                               \
	}                                                           \

#define STREAMEDRICHTEXT_NEEDCACHE_FUNC_IMPL_NOARGS(FUNC) STREAMEDRICHTEXT_NEEDCACHE_FUNC_IMPL_BASE(FUNC, *this)
#define STREAMEDRICHTEXT_NEEDCACHE_FUNC_IMPL(FUNC, ...) STREAMEDRICHTEXT_NEEDCACHE_FUNC_IMPL_BASE(FUNC, *this, __VA_ARGS__)
			Vector2 StreamedRichText::GetSize() const {
				STREAMEDRICHTEXT_NEEDCACHE_FUNC_IMPL_NOARGS(return DoGetSize);
			}
			Core::Collections::List<Core::Math::Rectangle> StreamedRichText::GetSelectionRegion(size_t s, size_t e) const {
				STREAMEDRICHTEXT_NEEDCACHE_FUNC_IMPL(return DoGetSelectionRegion, s, e);
			}
			void StreamedRichText::HitTest(const Core::Math::Vector2 &pt, size_t &over, size_t &caret) const {
				STREAMEDRICHTEXT_NEEDCACHE_FUNC_IMPL(DoHitTest, pt, over, caret);
			}
			void StreamedRichText::GetRelativeCaretPositionAndHeight(size_t caret, Core::Math::Vector2 &pos, double &h) const {
				STREAMEDRICHTEXT_NEEDCACHE_FUNC_IMPL(DoGetRelativeCaretPositionAndHeight, caret, false, 0.0, pos, h);
			}
			void StreamedRichText::GetRelativeCaretPositionAndHeight(size_t caret, double baseline, Core::Math::Vector2 &pos, double &h) const {
				STREAMEDRICHTEXT_NEEDCACHE_FUNC_IMPL(DoGetRelativeCaretPositionAndHeight, caret, true, baseline, pos, h);
			}
			size_t StreamedRichText::GetLineOfCaret(size_t caret) const {
				STREAMEDRICHTEXT_NEEDCACHE_FUNC_IMPL(return DoGetLineOfCaret, caret);
			}
			size_t StreamedRichText::GetLineOfCaret(size_t caret, double baseline) const {
				STREAMEDRICHTEXT_NEEDCACHE_FUNC_IMPL(return DoGetLineOfCaret, caret, baseline);
			}
			size_t StreamedRichText::GetLineNumber() const {
				STREAMEDRICHTEXT_NEEDCACHE_FUNC_IMPL_NOARGS(return DoGetLineNumber);
			}
			void StreamedRichText::GetLineBeginning(size_t line, size_t &caret, double &baseline) const {
				STREAMEDRICHTEXT_NEEDCACHE_FUNC_IMPL(DoGetLineBeginning, line, caret, baseline);
			}
			void StreamedRichText::GetLineCursorEnding(size_t line, size_t &caret, double &baseline) const {
				STREAMEDRICHTEXT_NEEDCACHE_FUNC_IMPL(DoGetLineCursorEnding, line, caret, baseline);
			}
			void StreamedRichText::GetLineEnding(size_t line, size_t &caret, double &baseline) const {
				STREAMEDRICHTEXT_NEEDCACHE_FUNC_IMPL(DoGetLineEnding, line, caret, baseline);
			}
			double StreamedRichText::GetLineTop(size_t line) const {
				STREAMEDRICHTEXT_NEEDCACHE_FUNC_IMPL(return DoGetLineTop, line);
			}
			double StreamedRichText::GetLineHeight(size_t line) const {
				STREAMEDRICHTEXT_NEEDCACHE_FUNC_IMPL(return DoGetLineHeight, line);
			}
			void StreamedRichText::Render(Renderer &r) const {
				STREAMEDRICHTEXT_NEEDCACHE_FUNC_IMPL(DoRender, r);
			}
#undef STREAMEDRICHTEXT_NEEDCACHE_FUNC_IMPL_BASE
#undef STREAMEDRICHTEXT_NEEDCACHE_FUNC_IMPL_NOARGS
#undef STREAMEDRICHTEXT_NEEDCACHE_FUNC_IMPL
		}
	}
}
