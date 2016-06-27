#include "Console.h"

namespace DE {
	namespace UI {
		using namespace Core;
		using namespace Core::Collections;

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

		void SimpleConsoleRunner::OnCommand(const String &str) {
			if (_curCommand != nullptr) {
				if (_curCommand->GetState() == CommandState::WaitingInput) {
					_curCommand->OnInput(str);
				}
			} else {
				if (EchoCommand) {
					if (_father->OutputTextBox().GetCursorX() > 0) {
						WriteLine(_TEXT(""));
					}
					WriteLine(_TEXT("> ") + str);
				}
				Core::Collections::List<String> parsed = CommandLineParser::SplitToWords(str);
				if (parsed.Count() == 0) {
					return;
				}
				Core::String target = parsed[0];
				for (size_t i = 0; i < target.Length(); ++i) {
					if (target[i] >= _TEXT('A') && target[i] <= _TEXT('Z')) {
						target[i] = target[i] - _TEXT('A') + _TEXT('a');
					}
				}
				const Command *exec = nullptr;
				_commands.ForEach([&](const Command &cmd) {
					if (cmd.GetName() == target) {
						exec = &cmd;
						return false;
					}
					return true;
				});
				if (exec == nullptr) {
					WriteLineWithColor(_TEXT("No such command: ") + parsed[0], Core::Color(255, 0, 0, 255));
					return;
				}
				_curCommand = exec->GetExecutable()(parsed);
			}
		}
	}
}
