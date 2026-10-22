#include "core/allocator.h"

namespace Protean::Allocator
{
	static SharedAllocator* shared_allocators[256] = {};
	static std::atomic<uint8_t> next_allocator_id{ 0 };

	uint8_t register_shared_allocator(SharedAllocator* allocator)
	{
		uint8_t id = next_allocator_id.fetch_add(1);
		shared_allocators[id] = allocator;
		return id;
	}

	uint8_t get_shared_allocator_count()
	{
		return next_allocator_id.load();
	}

	SharedAllocator* get_shared_allocator(uint8_t id)
	{
		return shared_allocators[id];
	}
}
