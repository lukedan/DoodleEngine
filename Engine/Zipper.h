#pragma once

#include "Math.h"
#include "BitSet.h"
#include "PriorityQueue.h"

namespace DE {
	namespace IO {
		class Unzipper;
		class Zipper {
				friend class Unzipper;
			public:
				Core::Collections::List<unsigned char> &Data() {
					return _data;
				}
				const Core::Collections::List<unsigned char> &Data() const {
					return _data;
				}
				void SetData(const void *data, size_t size) {
					_data.Clear();
					_data.PushBackRange((const unsigned char*)data, size);
				}

				Core::Collections::BitSet Zip() const;
			private:
				struct Node {
					Node *_left = nullptr, *_right = nullptr, *_father = nullptr;
					unsigned char _val;
					size_t _occ = 0;
					Core::Collections::BitSet _code;

					explicit Node(unsigned char v) : _val(v) {
					}

					static void FreeTree(Node*);
					static Node *Combine(Node*, Node*);
					static void GenCodeForTree(Node*);
					static void Insert(Node*, Node*);
				};
				class NodeComparer {
					public:
						static int Compare(const Node *lhs, const Node *rhs) {
							return Core::DefaultComparer<size_t>::Compare(rhs->_occ, lhs->_occ);
						}
				};

				Core::Collections::List<unsigned char> _data;
		};
		class Unzipper {
			public:
				Core::Collections::BitSet &Data() {
					return _data;
				}
				const Core::Collections::BitSet &Data() const {
					return _data;
				}
				void SetData(const void *data, size_t sizeInBytes) {
					_data.Clear();
					_data.PushBackBits(data, sizeInBytes<<3);
				}

				Core::Collections::List<unsigned char> Unzip() const;
			private:
				typedef Zipper::Node Node;

				Core::Collections::BitSet _data;
		};
	}
}
