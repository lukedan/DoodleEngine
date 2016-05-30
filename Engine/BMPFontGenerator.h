#pragma once

#include "Common.h"
#include "Rectangle.h"
#include "List.h"
#include "BMPFont.h"
#include "Renderer.h"

namespace DE {
	namespace Graphics {
		namespace TextRendering {
			class BMPFontGenerator {
				public:
					BMPFontGenerator() = default;

					BMPFont Generate(Renderer&) const;

					Core::ReferenceProperty<Core::String> Dictionary;
					Core::ReferenceProperty<Core::AsciiString> FontFile;
					Core::ReferenceProperty<double> FontSize = 15.0, TextureWidth = 300.0, TextureHeight = -1.0, BorderWidth = 1.0;
					Core::ReferenceProperty<Core::Math::Rectangle> FXMargins;
					Core::ReferenceProperty<std::function<bool(Gdiplus::Bitmap*)>> FX;
			};
		}
	}
}
