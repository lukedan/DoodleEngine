#pragma once

#include "Text.h"
#include "Control.h"

namespace DE {
	namespace UI {
		template <typename T/* = Graphics::TextRendering::BasicText*/> class ContentControl : public Control {
			public:
				virtual ~ContentControl() {
					StaticAssert(
						Core::IsBaseOf<T, Graphics::TextRendering::Text>::Result,
						"invalid content type"
					);
				}

				T &Content() {
					return _content;
				}
				const T &Content() const {
					return _content;
				}

				virtual void FitContent() {
					SetSize(_content.LayoutRectangle.TopLeft() + _content.GetSize() - _actualLayout.TopLeft());
				}
				virtual void FitContentHeight() {
					SetSize(Size(_size.Width, _content.LayoutRectangle.Top + _content.GetSize().Y - _actualLayout.Top));
				}

				virtual void Render(Graphics::Renderer &r) override {
					Control::Render(r);
					_content.Render(r);
				}
			protected:
				virtual void FinishLayoutChange() override {
					_content.LayoutRectangle = _actualLayout;
				}

				T _content;
		};
	}
}
