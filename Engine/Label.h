#pragma once

#include "ContentControl.h"

namespace DE {
	namespace UI {
		template <typename T = Graphics::TextRendering::BasicText> class Label : public ContentControl<T> {
				friend class World;
			public:
				Label() : ContentControl<T>() {
					ContentControl<T>::_focusable = false;
				}

				virtual void Update(double) override {
				}
		};
	}
}
