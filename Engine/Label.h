#pragma once

#include "ContentControl.h"

namespace DE {
	namespace UI {
		class Label : public ContentControl {
				friend class World;
			public:
				Label() : ContentControl() {
					_focusable = false;
				}

				virtual void Update(double) override {
				}
		};
	}
}
