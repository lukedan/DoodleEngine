#pragma once

#include "Math.h"
#include "Vector2.h"
#include "List.h"
#include "UtilsCommon.h"

namespace DE {
	namespace Utils {
		namespace CharacterPhysics {
			struct Wall {
				Core::Math::Vector2 Node1, Node2;
				unsigned Group = 1;

				void JoinGroup(int group) {
					Group |= 1<<group;
				}
				void QuitGroup(int group) {
					Group &= ~(1<<group);
				}
			};
			struct Character {
				Core::Math::Vector2 Position; // the center of the circle
				double Size = 10.0, MoveSpeed = 200.0; // the radius of the circle
				Direction GoingDirection = (Direction)0;
				unsigned Group = 1;

				void JoinGroup(int group) {
					Group |= 1<<group;
				}
				void QuitGroup(int group) {
					Group &= ~(1<<group);
				}
			};

			class Environment { // TODO: terribly incomplete & use customized speed direction
				public:
					constexpr static double DefaultFrequency = 60.0;

					Core::Collections::List<Wall> &Walls() {
						return _walls;
					}
					const Core::Collections::List<Wall> &Walls() const {
						return _walls;
					}

					Core::Collections::List<Character> &Characters() {
						return _chars;
					}
					const Core::Collections::List<Character> &Characters() const {
						return _chars;
					}

					double GetHertz() const {
						return _hz;
					}
					void SetHertz(double hz) {
						_hz = hz;
						_oohz = 1.0 / _hz;
					}
					size_t &Iterations() {
						return _iters;
					}
					const size_t &Iterations() const {
						return _iters;
					}

					void Update(double dt) {
						_intv += dt;
						while (_intv >= _oohz) {
							_intv -= _oohz;
							DoUpdate();
						}
					}
				private:
					Core::Collections::List<Wall> _walls;
					Core::Collections::List<Character> _chars;
					double _hz = DefaultFrequency, _intv, _oohz = 1.0 / DefaultFrequency;
					size_t _iters = 10;

					void DoUpdate() {
						for (size_t i = 0; i < _chars.Count(); ++i) {
							Character &cc = _chars[i];
							unsigned d = (unsigned)cc.GoingDirection;
							double spd = cc.MoveSpeed;
							Core::Math::Vector2 gdir;
							if (d & (unsigned)Direction::Left) {
								gdir.X -= spd;
							}
							if (d & (unsigned)Direction::Right) {
								gdir.X += spd;
							}
							if (d & (unsigned)Direction::Up) {
								gdir.Y -= spd;
							}
							if (d & (unsigned)Direction::Down) {
								gdir.Y += spd;
							}
							if (Core::Math::Abs(gdir.X * gdir.Y) > 0.0) {
								gdir *= Core::Math::OneOverSqrtTwo;
							}
							cc.Position += gdir * _oohz;
						}
						for (size_t i = 0; i < _iters; ++i) {
							for (size_t j = 0; j < _chars.Count(); ++j) {
								Character &cc = _chars[j];
								for (size_t k = j + 1; k < _chars.Count(); ++k) {
									Character &acc = _chars[k];
									if (
										(cc.Group & acc.Group) == 0 ||
										(cc.Position - acc.Position).LengthSquared() >=
										Core::Math::Square(cc.Size + acc.Size)
									) {
										continue;
									}
									Core::Math::Vector2 vdiff = acc.Position - cc.Position;
									double dis = vdiff.Length();
									Core::Math::Vector2
										p1 = cc.Position + vdiff * (cc.Size / dis),
										p2 = acc.Position + (-vdiff) * (acc.Size / dis),
										pmid = (p1 + p2) / 2.0;
									cc.Position = pmid + (cc.Position - p1);
									acc.Position = pmid + (acc.Position - p2); // TODO: just an approximate approach
								}
								Wall *nearest = nullptr;
								double minDist = 0.0;
								for (size_t k = 0; k < _walls.Count(); ++k) {
									Wall &cw = _walls[k];
									if ((cc.Group & cw.Group) == 0) {
										continue;
									}
									double cDist = Core::Math::PointSegmentDistance(cc.Position, cw.Node1, cw.Node2);
									if (nearest == nullptr || cDist < minDist) {
										nearest = &cw;
										minDist = cDist;
									}
								}
								if (nearest && minDist <= cc.Size) {
									Core::Math::Vector2 ccp, wNormal = nearest->Node2 - nearest->Node1;
									wNormal.RotateLeft90();
									if (
										Core::Math::Vector2::Cross(cc.Position - nearest->Node1, wNormal) *
										Core::Math::Vector2::Cross(cc.Position - nearest->Node2, wNormal) <= 0.0
									) {
										ccp = Core::Math::ProjectionY(cc.Position - nearest->Node1, nearest->Node2 - nearest->Node1) + nearest->Node1;
									} else {
										if ((nearest->Node1 - cc.Position).LengthSquared() > (nearest->Node2 - cc.Position).LengthSquared()) {
											ccp = nearest->Node2;
										} else {
											ccp = nearest->Node1;
										}
									}
									Core::Math::Vector2 dir = cc.Position - ccp;
									if (dir.LengthSquared() > 1.0) {
										dir.SetLength(cc.Size);
									}
									cc.Position = ccp + dir;
								}
							}
						}
					}
			};
		}
	}
}
