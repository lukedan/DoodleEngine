#include "MazeGenerator.h"


namespace DE {
	using namespace Core;
	using namespace Core::Collections;
	namespace Utils {
		namespace MazeGenerator {
			Maze Generate(
				size_t w, size_t h,
				BlockID start, BlockID finish, Method met,
				const List<BlockID> &disabled
			) {
				struct StepData {
					StepData() = default;
					StepData(BlockID fid, BlockID tid, Direction dir) : _fid(fid), _tid(tid), _dir(dir) {
					}

					BlockID _fid, _tid;
					Direction _dir = Direction::Up;
				};

				Maze finMz(w, h);
				Random r;
				List<BlockID> q;
				BitSet bs;
				List<Direction> path(Direction::Up, w * h);
				size_t tsz = w * h;
				unsigned char c = 0;
				for (size_t i = 0; i < (tsz>>3); ++i) {
					bs.PushBackBits(&c, (1<<3));
				}
				for (size_t i = 0; i < (tsz & BitSet::Mask); ++i) {
					bs.PushBack(false);
				}
				for (size_t i = 0; i < disabled.Count(); ++i) {
					BlockID id = disabled[i];
					if (!finMz.IsValidBlockID(id)) {
						throw OverflowException(_TEXT("index overflow"));
					}
					bs.SetAt(id.X + id.Y * w, true);
				}
				q.PushBack(start);
				bs.SetAt(start.X + start.Y * w, true);
				bool haveWay = false;
				while (q.Count() > 0) {
					List<StepData> avl;
					if (met == Method::BFS) {
						q.SwapToBack(r.NextBetween(0, q.Count()));
					}
					BlockID cur = q.Last();
					for (unsigned i = 1; i < (1<<5); i <<= 1) {
						if (finMz.CanMove(cur, (Direction)i)) {
							BlockID moved = cur.Go((Direction)i);
							if (moved == finish) {
								haveWay = true;
							}
							if (!bs.GetAt(moved.X + moved.Y * w)) {
								avl.PushBack(StepData(cur, moved, (Direction)i));
							}
						}
					}
					if (avl.Count() > 0) {
						StepData tar = avl[r.NextBetween(0, avl.Count())];
						size_t bid = tar._tid.X + tar._tid.Y * w;
						bs.SetAt(bid, true);
						path[bid] = tar._dir;
						finMz.SetWall(tar._fid, tar._dir, false);
						q.PushBack(tar._tid);
					} else {
						q.PopBack();
					}
				}
				if (haveWay) {
					for (BlockID cid = finish; cid != start; ) {
						Direction d = path[cid.Y * w + cid.X];
						finMz._rt.PushBack(d);
						cid = cid.Go(Reverse(d));
					}
					for (size_t i = 0, j = finMz._rt.Count() - 1; i < j; ++i, --j) {
						finMz._rt.Swap(i, j);
					}
				}
				return finMz;
			}
			Maze Generate(size_t w, size_t h, BlockID start, BlockID finish, Method met) {
				return Generate(w, h, start, finish, met, List<BlockID>());
			}
		}
	}
}
