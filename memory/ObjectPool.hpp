#pragma once
#include "Handle.hpp"
#include <cassert>
#include <climits>
#include <cstddef>
#include <vector>

namespace yks {

	/** Manages a pool of objects, providing persistent handles to them. */
	template <typename T>
	struct ObjectPool {
		size_t first_free_index; // in roster
	
		// For used entries: .index is index into pool.
		// For free entries: .index is index of next free entry.
		std::vector<Handle> roster;

		std::vector<T> pool;
		std::vector<size_t> pool_indices;

		ObjectPool()
			: first_free_index(SIZE_MAX)
		{}

		template <typename... Args>
		Handle emplace(Args&&... params) {
			// Expand roster if we're out of entries
			if (first_free_index >= roster.size()) {
				expand_roster();
			}

			// Pop head off of free list
			const size_t roster_index = first_free_index;
			first_free_index = roster[roster_index].index;

			// Point roster entry to right place and insert object
			roster[roster_index].index = pool.size();
			pool.emplace_back(std::forward<Args>(params)...);
			pool_indices.push_back(roster_index);

			return Handle(roster_index, roster[roster_index].generation);
		}

		void remove(const Handle h) {
			if (!isValid(h))
				return;

			// Indices for object being removed
			const size_t roster_index = h.index;
			const size_t pool_index = roster[roster_index].index;

			// Indices for object being moved into its place
			const size_t moved_roster_index = pool_indices.back();
			const size_t moved_pool_index = pool.size() - 1;
			assert(roster[moved_roster_index].index == moved_pool_index);

			// Move last element in place of the removed one, updating roster
			roster[moved_roster_index].index = pool_index;
			pool[pool_index] = std::move(pool[moved_pool_index]);
			pool.pop_back();
			pool_indices[pool_index] = pool_indices[moved_pool_index];
			pool_indices.pop_back();

			// Increment generation of removed roster entry and add it to free list
			++roster[roster_index].generation;
			roster[roster_index].index = first_free_index;
			first_free_index = roster_index;
		}

		T* operator[] (const Handle h) {
			if (isValid(h)) {
				assert(roster[h.index].index < pool.size());
				return &pool[roster[h.index].index];
			} else {
				return nullptr;
			}
		}

		const T* operator[] (const Handle h) const {
			if (isValid(h)) {
				assert(roster[h.index].index < pool.size());
				return &pool[roster[h.index].index];
			} else {
				return nullptr;
			}
		}

		/** Checks if object referenced by handle is still in the pool. */
		bool isValid(const Handle h) const {
			return h.index < roster.size() && roster[h.index].generation == h.generation;
		}

		/** Creates a handle to the object currently at pool[index]. */
		Handle makeHandle(size_t index) const {
			if (index >= pool.size())
				return Handle();
			else
				return Handle(index, roster[pool_indices[index]].generation);
		}

		/** Get index into pool for handle. */
		size_t getPoolIndex(const Handle h) const {
			if (isValid(h)) {
				return roster[h.index].index;
			} else {
				return SIZE_MAX;
			}
		}

	private:
		void expand_roster() {
			const Handle new_entry(first_free_index, 0);

			first_free_index = roster.size();
			roster.push_back(new_entry);

			assert(first_free_index < roster.size());
		}
	};

	#if 0
	void test_ObjectPool() {
		ObjectPool<float> objp;
		const auto& objp_c = objp;

		Handle h1 = objp.insert(42.0f);
		Handle h2 = objp.insert(1000.0f);

		float* f1 = objp[h1];
		assert(f1 && *f1 == 42.0f);
		const float* f2 = objp_c[h2];
		assert(f2 && *f2 == 1000.0f);

		objp.remove(h2);

		const float* f3 = objp_c[h2];
		assert(!f3);

		objp.remove(h1);

		Handle h3 = objp.insert(112233.0f);

		objp.remove(h1);

		const float* f4 = objp[h3];
		assert(f4 && *f4 == 112233.0f);

		objp.remove(h3);

		assert(objp.pool.empty());
	}
	#endif

}
