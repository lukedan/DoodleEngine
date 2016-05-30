#pragma once

#include <windows.h>
#include <windowsx.h>
#include <functional>

namespace DE {
	namespace Core {
		class Stopwatch {
			public:
				Stopwatch();

				void Start();
				void Restart();
				void Stop();
				double ElapsedSeconds();
				long long ElapsedTicks();
				double TickInSeconds();
				long long TickInTicks();

				static long long GetFrequency();
				static bool IsHighResolution();

				static long long Time(const std::function<void()>&);
				static double TimeInSeconds(const std::function<void()>&);
				static long long GetTime();
			private:
				class Initializer {
					public:
						Initializer();

						long long _pFreq;
						bool _highRes;
				};

				bool _started;
				LARGE_INTEGER _t;
				long long _first = 0, _last = 0;

				static Initializer _initObj;
		};
	}
}
