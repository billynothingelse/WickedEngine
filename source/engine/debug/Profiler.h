#pragma once
#include "render/GraphicsAPI/GraphicsDevice.h"
#include "render/Canvas.h"
#include "math/Color.h"


// QoL macros, allows writing just ScopedXxxProfiling without needing to declare a variable manually
#define ScopedCPUProfiling(name) Protean::Profiler::ScopedRangeCPU WI_PROFILER_CONCAT(_wi_profiler_cpu_range,__LINE__)(name)
#define ScopedGPUProfiling(name, cmd) Protean::Profiler::ScopedRangeGPU WI_PROFILER_CONCAT(_wi_profiler_gpu_range,__LINE__)(name, cmd)

// same as ScopedXxxProfiling, just will automatically use the function name as name, should only be used at the beginning of a function
#define ScopedCPUProfilingF ScopedCPUProfiling(__FUNCTION__)
#define ScopedGPUProfilingF(cmd) ScopedGPUProfiling(__FUNCTION__, cmd)

// internal helper macros to make somewhat unique variable names based on line numbers to prevent some compilers
// warning about shadowed variables
#define WI_PROFILER_CONCAT(x,y) WI_PROFILER_CONCAT_INDIRECT(x,y)
#define WI_PROFILER_CONCAT_INDIRECT(x,y) x##y

namespace Protean::Profiler
{
	typedef size_t range_id;

	// Begin collecting profiling data for the current frame
	void BeginFrame();

	// Finalize collecting profiling data for the current frame
	void EndFrame(Protean::Graphics::CommandList cmd);

	// Start a CPU profiling range
	range_id BeginRangeCPU(const char* name);

	// Start a GPU profiling range
	range_id BeginRangeGPU(const char* name, Protean::Graphics::CommandList cmd);

	// End a profiling range
	void EndRange(range_id id);

	// helper using RAII to avoid having to manually call BeginRangeCPU/EndRange at beginning/end
	struct ScopedRangeCPU
	{
		range_id id;
		inline ScopedRangeCPU(const char* name) { id = BeginRangeCPU(name); }
		inline ~ScopedRangeCPU() { EndRange(id); }
	};

	// same for BeginRangeGPU
	struct ScopedRangeGPU
	{
		range_id id;
		inline ScopedRangeGPU(const char* name, Protean::Graphics::CommandList cmd) { id = BeginRangeGPU(name, cmd); }
		inline ~ScopedRangeGPU() { EndRange(id); }
	};

	// Renders a basic text of the Profiling results to the (x,y) screen coordinate
	void DrawData(
		const Protean::Canvas& canvas,
		float x,
		float y,
		Protean::Graphics::CommandList cmd,
		Protean::Graphics::ColorSpace colorspace = Protean::Graphics::ColorSpace_SRGB
	);
	void DisableDrawForThisFrame();

	// Enable/disable profiling
	void SetEnabled(bool value);

	bool IsEnabled();

	void SetBackgroundColor(Protean::Color color);
	void SetTextColor(Protean::Color color);
};

