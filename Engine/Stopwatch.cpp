#include "Stopwatch.h"

#include "Common.h"

namespace DE {
	namespace Core {
		Stopwatch::Initializer::Initializer() {
			LARGE_INTEGER i;
			_highRes = QueryPerformanceFrequency(&i);
			_pFreq = i.QuadPart;
		}

		Stopwatch::Initializer Stopwatch::_initObj;

		Stopwatch::Stopwatch() : _started(false) {
		}

		long long Stopwatch::GetFrequency() {
			return _initObj._pFreq;
		}
		bool Stopwatch::IsHighResolution() {
			return _initObj._highRes;
		}
		long long Stopwatch::GetTime() {
			LARGE_INTEGER i;
			QueryPerformanceCounter(&i);
			return i.QuadPart;
		}

		void Stopwatch::Start() {
			if (_started) {
				throw InvalidOperationException(_TEXT("cannot be started twice"));
			}
			_started = true;
			QueryPerformanceCounter(&_t);
			_first = _last = _t.QuadPart;
		}
		void Stopwatch::Restart() {
			if (!_started) {
				throw InvalidOperationException(_TEXT("the stopwatch is not started"));
			}
			QueryPerformanceCounter(&_t);
			_first = _last = _t.QuadPart;
		}
		void Stopwatch::Stop() {
			if (!_started) {
				throw InvalidOperationException(_TEXT("the stopwatch is not started"));
			}
			QueryPerformanceCounter(&_t);
			_last = _t.QuadPart;
			_started = false;
		}

		double Stopwatch::ElapsedSeconds() {
			if (_started) {
				QueryPerformanceCounter(&_t);
				return (_t.QuadPart - _first) / (double)_initObj._pFreq;
			}
			return (_last - _first) / (double)_initObj._pFreq;
		}
		long long Stopwatch::ElapsedTicks() {
			if (_started) {
				QueryPerformanceCounter(&_t);
				return _t.QuadPart - _first;
			}
			return _last - _first;
		}
		double Stopwatch::TickInSeconds() {
			if (!_started) {
				throw InvalidOperationException(_TEXT("the stopwatch is not started"));
			}
			QueryPerformanceCounter(&_t);
			double d = (_t.QuadPart - _last) / (double)_initObj._pFreq;
			_last = _t.QuadPart;
			return d;
		}
		long long Stopwatch::TickInTicks() {
			if (!_started) {
				throw InvalidOperationException(_TEXT("the stopwatch is not started"));
			}
			QueryPerformanceCounter(&_t);
			long long res = _t.QuadPart - _last;
			_last = _t.QuadPart;
			return res;
		}

		long long Stopwatch::Time(const std::function<void()> &func) {
            LARGE_INTEGER start, end;
            QueryPerformanceCounter(&start);
            func();
            QueryPerformanceCounter(&end);
            return end.QuadPart - start.QuadPart;
        }
        double Stopwatch::TimeInSeconds(const std::function<void()> &func) {
            long long l = Time(func);
            return l / (double)_initObj._pFreq;
        }
	}
}
