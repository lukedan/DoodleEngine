#include "../CoreWrap.h"
#include "../GraphicsWrap.h"
#include "../UIWrap.h"
#include "../UtilsWrap.h"

// TODO remove these
using namespace DE;
using namespace DE::Core;
using namespace DE::Core::Input;
using namespace DE::Core::Math;
using namespace DE::Core::Collections;
using namespace DE::Graphics;
using namespace DE::Graphics::TextRendering;
using namespace DE::Graphics::TextRendering::FreeTypeAccess;
using namespace DE::Graphics::RenderingContexts;
using namespace DE::UI;
using namespace DE::IO;
using namespace DE::Utils;

namespace DE {
	namespace Designer {
		namespace Rendering {
			class NodeData;
			enum class LinkDataType {
				Enum,
				String
			};
			struct Link {
				LinkDataType DataType = LinkDataType::String;

				int IntData = 0;
				String StrData;

				NodeData *FromNode = nullptr, *ToNode = nullptr;
				size_t FromID = 0, ToID = 0;
			};
			struct LinkInfo {
				LinkInfo() = default;
				LinkInfo(const String &str) : Name(str) {
				}

				Link *Target = nullptr;
				String Name;
			};
			template <typename T> struct PartialList {
				PartialList() = default;
				PartialList(T *arr, size_t l) : Array(arr), Length(l) {
				}

				T *Array = nullptr;
				size_t Length = 0;
			};
			template <typename T> inline static T *GetAt(const List<PartialList<T>> &lst, size_t id) {
				for (size_t i = 0; i < lst.Count(); ++i) {
					if (lst[i].Length > id) {
						return lst[i].Array + id;
					}
					id -= lst[i].Length;
				}
				return nullptr;
			}
			template <typename T> inline static size_t FindFirst(const List<PartialList<T>> &lst, const std::function<bool(const T&)> &pred) {
				size_t id = 0;
				lst.ForEach([&](const PartialList<T> &plst) {
					for (size_t i = 0; i < plst.Length; ++i) {
						if (pred(plst.Array[i])) {
							return false;
						}
						++id;
					}
					return true;
				});
				return id;
			}
			template <typename T> inline static size_t FindFirst(const List<PartialList<T>> &lst, const T &obj) {
				return GetID(lst, [&](const T &o) {
					return o == obj;
				});
			}
			template <typename T> inline static void ForEach(const List<PartialList<T>> &lst, const std::function<bool(T&)> &op) {
				lst.ForEach([&](const PartialList<T> &plst) {
					for (size_t i = 0; i < plst.Length; ++i) {
						if (!op(plst.Array[i])) {
							return false;
						}
					}
					return true;
				});
			}
			template <typename T> inline static size_t GetCount(const List<PartialList<T>> &lst) {
				size_t count = 0;
				lst.ForEach([&](const PartialList<T> &list) {
					count += list.Length;
					return true;
				});
				return count;
			}

			class NodeData {
				public:
					NodeData(const String &id) : Identifier(id) {
					}

					virtual ~NodeData() {
					}

					virtual List<PartialList<LinkInfo>> GetInputSockets() {
						return List<PartialList<LinkInfo>>();
					}
					virtual List<PartialList<LinkInfo>> GetOutputSockets() {
						return List<PartialList<LinkInfo>>();
					}
					virtual void Refresh() {
					}

					virtual Control *GetEditor() = 0;
					virtual void OnMoved(const Vector2&) = 0;
					virtual void OnSetFont(const Font*) = 0;
					virtual void AddToCollection(ControlCollection&) = 0;
					virtual void RemoveFromCollection(ControlCollection&) = 0;

					const String Identifier;
			};

			template <typename T> class EnumNodeData : public NodeData {
				public:
					EnumNodeData(const String &id) : NodeData(id) {
						_mapping = T::GetMapping();
						_mapping.ForEachPair([&](const typename decltype(_mapping)::Pair &p) {
							typename SimpleComboBox<BasicText>::Item *item = &_selector.InsertItem();
							item->Content().Font = _selector.Content().Font;
							item->Content().Content = p.Key();
							item->Content().TextColor = Color(0, 0, 0, 255);
							item->Content().Padding = Math::Rectangle(-2.0, -2.0, 4.0, 4.0);
							item->FitContent();
							if (item->Content().Content == T::GetDefault()) {
								_selector.SetSelectedItem(item);
							}
							return true;
						});
						_selector.SetAnchor(Anchor::TopLeft);
					}

					List<PartialList<LinkInfo>> GetOutputSockets() override {
						List<PartialList<LinkInfo>> lst = NodeData::GetOutputSockets();
						lst.PushBack(PartialList<LinkInfo>(&_outSoc, 1));
						return lst;
					}

					Control *GetEditor() override {
						return &_selector;
					}
					void OnMoved(const Vector2 &newtl) override {
						_selector.SetMargins(Thickness(newtl.X, newtl.Y, 0.0, 0.0));
						_selector.FitLargestItem();
					}
					void OnSetFont(const Font *fnt) override {
						_selector.Content().Font = fnt;
						_selector.Items().ForEach([&](typename SimpleComboBox<BasicText>::Item *item) {
							item->Content().Font = fnt;
							item->FitContent();
							return true;
						});
						_selector.FitLargestItem();
					}
					void AddToCollection(ControlCollection &col) override {
						col.Insert(_selector);
						_selector.SetDropDownPanelFather(&col);
						_selector.SetDropPanelZIndex(1);
					}
					void RemoveFromCollection(ControlCollection &col) override {
						_selector.SetDropDownPanelFather(nullptr);
						col.Delete(_selector);
					}
				protected:
					LinkInfo _outSoc {_TEXT("Value")};
					SimpleComboBox<BasicText> _selector;
					Dictionary<String, int> _mapping;
			};
			class RenderModeEnumData {
				public:
					static Dictionary<String, int> GetMapping();
					static String GetDefault();
			};
			class AlphaFactorEnumData {
				public:
					static Dictionary<String, int> GetMapping();
					static String GetDefault();
			};
			class StencilFunctionEnumData {
				public:
					static Dictionary<String, int> GetMapping();
					static String GetDefault();
			};
			class StencilOperationEnumData {
				public:
					static Dictionary<String, int> GetMapping();
					static String GetDefault();
			};
			class StringNodeData : public NodeData { // TODO make it foldable
				public:
					StringNodeData(const String &id) : NodeData(id) {
						_text.Text().Padding = Math::Rectangle(-2.0, -2.0, 4.0, 4.0);
						_text.Text().TextColor = Color(0, 0, 0, 255);
						_text.SetText(_TEXT("String"));
						_text.WrapText = true;
						_text.SetAnchor(Anchor::TopLeft);
						_text.TextChanged += [&](const Info&) {
							ResetTextLayout();
						};
					}

					Control *GetEditor() override {
						return &_text;
					}
					void OnMoved(const Vector2 &newtl) override {
						_text.SetMargins(Thickness(newtl.X, newtl.Y, 0.0, 0.0));
						ResetTextLayout();
					}
					void OnSetFont(const Font *fnt) override {
						_text.Text().Font = fnt;
						ResetTextLayout();
					}
					void AddToCollection(ControlCollection &col) override {
						col.Insert(_text);
					}
					void RemoveFromCollection(ControlCollection &col) override {
						col.Delete(_text);
					}

					List<PartialList<LinkInfo>> GetOutputSockets() override {
						List<PartialList<LinkInfo>> lst = NodeData::GetOutputSockets();
						lst.PushBack(PartialList<LinkInfo>(&_outSoc, 1));
						return lst;
					}
				protected:
					LinkInfo _outSoc {_TEXT("Value")};
					TextBox<BasicText> _text;

					virtual void ResetTextLayout() {
						_text.Text().LayoutRectangle = Math::Rectangle(0.0, 0.0, 300.0, 0.0);
						Vector2 sz = _text.Text().GetSize();
						if (sz.X < 50.0) {
							sz.X = 50.0;
						}
						_text.SetSize(sz);
					}
			};
			class NoEditorNodeData : public NodeData {
				public:
					NoEditorNodeData(const String &id) : NodeData(id) {
						_text.Content().Padding = Math::Rectangle(-2.0, -2.0, 4.0, 4.0);
						_text.Content().WrapType = LineWrapType::WrapWordsNoOverflow;
						_text.SetAnchor(Anchor::TopLeft);
						_text.SetVisibility(Visibility::Ghost);
					}

					virtual String GetTitle() const = 0;

					Control *GetEditor() override {
						return &_text;
					}
					void OnMoved(const Vector2 &newtl) override {
						_text.SetMargins(Thickness(newtl.X, newtl.Y, 0.0, 0.0));
						ResetTextLayout();
					}
					void OnSetFont(const Font *fnt) override {
						_text.Content().Font = fnt;
						ResetTextLayout();
					}
					void AddToCollection(ControlCollection &col) override {
						col.Insert(_text);
					}
					void RemoveFromCollection(ControlCollection &col) override {
						col.Delete(_text);
					}
				protected:
					Label<BasicText> _text;

					virtual void ResetTextLayout() {
						_text.Content().Content = GetTitle();
						_text.SetSize(Size(150.0, 0.0));
						_text.FitContent();
					}
			};
			class RendererSettingsNodeData : public NoEditorNodeData {
				public:
					RendererSettingsNodeData(const String &id) : NoEditorNodeData(id) {
					}

					String GetTitle() const override {
						return _TEXT("Settings");
					}
					List<PartialList<LinkInfo>> GetInputSockets() override {
						List<PartialList<LinkInfo>> lst = NoEditorNodeData::GetInputSockets();
						lst.PushBack(PartialList<LinkInfo>(_inSoc, sizeof(_inSoc) / sizeof(LinkInfo)));
						return lst;
					}
					List<PartialList<LinkInfo>> GetOutputSockets() override {
						List<PartialList<LinkInfo>> lst = NoEditorNodeData::GetOutputSockets();
						lst.PushBack(PartialList<LinkInfo>(_outSoc, sizeof(_outSoc) / sizeof(LinkInfo)));
						return lst;
					}
				protected:
					LinkInfo _inSoc[4] {
						{_TEXT("AlphaBlend")},
						{_TEXT("Stencil")},
						{_TEXT("Shader")},
						{_TEXT("Continuity")}
					};
					LinkInfo _outSoc[2] {
						{_TEXT("Output")},
						{_TEXT("Continuity")}
					};
			};
			class AlphaBlendSettingsNodeData : public NoEditorNodeData {
				public:
					AlphaBlendSettingsNodeData(const String &id) : NoEditorNodeData(id) {
					}

					String GetTitle() const override {
						return _TEXT("Blend");
					}
					List<PartialList<LinkInfo>> GetInputSockets() override {
						List<PartialList<LinkInfo>> lst = NoEditorNodeData::GetInputSockets();
						lst.PushBack(PartialList<LinkInfo>(_inSoc, sizeof(_inSoc) / sizeof(LinkInfo)));
						return lst;
					}
					List<PartialList<LinkInfo>> GetOutputSockets() override {
						List<PartialList<LinkInfo>> lst = NoEditorNodeData::GetOutputSockets();
						lst.PushBack(PartialList<LinkInfo>(_outSoc, sizeof(_outSoc) / sizeof(LinkInfo)));
						return lst;
					}
				protected:
					LinkInfo _inSoc[2] {
						{_TEXT("SourceFactor")},
						{_TEXT("DestinationFactor")}
					};
					LinkInfo _outSoc[1] {
						{_TEXT("Output")}
					};
			};
			class StencilSettingsNodeData : public NoEditorNodeData {
				public:
					StencilSettingsNodeData(const String &id) : NoEditorNodeData(id) {
					}

					String GetTitle() const override {
						return _TEXT("Stencil");
					}
					List<PartialList<LinkInfo>> GetInputSockets() override {
						List<PartialList<LinkInfo>> lst = NoEditorNodeData::GetInputSockets();
						lst.PushBack(PartialList<LinkInfo>(_inSoc, sizeof(_inSoc) / sizeof(LinkInfo)));
						return lst;
					}
					List<PartialList<LinkInfo>> GetOutputSockets() override {
						List<PartialList<LinkInfo>> lst = NoEditorNodeData::GetOutputSockets();
						lst.PushBack(PartialList<LinkInfo>(_outSoc, sizeof(_outSoc) / sizeof(LinkInfo)));
						return lst;
					}
				protected:
					LinkInfo _inSoc[7] {
						{_TEXT("Cleaning")},
						{_TEXT("ComparisonFunc")},
						{_TEXT("Criterion")},
						{_TEXT("ComparisonMask")},
						{_TEXT("FailOp")},
						{_TEXT("ZFailOp")},
						{_TEXT("ZPassOp")}
					};
					LinkInfo _outSoc[1] {
						{_TEXT("Output")}
					};
			};
			class DrawCallNodeData : public NoEditorNodeData {
				public:
					DrawCallNodeData(const String &id) : NoEditorNodeData(id) {
					}

					String GetTitle() const override {
						return _TEXT("DrawCall");
					}
					List<PartialList<LinkInfo>> GetInputSockets() override {
						List<PartialList<LinkInfo>> lst = NoEditorNodeData::GetInputSockets();
						lst.PushBack(PartialList<LinkInfo>(_inSoc, sizeof(_inSoc) / sizeof(LinkInfo)));
						return lst;
					}
					List<PartialList<LinkInfo>> GetOutputSockets() override {
						List<PartialList<LinkInfo>> lst = NoEditorNodeData::GetOutputSockets();
						lst.PushBack(PartialList<LinkInfo>(_outSoc, sizeof(_outSoc) / sizeof(LinkInfo)));
						return lst;
					}
				protected:
					LinkInfo _inSoc[8] {
						{_TEXT("PreviousCall")},
						{_TEXT("Vertices")},
						{_TEXT("VertexListName")},
						{_TEXT("Mode")},
						{_TEXT("TextureName")},
						{_TEXT("Settings")},
						{_TEXT("ManualSetup")},
						{_TEXT("ManualCleanup")}
					};
					LinkInfo _outSoc[1] {
						{_TEXT("Target")}
					};
			};
			class BufferNodeData : public NoEditorNodeData {
				public:
					BufferNodeData(const String &id) : NoEditorNodeData(id) {
					}

					String GetTitle() const override {
						return _TEXT("Buffer");
					}
					List<PartialList<LinkInfo>> GetInputSockets() override {
						List<PartialList<LinkInfo>> lst = NoEditorNodeData::GetInputSockets();
						lst.PushBack(PartialList<LinkInfo>(_inSoc, sizeof(_inSoc) / sizeof(LinkInfo)));
						return lst;
					}
					List<PartialList<LinkInfo>> GetOutputSockets() override {
						List<PartialList<LinkInfo>> lst = NoEditorNodeData::GetOutputSockets();
						lst.PushBack(PartialList<LinkInfo>(_outSoc, sizeof(_outSoc) / sizeof(LinkInfo)));
						return lst;
					}
				protected:
					LinkInfo _inSoc[2] {
						{_TEXT("DrawCalls")},
						{_TEXT("Name")}
					};
					LinkInfo _outSoc[1] {
						{_TEXT("TextureOutput")}
					};
			};

			class NodePanel : public PanelBase {
				public:
					constexpr static double TipGap = 5.0;
					const static SolidBrush
						DefaultNodeBackground,
						DefaultSocketBrush,
						DefaultActivatedSocketBrush,
						DefaultTipBackground;
					const static Pen DefaultLinkPen;

					struct NodeSizeInfo {
						Vector2 EditorSize, OverallSize;
					};
					enum class HitObject {
						Nothing,
						Background,
						InSocket,
						OutSocket
					};
					struct HitTestInfo {
						HitTestInfo() = default;
						HitTestInfo(HitObject obj) : HitResult(obj) {
						}
						HitTestInfo(HitObject obj, size_t id) : HitResult(obj), HitID(id) {
						}

						HitObject HitResult = HitObject::Nothing;
						size_t HitID = 0;
					};
					struct NodeInfo {
						constexpr static double SocketHeight = 6.0, SocketWidth = 7.0, SocketGap = 2.0;

						NodeInfo() = default;
						NodeInfo(NodeData *data) :
							Node(data),
							EditorCache(Node->GetEditor()),
							InSocketsCache(Node->GetInputSockets()),
							OutSocketsCache(Node->GetOutputSockets())
						{
							Node->OnMoved(TopLeft);
						}

						NodeData *Node = nullptr;

						Vector2 TopLeft;
						Control *EditorCache = nullptr;
						List<PartialList<LinkInfo>> InSocketsCache, OutSocketsCache;

						NodeSizeInfo GetSize() const {
							NodeSizeInfo info;
							size_t sn = Max(GetCount<LinkInfo>(InSocketsCache), GetCount<LinkInfo>(OutSocketsCache));
							info.EditorSize = EditorCache->GetActualSize();
							info.OverallSize = Vector2(Max(info.EditorSize.X, 2.0 * SocketWidth + SocketGap), info.EditorSize.Y + sn * (SocketHeight + SocketGap) + SocketGap);
							return info;
						}
						Vector2 GetEditorSize() const {
							return EditorCache->GetActualSize();
						}
						HitTestInfo HitTest(const Vector2 &pos) const {
							NodeSizeInfo nsi = GetSize();
							Vector2 diff = pos - TopLeft;
							if (diff.X < 0.0 || diff.X > nsi.OverallSize.X || diff.Y < 0.0 || diff.Y > nsi.OverallSize.Y) {
								return HitTestInfo(HitObject::Nothing);
							}
							HitTestInfo res;
							double socdif = diff.Y - nsi.EditorSize.Y - SocketGap;
							if (socdif < 0.0 || (diff.X > SocketWidth && diff.X < nsi.OverallSize.X - SocketWidth)) {
								return HitTestInfo(HitObject::Background);
							}
							bool in = diff.X < nsi.OverallSize.X * 0.5;
							size_t
								y = static_cast<size_t>(floor(socdif / (SocketHeight + SocketGap))),
								cand = GetCount<LinkInfo>(in ? InSocketsCache : OutSocketsCache);
							if (y >= cand || socdif - y * (SocketHeight + SocketGap) >= SocketHeight) {
								return HitTestInfo(HitObject::Background);
							}
							return HitTestInfo(in ? HitObject::InSocket : HitObject::OutSocket, y);
						}
						Vector2 GetInSocketPos(size_t id) const {
							return TopLeft + Vector2(0.0, GetEditorSize().Y + (SocketGap + SocketHeight) * id + SocketHeight * 0.5 + SocketGap);
						}
						Vector2 GetOutSocketPos(size_t id) const {
							NodeSizeInfo nsi = GetSize();
							return TopLeft + Vector2(nsi.OverallSize.X, nsi.EditorSize.Y + (SocketGap + SocketHeight) * id + SocketHeight * 0.5 + SocketGap);
						}
						Link *&GetInSocketLink(size_t id) {
							return GetAt<LinkInfo>(InSocketsCache, id)->Target;
						}
						Link *&GetOutSocketLink(size_t id) {
							return GetAt<LinkInfo>(OutSocketsCache, id)->Target;
						}
					};

					~NodePanel() {
						_links.ForEach([&](Link *lnk) {
							lnk->~Link();
							GlobalAllocator::Free(lnk);
							return true;
						});
					}

					void InsertNode(NodeData *data) {
						data->AddToCollection(_col);
						NodeInfo ni(data);
						ni.Node->OnSetFont(_font);
						_nodes.PushBack(ni);
					}

					void Update(double dt) override {
						Vector2 rpos = _actualLayout.TopLeft() + GetRelativeMousePosition();
						if (_dragging) {
							if (_draggingNode >= _nodes.Count()) {
								_nodes.ForEach([&](NodeInfo &ni) {
									ni.TopLeft += rpos - _dragOffset;
									ni.Node->OnMoved(ni.TopLeft);
									return true;
								});
								_dragOffset = rpos;
								if (!IsKeyDown(VK_RBUTTON)) {
									_dragging = false;
								}
							} else {
								NodeInfo &curd = _nodes[_draggingNode];
								curd.TopLeft = rpos - _dragOffset;
								curd.Node->OnMoved(curd.TopLeft);
								if (!IsKeyDown(VK_LBUTTON)) {
									_dragging = false;
								}
							}
						}
						if (_linkTarget != HitObject::Nothing) {
							NodeData **nd;
							size_t *id;
							if (_linkTarget == HitObject::InSocket) {
								nd = &_tmpLnk.ToNode;
								id = &_tmpLnk.ToID;
							} else {
								nd = &_tmpLnk.FromNode;
								id = &_tmpLnk.FromID;
							}
							*nd = nullptr;
							for (size_t i = 0; i < _nodes.Count(); ++i) {
								HitTestInfo hti = _nodes[i].HitTest(rpos);
								if (hti.HitResult == _linkTarget) {
									bool hasold = (_linkTarget == HitObject::InSocket ? _nodes[i].GetInSocketLink(hti.HitID) : _nodes[i].GetOutSocketLink(hti.HitID));
									if (!hasold) {
										*nd = _nodes[i].Node;
										*id = hti.HitID;
										break;
									}
								}
							}
							if (!IsKeyDown(VK_LBUTTON)) {
								_linkTarget = HitObject::Nothing;
								if (*nd) {
									EstablishLink(_tmpLnk);
								}
							}
						}
						PanelBase::Update(dt);
					}
					void Render(Renderer &r) override {
						int hasTip = 0;
						String tipText;
						Vector2 tipLoc;
						_nodes.ForEach([&](const NodeInfo &ni) {
							NodeSizeInfo info = ni.GetSize();
							Vector2 tl = ni.TopLeft;
							FillRectWithFallback(r, NodeBackground, GetDefaultNodeBackground(), Math::Rectangle(tl, tl + info.OverallSize));
							Math::Rectangle rect(
								ni.TopLeft.X,
								ni.TopLeft.Y + info.EditorSize.Y + NodeInfo::SocketGap,
								NodeInfo::SocketWidth,
								NodeInfo::SocketHeight
							);
							Vector2 mousePos = _actualLayout.TopLeft() + GetRelativeMousePosition();
							ForEach<LinkInfo>(ni.InSocketsCache, [&](const LinkInfo &li) {
								if (mousePos.X > rect.Left && mousePos.X < rect.Right && mousePos.Y > rect.Top && mousePos.Y < rect.Bottom) {
									FillRectWithFallback(r, ActivatedSocketBrush, GetDefaultActivatedSocketBrush(), rect);
									hasTip = 1;
									tipText = li.Name;
									tipLoc = Vector2(rect.Left, rect.CenterY());
								} else {
									FillRectWithFallback(r, SocketBrush, GetDefaultSocketBrush(), rect);
								}
								rect.Translate(Vector2(0.0, NodeInfo::SocketHeight + NodeInfo::SocketGap));
								return true;
							});
							rect = Math::Rectangle(
								ni.TopLeft.X + info.OverallSize.X - NodeInfo::SocketWidth,
								ni.TopLeft.Y + info.EditorSize.Y + NodeInfo::SocketGap,
								NodeInfo::SocketWidth,
								NodeInfo::SocketHeight
							);
							ForEach<LinkInfo>(ni.OutSocketsCache, [&](const LinkInfo &li) {
								if (mousePos.X > rect.Left && mousePos.X < rect.Right && mousePos.Y > rect.Top && mousePos.Y < rect.Bottom) {
									FillRectWithFallback(r, ActivatedSocketBrush, GetDefaultActivatedSocketBrush(), rect);
									hasTip = 2;
									tipText = li.Name;
									tipLoc = Vector2(rect.Right, rect.CenterY());
								} else {
									FillRectWithFallback(r, SocketBrush, GetDefaultSocketBrush(), rect);
								}
								rect.Translate(Vector2(0.0, NodeInfo::SocketHeight + NodeInfo::SocketGap));
								return true;
							});
							return true;
						});
						_links.ForEach([&](Link *lnk) {
							DrawLink(r, *lnk);
							return true;
						});
						if (_linkTarget != HitObject::Nothing) {
							DrawLink(r, _tmpLnk);
						}
						PanelBase::Render(r);
						if (hasTip) {
							_tip.Content = tipText;
							Vector2 sz = _tip.GetSize();
							Math::Rectangle layout = (
								hasTip == 1 ?
								Math::Rectangle(tipLoc.X - sz.X - TipGap, tipLoc.Y - 0.5 * sz.Y, sz.X, sz.Y) :
								Math::Rectangle(tipLoc.X + TipGap, tipLoc.Y - 0.5 * sz.Y, sz.X, sz.Y)
							);
							_tip.HorizontalAlignment = (hasTip == 1 ? HorizontalTextAlignment::Right : HorizontalTextAlignment::Left);
							_tip.LayoutRectangle = layout;
							FillRectWithFallback(r, TipBackground, GetDefaultTipBackground(), layout);
							_tip.Render(r);
						}
					}
					void DrawLink(Renderer &r, const Link &lnk) {
						Vector2 f = GetRelativeMousePosition() + _actualLayout.TopLeft(), t = f;
						if (lnk.FromNode) {
							f = _nodes[FindWithNodeData(lnk.FromNode)].GetOutSocketPos(lnk.FromID);
						}
						if (lnk.ToNode) {
							t = _nodes[FindWithNodeData(lnk.ToNode)].GetInSocketPos(lnk.ToID);
						}
						Vector2 c1 = f, c2 = t;
						c1.X = c2.X = 0.5 * (c1.X + c2.X);
						double piv = Min(Abs(c1.Y - c2.Y) * 0.4, 50.0);
						if (c1.X - f.X < piv) {
							c1.X = f.X + piv;
							c2.X = t.X - piv;
						}
						Bezier bz(f, t, c1, c2);
						DrawLineStripWithFallback(r, LinkPen, GetDefaultLinkPen(), bz.GetLine(20));
					}

					void EstablishLink(const Link &lnk) {
						Link *nlnk = new (GlobalAllocator::Allocate(sizeof(Link))) Link(lnk);
						if (nlnk->FromNode) {
							_nodes[FindWithNodeData(nlnk->FromNode)].GetOutSocketLink(nlnk->FromID) = nlnk;
						}
						if (nlnk->ToNode) {
							_nodes[FindWithNodeData(nlnk->ToNode)].GetInSocketLink(nlnk->ToID) = nlnk;
						}
						_links.PushBack(nlnk);
					}
					void Unlink(Link *old) {
						_links.SwapRemove(_links.FindFirst(old));
						_nodes[FindWithNodeData(old->FromNode)].GetOutSocketLink(old->FromID) = nullptr;
						_nodes[FindWithNodeData(old->ToNode)].GetInSocketLink(old->ToID) = nullptr;
						old->~Link();
						GlobalAllocator::Free(old);
					}

					virtual const Brush *GetDefaultNodeBackground() const {
						return &DefaultNodeBackground;
					}
					virtual const Brush *GetDefaultSocketBrush() const {
						return &DefaultSocketBrush;
					}
					virtual const Brush *GetDefaultActivatedSocketBrush() const {
						return &DefaultActivatedSocketBrush;
					}
					virtual const Brush *GetDefaultTipBackground() const {
						return &DefaultTipBackground;
					}
					virtual const Pen *GetDefaultLinkPen() const {
						return &DefaultLinkPen;
					}

					const List<NodeInfo> GetAllNodes() const {
						return _nodes;
					}

					GetSetProperty<const Font*> ContentFont {
						[this](const Font *fnt) {
							_font = fnt;
							_nodes.ForEach([&](NodeInfo &ni) {
								ni.Node->OnSetFont(_font);
								return true;
							});
							_tip.Font = _font;
						}, [this]() {
							return _font;
						}
					};
					ReferenceProperty<const Brush*>
						NodeBackground {nullptr},
						SocketBrush {nullptr},
						ActivatedSocketBrush {nullptr},
						TipBackground {nullptr};
					ReferenceProperty<const Pen*> LinkPen {nullptr};
				protected:
					bool OnMouseDown(const MouseButtonInfo &info) override {
						if (PanelBase::HitTest(info.Position)) {
							return PanelBase::OnMouseDown(info);
						}
						if (info.Button == MouseButton::Left) {
							for (size_t i = 0; i < _nodes.Count(); ++i) {
								HitTestInfo res = _nodes[i].HitTest(info.Position);
								if (res.HitResult == HitObject::Background) {
									_dragging = true;
									_draggingNode = i;
									_dragOffset = info.Position - _nodes[i].TopLeft;
								} else if (res.HitResult == HitObject::Nothing) {
									continue;
								} else {
									NodeData **nd;
									size_t *id;
									Link *old = nullptr;
									if (res.HitResult == HitObject::InSocket) {
										old = _nodes[i].GetInSocketLink(res.HitID);
										nd = &_tmpLnk.ToNode;
										id = &_tmpLnk.ToID;
										_linkTarget = HitObject::OutSocket;
									} else {
										old = _nodes[i].GetOutSocketLink(res.HitID);
										nd = &_tmpLnk.FromNode;
										id = &_tmpLnk.FromID;
										_linkTarget = HitObject::InSocket;
									}
									if (old) {
										_tmpLnk = *old;
										Unlink(old);
										_linkTarget = (_linkTarget == HitObject::InSocket ? HitObject::OutSocket : HitObject::InSocket);
									} else {
										_tmpLnk = Link();
										*nd = _nodes[i].Node;
										*id = res.HitID;
									}
								}
								break;
							}
						} else if (info.Button == MouseButton::Right) {
							_dragging = true;
							_dragOffset = info.Position;
							_draggingNode = _nodes.Count();
						}
						return Control::OnMouseDown(info);
					}
					bool HitTest(const Vector2 &pt) const override {
						return Control::HitTest(pt);
					}

					size_t FindWithNodeData(NodeData *nd) const {
						for (size_t i = 0; i < _nodes.Count(); ++i) {
							if (_nodes[i].Node == nd) {
								return i;
							}
						}
						return _nodes.Count();
					}

					const Font *_font = nullptr;
					List<NodeInfo> _nodes;
					List<Link*> _links;
					BasicText _tip;

					bool _dragging = false;
					size_t _draggingNode = 0;
					Vector2 _dragOffset;

					HitObject _linkTarget = HitObject::Nothing;
					Link _tmpLnk;
			};

			Dictionary<String, std::function<NodeData*()>> GetNodeInfoRegisterationTable();
		}
	}
}
