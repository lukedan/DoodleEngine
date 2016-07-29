#pragma once

#include <windows.h>
#include <gdiplus.h>

#include "Renderer.h"

namespace DE {
	namespace Core {
		class Window;
	}
	namespace Graphics {
		namespace RenderingContexts {
			struct GLShaderObject {
				size_t VertID = 0, FragID = 0, ProgID = 0;
			};
			class GLContext : public RenderingContext { // TODO optimization & security issues
				public:
#ifndef DISABLE_GL_CHECKINGS
#	define AssertGLSuccess(X, INFO)                                                               \
		(X);                                                                                      \
		{                                                                                         \
			GLenum tmpvar_glerror = glGetError();                                                 \
			if (tmpvar_glerror != GL_NO_ERROR) {                                                  \
				throw ::DE::Core::SystemException(_TEXT("an error occured in OpenGL:" INFO));     \
			}                                                                                     \
		}
#else
#	define AssertGLSuccess(X, INFO) (X)
#endif
					explicit GLContext(Core::Window&);
					virtual ~GLContext();

				    virtual Renderer CreateRenderer() override {
				    	return Renderer(this);
				    }

				    virtual void Begin() override {
				    	MakeCurrent();
						RECT r;
                        GetClientRect(_hWnd, &r);
                        _fvp = Core::Math::Rectangle(_vp.Left, r.bottom - _vp.Bottom, _vp.Width(), _vp.Height());
						AssertGLSuccess(glViewport(_fvp.Left, _fvp.Top, _fvp.Width(), _fvp.Height()), "cannot set viewport");
					    AssertGLSuccess(glClearColor(_bkg.FloatR(), _bkg.FloatG(), _bkg.FloatB(), _bkg.FloatA()), "cannot set background");
				    	AssertGLSuccess(glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT), "cannot clear the background");
				    }
				    virtual void End() override {
				    	SwapBuffers(_hDC);
				    }

					virtual Gdiplus::Bitmap *GetScreenShot(const Core::Math::Rectangle &region) override  {
						MakeCurrent();
						Gdiplus::Bitmap *bmp = new Gdiplus::Bitmap(region.Width(), region.Height(), PixelFormat32bppARGB);
						Gdiplus::Rect r(0, 0, region.Width(), region.Height());
						Gdiplus::BitmapData data;
						ZeroMemory(&data, sizeof(data));
						AssertGDIPlusSuccess(bmp->LockBits(&r, Gdiplus::ImageLockModeWrite, PixelFormat32bppARGB, &data), "cannot lock the bitmap");

						Core::Math::Rectangle newrect = TransformRectangle(region);
                        AssertGLSuccess(glReadPixels(newrect.Left, newrect.Top, newrect.Width(), newrect.Height(), GL_BGRA_EXT, GL_UNSIGNED_BYTE, data.Scan0), "cannot get snapshot");
						AssertGDIPlusSuccess(bmp->UnlockBits(&data), "cannot unlock the bitmap");
		                AssertGDIPlusSuccess(bmp->RotateFlip(Gdiplus::RotateNoneFlipY), "cannot flip the bitmap");
						return bmp;
					}

					virtual void SetViewport(const Core::Math::Rectangle &vp) override {
						_vp = vp;
					}

					virtual void SetViewbox(const Core::Math::Rectangle &box) override {
						AssertGLSuccess(glMatrixMode(GL_PROJECTION), "cannot set matrix mode");
						AssertGLSuccess(glLoadIdentity(), "cannot load identity matrix");
						if (_inbuf) {
							AssertGLSuccess(glOrtho(box.Left, box.Right, box.Top, box.Bottom, -1, 1), "cannot set ortho");
						} else {
							AssertGLSuccess(glOrtho(box.Left, box.Right, box.Bottom, box.Top, -1, 1), "cannot set ortho");
						}
						AssertGLSuccess(glMatrixMode(GL_MODELVIEW), "cannot set matrix mode");
						AssertGLSuccess(glLoadIdentity(), "cannot load identity matrix");
					}
					virtual void SetBackground(const Core::Color &color) override {
						_bkg = color;
					}
					virtual Core::Color GetBackground() const override {
						float d[4];
						AssertGLSuccess(glGetFloatv(GL_COLOR_BUFFER_BIT, d), "cannot get current color");
						return Core::Color::FromFloats(d[0], d[1], d[2], d[3]);
					}

					virtual void SetStencilFunction(StencilComparisonFunction func, unsigned ref, unsigned mask) {
						AssertGLSuccess(glStencilFunc(GetStencilComparisonID(func), ref, mask), "cannot set stencil function");
					}
					virtual void SetStencilOperation(StencilOperation fail, StencilOperation zfail, StencilOperation zpass) {
						AssertGLSuccess(glStencilOp(GetStencilOperationID(fail), GetStencilOperationID(zfail), GetStencilOperationID(zpass)), "cannot set stencil operation");
					}
					virtual void SetClearStencilValue(unsigned val) {
						AssertGLSuccess(glClearStencil(val), "cannot set stencil clear value");
					}
					virtual void ClearStencil() {
						AssertGLSuccess(glClear(GL_STENCIL_BUFFER_BIT), "cannot clear stencil buffer");
					}

					virtual void SetPointSize(double size) override {
						AssertGLSuccess(glPointSize(size), "cannot set point size");
					}
					virtual double GetPointSize() const override {
						float sz;
						AssertGLSuccess(glGetFloatv(GL_POINT_SIZE, &sz), "cannot get point size");
						return sz;
					}
					virtual void SetLineWidth(double width) override {
						AssertGLSuccess(glLineWidth(static_cast<float>(width)), "cannot set line width");
					}
					virtual double GetLineWidth() const override {
						float width;
						AssertGLSuccess(glGetFloatv(GL_LINE_WIDTH, &width), "cannot get line width");
						return width;
					}

					virtual TextureInfo LoadTextureFromBitmap(Gdiplus::Bitmap &bmp) override {
						MakeCurrent();
						UINT w = bmp.GetWidth(), h = bmp.GetHeight();
						if (w == 0 || h == 0) {
							throw Core::InvalidArgumentException(_TEXT("the bitmap is invalid"));
						}
						Gdiplus::Rect r(0, 0, w, h);
						Gdiplus::BitmapData data;
						ZeroMemory(&data, sizeof(data));
						TextureInfo id;
						AssertGDIPlusSuccess(bmp.LockBits(&r, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &data), "cannot lock the bitmap");
						AssertGLSuccess(glGenTextures(1, &id.GLID), "cannot generate a texture");
						AssertGLSuccess(glBindTexture(GL_TEXTURE_2D, id.GLID), "cannot bind the texture");
						AssertGLSuccess(glPixelStorei(GL_UNPACK_ALIGNMENT, 4), "cannot set pixel storage");
						AssertGLSuccess(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT), "cannot set texture parameters");
						AssertGLSuccess(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT), "cannot set texture parameters");
						AssertGLSuccess(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR), "cannot set texture parameters");
						AssertGLSuccess(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR), "cannot set texture parameters");
						static const float fs[4] {0.0f, 0.0f, 0.0f, 0.0f};
						AssertGLSuccess(glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, fs), "cannot initialize texture");
						AssertGLSuccess(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, data.Scan0), "cannot set image pixels");
						AssertGDIPlusSuccess(bmp.UnlockBits(&data), "cannot unlock the bitmap");
						return id;
					}
					virtual void DeleteTexture(TextureInfo id) override {
						MakeCurrent();
						AssertGLSuccess(glDeleteTextures(1, &id.GLID), "cannot delete the texture");
					}
					virtual void BindTexture(TextureInfo id) override {
						AssertGLSuccess(glBindTexture(GL_TEXTURE_2D, id.GLID), "cannot bind the texture");
					}
					virtual void UnbindTexture() override {
						AssertGLSuccess(glBindTexture(GL_TEXTURE_2D, 0), "cannot unbind the texture");
					}
					virtual TextureInfo GetBoundTexture() const override {
						int id;
						AssertGLSuccess(glGetIntegerv(GL_TEXTURE_2D, &id), "cannot get texture index");
						TextureInfo tex;
						tex.GLID = id;
						return tex;
					}
					virtual double GetTextureHeight(TextureInfo id) const override {
						MakeCurrent();
						TextureInfo lTex = GetBoundTexture();
						AssertGLSuccess(glBindTexture(GL_TEXTURE_2D, id.GLID), "cannot bind the texture");
						int h;
						AssertGLSuccess(glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h), "cannot get texture height");
						AssertGLSuccess(glBindTexture(GL_TEXTURE_2D, lTex.GLID), "cannot bind the texture");
						return h;
					}
					virtual double GetTextureWidth(TextureInfo id) const override {
						MakeCurrent();
						TextureInfo lTex = GetBoundTexture();
						AssertGLSuccess(glBindTexture(GL_TEXTURE_2D, id.GLID), "cannot bind the texture");
						int w;
						AssertGLSuccess(glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w), "cannot get texture width");
						AssertGLSuccess(glBindTexture(GL_TEXTURE_2D, lTex.GLID), "cannot bind the texture");
						return w;
					}
					virtual TextureWrap GetHorizontalTextureWrap() const override {
						MakeCurrent();
						int mode;
						AssertGLSuccess(glGetIntegerv(GL_TEXTURE_WRAP_S, &mode), "cannot get texture wrap");
						return GetWrapMode(mode);
					}
					virtual void SetHorizontalTextureWrap(TextureWrap w) override {
						MakeCurrent();
						AssertGLSuccess(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GetWrapModeID(w)), "cannot set texture wrap");
					}
					virtual TextureWrap GetVerticalTextureWrap() const override {
						MakeCurrent();
						int mode;
						AssertGLSuccess(glGetIntegerv(GL_TEXTURE_WRAP_T, &mode), "cannot get texture wrap");
						return GetWrapMode(mode);
					}
					virtual void SetVerticalTextureWrap(TextureWrap w) override {
						MakeCurrent();
						AssertGLSuccess(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GetWrapModeID(w)), "cannot set texture wrap");
					}
					virtual Gdiplus::Bitmap *GetTextureImage(TextureInfo id) const override {
						MakeCurrent();
						TextureInfo lTex = GetBoundTexture();
						Gdiplus::Bitmap *bmp = new Gdiplus::Bitmap(GetTextureWidth(id), GetTextureHeight(id));
						AssertGLSuccess(glBindTexture(GL_TEXTURE_2D, id.GLID), "cannot bind the texture");
						Gdiplus::BitmapData data;
						ZeroMemory(&data, sizeof(data));
						Gdiplus::Rect r(0, 0, bmp->GetWidth(), bmp->GetHeight());
						AssertGDIPlusSuccess(bmp->LockBits(&r, Gdiplus::ImageLockModeWrite, PixelFormat32bppARGB, &data), "cannot lock the bitmap");
						AssertGLSuccess(glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, data.Scan0), "cannot get texture image");
						AssertGDIPlusSuccess(bmp->UnlockBits(&data), "cannot unlock the bitmap");
						AssertGLSuccess(glBindTexture(GL_TEXTURE_2D, lTex.GLID), "cannot bind the texture");
						return bmp;
					}
					virtual void SetTextureImage(TextureInfo id, Gdiplus::Bitmap &bmp) const {
						MakeCurrent();
						UINT w = bmp.GetWidth(), h = bmp.GetHeight();
						if (w == 0 || h == 0) {
							throw Core::InvalidArgumentException(_TEXT("the bitmap is invalid"));
						}
						Gdiplus::Rect r(0, 0, w, h);
						Gdiplus::BitmapData data;
						ZeroMemory(&data, sizeof(data));
						AssertGDIPlusSuccess(bmp.LockBits(&r, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &data), "cannot lock the bitmap");
						AssertGLSuccess(glBindTexture(GL_TEXTURE_2D, id.GLID), "cannot bind the texture");
						AssertGLSuccess(glTexImage2D(GL_TEXTURE_2D, 0, 4, w, h, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, data.Scan0), "cannot set image pixels");
						AssertGDIPlusSuccess(bmp.UnlockBits(&data), "cannot unlock the bitmap");
					}

					virtual void DrawVertices(const Vertex *vs, size_t count, RenderMode mode) override {
						if (count > 0) {
							if (vs == nullptr) {
								throw Core::InvalidArgumentException(_TEXT("the vertex list is null"));
							}
							AssertGLSuccess(glTexCoordPointer(2, GL_DOUBLE, sizeof(Vertex), &vs[0].UV), "cannot set UV pointer");
							AssertGLSuccess(glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(Vertex), &vs[0].Color), "cannot set color pointer");
							AssertGLSuccess(glVertexPointer(2, GL_DOUBLE, sizeof(Vertex), &vs[0].Position), "cannot set vertex pointer");
							AssertGLSuccess(glDrawArrays(GetModeID(mode), 0, count), "cannot draw the vertices");
						}
					}
					virtual void DrawVertices(
						const Core::Math::Vector2 *poss,
						const Core::Color *clrs,
						const Core::Math::Vector2 *uvs,
						size_t count,
						RenderMode mode
					) override {
						if (count > 0) {
							AssertGLSuccess(glTexCoordPointer(2, GL_DOUBLE, sizeof(Core::Math::Vector2), uvs), "cannot set UV pointer");
							AssertGLSuccess(glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(Core::Color), clrs), "cannot set color pointer");
							AssertGLSuccess(glVertexPointer(2, GL_DOUBLE, sizeof(Core::Math::Vector2), poss), "cannot set vertex pointer");
							AssertGLSuccess(glDrawArrays(GetModeID(mode), 0, count), "cannot draw the vertices");
						}
					}

					virtual FrameBufferInfo CreateFrameBuffer(const Core::Math::Rectangle &rect) override {
#ifndef DE_NO_GLEW
						MakeCurrent();
						FrameBufferInfo fbi;
						fbi.Region = rect;
						AssertGLSuccess(glGenFramebuffersEXT(1, &fbi.ID.GLID), "cannot create buffer");
						AssertGLSuccess(glGenTextures(1, &fbi.TextureID.GLID), "cannot generate texture");
						AssertGLSuccess(glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbi.ID.GLID), "cannot bind the buffer");
						AssertGLSuccess(glBindTexture(GL_TEXTURE_2D, fbi.TextureID.GLID), "cannot bind the texture");
						AssertGLSuccess(glViewport(0.0, 0.0, rect.Width(), rect.Height()), "cannot set the viewport");
						AssertGLSuccess(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR), "cannot set texture parameters");
						AssertGLSuccess(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR), "cannot set texture parameters");
						AssertGLSuccess(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE), "cannot set texture parameters");
						AssertGLSuccess(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE), "cannot set texture parameters");
						AssertGLSuccess(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0), "cannot set texture parameters");
						AssertGLSuccess(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0), "cannot set texture parameters");
						AssertGLSuccess(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, rect.Width(), rect.Height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr), "cannot set texture image");
						AssertGLSuccess(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, fbi.TextureID.GLID, 0), "cannot set frame buffer texture");
						if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT) {
							throw Core::SystemException(_TEXT("cannot create frame buffer"));
						}
						return fbi;
#else
						return FrameBufferInfo();
#endif
					}
					virtual void BeginFrameBuffer(const FrameBufferInfo &fbi) override {
#ifndef DE_NO_GLEW
						AssertGLSuccess(glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbi.ID.GLID), "cannot bind the frame buffer");
						AssertGLSuccess(glClearColor(0.0, 0.0, 0.0, 0.0), "cannot set clear color");
						AssertGLSuccess(glClear(GL_COLOR_BUFFER_BIT), "cannot clear the buffer");
						AssertGLSuccess(glViewport(0.0, 0.0, fbi.Region.Width(), fbi.Region.Height()), "cannot set viewport");
						_inbuf = true;
#endif
					}
					virtual void BackToDefaultFrameBuffer() override {
#ifndef DE_NO_GLEW
						AssertGLSuccess(glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0), "cannot bind the frame buffer");
						AssertGLSuccess(glViewport(_fvp.Left, _fvp.Top, _fvp.Width(), _fvp.Height()), "cannot set viewport");
						_inbuf = false;
#endif
					}
					virtual void DeleteFrameBuffer(const FrameBufferInfo &fbi) override {
#ifndef DE_NO_GLEW
						AssertGLSuccess(glDeleteFramebuffers(1, &fbi.ID.GLID), "cannot delete the frame buffer");
						AssertGLSuccess(glDeleteTextures(1, &fbi.TextureID.GLID), "cannot delete the texture");
#endif
					}

#ifndef DE_NO_GLEW
					virtual GLShaderObject CreateShader(const Core::AsciiString &vert, const Core::AsciiString &frag) {
						GLShaderObject obj;
						if (vert.Length() > 0) {
							obj.VertID = CreateSingleShader(GL_VERTEX_SHADER_ARB, vert);
						}
						if (frag.Length() > 0) {
							obj.FragID = CreateSingleShader(GL_FRAGMENT_SHADER_ARB, frag);
						}
						GLhandleARB prog = glCreateProgramObjectARB();
						if (obj.VertID > 0) {
							glAttachObjectARB(prog, obj.VertID);
						}
						if (obj.FragID > 0) {
							glAttachObjectARB(prog, obj.FragID);
						}
						glLinkProgramARB(prog);
						int res;
						glGetObjectParameterivARB(prog, GL_OBJECT_LINK_STATUS_ARB, &res);
						if (!res) {
#ifdef DEBUG
							char x[500];
							glGetInfoLogARB(prog, sizeof(x), nullptr, x);
							std::cout<<x<<std::endl;
#endif
							glDeleteObjectARB(prog);
							throw Core::SystemException(_TEXT("error when creating the program"));
						}
						obj.ProgID = prog;
						return obj;
					}
#endif
					virtual void UseShader(size_t shader) {
#ifndef DE_NO_GLEW
						glUseProgram(shader);
#endif
					}
#ifndef DE_NO_GLEW
					virtual void DeleteShader(const GLShaderObject &obj) {
						if (obj.VertID > 0) {
							glDeleteObjectARB(obj.VertID);
						}
						if (obj.FragID > 0) {
							glDeleteObjectARB(obj.FragID);
						}
						glDeleteObjectARB(obj.ProgID);
					}
#endif
				protected:
					virtual void SetRectangularClip(const Core::Math::Rectangle &rect) override {
						AssertGLSuccess(glEnable(GL_SCISSOR_TEST), "cannot enable scissor test");
						Core::Math::Rectangle newrect = TransformRectangle(rect);
						double aw = ceil(rect.Right) - floor(rect.Left), ah = ceil(rect.Bottom) - floor(rect.Top);
						AssertGLSuccess(glScissor(floor(newrect.Left), floor(newrect.Top), aw, ah), "cannot scissor the region");
					}
					virtual void ClearClip() override {
						AssertGLSuccess(glDisable(GL_SCISSOR_TEST), "cannot disable scissor test");
					}

					Core::Math::Rectangle TransformRectangle(const Core::Math::Rectangle &rect) {
						RECT client;
						GetClientRect(_hWnd, &client);
						return Core::Math::Rectangle(rect.Left, client.bottom - rect.Bottom, rect.Width(), rect.Height());
					}
				private:
				    HGLRC _hRC;
				    HDC _hDC;
				    HWND _hWnd;
				    Core::Color _bkg;
				    Core::Math::Rectangle _vp, _fvp;
				    bool _inbuf = false;

				    static GLContext *&GetCurrentContext();

					virtual void SetVertex(const Vertex &v) {
						AssertGLSuccess(glTexCoord2d(v.UV.X, v.UV.Y), "cannot set vertex UV");
						AssertGLSuccess(glColor4ub(v.Color.R, v.Color.G, v.Color.B, v.Color.A), "cannot set vertex color");
						AssertGLSuccess(glVertex2d(v.Position.X, v.Position.Y), "cannot set vertex position");
					}

#ifndef DE_NO_GLEW
					virtual GLhandleARB CreateSingleShader(GLenum type, const Core::AsciiString &prog) {
						GLhandleARB id = glCreateShaderObjectARB(type);
						const char *sprog = *prog;
						glShaderSourceARB(id, 1, &sprog, nullptr);
						glCompileShaderARB(id);
						int res;
						glGetObjectParameterivARB(id, GL_OBJECT_COMPILE_STATUS_ARB, &res);
						if (!res) {
#ifdef DEBUG
							char x[500];
							glGetInfoLogARB(id, sizeof(x), nullptr, x);
							std::cout<<x<<std::endl;
#endif
							glDeleteObjectARB(id);
							throw Core::SystemException(_TEXT("cannot create vertex shader"));
						}
						return id;
					}
#endif

					virtual void MakeCurrent() const {
						GLContext *&current = GetCurrentContext();
						if (current != this) {
							if (!wglMakeCurrent(_hDC, _hRC)) {
								throw Core::SystemException(_TEXT("cannot make the device context to be the current one"));
							}
							current = const_cast<GLContext*>(this);
						}
					}

					inline static GLenum GetModeID(RenderMode mode) {
						switch (mode) {
                            case RenderMode::Lines: {
                            	return GL_LINES;
                            }
                            case RenderMode::LineStrip: {
                            	return GL_LINE_STRIP;
                            }
                            case RenderMode::Points: {
                            	return GL_POINTS;
                            }
                            case RenderMode::Triangles: {
                            	return GL_TRIANGLES;
                            }
                            case RenderMode::TriangleFan: {
                            	return GL_TRIANGLE_FAN;
                            }
                            case RenderMode::TriangleStrip: {
                            	return GL_TRIANGLE_STRIP;
                            }
                            default: {
                            	return 0;
                            }
						}
					}
					inline static GLenum GetWrapModeID(TextureWrap wrap) {
						switch (wrap) {
							case TextureWrap::Repeat: {
								return GL_REPEAT;
							}
							case TextureWrap::RepeatBorder: {
								return GL_CLAMP;
							}
							case TextureWrap::None: {
								return GL_CLAMP_TO_BORDER;
							}
							default: {
								return 0;
							}
						}
					}
					inline static TextureWrap GetWrapMode(GLenum mode) {
						switch (mode) {
							case GL_REPEAT: {
								return TextureWrap::Repeat;
							}
							case GL_CLAMP: {
								return TextureWrap::RepeatBorder;
							}
							case GL_CLAMP_TO_BORDER: {
								return TextureWrap::None;
							}
							default: {
								return TextureWrap::None;
							}
						}
					}
					inline static GLenum GetStencilComparisonID(StencilComparisonFunction func) {
						switch (func) {
							case StencilComparisonFunction::Always: {
								return GL_ALWAYS;
							}
							case StencilComparisonFunction::Equal: {
								return GL_EQUAL;
							}
							case StencilComparisonFunction::Greater: {
								return GL_GREATER;
							}
							case StencilComparisonFunction::GreaterOrEqual: {
								return GL_GEQUAL;
							}
							case StencilComparisonFunction::Less: {
								return GL_LESS;
							}
							case StencilComparisonFunction::LessOrEqual: {
								return GL_LEQUAL;
							}
							case StencilComparisonFunction::Never: {
								return GL_NEVER;
							}
							case StencilComparisonFunction::NotEqual: {
								return GL_NOTEQUAL;
							}
							default: {
								return GL_ALWAYS;
							}
						}
					}
					inline static GLenum GetStencilOperationID(StencilOperation op) {
						switch (op) {
							case StencilOperation::BitwiseInvert: {
								return GL_INVERT;
							}
							case StencilOperation::ClampedDecrease: {
								return GL_DECR;
							}
							case StencilOperation::ClampedIncrease: {
								return GL_INCR;
							}
							case StencilOperation::Keep: {
								return GL_KEEP;
							}
							case StencilOperation::Replace: {
								return GL_REPLACE;
							}
							case StencilOperation::WrappedDecrease: {
								return GL_DECR_WRAP;
							}
							case StencilOperation::WrappedIncrease: {
								return GL_INCR_WRAP;
							}
							case StencilOperation::Zero: {
								return GL_ZERO;
							}
							default: {
								return GL_KEEP;
							}
						}
					}
			};
		}
	}
}
