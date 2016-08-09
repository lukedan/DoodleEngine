#pragma once

#include "Common.h"
#include "Window.h"
#include "String.h"

namespace DE {
	namespace IO {
		class Clipboard {
			public:
				template <typename Char = TCHAR> static void CopyStringToClipboard(const Core::Window&, const Core::StringBase<Char>&);

				template <typename Char = TCHAR> inline static Core::StringBase<Char> RetrieveString() {
					Core::AsciiString tmp;
					if (!TryRetrieveStringFromClipboard<Char>(tmp)) {
						throw Core::InvalidOperationException(_TEXT("retrieval failed"));
					}
					return tmp;
				}
				template <typename Char = TCHAR> static bool TryRetrieveStringFromClipboard(const Core::Window&, Core::StringBase<Char>&);

				static bool TryCopyDataToClipboard(HWND, const void*, size_t, UINT);
			protected:
				template <typename Char, UINT Flag> inline static bool TryRetrieveStringFromClipboardImpl(HWND hwnd, Core::StringBase<Char> &str) {
					if (!OpenClipboard(hwnd)) {
						return false;
					}
					bool res = false;
					HGLOBAL handle = GetClipboardData(Flag);
					if (handle) {
						LPVOID ptr = GlobalLock(handle);
						if (ptr) {
							str = Core::StringBase<Char>(static_cast<Char*>(ptr));
							res = true;
							GlobalUnlock(ptr);
						}
					}
					CloseClipboard();
					return res;
				}
		};
		template <> inline void Clipboard::CopyStringToClipboard<char>(const Core::Window &wnd, const Core::StringBase<char> &str) {
			if (!TryCopyDataToClipboard(wnd.Handle, *str, sizeof(char) * (str.Length() + 1), CF_TEXT)) {
				throw Core::SystemException(_TEXT("clipboard write failed"));
			}
		}
		template <> inline void Clipboard::CopyStringToClipboard<wchar_t>(const Core::Window &wnd, const Core::StringBase<wchar_t> &str) {
			if (!TryCopyDataToClipboard(wnd.Handle, *str, sizeof(wchar_t) * (str.Length() + 1), CF_UNICODETEXT)) {
				throw Core::SystemException(_TEXT("clipboard write failed"));
			}
		}
		template <> inline bool Clipboard::TryRetrieveStringFromClipboard<char>(const Core::Window &wnd, Core::StringBase<char> &str) {
			return TryRetrieveStringFromClipboardImpl<char, CF_TEXT>(wnd.Handle, str);
		}
		template <> inline bool Clipboard::TryRetrieveStringFromClipboard<wchar_t>(const Core::Window &wnd, Core::StringBase<wchar_t> &str) {
			return TryRetrieveStringFromClipboardImpl<wchar_t, CF_UNICODETEXT>(wnd.Handle, str);
		}
	}
}
