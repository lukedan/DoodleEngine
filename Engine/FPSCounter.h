#pragma once

#include "Queue.h"

namespace DE {
	namespace Core {
		class FPSCounter {
			public:
				void Update(double dt) {
					tot++;
					_tTot += dt;
					_q.PushTail(_tTot);
					while (_q.PeekHead() + 1.0 < _tTot) {
						_q.PopHead();
					}
					if (_q.Count() > max) {
						max = _q.Count();
					}
				}
				unsigned GetFPS() const {
					return _q.Count();
				}
				double GetAverageFPS() const {
					return tot / _tTot;
				}
				unsigned GetMaxFPS() const {
					return max;
				}
			private:
				Collections::Queue<double> _q;
				double _tTot = 0.0;
				unsigned tot = 0, max = 0;
		};
	}
}