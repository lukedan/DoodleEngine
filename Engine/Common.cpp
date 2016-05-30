#include "Common.h"

namespace DE {
	namespace Core {
#ifdef DEBUG
		int GetDumpID() {
			static int id = -1;
			return ++id;
		}
#endif
	}
}
