#pragma once

#include <gdiplus.h>
#include <sstream>

#include "Font.h"
#include "Rectangle.h"
#include "Vector2.h"
#include "RenderingContext.h"
#include "List.h"
#include "Dictionary.h"
#include "Queue.h"
#include "Renderer.h"
#include "FileAccess.h"
#include "Atlas.h"

namespace DE {
	namespace Graphics {
		namespace TextRendering {
			class BMPFont : public Font {
					friend class BMPFontGenerator;
				public:
					BMPFont() : Font(nullptr) {
					}
					explicit BMPFont(RenderingContexts::RenderingContext *c) : Font(c) {
					}
					virtual ~BMPFont() {
					}

					const Core::String &FontName() const {
						return _fontName;
					}

					void Save(const Core::String &imgName, IO::FileAccess &writer) const {
						writer.WriteBinaryString(_fontName);
						writer.WriteBinaryObject<double>(_height);
						_al.Save(
							imgName,
							writer,
							[](int, IO::FileAccess &wtr, const void *data) {
								if (data) {
									wtr.WriteBinaryRaw(data, sizeof(CharData));
								}
							}
						);
					}
					static BMPFont Load(Renderer &r, IO::FileAccess &reader) {
						BMPFont fnt;
						fnt._fontName = reader.ReadBinaryString<TCHAR>();
						fnt._height = reader.ReadBinaryObject<double>();
						fnt._al = Atlas::Load(
							r,
							reader,
							[](int, IO::FileAccess &rdr) {
								return new (Core::GlobalAllocator::Allocate(sizeof(CharData))) CharData(rdr.ReadBinaryObject<CharData>());
							}
						);
						fnt._al.FreeFunc() = [](int, void *ptr) {
							if (ptr) {
								Core::GlobalAllocator::Free(ptr);
							}
						};
						return fnt;
					}

					virtual const CharData &GetData(TCHAR c) const override {
						return *static_cast<CharData*>(_al[c].Tag);
					}
					virtual const AtlasTexture &GetTextureInfo(TCHAR c) const override {
						return _al[c];
					}
					virtual const TextureID &GetTexture(size_t p) const override {
						return _al.Textures()[p];
					}
					virtual bool HasData(TCHAR c) const override {
						return _al.AtlasTextures().ContainsKey(c);
					}
				private:
					BMPFont(RenderingContexts::RenderingContext *ctx, Atlas atl) : Font(ctx), _al(atl) {
					}

					Core::String _fontName;
					Atlas _al;
			};
		}
	}
}
