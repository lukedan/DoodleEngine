#pragma once

#include "Math.h"
#include "List.h"

namespace DE {
	namespace Core {
		namespace Collections {
			template <typename T, bool DirectMemoryAccess = !IsClass<T>::Result, class Comparer = DefaultComparer<T>> class PriorityQueue :
				protected List<T, DirectMemoryAccess>
			{
				public:
					void Insert(const T &obj) {
						Base::PushBack(obj);
						AdjustUp(Count() - 1);
					}
					using List<T, DirectMemoryAccess>::FindFirst;
					using List<T, DirectMemoryAccess>::FindLast;
					void ChangeKey(size_t index, const T &newV) {
						int x = Comparer::Compare(Base::At(index), newV);
						Base::At(index) = newV;
						if (x > 0) {
							AdjustDown(index, Count());
						} else if (x < 0) {
							AdjustUp(index);
						}
					}
					void Remove(size_t index) {
						Base::SwapToBack(index);
						Base::PopBack();
						AdjustDown(index, Count());
					}
					using List<T, DirectMemoryAccess>::Clear;

					const T &Max() const {
						return Base::First();
					}
					T ExtractMax() {
						Base::SwapToBack(0);
						AdjustDown(0, Base::Count() - 1);
						return Base::PopBack();
					}

					using List<T, DirectMemoryAccess>::Count;
					using List<T, DirectMemoryAccess>::Capicy;
					using List<T, DirectMemoryAccess>::MinCapicy;
				protected:
					void AdjustDown(size_t index, size_t realCount) {
						for (size_t maxi, t; index < realCount; index = maxi) {
							maxi = index;
							t = (index<<1) + 1;
							if (t < realCount && Comparer::Compare(Base::At(maxi), Base::At(t)) < 0) {
								maxi = t;
							}
							t = (index + 1)<<1;
							if (t < realCount && Comparer::Compare(Base::At(maxi), Base::At(t)) < 0) {
								maxi = t;
							}
							if (maxi == index) {
								break;
							}
							Base::Swap(maxi, index);
						}
					}
					void AdjustUp(size_t index) {
						for (
							size_t t = (index - 1)>>1;
							index > 0 && Comparer::Compare(Base::At(index), Base::At(t)) > 0;
							index = t, t = (t - 1)>>1
						) {
							Base::Swap(index, t);
						}
					}
				private:
					typedef List<T, DirectMemoryAccess> Base;
			};
		}
	}
}
