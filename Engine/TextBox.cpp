#include "TextBox.h"

namespace DE {
	namespace UI {
		using namespace Core;
		using namespace Graphics;

		const Pen TextBox::DefaultCaretPen(Color(0, 0, 0, 255), 1.0);
		const SolidBrush
			TextBox::DefaultSelectionBrush(Color(100, 150, 200, 100)),
			TextBox::DefaultTextBoxBackground(Color(255, 255, 255, 255));
		const Pen TextBox::DefaultTextBoxBorder(Color(0, 0, 0, 255), 1.0);
	}
}