#pragma once

#include <d3d9.h>

#include "RenderingContext.h"

namespace DE {
	namespace Core {
		class Window;
	}
	namespace Graphics {
		namespace RenderingContexts {
			class RenderingContext;
			class DirectDraw9Context : public RenderingContext {
				public:
					explicit DirectDraw9Context(Core::Window&);
					virtual ~DirectDraw9Context();

				    /*virtual Renderer CreateRenderer() {
				    	return Renderer(this);
				    }

				    virtual void Begin() {
				    	pDev->Clear(0, nullptr, D3DCLEAR_TARGET, clearClr, 1.0f, 0);
				    	pDev->BeginScene();
				    }
				    virtual void End() {
				    	pDev->EndScene();
				    	pDev->Present(nullptr, nullptr, nullptr, nullptr);
				    }

					virtual void LoadTextureFromBitmap(Gdiplus::Bitmap&, size_t&, size_t&, size_t&) {
					}
					virtual void DeleteTexture(const Texture&) {
					}

					virtual void SetViewport(const Core::Math::Rectangle &rect) {
						//D3DXMatrixOrthoLH(&ortho, rect.Width(), rect.Height(), -1.0, 1.0);
						pDev->SetTransform(D3DTS_PROJECTION, &ortho);
					}
					virtual void SetViewbox(const Core::Math::Rectangle&) {
					}
					virtual void SetBackground(const Core::Color &color) {
						clearClr = GetValue(color);
					}
					virtual Core::Color GetBackground() const {
						return GetColor(clearClr);
					}
					virtual void SetPointSize(double) {
					}
					virtual double GetPointSize() const {
					}
					virtual void SetLineWidth(double) {
					}
					virtual double GetLineWidth() const {
					}
					virtual void BindTexture(const Texture&) {
					}
					virtual void UnbindTexture() {
					}
					virtual void GetBoundTexture(size_t&, size_t&, size_t&) const {
					}

					virtual void DrawVertex(const Vertex &v) {
						pDev->DrawPrimitiveUP(D3DPT_POINTLIST, 1, &v, sizeof(Vertex));
					}
					virtual void DrawLine(const Vertex&, const Vertex&) {
					}
					virtual void DrawVertices(const Vertex*, size_t, RenderMode) {
					}*/
				private:
					LPDIRECT3D9 pD3D = nullptr;
					LPDIRECT3DDEVICE9 pDev = nullptr;
					HWND hWnd;
					D3DMATRIX ortho;
					unsigned long clearClr = 0xFFFFFFFF;

					static int GetModeID(RenderMode mode) {
						switch (mode) {
							case RenderMode::Lines: {
								return D3DPT_LINELIST;
							}
							case RenderMode::LineStrip: {
								return D3DPT_LINESTRIP;
							}
							case RenderMode::Points: {
								return D3DPT_POINTLIST;
							}
							case RenderMode::Triangles: {
								return D3DPT_TRIANGLELIST;
							}
							case RenderMode::TriangleFan: {
								return D3DPT_TRIANGLEFAN;
							}
							case RenderMode::TriangleStrip: {
								return D3DPT_TRIANGLESTRIP;
							}
							default: {
								return 0;
							}
						}
					}

					static unsigned long GetValue(const Core::Color &color) {
						return D3DCOLOR_RGBA(color.R, color.G, color.B, color.A);
					}
					static Core::Color GetColor(unsigned long color) {
						return Core::Color((color & 0xFF0000)>>16, (color & 0xFF00)>>8, color & 0xFF, (color & 0xFF000000)>>24);
					}
			};
		}
	}
}
