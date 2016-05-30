#include "GLContext.h"

namespace DE {
	namespace Graphics {
		namespace RenderingContexts {
			using namespace Core;

			GLContext *&GLContext::GetCurrentContext() {
				static GLContext *_current = nullptr;
				return _current;
			}

			GLContext::GLContext(Core::Window &window) : _hWnd(window.Handle) {
#ifndef DE_NO_GLEW
				static bool _glewInitialized = false;
#endif

				PIXELFORMATDESCRIPTOR pfd;
				int iFormat;
				_hDC = GetDC(_hWnd);
				ZeroMemory(&pfd, sizeof(pfd));
				pfd.nSize = sizeof(pfd);
				pfd.nVersion = 1;
				pfd.dwFlags =
					PFD_DRAW_TO_WINDOW |
					PFD_SUPPORT_OPENGL |
					PFD_DOUBLEBUFFER;
				pfd.iPixelType = PFD_TYPE_RGBA;
				pfd.cColorBits = 24;
				pfd.cDepthBits = 16;
				pfd.iLayerType = PFD_MAIN_PLANE;
				iFormat = ChoosePixelFormat(_hDC, &pfd);
				SetPixelFormat(_hDC, iFormat, &pfd);
				_hRC = wglCreateContext(_hDC);
				MakeCurrent();
#ifndef DE_NO_GLEW
				if (!_glewInitialized) { // NOTE must be initialized by a context
					if (glewInit() != GLEW_OK) {
						throw SystemException(_TEXT("cannot initialize GLEW"));
					}
					_glewInitialized = true;
				}
#endif

				AssertGLSuccess(glEnable(GL_BLEND), "cannot enable blend");
				AssertGLSuccess(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA), "cannot set blend function");
				AssertGLSuccess(glEnable(GL_TEXTURE_2D), "cannot enable textures");
				//AssertGLSuccess(glEnable(GL_STENCIL_TEST), "cannot enable masking");
				//AssertGLSuccess(glClearStencil(1), "cannot initialize masking");
				AssertGLSuccess(glEnableClientState(GL_VERTEX_ARRAY), "cannot enable vertex array");
				AssertGLSuccess(glEnableClientState(GL_TEXTURE_COORD_ARRAY), "cannot enable UV array");
				AssertGLSuccess(glEnableClientState(GL_COLOR_ARRAY), "cannot enable color array");
				AssertGLSuccess(glEnable(GL_POINT_SMOOTH), "cannot enable anti-alias");
				AssertGLSuccess(glHint(GL_POINT_SMOOTH_HINT, GL_NICEST), "cannot enable anti-alias");
				AssertGLSuccess(glEnable(GL_LINE_SMOOTH), "cannot enable anti-alias");
				AssertGLSuccess(glHint(GL_LINE_SMOOTH_HINT, GL_NICEST), "cannot enable anti-alias");
				AssertGLSuccess(glEnable(GL_POLYGON_SMOOTH), "cannot enable anti-alias");
				AssertGLSuccess(glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST), "cannot enable anti-alias");
			}
			GLContext::~GLContext() {
				GLContext *&current = GetCurrentContext();
				if (current == this) {
					current = nullptr;
				}
				if (!wglMakeCurrent(nullptr, nullptr)) {
					throw Core::SystemException(_TEXT("cannot change the current context"));
				}
				wglDeleteContext(_hRC);
				ReleaseDC(_hWnd, _hDC);
			}
		}
	}
}
