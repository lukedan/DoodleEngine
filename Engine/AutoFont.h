#pragma once

#include "Font.h"
#include "Atlas.h"

namespace DE {
	namespace Graphics {
		namespace TextRendering {
			class AutoFont : public Font {
				public:
					AutoFont() = default;
					AutoFont(RenderingContexts::RenderingContext *ctx, const FreeTypeAccess::FontFace &face) : Font(ctx), Face(face), _atl(ctx) {
						_atl.TargetAtlas->FreeFunc() = [](int, void *ptr) {
							CharData *data = static_cast<CharData*>(ptr);
							data->~CharData();
							Core::GlobalAllocator::Free(data);
						};
					}
					AutoFont(RenderingContexts::RenderingContext *ctx) : AutoFont(ctx, FreeTypeAccess::FontFace()) {
					}

					const CharData &GetData(TCHAR c) const override {
						CheckForData(c);
						return *static_cast<CharData*>(_atl.TargetAtlas->AtlasTextures()[c].Tag);
					}
					const AtlasTexture &GetTextureInfo(TCHAR c) const override {
						CheckForData(c);
						return _atl.TargetAtlas->AtlasTextures()[c];
					}
					const TextureID &GetTexture(size_t pg) const override {
						_atl.Flush();
						return _atl.TargetAtlas->Textures()[pg];
					}
					bool HasData(TCHAR c) const override {
						CheckForData(c);
						return *Face;
					}
					double GetHeight() const override {
						return Face->GetHeight();
					}

					Core::ReferenceProperty<double> ImageBorderWidth = 1.0;
					Core::ReferenceProperty<FreeTypeAccess::FontFace> Face;
				protected:
					mutable DynamicAtlas _atl; // NOTE usage of mutable object

					void CheckForData(TCHAR c) const {
						if (*Face) {
							if (!_atl.TargetAtlas->AtlasTextures().ContainsKey(c)) {
								CharCreationData data = Face->CreateChar(c, ImageBorderWidth);
								CharData *cd = new (Core::GlobalAllocator::Allocate(sizeof(CharData))) CharData(data.Data);
								_atl.Append(c, data.Image, cd, false);
								delete data.Image;
							}
						}
					}
			};
		}
	}
}
