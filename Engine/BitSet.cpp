#include "BitSet.h"

namespace DE {
	namespace Core {
		namespace Collections {
			#define DE_BS_BITMASK(X) (1<<(BitsPerChunk - 1 - (X)))
			void BitSet::PushBack(bool v) {
				if (_curB >= BitsPerChunk) {
					_curB = 0;
					Base::PushBack(0);
				}
				if (v) {
					Base::At(Base::Count() - 1) |= DE_BS_BITMASK(_curB);
				}
				++_curB;
			}
			bool BitSet::PopBack() {
				if (Base::Count() == 0) {
					throw InvalidOperationException(_TEXT("the BitSet is empty"));
				}
				unsigned char mask = DE_BS_BITMASK(--_curB);
				bool v = Base::At(Base::Count() - 1) & mask;
				if (_curB == 0) {
					Base::PopBack();
					_curB = BitsPerChunk;
				} else if (v) {
					Base::At(Base::Count() - 1) &= ~mask;
				}
				return v;
			}

			void BitSet::PushBackBits(const void *vsV, size_t bitNum) {
				const ChunkType *vs = (const ChunkType*)vsV, *e = vs + (bitNum >> SizeOffset);
				for (const ChunkType *c = vs; c != e; ++c) {
					PushBackChunk(*c);
				}
				bitNum &= Mask;
				for (size_t i = 0; i < bitNum; ++i) {
					PushBack((*e) & DE_BS_BITMASK(i));
				}
			}
			void BitSet::PopBackBits(void *vsV, size_t bitNum) {
				ChunkType *vs = (ChunkType*)vsV;
				if (bitNum > Count()) {
					throw InvalidOperationException(_TEXT("not enough bits"));
				}
				size_t addi = bitNum & Mask, al = bitNum >> SizeOffset;
				ChunkType *e = vs + al;
				if (addi > 0) {
					(*e) &= (1<<(BitsPerChunk - addi)) - 1;
					for (size_t i = addi; i > 0; --i) {
						if (PopBack()) {
							(*e) |= DE_BS_BITMASK(i - 1);
						}
					}
				}
				--vs;
				for (ChunkType *c = e - 1; c != vs; --c) {
					(*c) = PopBackChunk();
				}
			}
			void BitSet::Subsequence(void *arrV, size_t start, size_t len) const {
				ChunkType *arr = (ChunkType*)arrV;
				BitSet bs = Subsequence(start, len);
				if (len == 0) {
					return;
				}
				if ((len & Mask) == 0) {
					memcpy(arr, *bs, sizeof(ChunkType) * bs.Base::Count());
				} else {
					memcpy(arr, *bs, sizeof(ChunkType) * (bs.Base::Count() - 1));
					ChunkType clrMsk = (1<<(BitsPerChunk - (bs.Count() & Mask))) - 1;
					arr[bs.Base::Count()] &= clrMsk;
					arr[bs.Base::Count()] |= bs.Base::At(bs.Base::Count() - 1);
				}
			}
			BitSet BitSet::Subsequence(size_t start, size_t len) const {
				size_t eid = start + len, c = Count();
				if (start >= c || eid > c) {
					throw OverflowException(_TEXT("index overflow"));
				}
				if (len == 0) {
					return BitSet();
				}
				BitSet res;
				const ChunkType *s = &Base::At(start >> SizeOffset), *e = (&Base::At(0)) + (eid >> SizeOffset);
				if (s == e) {
					size_t te = eid & Mask;
					if (te == 0) {
						te = BitsPerChunk;
					}
					for (size_t n = (start & Mask); n < te; ++n) {
						res.PushBack(GetAt((start & (~Mask)) | n));
					}
				} else {
					if ((start & Mask) == 0) {
						--s;
					} else {
						for (size_t n = (start & Mask); n < BitsPerChunk; ++n) {
							res.PushBack(GetAt((start & (~Mask)) | n));
						}
					}
					for (const ChunkType *cc = s + 1; cc != e; ++cc) {
						res.PushBackChunk(*cc);
					}
					for (size_t n = 0; n < (eid & Mask); ++n) {
						res.PushBack(GetAt((eid & (~Mask)) | n));
					}
				}
				return res;
			}

			bool BitSet::GetAt(size_t id) const {
				if (Base::Count() == 0) {
					throw OverflowException(_TEXT("index overflow"));
				}
				size_t set = id >> SizeOffset, bit = id & Mask;
				if (set >= Base::Count() || (set == Base::Count() - 1 && bit >= _curB)) {
					throw OverflowException(_TEXT("index overflow"));
				}
				return Base::At(set) & DE_BS_BITMASK(bit);
			}
			void BitSet::SetAt(size_t id, bool v) {
				if (Base::Count() == 0) {
					throw OverflowException(_TEXT("index overflow"));
				}
				size_t set = id >> SizeOffset, bit = id & Mask;
				if (set >= Base::Count() || (set == Base::Count() - 1 && bit >= _curB)) {
					throw OverflowException(_TEXT("index overflow"));
				}
				if (v) {
					Base::At(set) |= DE_BS_BITMASK(bit);
				} else {
					Base::At(set) &= ~DE_BS_BITMASK(bit);
				}
			}

			void BitSet::PushBackChunk(ChunkType chunk) {
				if (_curB == BitsPerChunk) {
					Base::PushBack(chunk);
				} else {
					Base::At(Base::Count() - 1) |= (chunk>>_curB);
					Base::PushBack(chunk<<(BitsPerChunk - _curB));
				}
			}
			BitSet::ChunkType BitSet::PopBackChunk() {
				if (_curB == BitsPerChunk) {
					if (Base::Count() == 0) {
                        throw InvalidOperationException(_TEXT("the BitSet is empty"));
					}
                    return Base::PopBack();
				}
				if (Base::Count() < 2) {
					throw InvalidOperationException(_TEXT("not enough bits"));
				}
				ChunkType ret = Base::PopBack()>>(BitsPerChunk - _curB);
				return ret | (Base::At(Base::Count() - 1)<<_curB);
			}
		}
	}
}
