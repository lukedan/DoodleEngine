#include "ProgressBar.h"

namespace DE {
	namespace UI {
		using namespace Core;
		using namespace Graphics;

        const Graphics::SolidBrush
			SimpleProgressBar::DefaultFinishedBrush(Color(0, 200, 50, 255)),
			SimpleProgressBar::DefaultUnfinishedBrush(Color(255, 255, 255, 50));
	}
}