#include "Atlas.h"

#include "Queue.h"

using namespace DE;
using namespace DE::Core;
using namespace DE::Core::Math;
using namespace DE::Core::Collections;
using namespace DE::Graphics::RenderingContexts;

namespace DE {
	namespace Graphics {
		class TextureInfoComparer {
			public:
				static int Compare(const AtlasGenerator::TextureInfo &lhs, const AtlasGenerator::TextureInfo &rhs) {
					return Core::DefaultComparer<UINT>::Compare(lhs.Image->GetHeight(), rhs.Image->GetHeight());
				}
		};
		void UpperVector2(Vector2 &v) {
			v.X = ceil(v.X);
			v.Y = ceil(v.Y);
		}
		Atlas AtlasGenerator::Generate(Renderer &r) {
			if (r.GetContext() == nullptr) {
				throw InvalidArgumentException(_TEXT("the renderer is not bound to any context"));
			}
			Dictionary<int, AtlasTexture> ats;
			List<TextureID> ldtxs;
			List<double> yls;
			yls.PushBack(0.0);
			UnstableSort<List<TextureInfo>, TextureInfo, TextureInfoComparer>(_texs);
			_texs.Reverse();
			Vector2 cPos(-_border.Left, -_border.Top);
			Queue<Vector2> lastposs, curposs;
			lastposs.PushTail(Vector2(_xlimit, 0.0));
			size_t curTex = 0;
			_texs.ForEach([&](const TextureInfo &info) {
				if (info.Image->GetWidth() > _xlimit || (_ylimit > 0.0 && info.Image->GetHeight() > _ylimit)) {
					throw InvalidArgumentException(_TEXT("the image is bigger than the atlas page"));
				}
				UpperVector2(cPos);
				while (lastposs.PeekHead().X < cPos.X) {
					lastposs.PopHead();
				}
				if (ceil(cPos.X + info.Image->GetWidth() + _border.Right) > _xlimit) {
					cPos.X = -_border.Left;
					lastposs.ForEachHeadToTail([&](const Vector2 &vec) {
						curposs.PushTail(vec);
						return true;
					});
					lastposs = curposs;
					curposs.Clear();
				}
				cPos.Y = lastposs.PeekHead().Y;
				UpperVector2(cPos);
				if (_ylimit > 0.0 && ceil(cPos.Y + info.Image->GetHeight() + _border.Bottom) > _ylimit) {
					++curTex;
					yls.PushBack(0.0);
					cPos = Vector2(-_border.Left, -_border.Top);
					UpperVector2(cPos);
					lastposs.Clear();
					curposs.Clear();
					lastposs.PushTail(Vector2(_xlimit, 0.0));
				}
				AtlasTexture currentATex;
				currentATex.Page = curTex;
				currentATex.UVRect = Math::Rectangle(cPos.X, cPos.Y, info.Image->GetWidth(), info.Image->GetHeight());
				currentATex.Tag = info.Tag;
				ats[info.Key] = currentATex;
				cPos.X += info.Image->GetWidth() + _border.Right;
				Vector2 nvec(cPos.X, cPos.Y + info.Image->GetHeight() + _border.Bottom);
				UpperVector2(nvec);
				if (nvec.Y > yls.Last()) {
					yls.Last() = nvec.Y;
				}
				curposs.PushTail(nvec);
				return true;
			});
			List<Gdiplus::Bitmap*> texs;
			yls.ForEach([&](double d) {
				texs.PushBack(new Gdiplus::Bitmap(ceil(_xlimit), ceil(d), PixelFormat32bppARGB));
				return true;
			});
			for (size_t i = 0; i < yls.Count(); ++i) {
				Gdiplus::Graphics *g = new Gdiplus::Graphics(texs[i]);
				g->Clear(Gdiplus::Color::Transparent);
				_texs.ForEach([&](const TextureInfo &info) {
					AtlasTexture curATex = ats[info.Key];
					if (curATex.Page == i) {
						g->DrawImage(
							info.Image,
							static_cast<int>(curATex.UVRect.Left),
							static_cast<int>(curATex.UVRect.Top),
							static_cast<int>(curATex.UVRect.Width()),
							static_cast<int>(curATex.UVRect.Height())
						);
						curATex.UVRect.Left /= texs[i]->GetWidth();
						curATex.UVRect.Right /= texs[i]->GetWidth();
						curATex.UVRect.Top /= texs[i]->GetHeight();
						curATex.UVRect.Bottom /= texs[i]->GetHeight();
						ats[info.Key].UVRect = curATex.UVRect;
					}
					return true;
				});
				delete g;
			}
			if (*FX) {
				texs.ForEach([&](Gdiplus::Bitmap *bmp) {
					return (*FX)(bmp);
				});
			}
			texs.ForEach([&](Gdiplus::Bitmap *bmp) {
				ldtxs.PushBack(r.LoadTextureFromBitmap(*bmp));
				delete bmp;
				return true;
			});
			return Atlas(r.GetContext(), ldtxs, ats);
		}
	}
}
