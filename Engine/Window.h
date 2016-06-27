#pragma once

#include <iostream>

#include <windows.h>
#include <windowsx.h>
#include <ctime>
#include <tchar.h>
#include <imm.h>

#include "InputElement.h"
#include "Vector2.h"
#include "Stopwatch.h"
#include "String.h"
#include "Dictionary.h"
#include "Property.h"

namespace DE {
	namespace Core {
		enum class SizeChangeType {
			Maximized = SIZE_MAXIMIZED,
			Minimized = SIZE_MINIMIZED,
			Restore = SIZE_RESTORED,
			MaxHide = SIZE_MAXHIDE,
			MaxShow = SIZE_MAXSHOW
		};
		struct SizeChangeInfo {
			public:
				SizeChangeInfo() = default;
				SizeChangeInfo(WPARAM wParam, LPARAM lParam) :
					NewSize(Math::Vector2(LOWORD(lParam), HIWORD(lParam))),
					Type((SizeChangeType)wParam)
				{
				}
				explicit SizeChangeInfo(Math::Vector2 sz) :
					NewSize(sz), Type(SizeChangeType::Restore) {
				}

				ReferenceProperty<Math::Vector2, PropertyType::ReadOnly> NewSize;
				ReferenceProperty<SizeChangeType, PropertyType::ReadOnly> Type;
		};
		struct MessageInfo {
			public:
				MessageInfo() = default;
				MessageInfo(UINT msg, WPARAM wParam, LPARAM lParam) :
					ID(msg), WParam(wParam), LParam(lParam)
				{
				}

				ReferenceProperty<UINT, PropertyType::ReadOnly> ID;
				ReferenceProperty<WPARAM, PropertyType::ReadOnly> WParam;
				ReferenceProperty<LPARAM, PropertyType::ReadOnly> LParam;
		};
		class Window : public Input::InputElement {
				friend LRESULT CALLBACK DEWindowProc(HWND, UINT, WPARAM, LPARAM);
			public:
				explicit Window(const String&);
				Window(const Window&) = delete;
				Window &operator =(const Window&) = delete;
				virtual ~Window();

				bool Idle();

				void Show() {
					ShowWindow(_hWnd, SW_SHOW);
				}
				void Hide() {
					ShowWindow(_hWnd, SW_HIDE);
				}

				void PutToCenter();

				size_t &CursorOverrideCount() {
					return _cursorOverrideCount;
				}
				const size_t &CursorOverrideCount() const {
					return _cursorOverrideCount;
				}

				GetSetProperty<String> Title {
					[this](const String &newTitle) {
						SetWindowText(_hWnd, *newTitle);
					},
					[this]() {
						int l = GetWindowTextLength(_hWnd) + 1;
						LPTSTR title = static_cast<LPTSTR>(GlobalAllocator::Allocate(sizeof(TCHAR) * l));
						GetWindowText(_hWnd, title, l);
						String result = title;
						GlobalAllocator::Free(title);
						return title;
					}
				};
				GetSetProperty<Math::Vector2> Size {
					[this](const Math::Vector2 &size) {
						RECT r;
						GetWindowRect(_hWnd, &r);
						MoveWindow(_hWnd, r.left, r.top, size.X, size.Y, false);
					},
					[this]() {
						RECT r;
						GetWindowRect(_hWnd, &r);
						return Math::Vector2(r.right - r.left, r.bottom - r.top);
					}
				};
				GetSetProperty<Math::Vector2> ClientSize {
					[this](const Math::Vector2 &size) {
						RECT r, c;
						GetWindowRect(_hWnd, &r);
						GetClientRect(_hWnd, &c);
						MoveWindow(
							_hWnd,
							r.left,
							r.top,
							size.X - c.right + r.right - r.left,
							size.Y - c.bottom + r.bottom - r.top,
							false
						);
					},
					[this]() {
						RECT c;
						GetClientRect(_hWnd, &c);
						return Math::Vector2(c.right, c.bottom);
					}
				};
				GetSetProperty<HWND, PropertyType::ReadOnly> Handle {
					[this]() {
						return _hWnd;
					}
				};
				GetSetProperty<bool> IMEEnabled;
				GetSetProperty<bool> CursorLimited;
				GetSetProperty<DE::Core::String, PropertyType::ReadOnly> LastErrorMessage {
					[this]() {
						return _exMsg;
					}
				};

				Event<SizeChangeInfo> SizeChanged;
				Event<Info> CloseButtonClicked, OnSetCursor;
				Event<MessageInfo> OnMessageReceived;
			protected:
				virtual void OnMouseMove(const Input::MouseMoveInfo &info) {
					InputElement::OnMouseMove(info);
					if (!_ting) {
						if (!TrackMouseEvent(&_tme)) {
							throw SystemException(_TEXT("cannot track the mouse"));
						}
						_ting = true;
					}
				}
			private:
				HINSTANCE _hInstance;
				HWND _hWnd;
				ATOM _wndAtom;
				WNDCLASSEX _wcex;
				MSG _msg;
				HIMC _hImc;
				bool _imeEnable = false;
				TRACKMOUSEEVENT _tme;
				bool _ting = true, _limCursor = false;
				bool _hasEx = false;
				String _exMsg;

				void RelimitCursor();

				size_t _cursorOverrideCount = 0;
		};
	}
}
