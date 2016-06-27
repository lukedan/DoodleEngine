#pragma once

#include "Rectangle.h"
#include "Dictionary.h"
#include "String.h"
#include "FileAccess.h"
#include "Renderer.h"

namespace DE {
	namespace Graphics {
		struct AtlasTexture {
			AtlasTexture() = default;

			Core::Math::Rectangle UVRect;
			size_t Page = 0;
			void *Tag = nullptr;
		};
		class DynamicAtlas;
		class Atlas {
				friend class DynamicAtlas;
			public:
				Atlas() :
					_data(
						nullptr,
						[](AtlasData *ptr) {
							if (ptr == nullptr) {
								return;
							}
							if (ptr->_onDispose) {
								ptr->_ats.ForEachPair([&](const decltype(ptr->_ats)::Pair &pair) {
									ptr->_onDispose(pair.Key(), pair.Value().Tag);
									return true;
								});
							}
							if (!ptr || !(ptr->_ctx)) {
								return;
							}
							const Core::Collections::List<Texture> &ltex = ptr->_texs;
							Renderer r = ptr->_ctx->CreateRenderer();
							ltex.ForEach([&r](const Texture &tex) {
								r.UnloadTexture(tex);
								return true;
							});
							ptr->~AtlasData();
							Core::GlobalAllocator::Free(ptr);
						}
					)
				{
				}
				Atlas(
					RenderingContexts::RenderingContext *cc,
					const Core::Collections::List<Texture> &txs,
					const Core::Collections::Dictionary<int, AtlasTexture> &ats
				) : Atlas() {
					_data.SetSharedPointer(new (Core::GlobalAllocator::Allocate(sizeof(AtlasData))) AtlasData());
					_data->_texs = txs;
					_data->_ats = ats;
					_data->_ctx = cc;
				}
				~Atlas() {
				}

				const AtlasTexture &operator [](int key) const {
					if (!_data) {
						throw Core::InvalidOperationException(_TEXT("the Atlas is empty"));
					}
					return _data->_ats[key];
				}
				size_t Count() const {
					return _data->_ats.PairCount();
				}

				void Save(
					const Core::String &imgName,
					IO::FileAccess &writer,
					const std::function<void(int, IO::FileAccess&, const void*)> &writeTag = nullptr
				) const {
					if (_data && _data->_ctx) {
						const AtlasData &data = *_data;
						writer.WriteBinaryObject<size_t>(data._texs.Count());
						for (size_t i = 0; i < data._texs.Count(); ++i) {
							Core::String name = imgName + _TEXT("_") + Core::ToString(i) + _TEXT(".png");
							GdiPlusAccess::SaveBitmap(
								*(data._ctx->GetTextureImage(data._texs[i].GetID())),
								*name,
								Gdiplus::ImageFormatPNG
							);
							writer.WriteBinaryString(name);
						}
						writer.WriteBinaryObject<size_t>(data._ats.PairCount());
						for (
							const decltype(data._ats)::Node *cur = data._ats.GetFirstPair();
							cur;
							cur = cur->Next()
						) {
							writer.WriteBinaryObject<int>(cur->Value().Key());
							writer.WriteBinaryObject<Core::Math::Rectangle>(cur->Value().Value().UVRect);
							writer.WriteBinaryObject<size_t>(cur->Value().Value().Page);
							if (writeTag) {
								writeTag(cur->Value().Key(), writer, cur->Value().Value().Tag);
							}
						}
					}
				}
				static Atlas Load(
					Renderer &r,
					IO::FileAccess &reader,
					const std::function<void*(int, IO::FileAccess&)> &readTag = nullptr
				) {
					Atlas atl(
						r.GetContext(),
						Core::Collections::List<Texture>(),
						Core::Collections::Dictionary<int, AtlasTexture>()
					);
					size_t tCount = reader.ReadBinaryObject<size_t>();
					for (size_t i = 0; i < tCount; ++i) {
						Core::String file = reader.ReadBinaryString<TCHAR>();
						atl._data->_texs.PushBack(r.LoadTextureFromFile(file));
					}
					size_t pairC = reader.ReadBinaryObject<size_t>();
					for (size_t i = 0; i < pairC; ++i) {
						int curK = reader.ReadBinaryObject<int>();
						AtlasTexture curT;
						curT.UVRect = reader.ReadBinaryObject<Core::Math::Rectangle>();
						curT.Page = reader.ReadBinaryObject<size_t>();
						if (readTag) {
							curT.Tag = readTag(curK, reader);
						}
						atl._data->_ats.SetValue(curK, curT);
					}
					atl._data->_ctx = r.GetContext();
					return atl;
				}

				bool Valid() const {
					return _data != nullptr;
				}

				const Core::Collections::List<Texture> &Textures() const {
					if (!_data) {
						throw Core::InvalidOperationException(_TEXT("the Atlas is empty"));
					}
					return _data->_texs;
				}
				Core::Collections::List<Texture> &Textures() {
					if (!_data) {
						throw Core::InvalidOperationException(_TEXT("the Atlas is empty"));
					}
					return _data->_texs;
				}

				const Core::Collections::Dictionary<int, AtlasTexture> &AtlasTextures() const {
					if (!_data) {
						throw Core::InvalidOperationException(_TEXT("the Atlas is empty"));
					}
					return _data->_ats;
				}
				Core::Collections::Dictionary<int, AtlasTexture> &AtlasTextures() {
					if (!_data) {
						throw Core::InvalidOperationException(_TEXT("the Atlas is empty"));
					}
					return _data->_ats;
				}

				RenderingContexts::RenderingContext *const &Context() const {
					if (!_data) {
						throw Core::InvalidOperationException(_TEXT("the Atlas is empty"));
					}
					return _data->_ctx;
				}
				RenderingContexts::RenderingContext *&Context() {
					if (!_data) {
						throw Core::InvalidOperationException(_TEXT("the Atlas is empty"));
					}
					return _data->_ctx;
				}

				const std::function<void(int, void*)> &FreeFunc() const {
					if (!_data) {
						throw Core::InvalidOperationException(_TEXT("the Atlas is empty"));
					}
					return _data->_onDispose;
				}
				std::function<void(int, void*)> &FreeFunc() {
					if (!_data) {
						throw Core::InvalidOperationException(_TEXT("the Atlas is empty"));
					}
					return _data->_onDispose;
				}
			private:
				struct AtlasData {
					Core::Collections::List<Texture> _texs;
					Core::Collections::Dictionary<int, AtlasTexture> _ats;
					RenderingContexts::RenderingContext *_ctx = nullptr;
					std::function<void(int, void*)> _onDispose = nullptr;
				};

				Core::SharedPointer<AtlasData> _data;
		};

		class AtlasGenerator {
			public:
				struct TextureInfo {
					int Key = 0;
					Gdiplus::Bitmap *Image = nullptr;
					void *Tag = nullptr;
				};

				const Core::Collections::List<TextureInfo> &Textures() const {
					return _texs;
				}
				Core::Collections::List<TextureInfo> &Textures() {
					return _texs;
				}

				const Core::Math::Rectangle &BorderWidth() const {
					return _border;
				}
				Core::Math::Rectangle &BorderWidth() {
					return _border;
				}
				void SetUniformBorderWidth(double w) {
					_border.Left = _border.Top = -w;
					_border.Bottom = _border.Right = w;
				}

				const double &WidthLimit() const {
					return _xlimit;
				}
				double &WidthLimit() {
					return _xlimit;
				}

				const double &HeightLimit() const {
					return _ylimit;
				}
				double &HeightLimit() {
					return _ylimit;
				}

				Atlas Generate(Renderer&);

				Core::ReferenceProperty<std::function<bool(Gdiplus::Bitmap*)>> FX;
			protected:
				double _xlimit = 300.0, _ylimit = -1.0;
				Core::Math::Rectangle _border;
				Core::Collections::List<TextureInfo> _texs;
		};
		class DynamicAtlas {
			public:
				DynamicAtlas() = default;
				explicit DynamicAtlas(RenderingContexts::RenderingContext *ctx) : TargetAtlas(Atlas(
					ctx,
					Core::Collections::List<Texture>(),
					Core::Collections::Dictionary<int, AtlasTexture>()
				)) {
				}
				~DynamicAtlas() {
					if (_curBmp) {
						delete _g;
						delete _curBmp;
					}
				}

				void Flush() {
					if (_needFlush) {
						TargetAtlas->Context()->SetTextureImage(_tex.GetID(), *_curBmp);
						_needFlush = false;
					}
				}
				void Append(int id, Gdiplus::Bitmap *bmp, void *tag, bool instantly) {
					AtlasTexture tex;
					if (!_curBmp) {
						CreateNewTexturePage();
					}
					UINT w = bmp->GetWidth(), h = bmp->GetHeight();
					unsigned clb = ceil(-BorderWidth->Left), diffl = clb + w + ceil(BorderWidth->Right);
					if (_penPos.X + diffl > AtlasTextureWidth) {
						_penPos.X = 0.0;
						_penPos.Y += ceil(_mh);
						_mh = 0.0;
					}
					unsigned ctb = ceil(-BorderWidth->Top), diffh = ctb + h + ceil(BorderWidth->Bottom);
					if (_penPos.Y + diffh > AtlasTextureHeight) { // the texture is (almost) fully occupied
						CreateNewTexturePage();
						_penPos = Core::Math::Vector2();
						_mh = 0.0;
					}
					if (diffh > _mh) {
						_mh = diffh;
					}
					AssertGDIPlusSuccess(_g->DrawImage(
						bmp,
						static_cast<int>(_penPos.X + clb),
						static_cast<int>(_penPos.Y + ctb),
						w,
						h
					), _TEXT("cannot draw the image"));
					if (instantly) {
						Flush();
					} else {
						_needFlush = true;
					}
					Core::Math::Vector2 tl = _penPos, br;
					tl.X += clb;
					tl.Y += ctb;
					br = tl;
					br.X += w;
					br.Y += h;
					tl.X /= AtlasTextureWidth;
					tl.Y /= AtlasTextureHeight;
					br.X /= AtlasTextureWidth;
					br.Y /= AtlasTextureHeight;
					tex.UVRect = Core::Math::Rectangle(tl, br);
					tex.Tag = tag;
					tex.Page = TargetAtlas->Textures().Count() - 1;
					TargetAtlas->AtlasTextures()[id] = tex;
					_penPos.X += diffl;
				}

				Core::ReferenceProperty<Core::Math::Rectangle> BorderWidth;
				Core::ReferenceProperty<size_t> AtlasTextureWidth = 300.0, AtlasTextureHeight = 300.0;
				Core::ReferenceProperty<Atlas> TargetAtlas;
			protected:
				Gdiplus::Bitmap *_curBmp = nullptr;
				Gdiplus::Graphics *_g = nullptr;
				Core::Math::Vector2 _penPos;
				double _mh = 0.0;
				Graphics::Texture _tex;
				bool _needFlush = false;

				void CreateNewTexturePage() {
					if (_curBmp) {
						Flush();
						delete _g;
						delete _curBmp;
					}
					_curBmp = new Gdiplus::Bitmap(AtlasTextureWidth, AtlasTextureHeight);
					_g = Gdiplus::Graphics::FromImage(_curBmp);
					if (TargetAtlas->Context()) {
						Renderer r = TargetAtlas->Context()->CreateRenderer();
						_tex = r.LoadTextureFromBitmap(*_curBmp);
						TargetAtlas->Textures().PushBack(_tex);
					}
				}
		};
	}
}
