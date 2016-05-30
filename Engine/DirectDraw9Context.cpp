#include "DirectDraw9Context.h"

namespace DE {
	namespace Graphics {
		namespace RenderingContexts {
			DirectDraw9Context::DirectDraw9Context(Core::Window &window) : hWnd(window.Handle) {
				pD3D = Direct3DCreate9(D3D_SDK_VERSION);
				D3DPRESENT_PARAMETERS param;
				ZeroMemory(&param, sizeof(param));
				param.Windowed = true;
				param.SwapEffect = D3DSWAPEFFECT_DISCARD;
				param.BackBufferFormat = D3DFMT_UNKNOWN;
				pD3D->CreateDevice(
					D3DADAPTER_DEFAULT,
					D3DDEVTYPE_HAL,
					hWnd,
					D3DCREATE_SOFTWARE_VERTEXPROCESSING,
					&param,
					&pDev
				);
			}
			DirectDraw9Context::~DirectDraw9Context() {
                pDev->Release();
                pD3D->Release();
			}
		}
	}
}