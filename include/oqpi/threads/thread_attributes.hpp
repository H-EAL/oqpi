#pragma once

#include <limits>
#include <cstdint>

namespace oqpi {

	//----------------------------------------------------------------------------------------------
	enum core_affinity : uint32_t
	{
		core0 = 1,
		core1 = core0 << 1,
		core2 = core1 << 1,
		core3 = core2 << 1,
		core4 = core3 << 1,
		core5 = core4 << 1,
		core6 = core5 << 1,
		core7 = core6 << 1,

		all_cores = std::numeric_limits<uint32_t>::max()
	};
	//----------------------------------------------------------------------------------------------

	//----------------------------------------------------------------------------------------------
	enum class thread_priority
	{
		lowest,
		low,
		below_normal,
		normal,
		above_normal,
		high,
		highest
	};
	//----------------------------------------------------------------------------------------------

	//----------------------------------------------------------------------------------------------
	struct thread_attributes
	{
		const char		*pName;
		uint32_t		stackSize;
		core_affinity	coreAffinity;
		thread_priority	priority;
	};
	//----------------------------------------------------------------------------------------------

}
