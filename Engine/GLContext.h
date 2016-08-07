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
				    	if (_inbuf) {
							AssertGLSuccess(glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0), "cannot bind the frame buffer");
							_inbuf = false;
							DoSetViewboxOutOfBuffer();
				    	}
				    	// viewport correction
						RECT r;
                        GetClientRect(_hWnd, &r);
                        _fvp = Core::Math::Rectangle(_vp.Left, r.bottom - _vp.Bottom, _vp.Width(), _vp.Height());
						AssertGLSuccess(glViewport(_fvp.Left, _fvp.Top, _fvp.Width(), _fvp.Height()), "cannot set viewport");
				    	// viewport correction end
				    	AssertGLSuccess(glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT), "cannot clear the background");
				    }
				    virtual void End() override {
				    	SwapBuffers(_hDC);
				    }

					virtual void SetBlendFunction(BlendFactor src, BlendFactor dst) {
						AssertGLSuccess(glBlendFunc(GetBlendFactorName(src), GetBlendFactorName(dst)), "cannot set blend function");
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
						_vbox = box;
						DoSetViewbox();
					}
					virtual void SetBackground(const Core::Color &color) override {
					    AssertGLSuccess(glClearColor(color.FloatR(), color.FloatG(), color.FloatB(), color.FloatA()), "cannot set background");
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

					virtual TextureID LoadTextureFromBitmap(Gdiplus::Bitmap &bmp) override {
						MakeCurrent();
						UINT w = bmp.GetWidth(), h = bmp.GetHeight();
						if (w == 0 || h == 0) {
							throw Core::InvalidArgumentException(_TEXT("the bitmap is invalid"));
						}
						Gdiplus::Rect r(0, 0, w, h);
						Gdiplus::BitmapData data;
						ZeroMemory(&data, sizeof(data));
						TextureID id;
						AssertGDIPlusSuccess(bmp.LockBits(&r, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &data), "cannot lock the bitmap");
						AssertGLSuccess(glGenTextures(1, &id._id.GLID), "cannot generate a texture");
						AssertGLSuccess(glBindTexture(GL_TEXTURE_2D, id._id.GLID), "cannot bind the texture");
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
					virtual void DeleteTexture(TextureID id) override {
						MakeCurrent();
						AssertGLSuccess(glDeleteTextures(1, &id._id.GLID), "cannot delete the texture");
					}
					virtual void BindTexture(TextureID id) override {
						AssertGLSuccess(glBindTexture(GL_TEXTURE_2D, id._id.GLID), "cannot bind the texture");
					}
					virtual void UnbindTexture() override {
						AssertGLSuccess(glBindTexture(GL_TEXTURE_2D, 0), "cannot unbind the texture");
					}
					virtual TextureID GetBoundTexture() const override {
						int id;
						AssertGLSuccess(glGetIntegerv(GL_TEXTURE_2D, &id), "cannot get texture index");
						TextureID tex;
						tex._id.GLID = id;
						return tex;
					}
					virtual double GetTextureHeight(TextureID id) const override {
						MakeCurrent();
						TextureID lTex = GetBoundTexture();
						AssertGLSuccess(glBindTexture(GL_TEXTURE_2D, id._id.GLID), "cannot bind the texture");
						int h;
						AssertGLSuccess(glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h), "cannot get texture height");
						AssertGLSuccess(glBindTexture(GL_TEXTURE_2D, lTex._id.GLID), "cannot bind the texture");
						return h;
					}
					virtual double GetTextureWidth(TextureID id) const override {
						MakeCurrent();
						TextureID lTex = GetBoundTexture();
						AssertGLSuccess(glBindTexture(GL_TEXTURE_2D, id._id.GLID), "cannot bind the texture");
						int w;
						AssertGLSuccess(glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w), "cannot get texture width");
						AssertGLSuccess(glBindTexture(GL_TEXTURE_2D, lTex._id.GLID), "cannot bind the texture");
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
					virtual Gdiplus::Bitmap *GetTextureImage(TextureID id) const override {
						MakeCurrent();
						TextureID lTex = GetBoundTexture();
						Gdiplus::Bitmap *bmp = new Gdiplus::Bitmap(GetTextureWidth(id), GetTextureHeight(id));
						AssertGLSuccess(glBindTexture(GL_TEXTURE_2D, id._id.GLID), "cannot bind the texture");
						Gdiplus::BitmapData data;
						ZeroMemory(&data, sizeof(data));
						Gdiplus::Rect r(0, 0, bmp->GetWidth(), bmp->GetHeight());
						AssertGDIPlusSuccess(bmp->LockBits(&r, Gdiplus::ImageLockModeWrite, PixelFormat32bppARGB, &data), "cannot lock the bitmap");
						AssertGLSuccess(glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, data.Scan0), "cannot get texture image");
						AssertGDIPlusSuccess(bmp->UnlockBits(&data), "cannot unlock the bitmap");
						AssertGLSuccess(glBindTexture(GL_TEXTURE_2D, lTex._id.GLID), "cannot bind the texture");
						return bmp;
					}
					virtual void SetTextureImage(TextureID id, Gdiplus::Bitmap &bmp) const {
						MakeCurrent();
						UINT w = bmp.GetWidth(), h = bmp.GetHeight();
						if (w == 0 || h == 0) {
							throw Core::InvalidArgumentException(_TEXT("the bitmap is invalid"));
						}
						Gdiplus::Rect r(0, 0, w, h);
						Gdiplus::BitmapData data;
						ZeroMemory(&data, sizeof(data));
						AssertGDIPlusSuccess(bmp.LockBits(&r, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &data), "cannot lock the bitmap");
						AssertGLSuccess(glBindTexture(GL_TEXTURE_2D, id._id.GLID), "cannot bind the texture");
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

					virtual FrameBuffer CreateFrameBuffer(const Core::Math::Rectangle &rect) override {
#ifndef DE_NO_GLEW
						MakeCurrent();
						FrameBuffer fbi;
						fbi.Region = rect;

						AssertGLSuccess(glGenFramebuffersEXT(1, &fbi.BufferID._id.GLID.BufID), "cannot create buffer");
						AssertGLSuccess(glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbi.BufferID._id.GLID.BufID), "cannot bind the buffer");

						AssertGLSuccess(glGenTextures(1, &fbi.TextureID._id.GLID), "cannot generate texture");
						AssertGLSuccess(glBindTexture(GL_TEXTURE_2D, fbi.TextureID._id.GLID), "cannot bind the texture");
						AssertGLSuccess(glViewport(0.0, 0.0, rect.Width(), rect.Height()), "cannot set the viewport");
						AssertGLSuccess(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR), "cannot set texture parameters");
						AssertGLSuccess(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR), "cannot set texture parameters");
						AssertGLSuccess(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE), "cannot set texture parameters");
						AssertGLSuccess(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE), "cannot set texture parameters");
						AssertGLSuccess(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0), "cannot set texture parameters");
						AssertGLSuccess(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0), "cannot set texture parameters");
						AssertGLSuccess(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, rect.Width(), rect.Height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr), "cannot set texture image");
						AssertGLSuccess(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, fbi.TextureID._id.GLID, 0), "cannot set frame buffer texture");

						AssertGLSuccess(glGenRenderbuffersEXT(1, &fbi.BufferID._id.GLID.StencilBufID), "cannot generate stencil buffer");
						AssertGLSuccess(glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, fbi.BufferID._id.GLID.StencilBufID), "cannot bind stencil buffer");
						AssertGLSuccess(glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH24_STENCIL8, rect.Width(), rect.Height()), "cannot allocate stencil buffer storage");
						AssertGLSuccess(glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER_EXT, fbi.BufferID._id.GLID.StencilBufID), "cannot attach stencil buffer");

						if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT) {
							throw Core::SystemException(_TEXT("cannot create frame buffer"));
						}
						return fbi;
#else
						return FrameBuffer();
#endif
					}
					virtual void BeginFrameBuffer(const FrameBuffer &buf) override {
#ifndef DE_NO_GLEW
						if (buf.BufferID._id.GLID.BufID == 0) {
							BackToDefaultFrameBuffer();
							return;
						}
						AssertGLSuccess(glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, buf.BufferID._id.GLID.BufID), "cannot bind the frame buffer");
						AssertGLSuccess(glViewport(0.0, 0.0, buf.Region.Width(), buf.Region.Height()), "cannot set viewport");
						if (!_inbuf) {
							_inbuf = true;
							DoSetViewboxInBuffer();
						}
						AssertGLSuccess(glClearColor(0.0, 0.0, 0.0, 0.0), "cannot set clear color");
						AssertGLSuccess(glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT), "cannot clear the buffer");
#endif
					}
					virtual void ContinueFrameBuffer(const FrameBuffer &buf) override {
#ifndef DE_NO_GLEW
						AssertGLSuccess(glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, buf.BufferID._id.GLID.BufID), "cannot bind the frame buffer");
						AssertGLSuccess(glViewport(0.0, 0.0, buf.Region.Width(), buf.Region.Height()), "cannot set viewport");
						if (!_inbuf) {
							_inbuf = true;
							DoSetViewboxInBuffer();
						}
#endif
					}
					virtual void BackToDefaultFrameBuffer() override {
#ifndef DE_NO_GLEW
						AssertGLSuccess(glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0), "cannot bind the frame buffer");
						AssertGLSuccess(glViewport(_fvp.Left, _fvp.Top, _fvp.Width(), _fvp.Height()), "cannot set viewport");
						if (_inbuf) {
							_inbuf = false;
							DoSetViewboxOutOfBuffer();
						}
#endif
					}
					virtual void DeleteFrameBuffer(const FrameBuffer &fbi) override {
#ifndef DE_NO_GLEW
						AssertGLSuccess(glDeleteRenderbuffersEXT(1, &fbi.BufferID._id.GLID.StencilBufID), "cannot delete stencil buffer");
						AssertGLSuccess(glDeleteFramebuffers(1, &fbi.BufferID._id.GLID.BufID), "cannot delete the frame buffer");
						AssertGLSuccess(glDeleteTextures(1, &fbi.TextureID._id.GLID), "cannot delete the texture");
#endif
					}

#ifndef DE_NO_GLEW
					// gl-specific functions
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
#	ifdef DEBUG
							char x[500];
							glGetInfoLogARB(prog, sizeof(x), nullptr, x);
							std::cout<<x<<std::endl;
#	endif
							glDeleteObjectARB(prog);
							throw Core::SystemException(_TEXT("error when creating the program"));
						}
						obj.ProgID = prog;
						return obj;
					}
					virtual void UseShader(const GLShaderObject &shader) {
						glUseProgram(shader.ProgID);
					}
					virtual void BindTextureToSampler(const GLShaderObject &shader, const TextureID &tex, const Core::AsciiString &name, GLint slot) {
						GLint loc = glGetUniformLocation(shader.ProgID, *name);
						glUseProgram(shader.ProgID);
						glUniform1i(loc, slot);
						glActiveTexture(GL_TEXTURE0 + slot);
						glBindTexture(GL_TEXTURE_2D, tex._id.GLID);
					}
					virtual void SetShaderVariableFloat(const GLShaderObject &shader, const Core::AsciiString &name, double value) {
						GLint loc = glGetUniformLocation(shader.ProgID, *name);
						glUseProgram(shader.ProgID);
						glUniform1f(loc, value);
					}
					virtual void SetShaderVariableInt(const GLShaderObject &shader, const Core::AsciiString &name, int value) {
						GLint loc = glGetUniformLocation(shader.ProgID, *name);
						glUseProgram(shader.ProgID);
						glUniform1i(loc, value);
					}
					virtual void SetShaderVariableVector2(const GLShaderObject &shader, const Core::AsciiString &name, const Core::Math::Vector2 &value) {
						GLint loc = glGetUniformLocation(shader.ProgID, *name);
						glUseProgram(shader.ProgID);
						glUniform2f(loc, value.X, value.Y);
					}
					virtual void SetShaderVariableColor(const GLShaderObject &shader, const Core::AsciiString &name, const Core::Color &color) {
						GLint loc = glGetUniformLocation(shader.ProgID, *name);
						glUseProgram(shader.ProgID);
						glUniform4f(loc, color.R / 255.0, color.G / 255.0, color.B / 255.0, color.A / 255.0);
					}
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
				    // TODO optimize or simplify these corrections
				    // viewport correction (when window size changed)
				    Core::Math::Rectangle _vp, _fvp;
				    // viewbox correction (between framebuffers)
				    bool _inbuf = false;
				    Core::Math::Rectangle _vbox {0.0, 0.0, 1.0, 1.0};

				    static GLContext *&GetCurrentContext();

					virtual void SetVertex(const Vertex &v) {
						AssertGLSuccess(glTexCoord2d(v.UV.X, v.UV.Y), "cannot set vertex UV");
						AssertGLSuccess(glColor4ub(v.Color.R, v.Color.G, v.Color.B, v.Color.A), "cannot set vertex color");
						AssertGLSuccess(glVertex2d(v.Position.X, v.Position.Y), "cannot set vertex position");
					}

					virtual void PrepareSetViewbox() {
						glMatrixMode(GL_PROJECTION);
						glLoadIdentity();
					}
					virtual void DoSetViewbox() {
						PrepareSetViewbox();
						if (_inbuf) {
							AssertGLSuccess(glOrtho(_vbox.Left, _vbox.Right, _vbox.Top, _vbox.Bottom, -1, 1), "cannot set ortho");
						} else {
							AssertGLSuccess(glOrtho(_vbox.Left, _vbox.Right, _vbox.Bottom, _vbox.Top, -1, 1), "cannot set ortho");
						}
				    	glMatrixMode(GL_MODELVIEW);
				    	glLoadIdentity();
					}
					virtual void DoSetViewboxInBuffer() {
						PrepareSetViewbox();
						AssertGLSuccess(glOrtho(_vbox.Left, _vbox.Right, _vbox.Top, _vbox.Bottom, -1, 1), "cannot set ortho");
				    	glMatrixMode(GL_MODELVIEW);
				    	glLoadIdentity();
					}
					virtual void DoSetViewboxOutOfBuffer() {
						PrepareSetViewbox();
						AssertGLSuccess(glOrtho(_vbox.Left, _vbox.Right, _vbox.Bottom, _vbox.Top, -1, 1), "cannot set ortho");
				    	glMatrixMode(GL_MODELVIEW);
				    	glLoadIdentity();
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
#	ifdef DEBUG
							char x[500];
							glGetInfoLogARB(id, sizeof(x), nullptr, x);
							std::cout<<x<<std::endl;
#	endif
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
					inline static GLenum GetBlendFactorName(BlendFactor fact) {
						switch (fact) {
							case BlendFactor::InvertedSourceAlpha: {
								return GL_ONE_MINUS_SRC_ALPHA;
							}
							case BlendFactor::InvertedSourceColor: {
								return GL_ONE_MINUS_SRC_COLOR;
							}
							case BlendFactor::InvertedTargetAlpha: {
								return GL_ONE_MINUS_DST_ALPHA;
							}
							case BlendFactor::InvertedTargetColor: {
								return GL_ONE_MINUS_DST_COLOR;
							}
							case BlendFactor::One: {
								return GL_ONE;
							}
							case BlendFactor::SourceAlpha: {
								return GL_SRC_ALPHA;
							}
							case BlendFactor::SourceColor: {
								return GL_SRC_COLOR;
							}
							case BlendFactor::TargetAlpha: {
								return GL_DST_ALPHA;
							}
							case BlendFactor::TargetColor: {
								return GL_DST_COLOR;
							}
							case BlendFactor::Zero: {
								return GL_ZERO;
							}
							default: {
								return GL_ZERO;
							}
						}
					}
			};
		}
	}
}
