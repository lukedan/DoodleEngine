#include "ScrollBar.h"

namespace DE {
    namespace UI {
        using namespace Core;
        using namespace Graphics;

        const SolidBrush
            ScrollBarBase::DefaultPageButtonNormalBrush(Color(200, 200, 200, 255)),
            ScrollBarBase::DefaultPageButtonHoverBrush(Color(220, 220, 220, 255)),
            ScrollBarBase::DefaultPageButtonPressedBrush(Color(150, 150, 150, 255));
    }
}