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

		class Renderer {
				friend class RenderingContexts::RenderingContext;
				friend class RenderingContexts::GLContext;
				friend class RenderingContexts::DirectDraw9Context;
			public:
				Renderer() = default;

				void Begin() {
					if (_ctx) {
						_ctx->Begin();
					}
				}
				void SetViewport(const Core::Math::Rectangle&);
				void SetViewbox(const Core::Math::Rectangle &box) {
					if (_ctx) {
						_ctx->SetViewbox(box);
					}
				}
				void SetBackground(const Core::Color &back) {
					if (_ctx) {
						_ctx->SetBackground(back);
					}
				}
				void End() {
					if (_ctx) {
						_ctx->End();
					}
				}

				operator bool() const {
					return _ctx != nullptr;
				}

				TextureID LoadTextureFromFile(const Core::String &fileName) {
					Gdiplus::Bitmap b(*fileName);
					return LoadTextureFromBitmap(b);
				}
				TextureID LoadTextureFromBitmap(Gdiplus::Bitmap &bmp) {
					if (_ctx) {
						return _ctx->LoadTextureFromBitmap(bmp);
					}
					return TextureID();
				}
				void UnloadTexture(const TextureID &tex) {
					if (_ctx) {
						_ctx->DeleteTexture(tex);
					}
				}
				Gdiplus::Bitmap *GetTextureImage(const TextureID &tex) {
					if (_ctx) {
						return _ctx->GetTextureImage(tex);
					}
					return nullptr;
				}
				void SetTextureImage(const TextureID &tex, Gdiplus::Bitmap &bmp) {
					if (_ctx) {
						TextureID curTex = _ctx->GetBoundTexture();
						_ctx->SetTextureImage(tex, bmp);
						_ctx->BindTexture(curTex);
					}
				}
				void BindTexture(const TextureID &tex) {
					if (_ctx) {
						_ctx->BindTexture(tex);
					}
				}

				RenderingContexts::RenderingContext *GetContext() {
					return _ctx;
				}

				void SetLineWidth(double width) {
					if (_ctx) {
						_ctx->SetLineWidth(width);
					}
				}
				void SetPointSize(double size) {
					if (_ctx) {
						_ctx->SetPointSize(size);
					}
				}

				void SetStencilFunction(StencilComparisonFunction func, unsigned ref, unsigned mask) {
					if (_ctx) {
						_ctx->SetStencilFunction(func, ref, mask);
					}
				}
				void SetStencilOperation(StencilOperation fail, StencilOperation zfail, StencilOperation zpass) {
					if (_ctx) {
						_ctx->SetStencilOperation(fail, zfail, zpass);
					}
				}
				void SetClearStencilValue(unsigned v) {
					if (_ctx) {
						_ctx->SetClearStencilValue(v);
					}
				}
				void ClearStencil() {
					if (_ctx) {
						_ctx->ClearStencil();
					}
				}

				RenderingTarget GetRenderingTarget() const;

				void PushRectangularClip(const Core::Math::Rectangle &rect) {
					if (_ctx) {
						_ctx->PushRectangularClip(rect);
					}
				}
				void PopRectangularClip() {
					if (_ctx) {
						_ctx->PopRectangularClip();
					}
				}

				void SetVerticalTextureWrap(TextureWrap wrap) {
					if (_ctx) {
						_ctx->SetVerticalTextureWrap(wrap);
					}
				}
				void SetHorizontalTextureWrap(TextureWrap wrap) {
					if (_ctx) {
						_ctx->SetHorizontalTextureWrap(wrap);
					}
				}
				TextureWrap GetVerticalTextureWrap() const {
					if (_ctx) {
						return _ctx->GetVerticalTextureWrap();
					}
					return TextureWrap::None;
				}
				TextureWrap GetHorizontalTextureWrap() const {
					if (_ctx) {
						return _ctx->GetHorizontalTextureWrap();
					}
					return TextureWrap::None;
				}

				FrameBuffer CreateFrameBuffer(const Core::Math::Rectangle &rect) {
					if (_ctx) {
						return _ctx->CreateFrameBuffer(rect);
					}
					return FrameBuffer();
				}
				void BeginFrameBuffer(const FrameBuffer &buf) {
					if (_ctx) {
						_ctx->BeginFrameBuffer(buf);
					}
				}
				void BackToDefaultFrameBuffer() {
					if (_ctx) {
						_ctx->BackToDefaultFrameBuffer();
					}
				}
				void DeleteFrameBuffer(const FrameBuffer &buf) {
					if (_ctx) {
						_ctx->DeleteFrameBuffer(buf);
					}
				}

				void DrawVertices(const Vertex *vs, size_t count, RenderMode mode) {
					if (_ctx) {
						_ctx->DrawVertices(vs, count, mode);
					}
				}
				template <bool DMA> void DrawVertices(const Core::Collections::List<Vertex, DMA> &vxs, RenderMode mode) {
					DrawVertices(*vxs, vxs.Count(), mode);
				}

				Gdiplus::Bitmap *GetScreenShot(const Core::Math::Rectangle &rect) {
					if (_ctx) {
						return _ctx->GetScreenShot(rect);
					}
					return nullptr;
				}

			private:
				Renderer(RenderingContexts::RenderingContext *c) : _ctx(c) {
				}

                RenderingContexts::RenderingContext *_ctx = nullptr;
		};
	}
}
