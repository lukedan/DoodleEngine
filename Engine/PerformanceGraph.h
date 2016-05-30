#pragma once

#include "ContentControl.h"

namespace DE {
	namespace UI {
		class PerformanceGraph : public Control {
			public:
				constexpr static double DefaultTimeLimit = 5.0, MinHeightPercentage = 0.25, MaxHeightPercentage = 0.75, HeightRatio = 0.8;

				const Graphics::Pen *const &FrameTimePen() const {
					return _ftPen;
				}
				const Graphics::Pen *&FrameTimePen() {
					return _ftPen;
				}

				const Graphics::Pen *const &MemoryUsagePen() const {
					return _muPen;
				}
				const Graphics::Pen *&MemoryUsagePen() {
					return _muPen;
				}

				const double &TimeLimit() const {
					return _tLim;
				}
				double &TimeLimit() {
					return _tLim;
				}
			protected:
				constexpr static size_t FrameTimeID = 0, MemoryID = 1, MonitoredItemCount = 2, TimeID = MonitoredItemCount;
				struct FrameRecord {
					FrameRecord() = default;
					FrameRecord(double time, double frameTime, size_t mem) : Values { frameTime, static_cast<double>(mem), time } {
					}

					double Values[MonitoredItemCount + 1];
				};

				Core::Collections::Queue<FrameRecord> _rec;
				const Graphics::Pen *_ftPen = nullptr, *_muPen = nullptr;
				double _tLim = DefaultTimeLimit, _timeTot = 0.0;

				virtual void Update(double dt) override {
					_timeTot += dt;
					_rec.PushTail(FrameRecord(_timeTot, dt, Core::GlobalAllocator::UsedSize()));
					FrameRecord tmpRec;
					bool popped = false;
					while (_rec.PeekHead().Values[TimeID] + _tLim < _timeTot) {
						popped = true;
						tmpRec = _rec.PopHead();
					}
					if (popped) {
						_rec.PushHead(tmpRec);
					}
				}

				void RenderTrack(
					Graphics::Renderer &r,
					size_t id,
					const Graphics::Pen *pen,
					double maxV
				) {
					if (pen == nullptr) {
						return;
					}
					Core::Collections::List<Core::Math::Vector2> vxs;
					double
						top = GetActualLayout().Top,
						vph = GetActualLayout().Height(),
						left = GetActualLayout().Left,
						vpw = GetActualLayout().Width();
					double topV = static_cast<double>(maxV) * 1.0 / HeightRatio;
					_rec.ForEachTailToHead([&](const FrameRecord &obj) {
						vxs.PushBack(Core::Math::Vector2(
							left + vpw * (1.0 - (_timeTot - obj.Values[TimeID]) / _tLim),
							top + vph * (1.0 - static_cast<double>(obj.Values[id]) / topV)
						));
						return true;
					});
					pen->DrawLineStrip(vxs, r);
				}
				void GetMaxValues(double *arr) {
					for (size_t it = 0; it < MonitoredItemCount; ++it) {
						arr[it] = _rec.PeekHead().Values[it];
					}
					_rec.ForEachHeadToTail([&](const FrameRecord &obj) {
						for (size_t it = 0; it < MonitoredItemCount; ++it) {
							if (obj.Values[it] > arr[it]) {
								arr[it] = obj.Values[it];
							}
						}
						return true;
					});
				}
				virtual void Render(Graphics::Renderer &r) override {
					double maxV[MonitoredItemCount];
					GetMaxValues(maxV);
					RenderTrack(r, FrameTimeID, _ftPen, maxV[FrameTimeID]);
					RenderTrack(r, MemoryID, _muPen, maxV[MemoryID]);
				}
		};
	}
}