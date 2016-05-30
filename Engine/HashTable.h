#pragma once

#include "List.h"

namespace DE {
	namespace Core {
		namespace Collections { // TODO not finished
			template <typename T> class DirectHashFunc {
				public:
					inline static size_t Hash(const T &obj) {
						return static_cast<size_t>(obj);
					}
			};
			template <typename T> class AddressHashFunc {
				public:
					inline static size_t Hash(const T &obj) {
						return static_cast<size_t>(&obj);
					}
			};
			template <typename T, typename HashFunc> class HashTable {
				public:
					HashTable() {

					}

					bool Exists(const T &obj) {
						HashNode hn(obj);
					}
				protected:
					struct HashNode {
						HashNode(const T &obj, size_t slotn) : Obj(obj), Slot(HashFunc::Hash(obj) % slotn) {
						}

						T Obj;
						size_t Slot = 0;
						HashNode *Next = nullptr, *CPrev = nullptr, *CNext = nullptr;
					};
			};
		}
	}
}
