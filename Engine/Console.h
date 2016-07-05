#pragma once

#include "Panel.h"
#include "TextBox.h"

namespace DE {
	namespace UI {
		class SimpleConsoleTextBox : public PanelBase {
			public:
				constexpr static size_t DefaultBufferHeight = 800, DefaultBufferWidth = 80;
				constexpr static double DefaultCaretPhase = 0.5;
				const static Graphics::SolidBrush DefaultCaretBrush, DefaultBackground;

				SimpleConsoleTextBox() = default;
				SimpleConsoleTextBox(size_t bufSize) : PanelBase() {
					_bufSize = bufSize;
					_endBuf = _bufSize - 1;
				}
				~SimpleConsoleTextBox() {
					_disposing = true;
				}

				virtual const Graphics::TextRendering::Font *GetFont() const {
					return _fnt;
				}
				virtual void SetFont(const Graphics::TextRendering::Font *fnt) {
					_fnt = fnt;
					ResetLinesLayout(true);
				}

				virtual const Graphics::Brush *const &CaretBrush() const {
					return _caretBrush;
				}
				virtual const Graphics::Brush *&CaretBrush() {
					return _caretBrush;
				}

				virtual size_t GetBufferSize() const {
					return _bufSize;
				}
				// TODO SetBufferSize

 				virtual const Graphics::Brush *GetDefaultBackground() const override {
                	return &DefaultBackground;
                }

 				virtual void SetAbsoluteCursorPosition(size_t x, size_t y) {
 					if (y >= _bufSize || x >= DefaultBufferWidth) {
 						throw Core::InvalidArgumentException(_TEXT("invalid cursor position"));
 					}
 					_crsrX = x;
 					_crsrY = y;
 					MakeCursorVisible();
 				}
 				virtual void SetRelativeCursorPosition(size_t x, size_t y) {
 					if (x >= DefaultBufferWidth || y >= _cVisLineNum) {
 						throw Core::InvalidArgumentException(_TEXT("invalid cursor position"));
 					}
 					_crsrX = x;
 					_crsrY = _cFVisLine + y;
 					if (_crsrY >= _bufSize) {
 						_crsrY -= _bufSize;
 					}
 				}
 				virtual size_t GetCursorX() const {
 					return _crsrX;
 				}
 				virtual size_t GetAbsoluteCursorY() const {
 					return _crsrY;
 				}
 				virtual int GetRelativeCursorY() const { // NOTE overflows when there are more than 2 billion lines
 					return static_cast<int>(_crsrY) - static_cast<int>(_cFVisLine);
 				}

 				virtual Core::Color GetCursorColor() const {
 					return _cursorColor;
 				}
 				virtual void SetCursorColor(const Core::Color &c) {
 					_cursorColor = c;
 				}

				virtual void Write(const Core::String &str) {
					ConsoleLine *curL = &(_buf[GetLineID(_crsrY)]);
					MakeCursorVisible();
					for (size_t i = 0; i < str.Length(); ++i) {
						TCHAR c = str[i];
						if (c == _TEXT('\n') || c == _TEXT('\r')) {
							NextLine();
							curL = &(_buf[GetLineID(_crsrY)]);
						} else if (c == _TEXT('\t')) {
							for (size_t i = _tabSize - (_crsrX % _tabSize); i > 0; ) {
								--i;
								curL->Line[_crsrX] = _TEXT(' ');
								curL->CharColors[_crsrX] = _cursorColor;
								if ((++_crsrX) == DefaultBufferWidth) {
									NextLine();
									curL = &(_buf[GetLineID(_crsrY)]);
								}
							}
						} else {
							if (!(_fnt->HasData(c))) {
								continue;
							}
							curL->Line[_crsrX] = c;
							curL->CharColors[_crsrX] = _cursorColor;
							if ((++_crsrX) == DefaultBufferWidth) {
								NextLine();
								curL = &(_buf[GetLineID(_crsrY)]);
							}
						}
					}
					MakeCursorVisible();
				}
				virtual void WriteLine(const Core::String &str) {
					Write(str + _TEXT("\n"));
				}
				virtual void ClearConsole() {
					_crsrX = _crsrY = 0;
					_endBuf = _bufSize - 1;
					_caretTime = 0.0;
					_buf.Clear();
					_buf.PushBack(ConsoleLine(), _bufSize);
					_cFVisLine = 0;
					_sideBar.SetValue(0.0);
				}

				virtual void MakeCursorVisible() {
					if (_crsrY < _cFVisLine) {
						_cFVisLine = _crsrY;
					} else if (_crsrY >= _cFVisLine + _cVisLineNum) {
						_cFVisLine = _crsrY + 1 - _cVisLineNum;
					} else {
						return;
					}
					_sideBar.SetValue(static_cast<double>(_cFVisLine));
				}

				virtual void AutoSetWidth() {
					if (_fnt == nullptr) {
						return;
					}
					if (!Initialized()) {
						Initialize();
					}
					Graphics::TextRendering::BasicText tmp;
					tmp.Font = _fnt;
					tmp.Content = Core::String(_TEXT(' '), DefaultBufferWidth);
					SetSize(Size(tmp.GetSize().X + _sideBar.GetSize().Width, GetSize().Height));
				}
			protected:
				struct ConsoleLine {
					ConsoleLine() : Line(_TEXT(' '), DefaultBufferWidth), CharColors(Core::Color(), DefaultBufferWidth) {
					}

					Core::String Line;
					Core::Collections::List<Core::Color> CharColors;
				};

				size_t
					_bufSize = DefaultBufferHeight,
					_crsrX = 0, _crsrY = 0,
					_endBuf = DefaultBufferHeight - 1,
					_tabSize = 4,
					_cVisLineNum = 0,
					_cFVisLine = 0;
				double _caretTime = 0.0;
				Core::Collections::List<ConsoleLine> _buf;
				ScrollBarBase _sideBar;
				const Graphics::TextRendering::Font *_fnt = nullptr;
				const Graphics::Brush *_caretBrush = nullptr;
				Core::Color _cursorColor;

				size_t GetLineID(size_t line) const {
					size_t res = _endBuf + line + 1;
					if (res >= _bufSize) {
						res -= _bufSize;
					}
					return res;
				}
				virtual void NextLine() {
					if (_crsrY == _bufSize - 1) {
						if ((++_endBuf) >= _bufSize) {
							_endBuf -= _bufSize;
						}
						ConsoleLine &cur = _buf[GetLineID(_crsrY)];
						cur.Line = Core::String(_TEXT(' '), DefaultBufferWidth);
					} else {
						++_crsrY;
					}
					_crsrX = 0;
				}

				virtual void Initialize() override {
					PanelBase::Initialize();

					_buf.PushBack(ConsoleLine(), _bufSize);

					_sideBar.SetAnchor(Anchor::RightDock);
					_sideBar.SetSize(Size(ScrollBarBase::DefaultWidth, 0.0));
					_sideBar.SetMargins(Thickness(0.0));
					_sideBar.Scroll += [&](const ScrollInfo&) {
						ResetLinesLayout(false);
					};
					_col.Insert(_sideBar);
				}

				virtual bool HitTest(const Core::Math::Vector2 &pos) const override {
					return Control::HitTest(pos);
				}

				virtual void ForEachVisibleLine(const std::function<bool(size_t, size_t)> &func) const {
					size_t line = _endBuf + 1 + _cFVisLine, rl = _cFVisLine;
					for (size_t adv = 0; adv < _cVisLineNum; ++adv, ++line, ++rl) {
						if (line >= _bufSize) {
							line -= _bufSize;
						}
						if (!func(line, rl)) {
							break;
						}
					}
				}
				virtual void ResetLinesLayout(bool layoutChanged) {
					if (!Initialized()) {
						return;
					}
					if (_fnt == nullptr) {
						return;
					}
					double v = _sideBar.GetValue();
					if (layoutChanged) {
						_cVisLineNum = static_cast<size_t>(floor(GetActualSize().Height / _fnt->GetHeight()));
						if (v + static_cast<double>(_cVisLineNum) > static_cast<double>(_bufSize)) {
							v = Core::Math::Max(0.0, static_cast<double>(_bufSize) - static_cast<double>(_cVisLineNum));
						}
						_sideBar.SetScrollBarProperties(_bufSize, _cVisLineNum, v);
						_sideBar.PageDelta() = _cVisLineNum;
						_sideBar.StepDistanceRatio() = 1.0 / static_cast<double>(_cVisLineNum);
					}
					_cFVisLine = static_cast<size_t>(round(v));
				}
				virtual void FinishLayoutChange() override {
					PanelBase::FinishLayoutChange();
					ResetLinesLayout(true);
				}
				virtual void Update(double dt) override {
					PanelBase::Update(dt);
					_caretTime += dt;
					while (_caretTime > 2.0 * DefaultCaretPhase) {
						_caretTime -= 2.0 * DefaultCaretPhase;
					}
				}
				virtual void Render(Graphics::Renderer &r) override {
					if (_fnt != nullptr) {
						Graphics::TextRendering::StreamedRichText tmp;
						tmp.Padding = Core::Math::Rectangle();
						tmp.LayoutRectangle = Core::Math::Rectangle(
							GetActualLayout().Left, GetActualLayout().Top, GetActualSize().Width, _fnt->GetHeight()
						);
						ForEachVisibleLine([&](size_t id, size_t lineID) {
							tmp.Changes.Clear();
							const ConsoleLine &cLine = _buf[id];
							tmp.Content = cLine.Line;
							decltype(tmp)::ChangeInfo ci(0, decltype(tmp)::ChangeType::Font);
							ci.Parameters.NewFont = _fnt;
							tmp.Changes.PushBack(ci);
							Core::Color lastColor = cLine.CharColors[0];
							ci.Position = 0;
							ci.Type = decltype(tmp)::ChangeType::Color;
							ci.Parameters.NewColor.A = lastColor.A;
							ci.Parameters.NewColor.R = lastColor.R;
							ci.Parameters.NewColor.G = lastColor.G;
							ci.Parameters.NewColor.B = lastColor.B;
							tmp.Changes.PushBack(ci);
							for (size_t i = 1; i < cLine.CharColors.Count(); ++i) {
								if (cLine.CharColors[i] != lastColor) {
									ci.Position = i;
									lastColor = cLine.CharColors[i];
									ci.Parameters.NewColor.A = lastColor.A;
									ci.Parameters.NewColor.R = lastColor.R;
									ci.Parameters.NewColor.G = lastColor.G;
									ci.Parameters.NewColor.B = lastColor.B;
									tmp.Changes.PushBack(ci);
								}
							}
							tmp.Render(r);
							if (_caretTime < DefaultCaretPhase) {
								if (lineID == _crsrY) {
									Core::Math::Rectangle rect = tmp.GetCaretInfo(_crsrX);
									if (_caretBrush) {
										_caretBrush->FillRect(rect, r);
									} else {
										DefaultCaretBrush.FillRect(rect, r);
									}
								}
							}
							tmp.LayoutRectangle.Top += _fnt->GetHeight();
							tmp.LayoutRectangle.Bottom += _fnt->GetHeight();
							return true;
						});
					}
					PanelBase::Render(r);
				}

				virtual bool OnMouseScroll(const Core::Input::MouseScrollInfo &info) override {
					InputElement::OnMouseScroll(info);
					return _sideBar.HandleMouseScroll(info);
				}
		};
		class Console;
		class ConsoleRunnerBase {
				friend class Console;
			public:
				virtual ~ConsoleRunnerBase() {
				}

				virtual void Write(const Core::String&);
				virtual void WriteLine(const Core::String&);
				virtual void SetCursorColor(const Core::Color&);
				virtual Core::Color GetCursorColor() const;
				virtual void ClearConsole();

				virtual void SetAbsoluteCursorPosition(size_t, size_t);
				virtual void SetRelativeCursorPosition(size_t, size_t);

				virtual bool IsInputEnabled() const;
				virtual void EnableInput();
				virtual void DisableInput();
			protected:
				Console *_father = nullptr;

				virtual void OnCommand(const Core::String&) = 0;
				virtual void Update(double) = 0;
		};

		class CommandLineParser {
			public:
				inline static Core::Collections::List<Core::String> SplitToWords(const Core::String &str) {
					bool inQ = false;
					Core::Collections::List<Core::String> result;
					Core::String curStat = _TEXT("");
					for (size_t i = 0; i < str.Length(); ++i) {
						TCHAR curChar = str[i];
						if ((!inQ) && curChar == _TEXT(' ')) {
							if (curStat.Length() > 0) {
								result.PushBack(curStat);
								curStat = _TEXT("");
							}
						} else if (curChar == _TEXT('\"')) {
							inQ = !inQ;
						} else {
							curStat += curChar;
						}
					}
					if (curStat.Length() > 0) {
						result.PushBack(curStat);
					}
					return result;
				}
		};
		enum class CommandState {
			Running,
			WaitingInput,
			Terminated
		};
		class RunningCommand {
			public:
				virtual ~RunningCommand() {
				}

				virtual CommandState GetState() const = 0;
				virtual void Update(double) = 0;
				virtual int GetReturnValue() const = 0;
				virtual void OnInput(const Core::String&) {
				}
		};
		class CommandComparer;
		class Command {
				friend class CommandComparer;
			public:
				typedef std::function<RunningCommand*(const Core::Collections::List<Core::String>)> Executable;

				Command(const Core::String &name, const Executable &exec) : _name(name), _exec(exec) {
				}

				const Core::String &GetName() const {
					return _name;
				}
				const Executable &GetExecutable() const {
					return _exec;
				}
			protected:
				Core::String _name;
				Executable _exec;
		};
		class CommandComparer {
			public:
				inline static int Compare(const Command &lhs, const Command &rhs) {
					return Core::DefaultComparer<Core::String>::Compare(lhs._name, rhs._name);
				}
		};

		class SimpleRunningCommand : public RunningCommand {
			public:
				typedef std::function<int(const Core::Collections::List<Core::String>&)> ActualCommand;

				SimpleRunningCommand(const Core::Collections::List<Core::String> &args, const ActualCommand &cmd) :
					_argsCache(args), _com(cmd)
				{
				}

				virtual CommandState GetState() const override {
					if (_executed || _com == nullptr) {
						return CommandState::Terminated;
					}
					return CommandState::Running;
				}
				virtual void Update(double) override {
					if (!_executed) {
						_executed = true;
						if (_com != nullptr) {
							_retCache = _com(_argsCache);
						}
					}
				}
				virtual int GetReturnValue() const override {
					return _retCache;
				}

				const ActualCommand &GetCommand() const {
					return _com;
				}
			protected:
				Core::Collections::List<Core::String> _argsCache;
				ActualCommand _com;
				bool _executed = false;
				int _retCache = 0;
		};
		class SimpleConsoleRunner : public ConsoleRunnerBase {
			public:
				virtual ~SimpleConsoleRunner() {
					if (_curCommand != nullptr) {
						_curCommand->~RunningCommand();
						Core::GlobalAllocator::Free(_curCommand);
					}
				}

				const Core::Collections::SortedList<Command, CommandComparer> &Commands() const {
					return _commands;
				}
				Core::Collections::SortedList<Command, CommandComparer> &Commands() {
					return _commands;
				}

				Core::ReferenceProperty<bool> EchoCommand {true};
			protected:
				Core::Collections::SortedList<Command, CommandComparer> _commands;
				RunningCommand *_curCommand = nullptr;

				virtual void OnCommand(const Core::String&) override;
				virtual void Update(double dt) override {
					if (_curCommand) {
						_curCommand->Update(dt);
						switch (_curCommand->GetState()) {
							case CommandState::Running: {
								if (IsInputEnabled()) {
									DisableInput();
								}
								break;
							}
							case CommandState::WaitingInput: {
								if (!IsInputEnabled()) {
									EnableInput();
								}
								break;
							}
							case CommandState::Terminated: {
								if (_curCommand->GetReturnValue() != 0) {
									SetCursorColor(Core::Color(255, 0, 0, 255));
									WriteLine(_TEXT("Command terminated with return value ") + Core::ToString(_curCommand->GetReturnValue()));
								}
								_curCommand->~RunningCommand();
								Core::GlobalAllocator::Free(_curCommand);
								_curCommand = nullptr;
								EnableInput();
								break;
							}
						}
					}
				}
		};

		class Console : public PanelBase {
				friend class ConsoleRunnerBase;
			public:
				Console() : PanelBase() {
					_output.SetAnchor(Anchor::All);
					_output.SetMargins(Thickness(0.0));

					_input.SetAnchor(Anchor::BottomDock);
					_input.SetMargins(Thickness(0.0));
					_input.MultiLine = false;
					_input.Text().TextColor = Core::Color(0, 0, 0, 255);
					_input.KeyboardText += [&](const Core::Input::TextInfo &info) {
						if (info.GetChar() == _TEXT('\n') || info.GetChar() == _TEXT('\r')) {
							if (_runner != nullptr) {
								_runner->OnCommand(_input.Text().Content);
							}
							_input.SetText(_TEXT(""));
						}
					};
				}
				virtual ~Console() {
					if (_runner != nullptr) {
						_runner->_father = nullptr;
					}
				}

				void AttachRunner(ConsoleRunnerBase &runner) {
					if (_runner != nullptr) {
						throw Core::InvalidOperationException(_TEXT("there is already a runner attached to this console"));
					}
					_runner = &runner;
					_runner->_father = this;
				}
				ConsoleRunnerBase *GetAttachedRunner() {
					return _runner;
				}
				const ConsoleRunnerBase *GetAttachedRunner() const {
					return _runner;
				}
				void DetachRunner() {
					_runner->_father = nullptr;
					_runner = nullptr;
				}

				Core::GetSetProperty<const Graphics::TextRendering::Font*> Font {
					[this](const Graphics::TextRendering::Font *fnt) {
						_input.Text().Font = fnt;
						_input.SetDesiredVisibleLine(1);
						_output.SetFont(fnt);
						_output.SetMargins(Thickness(0.0, 0.0, 0.0, _input.GetSize().Height));
					},
					[this]() {
						return _input.Text().Font;
					}
				};

				virtual void ResetForInput() {
					_input.SetText(_TEXT(""));
					if (GetWorld()) {
						GetWorld()->SetFocus(&_input);
					}
				}

				virtual void AutoSetWidth() {
					_output.AutoSetWidth();
					SetSize(Size(_output.GetSize().Width, GetSize().Height));
				}

				const SimpleConsoleTextBox &OutputTextBox() const {
					return _output;
				}
			protected:
				SimpleConsoleTextBox _output;
				TextBox<Graphics::TextRendering::BasicText> _input;
				ConsoleRunnerBase *_runner = nullptr;

				virtual void Initialize() override {
					PanelBase::Initialize();
					_col.Insert(_output);
					_col.Insert(_input);
				}
				virtual void Update(double dt) override {
					PanelBase::Update(dt);
					if (_runner != nullptr) {
						_runner->Update(dt);
					}
				}
		};
	}
}
