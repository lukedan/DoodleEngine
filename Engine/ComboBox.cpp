#include "ComboBox.h"

namespace DE {
	namespace UI {
		using namespace Core;
		using namespace Core::Input;
		using namespace Graphics;

		const SolidBrush
			SimpleComboBox::DefaultNormalBrush(Color(255, 255, 255, 255)),
			SimpleComboBox::DefaultHoverBrush(Color(150, 150, 150, 255)),
			SimpleComboBox::DefaultPressedBrush(Color(100, 100, 100, 255)),
			SimpleComboBox::DefaultSelectedBrush(Color(100, 150, 255, 255));

		void SimpleComboBox::SetSelectedItem(Item *item) {
			if (item && &(item->_father) != this) {
				throw InvalidOperationException(_TEXT("the item doesn't belong to this ComboBox"));
			}
			if (_selection) {
				SetDefaultBrushes(_selection->_btn);
			}
			_selection = item;
			if (_selection) {
				SetSelectedBrushes(_selection->_btn);
				Content() = _selection->Content();
				Content().LayoutRectangle = _actualLayout;
			} else {
				Content().Content = _TEXT("");
			}
			OnSelectionChanged(ComboBoxSelectionChangedInfo(item));
		}
		void SimpleComboBox::OnSelectionChanged(const ComboBoxSelectionChangedInfo &info) {
			SelectionChanged(info);
		}
	}
}