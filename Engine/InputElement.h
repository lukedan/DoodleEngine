#pragma once

#include <winuser.h>

#include "Vector2.h"
#include "Math.h"
#include "Event.h"
#include "Property.h"

namespace DE {
	namespace Core {
		struct Info {
		};
		namespace Input {
			enum class MouseButton {
				Left = VK_LBUTTON,
				Right = VK_RBUTTON,
				Middle = VK_MBUTTON
			};
			inline int GetButtonBit(MouseButton btn) {
				switch (btn) {
					case MouseButton::Left:
						return MK_LBUTTON;
					case MouseButton::Right:
						return MK_RBUTTON;
					case MouseButton::Middle:
						return MK_MBUTTON;
					default:
						return 0;
				}
			}

			enum class SystemKey {
				Ctrl = MK_CONTROL,
				Shift = MK_SHIFT,
				LeftMouse = MK_LBUTTON,
				MiddleMouse = MK_MBUTTON,
				RightMouse = MK_RBUTTON,
				Alt = MK_ALT,
				None = 0
			};

			struct MouseMoveInfo {
				public:
					MouseMoveInfo() = default;
					MouseMoveInfo(const Math::Vector2 &pos, SystemKey keys) :
						Position(pos), Keys(keys)
					{
					}
					MouseMoveInfo(WPARAM wParam, LPARAM lParam) :
						Position(Math::Vector2(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam))),
						Keys((SystemKey)wParam)
					{
					}

					ReferenceProperty<Math::Vector2, PropertyType::ReadOnly> Position;
					ReferenceProperty<SystemKey, PropertyType::ReadOnly> Keys;

					bool ContainsKey(SystemKey key) const {
						return Keys.As<int>() & (int)key;
					}
			};
			struct MouseScrollInfo {
				public:
					MouseScrollInfo() = default;
					MouseScrollInfo(const Math::Vector2 &pos, SystemKey keys, int dt) :
						Position(pos), Keys(keys), Delta(dt)
					{
					}
					MouseScrollInfo(WPARAM wParam, LPARAM lParam, const Math::Vector2 &tl) :
						Position(Math::Vector2(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)) - tl),
						Keys((SystemKey)LOWORD(wParam)),
						Delta(GET_WHEEL_DELTA_WPARAM(wParam) / static_cast<double>(WHEEL_DELTA))
					{
					}

					ReferenceProperty<Math::Vector2, PropertyType::ReadOnly> Position;
					ReferenceProperty<SystemKey, PropertyType::ReadOnly> Keys;
					ReferenceProperty<double, PropertyType::ReadOnly> Delta;
			};
			struct BufferPivotHitInfo {
				public:
					BufferPivotHitInfo() = default;
					explicit BufferPivotHitInfo(int dt) : Delta(dt) {
					}

					ReferenceProperty<int, PropertyType::ReadOnly> Delta;
			};
			struct MouseScrollBuffer {
				public:
					Event<BufferPivotHitInfo> OnPivotHit;

					void OnScroll(double dt) {
						_curV += dt;
						if (Math::Abs(_curV) > Pivot) {
							int x = static_cast<int>(_curV / Pivot);
							_curV -= x * Pivot;
							OnPivotHit(BufferPivotHitInfo(x));
						}
					}

					ReferenceProperty<double> Pivot = 1.0;
				protected:
					double _curV = 0.0;
			};
			struct MouseButtonInfo {
				public:
					MouseButtonInfo() = default;
					MouseButtonInfo(const Math::Vector2 &pos, MouseButton button, SystemKey keys) :
						Position(pos), Button(button), Keys(keys)
					{
					}
					MouseButtonInfo(WPARAM wParam, LPARAM lParam) :
						Position(Math::Vector2(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam))),
						Button((MouseButton)0),
						Keys((SystemKey)wParam)
					{
					}
					MouseButtonInfo(WPARAM wParam, LPARAM lParam, MouseButton b, bool altdown) :
						Position(Math::Vector2(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam))),
						Button(b),
						Keys((SystemKey)(altdown ? wParam | (WPARAM)SystemKey::Alt : wParam))
					{
					}

					ReferenceProperty<Math::Vector2, PropertyType::ReadOnly> Position;
					ReferenceProperty<MouseButton, PropertyType::ReadOnly> Button;
					ReferenceProperty<SystemKey, PropertyType::ReadOnly> Keys;

					bool ContainsKey(SystemKey key) const {
						return Keys.As<int>() & (int)key;
					}
			};
			struct KeyInfo {
				public:
					KeyInfo() = default;
					KeyInfo(int key, int repeat, int scanCode, bool pDown, bool ext) :
						_key(key), _repeat(repeat), _scanCode(scanCode), _pDown(pDown), _ext(ext)
					{
					}
					KeyInfo(WPARAM wParam, LPARAM lParam) : _key(wParam) {
						_repeat = lParam & 0xFFFF;
						_scanCode = (lParam & 0xFF0000)>>16;
						_ext = lParam & 0x1000000;
						_pDown = lParam & 0x40000000;
					}

					int GetKey() const {
						return _key;
					}
					int GetRepeatCount() const {
						return _repeat;
					}
					int ScanCode() const {
						return _scanCode;
					}
					bool GetPreviousState() const {
						return _pDown;
					}
					bool IsExtendedKey() const {
						return _ext;
					}
				private:
					int _key = 0, _repeat = 0, _scanCode = 0;
					bool _pDown = false, _ext = false;
			};
			struct TextInfo { // TODO process lParam too
				public:
					TextInfo() = default;
					TextInfo(WPARAM wParam, LPARAM) : _c(wParam) {
					}

					wchar_t GetChar() const {
						return _c;
					}
				private:
					wchar_t _c = 0;
			};

			enum class DefaultCursorType : size_t {
				Arrow = (size_t)IDC_ARROW,
				IBeam = (size_t)IDC_IBEAM,
				Wait = (size_t)IDC_WAIT,
				Cross = (size_t)IDC_CROSS,
				UpArrow = (size_t)IDC_UPARROW,
				SizeSlash = (size_t)IDC_SIZENESW,
				SizeBackSlash = (size_t)IDC_SIZENWSE,
				SizeHorizontal = (size_t)IDC_SIZEWE,
				SizeVertical = (size_t)IDC_SIZENS,
				SizeAll = (size_t)IDC_SIZEALL,
				No = (size_t)IDC_NO,
				Hand = (size_t)IDC_HAND,
				AppStarting = (size_t)IDC_APPSTARTING,
				Help = (size_t)IDC_HELP,
				None = (size_t)nullptr
			};
			struct Cursor {
				public:
					Cursor() = default;
					explicit Cursor(DefaultCursorType type) : Type(type) {
					}

					void Set() const {
						SetCursor(LoadCursor(Source, (LPCTSTR)Type));
					}
					HINSTANCE Source = nullptr;
					DefaultCursorType Type = DefaultCursorType::Arrow;
			};

			class InputElement {
				public:
					InputElement() :
						MouseMove(), MouseDown(), MouseUp(), MouseHover(), KeyDown(), KeyUp(), MouseEnter(),
						MouseLeave(), GotFocus(), LostFocus(), MouseScroll(), KeyboardText(), _relPos(-1, -1), topLeft()
					{
					}
					virtual ~InputElement() {
					}

					const Math::Vector2 &GetRelativeMousePosition() const {
						return _relPos;
					}
					bool IsMouseOver() const {
						return _over;
					}
					SystemKey GetButtons() const {
						return (SystemKey)_keys;
					}

					bool IsButtonDown(SystemKey key) const {
						return _over && (_keys & (int)key);
					}

					virtual void SetCursor(const Cursor &cursor) {
						_cursor = cursor;
						OnCursorChanged(Info());
					}
					virtual Cursor GetCursor() const {
						return _cursor;
					}

					Event<MouseMoveInfo> MouseMove;
					Event<MouseButtonInfo> MouseDown, MouseUp, MouseHover;
					Event<KeyInfo> KeyDown, KeyUp;
					Event<Info> MouseEnter, MouseLeave, GotFocus, LostFocus, CursorChanged;
					Event<MouseScrollInfo> MouseScroll;
					Event<TextInfo> KeyboardText;
				protected:
					Math::Vector2 _relPos, topLeft;
					bool _over = false;
					int _keys = 0;
					Cursor _cursor;

					virtual void OnMouseMove(const MouseMoveInfo &info) {
						if (!_over) {
                            this->OnMouseEnter(Info());
						}
						_relPos = *info.Position - topLeft;
						MouseMove(info);
					}
					virtual bool OnMouseDown(const MouseButtonInfo &info) {
						_keys |= GetButtonBit(info.Button);
						MouseDown(info);
						return false;
					}
					virtual void OnMouseUp(const MouseButtonInfo &info) {
						int bit = GetButtonBit(info.Button);
						if (_keys & bit) {
							_keys &= (~bit);
							MouseUp(info);
						}
					}
					virtual void OnMouseEnter(const Info &info) {
						_over = true;
						MouseEnter(info);
					}
					virtual void OnMouseLeave(const Info &info) {
						_over = false;
						MouseLeave(info);
					}
					virtual void OnMouseHover(const MouseButtonInfo &info) {
						MouseHover(info);
					}
					virtual void OnKeyDown(const KeyInfo &info) {
						KeyDown(info);
					}
					virtual void OnKeyUp(const KeyInfo &info) {
						KeyUp(info);
					}
					virtual bool OnMouseScroll(const MouseScrollInfo &info) {
						MouseScroll(info);
						return false;
					}
					virtual void OnText(const TextInfo &info) {
						KeyboardText(info);
					}
					virtual void OnGotFocus(const Info &info) {
						GotFocus(info);
					}
					virtual void OnLostFocus(const Info &info) {
						LostFocus(info);
					}
					virtual void OnCursorChanged(const Info &info) {
						CursorChanged(info);
					}
			};
		}
	}
}
