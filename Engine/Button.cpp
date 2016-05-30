#include "Button.h"

namespace DE {
	namespace UI {
		using namespace Core;
		using namespace Graphics;

		const SolidBrush
			SimpleButton::DefaultNormalBrush(Color(180, 180, 180, 255)),
			SimpleButton::DefaultHoverBrush(Color(230, 230, 230, 255)),
			SimpleButton::DefaultPressedBrush(Color(130, 130, 130, 255));
	}
}