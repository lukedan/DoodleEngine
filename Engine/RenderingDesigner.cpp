#include "RenderingDesigner.h"

namespace DE {
	namespace Designer {
		namespace Rendering {
			const SolidBrush
				NodePanel::DefaultNodeBackground(Color(100, 100, 100, 255)),
				NodePanel::DefaultSocketBrush(Color(150, 150, 150, 255)),
				NodePanel::DefaultActivatedSocketBrush(Color(200, 200, 200, 255)),
				NodePanel::DefaultTipBackground(Color(50, 50, 50, 255));
			const Pen
				NodePanel::DefaultLinkPen(Color(180, 180, 180, 255), 2.0);

			Dictionary<String, int> RenderModeEnumData::GetMapping() {
				static Dictionary<String, int> _table;
				bool _initialized = false;

				if (!_initialized) {
					_table[_TEXT("Triangles")] = static_cast<int>(RenderMode::Triangles);
					_table[_TEXT("TriangleStrip")] = static_cast<int>(RenderMode::TriangleStrip);
					_table[_TEXT("TriangleFan")] = static_cast<int>(RenderMode::TriangleFan);
					_table[_TEXT("Lines")] = static_cast<int>(RenderMode::Lines);
					_table[_TEXT("LineStrip")] = static_cast<int>(RenderMode::LineStrip);
					_table[_TEXT("Points")] = static_cast<int>(RenderMode::Points);
					_initialized = true;
				}

				return _table;
			}
			String RenderModeEnumData::GetDefault() {
				return _TEXT("Triangles");
			}

			Dictionary<String, int> AlphaFactorEnumData::GetMapping() {
				static Dictionary<String, int> _table;
				bool _initialized = false;

				if (!_initialized) {
					_table[_TEXT("Zero")] = static_cast<int>(BlendFactor::Zero);
					_table[_TEXT("One")] = static_cast<int>(BlendFactor::One);
					_table[_TEXT("SourceAlpha")] = static_cast<int>(BlendFactor::SourceAlpha);
					_table[_TEXT("TargetAlpha")] = static_cast<int>(BlendFactor::TargetAlpha);
					_table[_TEXT("InvertedSourceAlpha")] = static_cast<int>(BlendFactor::InvertedSourceAlpha);
					_table[_TEXT("InvertedTargetAlpha")] = static_cast<int>(BlendFactor::InvertedTargetAlpha);
					_table[_TEXT("SourceColor")] = static_cast<int>(BlendFactor::SourceColor);
					_table[_TEXT("TargetColor")] = static_cast<int>(BlendFactor::TargetColor);
					_table[_TEXT("InvertedSourceColor")] = static_cast<int>(BlendFactor::InvertedSourceColor);
					_table[_TEXT("InvertedTargetColor")] = static_cast<int>(BlendFactor::InvertedTargetColor);
					_initialized = true;
				}

				return _table;
			}
			String AlphaFactorEnumData::GetDefault() {
				return _TEXT("Zero");
			}

			Dictionary<String, int> StencilFunctionEnumData::GetMapping() {
				static Dictionary<String, int> _table;
				bool _initialized = false;

				if (!_initialized) {
					_table[_TEXT("Never")] = static_cast<int>(StencilComparisonFunction::Never);
					_table[_TEXT("Always")] = static_cast<int>(StencilComparisonFunction::Always);
					_table[_TEXT("Equal")] = static_cast<int>(StencilComparisonFunction::Equal);
					_table[_TEXT("NotEqual")] = static_cast<int>(StencilComparisonFunction::NotEqual);
					_table[_TEXT("Less")] = static_cast<int>(StencilComparisonFunction::Less);
					_table[_TEXT("LessOrEqual")] = static_cast<int>(StencilComparisonFunction::LessOrEqual);
					_table[_TEXT("Greater")] = static_cast<int>(StencilComparisonFunction::Greater);
					_table[_TEXT("GreaterOrEqual")] = static_cast<int>(StencilComparisonFunction::GreaterOrEqual);
					_initialized = true;
				}

				return _table;
			}
			String StencilFunctionEnumData::GetDefault() {
				return _TEXT("Always");
			}

			Dictionary<String, int> StencilOperationEnumData::GetMapping() {
				static Dictionary<String, int> _table;
				bool _initialized = false;

				if (!_initialized) {
					_table[_TEXT("Keep")] = static_cast<int>(StencilOperation::Keep);
					_table[_TEXT("Zero")] = static_cast<int>(StencilOperation::Zero);
					_table[_TEXT("Replace")] = static_cast<int>(StencilOperation::Replace);
					_table[_TEXT("ClampedIncrease")] = static_cast<int>(StencilOperation::ClampedIncrease);
					_table[_TEXT("ClampedDecrease")] = static_cast<int>(StencilOperation::ClampedDecrease);
					_table[_TEXT("WrappedIncrease")] = static_cast<int>(StencilOperation::WrappedIncrease);
					_table[_TEXT("WrappedDecrease")] = static_cast<int>(StencilOperation::WrappedDecrease);
					_table[_TEXT("BitwiseInvert")] = static_cast<int>(StencilOperation::BitwiseInvert);
					_initialized = true;
				}

				return _table;
			}
			String StencilOperationEnumData::GetDefault() {
				return _TEXT("Keep");
			}

			Dictionary<String, std::function<NodeData*()>> GetNodeInfoRegisterationTable() {
				static Dictionary<String, std::function<NodeData*()>> _table;
				static bool _initialized = false;

				if (!_initialized) {
#define DE_REGISTER_NODE_DATA_TYPE(NAME)                                                                \
	_table[_TEXT(#NAME)] = []() {                                                                       \
		return new (GlobalAllocator::Allocate(sizeof(NAME##NodeData))) NAME##NodeData(_TEXT(#NAME));    \
	}                                                                                                   \

#define DE_REGISTER_NODE_DATA_TYPE_T(NAME)                                                                                          \
	_table[_TEXT(#NAME)] = []() {                                                                                                   \
		return new (GlobalAllocator::Allocate(sizeof(EnumNodeData<NAME##EnumData>))) EnumNodeData<NAME##EnumData>(_TEXT(#NAME));    \
	}                                                                                                                               \

					DE_REGISTER_NODE_DATA_TYPE_T(RenderMode);
					DE_REGISTER_NODE_DATA_TYPE_T(AlphaFactor);
					DE_REGISTER_NODE_DATA_TYPE_T(StencilFunction);
					DE_REGISTER_NODE_DATA_TYPE_T(StencilOperation);
					DE_REGISTER_NODE_DATA_TYPE(String);
					DE_REGISTER_NODE_DATA_TYPE(RendererSettings);
					DE_REGISTER_NODE_DATA_TYPE(AlphaBlendSettings);
					DE_REGISTER_NODE_DATA_TYPE(StencilSettings);
					DE_REGISTER_NODE_DATA_TYPE(DrawCall);
					DE_REGISTER_NODE_DATA_TYPE(Buffer);
#undef DE_REGISTER_NODE_DATA_TYPE
#undef DE_REGISTER_NODE_DATA_TYPE_T

					_initialized = true;
				}

				return _table;
			}
		}
	}
}
