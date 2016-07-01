#include <exception>
#include <iostream>
#include <sstream>
#include <fstream>
using namespace std;
#include <windows.h>

#include "DoodleEngine.h"
using namespace DE;
using namespace DE::Core;
using namespace DE::Core::Input;
using namespace DE::Core::Math;
using namespace DE::Core::Collections;
using namespace DE::Graphics;
using namespace DE::Graphics::TextRendering;
using namespace DE::Graphics::TextRendering::FreeTypeAccess;
using namespace DE::Graphics::RenderingContexts;
using namespace DE::UI;
using namespace DE::IO;
using namespace DE::Utils;
using namespace DE::Utils::LightCaster;
using namespace DE::Utils::CharacterPhysics;
using namespace DE::Utils::MazeGenerator;

void TerminateCall() {
	std::cout<<"here we go again\n";
}
class Test {
	public:
		Test() : window(_TEXT("TEST")), context(window) {
			std::set_terminate(TerminateCall);

			window.ClientSize = Vector2(1024.0, 768.0);
			window.PutToCenter();
			r = context.CreateRenderer();
		}
		virtual ~Test() {
		}

		void Run() {
			window.Show();
			stw.Start();
			window.CloseButtonClicked += [&](const Info&) { stop = true; };
			while (!stop) {
				while (window.Idle()) {
				}
				Update(stw.TickInSeconds());
				Render();
			}
		}

		virtual void Update(double) {
		}
		virtual void Render() = 0;
	protected:
		Window window;
		GLContext context;
		Renderer r;
		Stopwatch stw;
		bool stop = false;
};

class ControlTest : public Test {
	public:
		class RunningBFCommand : public RunningCommand {
			public:
				RunningBFCommand(const List<String>&, ControlTest &test) : _father(test) {
					String rawProg = _father.tBox.Text().Content;
					for (size_t i = 0; i < rawProg.Length(); ++i) {
						TCHAR c = rawProg[i];
						if (
							c == _TEXT('<') ||
							c == _TEXT('>') ||
							c == _TEXT('+') ||
							c == _TEXT('-') ||
							c == _TEXT('[') ||
							c == _TEXT(']') ||
							c == _TEXT('.') ||
							c == _TEXT(',')
						) {
							_program += c;
						}
					}
					_stack.PushBack(0);
				}

				CommandState GetState() const override {
					if (_progPtr >= _program.Length()) {
						return CommandState::Terminated;
					}
					if (_program[_progPtr] == _TEXT(',')) {
						return CommandState::WaitingInput;
					}
					return CommandState::Running;
				}
				virtual void Update(double) {
					while (_progPtr < _program.Length()) {
						switch (_program[_progPtr]) {
							case _TEXT('<'): {
								if (_stackPtr == 0) {
									_father.runner.SetCursorColor(Core::Color(255, 0, 0, 255));
									_father.runner.WriteLine(_TEXT("The pointer already points to the leftmost element in the array"));
									_father.runner.SetCursorColor(Core::Color(255, 255, 255, 255));
									_progPtr = _program.Length();
									_retCache = 1;
								} else {
									--_stackPtr;
								}
								break;
							}
							case _TEXT('>'): {
								++_stackPtr;
								while (_stackPtr >= _stack.Count()) {
									_stack.PushBack(0);
								}
								break;
							}
							case _TEXT('+'): {
								++_stack[_stackPtr];
								break;
							}
							case _TEXT('-'): {
								--_stack[_stackPtr];
								break;
							}
							case _TEXT('['): {
								if (_stack[_stackPtr] == 0) {
									size_t sd = 0;
									for (
										++_progPtr;
										_progPtr < _program.Length() && (sd > 0 || _program[_progPtr] != _TEXT(']'));
										++_progPtr
									) {
										if (_program[_progPtr] == _TEXT('[')) {
											++sd;
										} else if (_program[_progPtr] == _TEXT(']')) {
											--sd;
										}
									}
								}
								break;
							}
							case _TEXT(']'): {
								if (_stack[_stackPtr] != 0) {
									size_t sd = 0;
									if (_progPtr > 0) {
										--_progPtr;
									}
									for (; _progPtr > 0 && (sd > 0 || _program[_progPtr] != _TEXT('[')); --_progPtr) {
										if (_program[_progPtr] == _TEXT(']')) {
											++sd;
										} else if (_program[_progPtr] == _TEXT('[')) {
											--sd;
										}
									}
								}
								break;
							}
							case _TEXT('.'): {
								_father.runner.Write(static_cast<TCHAR>(_stack[_stackPtr]));
								break;
							}
							case _TEXT(','): {
								if (_cmdQ.Count() > 0) {
									_stack[_stackPtr] = static_cast<unsigned char>(_cmdQ.PopHead());
								} else {
									return; // wait for input
								}
								break;
							}
						}
						++_progPtr;
					}
					if (_progPtr > _program.Length()) {
						_father.runner.Write(_TEXT("\n"));
						if (_retCache == 0) {
							_father.runner.WriteLine(_TEXT("The program has terminated normally"));
						}
					}
				}
				virtual int GetReturnValue() const override {
					return _retCache;
				}
				virtual void OnInput(const String &str) {
					_father.runner.WriteLine(_TEXT(""));
					for (size_t i = 0; i < str.Length(); ++i) {
						_cmdQ.PushTail(str[i]);
					}
				}
			protected:
				String _program;
				List<unsigned char> _stack;
				size_t _progPtr = 0, _stackPtr = 0;
				int _retCache = 0;
				Queue<TCHAR> _cmdQ;
				ControlTest &_father;
		};
		class RunningBounceBallCommand : public RunningCommand {
			public:
				constexpr static size_t
					ScreenWidth = 40,
					ScreenHeight = 15,
					MainMenuStart = 9,
					MainMenuQuit = 11,
					BoardSize = 3,
					InitBoardX = 17,
					BoardY = 13,
					InitBallX = 18,
					InitBallY = 12;
				constexpr static double
					MoveCD = 0.1,
					FlyCD = 0.2;

				enum class GameState {
					MainMenu,
					Playing,
					GameOver,
					Terminated
				};

				RunningBounceBallCommand(const List<String> &args, ConsoleRunnerBase &runner) : _father(runner) {
					if (args.Count() > 1) {
						for (size_t i = 0; i < args[1].Length(); ++i) {
							_fileName += (char)(args[1][i]);
						}
					} else {
						_fileName = "map.txt";
					}
					if (IsKeyDown(VK_RETURN)) {
						_led = true;
					}
					InitializeState();
				}

				virtual CommandState GetState() const override {
					return (_state == GameState::Terminated ? CommandState::Terminated : CommandState::Running);
				}
				virtual void Update(double dt) override {
					switch (_state) {
						case GameState::MainMenu: {
							if (AlterChoice()) {
								if (_choice == 0) {
									_father.SetAbsoluteCursorPosition(0, MainMenuStart);
									_father.WriteLine(_TEXT("#               > start                #"));
									_father.SetAbsoluteCursorPosition(0, MainMenuQuit);
									_father.WriteLine(_TEXT("#                 quit                 #"));
								} else {
									_father.SetAbsoluteCursorPosition(0, MainMenuStart);
									_father.WriteLine(_TEXT("#                 start                #"));
									_father.SetAbsoluteCursorPosition(0, MainMenuQuit);
									_father.WriteLine(_TEXT("#               > quit                 #"));
								}
								_father.SetAbsoluteCursorPosition(0, ScreenHeight);
							}
							if (IsKeyDown(VK_RETURN)) {
								if (!_led) {
									if (_choice == 0) {
										_state = GameState::Playing;
									} else {
										_state = GameState::Terminated;
									}
									InitializeState();
									_led = true;
								}
							} else {
								_led = false;
							}
							break;
						}
						case GameState::Playing: {
							if (_moveCD <= 0.0) {
								_moveCD += dt;
							}
							if (IsKeyDown(VK_LEFT)) {
								if (_moveCD > 0.0 && _boardX > 1) {
									--_boardX;
									_wall[_boardX + BoardSize][ScreenHeight - 2] = 0;
									_wall[_boardX][ScreenHeight - 2] = 1;
									_moveCD -= MoveCD;
									if (!_started) {
										_started = true;
									}
								}
							}
							if (IsKeyDown(VK_RIGHT)) {
								if (_moveCD > 0.0 && _boardX + BoardSize < ScreenWidth - 1) {
									_wall[_boardX][ScreenHeight - 2] = 0;
									_wall[_boardX + BoardSize][ScreenHeight - 2] = 1;
									++_boardX;
									_moveCD -= MoveCD;
									if (!_started) {
										_spdX = true;
										_started = true;
									}
								}
							}
							if (_moveCD > 0.0) {
								_moveCD = 0.0;
							}

							if (_started) {
								_flyCD -= dt;
								while (_flyCD < 0.0) {
									_flyCD += FlyCD;
									_father.SetAbsoluteCursorPosition(_ballX, _ballY);
									_father.Write(_TEXT(" "));
									UpdateBall();
								}
							}

							_father.SetAbsoluteCursorPosition(1, ScreenHeight - 2);
							for (size_t t = 2; t < ScreenWidth; ++t) {
								_father.Write(_TEXT(" "));
							}
							_father.SetAbsoluteCursorPosition(_boardX, ScreenHeight - 2);
							for (size_t t = 0; t < BoardSize; ++t) {
								_father.SetCursorColor(Color(255, 255, 0, 255));
								_father.Write(_TEXT(":"));
							}
							_father.SetAbsoluteCursorPosition(_ballX, _ballY);
							_father.SetCursorColor(Color(0, 255, 0, 255));
							_father.Write(_TEXT("O"));
							_father.SetCursorColor(Color(255, 255, 255, 255));
							_father.SetAbsoluteCursorPosition(0, ScreenHeight);

							if (_ballY == ScreenHeight - 2 || _bkc == 0) {
								_state = GameState::GameOver;
								InitializeState();
							}
							break;
						}
						case GameState::GameOver: {
							if (AlterChoice()) {
								if (_choice == 0) {
									_father.SetAbsoluteCursorPosition(0, MainMenuStart);
									_father.WriteLine(_TEXT("#              > restart               #"));
									_father.SetAbsoluteCursorPosition(0, MainMenuQuit);
									_father.WriteLine(_TEXT("#                quit                  #"));
								} else {
									_father.SetAbsoluteCursorPosition(0, MainMenuStart);
									_father.WriteLine(_TEXT("#                restart               #"));
									_father.SetAbsoluteCursorPosition(0, MainMenuQuit);
									_father.WriteLine(_TEXT("#              > quit                  #"));
								}
								_father.SetAbsoluteCursorPosition(0, ScreenHeight);
							}
							if (IsKeyDown(VK_RETURN)) {
								if (!_led) {
									if (_choice == 0) {
										_state = GameState::Playing;
									} else {
										_state = GameState::Terminated;
									}
									InitializeState();
									_led = true;
								}
							} else {
								_led = false;
							}
							break;
						}
						default: {
							break;
						}
					}
				}
				virtual int GetReturnValue() const override {
					return 0;
				}
			protected:
				GameState _state = GameState::MainMenu;
				size_t _choice = 0, _maxChoice = 0;
				bool _ldd = false, _lud = false, _led = false, _spdX = false, _spdY = false, _started = false;
				ConsoleRunnerBase &_father;
				double _moveCD = 0.0, _flyCD = 0.0;
				size_t _boardX = 0, _ballX = 0, _ballY = 0, _wall[ScreenWidth][ScreenHeight], _bkc;
				AsciiString _fileName;

				virtual bool AlterChoice() {
					size_t oc = _choice;
					if (IsKeyDown(VK_DOWN)) {
						if (!_ldd) {
							if ((++_choice) == _maxChoice) {
								_choice = _maxChoice - 1;
							}
							_ldd = true;
						}
					} else {
						_ldd = false;
					}
					if (IsKeyDown(VK_UP)) {
						if (!_lud) {
							if (_choice > 0) {
								--_choice;
							}
							_lud = true;
						}
					} else {
						_lud = false;
					}
					return (_choice != oc);
				}
				virtual void UpdateBall() {
					int xdiff = (_spdX ? 1 : -1), ydiff = (_spdY ? 1 : -1);
					bool hasCol = false;
					do {
						size_t nx = static_cast<int>(_ballX) + xdiff, ny = static_cast<int>(_ballY) + ydiff;
						hasCol = false;
						if (_wall[nx][_ballY]) {
							if (_wall[nx][_ballY] == 2) {
								_wall[nx][_ballY] = 0;
								--_bkc;
								_father.SetAbsoluteCursorPosition(nx, _ballY);
								_father.Write(_TEXT(" "));
							}
							hasCol = true;
							_spdX = !_spdX;
							xdiff = -xdiff;
						}
						if (_wall[_ballX][ny]) {
							if (_wall[_ballX][ny] == 2) {
								_wall[_ballX][ny] = 0;
								--_bkc;
								_father.SetAbsoluteCursorPosition(_ballX, ny);
								_father.Write(_TEXT(" "));
							}
							hasCol = true;
							_spdY = !_spdY;
							ydiff = -ydiff;
						}
						if (!hasCol) {
							if (_wall[nx][ny]) {
								if (_wall[nx][ny] == 2) {
									_wall[nx][ny] = 0;
									--_bkc;
									_father.SetAbsoluteCursorPosition(nx, ny);
									_father.Write(_TEXT(" "));
								}
								hasCol = true;
								_spdX = !_spdX;
								_spdY = !_spdY;
								xdiff = -xdiff;
								ydiff = -ydiff;
							}
						}
					} while (hasCol);
					_father.SetAbsoluteCursorPosition(0, ScreenHeight);
					_ballX = static_cast<int>(_ballX) + xdiff;
					_ballY = static_cast<int>(_ballY) + ydiff;
				}
				virtual void InitializeState() {
					_father.ClearConsole();
					_choice = 0;
					switch (_state) {
						case GameState::MainMenu: {
							_maxChoice = 2;
							_father.WriteLine(_TEXT("########################################"));
							_father.WriteLine(_TEXT("#                                      #"));
							_father.WriteLine(_TEXT("#                                      #"));
							_father.WriteLine(_TEXT("#                                      #"));
							_father.WriteLine(_TEXT("#                                      #"));
							_father.WriteLine(_TEXT("#             BOUNCE BALL              #"));
							_father.WriteLine(_TEXT("#                                      #"));
							_father.WriteLine(_TEXT("#                                      #"));
							_father.WriteLine(_TEXT("#                                      #"));
							_father.WriteLine(_TEXT("#               > start                #"));
							_father.WriteLine(_TEXT("#                                      #"));
							_father.WriteLine(_TEXT("#                 quit                 #"));
							_father.WriteLine(_TEXT("#                                      #"));
							_father.WriteLine(_TEXT("#                                      #"));
							_father.WriteLine(_TEXT("########################################"));
							break;
						}
						case GameState::Playing: {
							_moveCD = 0.0;
							_flyCD = 0.0;
							_boardX = InitBoardX;
							_ballX = InitBallX;
							_ballY = InitBallY;
							_started = false;
							_spdX = _spdY = false;
							_bkc = 0;
							memset(_wall, 0, sizeof(_wall));
							for (size_t i = 0; i < ScreenWidth; ++i) {
								_wall[i][0] = _wall[i][ScreenHeight - 1] = 1;
							}
							for (size_t i = 1; i < ScreenHeight - 1; ++i) {
								_wall[0][i] = _wall[ScreenWidth - 1][i] = 1;
							}
							for (size_t i = 0; i < BoardSize; ++i) {
								_wall[_boardX + i][ScreenHeight - 2] = 1;
							}
							FileAccess acc(_fileName, FileAccessType::ReadText);
							for (size_t y = 1; y < ScreenHeight - 2; ++y) {
								for (size_t x = 1; x < ScreenWidth - 1; ++x) {
									for (TCHAR c = acc.ReadChar(); c != WEOF; c = acc.ReadChar()) {
										if (c == _TEXT('.')) {
											break;
										} else if (c == _TEXT('*')) {
											_wall[x][y] = 2;
											++_bkc;
											break;
										} else if (c == _TEXT('#')) {
											_wall[x][y] = 1;
											break;
										}
									}
								}
							}
							_father.WriteLine(_TEXT("########################################"));
							for (size_t y = 1; y < ScreenHeight - 1; ++y) {
								_father.Write(_TEXT("#"));
								for (size_t x = 1; x < ScreenWidth - 1; ++x) {
									if (_wall[x][y] == 2) {
										_father.SetCursorColor(Color(100, 100, 255, 255));
										_father.Write(_TEXT("*"));
										_father.SetCursorColor(Color(255, 255, 255, 255));
									} else if (_wall[x][y] == 1) {
										_father.Write(_TEXT("#"));
									} else {
										_father.Write(_TEXT(" "));
									}
								}
								_father.WriteLine(_TEXT("#"));
							}
							_father.WriteLine(_TEXT("########################################"));
							break;
						}
						case GameState::GameOver: {
							_maxChoice = 2;
							_father.WriteLine(_TEXT("########################################"));
							_father.WriteLine(_TEXT("#                                      #"));
							_father.WriteLine(_TEXT("#                                      #"));
							_father.WriteLine(_TEXT("#                                      #"));
							_father.WriteLine(_TEXT("#                                      #"));
							_father.WriteLine(_TEXT("#              GAME OVER               #"));
							_father.WriteLine(_TEXT("#                                      #"));
							_father.WriteLine(_TEXT("#                                      #"));
							_father.WriteLine(_TEXT("#                                      #"));
							_father.WriteLine(_TEXT("#              > restart               #"));
							_father.WriteLine(_TEXT("#                                      #"));
							_father.WriteLine(_TEXT("#                quit                  #"));
							_father.WriteLine(_TEXT("#                                      #"));
							_father.WriteLine(_TEXT("#                                      #"));
							_father.WriteLine(_TEXT("########################################"));
							break;
						}
						case GameState::Terminated: {
							break;
						}
					}
				}
		};

		ControlTest() : Test() {
			r = context.CreateRenderer();

			transBkg.BrushColor() = Color(200, 200, 200, 255);

//			gen.Dictionary = _TEXT("`1234567890-=\\~!@#$%^&*()_+|qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM[];',./{}:\"<>? \n\t");
//			gen.FontSize = 15.0;
//			gen.FontFile = "simsun.ttc";
//			fnt = gen.Generate(r);
//
//			gen.FontFile = "consolefont.ttf";
//			consFnt = gen.Generate(r);

			FontFace textface("cambria.ttc", 18.0);
			fnt = AutoFont(&context, textface);
			FontFace consface("Inconsolata.otf", 15.0);
			consFnt = AutoFont(&context, consface);

			//FileWriter writer(_TEXT("test.fnt"));
			//fnt.Save(_TEXT("test"), writer);
			//writer.Close();

			//FileAccess reader("test.fnt", FileAccessType::NewReadBinary);
			//fnt = BMPFont::Load(r, reader);

#ifdef DEBUG
//			base.Name = "the base panel";
//			popup.Name = "the popup panel";
//			p.Name = "the main wrap panel";
//			b.Name = "the button";
//			lbl.Name = "the label";
//			sBar.Name = "the slider";
//			pBar.Name = "the progress bar";
//			ckBox.Name = "the first check box";
//			tstat.Name = "the second check box";
//			rdOnly.Name = "the read-only check box";
//			view.Name = "the main scroll view";
//			comBox.Name = "the combo box";
//			tBox.Name = "the text box";
#endif

			base.SetAnchor(Anchor::All);
			base.SetMargins(Thickness());
			w.SetChild(&base);

			popup.SetAnchor(Anchor::All);
			popup.SetMargins(Thickness());
			base.Children().Insert(popup);
			base.Children().SetZIndex(popup, 1);

			view.SetAnchor(Anchor::All);
			view.SetMargins(Thickness(50.0));
			view.Background() = &transBkg;
			base.Children().Insert(view);

			p.SetLayoutDirection(LayoutDirection::Vertical);
			view.SetChild(&p);

			pgFT.PenColor() = Color(0, 0, 255, 255);
			pGraph.FrameTimePen() = &pgFT;

			pgMU.PenColor() = Color(0, 255, 0, 255);
			pGraph.MemoryUsagePen() = &pgMU;

			pGraph.SetAnchor(Anchor::TopDock);
			pGraph.SetMargins(Thickness(10.0));
			pGraph.SetSize(Size(0.0, 100.0));
			p.Children().Insert(pGraph);

			b.SetAnchor(Anchor::Top);
			b.SetSize(Size(150.0, 25.0));
			b.SetMargins(Thickness(10.0));
			b.Click += [&](const Info&) {
				sBar.SetValue(1.0);
			};
			b.Content().Content = _TEXT("Restore Size");
			b.Content().Font = &fnt;
			b.Content().CacheFormat();
			b.Content().TextColor = Color(0, 0, 0, 255);
			p.Children().Insert(b);

			bDump.SetAnchor(Anchor::Top);
			bDump.SetSize(Size(150.0, 25.0));
			bDump.SetMargins(Thickness(10.0));
			bDump.Click += [&](const Info&) {
				GlobalAllocator::DumpAsText("tmpdump.txt");
			};
			bDump.Content().Content = _TEXT("Dump");
			bDump.Content().Font = &fnt;
			bDump.Content().CacheFormat();
			bDump.Content().TextColor = Color(0, 0, 0, 255);
			p.Children().Insert(bDump);

			lbl.SetAnchor(Anchor::Top);
			lbl.SetSize(Size(100.0, 25.0));
			lbl.SetMargins(Thickness(10.0));
			lbl.Content().Font = &fnt;
			lbl.Content().TextColor = Color(0, 0, 0, 255);
			lbl.Content().Content = _TEXT("The quick brown fox jumps over the lazy dog");
			lbl.Content().Scale = 0.0;
			lbl.FitContent();
			p.Children().Insert(lbl);

			sBarIndic.BrushColor() = Color(50, 50, 50, 255);

			sBar.SetAnchor(Anchor::TopLeft);
			sBar.SetMargins(Thickness(0.0));
			sBar.SetSize(Size(200.0, 20.0));
			sBar.SetMaxValue(5.0);
			sBar.SetLayoutDirection(LayoutDirection::Horizontal);
			sBar.IndicatorBrush() = &sBarIndic;
			sBar.JumpToClickPosition() = true;
			sBar.ValueChanged += [&](const Info&) {
				lbl.Content().Scale = sBar.GetValue();
				lbl.FitContent();
				p.FitContent();
			};
			p.Children().Insert(sBar);

			pBar.SetAnchor(Anchor::Top);
			pBar.SetMargins(Thickness(10.0));
			pBar.SetSize(Size(15.0, 160.0));
			pBar.SetMaxProgress(0.8);
			pBar.SetLayoutDirection(LayoutDirection::Vertical);
			p.Children().Insert(pBar);

			ckBox.SetAnchor(Anchor::TopDock);
			ckBox.SetMargins(Thickness(10.0));
			ckBox.Content().Font = &fnt;
			ckBox.Content().Content = _TEXT("keep the system running");
			ckBox.Content().CacheFormat();
			ckBox.Content().TextColor = Color(0, 0, 0, 255);
//			ckBox.StateChanged += [this](const CheckBoxStateChangeInfo &info) {
//				consoleTB.SetLineColor(Color(255, 255, 0, 255));
//				consoleTB.Write(_TEXT("the system is now "));
//				if (info.NewState == CheckBoxState::Checked) {
//					consoleTB.WriteLine(_TEXT("running"));
//				} else {
//					consoleTB.WriteLine(_TEXT("not running"));
//				}
//			};
			ckBox.FitContent();
			p.Children().Insert(ckBox);

			tstat.SetAnchor(Anchor::TopDock);
			tstat.SetMargins(Thickness(10.0));
			tstat.Content().Font = &fnt;
			tstat.Content().Content = _TEXT("The CheckBox above is\nbutton stYle");
			tstat.Content().CacheFormat();
			tstat.StateChanged += [this](const CheckBoxStateChangeInfo &info) {
				ckBox.Type = (info.NewState == CheckBoxState::Checked ? CheckBoxType::Button : CheckBoxType::Box);
			};
			tstat.Content().TextColor = Color(0, 0, 0, 255);
			tstat.FitContent();
			p.Children().Insert(tstat);

			tBox.Text()<<
				&fnt<<Color(0, 0, 0, 255)<<_TEXT("123")<<
				TextFormatStreaming::NewLocalVerticalPosition(1.0)<<_TEXT("45")<<
				Color(255, 0, 0, 255)<<TextFormatStreaming::NewScale(1.5)<<_TEXT("67890")<<
				&consFnt<<Color(0, 0, 0, 255)<<TextFormatStreaming::NewScale(1.0)<<_TEXT("vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv")<<
				&fnt;
//			tBox.Text().TextColor = Color(0, 0, 0, 255);
//			tBox.Text().Font = &fnt;
//			tBox.Text().TextColor = Color(0, 0, 0, 255);

			// tests below have passed
//			tBox.Text().Scale = 1.5;
//			tBox.Text().HorizontalAlignment = HorizontalTextAlignment::Center; // only locally centered

			tBox.SetAnchor(Anchor::TopDock);
			tBox.SetSize(Size(0.0, fnt.GetHeight() * 10));
			tBox.SetMargins(Thickness(10.0));
			tBoxCaret.PenColor() = Color(0, 0, 0, 255);
			tBox.CaretPen() = &tBoxCaret;
			tBox.WrapText = true;
			p.Children().Insert(tBox);

			rdOnly.SetAnchor(Anchor::TopLeft);
			rdOnly.SetMargins(Thickness(10.0));
			rdOnly.Content().Font = &fnt;
			rdOnly.Content().Content = _TEXT("The TextBox above is ReadOnly");
			rdOnly.Content().CacheFormat();
			rdOnly.StateChanged += [this](const CheckBoxStateChangeInfo &info) {
				if (info.NewState == CheckBoxState::Checked) {
					tBox.ReadOnly() = true;
				} else {
					tBox.ReadOnly() = false;
				}
			};
			rdOnly.Content().TextColor = Color(0, 0, 0, 255);
			rdOnly.FitContent();
			p.Children().Insert(rdOnly);

			runner.Commands().InsertLeft(Command(_TEXT("help"), [&](const List<String> &args) {
				return new (GlobalAllocator::Allocate(sizeof(SimpleRunningCommand))) SimpleRunningCommand(args, [&](const List<String>&) {
					runner.WriteLine(_TEXT("beer	prints the lyrics of the song \"99 bottles of beer\""));
					runner.WriteLine(_TEXT("cls		clears the console screen"));
					runner.WriteLine(_TEXT("bf		runs the brainf*ck program specified in the parameters"));
					runner.WriteLine(_TEXT("ball	start a bounce ball game"));
					runner.WriteLine(_TEXT("gblur	apply gaussian blur effect to a specified bitmap"));
					runner.WriteLine(_TEXT("zip		zip or unzip a specified file"));
					runner.WriteLine(_TEXT("exit	exit the program"));
					return 0;
				});
			}));
			runner.Commands().InsertLeft(Command(_TEXT("beer"), [&](const List<String> &args) {
				return new (GlobalAllocator::Allocate(sizeof(SimpleRunningCommand))) SimpleRunningCommand(args, [&](const List<String>&) {
					for (int x = 99; x > 1; ) {
						runner.SetCursorColor(Core::Color(255 - x * 2.55, x * 2.55, 0, 255));
						runner.Write(ToString(x));
						runner.SetCursorColor(Core::Color(255, 255, 255, 255));
						runner.Write(_TEXT(" bottles of beer on the wall, "));
						runner.SetCursorColor(Core::Color(255 - x * 2.55, x * 2.55, 0, 255));
						runner.Write(ToString(x));
						runner.SetCursorColor(Core::Color(255, 255, 255, 255));
						runner.WriteLine(_TEXT(" bottles of beer."));
						runner.Write(_TEXT("Take one down and pass it around, "));
						--x;
						runner.SetCursorColor(Core::Color(255 - x * 2.55, x * 2.55, 0, 255));
						runner.Write(ToString(x));
						runner.SetCursorColor(Core::Color(255, 255, 255, 255));
						runner.WriteLine(_TEXT(" bottles of beer on the wall."));
						runner.WriteLine(_TEXT(""));
					}
					runner.SetCursorColor(Core::Color(255, 0, 0, 255));
					runner.Write(_TEXT("1"));
					runner.SetCursorColor(Core::Color(255, 255, 255, 255));
					runner.Write(_TEXT(" bottles of beer on the wall, "));
					runner.SetCursorColor(Core::Color(255, 0, 0, 255));
					runner.Write(_TEXT("1"));
					runner.SetCursorColor(Core::Color(255, 255, 255, 255));
					runner.WriteLine(_TEXT(" bottles of beer."));
					runner.WriteLine(_TEXT("Take one down and pass it around, no more bottles of beer on the wall."));
					runner.WriteLine(_TEXT(""));
					runner.WriteLine(_TEXT("No more bottles of beer on the wall, no more bottles of beer."));
					runner.Write(_TEXT("Go to the store and buy some more, "));
					runner.SetCursorColor(Core::Color(0, 255, 0, 255));
					runner.Write(_TEXT("99"));
					runner.SetCursorColor(Core::Color(255, 255, 255, 255));
					runner.WriteLine(_TEXT(" bottles of beer on the wall."));
					return 0;
				});
			}));
			runner.Commands().InsertLeft(Command(_TEXT("cls"), [&](const List<String> &args) {
				return new (GlobalAllocator::Allocate(sizeof(SimpleRunningCommand))) SimpleRunningCommand(args, [&](const List<String>&) {
					runner.ClearConsole();
					return 0;
				});
			}));
			runner.Commands().InsertLeft(Command(_TEXT("bf"), [&](const List<String> &args) {
				return new (GlobalAllocator::Allocate(sizeof(RunningBFCommand))) RunningBFCommand(args, *this);
			}));
			runner.Commands().InsertLeft(Command(_TEXT("ball"), [&](const List<String> &args) {
				return new (GlobalAllocator::Allocate(sizeof(RunningBounceBallCommand))) RunningBounceBallCommand(args, runner);
			}));
			runner.Commands().InsertLeft(Command(_TEXT("gblur"), [&](const List<String> &args) {
				return new (GlobalAllocator::Allocate(sizeof(SimpleRunningCommand))) SimpleRunningCommand(args, [&](const List<String> &args) {
					if (args.Count() != 5) {
						runner.WriteLine(_TEXT("Usage: gblur (file name) (x radius) (y radius) (output file name)"));
						return 0;
					}
					Gdiplus::Bitmap *bmp = new Gdiplus::Bitmap(*(args[1]));
					Gdiplus::BitmapData data;
					Gdiplus::Rect rect;
					rect.X = rect.Y = 0;
					rect.Width = bmp->GetWidth();
					rect.Height = bmp->GetHeight();
					bmp->LockBits(&rect, Gdiplus::ImageLockModeRead | Gdiplus::ImageLockModeWrite, PixelFormat32bppARGB, &data);
					GdiPlusAccess::GaussianBlur(data, Extract<size_t>(args[2]), Extract<size_t>(args[3]));
					bmp->UnlockBits(&data);
					GdiPlusAccess::SaveBitmap(*bmp, args[4], Gdiplus::ImageFormatBMP);
					return 0;
				});
			}));
			runner.Commands().InsertLeft(Command(_TEXT("sysinfo"), [&](const List<String> &args) {
				return new (GlobalAllocator::Allocate(sizeof(SimpleRunningCommand))) SimpleRunningCommand(args, [&](const List<String>&) {
					runner.Write(
						_TEXT("FPS:\t\t\t\t") + ToString(counter.GetFPS()) + _TEXT("\n")
						_TEXT("Average FPS:\t\t") + ToString(counter.GetAverageFPS()) + _TEXT("\n")
						_TEXT("Memory Usage:\t\t") + ToString(GlobalAllocator::UsedSize()) + _TEXT("\n")
						_TEXT("Memory Allocated:\t") + ToString(GlobalAllocator::AllocatedSize()) + _TEXT("\n")
					);
					return 0;
				});
			}));
			runner.Commands().InsertLeft(Command(_TEXT("exit"), [&](const List<String> &args) {
				return new (GlobalAllocator::Allocate(sizeof(SimpleRunningCommand))) SimpleRunningCommand(args, [&](const List<String>&) {
					stop = true;
					return 0;
				});
			}));
			runner.Commands().InsertLeft(Command(_TEXT("zip"), [&](const List<String> &args) {
				return new (GlobalAllocator::Allocate(sizeof(SimpleRunningCommand))) SimpleRunningCommand(args, [&](const List<String> &args) {
					if (args.Count() == 4) {
						if (args[1] == _TEXT("z")) {
							Zipper zp;
							AsciiString
								from = NarrowString(args[2]),
								to = NarrowString(args[3]);
							if (!FileAccess::Exists(from)) {
								runner.SetCursorColor(Color(255, 0, 0, 255));
								runner.WriteLine(_TEXT("file ") + args[2] + _TEXT(" does not exist"));
								return -1;
							}
							FileAccess r(from, FileAccessType::ReadBinary);
							size_t sz = r.GetSize();
							unsigned char *data = (unsigned char*)GlobalAllocator::Allocate(sz);
							if (r.ReadBinaryRaw(data, sz) != sz) {
								runner.SetCursorColor(Color(255, 0, 0, 255));
								runner.WriteLine(_TEXT("cannot read the whole file"));
								return -1;
							}
							zp.SetData(data, sz);
							GlobalAllocator::Free(data);
							BitSet bs = zp.Zip();
							FileAccess w(to, FileAccessType::NewWriteBinary);
							w.WriteBinaryRaw(*bs, sizeof(BitSet::ChunkType) * bs.ChunkCount());
							runner.WriteLine(_TEXT("file successfully zipped, zipped size: ") + ToString(bs.ChunkCount()) + _TEXT(" bytes"));
							return 0;
						} else if (args[1] == _TEXT("u")) {
							Unzipper uzp;
							AsciiString
								from = NarrowString(args[2]),
								to = NarrowString(args[3]);
							if (!FileAccess::Exists(from)) {
								runner.SetCursorColor(Color(255, 0, 0, 255));
								runner.WriteLine(_TEXT("file ") + args[2] + _TEXT(" does not exist"));
								return -1;
							}
							FileAccess rf(from, FileAccessType::ReadBinary);
							size_t sz = rf.GetSize();
							unsigned char *data = (unsigned char*)GlobalAllocator::Allocate(sz);
							if (rf.ReadBinaryRaw(data, sz) != sz) {
								runner.SetCursorColor(Color(255, 0, 0, 255));
								runner.WriteLine(_TEXT("cannot read the whole file"));
								return -1;
							}
							uzp.SetData(data, sz);
							GlobalAllocator::Free(data);
							List<unsigned char> res = uzp.Unzip();
							FileAccess rt(to, FileAccessType::NewWriteBinary);
							rt.WriteBinaryRaw(*res, sizeof(unsigned char) * res.Count());
							runner.WriteLine(_TEXT("file successfully unzipped, unzipped size: ") + ToString(res.Count()) + _TEXT(" bytes"));
							return 0;
						}
					}
					runner.WriteLine(_TEXT("Usage: zip [z / u] (source file name) (target file name)"));
					return 0;
				});
			}));

			console.SetAnchor(Anchor::Top);
			console.SetMargins(Thickness(10.0));
			console.Font = &consFnt;
			console.SetSize(Size(0.0, 500.0));
			console.AutoSetWidth();
			console.AttachRunner(runner);
			p.Children().Insert(console);

			bLoad.SetAnchor(Anchor::Top);
			bLoad.SetSize(Size(150.0, 25.0));
			bLoad.SetMargins(Thickness(10.0));
			bLoad.Click += [&](const Info&) {
				LoadTextBoxContentFromFile("testbf.bf");
			};
			bLoad.Content().Content = _TEXT("Load text");
			bLoad.Content().Font = &fnt;
			bLoad.Content().CacheFormat();
			bLoad.Content().TextColor = Color(0, 0, 0, 255);
			p.Children().Insert(bLoad);

			p.Children().Insert(comBox);
			comBox.SetDropDownPanel(&popup);
			comBox.Content().Font = &fnt;
			comBox.Content().Content = _TEXT("Please choose your option");
			comBox.Content().TextColor = Color(0, 0, 0, 255);
			comBox.FitContent();
			comBox.SetAnchor(Anchor::TopDock);
			comBox.SetMargins(Thickness(10));
			InsertComboBoxItem(_TEXT("the first one"));
			InsertComboBoxItem(_TEXT("the second one"));
			InsertComboBoxItem(_TEXT("another one"));
			InsertComboBoxItem(_TEXT("yet another one"));
			InsertComboBoxItem(_TEXT("bored?"));
			InsertComboBoxItem(_TEXT("I don't think so"));
			InsertComboBoxItem(_TEXT("hold on"));
			InsertComboBoxItem(_TEXT("nearing the end"));
			InsertComboBoxItem(_TEXT("the last one. hell it's sooooo looooooooong"));

			w.SetFather(&window);
			window.SizeChanged += [&](const SizeChangeInfo &info) {
				Math::Rectangle newVp(info.NewSize);
				r.SetViewport(newVp);
				r.SetViewbox(newVp);
				w.SetBounds(newVp);
			};

			p.FitContent();
		}
		void LoadTextBoxContentFromFile(const Core::AsciiString &file) {
			setlocale(LC_ALL, "");
			FileAccess acc(file, FileAccessType::ReadText);
			// TODO substitute solution
			String str;
			while (acc.Valid()) {
				TCHAR c = acc.ReadChar();
				if (c != WEOF) {
					str += c;
				}
			}
			tBox.SetText(str);
		}
		void InsertComboBoxItem(const Core::String &text) {
			typename SimpleComboBox<BasicText>::Item *item = &comBox.InsertItem();
			item->Content().Font = &fnt;
			item->Content().Content = text;
			item->Content().TextColor = Color(0, 0, 0, 255);
			item->FitContent();
		}

		virtual void Update(double dt) {
			counter.Update(dt);
			w.Update(dt);
		}
		virtual void Render() {
			r.Begin();
			w.Render(r);
			r.End();
		}
	private:
		AutoFont fnt, consFnt;

		SolidBrush transBkg;
		SolidBrush sBarIndic;
		Pen pgFT, pgMU;
		Pen tBoxCaret;

		UI::World w;
		Panel base, popup;
		WrapPanel p;
		SimpleButton<BasicText> b, bDump, bLoad;
		Label<BasicText> lbl;
		SliderBase sBar;
		SimpleProgressBar pBar;
		SimpleCheckBox<BasicText> ckBox, tstat, rdOnly;
		SimpleScrollView view;
		SimpleComboBox<BasicText> comBox;
//		TextBox<BasicText> tBox;
		TextBox<StreamedRichText> tBox;
		PerformanceGraph pGraph;
		SimpleConsoleRunner runner;
		Console console;

		FPSCounter counter;

//		BMPFontGenerator gen;
};
class LightTest : public Test {
	public:
		LightTest() : Test() {
			phys.Characters().PushBack(CharacterPhysics::Character());
			phys.Characters()[0].MoveSpeed = 100.0;

			window.SizeChanged += [&](const SizeChangeInfo &info) {
				Vector2 v = info.NewSize;
				r.SetViewport(Math::Rectangle(v));
				r.SetViewbox(Math::Rectangle(v));
			};
			window.MouseDown += [&](const MouseButtonInfo &info) {
				if (info.Button == MouseButton::Left) {
					if (IsKeyDown(VK_LSHIFT)) {
						phys.Characters()[0].Position = info.Position;
					} else {
						SpawnWallBlock(info.Position);
					}
				} else if (info.Button == MouseButton::Right) {
					Character cc;
					cc.Position = info.Position;
					cc.Size = 5.0 + 20.0 * random.NextDouble();
					phys.Characters().PushBack(cc);
				}
			};
			window.KeyboardText += [&](const TextInfo &info) {
				if (info.GetChar() == _TEXT('t')) {
					switch (mode) {
						case RenderMode::Triangles: {
							mode = RenderMode::Lines;
							break;
						}
						case RenderMode::Lines: {
							mode = RenderMode::Triangles;
							break;
						}
						default: {
							break;
						}
					}
				} else if (info.GetChar() == _TEXT('w')) {
					rw = !rw;
				} else if (info.GetChar() == _TEXT('1')) {
					--curRN;
				} else if (info.GetChar() == _TEXT('2')) {
					++curRN;
				}
			};
		}
		~LightTest() {
		}

		void Update(double dt) override {
			counter.Update(dt);
			phys.Characters()[0].GoingDirection = (Direction)(
				(IsKeyDown(VK_LEFT) ? (unsigned)Direction::Left : 0) |
				(IsKeyDown(VK_RIGHT) ? (unsigned)Direction::Right : 0) |
				(IsKeyDown(VK_DOWN) ? (unsigned)Direction::Down : 0) |
				(IsKeyDown(VK_UP) ? (unsigned)Direction::Up : 0));
			if (IsKeyDown('R')) {
				phys.Characters()[0].Position = Vector2();
			}
			phys.Update(dt);
			wstringstream ss;
			Vector2 v = phys.Characters()[0].Position;
			ss<<"DoodleEngine"<<" FPS:"<<counter.GetFPS()<<" ("<<v.X<<", "<<v.Y<<")";
			window.Title = ss.str().c_str();
			if (IsKeyDown(VK_SPACE)) {
				ShuffleWalls();
			}
			if (IsKeyDown(VK_RETURN)) {
				ShuffleCells();
			}
			if (IsKeyDown(VK_RSHIFT)) {
				ShuffleCells(1.0);
			}
			if (IsKeyDown('M')) {
				GenerateMaze(20, 20, Method::DFS, Vector2(50.0, 50.0), 30.0);
			}
			if (IsKeyDown('N')) {
				GenerateMaze(20, 20, Method::BFS, Vector2(50.0, 50.0), 30.0);
			}
			if (IsKeyDown('H')) {
				GenerateHDMaze(20, 20, Method::DFS, Vector2(50.0, 50.0), 30.0, 5.0);
			}
			if (IsKeyDown('J')) {
				GenerateHDMaze(20, 20, Method::BFS, Vector2(50.0, 50.0), 30.0, 5.0);
			}
			if (IsKeyDown('Q')) {
				GenerateWave(Vector2(100.0, 100.0), 500.0, 1.0, 0.1, 0.5);
			}

			vs.Clear();
			Color c(255, 0, 0, 255);
			l.Position = phys.Characters()[0].Position;
			List<LightCaster::CastResult> res = caster.Cast(l);
			if (mode == RenderMode::Lines) {
//				for (size_t i = 0; i < res.Count(); ++i) {
//					const LightCaster::CastResult &cRes = res[i];
//					if (cRes.Type == LightCaster::SplitType::FromSource) {
//						vs.PushBack(Vertex(cRes.SourcePoint1, c));
//						vs.PushBack(Vertex(cRes.TargetPoint1, c));
//						vs.PushBack(Vertex(cRes.TargetPoint1, c));
//						vs.PushBack(Vertex(cRes.TargetPoint2, c));
//						vs.PushBack(Vertex(cRes.TargetPoint2, c));
//						vs.PushBack(Vertex(cRes.SourcePoint1, c));
//					} else {
//						vs.PushBack(Vertex(cRes.SourcePoint1, c));
//						vs.PushBack(Vertex(cRes.TargetPoint1, c));
//						vs.PushBack(Vertex(cRes.TargetPoint1, c));
//						vs.PushBack(Vertex(cRes.TargetPoint2, c));
//						vs.PushBack(Vertex(cRes.TargetPoint2, c));
//						vs.PushBack(Vertex(cRes.SourcePoint2, c));
//						vs.PushBack(Vertex(cRes.SourcePoint2, c));
//						vs.PushBack(Vertex(cRes.SourcePoint1, c));
//					}
//				}
				if (curRN >= static_cast<int>(res.Count())) {
					curRN = res.Count() - 1;
				}
				if (curRN < 0) {
					curRN = 0;
				}
				for (int x = curRN; x >= 0; ) {
					const LightCaster::CastResult &cRes = res[x];
					if (cRes.Type == LightCaster::SplitType::FromSource) {
						vs.PushBack(Vertex(cRes.SourcePoint1, c));
						vs.PushBack(Vertex(cRes.TargetPoint1, c));
						vs.PushBack(Vertex(cRes.TargetPoint1, c));
						vs.PushBack(Vertex(cRes.TargetPoint2, c));
						vs.PushBack(Vertex(cRes.TargetPoint2, c));
						vs.PushBack(Vertex(cRes.SourcePoint1, c));
					} else {
						vs.PushBack(Vertex(cRes.SourcePoint1, c));
						vs.PushBack(Vertex(cRes.TargetPoint1, c));
						vs.PushBack(Vertex(cRes.TargetPoint1, c));
						vs.PushBack(Vertex(cRes.TargetPoint2, c));
						vs.PushBack(Vertex(cRes.TargetPoint2, c));
						vs.PushBack(Vertex(cRes.SourcePoint2, c));
						vs.PushBack(Vertex(cRes.SourcePoint2, c));
						vs.PushBack(Vertex(cRes.SourcePoint1, c));
					}
					x = cRes.Father;
				}
			} else {
				for (size_t i = 0; i < res.Count(); ++i) {
					const LightCaster::CastResult &cRes = res[i];
					if (cRes.Type == LightCaster::SplitType::FromSource) {
//						vs.PushBack(Vertex(cRes.SourcePoint1, Color(255, 255, 255, 255 * cRes.SourceStrength1)));
//						vs.PushBack(Vertex(cRes.TargetPoint1, Color(255, 255, 255, 255 * cRes.TargetStrength1)));
//						vs.PushBack(Vertex(cRes.TargetPoint2, Color(255, 255, 255, 255 * cRes.TargetStrength2)));
						vs.PushBack(Vertex(cRes.SourcePoint1, Color(255, 255, 255, 50)));
						vs.PushBack(Vertex(cRes.TargetPoint1, Color(255, 255, 255, 50)));
						vs.PushBack(Vertex(cRes.TargetPoint2, Color(255, 255, 255, 50)));
					} else {
//						Color
//							s1 = Color(255, 255, 255, 255 * cRes.SourceStrength1),
//							s2 = Color(255, 255, 255, 255 * cRes.SourceStrength2),
//							t1 = Color(255, 255, 255, 255 * cRes.TargetStrength1),
//							t2 = Color(255, 255, 255, 255 * cRes.TargetStrength2);
//						vs.PushBack(Vertex(cRes.SourcePoint1, s1));
//						vs.PushBack(Vertex(cRes.SourcePoint2, s2));
//						vs.PushBack(Vertex(cRes.TargetPoint1, t1));
//						vs.PushBack(Vertex(cRes.SourcePoint2, s2));
//						vs.PushBack(Vertex(cRes.TargetPoint1, t1));
//						vs.PushBack(Vertex(cRes.TargetPoint2, t2));
						vs.PushBack(Vertex(cRes.SourcePoint1, Color(255, 255, 255, 50)));
						vs.PushBack(Vertex(cRes.SourcePoint2, Color(255, 255, 255, 50)));
						vs.PushBack(Vertex(cRes.TargetPoint1, Color(255, 255, 255, 50)));
						vs.PushBack(Vertex(cRes.SourcePoint2, Color(255, 255, 255, 50)));
						vs.PushBack(Vertex(cRes.TargetPoint1, Color(255, 255, 255, 50)));
						vs.PushBack(Vertex(cRes.TargetPoint2, Color(255, 255, 255, 50)));
					}
				}
			}
		}
		void Render() override {
			r.Begin();
			r.DrawVertices(vs, mode);
			if (rw) {
				List<Vertex> vw;
				Color c(0, 255, 0, 255);
				for (size_t i = 0; i < caster.Walls().Count(); ++i) {
					vw.PushBack(Vertex(caster.Walls()[i].Node1, c));
					vw.PushBack(Vertex(caster.Walls()[i].Node2, c));
				}
				r.SetLineWidth(1.0);
				r.DrawVertices(vw, RenderMode::Lines);
			}
			for (size_t i = 0; i < phys.Characters().Count(); ++i) {
				const Character &acc = phys.Characters()[i];
				List<Vertex> vxs = CreateCircle(acc.Position, acc.Size, 20, Color(255, 0, 0, 255));
				r.SetLineWidth(1.0);
				r.DrawVertices(vxs, RenderMode::Lines);
			}
			r.DrawVertices(_tWall, RenderMode::Lines);
			_tWall.Clear();
			r.SetPointSize(5.0);
			r.DrawVertices(_tPoints, RenderMode::Points);
			_tPoints.Clear();
			r.End();
		}
	private:
		FPSCounter counter;

		Light l;
		List<Vertex> vs, _tWall, _tPoints;
		Random random;
		RenderMode mode = RenderMode::Triangles;
		Vector2 moveDir;
		bool rw = false;

		int curRN = 0;

		Caster caster;
		Environment phys;

		void ShuffleWalls() {
			caster.Walls().Clear();
			phys.Walls().Clear();
			for (size_t i = 0; i < 20; ++i) {
				SpawnWall(
					Vector2(random.NextDouble() * 600, random.NextDouble() * 600),
					Vector2(random.NextDouble() * 600, random.NextDouble() * 600));
			}
		}
		void GenerateWave(const Vector2 &start, double stretch, double ymult, double lmult, double partl) {
			caster.Walls().Clear();
			phys.Walls().Clear();
			double lastv = 0.0;
			for (double x = partl; x < stretch; x += partl) {
				double curv = sin(x * lmult) * ymult;
				SpawnWall(
					Vector2(start.X + x - partl, start.Y + lastv),
					Vector2(start.X + x, start.Y + curv)
				);
				lastv = curv;
			}
		}
		void GenerateMaze(size_t w, size_t h, Method met, const Vector2 &pos, double pathThickness) {
			caster.Walls().Clear();
			phys.Walls().Clear();
			List<BlockID> bs;
			BlockID ncc;
			for (ncc.Y = 5; ncc.Y < 15; ++ncc.Y) {
				for (ncc.X = 5; ncc.X < 15; ++ncc.X) {
					if (Square(ncc.X - 9.5) + Square(ncc.Y - 9.5) < 25) {
						bs.PushBack(ncc);
					}
				}
			}
			Maze m = MazeGenerator::Generate(w, h, BlockID(0, 0), BlockID(w - 1, h - 1), met, bs);
			m.ClearWallInRegion(bs);
			m.SetWall(BlockID(0, 0), Direction::Left, false);
			m.SetWall(BlockID(w - 1, h - 1), Direction::Right, false);
		 	for (BlockID id(0, 0); id.Y < h; ++id.Y) {
				for (id.X = 0; id.X < w; ++id.X) {
					if (m.IsWall(id, Direction::Left)) {
						SpawnWall(
							Vector2(pos.X + pathThickness * id.X, pos.Y + pathThickness * id.Y),
							Vector2(pos.X + pathThickness * id.X, pos.Y + pathThickness * (id.Y + 1)));
					}
					if (m.IsWall(id, Direction::Up)) {
						SpawnWall(
							Vector2(pos.X + pathThickness * id.X, pos.Y + pathThickness * id.Y),
							Vector2(pos.X + pathThickness * (id.X + 1), pos.Y + pathThickness * id.Y));
					}
					if (id.Y == h - 1 && m.IsWall(id, Direction::Down)) {
						SpawnWall(
							Vector2(pos.X + pathThickness * id.X, pos.Y + pathThickness * (id.Y + 1)),
							Vector2(pos.X + pathThickness * (id.X + 1), pos.Y + pathThickness * (id.Y + 1)));
					}
				}
				--id.X;
				if (m.IsWall(id, Direction::Right)) {
					SpawnWall(
						Vector2(pos.X + pathThickness * (id.X + 1), pos.Y + pathThickness * id.Y),
						Vector2(pos.X + pathThickness * (id.X + 1), pos.Y + pathThickness * (id.Y + 1)));
				}
			}
		}
		Vector2 GetCornerPt(const Vector2 &pos, const BlockID &id, double pathThickness, double hwt, Direction d) {
			switch (d) {
				case Direction::Right: {
					return Vector2(
						pos.X + (id.X + 1) * pathThickness - hwt,
						pos.Y + (id.Y + 1) * pathThickness - hwt);
				}
				case Direction::Up: {
					return Vector2(
						pos.X + (id.X + 1) * pathThickness - hwt,
						pos.Y + id.Y * pathThickness + hwt);
				}
				case Direction::Left: {
					return Vector2(
						pos.X + id.X * pathThickness + hwt,
						pos.Y + id.Y * pathThickness + hwt);
				}
				case Direction::Down: {
					return Vector2(
						pos.X + id.X * pathThickness + hwt,
						pos.Y + (id.Y + 1) * pathThickness - hwt);
				}
			}
			return pos;
		}
		void GenerateHDMaze(size_t w, size_t h, Method met, const Vector2 &pos, double pathThickness, double wallThickess) {
			caster.Walls().Clear();
			phys.Walls().Clear();
			List<BlockID> bs;
			BlockID ncc;
			for (ncc.Y = 5; ncc.Y < 15; ++ncc.Y) {
				for (ncc.X = 5; ncc.X < 15; ++ncc.X) {
					if (Square(ncc.X - 9.5) + Square(ncc.Y - 9.5) < 25) {
						bs.PushBack(ncc);
					}
				}
			}
			double hwt = wallThickess / 2.0;
			Maze m = MazeGenerator::Generate(w, h, BlockID(0, 0), BlockID(w - 1, h - 1), met, bs);
			m.ClearWallInRegion(bs);
			m.SetWall(BlockID(0, 0), Direction::Left, false);
			m.SetWall(BlockID(w - 1, h - 1), Direction::Right, false);
			BlockID cur(0, 0);
			Direction curd = Direction::Right;
			Vector2 lastPt = Vector2(pos.X - hwt, pos.Y + pathThickness - hwt);
			while (cur.X != 0 || cur.Y != 0 || curd != Direction::Left) {
				Direction nd = TurnRight(curd);
				if (!m.IsWall(cur, nd)) {
					Vector2 thisPt = GetCornerPt(pos, cur, pathThickness, hwt, nd);
					SpawnWall(lastPt, thisPt);
					lastPt = thisPt;
					curd = nd;
					cur = cur.Go(curd);
				} else if (m.IsWall(cur, curd)) {
					Vector2 thisPt = GetCornerPt(pos, cur, pathThickness, hwt, curd);
					SpawnWall(lastPt, thisPt);
					lastPt = thisPt;
					curd = TurnLeft(curd);
				} else {
					cur = cur.Go(curd);
				}
				if (cur.X == m.Width()) {
					curd = Direction::Left;
					Vector2 thisPt = GetCornerPt(pos, BlockID(m.Width(), m.Height() - 1), pathThickness, hwt, Direction::Down);
					SpawnWall(lastPt, thisPt);
					lastPt = GetCornerPt(pos, BlockID(m.Width(), m.Height() - 1), pathThickness, hwt, Direction::Left);
					--cur.X;
				}
			}
			// spawn borders
			SpawnWall(lastPt, Vector2(pos.X - hwt, pos.Y + hwt));
			SpawnWall(Vector2(pos.X - hwt, pos.Y + hwt), Vector2(pos.X - hwt, pos.Y - hwt));
			SpawnWall(
				Vector2(pos.X - hwt, pos.Y - hwt),
				Vector2(pos.X + m.Width() * pathThickness + hwt, pos.Y - hwt));
			SpawnWall(
				Vector2(pos.X + m.Width() * pathThickness + hwt, pos.Y - hwt),
				Vector2(pos.X + m.Width() * pathThickness + hwt, pos.Y + (m.Height() - 1) * pathThickness + hwt));
			SpawnWall(
				Vector2(pos.X + m.Width() * pathThickness + hwt, pos.Y + m.Height() * pathThickness - hwt),
				Vector2(pos.X + m.Width() * pathThickness + hwt, pos.Y + m.Height() * pathThickness + hwt));
			SpawnWall(
				Vector2(pos.X + m.Width() * pathThickness + hwt, pos.Y + m.Height() * pathThickness + hwt),
				Vector2(pos.X - hwt, pos.Y + m.Height() * pathThickness + hwt));
			SpawnWall(
				Vector2(pos.X - hwt, pos.Y + m.Height() * pathThickness + hwt),
				Vector2(pos.X - hwt, pos.Y + pathThickness - hwt));
		}
		void ShuffleCells(double poss = 0.6) {
			caster.Walls().Clear();
			phys.Walls().Clear();
			for (double x = 40.0; x <= 400.0; x += 40.0) {
				for (double y = 40.0; y <= 400.0; y += 40.0) {
					if (random.NextDouble() < poss) {
						SpawnWallBlock(Vector2(x, y));
					}
				}
			}
		}
		void SpawnWallBlock(const Vector2 &pos) {
			Vector2
				tl = pos + Vector2(-10, -10),
				tr = pos + Vector2(10, -10),
				bl = pos + Vector2(-10, 10),
				br = pos + Vector2(10, 10);
			SpawnWall(tl, bl);
			SpawnWall(tl, tr);
			SpawnWall(tr, br);
			SpawnWall(bl, br);
		}
		void SpawnWall(const Vector2 &v1, const Vector2 &v2) {
			LightCaster::Wall lcw;
			lcw.Node1 = v1;
			lcw.Node2 = v2;
			lcw.IsMirror = true;
			caster.Walls().PushBack(lcw);
			CharacterPhysics::Wall cpw;
			cpw.Node1 = v1;
			cpw.Node2 = v2;
			phys.Walls().PushBack(cpw);
		}

		static List<Vertex> CreateCircle(const Vector2 &origin, double r, size_t split, const Color &color) {
			List<Vertex> res;
			Vertex last = Vertex(Vector2(origin.X + r, origin.Y), color);
			for (size_t i = 0; i < split; ++i) {
				double curR = i * 2 * Pi / (double)split;
				res.PushBack(last);
				res.PushBack(last = Vertex(Vector2(origin.X + r * cos(curR), origin.Y + r * sin(curR)), color));
			}
			res.PushBack(last);
			res.PushBack(res.First());
			return res;
		}
};

int main() {
	{
		try {
			ControlTest pl;
//			LightTest pl;
			pl.Run();
		} catch (Exception &e) {
			ShowMessage(e.Message());
		}
	}
	return 0;
}
