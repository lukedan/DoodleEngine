#include "ControlCollection.h"

#ifdef DEBUG
#	include <fstream>
	using namespace std;
#endif

#include "Panel.h"

namespace DE {
	namespace UI {
		using namespace Core;
		using namespace Core::Collections;

		void ControlCollection::Insert(Control &con) {
			if (con._world != nullptr) {
				throw InvalidOperationException(_TEXT("the control is already a child of another world"));
			}
			if (con._father != nullptr) {
				throw InvalidOperationException(_TEXT("the control is already a child of another panel"));
			}
			con._zIndex = 0;
			con.SetWorld(_world);
			con._father = _father;
			_cons.InsertRight(&con);
			if (_world) {
				con.ResetLayout();
			}
			if (!_father->_disposing) {
				_father->OnChildrenChanged(CollectionChangeInfo<Control*>(&con, ChangeType::Add));
			}
		}
		void ControlCollection::Delete(Control &con) {
			if (con._father != _father) {
				throw InvalidOperationException(_TEXT("the control is not a child of this panel"));
			}
			con.SetWorld(nullptr);
			con._father = nullptr;
			if (!_cons.Delete<Core::EqualityPredicate<Control*>>(&con)) {
				throw InvalidOperationException(_TEXT("control not found in the table"));
			}
			if (_world) {
				if (!_father->_disposing) {
					_father->ResetChildrenLayout();
				}
			}
			if (!_father->_disposing) {
				_father->OnChildrenChanged(CollectionChangeInfo<Control*>(&con, ChangeType::Remove));
			}
		}
		void ControlCollection::SetZIndex(Control &con, int zIndex) {
			if (con._father != _father) {
				throw InvalidOperationException(_TEXT("the control is not a child of this panel"));
			}
			if (!_cons.Delete<Core::EqualityPredicate<Control*>>(&con)) {
				throw InvalidOperationException(_TEXT("control not found in the table"));
			}
			con._zIndex = zIndex;
			_cons.InsertRight(&con);
			if (_world) {
				con.ResetLayout();
			}
			if (!_father->_disposing) {
				_father->OnChildrenChanged(CollectionChangeInfo<Control*>(&con, ChangeType::Modify));
			}
		}
	}
}
