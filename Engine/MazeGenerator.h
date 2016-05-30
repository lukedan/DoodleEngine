#pragma once

#include "List.h"
#include "BitSet.h"
#include "Random.h"
#include "ReferenceCounter.h"
#include "UtilsCommon.h"

namespace DE {
	namespace Utils {
		namespace MazeGenerator {
			class Maze;
			struct BlockID;

			enum class Method {
				BFS,
				DFS
			};
			Maze Generate(size_t, size_t, BlockID, BlockID, Method, const Core::Collections::List<BlockID>&);
			Maze Generate(size_t, size_t, BlockID, BlockID, Method);

			struct Block {
				Block() = default;
				Block(bool l, bool u, bool r, bool d) : Left(l), Up(u), Right(r), Down(d) {
				}

				bool Left = true, Up = true, Right = true, Down = true;
				bool Get(Direction dir) {
					switch (dir) {
						case Direction::Left: {
							return Left;
						}
						case Direction::Right: {
							return Right;
						}
						case Direction::Up: {
							return Up;
						}
						case Direction::Down: {
							return Down;
						}
					}
					return false;
				}
			};
			struct BlockID {
				BlockID() = default;
				BlockID(size_t x, size_t y) : X(x), Y(y) {
				}

				BlockID Left() const {
					if (X == 0) {
						throw Core::UnderflowException(_TEXT("index underflow"));
					}
					return BlockID(X - 1, Y);
				}
				BlockID Right() const {
					return BlockID(X + 1, Y);
				}
				BlockID Up() const {
					if (Y == 0) {
						throw Core::UnderflowException(_TEXT("index underflow"));
					}
					return BlockID(X, Y - 1);
				}
				BlockID Down() const {
					return BlockID(X, Y + 1);
				}
				BlockID Go(Direction dir) const {
					switch (dir) {
						case Direction::Left: {
							return Left();
						}
						case Direction::Right: {
							return Right();
						}
						case Direction::Up: {
							return Up();
						}
						case Direction::Down: {
							return Down();
						}
					}
					return *this;
				}

				friend bool operator ==(const BlockID &lhs, const BlockID &rhs) {
					return lhs.X == rhs.X && lhs.Y == rhs.Y;
				}
				friend bool operator !=(const BlockID &lhs, const BlockID &rhs) {
					return lhs.X != rhs.X || lhs.Y != rhs.Y;
				}

				size_t X = 0, Y = 0;
			};
			class Maze {
					friend Maze Generate(size_t, size_t, BlockID, BlockID, Method, const Core::Collections::List<BlockID>&);
				public:
					Maze(const Maze&) = default;
					Maze &operator =(const Maze&) = default;

					bool IsWall(BlockID id, Direction dir) const {
						size_t rid = (_w + 1) * id.Y + id.X;
						switch (dir) {
							case Direction::Left: {
								return _l.GetAt(rid);
							}
							case Direction::Right: {
								return _l.GetAt(rid + 1);
							}
							case Direction::Up: {
								return _t.GetAt(rid);
							}
							case Direction::Down: {
								return _t.GetAt(rid + 1 + _w);
							}
						}
						return false;
					}
					void SetWall(BlockID id, Direction dir, bool wall) {
						size_t rid = (_w + 1) * id.Y + id.X;
						switch (dir) {
							case Direction::Left: {
								_l.SetAt(rid, wall);
								break;
							}
							case Direction::Right: {
								_l.SetAt(rid + 1, wall);
								break;
							}
							case Direction::Up: {
								_t.SetAt(rid, wall);
								break;
							}
							case Direction::Down: {
								_t.SetAt(rid + 1 + _w, wall);
								break;
							}
						}
					}
					Block GetAt(BlockID id) const {
						Block bk;
						size_t bid = (_w + 1) * id.Y + id.X;
						bk.Left = _l.GetAt(bid);
						bk.Up = _t.GetAt(bid);
						bk.Right = _l.GetAt(++bid);
						bk.Down = _t.GetAt(bid + _w);
						return bk;
					}
					void ClearWallInRegion(const Core::Collections::List<BlockID> &region) {
						Core::Collections::BitSet bs;
						unsigned char x = 0;
						size_t tarnum = _w * _h;
						for (size_t i = 0; i < (tarnum>>3); ++i) {
							bs.PushBackBits(&x, (1<<3));
						}
						for (size_t i = 0; i < (tarnum & Core::Collections::BitSet::Mask); ++i) {
							bs.PushBack(false);
						}
						for (size_t i = 0; i < region.Count(); ++i) {
							BlockID cid = region[i];
							if (!IsValidBlockID(cid)) {
								throw Core::OverflowException(_TEXT("index overflow"));
							}
							bs.SetAt(cid.X + cid.Y * _w, true);
						}
						for (size_t i = 0; i < region.Count(); ++i) {
							BlockID cid = region[i];
							for (unsigned dir = 1; dir < 0x10; dir <<= 1) {
								Direction dd = (Direction)dir;
								if (CanMove(cid, dd)) {
									BlockID nid = cid.Go(dd);
									if (bs.GetAt(nid.X + nid.Y * _w)) {
										SetWall(cid, dd, false);
									}
								}
							}
						}
					}
					void ForEach(const std::function<void(BlockID, Block)> &op) {
						if (op) {
							BlockID id;
							for (id.Y = 0; id.Y < _h; ++id.Y) {
								for (id.X = 0; id.X < _w; ++id.X) {
									op(id, GetAt(id));
								}
							}
						}
					}
					bool IsValidBlockID(BlockID id) const {
						return id.X < _w && id.Y < _h;
					}
					bool CanPass() const {
						return _rt.Count() > 0;
					}

					const Core::Collections::List<Direction> &GetRoute() const {
						return _rt;
					}
					size_t Width() const {
						return _w;
					}
					size_t Height() const {
						return _h;
					}
				private:
					Maze(size_t w, size_t h) : _w(w), _h(h) {
						if (w == 0 || h == 0) {
							throw Core::InvalidArgumentException(_TEXT("invalid maze size"));
						}
						size_t x = (w + 1) * (h + 1);
						unsigned char n = 0xFF;
						for (size_t i = 0; i < (x>>3); ++i) {
							_l.PushBackBits(&n, (1<<3));
							_t.PushBackBits(&n, (1<<3));
						}
						for (size_t i = 0; i < (x & Core::Collections::BitSet::Mask); ++i) {
							_l.PushBack(true);
							_t.PushBack(true);
						}
					}

					size_t _w = 0, _h = 0;
					Core::Collections::BitSet _l, _t;
					Core::Collections::List<Direction> _rt;

					bool CanMove(BlockID id, Direction d) const {
						switch (d) {
							case Direction::Left: {
								return id.X > 0;
							}
							case Direction::Right: {
								return id.X < _w - 1;
							}
							case Direction::Down: {
								return id.Y < _h - 1;
							}
							case Direction::Up: {
								return id.Y > 0;
							}
						}
						return false;
					}
			};
		}
	}
}