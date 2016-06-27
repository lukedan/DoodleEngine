#include "ft2build.h"
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include "BMPFontGenerator.h"
#include "Common.h"
#include "Queue.h"
#include "ObjectAllocator.h"
#include "BMPFont.h"
#include "Math.h"

namespace DE {
	namespace Graphics {
		namespace TextRendering {
			using namespace Gdiplus;
			using namespace std;
			using namespace Core;
			using namespace Core::Math;
			using namespace Core::Collections;
			BMPFont BMPFontGenerator::Generate(Renderer &r) const {
#define TRY_FREETYPE(X) if (X) { throw SystemException(_TEXT("error occurred in FreeType")); }
				String str = Dictionary;
				AtlasGenerator atlgen;
				UnstableSort<TCHAR>(*str, str.Length());
				List<Bitmap*> bmps2disp;
				// initialize FreeType
				FT_Library lib;
				FT_Face face;
				TRY_FREETYPE(FT_Init_FreeType(&lib));
				TRY_FREETYPE(FT_New_Face(lib, **FontFile, 0, &face));
				if (face->face_flags & FT_FACE_FLAG_SCALABLE) {
					FT_Size_RequestRec req;
					req.horiResolution = req.vertResolution = 0;
					req.height = static_cast<FT_Long>(FontSize * 64.0);
					req.width = 0;
					req.type = FT_SIZE_REQUEST_TYPE_NOMINAL;
					TRY_FREETYPE(FT_Request_Size(face, &req));
				} else {
					TRY_FREETYPE(FT_Select_Size(face, 0));
				}
				// get all char data
				TCHAR last = _TEXT('\0');
				int border = static_cast<int>(ceil(BorderWidth));
				for (size_t i = 0; i < str.Length(); ++i) {
					if (str[i] == last) {
						continue;
					}
					// get current char data
					CharData *data = new (GlobalAllocator::Allocate(sizeof(CharData))) CharData();
					data->Character = str[i];
					TRY_FREETYPE(FT_Load_Char(face, data->Character, FT_LOAD_RENDER));
					const FT_Bitmap &bmpdata = face->glyph->bitmap;
					data->Placement = Math::Rectangle(
						face->glyph->bitmap_left,
						(face->size->metrics.ascender - face->glyph->metrics.horiBearingY) / 64.0,
						bmpdata.width,
						bmpdata.rows
					);
					data->Placement.Left -= border;
					data->Placement.Top -= border;
					data->Placement.Right += border;
					data->Placement.Bottom += border;
					data->Advance = face->glyph->advance.x / 64.0;
					// generate the bitmap
					Bitmap *bmp = new Bitmap(bmpdata.width + 2 * border, bmpdata.rows + 2 * border);
					bmps2disp.PushBack(bmp);
					if (bmpdata.width > 0 && bmpdata.rows > 0) {
						BitmapData bmpdt;
						Rect r;
						r.X = r.Y = border;
						r.Width = bmpdata.width;
						r.Height = bmpdata.rows;
						AssertGDIPlusSuccess(bmp->LockBits(&r, ImageLockModeWrite, PixelFormat32bppARGB, &bmpdt), _TEXT("cannot lock the bitmap"));
						const unsigned char *buf = bmpdata.buffer;
						int *first = static_cast<int*>(bmpdt.Scan0);
						for (UINT y = 0; y < bmpdata.rows; ++y) {
							int *beg = first;
							for (UINT x = 0; x < bmpdata.width; ++x) {
								*beg = 0xFFFFFF | (static_cast<int>(*buf) << 24);
								++beg;
								++buf;
							}
							first = reinterpret_cast<int*>(reinterpret_cast<size_t>(first) + bmpdt.Stride);
						}
						AssertGDIPlusSuccess(bmp->UnlockBits(&bmpdt), _TEXT("cannot unlock the bitmap"));
					}
					AtlasGenerator::TextureInfo texInfo;
					texInfo.Tag = data;
					texInfo.Key = data->Character;
					texInfo.Image = bmp;
					atlgen.Textures().PushBack(texInfo);
					last = str[i];
				}
				atlgen.BorderWidth() = FXMargins;
				atlgen.HeightLimit() = TextureHeight;
				atlgen.WidthLimit() = TextureWidth;
				atlgen.FX = FX;
				BMPFont result(r.GetContext(), atlgen.Generate(r));
				result._al.FreeFunc() = [](int, void *ptr) {
					if (ptr) {
						GlobalAllocator::Free(ptr);
					}
				};
				result._height = face->size->metrics.height / 64.0;

				bmps2disp.ForEach([&](Gdiplus::Bitmap *bmp) {
					delete bmp;
					return true;
				});
				TRY_FREETYPE(FT_Done_Face(face));
				TRY_FREETYPE(FT_Done_FreeType(lib));

				return result;
#undef TRY_FREETYPE
			}
		}
	}
}
