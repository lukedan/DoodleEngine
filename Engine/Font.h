#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H

#include "..\CoreWrap.h"
#include "Atlas.h"

namespace DE {
    namespace Graphics {
    	namespace TextRendering {
    		struct CharData {
    			TCHAR Character = _TEXT('\0');
    			Core::Math::Rectangle Placement;
    			double Advance = 0.0;
    		};
    		struct CharCreationData {
    			CharData Data;
    			Gdiplus::Bitmap *Image = nullptr;
    		};

    		namespace FreeTypeAccess {
				class FreeTypeInitializer {
						friend class FontFace;
					protected:
						struct _FreeTypeInitObject {
							_FreeTypeInitObject();
							~_FreeTypeInitObject();

							FT_Library Lib;
						};
						static _FreeTypeInitObject _initObj;
				};
				class FontFace {
					public:
						FontFace() = default;
						FontFace(const Core::AsciiString&, double);

						CharCreationData CreateChar(TCHAR, double) const;
						double GetHeight() const;

						bool IsValid() const {
							return _face != nullptr;
						}
						operator bool() const {
							return _face != nullptr;
						}
					protected:
						Core::SharedPointer<FT_Face> _face = nullptr;
				};
    		}

    		class Font {
    				friend class Text;
    			public:
    				Font() = default;
    				explicit Font(RenderingContexts::RenderingContext *rc) : _context(rc) {
    				}
    				virtual ~Font() {
    				}

    				virtual const CharData &GetData(TCHAR) const = 0;
    				virtual const AtlasTexture &GetTextureInfo(TCHAR) const = 0;
    				virtual const Texture &GetTexture(size_t) const = 0;
    				virtual bool HasData(TCHAR c) const = 0;
    				virtual double GetHeight() const {
    					return _height;
    				}
    			protected:
    				double _height = 0.0;
    				RenderingContexts::RenderingContext *_context = nullptr;
    		};
    	}
    }
}
