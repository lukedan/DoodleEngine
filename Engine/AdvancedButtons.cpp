#include "AdvancedButtons.h"

using namespace DE::Core;
using namespace DE::Graphics;

namespace DE {
	namespace UI {
		const Size CheckBoxBase::DefaultBoxSize(10.0, 10.0);

		const SolidBrush
			SimpleCheckBox::DefaultNormalBoxBrush(Color(180, 180, 180, 255)),
			SimpleCheckBox::DefaultHoverBoxBrush(Color(230, 230, 230, 255)),
			SimpleCheckBox::DefaultPressedBoxBrush(Color(130, 130, 130, 255)),
			SimpleCheckBox::DefaultCheckedBoxBrush(Color(80, 80, 80, 255));
		const Pen SimpleCheckBox::DefaultCheckPen(Color(0, 0, 0, 255), 2.5);
		const double SimpleCheckBox::CheckSize = 0.8;
	}
}