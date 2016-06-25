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
				const String &content,
				BasicTextFormatCache &cache,
				const TextRendering::Font *f,
				LineWrapType wrap,
				double maxWidth,
				double scale
			) {
				cache.LineBreaks.Clear();
				cache.LineLengths.Clear();
				cache.Size = Vector2();
				if (f == nullptr) {
					return;
				}
				bool hasBreakable = false;
				size_t lastBreakable = 0;
				double curLineLen = 0.0, lastBreakableLen = 0.0;
				CharData curData;
				for (size_t i = 0; i < content.Length(); ++i) {
					curData = f->GetData(content[i]);
					curLineLen += curData.Advance * scale;
					if (curLineLen > maxWidth && cache.LineBreaks.Count() > 0 && cache.LineBreaks.Last() + 1 != i) {
						if (!(wrap == LineWrapType::WrapWords && !hasBreakable) && wrap != LineWrapType::NoWrap) {
							// if the current line can be wrapped
							if (wrap == LineWrapType::Wrap || (wrap == LineWrapType::WrapWordsNoOverflow && !hasBreakable)) {
								// coerce wrapping
								if (i > 0) {
									lastBreakable = i - 1;
									lastBreakableLen = curLineLen - curData.Advance * scale;
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
					switch (content[i]) {
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
							TCHAR curc = content[i];
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
				cache.Size.Y = cache.LineLengths.Count() * f->GetHeight() * scale;
			}
			void BasicText::DoRender(Renderer &r, const BasicText &txt, const BasicTextFormatCache &cc, bool roundToInt) {
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
			List<Math::Rectangle> BasicText::DoGetSelectionRegion(const BasicText &txt, size_t start, size_t end, const BasicTextFormatCache &cc) {
				// TODO: only suitable for left-aligned texts
				if (start >= end) {
					return List<Math::Rectangle>();
				}
				size_t sl = 0, el = 0;
				cc.LineBreaks.ForEach([&](size_t clb) {
					if (clb < start) {
						++sl;
					}
					if (clb + 1 < end) {
						++el;
					} else {
						return false;
					}
					return true;
				});
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
			void BasicText::DoHitTest(const BasicText &txt, const Vector2 &pos, size_t &over, size_t &caret, const BasicTextFormatCache &cc) {
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
				const BasicText &text,
				size_t caret,
				bool useBaseline,
				double baselinePos,
				const BasicTextFormatCache &cache
			) {
				size_t line = (useBaseline ? DoGetLineOfCaret(text, caret, baselinePos, cache) : DoGetLineOfCaret(text, caret, cache));
				Vector2 pos(0.0, line * text.Font->GetHeight()); // no need to multiply Scale, for it's done later
				for (size_t ls = (line == 0 ? 0 : cache.LineBreaks[line - 1] + 1); ls < caret; ++ls) {
					pos.X += text.Font->GetData(text.Content[ls]).Advance;
				}
				return Vector2(
					_GetRelativeLineBegin(cache.LineLengths[line], text),
					_GetRelativeLayoutTop(cache, text)
				) + pos * text.Scale;
			}
			size_t BasicText::DoCaretVerticalMove( // TODO deprecated
				const BasicText &txt,
				size_t caret,
				size_t abortLine,
				double x,
				double diffParam,
				const BasicTextFormatCache &cache
			) {
				bool isAtBeg = false;
				TCHAR endChar;
				size_t lineID = 0;
				cache.LineBreaks.ForEach([&](size_t id) {
					if (id < caret) {
						++lineID;
						if (id + 1 == caret) {
							isAtBeg = true;
							endChar = txt.Content[id];
							return false;
						}
						return true;
					}
					return false;
				});
				double lineHeight = txt.Scale * txt.Font->GetHeight();
				Vector2 htpos(x, txt.LayoutRectangle.Top + lineHeight * (lineID + diffParam));
				if (isAtBeg) {
					double midPvt = cache.LineLengths[lineID];
					midPvt = _GetLineBegin(midPvt, txt) + 0.5 * midPvt;
					if (endChar != _TEXT('\n') && x > midPvt) { // 'virtual' cursor
						--lineID;
						htpos.Y -= lineHeight;
					}
				}
				if (lineID != abortLine) {
					size_t dummy;
					DoHitTest(txt, htpos, dummy, caret, cache);
				}
				return caret;
			}
			size_t BasicText::DoGetLineOfCaret(
				const BasicText &text,
				size_t caret,
				double baselinePos,
				const BasicTextFormatCache &cache
			) {
				size_t line = 0;
				bool end;
				TCHAR endchar;
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
					midpvt = _GetLineBegin(midpvt, text) - 0.5 * midpvt;
					if (baselinePos > midpvt) {
						--line;
					}
				}
				return line;
			}
			size_t BasicText::DoGetLineOfCaret(const BasicText &text, size_t caret, const BasicTextFormatCache &cache) {
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

			void BasicText::CacheFormat() {
				DoCache(Content, FormatCache, Font, WrapType, LayoutRectangle.Width(), Scale);
				FormatCached = true;
			}
			void BasicText::Render(Renderer &r) const {
				if (Font != nullptr) {
					if (FormatCached) {
						DoRender(r, *this, FormatCache, RoundToInteger);
					} else {
						BasicTextFormatCache cc;
						DoCache(Content, cc, Font, WrapType, LayoutRectangle.Width(), Scale);
						DoRender(r, *this, cc, RoundToInteger);
					}
				}
			}
			Core::Collections::List<Core::Math::Rectangle> BasicText::GetSelectionRegion(size_t start, size_t end) const {
				if (start > Content.Length() || end > Content.Length() || start > end) {
					throw InvalidArgumentException(_TEXT("index overflow"));
				}
				if (Font != nullptr) {
					if (FormatCached) {
						return DoGetSelectionRegion(*this, start, end, FormatCache);
					} else {
						BasicTextFormatCache cc;
						DoCache(Content, cc, Font, WrapType, LayoutRectangle.Width(), Scale);
						return DoGetSelectionRegion(*this, start, end, cc);
					}
				}
				return Core::Collections::List<Core::Math::Rectangle>();
			}
			double BasicText::GetLineBegin(size_t caret) const {
				if (Font) {
					if (FormatCached) {

					}
				}
				// TODO
			}
			double BasicText::GetLineEnd(size_t caret) const {

			}
			void BasicText::HitTest(const Vector2 &pos, size_t &over, size_t &caret) const {
				if (Font != nullptr) {
					if (FormatCached) {
						DoHitTest(*this, pos, over, caret, FormatCache);
					} else {
						BasicTextFormatCache cc;
						DoCache(Content, cc, Font, WrapType, LayoutRectangle.Width(), Scale);
						DoHitTest(*this, pos, over, caret, cc);
					}
				}
			}
			Vector2 BasicText::GetRelativeCaretPosition(size_t caret) const {
				if (caret > Content.Length()) {
					throw InvalidArgumentException(_TEXT("caret index overflow"));
				}
				if (Font) {
					if (FormatCached) {
						return DoGetRelativeCaretPosition(*this, caret, false, 0.0, FormatCache);
					} else {
						BasicTextFormatCache cc;
						DoCache(Content, cc, Font, WrapType, LayoutRectangle.Width(), Scale);
						return DoGetRelativeCaretPosition(*this, caret, false, 0.0, cc);
					}
				}
				return Vector2();
			}
			Vector2 BasicText::GetRelativeCaretPosition(size_t caret, double baseline) const {
				if (caret > Content.Length()) {
					throw InvalidArgumentException(_TEXT("caret index overflow"));
				}
				if (Font) {
					if (FormatCached) {
						return DoGetRelativeCaretPosition(*this, caret, true, baseline, FormatCache);
					} else {
						BasicTextFormatCache cc;
						DoCache(Content, cc, Font, WrapType, LayoutRectangle.Width(), Scale);
						return DoGetRelativeCaretPosition(*this, caret, true, baseline, cc);
					}
				}
				return Vector2();
			}
			size_t BasicText::CaretLineUp(size_t caretID, double x) const {
				if (Font) {
					if (FormatCached) {
						return DoCaretVerticalMove(*this, caretID, 0, x, -0.5, FormatCache);
					} else {
						BasicTextFormatCache cache;
						DoCache(Content, cache, Font, WrapType, LayoutRectangle.Width(), Scale);
						return DoCaretVerticalMove(*this, caretID, 0, x, -0.5, cache);
					}
				}
				return 0;
			}
			size_t BasicText::CaretLineDown(size_t caretID, double x) const {
				if (Font) {
					if (FormatCached) {
						if (FormatCache.LineLengths.Count() == 0) {
							return caretID;
						}
						return DoCaretVerticalMove(*this, caretID, FormatCache.LineLengths.Count() - 1, x, 1.5, FormatCache);
					} else {
						BasicTextFormatCache cache;
						DoCache(Content, cache, Font, WrapType, LayoutRectangle.Width(), Scale);
						if (cache.LineLengths.Count() == 0) {
							return caretID;
						}
						return DoCaretVerticalMove(*this, caretID, cache.LineLengths.Count() - 1, x, 1.5, cache);
					}
				}
				return 0;
			}
		}
	}
}
