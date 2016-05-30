#include "Text.h"

namespace DE {
	namespace Graphics {
		namespace TextRendering {
			using namespace Core;
			using namespace Core::Math;
			using namespace Core::Collections;
			using namespace RenderingContexts;

			// local functions
			double GetLineBegin(double lineLen, double layoutLeft, double layoutWidth, double lPad, double rPad, HorizontalTextAlignment align) {
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
			double GetLineBegin(double lineLen, const Text &txt) {
				return GetLineBegin(
					lineLen, txt.LayoutRectangle.Left, txt.LayoutRectangle.Width(), txt.Padding.Left, txt.Padding.Right, txt.HorizontalAlignment
				);
			}
			double GetRelativeLineBegin(double lineLen, const Text &txt) {
				return GetLineBegin(
					lineLen, 0.0, txt.LayoutRectangle.Width(), txt.Padding.Left, txt.Padding.Right, txt.HorizontalAlignment
				);
			}
			double GetLineEnd(double lineLen, double layoutLeft, double layoutWidth, double lPad, double rPad, HorizontalTextAlignment align) {
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
			double GetLineEnd(double lineLen, const Text &txt) {
				return GetLineEnd(
					lineLen, txt.LayoutRectangle.Left, txt.LayoutRectangle.Width(), txt.Padding.Left, txt.Padding.Right, txt.HorizontalAlignment
				);
			}
			double GetRelativeLineEnd(double lineLen, const Text &txt) {
				return GetLineEnd(
					lineLen, 0.0, txt.LayoutRectangle.Width(), txt.Padding.Left, txt.Padding.Right, txt.HorizontalAlignment
				);
			}
			double GetLayoutTop(double vertLen, double layoutTop, double layoutHeight, double tPad, double bPad, VerticalTextAlignment align) {
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
			double GetLayoutTop(const TextFormatCache &cc, const Text &txt) {
				return GetLayoutTop(
					cc.Size.Y, txt.LayoutRectangle.Top, txt.LayoutRectangle.Height(), txt.Padding.Top, txt.Padding.Bottom, txt.VerticalAlignment
				);
			}
			double GetRelativeLayoutTop(const TextFormatCache &cc, const Text &txt) {
				return GetLayoutTop(
					cc.Size.Y, 0.0, txt.LayoutRectangle.Height(), txt.Padding.Top, txt.Padding.Bottom, txt.VerticalAlignment
				);
			}

			void Text::DoCache(const String &content, TextFormatCache &cache, const TextRendering::Font *f, double maxWidth, double scale) {
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
					if (maxWidth > 0.0 && hasBreakable && curLineLen > maxWidth) {
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
								(curc >= _TEXT('a') && curc <= _TEXT('z'))
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
			void Text::CacheFormat() {
				DoCache(Content, FormatCache, Font, MaxWidth, Scale);
				FormatCached = true;
			}
			void Text::RenderText(Renderer &r, const Text &txt, const TextFormatCache &cc, bool roundToInt) {
				if (r.GetContext() == nullptr || txt.Font == nullptr || txt.Content.Empty() || cc.LineLengths.Count() == 0) {
					return;
				}
				size_t curB = 0;
				Vector2 pos(
					GetLineBegin(cc.LineLengths[0], txt),
					GetLayoutTop(cc, txt)
				);
				pos.X = GetLineBegin(cc.LineLengths[0], txt);
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
					Rectangle charRect = data.Placement, realUV = ctex.UVRect;
					charRect.Scale(Vector2(), txt.Scale);
					charRect.Translate(aRPos);
					if (txt.UseClip) {
						Rectangle isectRect;
						if (Rectangle::Intersect(txt.Clip, charRect, isectRect) == IntersectionType::Full) {
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
						pos.X = GetLineBegin(cc.LineLengths[curB], txt);
						pos.Y += txt.Font->GetHeight() * txt.Scale;
					}
				}
				if (vs.Count() > 0) {
					r<<txt.Font->GetTexture(lstpg);
					r.DrawVertices(vs, RenderMode::Triangles);
				}
				r<<Texture();
			}
			// another helper function
			void AddRectangleToListWithClip(List<Math::Rectangle> &list, const Math::Rectangle &r, const Math::Rectangle &clip, bool useClip) {
				if (useClip) {
					Math::Rectangle isect;
					if (Math::Rectangle::Intersect(clip, r, isect) != IntersectionType::None) {
						list.PushBack(isect);
					}
				} else {
					list.PushBack(r);
				}
			}
			List<Math::Rectangle> Text::GetSelectionRegion(const Text &txt, size_t start, size_t end, const TextFormatCache &cc) {
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
					top = GetLayoutTop(cc, txt),
					sll = GetLineBegin(cc.LineLengths[sl], txt),
					ell = sll;
				size_t
					slc = (sl > 0 ? cc.LineBreaks[sl - 1] + 1 : 0),
					elc = slc;
				if (sl != el) {
					ell = GetLineBegin(cc.LineLengths[el], txt);
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
					AddRectangleToListWithClip(result, Math::Rectangle(sll, top + lineHeight * sl, ell - sll, lineHeight), txt.Clip, txt.UseClip);
				} else {
					AddRectangleToListWithClip(result, Math::Rectangle(
						sll,
						top += lineHeight * sl,
						GetLineEnd(cc.LineLengths[sl], txt) - sll,
						lineHeight
					), txt.Clip, txt.UseClip);
					for (size_t x = sl + 1; x < el; ++x) {
						AddRectangleToListWithClip(result, Math::Rectangle(
							GetLineBegin(cc.LineLengths[x], txt),
							top += lineHeight,
							cc.LineLengths[x],
							lineHeight
						), txt.Clip, txt.UseClip);
					}
					double l = GetLineBegin(cc.LineLengths[el], txt);
					AddRectangleToListWithClip(result, Math::Rectangle(
						l,
						top + lineHeight,
						ell - l,
						lineHeight
					), txt.Clip, txt.UseClip);
				}
				return result;
			}
			void Text::HitTest(const Text &txt, const Vector2 &pos, size_t &over, size_t &caret, const TextFormatCache &cc) {
				double top = GetLayoutTop(cc, txt);
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
				double sx = GetLineBegin(cc.LineLengths[line], txt);
				if (pos.X <= sx) {
					over = caret = lbeg;
					return;
				}
				for (; lbeg < lend; ++lbeg) {
					double adv = txt.Font->GetData(txt.Content[lbeg]).Advance * txt.Scale;
					if (pos.X <= sx + adv) {
						over = lbeg;
						caret = (pos.X <= sx + 0.5 * adv ? lbeg : lbeg + 1);
						return;
					}
					sx += adv;
				}
				over = (caret = lend) - 1;
			}
			Vector2 Text::GetRelativeCaretPosition(const Text &text, size_t caret, const TextFormatCache &cache) {
				size_t line = 0;
				cache.LineBreaks.ForEach(
					[&](const size_t &breakpos) {
						if (breakpos < caret) {
							++line;
							return true;
						}
						return false;
					}
				);
				Vector2 pos(0.0, line * text.Font->GetHeight());
				for (size_t ls = (line == 0 ? 0 : cache.LineBreaks[line - 1] + 1); ls < caret; ++ls) {
					pos.X += text.Font->GetData(text.Content[ls]).Advance;
				}
				return Vector2(
					GetRelativeLineBegin(cache.LineLengths[line],text),
					GetRelativeLayoutTop(
						cache,
						text
					)
				) + pos * text.Scale;
			}

			void Text::Render(Renderer &r) const {
				if (Font != nullptr) {
					if (FormatCached) {
						Text::RenderText(r, *this, FormatCache, RoundToInteger);
					} else {
						TextFormatCache cc;
						Text::DoCache(Content, cc, Font, MaxWidth, Scale);
						Text::RenderText(r, *this, cc, RoundToInteger);
					}
				}
			}
			Core::Collections::List<Core::Math::Rectangle> Text::GetSelectionRegion(size_t start, size_t end) const {
				if (start > Content.Length() || end > Content.Length() || start > end) {
					throw InvalidArgumentException(_TEXT("index overflow"));
				}
				if (Font != nullptr) {
					if (FormatCached) {
						return Text::GetSelectionRegion(*this, start, end, FormatCache);
					} else {
						TextFormatCache cc;
						Text::DoCache(Content, cc, Font, MaxWidth, Scale);
						return Text::GetSelectionRegion(*this, start, end, cc);
					}
				}
				return Core::Collections::List<Core::Math::Rectangle>();
			}
			void Text::HitTest(const Vector2 &pos, size_t &over, size_t &caret) const {
				if (Font != nullptr) {
					if (FormatCached) {
						Text::HitTest(*this, pos, over, caret, FormatCache);
					} else {
						TextFormatCache cc;
						Text::DoCache(Content, cc, Font, MaxWidth, Scale);
						Text::HitTest(*this, pos, over, caret, cc);
					}
				}
			}
			Vector2 Text::GetRelativeCaretPosition(size_t caret) const {
				if (caret > Content.Length()) {
					throw InvalidArgumentException(_TEXT("caret index overflow"));
				}
				if (Font != nullptr) {
					if (FormatCached) {
						return Text::GetRelativeCaretPosition(*this, caret, FormatCache);
					} else {
						TextFormatCache cc;
						Text::DoCache(Content, cc, Font, MaxWidth, Scale);
						return Text::GetRelativeCaretPosition(*this, caret, cc);
					}
				}
				return Vector2();
			}
		}
	}
}
