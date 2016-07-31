#pragma once

#ifdef DE_NO_GLEW
#	include <gl/gl.h>
#	include <gl/glu.h>
#	include <gl/glext.h>
#else
#	include <gl/glew.h>
#endif

#include <windows.h>
#include <gdiplus.h>

#include "Window.h"
#include "List.h"
#include "Color.h"
#include "Vector2.h"
#include "Rectangle.h"
#include "String.h"

namespace DE {
	namespace Graphics {
		class Renderer;
		namespace RenderingContexts {
			class RenderingContext;
			class GLContext;
		}

		struct Vertex {
			Vertex() = default;
			explicit Vertex(const Core::Math::Vector2 &pos) : Position(pos), Color(), UV() {
			}
			Vertex(const Core::Math::Vector2 &pos, const Core::Color &color) : Position(pos), Color(color), UV() {
			}
			Vertex(const Core::Math::Vector2 &pos, const Core::Color &color, const Core::Math::Vector2 &uv)
				: Position(pos), Color(color), UV(uv)
			{
			}
			Core::Math::Vector2 Position;
			Core::Color Color;
			Core::Math::Vector2 UV;
		};

		enum class RenderMode {
			Triangles,
			TriangleStrip,
			TriangleFan,
			Lines,
			LineStrip,
			Points
		};
		enum class TextureWrap {
			None,
			Repeat,
			RepeatBorder
		};
		enum class ShaderTarget {
			GLSLFragment,
			GLSLVertex,
			HLSL
		};
		enum class StencilComparisonFunction {
			Never,
			Always,
			Equal,
			NotEqual,
			Less,
			LessOrEqual,
			Greater,
			GreaterOrEqual
		};
		enum class StencilOperation {
			Keep,
			Zero,
			Replace,
			ClampedIncrease,
			ClampedDecrease,
			WrappedIncrease,
			WrappedDecrease,
			BitwiseInvert
		};

		struct TextureID {
				friend class RenderingContexts::RenderingContext;
				friend class RenderingContexts::GLContext;
				friend class Renderer;
			public:
				TextureID() {
					std::memset(&_id, 0, sizeof(_id));
				}
			protected:
				union {
					GLuint GLID;
				} _id;
		};
		struct FrameBufferID {
				friend class RenderingContexts::RenderingContext;
				friend class RenderingContexts::GLContext;
				friend class Renderer;
			public:
				FrameBufferID() {
					std::memset(&_id, 0, sizeof(_id));
				}
			protected:
				union {
					struct {
						GLuint BufID, StencilBufID;
					} GLID;
				} _id;
		};
		struct FrameBuffer {
			FrameBufferID BufferID;
			TextureID TextureID;
			Core::Math::Rectangle Region;
		};
		namespace RenderingContexts {
			class RenderingContext {
				public:
					virtual ~RenderingContext() {
					}

				    virtual Renderer CreateRenderer() = 0;

				    virtual void Begin() = 0;
				    virtual void End() = 0;

					virtual void SetViewport(const Core::Math::Rectangle&) = 0;

					virtual void SetViewbox(const Core::Math::Rectangle&) = 0;
					virtual void SetBackground(const Core::Color&) = 0;
					virtual Core::Color GetBackground() const = 0;
					virtual Gdiplus::Bitmap *GetScreenShot(const Core::Math::Rectangle&) = 0;

					// NOTE the opengl specification is partly int and partly unsigned... whatever
					// TODO Get functions
					virtual void SetStencilFunction(StencilComparisonFunction, unsigned, unsigned) = 0;
					virtual void SetStencilOperation(StencilOperation, StencilOperation, StencilOperation) = 0;
					virtual void SetClearStencilValue(unsigned) = 0;
					virtual void ClearStencil() = 0;

					virtual void SetPointSize(double) = 0;
					virtual double GetPointSize() const = 0;
					virtual void SetLineWidth(double) = 0;
					virtual double GetLineWidth() const = 0;

					virtual TextureID LoadTextureFromBitmap(Gdiplus::Bitmap&) = 0;
					virtual void DeleteTexture(TextureID) = 0;
					virtual void BindTexture(TextureID) = 0;
					virtual void UnbindTexture() = 0;
					virtual TextureID GetBoundTexture() const = 0;
					virtual double GetTextureHeight(TextureID) const = 0;
					virtual double GetTextureWidth(TextureID) const = 0;
					virtual TextureWrap GetHorizontalTextureWrap() const = 0;
					virtual void SetHorizontalTextureWrap(TextureWrap) = 0;
					virtual TextureWrap GetVerticalTextureWrap() const = 0;
					virtual void SetVerticalTextureWrap(TextureWrap) = 0;
					virtual Gdiplus::Bitmap *GetTextureImage(TextureID) const = 0;
					virtual void SetTextureImage(TextureID, Gdiplus::Bitmap&) const = 0;

					virtual FrameBuffer CreateFrameBuffer(const Core::Math::Rectangle&) = 0;
					virtual void BeginFrameBuffer(const FrameBuffer&) = 0;
					virtual void BackToDefaultFrameBuffer() = 0;
					virtual void DeleteFrameBuffer(const FrameBuffer&) = 0;

					virtual void DrawVertices(const Vertex*, size_t, RenderMode) = 0;
					virtual void DrawVertices(const Core::Math::Vector2*, const Core::Color*, const Core::Math::Vector2*, size_t, RenderMode) = 0;

					virtual void PushRectangularClip(const Core::Math::Rectangle &rect) {
						Core::Math::Rectangle tarclip = rect;
						if (_clip.Count() > 0) {
							if (Core::Math::Rectangle::Intersect(rect, _clip.Last(), tarclip) == Core::Math::IntersectionType::None) {
								tarclip = Core::Math::Rectangle(rect.Left, rect.Top, 0.0, 0.0);
							}
						}
						_clip.PushBack(tarclip);
						SetRectangularClip(tarclip);
					}
					virtual void PopRectangularClip() {
						_clip.PopBack();
						if (_clip.Count() > 0) {
							SetRectangularClip(_clip.Last());
						} else {
							ClearClip();
						}
					}
				protected:
					virtual void SetRectangularClip(const Core::Math::Rectangle&) = 0;
					virtual void ClearClip() = 0;
				private:
					Core::Collections::List<Core::Math::Rectangle> _clip;
			};
		}
	}
}
