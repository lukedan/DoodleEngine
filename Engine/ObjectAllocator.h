#pragma once

#include <new>
#include <cstddef>
#include <cstdlib>
#include <cmath>
#include <tchar.h>

#include <map>

#include "Common.h"

namespace DE {
	namespace Core {
		class ObjectAllocator {
			public:
				ObjectAllocator(
					size_t chunkSize = DefaultChunkSize,
					size_t minSize = DefaultMinimumSize,
					size_t zoomLevel = DefaultZoomLevel,
					double zoom = DefaultZoom
				);
				ObjectAllocator(const ObjectAllocator&) = delete;
				ObjectAllocator &operator =(const ObjectAllocator&) = delete;
				~ObjectAllocator();

				void *Allocate(size_t);
				void *Allocate(size_t, size_t&);
				void Free(void*);

				size_t AllocatedSize() const {
					return _totAlloc;
				}
				size_t UsedSize() const {
					return _totUse;
				}

				void Dump(const char*) const;
				void DumpAsText(const char*, size_t = 30) const;

				constexpr static size_t
					DefaultChunkSize = (1<<16), // 65536
					DefaultMinimumSize = 16,
					DefaultZoomLevel = 10; // 8192
				constexpr static double DefaultZoom = 2.0;
			private:
				struct ChunkData;
				struct LevelData {
					size_t _allocSize, _blockSize, _blockNum, _remains = 0, _level;
					ChunkData *_firstChunk = nullptr;
					void *_recycle = nullptr, *_remain = nullptr;
				};
				struct ChunkData {
					ChunkData *_next = nullptr, *_prev = nullptr;
					size_t _allocedSlotNum = 0;
					LevelData *_data;
				};
				struct Ptr3 {
					void *_pre, *_pos, *_post;
				};
				struct SizeTPtr2 {
					size_t _size;
					void *_pre;
				};

				LevelData *_levels;
				size_t _levelNum, _totUse = 0, _totAlloc = 0;
				/****** MEMORY ARRANGEMENT ******
				 *				+-------------------------+------------------------------------------------------------------+
				 *				|1     sizeof(void*)      |2                  sizeof(_levelBlockSizes[i])                    |
				 *				|                         |3     sizeof(void*)      |4        sizeof(void*)         |5 other |
				 *  usage:		|-------------------------+-------------------------+-------------------------------+--------|
				 *    recycled:	|         nullptr         | points to the chunkData | points to next recycled chunk |  none  |
				 *    remain:	|         nullptr         | points to the chunkData |                  none                  |
				 *    using:	| points to the chunkData |                               using                              |
				 *				+-------------------------+------------------------------------------------------------------+
				 ********************************/
		};
		class GlobalAllocator {
			public:
                static void *Allocate(size_t sz) {
                	return GetAlloc().Allocate(sz);
                }
                static void *Allocate(size_t sz, size_t &actualSz) {
                	return GetAlloc().Allocate(sz, actualSz);
                }
                static void Free(void *ptr) {
					GetAlloc().Free(ptr);
                }

                static size_t UsedSize() {
                	return GetAlloc().UsedSize();
                }
                static size_t AllocatedSize() {
                	return GetAlloc().AllocatedSize();
                }

                static void Dump(const char *fileName) {
                	GetAlloc().Dump(fileName);
                }
                static void DumpAsText(const char *fileName) {
                	GetAlloc().DumpAsText(fileName);
                }
			private:
				static ObjectAllocator &GetAlloc();
		};
//		class GlobalAllocator { // for memory test
//			public:
//				static void *Allocate(size_t sz) {
//					void *mem = malloc(sz);
//					_map[mem] = sz;
//					_sztot += sz;
//					ShowMessage(_TEXT("ALLOCATED MEMORY AT 0x%p\n"), mem);
//					return mem;
//				}
//				static void *Allocate(size_t sz, size_t &actualSz) {
//					actualSz = sz;
//					void *mem = malloc(sz);
//					_map[mem] = sz;
//					_sztot += sz;
//					ShowMessage(_TEXT("ALLOCATED MEMORY AT 0x%p\n"), mem);
//					return mem;
//				}
//				static void Free(void *ptr) {
//					if (_map[ptr] == 0) {
//						throw InvalidOperationException(_TEXT("trying to free memory that wasn't allocated by GlobalAllocator"));
//						_map.erase(ptr);
//					}
//					_sztot -= _map[ptr];
//					_map.erase(ptr);
//					ShowMessage(_TEXT("FREED MEMORY AT 0x%p\n"), ptr);
//					free(ptr);
//				}
//
//				static size_t UsedSize() {
//					return _sztot;
//				}
//				static size_t AllocatedSize() {
//					return _sztot;
//				}
//
//				static void Dump(const TCHAR *fileName) {
//					//_allocator.Dump(fileName);
//				}
//				static void DumpAsText(const char *fileName) {
//					//_allocator.DumpAsText(fileName);
//				}
//			private:
//				static std::map<void*, size_t> _map;
//				static size_t _sztot;
//		};
	}
}