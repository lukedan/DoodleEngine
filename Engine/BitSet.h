#pragma once

#include "Math.h"
#include "List.h"

namespace DE {
	namespace Core {
		namespace Collections {
			class BitSet : protected List<unsigned char, false> {
				public:
					typedef unsigned char ChunkType;
					constexpr static size_t SizeOffset = 3, BitsPerChunk = 1 << SizeOffset, Mask = BitsPerChunk - 1;

					void PushBack(bool);
					void PushBackBits(const BitSet &rhs) {
						PushBackBits(*rhs, rhs.Count());
					}
					void PushBackBits(const void*, size_t);

					bool PopBack();
					BitSet PopBackBits(size_t len) {
						size_t csz = ((len + BitsPerChunk - 1) >> SizeOffset) * sizeof(ChunkType);
						ChunkType *arr = (ChunkType*)GlobalAllocator::Allocate(csz);
						memset(arr, 0, csz);
						PopBackBits(arr, len);
						BitSet ret;
						ret.PushBackBits(arr, len);
						GlobalAllocator::Free(arr);
						return ret;
					}
					void PopBackBits(void*, size_t);

					void Subsequence(void*, size_t, size_t) const;
					BitSet Subsequence(size_t, size_t) const;

					bool GetAt(size_t) const;
					void SetAt(size_t, bool);
					void Clear() {
						Base::Clear();
						_curB = BitsPerChunk;
					}

					using List<unsigned char, false>::operator*;

					size_t Count() const {
						return (Base::Count() << SizeOffset) - (BitsPerChunk - _curB);
					}
					size_t ChunkCount() const {
						return Base::Count();
					}
					size_t Capicy() const {
						return Base::Capicy() << SizeOffset;
					}
				private:
					typedef List<unsigned char, false> Base;

					void PushBackChunk(ChunkType);
					ChunkType PopBackChunk();

					size_t _curB = BitsPerChunk;
			};
		}
	}
}
