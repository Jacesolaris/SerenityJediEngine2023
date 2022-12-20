/*
===========================================================================
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

////////////////////////////////////////////////////////////////////////////////////////
// RAVEN STANDARD TEMPLATE LIBRARY
//  (c) 2002 Activision
//
//
// Heap
// ------
//
//
//
//
// TODO:
//
//
// NOTES:
//
//
////////////////////////////////////////////////////////////////////////////////////////
#if !defined(RATL_HEAP_VS_INC)
#define RATL_HEAP_VS_INC

////////////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////////////
#if !defined(RATL_COMMON_INC)
#include "ratl_common.h"
#endif
namespace ratl
{
	////////////////////////////////////////////////////////////////////////////////////////
	// The Vector Class
	////////////////////////////////////////////////////////////////////////////////////////
	template <class T>
	class heap_base : public ratl_base
	{
	public:
		using TStorageTraits = T;
		using TTValue = typename T::TValue;
		////////////////////////////////////////////////////////////////////////////////////
		// Capacity Enum
		////////////////////////////////////////////////////////////////////////////////////
		static const int CAPACITY = T::CAPACITY;
		////////////////////////////////////////////////////////////////////////////////////
		// Data
		////////////////////////////////////////////////////////////////////////////////////
	private:
		array_base<TStorageTraits> mData; // The Memory
		int mPush; // Address Of Next Add Location

		////////////////////////////////////////////////////////////////////////////////////
		// Returns The Location Of Node (i)'s Parent Node (The Parent Node Of Zero Is Zero)
		////////////////////////////////////////////////////////////////////////////////////
		static int parent(const int i)
		{
			return (i - 1) / 2;
		}

		////////////////////////////////////////////////////////////////////////////////////
		// Returns The Location Of Node (i)'s Left Child (The Child Of A Leaf Is The Leaf)
		////////////////////////////////////////////////////////////////////////////////////
		static int left(const int i)
		{
			return 2 * i + 1;
		}

		////////////////////////////////////////////////////////////////////////////////////
		// Returns The Location Of Node (i)'s Right Child (The Child Of A Leaf Is The Leaf)
		////////////////////////////////////////////////////////////////////////////////////
		static int right(const int i)
		{
			return 2 * i + 2;
		}

		////////////////////////////////////////////////////////////////////////////////////
		// Returns The Location Of Largest Child Of Node (i)
		////////////////////////////////////////////////////////////////////////////////////
		int largest_child(const int i) const
		{
			if (left(i) < mPush)
			{
				if (right(i) < mPush)
				{
					return mData[right(i)] < mData[left(i)] ? left(i) : right(i);
				}
				return left(i); // Node i only has a left child, so by default it is the biggest
			}
			return i; // Node i is a leaf, so just return it
		}

		////////////////////////////////////////////////////////////////////////////////////
		// Swaps Two Element Locations
		////////////////////////////////////////////////////////////////////////////////////
		void swap(int a, int b)
		{
			if (a == b)
			{
				return;
			}
			assert(a >= 0 && b >= 0 && a < CAPACITY&& b < CAPACITY);
			mData.swap(a, b);
		}

		////////////////////////////////////////////////////////////////////////////////////
		// Swaps The Data Up The Heap Until It Reaches A Valid Location
		////////////////////////////////////////////////////////////////////////////////////
		void reheapify_upward(int Pos)
		{
			while (Pos && mData[parent(Pos)] < mData[Pos])
			{
				swap(parent(Pos), Pos);
				Pos = parent(Pos);
			}
		}

		////////////////////////////////////////////////////////////////////////////////////
		// Swaps The Data Down The Heap Until It Reaches A Valid Location
		////////////////////////////////////////////////////////////////////////////////////
		void reheapify_downward(int Pos)
		{
			int largestChild = largest_child(Pos);
			while (largestChild != Pos && mData[Pos] < mData[largestChild])
			{
				swap(largestChild, Pos);
				Pos = largestChild;
				largestChild = largest_child(Pos);
			}
		}

		////////////////////////////////////////////////////////////////////////////////////
		// Validate Will Run Through The Heap And Make Sure The Top Element Is Smallest
		////////////////////////////////////////////////////////////////////////////////////
		bool valid()
		{
			for (int i = 1; i < mPush; i++)
			{
				if (mData[0] < mData[i])
				{
					return false;
				}
			}
			return true;
		}

	public:
		////////////////////////////////////////////////////////////////////////////////////
		// Constructor
		////////////////////////////////////////////////////////////////////////////////////
		heap_base() : mPush(0)
		{
		}

		////////////////////////////////////////////////////////////////////////////////////
		// Get The Size (The Difference Between The Push And Pop "Pointers")
		////////////////////////////////////////////////////////////////////////////////////
		int size() const
		{
			return mPush;
		}

		////////////////////////////////////////////////////////////////////////////////////
		// Check To See If The Size Is Zero
		////////////////////////////////////////////////////////////////////////////////////
		bool empty() const
		{
			return !size();
		}

		////////////////////////////////////////////////////////////////////////////////////
		// Check To See If The Size Is Full
		////////////////////////////////////////////////////////////////////////////////////
		bool full() const
		{
			return size() == CAPACITY;
		}

		////////////////////////////////////////////////////////////////////////////////////
		// Empty Out The Entire Heap
		////////////////////////////////////////////////////////////////////////////////////
		void clear()
		{
			mPush = 0;
			mData.clear();
		}

		////////////////////////////////////////////////////////////////////////////////////
		// Get The Data Value At The Top Of The Heap
		////////////////////////////////////////////////////////////////////////////////////
		const TTValue& top() const
		{
			assert(mPush > 0); // Don't Try To Look At This If There Is Nothing In Here
			return mData[0];
		}

		////////////////////////////////////////////////////////////////////////////////////
		// Add A Value To The Queue
		////////////////////////////////////////////////////////////////////////////////////
		void push(const TTValue& nValue)
		{
			assert(size() < CAPACITY);

			// Add It
			//--------
			mData.construct(mPush, nValue);

			// Fix Possible Heap Inconsistancies
			//-----------------------------------
			reheapify_upward(mPush);

			mPush++;
			assert(valid());
		}

		////////////////////////////////////////////////////////////////////////////////////
		// Alloc A Value, call push_alloced to add
		////////////////////////////////////////////////////////////////////////////////////
		TTValue& alloc()
		{
			assert(size() < CAPACITY);

			// Add It
			//--------
			mData.construct(mPush);

			return mData[mPush];
		}

		////////////////////////////////////////////////////////////////////////////////////
		// Alloc A Raw Value for placement new, call push_alloced to add
		////////////////////////////////////////////////////////////////////////////////////
		TRatlNew* alloc_raw()
		{
			assert(size() < CAPACITY);

			return mData.alloc_raw(mPush);
		}

		////////////////////////////////////////////////////////////////////////////////////
		// Add A Value To The Queue, after filling an alloced slot
		////////////////////////////////////////////////////////////////////////////////////
		void push_alloced()
		{
			assert(size() < CAPACITY);
			// Fix Possible Heap Inconsistancies
			//-----------------------------------
			reheapify_upward(mPush);

			mPush++;
			assert(valid());
		}

		////////////////////////////////////////////////////////////////////////////////////
		// Remove A Value From The Queue
		////////////////////////////////////////////////////////////////////////////////////
		void pop()
		{
			assert(size() > 0);

			mPush--;

			// Swap The Lowest Element Up To The Spot We Just "Erased"
			//---------------------------------------------------------
			swap(0, mPush);
			mData.destruct(mPush);

			// Fix Possible Heap Inconsistencies
			//-----------------------------------
			reheapify_downward(0);
			assert(valid());
		}
	};

	template <class T, int ARG_CAPACITY>
	class heap_vs : public heap_base<storage::value_semantics<T, ARG_CAPACITY>>
	{
	public:
		using TStorageTraits = storage::value_semantics<T, ARG_CAPACITY>;
		using TTValue = typename TStorageTraits::TValue;
		static const int CAPACITY = ARG_CAPACITY;

		heap_vs()
		{
		}
	};

	template <class T, int ARG_CAPACITY>
	class heap_os : public heap_base<storage::object_semantics<T, ARG_CAPACITY>>
	{
	public:
		using TStorageTraits = storage::object_semantics<T, ARG_CAPACITY>;
		using TTValue = typename TStorageTraits::TValue;
		static const int CAPACITY = ARG_CAPACITY;

		heap_os()
		{
		}
	};

	template <class T, int ARG_CAPACITY, int ARG_MAX_CLASS_SIZE>
	class heap_is : public heap_base<storage::virtual_semantics<T, ARG_CAPACITY, ARG_MAX_CLASS_SIZE>>
	{
	public:
		using TStorageTraits = storage::virtual_semantics<T, ARG_CAPACITY, ARG_MAX_CLASS_SIZE>;
		using TTValue = typename TStorageTraits::TValue;
		static const int CAPACITY = ARG_CAPACITY;
		static const int MAX_CLASS_SIZE = ARG_MAX_CLASS_SIZE;

		heap_is()
		{
		}
	};
}
#endif
