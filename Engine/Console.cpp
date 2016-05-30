#include "Console.h"

namespace DE {
	namespace UI {
		using namespace Core;

		const Graphics::SolidBrush
			SimpleConsoleTextBox::DefaultCaretBrush(Color(255, 255, 255, 255)),
			SimpleConsoleTextBox::DefaultBackground(Color(0, 0, 0, 255));

		void ConsoleRunnerBase::Write(const Core::String &str) {
			if (_father) {
				_father->_output.Write(str);
			}
		}
		void ConsoleRunnerBase::WriteLine(const Core::String &str) {
			if (_father) {
				_father->_output.WriteLine(str);
			}
		}
		void ConsoleRunnerBase::WriteLineWithColor(const Core::String &str, const Core::Color &c) {
			if (_father) {
				Color last = _father->_output.GetLineColor();
				_father->_output.SetLineColor(c);
				_father->_output.WriteLine(str);
				_father->_output.SetLineColor(last);
			}
		}
		void ConsoleRunnerBase::SetLineColor(const Core::Color &c) {
			if (_father) {
				_father->_output.SetLineColor(c);
			}
		}
		Core::Color ConsoleRunnerBase::GetLineColor() const {
			return (_father != nullptr ? _father->_output.GetLineColor() : Color());
		}
		void ConsoleRunnerBase::ClearConsole() {
			if (_father) {
				_father->_output.ClearConsole();
			}
		}

		void ConsoleRunnerBase::SetAbsoluteCursorPosition(size_t x, size_t y) {
			if (_father) {
				_father->_output.SetAbsoluteCursorPosition(x, y);
			}
		}
		void ConsoleRunnerBase::SetRelativeCursorPosition(size_t x, size_t y) {
			if (_father) {
				_father->_output.SetRelativeCursorPosition(x, y);
			}
		}

		bool ConsoleRunnerBase::IsInputEnabled() const {
			if (_father) {
				return !(_father->_input.ReadOnly());
			}
			return false;
		}
		void ConsoleRunnerBase::EnableInput() {
			if (_father) {
				_father->_input.ReadOnly() = false;
			}
		}
		void ConsoleRunnerBase::DisableInput() {
			if (_father) {
				_father->_input.SetText(_TEXT(""));
				_father->_input.ReadOnly() = true;
			}
		}
	}
}