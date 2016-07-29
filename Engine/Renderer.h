#pragma once

#include "Rectangle.h"
#include "Color.h"
#include "RenderingContext.h"
#include "ReferenceCounter.h"

namespace DE {
	namespace Graphics {
		struct Vertex;
		enum class RenderMode;
		enum class TextureWrap;
		enum class RenderingTarget;
		namespace RenderingContexts {
			class RenderingContext;
			class GLContext;
			class DirectDraw9Context;
		}

	    struct LineWidth {
                friend class Renderer;
            public:
                LineWidth(double width) : w(width) {
                }
            private:
                double w;
	    };
	    struct PointSize {
                friend class Renderer;
	        public:
	            PointSize(double size) : s(size) {
	            }
            private:
                double s;
	    };
		enum class TextAlignment {
			Left = Gdiplus::StringAlignmentNear,
			Right = Gdiplus::StringAlignmentFar,
			Center = Gdiplus::StringAlignmentCenter
		};
		class GdiPlusAccess {
#define AssertGDIPlusSuccess(OP, MSG) \
	if ((OP) != ::Gdiplus::Ok) { \
		throw ::DE::Core::SystemException(_TEXT("an error occurred in GDIPlus: " MSG)); \
	}
			public:
				static void GetEncoderCLSID(const GUID&, CLSID&);
				static void SaveBitmap(Gdiplus::Bitmap&, const Core::String&, const GUID&);

				static void GaussianBlur(Gdiplus::BitmapData&, size_t, size_t);
				static void AverageBlur(Gdiplus::BitmapData&, size_t, size_t);
				static void Pixelate(Gdiplus::BitmapData&, size_t, size_t);
			private:
				class Initializer {
					public:
						Initializer();
						~Initializer();
					private:
						ULONG_PTR _token;
				};
				static Initializer _initObj;
		};

		class Renderer;
		struct Texture {
				friend class Renderer;
			public:
				Texture() {
					memset(&_id, 0, sizeof(_id));
				}

				TextureInfo GetID() const {
					return _id;
				}
			private:
				TextureInfo _id;
		};

		class Renderer {
				friend class RenderingContexts::RenderingContext;
				friend class RenderingContexts::GLContext;
				friend class RenderingContexts::DirectDraw9Context;
			public:
				Renderer() = default;

				void Begin();
				void SetViewport(const Core::Math::Rectangle&);
				void SetViewbox(const Core::Math::Rectangle&);
				void SetBackground(const Core::Color&);
				void End();

				operator bool() const {
					return context != nullptr;
				}

				Texture LoadTextureFromFile(const Core::String&);
				Texture LoadTextureFromBitmap(Gdiplus::Bitmap&);
				Texture LoadTextureFromText(
					const Core::String&,
					const Core::String&,
					double,
					TextAlignment = TextAlignment::Left,
					double = 0.0
				);
				void UnloadTexture(const Texture&);
				Gdiplus::Bitmap *GetTextureImage(const Texture&);
				void SetTextureImage(const Texture&, Gdiplus::Bitmap&);
				void SetTexture(const Texture&);

				RenderingContexts::RenderingContext *GetContext() {
					return context;
				}

				Renderer &operator <<(const Core::Color&);
				Renderer &operator <<(const Core::Math::Vector2&);
				Renderer &operator <<(const Texture&);
				Renderer &operator <<(const Core::Math::Rectangle&);
				Renderer &operator <<(RenderingTarget);

				Renderer &operator <<(const LineWidth &width) {
					SetLineWidth(width.w);
					return *this;
				}
				Renderer &operator <<(const PointSize &size) {
					SetPointSize(size.s);
					return *this;
				}
				void SetLineWidth(double);
				void SetPointSize(double);

				void SetStencilFunction(StencilComparisonFunction, unsigned, unsigned);
				void SetStencilOperation(StencilOperation, StencilOperation, StencilOperation);
				void SetClearStencilValue(unsigned);
				void ClearStencil();

				RenderingTarget GetRenderingTarget() const;

				Renderer &PushRectangularClip(const Core::Math::Rectangle&);
				Renderer &PopRectangularClip();

				Renderer &PushBackRect(Core::Collections::List<Vertex>&, const Core::Math::Rectangle&);
				Renderer &PushBackRect(
					Core::Collections::List<Vertex>&,
					const Core::Math::Rectangle&,
					const Core::Math::Rectangle&
				);
				Renderer &PushBackRect(
					Core::Collections::List<Vertex>&,
					const Core::Math::Rectangle&,
					const Core::Math::Rectangle&,
					const Core::Color&,
					const Core::Color&,
					const Core::Color&,
					const Core::Color&
				);

				Renderer &SetVerticalTextureWrap(TextureWrap);
				Renderer &SetHorizontalTextureWrap(TextureWrap);
				TextureWrap GetVerticalTextureWrap() const;
				TextureWrap GetHorizontalTextureWrap() const;

				Renderer &DrawVertices(const Vertex*, size_t, RenderMode);
				template <bool DMA> Renderer &DrawVertices(const Core::Collections::List<Vertex, DMA> &vxs, RenderMode mode) {
					DrawVertices(*vxs, vxs.Count(), mode);
					return *this;
				}

				Gdiplus::Bitmap *GetScreenShot(const Core::Math::Rectangle&);

				Core::Color &Color() {
					return curClr;
				}
				const Core::Color &Color() const {
					return curClr;
				}
			private:
				Renderer(RenderingContexts::RenderingContext *c) : context(c), curClr() {
				}

                RenderingContexts::RenderingContext *context = nullptr;
                Core::Color curClr;
		};
	}
}
