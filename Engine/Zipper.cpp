#include "Zipper.h"

namespace DE {
	namespace IO {
		using namespace Core;
		using namespace Core::Collections;

		void Zipper::Node::FreeTree(Node *n) {
			List<Node*> stk;
			stk.PushBack(n);
			while (stk.Count() > 0) {
				n = stk.PopBack();
				if (n->_left) {
					stk.PushBack(n->_left);
				}
				if (n->_right) {
					stk.PushBack(n->_right);
				}
				n->~Node();
				GlobalAllocator::Free(n);
			}
		}
		Zipper::Node *Zipper::Node::Combine(Node *lhs, Node *rhs) {
			Node *n = new (GlobalAllocator::Allocate(sizeof(Node))) Node(0);
			n->_occ = lhs->_occ + rhs->_occ;
			lhs->_father = rhs->_father = n;
			n->_left = lhs;
			n->_right = rhs;
			return n;
		}
		void Zipper::Node::GenCodeForTree(Node *n) {
            List<Node*> stk;
            n->_code.Clear();
            if (n->_left) {
				stk.PushBack(n->_left);
            }
            if (n->_right) {
				stk.PushBack(n->_right);
            }
            while (stk.Count() > 0) {
				n = stk.PopBack();
				n->_code = n->_father->_code;
				n->_code.PushBack(n == n->_father->_right);
				if (n->_left) {
					stk.PushBack(n->_left);
				}
				if (n->_right) {
					stk.PushBack(n->_right);
				}
            }
		}
		void Zipper::Node::Insert(Node *root, Node *target) {
			Node **tar = &root, *father;
			size_t tcnt = target->_code.Count() - 1;
            for (size_t i = 0; i < tcnt; ++i) {
				father = (*tar);
				if (target->_code.GetAt(i)) {
					tar = &((*tar)->_right);
				} else {
					tar = &((*tar)->_left);
				}
				if (!(*tar)) {
					(*tar) = new (GlobalAllocator::Allocate(sizeof(Node))) Node(0);
					(*tar)->_father = father;
				}
            }
            if (target->_code.GetAt(tcnt)) {
				(*tar)->_right = target;
            } else {
            	(*tar)->_left = target;
            }
		}

		BitSet Zipper::Zip() const {
			Node *ns[256];
			size_t has = 256;
			for (size_t i = 0; i < 256; ++i) { // allocate nodes
				ns[i] = new (GlobalAllocator::Allocate(sizeof(Node))) Node(i);
			}
			for (size_t i = 0; i < _data.Count(); ++i) { // frequency of each key
                ++(ns[_data[i]]->_occ);
			}
			PriorityQueue<Node*, true, NodeComparer> q;
			for (size_t i = 0; i < 256; ++i) {
				if (ns[i]->_occ > 0) { // push it into the priority queue
					q.Insert(ns[i]);
				} else { // the node is of no use
					--has;
					ns[i]->~Node();
					GlobalAllocator::Free(ns[i]);
					ns[i] = nullptr;
				}
			}
			while (q.Count() > 1) { // build the tree
				q.Insert(Node::Combine(q.ExtractMax(), q.ExtractMax()));
			}
			Node *root = q.ExtractMax();
			Node::GenCodeForTree(root);

			// generate code
			BitSet ret;
			ret.PushBackBits(&has, sizeof(size_t) << 3); // available nodes
			for (size_t i = 0; i < 256; ++i) {
				if (ns[i]) {
					ret.PushBackBits(&(ns[i]->_val), sizeof(unsigned char) << 3); // value of node
					unsigned char x = ns[i]->_code.Count();
					ret.PushBackBits(&x, sizeof(unsigned char) << 3); // length of code
					ret.PushBackBits(ns[i]->_code); // code of node
				}
			}
			size_t len = _data.Count();
			ret.PushBackBits(&len, sizeof(size_t) << 3);
			for (size_t i = 0; i < len; ++i) { // data
				ret.PushBackBits(ns[_data[i]]->_code);
			}
			Node::FreeTree(root);
			return ret;
		}
		List<unsigned char> Unzipper::Unzip() const {
			size_t availN, pos = 0;
			_data.Subsequence(&availN, pos, sizeof(size_t) << 3); // node count
			pos += sizeof(size_t) << 3;
			List<Node*> ns;
			for (size_t i = 0; i < availN; ++i) { // read all nodes
				unsigned char len;
				Node *n = new (GlobalAllocator::Allocate(sizeof(Node))) Node(0);
				_data.Subsequence(&(n->_val), pos, sizeof(unsigned char) << 3);
				pos += sizeof(unsigned char) << 3;
				_data.Subsequence(&len, pos, sizeof(unsigned char) << 3);
				pos += sizeof(unsigned char) << 3;
				n->_code = _data.Subsequence(pos, len);
				pos += len;
				ns.PushBack(n);
			}
			// build the tree
			Node *root = new (GlobalAllocator::Allocate(sizeof(Node))) Node(0);
			for (size_t i = 0; i < ns.Count(); ++i) {
				Node::Insert(root, ns[i]);
			}
			size_t len;
			_data.Subsequence(&len, pos, sizeof(size_t) << 3); // length of file
			pos += sizeof(size_t) << 3;
			// read the file and convert
			List<unsigned char> ret;
			for (size_t i = 0; i < len; ++i) {
				Node *cur = root;
                while (cur->_left || cur->_right) {
					if (_data.GetAt(pos)) {
						cur = cur->_right;
					} else {
						cur = cur->_left;
					}
					++pos;
                }
                ret.PushBack(cur->_val);
			}
			Node::FreeTree(root);
			return ret;
		}
	}
}
