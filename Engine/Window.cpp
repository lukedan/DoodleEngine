#include "Window.h"
#include "WinUser.h"
#include "Common.h"

namespace DE {
	namespace Core {
		using namespace Math;
		using namespace Input;
		using namespace Collections;

		LRESULT CALLBACK DEWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
			Window *form = reinterpret_cast<Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
			if (form == nullptr) {
				return DefWindowProc(hWnd, msg, wParam, lParam);
			}
			form->OnMessageReceived(MessageInfo(msg, wParam, lParam));
			switch (msg) {
				case WM_CLOSE: {
					form->CloseButtonClicked(Info());
					return 0;
				}
				case WM_SIZE: {
					form->RelimitCursor();
					form->SizeChanged(SizeChangeInfo(wParam, lParam));
					return 0;
				}
				case WM_SETCURSOR: {
					if (form->_cursorOverrideCount == 0) {
						form->GetCursor().Set();
					}
					form->OnSetCursor(Info());
					return true;
				}
				case WM_MOVING: {
					form->RelimitCursor();
					return 0;
				}
				case WM_MOVE: {
					form->RelimitCursor(); // TODO need hooks?
					return 0;
				}

				case WM_KEYDOWN: {
					form->OnKeyDown(KeyInfo(wParam, lParam));
					return 0;
				}
				case WM_KEYUP: {
					form->OnKeyUp(KeyInfo(wParam, lParam));
					return 0;
				}
				case WM_CHAR: {
					form->OnText(TextInfo(wParam, lParam));
					return 0;
				}
				case WM_GETDLGCODE: { //TODO: don't know if it's right
					return DLGC_WANTALLKEYS;
				}
				case WM_IME_SETCONTEXT: { // same as above
					return IME_CAND_READ;
				}
				case WM_IME_CHAR: {
					form->OnText(TextInfo(wParam, lParam));
					return 0;
				}

				case WM_MOUSEWHEEL: {
					POINT p;
					p.x = p.y = 0;
					if (!ClientToScreen(hWnd, &p)) {
						throw Core::SystemException(_TEXT("cannot convert between scopes"));
					}
					form->OnMouseScroll(MouseScrollInfo(wParam, lParam, Math::Vector2(p.x, p.y)));
					return 0;
				}

				case WM_MOUSEMOVE: {
					form->OnMouseMove(MouseMoveInfo(wParam, lParam));
					return 0;
				}
				case WM_MOUSELEAVE: {
					form->_ting = false;
					form->OnMouseLeave(Info());
					return 0;
				}
				case WM_MOUSEHOVER: {
					form->_ting = false;
					form->OnMouseHover(MouseButtonInfo(wParam, lParam));
					return 0;
				}

				case WM_LBUTTONDOWN: {
					form->OnMouseDown(MouseButtonInfo(wParam, lParam, MouseButton::Left, GetKeyState(VK_MENU) < 0));
					return 0;
				}
				case WM_LBUTTONUP: {
					form->OnMouseUp(MouseButtonInfo(wParam, lParam, MouseButton::Left, GetKeyState(VK_MENU) < 0));
					return 0;
				}
				case WM_RBUTTONDOWN: {
					form->OnMouseDown(MouseButtonInfo(wParam, lParam, MouseButton::Right, GetKeyState(VK_MENU) < 0));
					return 0;
				}
				case WM_RBUTTONUP: {
					form->OnMouseUp(MouseButtonInfo(wParam, lParam, MouseButton::Right, GetKeyState(VK_MENU) < 0));
					return 0;
				}
				case WM_MBUTTONDOWN: {
					form->OnMouseDown(MouseButtonInfo(wParam, lParam, MouseButton::Middle, GetKeyState(VK_MENU) < 0));
					return 0;
				}
				case WM_MBUTTONUP: {
					form->OnMouseUp(MouseButtonInfo(wParam, lParam, MouseButton::Middle, GetKeyState(VK_MENU) < 0));
					return 0;
				}

				case WM_SETFOCUS: {
					form->RelimitCursor();
					form->OnGotFocus(Info());
					return 0;
				}
				case WM_KILLFOCUS: {
					form->OnLostFocus(Info());
					ClipCursor(nullptr);
					return 0;
				}

				case WM_ERASEBKGND: {
					return true;
				}
			}
			return DefWindowProc(hWnd, msg, wParam, lParam);
		}

		Window::Window(const String &clsName) : // TODO add error processing for WINAPI functions
			InputElement(),
			IMEEnabled(
				[this](bool b) {
					if (b) {
						if (!_imeEnable) {
							ImmAssociateContext(_hWnd, _hImc);
						}
					} else {
						if (_imeEnable) {
							ImmAssociateContext(_hWnd, 0);
						}
					}
				},
				[this]() {
					return _imeEnable;
				}
			),
			CursorLimited(
				[this](bool b) {
					_limCursor = b;
					RelimitCursor();
				},
				[this]() {
					return _limCursor;
				}
			)
		{
			if (clsName.Empty()) {
				throw InvalidArgumentException(_TEXT("window class name must not be null"));
			}
			_hInstance = GetModuleHandle(nullptr);
			ZeroMemory(&_wcex, sizeof(_wcex));
			_wcex.cbSize = sizeof(_wcex);
			_wcex.style = CS_OWNDC;
			_wcex.lpfnWndProc = DEWindowProc;
			_wcex.cbClsExtra = 0;
			_wcex.cbWndExtra = 0;
			_wcex.hInstance = _hInstance;
			_wcex.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
			_wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
			_wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
			_wcex.lpszMenuName = nullptr;
			_wcex.lpszClassName = *clsName;
			_wndAtom = RegisterClassEx(&_wcex);
			if (_wndAtom == 0) {
				throw SystemException(_TEXT("cannot register the window"));
			}
			_hWnd = CreateWindowEx(
				0, *clsName, *clsName, WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU,
				CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
				nullptr, nullptr, _hInstance, nullptr);
			SetWindowLongPtr(_hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
			_hImc = ImmGetContext(_hWnd);

			_tme.cbSize = sizeof(TRACKMOUSEEVENT);
			_tme.dwFlags = TME_LEAVE | TME_HOVER;
			_tme.hwndTrack = _hWnd;
			_tme.dwHoverTime = HOVER_DEFAULT;
			if (!TrackMouseEvent(&_tme)) {
				throw SystemException(_TEXT("cannot track the mouse"));
			}
		}
		Window::~Window() {
			ImmReleaseContext(_hWnd, _hImc);
			DestroyWindow(_hWnd);
			if (UnregisterClass((LPCTSTR)(size_t)_wndAtom, _hInstance) == 0) {
				throw SystemException(_TEXT("cannot unregister the window"));
			}
		}

		void Window::Idle() {
			if (PeekMessage(&_msg, _hWnd, 0, 0, PM_REMOVE)) {
				if (_msg.message != WM_QUIT) {
					TranslateMessage(&_msg);
					DispatchMessage(&_msg);
				}
			}
		}
		void Window::PutToCenter() {
			RECT r, work;
			GetWindowRect(_hWnd, &r);
			SystemParametersInfo(SPI_GETWORKAREA, 0, &work, 0);
			MoveWindow(
				_hWnd,
				(work.right - work.left - r.right + r.left) / 2.0 + work.left,
				(work.bottom - work.top - r.bottom + r.top) / 2.0 + work.top,
				r.right - r.left,
				r.bottom - r.top,
				false
			);
		}
		void Window::RelimitCursor() {
			if (_limCursor) {
				std::cout<<"relimiting cursor\n";
				std::cout.flush();
				RECT r;
				GetWindowRect(_hWnd, &r);
				if (!ClipCursor(&r)) {
					throw SystemException(_TEXT("cannot limit the cursor"));
				}
			} else {
				ClipCursor(nullptr);
			}
		}
	}
}
