#include "Renderer.h"

#include "List.h"
#include "Queue.h"

namespace DE {
	namespace Graphics {
		using namespace Gdiplus;
		using namespace Core::Math;
		using namespace Core::Collections;
		using namespace RenderingContexts;

		GdiPlusAccess::Initializer::Initializer() {
			if (_token == 0) {
				GdiplusStartupInput input;
				AssertGDIPlusSuccess(GdiplusStartup(&_token, &input, nullptr), "cannot initialize GdiPlus");
			}
		}
		GdiPlusAccess::Initializer::~Initializer() {
			GdiplusShutdown(_token);
		}

		GdiPlusAccess::Initializer GdiPlusAccess::_initObj;
		void GdiPlusAccess::GetEncoderCLSID(const GUID &format, CLSID &clsid) {
			UINT num = 0, size = 0;
			ImageCodecInfo *pImageCodecInfo = nullptr;
			AssertGDIPlusSuccess(GetImageEncodersSize(&num, &size), "cannot get encoder size");
			if (size == 0) {
				throw Core::SystemException(_TEXT("encoder not found"));
			}
			pImageCodecInfo = (ImageCodecInfo*)Core::GlobalAllocator::Allocate(size);
			AssertGDIPlusSuccess(GetImageEncoders(num, size, pImageCodecInfo), "cannot get encoder info");
			bool got = false;
			for (size_t j = 0; j < num; j++) {
				if (pImageCodecInfo[j].FormatID == format) {
					clsid = pImageCodecInfo[j].Clsid;
					got = true;
					break;
				}
			}
			Core::GlobalAllocator::Free(pImageCodecInfo);
			if (!got) {
				throw Core::SystemException(_TEXT("encoder not found"));
			}
		}
		void GdiPlusAccess::SaveBitmap(Gdiplus::Bitmap &bmp, const Core::String &fileName, const GUID &format) {
			CLSID encoder;
			GetEncoderCLSID(format, encoder);
			AssertGDIPlusSuccess(bmp.Save(*fileName, &encoder, nullptr), "cannot save bitmap");
		}

		template <size_t sz> struct DArr {
			DArr() {
				memset(V, 0, sizeof(V));
			}
			DArr<sz> &operator +=(const DArr<sz> &rhs) {
				for (size_t i = 0; i < sz; ++i) {
					V[i] += rhs.V[i];
				}
				return *this;
			}
			DArr<sz> operator +(const DArr<sz> &rhs) const {
				DArr<sz> result = (*this);
				result += rhs;
				return result;
			}
			DArr<sz> &operator -=(const DArr<sz> &rhs) {
				for (size_t i = 0; i < sz; ++i) {
					V[i] -= rhs.V[i];
				}
				return *this;
			}
			DArr<sz> operator -(const DArr<sz> &rhs) const {
				DArr<sz> result = (*this);
				result += rhs;
				return result;
			}
			DArr<sz> &operator *=(double rhs) {
				for (size_t i = 0; i < sz; ++i) {
					V[i] *= rhs;
				}
				return *this;
			}
			DArr<sz> operator *(double rhs) {
				DArr<sz> result = (*this);
				result *= rhs;
				return result;
			}
			DArr<sz> &operator /=(double rhs) {
				for (size_t i = 0; i < sz; ++i) {
					V[i] /= rhs;
				}
				return *this;
			}
			DArr<sz> operator /(double rhs) {
				DArr<sz> result = (*this);
				result /= rhs;
				return result;
			}

			double V[sz];
		};
		Core::Color &GetColor(Gdiplus::BitmapData &data, size_t x, size_t y) {
			// NOTE default pixel format ARGB
			return *reinterpret_cast<Core::Color*>(reinterpret_cast<size_t>(data.Scan0) + y * Abs(data.Stride) + x * sizeof(Core::Color));
		}
		DArr<4> ColorToDArr(const Core::Color &c) {
			DArr<4> result;
			result.V[0] = static_cast<double>(c.R);
			result.V[1] = static_cast<double>(c.G);
			result.V[2] = static_cast<double>(c.B);
			result.V[3] = static_cast<double>(c.A);
			return result;
		}
		Core::Color DArrToColor(const DArr<4> &arr) {
			Core::Color result;
			result.R = static_cast<unsigned char>(round(arr.V[0]));
			result.G = static_cast<unsigned char>(round(arr.V[1]));
			result.B = static_cast<unsigned char>(round(arr.V[2]));
			result.A = static_cast<unsigned char>(round(arr.V[3]));
			return result;
		}
		void GdiPlusAccess::GaussianBlur(Gdiplus::BitmapData &data, size_t xRadius, size_t yRadius) {
			List<List<double>> lds;
			double tot = 0.0;
			int ixrad = static_cast<int>(xRadius), iyrad = static_cast<int>(yRadius);
			for (int x = -ixrad; x <= ixrad; ++x) {
				lds.PushBack(List<double>());
				List<double> &tarList = lds.Last();
				for (int y = -iyrad; y <= iyrad; ++y) {
					double dt = 0.0;
					if (xRadius != 0) {
						dt += Square(x / static_cast<double>(xRadius));
					}
					if (yRadius != 0) {
						dt += Square(y / static_cast<double>(yRadius));
					}
					double dis = 1.0 - sqrt(dt);
					if (dis < 0.0) {
						dis = 0.0;
					}
					tarList.PushBack(dis);
					tot += dis;
				}
			}
			for (int y = -iyrad; y <= iyrad; ++y) {
				for (int x = -ixrad; x <= ixrad; ++x) {
					lds[x + ixrad][y + iyrad] /= tot;
				}
			}
			List<List<Core::Color>> result;
			for (size_t x = 0; x < data.Width; ++x) {
				result.PushBack(List<Core::Color>());
				List<Core::Color> &tarList = result.Last();
				for (size_t y = 0; y < data.Height; ++y) {
					DArr<4> curPx;
					for (int dx = -ixrad; dx <= ixrad; ++dx) {
						size_t px = ((-dx) > static_cast<int>(x) ? 0 : static_cast<size_t>(static_cast<int>(x) + dx));
						if (px >= data.Width) {
							px = data.Width - 1;
						}
						for (int dy = -iyrad; dy <= iyrad; ++dy) {
							size_t py = ((-dy) > static_cast<int>(y) ? 0 : static_cast<size_t>(static_cast<int>(y) + dy));
							if (py >= data.Height) {
								py = data.Height - 1;
							}
							double mult = lds[dx + ixrad][dy + iyrad];
							curPx += ColorToDArr(GetColor(data, px, py)) * mult;
						}
					}
					tarList.PushBack(DArrToColor(curPx));
				}
			}
			for (size_t x = 0; x < data.Width; ++x) {
				for (size_t y = 0; y < data.Height; ++y) {
					GetColor(data, x, y) = result[x][y];
				}
			}
		}
		void GdiPlusAccess::AverageBlur(Gdiplus::BitmapData &data, size_t xRadius, size_t yRadius) {
			struct ColumnCache {
				List<DArr<4>> Content;
				size_t Index = 0;
			};
			Queue<ColumnCache> cache;
			List<List<DArr<4>>> result;
			for (size_t x = 0; x < data.Width; ++x) {
				for (size_t cx = (cache.Count() == 0 ? 0 : x + xRadius); cx < data.Width && cx <= x + xRadius; ++cx) {
					ColumnCache curCol;
					curCol.Index = cx;
					curCol.Content.PushBack(ColorToDArr(GetColor(data, cx, 0)) * static_cast<double>(yRadius + 1));
					for (size_t ay = 1; ay <= yRadius; ++ay) {
						curCol.Content.Last() += ColorToDArr(GetColor(data, cx, (ay >= data.Height ? data.Height - 1 : ay)));
					}
					for (size_t cy = 1; cy < data.Height; ++cy) {
						DArr<4> lpx = curCol.Content.Last();
						curCol.Content.PushBack(lpx);
						DArr<4> &curPx = curCol.Content.Last();
						curPx -= ColorToDArr(GetColor(data, cx, (cy < yRadius + 1 ? 0 : cy - yRadius - 1)));
						curPx += ColorToDArr(GetColor(data, cx, Core::Math::Min(cy + yRadius, static_cast<size_t>(data.Height) - 1)));
					}
					cache.PushHead(curCol);
				}
				while (cache.PeekTail().Index + xRadius < x) {
					cache.PopTail();
				}
				result.PushBack(List<DArr<4>>());
				List<DArr<4>> &curList = result.Last();
				for (size_t y = 0; y < data.Height; ++y) {
					curList.PushBack(DArr<4>());
					DArr<4> &curPx = curList.Last();
					cache.ForEachTailToHead([&](const ColumnCache &cc) {
						size_t repeat = 1;
						if (cc.Index == 0) {
							repeat = xRadius + 1 - x;
						} else if (cc.Index == data.Width - 1) {
							repeat = x + xRadius + 2 - data.Width;
						}
						for (size_t i = 0; i < repeat; ++i) {
							curPx += cc.Content[static_cast<size_t>(y)];
						}
						return true;
					});
				}
			}
			for (size_t x = 0; x < data.Width; ++x) {
				for (size_t y = 0; y < data.Height; ++y) {
					GetColor(data, x, y) = DArrToColor(result[x][y] / ((2 * xRadius + 1) * (2 * yRadius + 1)));
				}
			}
		}
		void GdiPlusAccess::Pixelate(Gdiplus::BitmapData &data, size_t xLen, size_t yLen) {
			size_t nsx = (data.Width - 1) / xLen, nsy = (data.Height - 1) / yLen, cbxs = 0, nbxs = xLen;
			for (size_t x = 0; x < nsx; ++x, cbxs += xLen, nbxs += xLen) {
				size_t cbys = 0, nbys = yLen;
				for (size_t y = 0; y < nsy; ++y, cbys += yLen, nbys += yLen) {
					DArr<4> curPx;
					size_t samc = 0;
					for (size_t ys = cbys; ys < data.Height && ys < nbys; ++ys) {
						for (size_t xs = cbxs; xs < data.Width && xs < nbxs; ++xs) {
							curPx += ColorToDArr(GetColor(data, xs, ys));
							++samc;
						}
					}
					Core::Color acc = DArrToColor(curPx / samc);
					for (size_t ys = cbys; ys < data.Height && ys < nbys; ++ys) {
						for (size_t xs = cbxs; xs < data.Width && xs < nbxs; ++xs) {
							GetColor(data, xs, ys) = acc;
						}
					}
				}
			}
		}

		void Renderer::SetViewport(const Core::Math::Rectangle &vp) {
			if (_ctx) {
				_ctx->SetViewport(vp);
			}
		}
	}
}
