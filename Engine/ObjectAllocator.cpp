#include <cstring>

#include "ObjectAllocator.h"
#include "Common.h"
#include "Math.h"

namespace DE {
	namespace Core {
		using namespace Math;

		int allocn = 0;

		ObjectAllocator::ObjectAllocator(size_t chunkSize, size_t minSize, size_t zoomLevel, double zoom) :
			_levels(new LevelData[zoomLevel]), _levelNum(zoomLevel)
		{
			if (!_levels) {
				throw SystemException(_TEXT("cannot allocate memory"));
			}
			if (minSize < (sizeof(void*)<<1)) {
				throw UnderflowException(_TEXT("the size of the memory chunk is too small"));
			}
			if (zoom < 1.0) {
				throw InvalidArgumentException(_TEXT("zoom cannot be smaller than 1"));
			}

			double curSize = minSize;
			for (size_t i = 0; i < zoomLevel; ++i, curSize *= zoom) {
				LevelData &curData = _levels[i];
				curData._level = i;
				curData._blockSize = (size_t)curSize;
				curData._blockNum =
					(size_t)ceil((chunkSize - sizeof(ChunkData)) / (curData._blockSize + sizeof(void*)));
				curData._allocSize =
					sizeof(ChunkData) + curData._blockNum * (curData._blockSize + sizeof(void*));
			}
		}
		ObjectAllocator::~ObjectAllocator() {
#ifdef DEBUG
			if (_totUse > 0) {
				DumpAsText("dump.txt");
			}
#endif
			for (size_t lvl = 0; lvl < _levelNum; ++lvl) {
				for (ChunkData *curData = _levels[lvl]._firstChunk, *nextData = nullptr; curData; curData = nextData) {
					nextData = curData->_next;
					free(curData);
				}
			}
			free(_levels);
			if (_totUse > 0) {
				throw InvalidOperationException(_TEXT("some memory was not freed"));
			}
		}

		void *ObjectAllocator::Allocate(size_t targetSize) {
			size_t mxSz;
			return Allocate(targetSize, mxSz);
		}
		void *ObjectAllocator::Allocate(size_t targetSize, size_t &maxSize) {
			size_t tarLvl = 0;
			for (; tarLvl < _levelNum && _levels[tarLvl]._blockSize < targetSize; ++tarLvl) {
			}
			++allocn;
			if (tarLvl >= _levelNum) { // allocate from system memory
				maxSize = targetSize;
				size_t sz = sizeof(SizeTPtr2) + targetSize;
				SizeTPtr2 *mem = static_cast<SizeTPtr2*>(malloc(sz));
				if (!mem) {
					throw SystemException(_TEXT("cannot allocate memory"));
				}
				_totUse += sz;
				_totAlloc += sz;
				mem->_size = sz;
				mem->_pre = this;
				return mem + 1;
			}
			LevelData &lvlData = _levels[tarLvl];
			maxSize = lvlData._blockSize;
			size_t allocSz = lvlData._blockSize + sizeof(void*);
			_totUse += allocSz;
			if (lvlData._recycle) { // recycled memory
				Ptr3 &p3 = *static_cast<Ptr3*>(lvlData._recycle);
				lvlData._recycle = p3._post;
				p3._pre = p3._pos;
				++(static_cast<ChunkData*>(p3._pre)->_allocedSlotNum);
				return &p3._pos;
			}
			if (lvlData._remains == 0) { // allocate new chunk
				ChunkData *data = new (malloc(lvlData._allocSize)) ChunkData();
				if (!data) {
					throw SystemException(_TEXT("cannot allocate memory"));
				}
				_totAlloc += lvlData._allocSize;
				memset(data + 1, 0, lvlData._allocSize - sizeof(ChunkData));
				data->_next = lvlData._firstChunk;
				data->_data = &lvlData;
				lvlData._firstChunk = data;
				if (data->_next) {
					data->_next->_prev = data;
				}
				lvlData._remains = lvlData._blockNum;
				Ptr3 &p3 = *reinterpret_cast<Ptr3*>(data + 1);
				lvlData._remain = data + 1;
				p3._pos = data;
			}
			Ptr3 &p3 = *static_cast<Ptr3*>(lvlData._remain);
			--(lvlData._remains);
			lvlData._remain = (void***)((size_t)lvlData._remain + allocSz);
			p3._pre = p3._pos;
			++(static_cast<ChunkData*>(p3._pre)->_allocedSlotNum);
			if (lvlData._remains > 0) {
				Ptr3 &ap3 = *static_cast<Ptr3*>(lvlData._remain);
				ap3._pos = p3._pre;
			}
			return &p3._pos;
		}
		void ObjectAllocator::Free(void *ptr) {
			if (ptr == nullptr) {
				return;
			}
			Ptr3 &p3 = *(Ptr3*)((size_t)ptr - sizeof(void*));
			--allocn;
			if (p3._pre == nullptr) {
				throw InvalidArgumentException(_TEXT("the memory was freed or wasn't allocated from this allocator"));
			}
			if (p3._pre == this) { // allocated from system memory
				SizeTPtr2 &p2 = *(SizeTPtr2*)(((size_t)ptr) - sizeof(SizeTPtr2));
				_totUse -= p2._size;
				_totAlloc -= p2._size;
				free(&p2);
				return;
			}
			ChunkData &data = *static_cast<ChunkData*>(p3._pre); // link it to recycle
			size_t allocSz = data._data->_blockSize + sizeof(void*);
			_totUse -= allocSz;
			p3._pos = p3._pre;
			p3._pre = nullptr;
			p3._post = data._data->_recycle;
			data._data->_recycle = &p3;
			--(data._allocedSlotNum);
		}

		void ObjectAllocator::Dump(const char *fileName) const {
			FILE *out = fopen(fileName, "wb");
			for (size_t i = 0; i < _levelNum; ++i) {
				for (const ChunkData *cd = _levels[i]._firstChunk; cd; cd = cd->_next) {
					fwrite(cd, _levels[i]._allocSize, 1, out);
				}
			}
			fclose(out);
		}
		void ObjectAllocator::DumpAsText(const char *fileName, size_t lineLen) const {
			FILE *out = fopen(fileName, "w");
			for (size_t i = 0; i < _levelNum; ++i) {
				fprintf(out, "LEVEL %u\n", _levels[i]._level);
				for (const ChunkData *cd = _levels[i]._firstChunk; cd; cd = cd->_next) {
					fprintf(
						out, "  CHUNK 0x%p\n    ALLOCATED SLOTS = %u\n    PREV = 0x%p\n    NEXT = 0x%p\n",
						(void*)cd, cd->_allocedSlotNum, (void*)cd->_prev, (void*)cd->_next
					);
					const Ptr3 *p3 = (const Ptr3*)(cd + 1);
					for (size_t j = 0; j < _levels[i]._blockNum; ++j, p3 = (const Ptr3*)((size_t)p3 + sizeof(void*) + _levels[i]._blockSize)) {
						if (p3->_pre) {
							fprintf(out, "      ALLOCATED 0x%p\n        PRE=0x%p\n         ", &(p3->_pos), p3->_pre);
						} else {
							fprintf(out, "      UNALLOCED 0x%p\n        POS=0x%p\n        POST=0x%p\n         ", &(p3->_pos), p3->_pos, p3->_post);
						}
						const unsigned char *arr = reinterpret_cast<const unsigned char *const>(&(p3->_pos));

						char c[3] = { '0', '0', '\0' };
						size_t alr = 0, lst = 0;
						for (size_t csz = 0; csz < _levels[i]._blockSize; ++csz, ++alr) {
							if (alr == lineLen) {
								fprintf(out, "  ");
								for (size_t tPos = lst; tPos < csz; ++tPos) {
									char cc = arr[tPos];
									if (cc >= ' ' && cc <= '~') {
										fprintf(out, "%c", cc);
									} else {
										fprintf(out, ".");
									}
								}
								alr = 0;
								lst = csz;
								fprintf(out, "\n         ");
							}
							unsigned char cur = arr[csz];
							c[0] = (cur>>4) + '0';
							c[1] = (cur & 0xF) + '0';
							if (c[0] > '9') {
								c[0] = c[0] - '9' + 'A' - 1;
							}
							if (c[1] > '9') {
								c[1] = c[1] - '9' + 'A' - 1;
							}
							fprintf(out, " %s", c);
						}
						for (size_t tPos = alr; tPos < lineLen; ++tPos) {
							fprintf(out, "   ");
						}
						fprintf(out, "  ");
						for (size_t tPos = lst; tPos < _levels[i]._blockSize; ++tPos) {
							char cc = arr[tPos];
							if (cc >= ' ' && cc <= '~') {
								fprintf(out, "%c", cc);
							} else {
								fprintf(out, ".");
							}
						}

						fprintf(out, "\n");
					}
					fprintf(out, "\n");
				}
				fprintf(out, "\n");
			}
			fclose(out);
		}

		ObjectAllocator &GlobalAllocator::GetAlloc() {
			static ObjectAllocator _alloc;
			return _alloc;
		}
//		std::map<void*, size_t> GlobalAllocator::_map;
//		size_t GlobalAllocator::_sztot = 0;
	}
}
