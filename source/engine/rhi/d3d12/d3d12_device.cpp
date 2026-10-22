#include "rhi/d3d12/d3d12_internal.h"
// D3D12 backend: Device creation, frame submission and queries

#ifdef WICKEDENGINE_BUILD_DX12

// The allocator implementation is compiled once, here, after the D3D12 headers (D3D12MA_D3D12_HEADERS_ALREADY_INCLUDED)
#include "D3D12MemAlloc.cpp" // include this here because we use D3D12MA_D3D12_HEADERS_ALREADY_INCLUDED

namespace Protean::Graphics
{
	using namespace D3D12Internal;


#ifdef PLATFORM_XBOX
std::mutex queue_locker;
#endif // PLATFORM_XBOX

	void D3D12Device::CommandQueue::signal(const Semaphore& semaphore)
	{
		if (queue == nullptr)
			return;
		dx12_check(queue->Signal(semaphore.fence.Get(), semaphore.fenceValue));
	}
	void D3D12Device::CommandQueue::wait(const Semaphore& semaphore)
	{
		if (queue == nullptr)
			return;
		dx12_check(queue->Wait(semaphore.fence.Get(), semaphore.fenceValue));
	}
	void D3D12Device::CommandQueue::submit()
	{
		if (queue == nullptr)
			return;
		if (submit_cmds.empty())
			return;

		queue->ExecuteCommandLists(
			(UINT)submit_cmds.size(),
			submit_cmds.data()
		);

		submit_cmds.clear();
	}

	void D3D12Device::CopyAllocator::init(D3D12Device* device)
	{
		this->device = device;
#ifdef PLATFORM_XBOX
		// Xbox only has 1 copy queue
		queue = device->queues[QUEUE_COPY].queue;
#else
		// On PC we can create secondary copy queue for background uploading tasks:
		D3D12_COMMAND_QUEUE_DESC desc = {};
		desc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
		desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		desc.NodeMask = 0;
		HRESULT hr = dx12_check(device->device->CreateCommandQueue(&desc, PPV_ARGS(queue)));
		if (FAILED(hr))
		{
			wilog_messagebox("ID3D12Device::CreateCommandQueue[CopyAllocator] failed! ERROR: %s", Protean::Helper::GetPlatformErrorString(hr).c_str());
			Protean::Platform::Exit();
		}
		dx12_check(queue->SetName(L"CopyAllocator"));
#endif // PLATFORM_XBOX
	}
	D3D12Device::CopyAllocator::CopyCMD D3D12Device::CopyAllocator::allocate(uint64_t staging_size)
	{
		CopyCMD cmd;

		locker.lock();
		// Try to search for a staging buffer that can fit the request:
		for (size_t i = 0; i < freelist.size(); ++i)
		{
			if (freelist[i].uploadbuffer.desc.size >= staging_size)
			{
				cmd = std::move(freelist[i]);
				std::swap(freelist[i], freelist.back());
				freelist.pop_back();
				break;
			}
		}
		locker.unlock();

		// If no buffer was found that fits the data, create one:
		if (!cmd.IsValid())
		{
			dx12_check(device->device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, PPV_ARGS(cmd.commandAllocator)));
			dx12_check(device->device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, cmd.commandAllocator.Get(), nullptr, PPV_ARGS(cmd.commandList)));
			cmd.commandList->SetName(L"CopyAllocator::commandList");

			dx12_check(cmd.commandList->Close());

			dx12_check(device->device->CreateFence(0, D3D12_FENCE_FLAG_NONE, PPV_ARGS(cmd.fence)));
			dx12_check(cmd.fence->SetName(L"CopyAllocator::fence"));

			if (staging_size > 0)
			{
				GPUBufferDesc uploaddesc;
				uploaddesc.size = Protean::Math::GetNextPowerOfTwo(staging_size);
				uploaddesc.size = std::max(uploaddesc.size, uint64_t(65536));
				uploaddesc.usage = Usage_UPLOAD;
				bool upload_success = device->CreateBuffer(&uploaddesc, nullptr, &cmd.uploadbuffer);
				assert(upload_success);
				device->SetName(&cmd.uploadbuffer, "CopyAllocator::uploadBuffer");
			}
		}

		// begin command list in valid state:
		dx12_check(cmd.commandAllocator->Reset());
		dx12_check(cmd.commandList->Reset(cmd.commandAllocator.Get(), nullptr));

		return cmd;
	}
	void D3D12Device::CopyAllocator::submit(CopyCMD cmd, bool wait_cpu)
	{
		dx12_check(cmd.commandList->Close());
		ID3D12CommandList* commandlists[] = {
			cmd.commandList.Get()
		};

		cmd.fenceValue++;

		{
#ifdef PLATFORM_XBOX
			std::scoped_lock lock(queue_locker); // queue operations are not thread-safe on XBOX
#endif // PLATFORM_XBOX

			queue->ExecuteCommandLists(1, commandlists);
			dx12_check(queue->Signal(cmd.fence.Get(), cmd.fenceValue));
		}

		if (wait_cpu)
		{
			dx12_check(cmd.fence->SetEventOnCompletion(cmd.fenceValue, nullptr));

			std::scoped_lock lock(locker);
			freelist.push_back(cmd);
		}
		else
		{
#ifdef PLATFORM_XBOX
			std::scoped_lock lock(queue_locker); // queue operations are not thread-safe on XBOX
#endif // PLATFORM_XBOX
			for (int q = 0; q < QUEUE_COUNT; ++q)
			{
				CommandQueue& queue = device->queues[q];
				if (queue.queue == nullptr)
					continue;
				queue.queue->Wait(cmd.fence.Get(), cmd.fenceValue);
			}

			std::scoped_lock lock(locker);
			async_worklist.push_back(cmd);
		}
	}

	void D3D12Device::DescriptorBinder::init(D3D12Device* device)
	{
		this->device = device;

		// Reset state to empty:
		reset();
	}
	void D3D12Device::DescriptorBinder::reset()
	{
		table = {};
		optimizer_graphics = nullptr;
		dirty_graphics = 0ull;
		optimizer_compute = nullptr;
		dirty_compute = 0ull;
	}
	void D3D12Device::DescriptorBinder::flush(bool graphics, CommandList cmd)
	{
		uint64_t& dirty = graphics ? dirty_graphics : dirty_compute;
		if (dirty == 0ull)
			return;

		CommandList_DX12& commandlist = device->GetCommandList(cmd);
		ID3D12GraphicsCommandList* graphicscommandlist = commandlist.GetGraphicsCommandList();
		auto pso_internal = graphics ? to_internal(commandlist.active_pso) : to_internal(commandlist.active_cs);
		const RootSignatureOptimizer& optimizer = pso_internal->rootsig_optimizer;

		DWORD index;
		while (_BitScanReverse64(&index, dirty)) // This will make sure that only the dirty root params are iterated, bit-by-bit
		{
			const UINT root_parameter_index = (UINT)index;
			dirty ^= 1ull << root_parameter_index; // remove dirty bit of this root parameter
			const D3D12_ROOT_PARAMETER1& param = pso_internal->rootsig_desc->Desc_1_1.pParameters[root_parameter_index];
			const RootSignatureOptimizer::RootParameterStatistics& stats = optimizer.root_stats[root_parameter_index];

			switch (param.ParameterType)
			{

			case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
			{
				DescriptorHeapGPU& heap = stats.is_sampler ? device->descriptorheap_sam : device->descriptorheap_res;
				D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle = heap.start_gpu;

				if (stats.descriptorCopyCount == 1 && param.DescriptorTable.pDescriptorRanges[0].RangeType != D3D12_DESCRIPTOR_RANGE_TYPE_CBV)
				{
					// This is a special case of 1 descriptor per table. This doesn't need copying, but we can just reference single descriptors by their index
					//	because descriptor index was created already in shader visible descriptor heap for bindless access.
					//	We don't do this for constant buffers, because that needs to support dynamic offset (so we always create descriptor for them on the fly)
					const D3D12_DESCRIPTOR_RANGE1& range = param.DescriptorTable.pDescriptorRanges[0];
					switch (range.RangeType)
					{
					case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
						{
							const UINT reg = range.BaseShaderRegister;
							const GPUResource& resource = table.SRV[reg];
							if (resource.IsValid())
							{
								int subresource = table.SRV_index[reg];
								auto internal_state = to_internal(&resource);
								int descriptor_index = subresource < 0 ? internal_state->srv.index : internal_state->subresources_srv[subresource].index;
								gpu_handle.ptr += descriptor_index * device->resource_descriptor_size;
							}
						}
						break;
					case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
						{
							const UINT reg = range.BaseShaderRegister;
							const GPUResource& resource = table.UAV[reg];
							if (resource.IsValid())
							{
								int subresource = table.UAV_index[reg];
								auto internal_state = to_internal(&resource);
								int descriptor_index = subresource < 0 ? internal_state->uav.index : internal_state->subresources_uav[subresource].index;
								gpu_handle.ptr += descriptor_index * device->resource_descriptor_size;
							}
						}
						break;
					case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER:
						{
							const UINT reg = range.BaseShaderRegister;
							const Sampler& sam = table.SAM[reg];
							if (sam.IsValid())
							{
								auto internal_state = to_internal(&sam);
								int descriptor_index = internal_state->descriptor.index;
								gpu_handle.ptr += descriptor_index * device->sampler_descriptor_size;
							}
						}
						break;
					case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
						assert(0); // This must not be reached, constant buffers are handled below with dynamically created descriptors
						break;
					default:
						assert(0);
						break;
					}
				}
				else if (stats.descriptorCopyCount > 0)
				{
					D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle = heap.start_cpu;
					const uint32_t descriptorSize = stats.is_sampler ? device->sampler_descriptor_size : device->resource_descriptor_size;
					const uint32_t bindless_capacity = stats.is_sampler ? BINDLESS_SAMPLER_CAPACITY : BINDLESS_RESOURCE_CAPACITY;

					// Remarks:
					//	This is allocating from the global shader visible descriptor heaps in a simple incrementing
					//	lockless ring buffer fashion.
					//	In this lockless method, a descriptor array that is to be allocated might not fit without
					//	completely wrapping the beginning of the allocation.
					//	But completely wrapping after the fact we discovered that the array couldn't fit,
					//	it wouldn't be thread safe any more without introducing locks
					//	For that reason, we are reserving an excess amount of descriptors at the end which can't be normally
					//	allocated, but any out of bounds descriptors can still be safely written into it
					//
					//	This method wastes a number of descriptors essentially at the end of the heap, but it is simple
					//	and safe to implement
					//
					//	The excess amount is essentially equal to the maximum number of descriptors that can be allocated at once.

					// The reservation is the maximum amount of descriptors that can be allocated once
					static constexpr uint32_t wrap_reservation_cbv_srv_uav = DESCRIPTORBINDER_CBV_COUNT + DESCRIPTORBINDER_SRV_COUNT + DESCRIPTORBINDER_UAV_COUNT;
					static constexpr uint32_t wrap_reservation_sampler = DESCRIPTORBINDER_SAMPLER_COUNT;
					const uint32_t wrap_reservation = stats.is_sampler ? wrap_reservation_sampler : wrap_reservation_cbv_srv_uav;
					const uint32_t wrap_effective_size = heap.heapDesc.NumDescriptors - bindless_capacity - wrap_reservation;
					assert(wrap_reservation >= stats.descriptorCopyCount); // for correct lockless wrap behaviour

					const uint64_t offset = heap.allocationOffset.fetch_add(stats.descriptorCopyCount);
					const uint64_t wrapped_offset = offset % wrap_effective_size;
					const uint32_t ringoffset = (bindless_capacity + (uint32_t)wrapped_offset) * descriptorSize;
					const uint64_t wrapped_offset_end = wrapped_offset + stats.descriptorCopyCount;

					// Check that gpu offset doesn't intersect with our newly allocated range, if it does, we need to wait until gpu finishes with it:
					//	First check is with the cached completed value to avoid API call into fence object
					uint64_t wrapped_gpu_offset = heap.cached_completedValue % wrap_effective_size;
					if ((wrapped_offset < wrapped_gpu_offset) && (wrapped_gpu_offset < wrapped_offset_end))
					{
						// Second check is with current fence value with API call:
						wrapped_gpu_offset = heap.fence->GetCompletedValue() % wrap_effective_size;
						if ((wrapped_offset < wrapped_gpu_offset) && (wrapped_gpu_offset < wrapped_offset_end))
						{
							// Third step is actual wait until GPU updates fence so that requested descriptors are free:
							dx12_check(heap.fence->SetEventOnCompletion(heap.fenceValue, nullptr));
						}
					}

					gpu_handle.ptr += (size_t)ringoffset;
					cpu_handle.ptr += (size_t)ringoffset;

					for (UINT i = 0; i < param.DescriptorTable.NumDescriptorRanges; ++i)
					{
						const D3D12_DESCRIPTOR_RANGE1& range = param.DescriptorTable.pDescriptorRanges[i];
						switch (range.RangeType)
						{
						case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
							assert(range.NumDescriptors <= DESCRIPTORBINDER_SRV_COUNT);
							device->device->CopyDescriptorsSimple(range.NumDescriptors, cpu_handle, device->nullSRV, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
							for (UINT idx = 0; idx < range.NumDescriptors; ++idx)
							{
								const UINT reg = range.BaseShaderRegister + idx;
								const GPUResource& resource = table.SRV[reg];
								if (resource.IsValid())
								{
									int subresource = table.SRV_index[reg];
									auto internal_state = to_internal(&resource);
									D3D12_CPU_DESCRIPTOR_HANDLE src_handle = subresource < 0 ? internal_state->srv.handle : internal_state->subresources_srv[subresource].handle;
									device->device->CopyDescriptorsSimple(1, cpu_handle, src_handle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
								}
								cpu_handle.ptr += descriptorSize;
							}
							break;
						case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
							assert(range.NumDescriptors <= DESCRIPTORBINDER_UAV_COUNT);
							device->device->CopyDescriptorsSimple(range.NumDescriptors, cpu_handle, device->nullUAV, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
							for (UINT idx = 0; idx < range.NumDescriptors; ++idx)
							{
								const UINT reg = range.BaseShaderRegister + idx;
								const GPUResource& resource = table.UAV[reg];
								if (resource.IsValid())
								{
									int subresource = table.UAV_index[reg];
									auto internal_state = to_internal(&resource);
									D3D12_CPU_DESCRIPTOR_HANDLE src_handle = subresource < 0 ? internal_state->uav.handle : internal_state->subresources_uav[subresource].handle;
									device->device->CopyDescriptorsSimple(1, cpu_handle, src_handle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
								}
								cpu_handle.ptr += descriptorSize;
							}
							break;
						case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
							assert(range.NumDescriptors <= DESCRIPTORBINDER_CBV_COUNT);
							device->device->CopyDescriptorsSimple(range.NumDescriptors, cpu_handle, device->nullCBV, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
							for (UINT idx = 0; idx < range.NumDescriptors; ++idx)
							{
								const UINT reg = range.BaseShaderRegister + idx;
								const GPUBuffer& buffer = table.CBV[reg];
								if (buffer.IsValid())
								{
									uint64_t offset = table.CBV_offset[reg];
									auto internal_state = to_internal(&buffer);

									D3D12_CONSTANT_BUFFER_VIEW_DESC cbv;
									cbv.BufferLocation = internal_state->gpu_address;
									cbv.BufferLocation += offset;
									cbv.SizeInBytes = align(UINT(buffer.desc.size - offset), (UINT)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
									cbv.SizeInBytes = std::min(cbv.SizeInBytes, 65536u);

									device->device->CreateConstantBufferView(&cbv, cpu_handle);
								}
								cpu_handle.ptr += descriptorSize;
							}
							break;
						case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER:
							assert(range.NumDescriptors <= DESCRIPTORBINDER_SAMPLER_COUNT);
							device->device->CopyDescriptorsSimple(range.NumDescriptors, cpu_handle, device->nullSAM, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
							for (UINT idx = 0; idx < range.NumDescriptors; ++idx)
							{
								const UINT reg = range.BaseShaderRegister + idx;
								const Sampler& sam = table.SAM[reg];
								if (sam.IsValid())
								{
									auto internal_state = to_internal(&sam);
									D3D12_CPU_DESCRIPTOR_HANDLE src_handle = internal_state->descriptor.handle;
									device->device->CopyDescriptorsSimple(1, cpu_handle, src_handle, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
								}
								cpu_handle.ptr += descriptorSize;
							}
							break;
						default:
							assert(0);
							break;
						}
					}
				}

				if (graphics)
				{
					graphicscommandlist->SetGraphicsRootDescriptorTable(
						root_parameter_index,
						gpu_handle
					);
				}
				else
				{
					graphicscommandlist->SetComputeRootDescriptorTable(
						root_parameter_index,
						gpu_handle
					);
				}
			}
			break;

			case D3D12_ROOT_PARAMETER_TYPE_CBV:
				{
					assert(param.Descriptor.ShaderRegister < DESCRIPTORBINDER_CBV_COUNT);
					const GPUBuffer& buffer = table.CBV[param.Descriptor.ShaderRegister];

					D3D12_GPU_VIRTUAL_ADDRESS address = {};
					if (buffer.IsValid())
					{
						uint64_t offset = table.CBV_offset[param.Descriptor.ShaderRegister];
						auto internal_state = to_internal(&buffer);
						address = internal_state->gpu_address;
						address += offset;
					}

					if (graphics)
					{
						graphicscommandlist->SetGraphicsRootConstantBufferView(
							root_parameter_index,
							address
						);
					}
					else
					{
						graphicscommandlist->SetComputeRootConstantBufferView(
							root_parameter_index,
							address
						);
					}
				}
				break;

			case D3D12_ROOT_PARAMETER_TYPE_SRV:
				{
					assert(param.Descriptor.ShaderRegister < DESCRIPTORBINDER_SRV_COUNT);
					const GPUResource& resource = table.SRV[param.Descriptor.ShaderRegister];

					D3D12_GPU_VIRTUAL_ADDRESS address = {};
					if (resource.IsValid())
					{
						assert(resource.IsBuffer());
						auto internal_state = to_internal(&resource);
						address = internal_state->gpu_address;

						const int subresource = table.SRV_index[param.Descriptor.ShaderRegister];
						if (subresource >= 0)
						{
							address += internal_state->subresources_srv[subresource].buffer_offset; // note: root descriptor can only be buffer in DX12
						}
					}

					if (graphics)
					{
						graphicscommandlist->SetGraphicsRootShaderResourceView(
							root_parameter_index,
							address
						);
					}
					else
					{
						graphicscommandlist->SetComputeRootShaderResourceView(
							root_parameter_index,
							address
						);
					}
				}
				break;

			case D3D12_ROOT_PARAMETER_TYPE_UAV:
				{
					assert(param.Descriptor.ShaderRegister < DESCRIPTORBINDER_UAV_COUNT);
					const GPUResource& resource = table.UAV[param.Descriptor.ShaderRegister];

					D3D12_GPU_VIRTUAL_ADDRESS address = {};
					if (resource.IsValid())
					{
						assert(resource.IsBuffer());
						auto internal_state = to_internal(&resource);
						address = internal_state->gpu_address;

						const int subresource = table.UAV_index[param.Descriptor.ShaderRegister];
						if (subresource >= 0)
						{
							address += internal_state->subresources_uav[subresource].buffer_offset; // note: root descriptor can only be buffer in DX12
						}
					}

					if (graphics)
					{
						graphicscommandlist->SetGraphicsRootUnorderedAccessView(
							root_parameter_index,
							address
						);
					}
					else
					{
						graphicscommandlist->SetComputeRootUnorderedAccessView(
							root_parameter_index,
							address
						);
					}
				}
				break;

			default:
				assert(0);
				break;
			}
		}

		assert(dirty == 0ull); // check that all dirty root parameters were handled
	}

	// Engine functions
	D3D12Device::D3D12Device(ValidationMode validationMode_, GPUPreference preference)
	{
		Protean::Timer timer;

		SHADER_IDENTIFIER_SIZE = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
		TOPLEVEL_ACCELERATION_STRUCTURE_INSTANCE_SIZE = sizeof(D3D12_RAYTRACING_INSTANCE_DESC);

#ifndef PLATFORM_XBOX
		VIDEO_DECODE_BITSTREAM_ALIGNMENT = D3D12_VIDEO_DECODE_MIN_BITSTREAM_OFFSET_ALIGNMENT;
#endif // PLATFORM_XBOX

		validationMode = validationMode_;

#ifdef PLATFORM_WINDOWS_DESKTOP
		HMODULE dxgi = LoadLibraryEx(L"dxgi.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
		if (dxgi == nullptr)
		{
			std::stringstream ss("");
			ss << "Failed to load dxgi.dll! ERROR: " << std::hex << GetLastError();
			Protean::Helper::messageBox(ss.str(), "Error!");
			Protean::Platform::Exit();
		}

		HMODULE dx12 = LoadLibraryEx(L"d3d12.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
		if (dx12 == nullptr)
		{
			std::stringstream ss("");
			ss << "Failed to load d3d12.dll! ERROR: " << std::hex << GetLastError();
			Protean::Helper::messageBox(ss.str(), "Error!");
			Protean::Platform::Exit();
		}

		CreateDXGIFactory2 = (PFN_CREATE_DXGI_FACTORY_2)wiGetProcAddress(dxgi, "CreateDXGIFactory2");
		assert(CreateDXGIFactory2 != nullptr);
		if (CreateDXGIFactory2 == nullptr)
		{
			std::stringstream ss("");
			ss << "Failed to load CreateDXGIFactory2! ERROR: " << std::hex << GetLastError();
			Protean::Helper::messageBox(ss.str(), "Error!");
			Protean::Platform::Exit();
		}

#ifdef _DEBUG
		if (validationMode != ValidationMode_Disabled)
		{
			DXGIGetDebugInterface1 = (PFN_DXGI_GET_DEBUG_INTERFACE1)wiGetProcAddress(dxgi, "DXGIGetDebugInterface1");
			assert(DXGIGetDebugInterface1 != nullptr);
		}
#endif

		D3D12CreateDevice = (PFN_D3D12_CREATE_DEVICE)wiGetProcAddress(dx12, "D3D12CreateDevice");
		assert(D3D12CreateDevice != nullptr);
		if (D3D12CreateDevice == nullptr)
		{
			std::stringstream ss("");
			ss << "Failed to load D3D12CreateDevice! ERROR: " << std::hex << GetLastError();
			Protean::Helper::messageBox(ss.str(), "Error!");
			Protean::Platform::Exit();
		}

		D3D12CreateVersionedRootSignatureDeserializer = (PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER)wiGetProcAddress(dx12, "D3D12CreateVersionedRootSignatureDeserializer");
		assert(D3D12CreateVersionedRootSignatureDeserializer != nullptr);
		if (D3D12CreateVersionedRootSignatureDeserializer == nullptr)
		{
			std::stringstream ss("");
			ss << "Failed to load D3D12CreateVersionedRootSignatureDeserializer! ERROR: " << std::hex << GetLastError();
			Protean::Helper::messageBox(ss.str(), "Error!");
			Protean::Platform::Exit();
		}

		D3D12SerializeRootSignature = (PFN_D3D12_SERIALIZE_ROOT_SIGNATURE)wiGetProcAddress(dx12, "D3D12SerializeRootSignature");
		assert(D3D12SerializeRootSignature != nullptr);
		if (D3D12SerializeRootSignature == nullptr)
		{
			std::stringstream ss("");
			ss << "Failed to load D3D12SerializeRootSignature! ERROR: " << std::hex << GetLastError();
			Protean::Helper::messageBox(ss.str(), "Error!");
			Protean::Platform::Exit();
		}

		if (validationMode != ValidationMode_Disabled)
		{
			// Enable the debug layer.
			auto D3D12GetDebugInterface = (PFN_D3D12_GET_DEBUG_INTERFACE)wiGetProcAddress(dx12, "D3D12GetDebugInterface");
			if (D3D12GetDebugInterface)
			{
				ComPtr<ID3D12Debug> d3dDebug;
				if (SUCCEEDED(D3D12GetDebugInterface(PPV_ARGS(d3dDebug))))
				{
					d3dDebug->EnableDebugLayer();
					if (validationMode == ValidationMode_GPU)
					{
						ComPtr<ID3D12Debug1> d3dDebug1;
						if (SUCCEEDED(d3dDebug.As(&d3dDebug1)))
						{
							d3dDebug1->SetEnableGPUBasedValidation(TRUE);
							d3dDebug1->SetEnableSynchronizedCommandQueueValidation(TRUE);
						}
					}
				}

				// DRED
				ComPtr<ID3D12DeviceRemovedExtendedDataSettings1> pDredSettings;
				if (SUCCEEDED(D3D12GetDebugInterface(PPV_ARGS(pDredSettings))))
				{
					// Turn on auto-breadcrumbs and page fault reporting.
					pDredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
					pDredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
					pDredSettings->SetBreadcrumbContextEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
				}
			}

#if defined(_DEBUG)
			ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
			if (DXGIGetDebugInterface1 != nullptr && SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgiInfoQueue.GetAddressOf()))))
			{
				dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
				dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
				//dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING, true);

				DXGI_INFO_QUEUE_MESSAGE_ID hide[] =
				{
					80 /* IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides. */,
				};
				DXGI_INFO_QUEUE_FILTER filter = {};
				filter.DenyList.NumIDs = static_cast<UINT>(std::size(hide));
				filter.DenyList.pIDList = hide;
				dxgiInfoQueue->AddStorageFilterEntries(DXGI_DEBUG_DXGI, &filter);
			}
#endif
		}
#endif // PLATFORM_WINDOWS_DESKTOP

		HRESULT hr;
		ComPtr<IDXGIAdapter1> dxgiAdapter;

#ifdef PLATFORM_XBOX

		hr = Protean::Graphics::Xbox::CreateDevice(device, dxgiAdapter, validationMode);

#else
		hr = CreateDXGIFactory2((validationMode != ValidationMode_Disabled) ? DXGI_CREATE_FACTORY_DEBUG : 0u, PPV_ARGS(dxgiFactory));
		if (FAILED(hr))
		{
			wilog_messagebox("CreateDXGIFactory2 failed! ERROR: %s", Protean::Helper::GetPlatformErrorString(hr).c_str());
			Protean::Platform::Exit();
		}

		// Determines whether tearing support is available for fullscreen borderless windows.
		{
			BOOL allowTearing = FALSE;

			ComPtr<IDXGIFactory5> dxgiFactory5;
			hr = dxgiFactory.As(&dxgiFactory5);
			if (SUCCEEDED(hr))
			{
				hr = dxgiFactory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
			}

			if (FAILED(hr) || !allowTearing)
			{
				tearingSupported = false;
				Protean::Helper::DebugOut("WARNING: Variable refresh rate displays not supported\n");
			}
			else
			{
				tearingSupported = true;
			}
		}

		// pick the highest performance adapter that is able to create the device

		ComPtr<IDXGIFactory6> dxgiFactory6;
		const bool queryByPreference = SUCCEEDED(dxgiFactory.As(&dxgiFactory6));
		auto NextAdapter = [&](uint32_t index, IDXGIAdapter1** ppAdapter)
		{
			if (queryByPreference)
			{
				return dxgiFactory6->EnumAdapterByGpuPreference(index, preference == GPUPreference_Integrated ? DXGI_GPU_PREFERENCE_MINIMUM_POWER : DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(ppAdapter));
			}
			return dxgiFactory->EnumAdapters1(index, ppAdapter);
		};

		const bool vendor_preference = preference > GPUPreference_Integrated;
		ComPtr<IDXGIAdapter1> tempAdapter;
		for (uint32_t i = 0; NextAdapter(i, tempAdapter.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND; ++i)
		{
			bool vendor_priority = false;
			DXGI_ADAPTER_DESC1 adapterDesc;
			hr = tempAdapter->GetDesc1(&adapterDesc);
			if (SUCCEEDED(hr))
			{
				// Don't select the Basic Render Driver adapter.
				if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
				{
					continue;
				}

				if (preference == GPUPreference_AMD)
				{
					vendor_priority = Protean::Helper::toUpper(std::wstring(adapterDesc.Description)).find(L"AMD") != std::wstring::npos;
				}
				else if (preference == GPUPreference_Nvidia)
				{
					vendor_priority = Protean::Helper::toUpper(std::wstring(adapterDesc.Description)).find(L"NVIDIA") != std::wstring::npos;
				}
				else if (preference == GPUPreference_Intel)
				{
					vendor_priority = Protean::Helper::toUpper(std::wstring(adapterDesc.Description)).find(L"INTEL") != std::wstring::npos;
				}
			}

			if (device == nullptr || vendor_priority)
			{
				D3D_FEATURE_LEVEL featurelevels[] = {
					D3D_FEATURE_LEVEL_12_2,
					D3D_FEATURE_LEVEL_12_1,
					D3D_FEATURE_LEVEL_12_0,
					D3D_FEATURE_LEVEL_11_1,
					D3D_FEATURE_LEVEL_11_0,
				};
				for (auto& featureLevel : featurelevels)
				{
					if (SUCCEEDED(D3D12CreateDevice(tempAdapter.Get(), featureLevel, PPV_ARGS(device))))
					{
						dxgiAdapter = tempAdapter;
						break;
					}
				}
			}
			if (device != nullptr && (!vendor_preference || vendor_priority))
				break;
		}

		assert(dxgiAdapter != nullptr);
		if (dxgiAdapter == nullptr)
		{
			wilog_messagebox("DXGI: No capable graphics adapter found!");
			Protean::Platform::Exit();
		}

		assert(device != nullptr);
		if (device == nullptr)
		{
			wilog_messagebox("D3D12: Device couldn't be created!");
			Protean::Platform::Exit();
		}

		if (validationMode != ValidationMode_Disabled)
		{
			// Configure debug device (if active).
			ComPtr<ID3D12InfoQueue> d3dInfoQueue;
			if (SUCCEEDED(device.As(&d3dInfoQueue)))
			{
#ifdef _DEBUG
				d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
				d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
				//d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
#endif

				std::vector<D3D12_MESSAGE_SEVERITY> enabledSeverities;
				std::vector<D3D12_MESSAGE_ID> disabledMessages;

				// These severities should be seen all the time
				enabledSeverities.push_back(D3D12_MESSAGE_SEVERITY_CORRUPTION);
				enabledSeverities.push_back(D3D12_MESSAGE_SEVERITY_ERROR);
				enabledSeverities.push_back(D3D12_MESSAGE_SEVERITY_WARNING);
				enabledSeverities.push_back(D3D12_MESSAGE_SEVERITY_MESSAGE);

				if (validationMode == ValidationMode_Verbose)
				{
					// Verbose only filters
					enabledSeverities.push_back(D3D12_MESSAGE_SEVERITY_INFO);
				}

				disabledMessages.push_back(D3D12_MESSAGE_ID_DRAW_EMPTY_SCISSOR_RECTANGLE);
				disabledMessages.push_back(D3D12_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS);
				disabledMessages.push_back(D3D12_MESSAGE_ID_HEAP_ADDRESS_RANGE_INTERSECTS_MULTIPLE_BUFFERS);

				D3D12_INFO_QUEUE_FILTER filter = {};
				filter.AllowList.NumSeverities = static_cast<UINT>(enabledSeverities.size());
				filter.AllowList.pSeverityList = enabledSeverities.data();
				filter.DenyList.NumIDs = static_cast<UINT>(disabledMessages.size());
				filter.DenyList.pIDList = disabledMessages.data();
				d3dInfoQueue->AddStorageFilterEntries(&filter);
			}
		}
#endif // PLATFORM_XBOX

		D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
		allocatorDesc.pDevice = device.Get();
		allocatorDesc.pAdapter = dxgiAdapter.Get();
		//allocatorDesc.PreferredBlockSize = 256 * 1024 * 1024;
		//allocatorDesc.Flags |= D3D12MA::ALLOCATOR_FLAG_ALWAYS_COMMITTED;
		allocatorDesc.Flags |= D3D12MA::ALLOCATOR_FLAG_DEFAULT_POOLS_NOT_ZEROED;
		allocatorDesc.Flags |= D3D12MA::ALLOCATOR_FLAG_MSAA_TEXTURES_ALWAYS_COMMITTED;
		allocatorDesc.Flags |= D3D12MA::ALLOCATOR_FLAG_DONT_PREFER_SMALL_BUFFERS_COMMITTED; // small committed buffers are a performance problem on windows 11

		allocationhandler = Protean::Allocator::make_shared_single<AllocationHandler>();
		allocationhandler->device = device;

		hr = dx12_check(D3D12MA::CreateAllocator(&allocatorDesc, &allocationhandler->allocator));
		if (FAILED(hr))
		{
			wilog_messagebox("D3D12MA::CreateAllocator failed! ERROR: %s", Protean::Helper::GetPlatformErrorString(hr).c_str());
			Protean::Platform::Exit();
		}

		{
			queues[QUEUE_GRAPHICS].desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
			queues[QUEUE_GRAPHICS].desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
			queues[QUEUE_GRAPHICS].desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			queues[QUEUE_GRAPHICS].desc.NodeMask = 0;
			hr = dx12_check(device->CreateCommandQueue(&queues[QUEUE_GRAPHICS].desc, PPV_ARGS(queues[QUEUE_GRAPHICS].queue)));
			if (FAILED(hr))
			{
				wilog_messagebox("ID3D12Device::CreateCommandQueue[QUEUE_GRAPHICS] failed! ERROR: %s", Protean::Helper::GetPlatformErrorString(hr).c_str());
				Protean::Platform::Exit();
			}
			dx12_check(queues[QUEUE_GRAPHICS].queue->SetName(L"QUEUE_GRAPHICS"));
		}

		{
			queues[QUEUE_COMPUTE].desc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
			queues[QUEUE_COMPUTE].desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
			queues[QUEUE_COMPUTE].desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			queues[QUEUE_COMPUTE].desc.NodeMask = 0;
			hr = dx12_check(device->CreateCommandQueue(&queues[QUEUE_COMPUTE].desc, PPV_ARGS(queues[QUEUE_COMPUTE].queue)));
			if (FAILED(hr))
			{
				wilog_messagebox("ID3D12Device::CreateCommandQueue[QUEUE_COMPUTE] failed! ERROR: %s", Protean::Helper::GetPlatformErrorString(hr).c_str());
				Protean::Platform::Exit();
			}
			dx12_check(queues[QUEUE_COMPUTE].queue->SetName(L"QUEUE_COMPUTE"));
		}

		{
			queues[QUEUE_COPY].desc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
			queues[QUEUE_COPY].desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
			queues[QUEUE_COPY].desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			queues[QUEUE_COPY].desc.NodeMask = 0;
			hr = dx12_check(device->CreateCommandQueue(&queues[QUEUE_COPY].desc, PPV_ARGS(queues[QUEUE_COPY].queue)));
			if (FAILED(hr))
			{
				wilog_messagebox("ID3D12Device::CreateCommandQueue[QUEUE_COPY] failed! ERROR: %s", Protean::Helper::GetPlatformErrorString(hr).c_str());
				Protean::Platform::Exit();
			}
			dx12_check(queues[QUEUE_COPY].queue->SetName(L"QUEUE_COPY"));
		}

		if (SUCCEEDED(device.As(&video_device)))
		{
			queues[QUEUE_VIDEO_DECODE].desc.Type = D3D12_COMMAND_LIST_TYPE_VIDEO_DECODE;
			queues[QUEUE_VIDEO_DECODE].desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
			queues[QUEUE_VIDEO_DECODE].desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			queues[QUEUE_VIDEO_DECODE].desc.NodeMask = 0;
			hr = dx12_check(device->CreateCommandQueue(&queues[QUEUE_VIDEO_DECODE].desc, PPV_ARGS(queues[QUEUE_VIDEO_DECODE].queue)));
			if (SUCCEEDED(hr))
			{
				dx12_check(queues[QUEUE_VIDEO_DECODE].queue->SetName(L"QUEUE_VIDEO_DECODE"));

				D3D12_FEATURE_DATA_VIDEO_DECODE_PROFILE_COUNT video_decode_profile_count = {};
				dx12_check(video_device->CheckFeatureSupport(D3D12_FEATURE_VIDEO_DECODE_PROFILE_COUNT, &video_decode_profile_count, sizeof(video_decode_profile_count)));
				video_decode_profile_list.resize(video_decode_profile_count.ProfileCount);
				D3D12_FEATURE_DATA_VIDEO_DECODE_PROFILES video_decode_profiles = {};
				video_decode_profiles.ProfileCount = video_decode_profile_count.ProfileCount;
				video_decode_profiles.pProfiles = video_decode_profile_list.data();
				dx12_check(video_device->CheckFeatureSupport(D3D12_FEATURE_VIDEO_DECODE_PROFILES, &video_decode_profiles, sizeof(video_decode_profiles)));

				for (auto& profile : video_decode_profile_list)
				{
					if (profile == D3D12_VIDEO_DECODE_PROFILE_H264)
					{
						capabilities |= GraphicsDeviceCapability_VIDEO_DECODE_H264;
					}
					else if (profile == D3D12_VIDEO_DECODE_PROFILE_HEVC_MAIN)
					{
						//capabilities |= GraphicsDeviceCapability_VIDEO_DECODE_H265; //TODO
					}
				}
			}
		}

		rtv_descriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		dsv_descriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		resource_descriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		sampler_descriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

		// Resource descriptor heap (shader visible):
		{
			descriptorheap_res.heapDesc.NodeMask = 0;
			descriptorheap_res.heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			descriptorheap_res.heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			descriptorheap_res.heapDesc.NumDescriptors = 1000000; // tier 1 limit
			hr = dx12_check(device->CreateDescriptorHeap(&descriptorheap_res.heapDesc, PPV_ARGS(descriptorheap_res.heap_GPU)));
			if (FAILED(hr))
			{
				wilog_messagebox("ID3D12Device::CreateDescriptorHeap[CBV_SRV_UAV] failed! ERROR: %s", Protean::Helper::GetPlatformErrorString(hr).c_str());
				Protean::Platform::Exit();
			}

			descriptorheap_res.start_cpu = descriptorheap_res.heap_GPU->GetCPUDescriptorHandleForHeapStart();
			descriptorheap_res.start_gpu = descriptorheap_res.heap_GPU->GetGPUDescriptorHandleForHeapStart();

			hr = dx12_check(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, PPV_ARGS(descriptorheap_res.fence)));
			if (FAILED(hr))
			{
				wilog_messagebox("ID3D12Device::CreateFence[CBV_SRV_UAV] failed! ERROR: %s", Protean::Helper::GetPlatformErrorString(hr).c_str());
				Protean::Platform::Exit();
			}
			dx12_check(descriptorheap_res.fence->SetName(L"DescriptorHeapGPU[CBV_SRV_UAV]::fence"));
			descriptorheap_res.fenceValue = descriptorheap_res.fence->GetCompletedValue();

			allocationhandler->free_bindless_res.reserve(BINDLESS_RESOURCE_CAPACITY);
			for (int i = 0; i < BINDLESS_RESOURCE_CAPACITY; ++i)
			{
				allocationhandler->free_bindless_res.push_back(BINDLESS_RESOURCE_CAPACITY - i - 1);
			}
		}

		// Sampler descriptor heap (shader visible):
		{
			descriptorheap_sam.heapDesc.NodeMask = 0;
			descriptorheap_sam.heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
			descriptorheap_sam.heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			descriptorheap_sam.heapDesc.NumDescriptors = 2048; // tier 1 limit
			hr = dx12_check(device->CreateDescriptorHeap(&descriptorheap_sam.heapDesc, PPV_ARGS(descriptorheap_sam.heap_GPU)));
			if (FAILED(hr))
			{
				wilog_messagebox("ID3D12Device::CreateDescriptorHeap[SAMPLER] failed! ERROR: %s", Protean::Helper::GetPlatformErrorString(hr).c_str());
				Protean::Platform::Exit();
			}

			descriptorheap_sam.start_cpu = descriptorheap_sam.heap_GPU->GetCPUDescriptorHandleForHeapStart();
			descriptorheap_sam.start_gpu = descriptorheap_sam.heap_GPU->GetGPUDescriptorHandleForHeapStart();

			hr = dx12_check(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, PPV_ARGS(descriptorheap_sam.fence)));
			if (FAILED(hr))
			{
				wilog_messagebox("ID3D12Device::CreateFence[SAMPLER] failed! ERROR: %s", Protean::Helper::GetPlatformErrorString(hr).c_str());
				Protean::Platform::Exit();
			}
			dx12_check(descriptorheap_sam.fence->SetName(L"DescriptorHeapGPU[SAMPLER]::fence"));
			descriptorheap_sam.fenceValue = descriptorheap_sam.fence->GetCompletedValue();

			allocationhandler->free_bindless_sam.reserve(BINDLESS_SAMPLER_CAPACITY);
			for (int i = 0; i < BINDLESS_SAMPLER_CAPACITY; ++i)
			{
				allocationhandler->free_bindless_sam.push_back(BINDLESS_SAMPLER_CAPACITY - i - 1);
			}
		}

		// Create frame-resident resources:
		for (uint32_t buffer = 0; buffer < BUFFERCOUNT; ++buffer)
		{
			for (int queue = 0; queue < QUEUE_COUNT; ++queue)
			{
				hr = dx12_check(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, PPV_ARGS(frame_fence[buffer][queue])));
				if (FAILED(hr))
				{
					wilog_messagebox("ID3D12Device::CreateFence[FRAME] failed! ERROR: %s", Protean::Helper::GetPlatformErrorString(hr).c_str());
					Protean::Platform::Exit();
				}
				switch (queue)
				{
				case QUEUE_GRAPHICS:
					dx12_check(frame_fence[buffer][queue]->SetName(L"frame_fence[QUEUE_GRAPHICS]"));
					break;
				case QUEUE_COMPUTE:
					dx12_check(frame_fence[buffer][queue]->SetName(L"frame_fence[QUEUE_COMPUTE]"));
					break;
				case QUEUE_COPY:
					dx12_check(frame_fence[buffer][queue]->SetName(L"frame_fence[QUEUE_COPY]"));
					break;
				case QUEUE_VIDEO_DECODE:
					dx12_check(frame_fence[buffer][queue]->SetName(L"frame_fence[QUEUE_VIDEO_DECODE]"));
					break;
				};
			}
		}

		copyAllocator.init(this);

		// Query features:

		capabilities |= GraphicsDeviceCapability_TESSELLATION;
		capabilities |= GraphicsDeviceCapability_PREDICATION;
		capabilities |= GraphicsDeviceCapability_DEPTH_RESOLVE_MIN_MAX;
		capabilities |= GraphicsDeviceCapability_STENCIL_RESOLVE_MIN_MAX;

#ifdef PLATFORM_XBOX
		adapterName = Protean::Graphics::Xbox::GetAdapterName();
		//capabilities |= GraphicsDeviceCapability_MESH_SHADER; // TODO Mesh shader XBOX
		capabilities |= GraphicsDeviceCapability_DEPTH_BOUNDS_TEST;
		capabilities |= GraphicsDeviceCapability_GENERIC_SPARSE_TILE_POOL;
		capabilities |= GraphicsDeviceCapability_CACHE_COHERENT_UMA;
		casting_fully_typed_formats = false;
		resource_heap_tier = D3D12_RESOURCE_HEAP_TIER_2;
		additionalShadingRatesSupported = false;
		capabilities |= GraphicsDeviceCapability_VARIABLE_RATE_SHADING;
		capabilities |= GraphicsDeviceCapability_VARIABLE_RATE_SHADING_TIER2;
		VARIABLE_RATE_SHADING_TILE_SIZE = 8;
		capabilities |= GraphicsDeviceCapability_RAYTRACING;
		capabilities |= GraphicsDeviceCapability_UAV_LOAD_FORMAT_R11G11B10_FLOAT;
		capabilities |= GraphicsDeviceCapability_UAV_LOAD_FORMAT_COMMON;
		capabilities |= GraphicsDeviceCapability_SPARSE_BUFFER;
		capabilities |= GraphicsDeviceCapability_SPARSE_TEXTURE2D;
		capabilities |= GraphicsDeviceCapability_SPARSE_NULL_MAPPING;
		capabilities |= GraphicsDeviceCapability_SAMPLER_MINMAX;
		capabilities |= GraphicsDeviceCapability_SPARSE_TEXTURE3D;
		capabilities |= GraphicsDeviceCapability_CONSERVATIVE_RASTERIZATION;
		capabilities |= GraphicsDeviceCapability_R9G9B9E5_SHAREDEXP_RENDERABLE;
		VIDEO_DECODE_BITSTREAM_ALIGNMENT = 256; // TODO
#else
		// Init feature check (https://devblogs.microsoft.com/directx/introducing-a-new-api-for-checking-feature-support-in-direct3d-12/)
		CD3DX12FeatureSupport features;
		dx12_check(features.Init(device.Get()));

		// Init adapter properties
		{
			DXGI_ADAPTER_DESC1 adapterDesc;
			hr = dxgiAdapter->GetDesc1(&adapterDesc);
			if (SUCCEEDED(hr))
			{
				vendorId = adapterDesc.VendorId;
				deviceId = adapterDesc.DeviceId;
				Protean::Helper::StringConvert(adapterDesc.Description, adapterName);

				// Convert the adapter's D3D12 driver version to a readable string like "24.21.13.9793".
				LARGE_INTEGER umdVersion;
				if (dxgiAdapter->CheckInterfaceSupport(__uuidof(IDXGIDevice), &umdVersion) != DXGI_ERROR_UNSUPPORTED)
				{
					uint64_t encodedVersion = umdVersion.QuadPart;
					std::ostringstream o;
					o << "D3D12 driver version ";
					uint16_t driverVersion[4] = {};

					for (size_t i = 0; i < 4; ++i) {
						driverVersion[i] = (encodedVersion >> (48 - 16 * i)) & 0xFFFF;
						o << driverVersion[i] << ".";
					}

					driverDescription = o.str();
				}

				// Detect adapter type.
				if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
				{
					adapterType = AdapterType_Cpu;
				}
				else
				{
					adapterType = features.UMA() ? AdapterType_IntegratedGpu : AdapterType_DiscreteGpu;
				}
			}
		}

		if (features.HighestShaderModel() < D3D_SHADER_MODEL_6_0)
		{
			std::string error = "Shader model 6.0 is required, but not supported by your system!\n";
			error += "Adapter name: " + adapterName + "\n";
			error += "Adapter type: ";
			switch (adapterType)
			{
			case Protean::Graphics::AdapterType_IntegratedGpu:
				error += "Integrated GPU";
				break;
			case Protean::Graphics::AdapterType_DiscreteGpu:
				error += "Discrete GPU";
				break;
			case Protean::Graphics::AdapterType_VirtualGpu:
				error += "Virtual GPU";
				break;
			case Protean::Graphics::AdapterType_Cpu:
				error += "CPU";
				break;
			default:
				error += "Unknown";
				break;
			}
			error += "\nExiting.";
			wilog_messagebox("%s", error.c_str());
			Protean::Platform::Exit();
		}

		if (features.ConservativeRasterizationTier() >= D3D12_CONSERVATIVE_RASTERIZATION_TIER_1)
		{
			capabilities |= GraphicsDeviceCapability_CONSERVATIVE_RASTERIZATION;
		}
		if (features.ROVsSupported() == TRUE)
		{
			capabilities |= GraphicsDeviceCapability_RASTERIZER_ORDERED_VIEWS;
		}
		if (features.TiledResourcesTier() >= D3D12_TILED_RESOURCES_TIER_1)
		{
			capabilities |= GraphicsDeviceCapability_SPARSE_BUFFER;
			capabilities |= GraphicsDeviceCapability_SPARSE_TEXTURE2D;

			if (features.TiledResourcesTier() >= D3D12_TILED_RESOURCES_TIER_2)
			{
				capabilities |= GraphicsDeviceCapability_SPARSE_NULL_MAPPING;

				// https://docs.microsoft.com/en-us/windows/win32/direct3d11/tiled-resources-texture-sampling-features
				capabilities |= GraphicsDeviceCapability_SAMPLER_MINMAX;

				if (features.TiledResourcesTier() >= D3D12_TILED_RESOURCES_TIER_3)
				{
					capabilities |= GraphicsDeviceCapability_SPARSE_TEXTURE3D;
				}
			}
		}

		if (features.TypedUAVLoadAdditionalFormats())
		{
			// More info about UAV format load support: https://docs.microsoft.com/en-us/windows/win32/direct3d12/typed-unordered-access-view-loads
			capabilities |= GraphicsDeviceCapability_UAV_LOAD_FORMAT_COMMON;

			D3D12_FORMAT_SUPPORT1 formatSupport1 = D3D12_FORMAT_SUPPORT1_NONE;
			D3D12_FORMAT_SUPPORT2 formatSupport2 = D3D12_FORMAT_SUPPORT2_NONE;

			hr = features.FormatSupport(DXGI_FORMAT_R11G11B10_FLOAT, formatSupport1, formatSupport2);
			if (SUCCEEDED(hr) && (formatSupport2 & D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD) != 0)
			{
				capabilities |= GraphicsDeviceCapability_UAV_LOAD_FORMAT_R11G11B10_FLOAT;
			}
		}

		if (features.RaytracingTier() >= D3D12_RAYTRACING_TIER_1_1)
		{
			capabilities |= GraphicsDeviceCapability_RAYTRACING;
		}

		if (features.VariableShadingRateTier() >= D3D12_VARIABLE_SHADING_RATE_TIER_1)
		{
			capabilities |= GraphicsDeviceCapability_VARIABLE_RATE_SHADING;
			if (features.VariableShadingRateTier() >= D3D12_VARIABLE_SHADING_RATE_TIER_2)
			{
				capabilities |= GraphicsDeviceCapability_VARIABLE_RATE_SHADING_TIER2;
				VARIABLE_RATE_SHADING_TILE_SIZE = features.ShadingRateImageTileSize();
			}
		}

		additionalShadingRatesSupported = features.AdditionalShadingRatesSupported();

		if (features.MeshShaderTier() >= D3D12_MESH_SHADER_TIER_1)
		{
			capabilities |= GraphicsDeviceCapability_MESH_SHADER;
		}

		if (features.DepthBoundsTestSupported() == TRUE)
		{
			capabilities |= GraphicsDeviceCapability_DEPTH_BOUNDS_TEST;
		}

		if (features.HighestRootSignatureVersion() < D3D_ROOT_SIGNATURE_VERSION_1_1)
		{
			wilog_messagebox("DX12: Root signature version 1.1 not supported!");
			Protean::Platform::Exit();
		}

		// Fully typed casting: https://microsoft.github.io/DirectX-Specs/d3d/RelaxedCasting.html#casting-rules-for-rs2-drivers
		casting_fully_typed_formats = features.CastingFullyTypedFormatSupported();

		resource_heap_tier = features.ResourceHeapTier();

		if (resource_heap_tier >= D3D12_RESOURCE_HEAP_TIER_2)
		{
			capabilities |= GraphicsDeviceCapability_ALIASING_GENERIC;
		}

		if (features.CacheCoherentUMA())
		{
			capabilities |= GraphicsDeviceCapability_CACHE_COHERENT_UMA;

			D3D12MA::POOL_DESC pool_desc = {};
			pool_desc.HeapProperties.Type = D3D12_HEAP_TYPE_CUSTOM;
			pool_desc.HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE;
			pool_desc.HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
			dx12_check(allocationhandler->allocator->CreatePool(&pool_desc, &allocationhandler->uma_pool));
		}
#endif // PLATFORM_XBOX

#ifdef PLATFORM_WINDOWS_DESKTOP
		// Create fence to detect device removal
		{
			dx12_check(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(deviceRemovedFence.GetAddressOf())));
			dx12_check(deviceRemovedFence->SetName(L"deviceRemovedFence"));

			HANDLE deviceRemovedEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
			dx12_check(deviceRemovedFence->SetEventOnCompletion(UINT64_MAX, deviceRemovedEvent));

			RegisterWaitForSingleObject(
				&deviceRemovedWaitHandle,
				deviceRemovedEvent,
				HandleDeviceRemoved,
				this, // Pass the device as our context
				INFINITE, // No timeout
				0 // No flags
			);
		}
#endif // PLATFORM_WINDOWS_DESKTOP

		// Create common indirect command signatures:

		D3D12_COMMAND_SIGNATURE_DESC cmd_desc = {};

		D3D12_INDIRECT_ARGUMENT_DESC dispatchArgs[1];
		dispatchArgs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;

		D3D12_INDIRECT_ARGUMENT_DESC drawInstancedArgs[1];
		drawInstancedArgs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;

		D3D12_INDIRECT_ARGUMENT_DESC drawIndexedInstancedArgs[1];
		drawIndexedInstancedArgs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

		cmd_desc.ByteStride = sizeof(D3D12_DISPATCH_ARGUMENTS);
		cmd_desc.NumArgumentDescs = 1;
		cmd_desc.pArgumentDescs = dispatchArgs;
		hr = dx12_check(device->CreateCommandSignature(&cmd_desc, nullptr, PPV_ARGS(dispatchIndirectCommandSignature)));
		if (FAILED(hr))
		{
			wilog_messagebox("ID3D12Device::CreateCommandSignature[dispatchIndirect] failed! ERROR: %s", Protean::Helper::GetPlatformErrorString(hr).c_str());
			Protean::Platform::Exit();
		}

		cmd_desc.ByteStride = sizeof(D3D12_DRAW_ARGUMENTS);
		cmd_desc.NumArgumentDescs = 1;
		cmd_desc.pArgumentDescs = drawInstancedArgs;
		hr = dx12_check(device->CreateCommandSignature(&cmd_desc, nullptr, PPV_ARGS(drawInstancedIndirectCommandSignature)));
		if (FAILED(hr))
		{
			wilog_messagebox("ID3D12Device::CreateCommandSignature[drawInstancedIndirect] failed! ERROR: %s", Protean::Helper::GetPlatformErrorString(hr).c_str());
			Protean::Platform::Exit();
		}

		cmd_desc.ByteStride = sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
		cmd_desc.NumArgumentDescs = 1;
		cmd_desc.pArgumentDescs = drawIndexedInstancedArgs;
		hr = dx12_check(device->CreateCommandSignature(&cmd_desc, nullptr, PPV_ARGS(drawIndexedInstancedIndirectCommandSignature)));
		if (FAILED(hr))
		{
			wilog_messagebox("ID3D12Device::CreateCommandSignature[drawIndexedInstancedIndirect] failed! ERROR: %s", Protean::Helper::GetPlatformErrorString(hr).c_str());
			Protean::Platform::Exit();
		}

		if (CheckCapability(GraphicsDeviceCapability_MESH_SHADER))
		{
			D3D12_INDIRECT_ARGUMENT_DESC dispatchMeshArgs[1];
#ifdef PLATFORM_XBOX
			Protean::Graphics::Xbox::FillDispatchMeshIndirectArgumentDesc(dispatchMeshArgs[0], cmd_desc);
#else
			dispatchMeshArgs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH;
			cmd_desc.ByteStride = sizeof(D3D12_DISPATCH_MESH_ARGUMENTS);
#endif // PLATFORM_XBOX
			cmd_desc.NumArgumentDescs = 1;
			cmd_desc.pArgumentDescs = dispatchMeshArgs;
			dx12_check(device->CreateCommandSignature(&cmd_desc, nullptr, PPV_ARGS(dispatchMeshIndirectCommandSignature)));
		}

		allocationhandler->descriptors_res.init(this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 4096);
		allocationhandler->descriptors_sam.init(this, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 256);
		allocationhandler->descriptors_rtv.init(this, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 512);
		allocationhandler->descriptors_dsv.init(this, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 128);

		D3D12_DESCRIPTOR_HEAP_DESC nullHeapDesc = {};
		nullHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		nullHeapDesc.NumDescriptors = DESCRIPTORBINDER_CBV_COUNT + DESCRIPTORBINDER_SRV_COUNT + DESCRIPTORBINDER_UAV_COUNT;
		dx12_check(device->CreateDescriptorHeap(&nullHeapDesc, PPV_ARGS(nulldescriptorheap_cbv_srv_uav)));
		if (FAILED(hr))
		{
			wilog_messagebox("ID3D12Device::CreateDescriptorHeap[nulldescriptorheap_cbv_srv_uav] failed! ERROR: %s", Protean::Helper::GetPlatformErrorString(hr).c_str());
		}

		nullHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
		nullHeapDesc.NumDescriptors = DESCRIPTORBINDER_SAMPLER_COUNT;
		hr = dx12_check(device->CreateDescriptorHeap(&nullHeapDesc, PPV_ARGS(nulldescriptorheap_sampler)));
		if (FAILED(hr))
		{
			wilog_messagebox("ID3D12Device::CreateDescriptorHeap[nulldescriptorheap_sampler] failed! ERROR: %s", Protean::Helper::GetPlatformErrorString(hr).c_str());
		}

		nullCBV = nulldescriptorheap_cbv_srv_uav->GetCPUDescriptorHandleForHeapStart();
		for (uint32_t i = 0; i < DESCRIPTORBINDER_CBV_COUNT; ++i)
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
			D3D12_CPU_DESCRIPTOR_HANDLE handle = nullCBV;
			handle.ptr += i * resource_descriptor_size;
			device->CreateConstantBufferView(&cbv_desc, handle);
		}

		nullSRV = nullCBV;
		nullSRV.ptr += DESCRIPTORBINDER_CBV_COUNT * resource_descriptor_size;
		for (uint32_t i = 0; i < DESCRIPTORBINDER_SRV_COUNT; ++i)
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
			srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srv_desc.Format = DXGI_FORMAT_R32_UINT;
			srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			D3D12_CPU_DESCRIPTOR_HANDLE handle = nullSRV;
			handle.ptr += i * resource_descriptor_size;
			device->CreateShaderResourceView(nullptr, &srv_desc, handle);
		}

		nullUAV = nullSRV;
		nullUAV.ptr += DESCRIPTORBINDER_SRV_COUNT * resource_descriptor_size;
		for (uint32_t i = 0; i < DESCRIPTORBINDER_UAV_COUNT; ++i)
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
			uav_desc.Format = DXGI_FORMAT_R32_UINT;
			uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			D3D12_CPU_DESCRIPTOR_HANDLE handle = nullUAV;
			handle.ptr += i * resource_descriptor_size;
			device->CreateUnorderedAccessView(nullptr, nullptr, &uav_desc, handle);
		}

		nullSAM = nulldescriptorheap_sampler->GetCPUDescriptorHandleForHeapStart();
		for (uint32_t i = 0; i < DESCRIPTORBINDER_SAMPLER_COUNT; ++i)
		{
			D3D12_SAMPLER_DESC sampler_desc = {};
			sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
			sampler_desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
			D3D12_CPU_DESCRIPTOR_HANDLE handle = nullSAM;
			handle.ptr += i * sampler_descriptor_size;
			device->CreateSampler(&sampler_desc, handle);
		}

		// Descriptor safety feature:
		//	We init null descriptors for bindless index = 0 for access safety
		//	Because shader compiler sometimes incorrectly loads descriptor outside of safety branch
		//	Note: these are never freed, this is intentional
		{
			int index = allocationhandler->free_bindless_res.back();
			allocationhandler->free_bindless_res.pop_back();
			wilog_assert(index == 0, "Descriptor safety feature error: descriptor index must be 0!");
			D3D12_CPU_DESCRIPTOR_HANDLE dst_bindless = descriptorheap_res.start_cpu;
			dst_bindless.ptr += index * resource_descriptor_size;
			device->CopyDescriptorsSimple(1, dst_bindless, nullSRV, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
		{
			int index = allocationhandler->free_bindless_sam.back();
			allocationhandler->free_bindless_sam.pop_back();
			wilog_assert(index == 0, "Descriptor safety feature error: descriptor index must be 0!");
			D3D12_CPU_DESCRIPTOR_HANDLE dst_bindless = descriptorheap_sam.start_cpu;
			dst_bindless.ptr += index * sampler_descriptor_size;
			device->CopyDescriptorsSimple(1, dst_bindless, nullSAM, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
		}

		hr = dx12_check(queues[QUEUE_GRAPHICS].queue->GetTimestampFrequency(&TIMESTAMP_FREQUENCY));
		if (FAILED(hr))
		{
			wilog_messagebox("ID3D12CommandQueue::GetTimestampFrequency[QUEUE_GRAPHICS] failed! ERROR: %s", Protean::Helper::GetPlatformErrorString(hr).c_str());
		}

		wilog("Created D3D12Device (%d ms)\nAdapter: %s", (int)std::round(timer.elapsed()), adapterName.c_str());
	}

	D3D12Device::~D3D12Device()
	{
		WaitForGPU();

#ifdef PLATFORM_WINDOWS_DESKTOP
		std::ignore = UnregisterWait(deviceRemovedWaitHandle);
		deviceRemovedFence.Reset();
#endif // PLATFORM_WINDOWS_DESKTOP
	}

	CommandList D3D12Device::BeginCommandList(QUEUE_TYPE queue)
	{
		HRESULT hr;

		cmd_locker.lock();
		uint32_t cmd_current = cmd_count++;
		if (cmd_current >= commandlists.size())
		{
			commandlists.push_back(cmd_allocator.allocate());
		}
		CommandList cmd;
		cmd.internal_state = commandlists[cmd_current];
		cmd_locker.unlock();

		CommandList_DX12& commandlist = GetCommandList(cmd);
		commandlist.reset(GetBufferIndex());
		commandlist.queue = queue;
		commandlist.id = cmd_current;

		if (commandlist.GetCommandList() == nullptr)
		{
			// need to create one more command list:

			for (uint32_t buffer = 0; buffer < BUFFERCOUNT; ++buffer)
			{
				dx12_check(device->CreateCommandAllocator(queues[queue].desc.Type, PPV_ARGS(commandlist.commandAllocators[buffer][queue])));
			}

			if (queue == QUEUE_VIDEO_DECODE)
			{
				ComPtr<ID3D12VideoDecodeCommandList> videoCommandList;
#ifdef PLATFORM_XBOX
				dx12_check(device->CreateCommandList(0, queues[queue].desc.Type, commandlist.commandAllocators[0][queue].Get(), nullptr, PPV_ARGS(videoCommandList)));
				hr = videoCommandList->Close();
#else
				hr = dx12_check(device->CreateCommandList1(0, queues[queue].desc.Type, D3D12_COMMAND_LIST_FLAG_NONE, PPV_ARGS(videoCommandList)));
#endif // PLATFORM_XBOX
				commandlist.commandLists[queue] = videoCommandList;
			}
			else if (queue == QUEUE_COPY)
			{
				ComPtr<ID3D12GraphicsCommandList> copyCommandList;
#ifdef PLATFORM_XBOX
				hr = dx12_check(device->CreateCommandList(0, queues[queue].desc.Type, commandlist.commandAllocators[0][queue].Get(), nullptr, PPV_ARGS(copyCommandList)));
				hr = copyCommandList->Close();
#else
				hr = dx12_check(device->CreateCommandList1(0, queues[queue].desc.Type, D3D12_COMMAND_LIST_FLAG_NONE, PPV_ARGS(copyCommandList)));
#endif // PLATFORM_XBOX
				commandlist.commandLists[queue] = copyCommandList;
			}
			else
			{
				ComPtr<CommandList_DX12::graphics_command_list_version> graphicsCommandList;
#ifdef PLATFORM_XBOX
				hr = dx12_check(device->CreateCommandList(0, queues[queue].desc.Type, commandlist.commandAllocators[0][queue].Get(), nullptr, PPV_ARGS(graphicsCommandList)));
				hr = graphicsCommandList->Close();
#else
				hr = dx12_check(device->CreateCommandList1(0, queues[queue].desc.Type, D3D12_COMMAND_LIST_FLAG_NONE, PPV_ARGS(graphicsCommandList)));
#endif // PLATFORM_XBOX
				commandlist.commandLists[queue] = graphicsCommandList;
			}

			std::wstring ws = L"cmd" + std::to_wstring(commandlist.id);
			commandlist.GetCommandList()->SetName(ws.c_str());

			commandlist.binder.init(this);
		}

		// Start the command list in a default state:
		dx12_check(commandlist.GetCommandAllocator()->Reset());

		if (queue == QUEUE_VIDEO_DECODE)
		{
			dx12_check(commandlist.GetVideoDecodeCommandList()->Reset(commandlist.GetCommandAllocator()));
		}
		else
		{
			dx12_check(commandlist.GetGraphicsCommandList()->Reset(commandlist.GetCommandAllocator(), nullptr));
		}

		if (queue == QUEUE_GRAPHICS || queue == QUEUE_COMPUTE)
		{
			ID3D12DescriptorHeap* heaps[] = {
				descriptorheap_res.heap_GPU.Get(),
				descriptorheap_sam.heap_GPU.Get()
			};
			commandlist.GetGraphicsCommandList()->SetDescriptorHeaps(arraysize(heaps), heaps);
		}

		if (queue == QUEUE_GRAPHICS)
		{
			D3D12_RECT pRects[D3D12_VIEWPORT_AND_SCISSORRECT_MAX_INDEX + 1];
			for (uint32_t i = 0; i < arraysize(pRects); ++i)
			{
				pRects[i].left = 0;
				pRects[i].right = 16384;
				pRects[i].top = 0;
				pRects[i].bottom = 16384;
			}
			commandlist.GetGraphicsCommandList()->RSSetScissorRects(arraysize(pRects), pRects);
		}

		return cmd;
	}

	void D3D12Device::SubmitCommandLists()
	{
#ifdef PLATFORM_XBOX
		std::scoped_lock lock(queue_locker); // queue operations are not thread-safe on XBOX
#endif // PLATFORM_XBOX

		// Submit current frame:
		{
			uint32_t cmd_last = cmd_count;
			cmd_count = 0;
			for (uint32_t cmd = 0; cmd < cmd_last; ++cmd)
			{
				CommandList_DX12& commandlist = *commandlists[cmd];
				if (commandlist.queue == QUEUE_VIDEO_DECODE)
				{
					dx12_check(commandlist.GetVideoDecodeCommandList()->Close());
				}
				else
				{
					dx12_check(commandlist.GetGraphicsCommandList()->Close());
				}

				CommandQueue& queue = queues[commandlist.queue];
				const bool dependency = !commandlist.signals.empty() || !commandlist.waits.empty();

				if (dependency)
				{
					// If the current commandlist must resolve a dependency, then previous ones will be submitted before doing that:
					//	This improves GPU utilization because not the whole batch of command lists will need to synchronize, but only the one that handles it
					queue.submit();
				}

				queue.submit_cmds.push_back(commandlist.GetCommandList());

				if (dependency)
				{

					for(auto& semaphore : commandlist.waits)
					{
						// Wait for command list dependency:
						queue.wait(semaphore);

						// semaphore is not recycled here, only the signals recycle themselves vecause wait will use the same
					}
					commandlist.waits.clear();

					queue.submit();

					for(auto& semaphore : commandlist.signals)
					{
						// Signal this command list's completion:
						queue.signal(semaphore);

						// recycle semaphore:
						free_semaphore(semaphore);
					}
					commandlist.signals.clear();
				}

				for (auto& x : commandlist.pipelines_worker)
				{
					if (pipelines_global.count(x.first) == 0)
					{
						pipelines_global[x.first] = x.second;
					}
				}
				commandlist.pipelines_worker.clear();
			}

			// Mark the completion of queues for this frame:
			frame_fence_values[GetBufferIndex()]++;
			for (int q = 0; q < QUEUE_COUNT; ++q)
			{
				CommandQueue& queue = queues[q];
				if (queue.queue == nullptr)
					continue;

				queue.submit();

				dx12_check(queue.queue->Signal(frame_fence[GetBufferIndex()][q].Get(), frame_fence_values[GetBufferIndex()]));
			}

			for (uint32_t cmd = 0; cmd < cmd_last; ++cmd)
			{
				CommandList_DX12& commandlist = *commandlists[cmd];
				for (auto& swapchain : commandlist.swapchains)
				{
					auto swapchain_internal = to_internal(swapchain);

#ifdef PLATFORM_XBOX

					Protean::Graphics::Xbox::Present(
						device.Get(),
						queues[QUEUE_GRAPHICS].queue.Get(),
						swapchain_internal->textures[swapchain_internal->bufferIndex]->resource.Get(),
						swapchain->desc.vsync
					);

					swapchain_internal->bufferIndex = (swapchain_internal->bufferIndex + 1) % (uint32_t)swapchain_internal->textures.size();

#else
					UINT presentFlags = 0;
					if (!swapchain->desc.vsync && !swapchain->desc.fullscreen)
					{
						presentFlags = DXGI_PRESENT_ALLOW_TEARING;
					}

					HRESULT hr = dx12_check(swapchain_internal->swapChain->Present(swapchain->desc.vsync, presentFlags));

					// If the device was reset we must completely reinitialize the renderer.
					if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
					{
						wilog_messagebox("Device Lost on Present: %s", Protean::Helper::GetPlatformErrorString(((hr == DXGI_ERROR_DEVICE_REMOVED) ? device->GetDeviceRemovedReason() : hr)).c_str());

						// Handle device lost
						OnDeviceRemoved();
					}
#endif // PLATFORM_XBOX
				}
			}
		}

		// Sync up every queue to every other queue at the end of the frame:
		//	Note: it disables overlapping queues into the next frame
		for (int queue1 = 0; queue1 < QUEUE_COUNT; ++queue1)
		{
			if (queues[queue1].queue == nullptr)
				continue;
			for (int queue2 = 0; queue2 < QUEUE_COUNT; ++queue2)
			{
				if (queue1 == queue2)
					continue;
				if (queues[queue2].queue == nullptr)
					continue;
				ID3D12Fence* fence = frame_fence[GetBufferIndex()][queue2].Get();
				queues[queue1].queue->Wait(fence, frame_fence_values[GetBufferIndex()]);
			}
		}

		descriptorheap_res.SignalGPU(queues[QUEUE_GRAPHICS].queue.Get());
		descriptorheap_sam.SignalGPU(queues[QUEUE_GRAPHICS].queue.Get());

		// From here, we begin a new frame, this affects GetBufferIndex()!
		FRAMECOUNT++;

		// Initiate stalling CPU when GPU is not yet finished with next frame:
		const uint32_t bufferindex = GetBufferIndex();
		for (int queue = 0; queue < QUEUE_COUNT; ++queue)
		{
			if (queues[queue].queue == nullptr)
				continue;
			ID3D12Fence* fence = frame_fence[bufferindex][queue].Get();
			if (fence->GetCompletedValue() < frame_fence_values[GetBufferIndex()])
			{
				// nullptr event handle will simply wait immediately:
				//	https://docs.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12fence-seteventoncompletion#remarks
				dx12_check(fence->SetEventOnCompletion(frame_fence_values[GetBufferIndex()], nullptr));
			}
		}

		allocationhandler->Update(FRAMECOUNT, BUFFERCOUNT);

		// Reusing completed async copies:
		{
			std::scoped_lock lck(copyAllocator.locker);
			while (!copyAllocator.async_worklist.empty() && copyAllocator.async_worklist.front().fence->GetCompletedValue() >= copyAllocator.async_worklist.front().fenceValue)
			{
				copyAllocator.freelist.push_back(std::move(copyAllocator.async_worklist.front()));
				copyAllocator.async_worklist.pop_front();
			}
		}
	}

	void D3D12Device::OnDeviceRemoved()
	{
#ifdef PLATFORM_WINDOWS_DESKTOP
		static std::mutex onDeviceRemovedMutex;
		std::scoped_lock lck(onDeviceRemovedMutex);

		if (deviceRemoved)
		{
			return;
		}
		deviceRemoved = true;

		const char* removedReasonString;
		HRESULT removedReason = device->GetDeviceRemovedReason();

		switch (removedReason)
		{
		case DXGI_ERROR_DEVICE_HUNG:
			removedReasonString = "DXGI_ERROR_DEVICE_HUNG";
			break;
		case DXGI_ERROR_DEVICE_REMOVED:
			removedReasonString = "DXGI_ERROR_DEVICE_REMOVED";
			break;
		case DXGI_ERROR_DEVICE_RESET:
			removedReasonString = "DXGI_ERROR_DEVICE_RESET";
			break;
		case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
			removedReasonString = "DXGI_ERROR_DRIVER_INTERNAL_ERROR";
			break;
		case DXGI_ERROR_INVALID_CALL:
			removedReasonString = "DXGI_ERROR_INVALID_CALL";
			break;
		case DXGI_ERROR_ACCESS_DENIED:
			removedReasonString = "DXGI_ERROR_ACCESS_DENIED";
			break;
		default:
			removedReasonString = "Unknown DXGI error";
			break;
		}


		// Based on: https://github.com/simco50/D3D12_Research/blob/869373829444f029eb3a6ce95dd3aa541c665bb2/D3D12/Graphics/RHI/Graphics.cpp

		//D3D12_AUTO_BREADCRUMB_OP
		constexpr const char* OpNames[] =
		{
			"SetMarker",
			"BeginEvent",
			"EndEvent",
			"DrawInstanced",
			"DrawIndexedInstanced",
			"ExecuteIndirect",
			"Dispatch",
			"CopyBufferRegion",
			"CopyTextureRegion",
			"CopyResource",
			"CopyTiles",
			"ResolveSubresource",
			"ClearRenderTargetView",
			"ClearUnorderedAccessView",
			"ClearDepthStencilView",
			"ResourceBarrier",
			"ExecuteBundle",
			"Present",
			"ResolveQueryData",
			"BeginSubmission",
			"EndSubmission",
			"DecodeFrame",
			"ProcessFrames",
			"AtomicCopyBufferUint",
			"AtomicCopyBufferUint64",
			"ResolveSubresourceRegion",
			"WriteBufferImmediate",
			"DecodeFrame1",
			"SetProtectedResourceSession",
			"DecodeFrame2",
			"ProcessFrames1",
			"BuildRaytracingAccelerationStructure",
			"EmitRaytracingAccelerationStructurePostBuildInfo",
			"CopyRaytracingAccelerationStructure",
			"DispatchRays",
			"InitializeMetaCommand",
			"ExecuteMetaCommand",
			"EstimateMotion",
			"ResolveMotionVectorHeap",
			"SetPipelineState1",
			"InitializeExtensionCommand",
			"ExecuteExtensionCommand",
			"DispatchMesh",
			"EncodeFrame",
			"ResolveEncoderOutputMetadata",
		};
		static_assert(ARRAYSIZE(OpNames) == D3D12_AUTO_BREADCRUMB_OP_RESOLVEENCODEROUTPUTMETADATA + 1, "OpNames array length mismatch");

		//D3D12_DRED_ALLOCATION_TYPE
		constexpr const char* AllocTypesNames[] =
		{
			"CommandQueue",
			"CommandAllocator",
			"PipelineState",
			"CommandList",
			"Fence",
			"DescriptorHeap",
			"Heap",
			"Unknown",
			"QueryHeap",
			"CommandSignature",
			"PipelineLibrary",
			"VideoDecoder",
			"Unknown",
			"VideoProcessor",
			"Unknown",
			"Resource",
			"Pass",
			"CryptoSession",
			"CryptoSessionPolicy",
			"ProtectedResourceSession",
			"VideoDecoderHeap",
			"CommandPool",
			"CommandRecorder",
			"StateObjectr",
			"MetaCommand",
			"SchedulingGroup",
			"VideoMotionEstimator",
			"VideoMotionVectorHeap",
			"VideoExtensionCommand",
			"VideoEncoder",
			"VideoEncoderHeap",
		};
		static_assert(ARRAYSIZE(AllocTypesNames) == D3D12_DRED_ALLOCATION_TYPE_VIDEO_ENCODER_HEAP - D3D12_DRED_ALLOCATION_TYPE_COMMAND_QUEUE + 1, "AllocTypes array length mismatch");

		std::string log;

		ComPtr<ID3D12DeviceRemovedExtendedData1> pDred;
		if (SUCCEEDED(device->QueryInterface(PPV_ARGS(pDred))))
		{
			D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT1 pDredAutoBreadcrumbsOutput = {};
			if (SUCCEEDED(pDred->GetAutoBreadcrumbsOutput1(&pDredAutoBreadcrumbsOutput)))
			{
				log += "[DRED] Last tracked GPU operations:\n";

				std::map<int, const wchar_t*> contextStrings;

				const D3D12_AUTO_BREADCRUMB_NODE1* pNode = pDredAutoBreadcrumbsOutput.pHeadAutoBreadcrumbNode;
				while (pNode && pNode->pLastBreadcrumbValue)
				{
					int lastCompletedOp = *pNode->pLastBreadcrumbValue;

					char commandlistname[1024] = {};
					char commandqueuename[1024] = {};

					// Note: pCommandListDebugNameA and similar doesn't seem to contain anything while pCommandListDebugNameW does, so convert these by hand:
					if (pNode->pCommandListDebugNameW != nullptr)
					{
						Protean::Helper::StringConvert(pNode->pCommandListDebugNameW, commandlistname, arraysize(commandlistname));
					}
					if (pNode->pCommandQueueDebugNameW != nullptr)
					{
						Protean::Helper::StringConvert(pNode->pCommandQueueDebugNameW, commandqueuename, arraysize(commandqueuename));
					}

					log += std::string("[DRED] Commandlist = [") + commandlistname + std::string("], CommandQueue = [") + commandqueuename + std::string("], lastCompletedOp = [") + std::to_string(lastCompletedOp) + "], BreadCrumbCount = [" + std::to_string(pNode->BreadcrumbCount) + "]\n";

					int firstOp = std::max(lastCompletedOp - 100, 0);
					int lastOp = std::min(lastCompletedOp + 20, int(pNode->BreadcrumbCount) - 1);

					contextStrings.clear();
					for (uint32_t breadcrumbContext = firstOp; breadcrumbContext < pNode->BreadcrumbContextsCount; ++breadcrumbContext)
					{
						const D3D12_DRED_BREADCRUMB_CONTEXT& context = pNode->pBreadcrumbContexts[breadcrumbContext];
						contextStrings[context.BreadcrumbIndex] = context.pContextString;
					}

					for (int op = firstOp; op <= lastOp; ++op)
					{
						D3D12_AUTO_BREADCRUMB_OP breadcrumbOp = pNode->pCommandHistory[op];

						std::string contextString;
						auto it = contextStrings.find(op);
						if (it != contextStrings.end())
						{
							Protean::Helper::StringConvert(it->second, contextString);
						}

						const char* opName = (breadcrumbOp < arraysize(OpNames)) ? OpNames[breadcrumbOp] : "Unknown Op";
						log += "\tOp: " + std::to_string(op) + " " + opName + " " + contextString + "\n";
					}

					if (lastCompletedOp != (int)pNode->BreadcrumbCount)
					{
						log += "ERROR: lastCompletedOp = " + std::to_string(lastCompletedOp) + ", BreadcrumbCount = " + std::to_string((int)pNode->BreadcrumbCount) + "\n";
					}

					pNode = pNode->pNext;
				}
			}

			D3D12_DRED_PAGE_FAULT_OUTPUT1 DredPageFaultOutput = {};
			if (SUCCEEDED(pDred->GetPageFaultAllocationOutput1(&DredPageFaultOutput)) && DredPageFaultOutput.PageFaultVA != 0)
			{
				log += "[DRED] PageFault at VA GPUAddress " + std::to_string(DredPageFaultOutput.PageFaultVA) + "\n";

				const D3D12_DRED_ALLOCATION_NODE1* pNode = DredPageFaultOutput.pHeadExistingAllocationNode;
				if (pNode)
				{
					log += "[DRED] Active objects with VA ranges that match the faulting VA:\n";
					while (pNode)
					{
						uint32_t alloc_type_index = pNode->AllocationType - D3D12_DRED_ALLOCATION_TYPE_COMMAND_QUEUE;
						const char* AllocTypeName = (alloc_type_index < arraysize(AllocTypesNames)) ? AllocTypesNames[alloc_type_index] : "Unknown Alloc";
						char objectname[1024] = {};
						if (pNode->ObjectNameW != nullptr)
						{
							Protean::Helper::StringConvert(pNode->ObjectNameW, objectname, arraysize(objectname));
						}
						log += std::string("\tName: ") + objectname + std::string(" ") + AllocTypeName + std::string("\n");
						pNode = pNode->pNext;
					}
				}

				pNode = DredPageFaultOutput.pHeadRecentFreedAllocationNode;
				if (pNode)
				{
					log +=  "[DRED] Recent freed objects with VA ranges that match the faulting VA:\n";
					while (pNode)
					{
						uint32_t allocTypeIndex = pNode->AllocationType - D3D12_DRED_ALLOCATION_TYPE_COMMAND_QUEUE;
						const char* AllocTypeName = (allocTypeIndex < arraysize(AllocTypesNames)) ? AllocTypesNames[allocTypeIndex] : "Unknown Alloc";
						char objectname[1024] = {};
						if (pNode->ObjectNameW != nullptr)
						{
							Protean::Helper::StringConvert(pNode->ObjectNameW, objectname, arraysize(objectname));
						}
						log += std::string("\tName: ") + objectname + std::string(" (Type: ") + AllocTypeName + std::string(")\n");
						pNode = pNode->pNext;
					}
				}
			}
		}

		if (!log.empty())
		{
			Protean::Backlog::post(log, Protean::Backlog::LogLevel_Error);
		}

		std::string message = "D3D12: device removed, cause: ";
		message += removedReasonString;
		wilog_messagebox("%s", message.c_str());
		Protean::Platform::Exit();
#endif // PLATFORM_WINDOWS_DESKTOP
	}

	void D3D12Device::WaitForGPU() const
	{
		ComPtr<ID3D12Fence> fence;
		dx12_check(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, PPV_ARGS(fence)));

		for (auto& queue : queues)
		{
			if (queue.queue == nullptr)
				continue;
			fence->Signal(0);
			dx12_check(queue.queue->Signal(fence.Get(), 1));
			dx12_check(fence->SetEventOnCompletion(1, nullptr));
		}

		fence->Signal(0);
		dx12_check(copyAllocator.queue->Signal(fence.Get(), 1));
		dx12_check(fence->SetEventOnCompletion(1, nullptr));
	}

	void D3D12Device::ClearPipelineStateCache()
	{
		multidraw_signatures.clear();
		pipelines_global.clear();

		for (auto& x : commandlists)
		{
			x->pipelines_worker.clear();
		}
	}

	ID3D12Resource* D3D12Device::GetTextureInternalResource(const Texture* texture)
	{
		return to_internal(texture)->resource.Get();
	}

	ID3D12CommandQueue* D3D12Device::GetGraphicsCommandQueue()
	{
		return queues[QUEUE_GRAPHICS].queue.Get();
	}
}

#endif // WICKEDENGINE_BUILD_DX12
