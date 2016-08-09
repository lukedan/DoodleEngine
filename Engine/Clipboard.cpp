#include "Clipboard.h"

namespace DE {
	namespace IO {
		bool Clipboard::TryCopyDataToClipboard(HWND handle, const void *data, size_t sz, UINT type) {
			if (!OpenClipboard(handle)) {
				return false;
			}
			bool res = false;
			if (EmptyClipboard()) {
				HGLOBAL handle = GlobalAlloc(GMEM_MOVEABLE, sz);
				if (handle) {
					LPVOID ptr = GlobalLock(handle);
					if (ptr) {
						std::memcpy(ptr, data, sz);
						GlobalUnlock(handle); // NOTE not checked
						if (SetClipboardData(type, handle)) {
							res = true;
						}
					}
				}
			}
			CloseClipboard();
			return res;
		}
	}
}
