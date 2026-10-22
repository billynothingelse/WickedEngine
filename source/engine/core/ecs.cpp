#include "core/ecs.h"

namespace Protean
{
	Entity CreateEntity()
	{
		static std::atomic<Entity> next{ INVALID_ENTITY + 1 };
		return next.fetch_add(1);
	}
}
