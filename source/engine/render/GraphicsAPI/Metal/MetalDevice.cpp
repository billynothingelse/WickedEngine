#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define MTK_PRIVATE_IMPLEMENTATION
#define IR_PRIVATE_IMPLEMENTATION
#include "render/GraphicsAPI/Metal/MetalInternal.h"
// Metal backend: Device creation, frame submission and queries

#ifdef __APPLE__

namespace Protean::Graphics
{
	using namespace MetalInternal;

	MetalDevice::MetalDevice(ValidationMode validationMode_, GPUPreference preference)
	{
		Protean::Timer timer;
		device = NS::TransferPtr(MTL::CreateSystemDefaultDevice());
		
		if (device.get() == nullptr)
		{
			wilog_messagebox("Metal graphics device creation failed, exiting!");
			Protean::Platform::Exit();
		}
		if (!device->supportsFamily(MTL::GPUFamilyMetal4))
		{
			wilog_messagebox("Metal 4 graphics is not supported by your system, exiting!");
			Protean::Platform::Exit();
		}
		
		adapterName = device->name()->cString(NS::UTF8StringEncoding);
		
		textureUlongAtomics =
			device->supportsFamily(MTL::GPUFamilyApple8) |
			device->supportsFamily(MTL::GPUFamilyApple9) |
			device->supportsFamily(MTL::GPUFamilyMac2)
		;
		
		capabilities |= GraphicsDeviceCapability_SAMPLER_MINMAX;
		capabilities |= GraphicsDeviceCapability_ALIASING_GENERIC;
		capabilities |= GraphicsDeviceCapability_DEPTH_BOUNDS_TEST;
		capabilities |= GraphicsDeviceCapability_UAV_LOAD_FORMAT_COMMON;
		capabilities |= GraphicsDeviceCapability_UAV_LOAD_FORMAT_R11G11B10_FLOAT;
		capabilities |= GraphicsDeviceCapability_SPARSE_BUFFER;
		capabilities |= GraphicsDeviceCapability_SPARSE_TEXTURE2D;
		capabilities |= GraphicsDeviceCapability_SPARSE_TEXTURE3D;
		capabilities |= GraphicsDeviceCapability_SPARSE_NULL_MAPPING;
		capabilities |= GraphicsDeviceCapability_DEPTH_RESOLVE_MIN_MAX;
		capabilities |= GraphicsDeviceCapability_STENCIL_RESOLVE_MIN_MAX;
		capabilities |= GraphicsDeviceCapability_MESH_SHADER;
		capabilities |= GraphicsDeviceCapability_COPY_BETWEEN_DIFFERENT_IMAGE_ASPECTS_NOT_SUPPORTED;
		capabilities |= GraphicsDeviceCapability_VIDEO_DECODE_H264;
		
		if (device->hasUnifiedMemory())
		{
			capabilities |= GraphicsDeviceCapability_CACHE_COHERENT_UMA;
		}
		if (device->supportsRaytracing())
		{
			capabilities |= GraphicsDeviceCapability_RAYTRACING;
			TOPLEVEL_ACCELERATION_STRUCTURE_INSTANCE_SIZE = sizeof(MTL::IndirectAccelerationStructureInstanceDescriptor);
		}
		
		TIMESTAMP_FREQUENCY = device->queryTimestampFrequency();
		
		allocationhandler = Protean::Allocator::make_shared_single<AllocationHandler>();
		
		argument_table_desc = NS::TransferPtr(MTL4::ArgumentTableDescriptor::alloc()->init());
		argument_table_desc->setInitializeBindings(false);
		argument_table_desc->setSupportAttributeStrides(true);
		argument_table_desc->setMaxBufferBindCount(31);
		argument_table_desc->setMaxTextureBindCount(0);
		argument_table_desc->setMaxSamplerStateBindCount(0);
		
#ifdef PLATFORM_IOS
		const uint64_t real_bindless_resource_capacity = 100000; // ios residency set creation limit
#else
		const uint64_t real_bindless_resource_capacity = BINDLESS_RESOURCE_CAPACITY;
#endif // PLATFORM_IOS
		
		descriptor_heap_res = NS::TransferPtr(device->newBuffer(real_bindless_resource_capacity * sizeof(IRDescriptorTableEntry), MTL::ResourceStorageModeShared));
		descriptor_heap_res->setLabel(NS::TransferPtr(NS::String::alloc()->init("descriptor_heap_res", NS::UTF8StringEncoding)).get());
		
		const uint64_t real_bindless_sampler_capacity = std::min((uint64_t)BINDLESS_SAMPLER_CAPACITY, (uint64_t)device->maxArgumentBufferSamplerCount());
		descriptor_heap_sam = NS::TransferPtr(device->newBuffer(real_bindless_sampler_capacity * sizeof(IRDescriptorTableEntry), MTL::ResourceStorageModeShared));
		descriptor_heap_sam->setLabel(NS::TransferPtr(NS::String::alloc()->init("descriptor_heap_sam", NS::UTF8StringEncoding)).get());
		
		descriptor_heap_res_data = (IRDescriptorTableEntry*)descriptor_heap_res->contents();
		descriptor_heap_sam_data = (IRDescriptorTableEntry*)descriptor_heap_sam->contents();
		
		allocationhandler->free_bindless_res.reserve(real_bindless_resource_capacity);
		allocationhandler->free_bindless_sam.reserve(real_bindless_sampler_capacity);
		for (int i = 0; i < real_bindless_sampler_capacity; ++i)
		{
			allocationhandler->free_bindless_sam.push_back((int)real_bindless_sampler_capacity - i - 1);
		}
		for (int i = 0; i < real_bindless_resource_capacity; ++i)
		{
			allocationhandler->free_bindless_res.push_back((int)real_bindless_resource_capacity - i - 1);
		}
		
		NS::SharedPtr<MTL::ResidencySetDescriptor> residency_set_descriptor = NS::TransferPtr(MTL::ResidencySetDescriptor::alloc()->init());
		residency_set_descriptor->setInitialCapacity(real_bindless_resource_capacity + real_bindless_sampler_capacity);
		NS::Error* error = nullptr;
		allocationhandler->residency_set = NS::TransferPtr(device->newResidencySet(residency_set_descriptor.get(), &error));
		if (error != nullptr)
		{
			NS::String* errDesc = error->localizedDescription();
			wilog_assert(0, "%s", errDesc->utf8String());
			error->release();
		}
		assert(allocationhandler->residency_set.get() != nullptr);
		allocationhandler->make_resident(descriptor_heap_res.get());
		allocationhandler->make_resident(descriptor_heap_sam.get());
		
#ifdef USE_TEXTURE_VIEW_POOL
		NS::SharedPtr<MTL::ResourceViewPoolDescriptor> view_pool_desc = NS::TransferPtr(MTL::ResourceViewPoolDescriptor::alloc()->init());
		view_pool_desc->setResourceViewCount(real_bindless_resource_capacity);
		texture_view_pool = NS::TransferPtr(device->newTextureViewPool(view_pool_desc.get(), &error));
		if (error != nullptr)
		{
			NS::String* errDesc = error->localizedDescription();
			wilog_assert(0, "%s", errDesc->utf8String());
			error->release();
		}
#endif // USE_TEXTURE_VIEW_POOL
		
		// Static samplers workaround:
		NS::SharedPtr<MTL::SamplerDescriptor> sampler_descriptor = NS::TransferPtr(MTL::SamplerDescriptor::alloc()->init());
		sampler_descriptor->setLodMinClamp(0);
		sampler_descriptor->setLodMaxClamp(FLT_MAX);
		sampler_descriptor->setBorderColor(MTL::SamplerBorderColorTransparentBlack);
		sampler_descriptor->setMaxAnisotropy(1);
		sampler_descriptor->setReductionMode(MTL::SamplerReductionModeWeightedAverage);
		sampler_descriptor->setSupportArgumentBuffers(true);
		
		sampler_descriptor->setMinFilter(MTL::SamplerMinMagFilterLinear);
		sampler_descriptor->setMagFilter(MTL::SamplerMinMagFilterLinear);
		sampler_descriptor->setMipFilter(MTL::SamplerMipFilterLinear);
		sampler_descriptor->setSAddressMode(MTL::SamplerAddressModeClampToEdge);
		sampler_descriptor->setRAddressMode(MTL::SamplerAddressModeClampToEdge);
		sampler_descriptor->setTAddressMode(MTL::SamplerAddressModeClampToEdge);
		static_samplers[0] = NS::TransferPtr(device->newSamplerState(sampler_descriptor.get()));
		sampler_descriptor->setSAddressMode(MTL::SamplerAddressModeRepeat);
		sampler_descriptor->setRAddressMode(MTL::SamplerAddressModeRepeat);
		sampler_descriptor->setTAddressMode(MTL::SamplerAddressModeRepeat);
		static_samplers[1] = NS::TransferPtr(device->newSamplerState(sampler_descriptor.get()));
		sampler_descriptor->setSAddressMode(MTL::SamplerAddressModeMirrorRepeat);
		sampler_descriptor->setRAddressMode(MTL::SamplerAddressModeMirrorRepeat);
		sampler_descriptor->setTAddressMode(MTL::SamplerAddressModeMirrorRepeat);
		static_samplers[2] = NS::TransferPtr(device->newSamplerState(sampler_descriptor.get()));
		
		sampler_descriptor->setMinFilter(MTL::SamplerMinMagFilterNearest);
		sampler_descriptor->setMagFilter(MTL::SamplerMinMagFilterNearest);
		sampler_descriptor->setMipFilter(MTL::SamplerMipFilterNearest);
		sampler_descriptor->setSAddressMode(MTL::SamplerAddressModeClampToEdge);
		sampler_descriptor->setRAddressMode(MTL::SamplerAddressModeClampToEdge);
		sampler_descriptor->setTAddressMode(MTL::SamplerAddressModeClampToEdge);
		static_samplers[3] = NS::TransferPtr(device->newSamplerState(sampler_descriptor.get()));
		sampler_descriptor->setSAddressMode(MTL::SamplerAddressModeRepeat);
		sampler_descriptor->setRAddressMode(MTL::SamplerAddressModeRepeat);
		sampler_descriptor->setTAddressMode(MTL::SamplerAddressModeRepeat);
		static_samplers[4] = NS::TransferPtr(device->newSamplerState(sampler_descriptor.get()));
		sampler_descriptor->setSAddressMode(MTL::SamplerAddressModeMirrorRepeat);
		sampler_descriptor->setRAddressMode(MTL::SamplerAddressModeMirrorRepeat);
		sampler_descriptor->setTAddressMode(MTL::SamplerAddressModeMirrorRepeat);
		static_samplers[5] = NS::TransferPtr(device->newSamplerState(sampler_descriptor.get()));
		
		sampler_descriptor->setMaxAnisotropy(16);
		sampler_descriptor->setMinFilter(MTL::SamplerMinMagFilterLinear);
		sampler_descriptor->setMagFilter(MTL::SamplerMinMagFilterLinear);
		sampler_descriptor->setMipFilter(MTL::SamplerMipFilterLinear);
		sampler_descriptor->setSAddressMode(MTL::SamplerAddressModeClampToEdge);
		sampler_descriptor->setRAddressMode(MTL::SamplerAddressModeClampToEdge);
		sampler_descriptor->setTAddressMode(MTL::SamplerAddressModeClampToEdge);
		static_samplers[6] = NS::TransferPtr(device->newSamplerState(sampler_descriptor.get()));
		sampler_descriptor->setSAddressMode(MTL::SamplerAddressModeRepeat);
		sampler_descriptor->setRAddressMode(MTL::SamplerAddressModeRepeat);
		sampler_descriptor->setTAddressMode(MTL::SamplerAddressModeRepeat);
		static_samplers[7] = NS::TransferPtr(device->newSamplerState(sampler_descriptor.get()));
		sampler_descriptor->setSAddressMode(MTL::SamplerAddressModeMirrorRepeat);
		sampler_descriptor->setRAddressMode(MTL::SamplerAddressModeMirrorRepeat);
		sampler_descriptor->setTAddressMode(MTL::SamplerAddressModeMirrorRepeat);
		static_samplers[8] = NS::TransferPtr(device->newSamplerState(sampler_descriptor.get()));
		
		sampler_descriptor->setMaxAnisotropy(1);
		sampler_descriptor->setLodMaxClamp(0);
		sampler_descriptor->setMinFilter(MTL::SamplerMinMagFilterLinear);
		sampler_descriptor->setMagFilter(MTL::SamplerMinMagFilterLinear);
		sampler_descriptor->setMipFilter(MTL::SamplerMipFilterNearest);
		sampler_descriptor->setSAddressMode(MTL::SamplerAddressModeClampToEdge);
		sampler_descriptor->setRAddressMode(MTL::SamplerAddressModeClampToEdge);
		sampler_descriptor->setTAddressMode(MTL::SamplerAddressModeClampToEdge);
		sampler_descriptor->setCompareFunction(MTL::CompareFunctionGreaterEqual);
		static_samplers[9] = NS::TransferPtr(device->newSamplerState(sampler_descriptor.get()));
		
		for (uint32_t i = 0; i < arraysize(static_samplers); ++i)
		{
			IRDescriptorTableSetSampler(&static_sampler_descriptors.samplers[i], static_samplers[i].get(), 0);
		}
		
		queues[QUEUE_GRAPHICS].queue = NS::TransferPtr(device->newMTL4CommandQueue());
		queues[QUEUE_GRAPHICS].queue->addResidencySet(allocationhandler->residency_set.get());
		queues[QUEUE_COMPUTE].queue = NS::TransferPtr(device->newMTL4CommandQueue());
		queues[QUEUE_COMPUTE].queue->addResidencySet(allocationhandler->residency_set.get());
		queues[QUEUE_COPY].queue = NS::TransferPtr(device->newMTL4CommandQueue());
		queues[QUEUE_COPY].queue->addResidencySet(allocationhandler->residency_set.get());
		queues[QUEUE_VIDEO_DECODE].queue = NS::TransferPtr(device->newMTL4CommandQueue());
		queues[QUEUE_VIDEO_DECODE].queue->addResidencySet(allocationhandler->residency_set.get());
		
		for (uint32_t q = 0; q < QUEUE_COUNT; ++q)
		{
			for (uint32_t i = 0; i < BUFFERCOUNT; ++i)
			{
				if (queues[q].queue.get() == nullptr)
					continue;
				frame_fence[i][q] = NS::TransferPtr(device->newSharedEvent());
				frame_fence[i][q]->setSignaledValue(0);
			}
		}
		
		copyAllocator.init(this);
		
		wilog("Created MetalDevice (%d ms)", (int)std::round(timer.elapsed()));
	}

	MetalDevice::~MetalDevice()
	{
		WaitForGPU();
	}

	CommandList MetalDevice::BeginCommandList(QUEUE_TYPE queue)
	{
		cmd_locker.lock();
		uint32_t cmd_current = cmd_count++;
		if (cmd_current >= commandlists.size())
		{
			commandlists.push_back(cmd_allocator.allocate());
			auto& commandlist = commandlists.back();
			commandlist->commandbuffer = NS::TransferPtr(device->newCommandBuffer());
			for (auto& x : commandlist->commandallocators)
			{
				x = NS::TransferPtr(device->newCommandAllocator());
			}
			NS::Error* error = nullptr;
			commandlist->argument_table = NS::TransferPtr(device->newArgumentTable(argument_table_desc.get(), &error));
			if (error != nullptr)
			{
				NS::String* errDesc = error->localizedDescription();
				wilog_assert(0, "%s", errDesc->utf8String());
				error->release();
			}
		}
		CommandList cmd;
		cmd.internal_state = commandlists[cmd_current];
		cmd_locker.unlock();

		CommandList_Metal& commandlist = GetCommandList(cmd);
		commandlist.reset(GetBufferIndex());
		commandlist.queue = queue;
		commandlist.id = cmd_current;
		commandlist.commandallocators[GetBufferIndex()]->reset();
		commandlist.commandbuffer->beginCommandBuffer(commandlist.commandallocators[GetBufferIndex()].get());
		commandlist.argument_table->setAddress(descriptor_heap_res->gpuAddress(), kIRDescriptorHeapBindPoint);
		commandlist.argument_table->setAddress(descriptor_heap_sam->gpuAddress(), kIRSamplerHeapBindPoint);
		
		return cmd;
	}

	void MetalDevice::SubmitCommandLists()
	{
		allocationhandler->destroylocker.lock();
		allocationhandler->residency_set->commit();
		allocationhandler->destroylocker.unlock();
		
		uint32_t cmd_last = cmd_count;
		cmd_count = 0;
		
		// Presents wait:
		for (uint32_t cmd = 0; cmd < cmd_last; ++cmd)
		{
			auto& commandlist = *commandlists[cmd];
			for (auto& x : commandlist.presents)
			{
				CommandQueue& queue = queues[commandlist.queue];
				queue.queue->wait(x.get());
			}
		}
		
		// Submit work and resolve dependencies:
		for (uint32_t cmd = 0; cmd < cmd_last; ++cmd)
		{
			auto& commandlist = *commandlists[cmd];
			if (!commandlist.barriers.empty())
			{
				if (commandlist.compute_encoder.get() == nullptr)
				{
					commandlist.compute_encoder = NS::TransferPtr(commandlist.commandbuffer->computeCommandEncoder()->retain());
				}
				MTL4::VisibilityOptions visibility_options = MTL4::VisibilityOptionNone;
				for (auto& x : commandlist.barriers)
				{
					if (x.type == GPUBarrier::Type_ALIASING)
					{
						visibility_options |= MTL4::VisibilityOptionResourceAlias;
					}
				}
				commandlist.compute_encoder->barrierAfterStages(MTL::StageAll, MTL::StageAll, visibility_options);
				commandlist.barriers.clear();
			}
			if (commandlist.compute_encoder.get() != nullptr)
			{
				commandlist.compute_encoder->endEncoding();
				commandlist.compute_encoder.reset();
				commandlist.compute_encoder = nullptr;
			}
			commandlist.commandbuffer->endCommandBuffer();
			
			CommandQueue& queue = queues[commandlist.queue];
			const bool dependency = !commandlist.signals.empty() || !commandlist.waits.empty();
			if (dependency)
			{
				queue.submit();
			}
			
			queue.submit_cmds.push_back(commandlist.commandbuffer.get());
			
			if (dependency)
			{
				for (auto& semaphore : commandlist.waits)
				{
					queue.wait(semaphore);
					// recycle semaphore only in signals
				}
				commandlist.waits.clear();
				
				queue.submit();
				
				for (auto& semaphore : commandlist.signals)
				{
					queue.signal(semaphore);
					free_semaphore(semaphore);
				}
				commandlist.signals.clear();
			}
			
			// Worker pipelines are merged in to global:
			for (auto& x : commandlist.pipelines_worker)
			{
				if (pipelines_global.count(x.first) == 0)
				{
					pipelines_global[x.first] = std::move(x.second);
				}
				else
				{
					allocationhandler->destroylocker.lock();
					allocationhandler->destroyer_render_pipelines.push_back(std::make_pair(x.second.pipeline, allocationhandler->framecount));
					allocationhandler->destroyer_depth_stencil_states.push_back(std::make_pair(x.second.depth_stencil_state, allocationhandler->framecount));
					allocationhandler->destroylocker.unlock();
				}
			}
			commandlist.pipelines_worker.clear();
		}
		
		// Mark the completion of queues for this frame:
		frame_fence_values[GetBufferIndex()]++;
		for (int q = 0; q < QUEUE_COUNT; ++q)
		{
			CommandQueue& queue = queues[q];
			if (queue.queue.get() == nullptr)
				continue;
			
			queue.submit();
			
			queue.queue->signalEvent(frame_fence[GetBufferIndex()][q].get(), frame_fence_values[GetBufferIndex()]);
		}
		
		// Presents submit:
		for (uint32_t cmd = 0; cmd < cmd_last; ++cmd)
		{
			auto& commandlist = *commandlists[cmd];
			for (auto& x : commandlist.presents)
			{
				CommandQueue& queue = queues[commandlist.queue];
				queue.queue->signalDrawable(x.get());
				x->present();
			}
		}
		
		// Sync up every queue to every other queue at the end of the frame:
		//	Note: it disables overlapping queues into the next frame
		for (int queue1 = 0; queue1 < QUEUE_COUNT; ++queue1)
		{
			if (queues[queue1].queue.get() == nullptr)
				continue;
			for (int queue2 = 0; queue2 < QUEUE_COUNT; ++queue2)
			{
				if (queue1 == queue2)
					continue;
				if (queues[queue2].queue.get() == nullptr)
					continue;
				MTL::SharedEvent* fence = frame_fence[GetBufferIndex()][queue2].get();
				queues[queue1].queue->wait(fence, frame_fence_values[GetBufferIndex()]);
			}
		}
		
		// From here, we begin a new frame, this affects GetBufferIndex()!
		FRAMECOUNT++;
		
		// Initiate stalling CPU when GPU is not yet finished with next frame:
		const uint32_t bufferindex = GetBufferIndex();
		for (int queue = 0; queue < QUEUE_COUNT; ++queue)
		{
			if (queues[queue].queue.get() == nullptr)
				continue;
			MTL::SharedEvent* fence = frame_fence[bufferindex][queue].get();
			if (fence->signaledValue() < frame_fence_values[GetBufferIndex()])
			{
				fence->waitUntilSignaledValue(frame_fence_values[GetBufferIndex()], ~0ull);
			}
		}
		
		allocationhandler->Update(FRAMECOUNT, BUFFERCOUNT);
		
		// Reusing completed async copies:
		{
			std::scoped_lock lck(copyAllocator.locker);
			while (!copyAllocator.async_worklist.empty() && copyAllocator.async_worklist.front().event->signaledValue() >= copyAllocator.async_worklist.front().fenceValue)
			{
				copyAllocator.freelist.push_back(std::move(copyAllocator.async_worklist.front()));
				copyAllocator.async_worklist.pop_front();
			}
		}
	}

	void MetalDevice::WaitForGPU() const
	{
		NS::SharedPtr<MTL::SharedEvent> event = NS::TransferPtr(device->newSharedEvent());
		for (auto& queue : queues)
		{
			if (queue.queue.get() == nullptr)
				continue;
			event->setSignaledValue(0);
			queue.queue->signalEvent(event.get(), 1);
			event->waitUntilSignaledValue(1, ~0ull);
		}

		event->setSignaledValue(0);
		copyAllocator.uploadqueue->signalEvent(event.get(), 1);
		event->waitUntilSignaledValue(1, ~0ull);
	}

	void MetalDevice::ClearPipelineStateCache()
	{
		std::scoped_lock locker(allocationhandler->destroylocker);
		uint64_t framecount = allocationhandler->framecount;
		for (auto& x : pipelines_global)
		{
			allocationhandler->destroyer_render_pipelines.push_back(std::make_pair(std::move(x.second.pipeline), framecount));
			allocationhandler->destroyer_depth_stencil_states.push_back(std::make_pair(std::move(x.second.depth_stencil_state), framecount));
		}
		pipelines_global.clear();
	}
}

#endif // __APPLE__
