#include "Font.h"

#define TRY_FREETYPE(X) if (X) { throw ::DE::Core::SystemException(_TEXT("error occurred in FreeType")); }
namespace DE {
	namespace Graphics {
		namespace TextRendering {
			using namespace DE;
			using namespace DE::Core;
			using namespace DE::Core::Math;
			using namespace DE::Core::Collections;

			namespace FreeTypeAccess {
				FreeTypeInitializer::_FreeTypeInitObject FreeTypeInitializer::_initObj;

				FreeTypeInitializer::_FreeTypeInitObject::_FreeTypeInitObject() {
					TRY_FREETYPE(FT_Init_FreeType(&Lib));
				}
				FreeTypeInitializer::_FreeTypeInitObject::~_FreeTypeInitObject() {
					TRY_FREETYPE(FT_Done_FreeType(Lib));
				}

				FontFace::FontFace(const AsciiString &str, double sz) : _face(nullptr, [](FT_Face *face) {
					TRY_FREETYPE(FT_Done_Face(*face));
					GlobalAllocator::Free(face);
				}) {
					FT_Face *fac = new (GlobalAllocator::Allocate(sizeof(FT_Face))) FT_Face();
					TRY_FREETYPE(FT_New_Face(FreeTypeInitializer::_initObj.Lib, *str, 0, fac));
					_face.SetSharedPointer(fac);
					TRY_FREETYPE(FT_Set_Pixel_Sizes(*fac, 0, sz));
				}
				double FontFace::GetHeight() const {
					return (*_face)->size->metrics.height / 64.0;
				}
				CharCreationData FontFace::CreateChar(TCHAR c, double dbBdr) const {
					if (!IsValid()) {
						return CharCreationData();
					}
					int border = static_cast<int>(ceil(dbBdr));
					CharData data;
					data.Character = c;
					TRY_FREETYPE(FT_Load_Char(*_face, data.Character, FT_LOAD_RENDER));
					const FT_Bitmap &bmpdata = (*_face)->glyph->bitmap;
					data.Placement = Math::Rectangle(
						(*_face)->glyph->bitmap_left,
						((*_face)->size->metrics.ascender - (*_face)->glyph->metrics.horiBearingY) / 64.0,
						bmpdata.width,
						bmpdata.rows
					);
					data.Placement.Left -= border;
					data.Placement.Top -= border;
					data.Placement.Right += border;
					data.Placement.Bottom += border;
					data.Advance = (*_face)->glyph->advance.x / 64.0;
					// generate the bitmap
					Gdiplus::Bitmap *bmp = new Gdiplus::Bitmap(bmpdata.width + 2 * border, bmpdata.rows + 2 * border);
					if (bmpdata.width > 0 && bmpdata.rows > 0) {
						Gdiplus::BitmapData bmpdt;
						Gdiplus::Rect r;
						r.X = r.Y = border;
						r.Width = bmpdata.width;
						r.Height = bmpdata.rows;
						AssertGDIPlusSuccess(
							bmp->LockBits(&r, Gdiplus::ImageLockModeWrite, PixelFormat32bppARGB, &bmpdt),
							_TEXT("cannot lock the bitmap")
						);
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
					CharCreationData res;
					res.Data = data;
					res.Image = bmp;
					return res;
				}
			}
		}
	}
}
