#pragma once

#include "Text.h"
#include "Control.h"

namespace DE {
	namespace UI {
		class ContentControl : public Control {
			public:
				virtual ~ContentControl() {
				}

				Graphics::TextRendering::Text &Content() {
					return _content;
				}
				const Graphics::TextRendering::Text &Content() const {
					return _content;
				}

				virtual void FitText() {
					SetSize(_content.LayoutRectangle.TopLeft() + _content.GetSize() - _actualLayout.TopLeft());
				}

				virtual void Render(Graphics::Renderer &r) override {
					Control::Render(r);
					_content.Render(r);
				}
			protected:
				virtual void FinishLayoutChange() override {
					_content.LayoutRectangle = _actualLayout;
				}

				Graphics::TextRendering::Text _content;
		};
	}
}