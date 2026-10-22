#include "render/render_path_3d_path_tracing.h"
#include "render/renderer.h"
#include "render/image.h"
#include "core/helper.h"
#include "render/texture_helper.h"
#include "render/sprite.h"
#include "debug/profiler.h"
#include "scene/scene.h"
#include "debug/backlog.h"

#if __has_include("OpenImageDenoise/oidn.hpp")
#include "OpenImageDenoise/oidn.hpp"
#if OIDN_VERSION_MAJOR >= 2
#define OPEN_IMAGE_DENOISE
#ifdef _WIN32
#pragma comment(lib,"OpenImageDenoise.lib")
#endif
// Also provide the required DLL files from OpenImageDenoise release near the exe!
#endif // OIDN_VERSION_MAJOR >= 2
#endif // __has_include("OpenImageDenoise/oidn.hpp")

using namespace Protean::Graphics;
using namespace Protean;


namespace Protean
{
#ifdef OPEN_IMAGE_DENOISE
	bool DenoiserCallback(void* userPtr, double n)
	{
		auto renderpath = (RenderPath3D_PathTracing*)userPtr;
		if (renderpath->getProgress() < 1)
		{
			renderpath->denoiserProgress = 0;
			return false;
		}
		renderpath->denoiserProgress = (float)n;
		return true;
	}
	bool RenderPath3D_PathTracing::isDenoiserAvailable() const { return true; }
#else
	bool RenderPath3D_PathTracing::isDenoiserAvailable() const { return false; }
#endif // OPEN_IMAGE_DENOISE

	RenderPath3D_PathTracing::~RenderPath3D_PathTracing()
	{
		Protean::JobSystem::Wait(denoiserContext);
	}

	void RenderPath3D_PathTracing::ResizeBuffers()
	{
		DeleteGPUResources();

		GraphicsDevice* device = Protean::Graphics::GetDevice();

		XMUINT2 internalResolution = GetInternalResolution();

		{
			TextureDesc desc;
			desc.format = Protean::Renderer::format_rendertarget_main;
			desc.bind_flags = BindFlag_RENDER_TARGET | BindFlag_SHADER_RESOURCE | BindFlag_UNORDERED_ACCESS;
			desc.width = internalResolution.x;
			desc.height = internalResolution.y;
			device->CreateTexture(&desc, nullptr, &rtMain);
			device->SetName(&rtMain, "pathtracing.rtMain");

			desc.format = Protean::Renderer::format_idbuffer;
			desc.bind_flags = BindFlag_UNORDERED_ACCESS | BindFlag_SHADER_RESOURCE;
			desc.width = internalResolution.x;
			desc.height = internalResolution.y;
			desc.sample_count = 1;
			device->CreateTexture(&desc, nullptr, &rtPrimitiveID);
			device->SetName(&rtPrimitiveID, "pathtracing.rtPrimitiveID");

			desc.bind_flags = BindFlag_UNORDERED_ACCESS | BindFlag_SHADER_RESOURCE | BindFlag_RENDER_TARGET;
			desc.format = Format_R32G32B32A32_FLOAT;
			desc.width = internalResolution.x;
			desc.height = internalResolution.y;
			device->CreateTexture(&desc, nullptr, &traceResult);
			device->SetName(&traceResult, "pathtracing.traceResult");

#ifdef OPEN_IMAGE_DENOISE
			desc.bind_flags = BindFlag_UNORDERED_ACCESS;
			desc.layout = ResourceState_UNORDERED_ACCESS;
			device->CreateTexture(&desc, nullptr, &denoiserAlbedo);
			device->SetName(&denoiserAlbedo, "pathtracing.denoiserAlbedo");
			device->CreateTexture(&desc, nullptr, &denoiserNormal);
			device->SetName(&denoiserNormal, "pathtracing.denoiserNormal");
#endif // OPEN_IMAGE_DENOISE

			{
				desc.bind_flags = BindFlag_UNORDERED_ACCESS | BindFlag_SHADER_RESOURCE;
				desc.format = Format_R32_FLOAT;
				device->CreateTexture(&desc, nullptr, &traceDepth);
				device->SetName(&traceDepth, "pathtracing.traceDepth");
				desc.format = Format_R8_UINT;
				device->CreateTexture(&desc, nullptr, &traceStencil);
				device->SetName(&traceStencil, "pathtracing.traceStencil");

				depthBuffer_Copy = traceDepth;
				depthBuffer_Copy1 = traceDepth;

				desc.layout = ResourceState_DEPTHSTENCIL;
				desc.bind_flags = BindFlag_DEPTH_STENCIL;
				desc.format = Protean::Renderer::format_depthbuffer_main;
				device->CreateTexture(&desc, nullptr, &depthBuffer_Main);
				device->SetName(&depthBuffer_Main, "pathtracing.depthBuffer_Main");
			}
		}
		{
			TextureDesc desc;
			desc.bind_flags = BindFlag_SHADER_RESOURCE | BindFlag_UNORDERED_ACCESS;
			desc.format = Format_R11G11B10_FLOAT;
			desc.width = internalResolution.x;
			desc.height = internalResolution.y;
			device->CreateTexture(&desc, nullptr, &rtPostprocess);
			device->SetName(&rtPostprocess, "pathtracing.rtPostprocess");


			desc.width /= 4;
			desc.height /= 4;
			desc.bind_flags = BindFlag_UNORDERED_ACCESS | BindFlag_SHADER_RESOURCE;
			device->CreateTexture(&desc, nullptr, &rtGUIBlurredBackground[0]);
			device->SetName(&rtGUIBlurredBackground[0], "pathtracing.rtGUIBlurredBackground[0]");

			desc.width /= 4;
			desc.height /= 4;
			device->CreateTexture(&desc, nullptr, &rtGUIBlurredBackground[1]);
			device->SetName(&rtGUIBlurredBackground[1], "pathtracing.rtGUIBlurredBackground[1]");
			device->CreateTexture(&desc, nullptr, &rtGUIBlurredBackground[2]);
			device->SetName(&rtGUIBlurredBackground[2], "pathtracing.rtGUIBlurredBackground[2]");
		}

		Protean::Renderer::CreateLuminanceResources(luminanceResources, internalResolution);
		Protean::Renderer::CreateBloomResources(bloomResources, internalResolution);

		setLightShaftsEnabled(getLightShaftsEnabled());
		setVolumeLightsEnabled(getVolumeLightsEnabled());

		resetProgress();

		RenderPath2D::ResizeBuffers(); // we don't need to use any buffers from RenderPath3D, so skip those
	}

	void RenderPath3D_PathTracing::Update(float dt)
	{
		setOcclusionCullingEnabled(false);

		if (camera->IsDirty())
		{
			camera->SetDirty(false);
			resetProgress();
		}
		else
		{
			for (size_t i = 0; i < scene->transforms.GetCount(); ++i)
			{
				const TransformComponent& transform = scene->transforms[i];

				if (transform.IsDirty())
				{
					sam = -1;
					break;
				}
			}

			if (sam >= 0)
			{
				for (size_t i = 0; i < scene->materials.GetCount(); ++i)
				{
					const MaterialComponent& material = scene->materials[i];

					if (material.IsDirty())
					{
						sam = -1;
						break;
					}
				}
			}
		}
		sam++;

		if (sam > target)
		{
			sam = target;
		}

		scene->SetAccelerationStructureUpdateRequested(sam == 0);
		setSceneUpdateEnabled(sam == 0);

		RenderPath3D::Update(dt);

#ifdef OPEN_IMAGE_DENOISE
		if (sam == target)
		{
			if (!denoiserResult.IsValid() && !Protean::JobSystem::IsBusy(denoiserContext))
			{
				//Protean::Helper::saveTextureToFile(denoiserAlbedo, "C:/PROJECTS/WickedEngine/Editor/_albedo.png");
				//Protean::Helper::saveTextureToFile(denoiserNormal, "C:/PROJECTS/WickedEngine/Editor/_normal.png");

				texturedata_src.clear();
				texturedata_dst.clear();
				texturedata_albedo.clear();
				texturedata_normal.clear();

				if (Protean::Helper::saveTextureToMemory(traceResult, texturedata_src))
				{
					Protean::Helper::saveTextureToMemory(denoiserAlbedo, texturedata_albedo);
					Protean::Helper::saveTextureToMemory(denoiserNormal, texturedata_normal);

					texturedata_dst.resize(texturedata_src.size());

					Protean::JobSystem::Execute(denoiserContext, [&](Protean::JobSystem::JobArgs args) {

						size_t width = (size_t)traceResult.desc.width;
						size_t height = (size_t)traceResult.desc.height;
						{
							// https://github.com/OpenImageDenoise/oidn#c11-api-example

							// Create an Intel Open Image Denoise device
							static oidn::DeviceRef device = oidn::newDevice();
							static bool init = false;
							if (!init)
							{
								device.commit();
								init = true;
							}

							oidn::BufferRef texturedata_src_buffer = device.newBuffer(texturedata_src.size());
							oidn::BufferRef texturedata_dst_buffer = device.newBuffer(texturedata_dst.size());
							oidn::BufferRef texturedata_albedo_buffer;
							oidn::BufferRef texturedata_normal_buffer;

							texturedata_src_buffer.write(0, texturedata_src.size(), texturedata_src.data());

							// Create a denoising filter
							oidn::FilterRef filter = device.newFilter("RT"); // generic ray tracing filter
							filter.setImage("color", texturedata_src_buffer, oidn::Format::Float3, width, height, 0, sizeof(XMFLOAT4));
							if (!texturedata_albedo.empty())
							{
								texturedata_albedo_buffer = device.newBuffer(texturedata_albedo.size());
								texturedata_albedo_buffer.write(0, texturedata_albedo.size(), texturedata_albedo.data());
								filter.setImage("albedo", texturedata_albedo_buffer, oidn::Format::Float3, width, height, 0, sizeof(XMFLOAT4)); // optional
							}
							if (!texturedata_normal.empty())
							{
								texturedata_normal_buffer = device.newBuffer(texturedata_normal.size());
								texturedata_normal_buffer.write(0, texturedata_normal.size(), texturedata_normal.data());
								filter.setImage("normal", texturedata_normal_buffer, oidn::Format::Float3, width, height, 0, sizeof(XMFLOAT4)); // optional
							}
							filter.setImage("output", texturedata_dst_buffer, oidn::Format::Float3, width, height, 0, sizeof(XMFLOAT4));
							filter.set("hdr", true); // image is HDR
							//filter.set("cleanAux", true);
							filter.commit();

							denoiserProgress = 0;
							filter.setProgressMonitorFunction(DenoiserCallback, this);

							// Filter the image
							filter.execute();

							// Check for errors
							const char* errorMessage;
							auto error = device.getError(errorMessage);
							if (error != oidn::Error::None && error != oidn::Error::Cancelled)
							{
								Protean::Backlog::post(std::string("[OpenImageDenoise error] ") + errorMessage);
							}
							else
							{
								texturedata_dst_buffer.read(0, texturedata_dst.size(), texturedata_dst.data());
							}
						}

						GraphicsDevice* device = Protean::Graphics::GetDevice();

						TextureDesc desc;
						desc.width = (uint32_t)width;
						desc.height = (uint32_t)height;
						desc.bind_flags = BindFlag_SHADER_RESOURCE;
						desc.format = Format_R32G32B32A32_FLOAT;

						SubresourceData initdata;
						initdata.data_ptr = texturedata_dst.data();
						initdata.row_pitch = uint32_t(sizeof(XMFLOAT4) * width);
						device->CreateTexture(&desc, &initdata, &denoiserResult);
						device->SetName(&denoiserResult, "pathtracing.denoiserResult");

						});
				}
			}
		}
		else
		{
			Protean::JobSystem::Wait(denoiserContext);
			denoiserResult = {};
			denoiserProgress = 0;
		}
#endif // OPEN_IMAGE_DENOISE
	}

	void RenderPath3D_PathTracing::Render() const
	{
		GraphicsDevice* device = Protean::Graphics::GetDevice();
		Protean::JobSystem::context ctx;

		if (sam < target)
		{
			CommandList cmd_copypages;
			if (scene->terrains.GetCount() > 0)
			{
				cmd_copypages = device->BeginCommandList(QUEUE_COPY);
				Protean::JobSystem::Execute(ctx, [this, cmd_copypages](Protean::JobSystem::JobArgs args) {
					for (size_t i = 0; i < scene->terrains.GetCount(); ++i)
					{
						scene->terrains[i].CopyVirtualTexturePageStatusGPU(cmd_copypages);
					}
					});
			}

			// Setup:
			CommandList cmd = device->BeginCommandList();
			Protean::JobSystem::Execute(ctx, [this, cmd](Protean::JobSystem::JobArgs args) {

				Protean::Renderer::BindCameraCB(
					*camera,
					camera_previous,
					camera_reflection,
					cmd
				);
				Protean::Renderer::UpdateRenderData(visibility_main, frameCB, cmd);

				if (scene->IsAccelerationStructureUpdateRequested())
				{
					Protean::Renderer::UpdateRaytracingAccelerationStructures(*scene, cmd);
				}
			});

			// Main scene:
			cmd = device->BeginCommandList();
			if (cmd_copypages.IsValid())
			{
				device->WaitCommandList(cmd, cmd_copypages);
			}
			Protean::JobSystem::Execute(ctx, [this, cmd](Protean::JobSystem::JobArgs args) {

				GraphicsDevice* device = Protean::Graphics::GetDevice();

				Protean::Renderer::BindCameraCB(
					*camera,
					camera_previous,
					camera_reflection,
					cmd
				);
				Protean::Renderer::BindCommonResources(cmd);
				Protean::Renderer::UpdateRenderDataAsync(visibility_main, frameCB, cmd);

				// just clear depth for safety:
				RenderPassImage rp[] = {
					RenderPassImage::DepthStencil(&depthBuffer_Main, RenderPassImage::LoadOp_CLEAR),
				};
				device->RenderPassBegin(rp, arraysize(rp), cmd);
				device->RenderPassEnd(cmd);

				if (scene->weather.IsRealisticSky())
				{
					Protean::Renderer::ComputeSkyAtmosphereTextures(cmd);
					Protean::Renderer::ComputeSkyAtmosphereSkyViewLut(cmd);
				}

				if (Protean::Renderer::GetRaytraceDebugBVHVisualizerEnabled())
				{
					RenderPassImage rp[] = {
						RenderPassImage::RenderTarget(&traceResult, RenderPassImage::LoadOp_CLEAR)
					};
					device->RenderPassBegin(rp, arraysize(rp), cmd);

					Viewport vp;
					vp.width = (float)traceResult.GetDesc().width;
					vp.height = (float)traceResult.GetDesc().height;
					device->BindViewports(1, &vp, cmd);

					Protean::Renderer::RayTraceSceneBVH(*scene, cmd);

					device->RenderPassEnd(cmd);
				}
				else
				{
					auto range = Protean::Profiler::BeginRangeGPU("Traced Scene", cmd);

					Protean::Renderer::RayTraceScene(
						*scene,
						traceResult,
						sam,
						cmd,
						denoiserAlbedo.IsValid() ? &denoiserAlbedo : nullptr,
						denoiserNormal.IsValid() ? &denoiserNormal : nullptr,
						&traceDepth,
						&traceStencil,
						&depthBuffer_Main,
						&rtPrimitiveID
					);

					Protean::Profiler::EndRange(range); // Traced Scene

					if (sam == 0 && getMeshBlendEnabled() && visibility_main.IsMeshBlendVisible())
					{
						Protean::Renderer::PostProcess_MeshBlend_EdgeProcess(meshblendResources, cmd);
					}
				}

				if (getVolumeLightsEnabled() && visibility_main.IsRequestedVolumetricLights())
				{
					Protean::Renderer::DrawShadowmaps(visibility_main, cmd);
				}

				if (scene->weather.IsVolumetricClouds())
				{
					Protean::Renderer::Postprocess_VolumetricClouds(
						volumetriccloudResources,
						cmd,
						*camera,
						camera_previous,
						camera_reflection,
						false,
						scene->weather.volumetricCloudsWeatherMapFirst.IsValid() ? &scene->weather.volumetricCloudsWeatherMapFirst.GetTexture() : nullptr,
						scene->weather.volumetricCloudsWeatherMapSecond.IsValid() ? &scene->weather.volumetricCloudsWeatherMapSecond.GetTexture() : nullptr
					);
				}

			});

			if (scene->terrains.GetCount() > 0)
			{
				CommandList cmd_allocation_tilerequest = device->BeginCommandList(QUEUE_COMPUTE);
				device->WaitCommandList(cmd_allocation_tilerequest, cmd); // wait for opaque scene
				Protean::JobSystem::Execute(ctx, [this, cmd_allocation_tilerequest](Protean::JobSystem::JobArgs args) {
					for (size_t i = 0; i < scene->terrains.GetCount(); ++i)
					{
						scene->terrains[i].AllocateVirtualTextureTileRequestsGPU(cmd_allocation_tilerequest);
					}
					});

				CommandList cmd_writeback_tilerequest = device->BeginCommandList(QUEUE_COPY);
				device->WaitCommandList(cmd_writeback_tilerequest, cmd_allocation_tilerequest);
				Protean::JobSystem::Execute(ctx, [this, cmd_writeback_tilerequest](Protean::JobSystem::JobArgs args) {
					for (size_t i = 0; i < scene->terrains.GetCount(); ++i)
					{
						scene->terrains[i].WritebackTileRequestsGPU(cmd_writeback_tilerequest);
					}
					});
			}
		}

		// Composite, tonemap etc:
		CommandList cmd = device->BeginCommandList();
		Protean::JobSystem::Execute(ctx, [this, cmd](Protean::JobSystem::JobArgs args) {

			GraphicsDevice* device = Protean::Graphics::GetDevice();

			Protean::Renderer::BindCameraCB(
				*camera,
				camera_previous,
				camera_reflection,
				cmd
			);
			Protean::Renderer::BindCommonResources(cmd);

			if (scene->weather.IsRealisticSky())
			{
				Protean::Renderer::ComputeSkyAtmosphereSkyViewLut(cmd);

				if (scene->weather.IsRealisticSkyAerialPerspective())
				{
					Protean::Renderer::ComputeSkyAtmosphereCameraVolumeLut(cmd);
				}
			}
			if (scene->weather.IsRealisticSky() && scene->weather.IsRealisticSkyAerialPerspective())
			{
				Protean::Renderer::Postprocess_AerialPerspective(
					aerialperspectiveResources,
					cmd
				);
			}

			RenderVolumetrics(cmd);

			RenderLightShafts(cmd);

			// Composite other effects on top:
			{
				RenderPassImage rp[] = {
					RenderPassImage::RenderTarget(&rtMain, RenderPassImage::LoadOp_CLEAR),
					RenderPassImage::DepthStencil(&depthBuffer_Main, RenderPassImage::LoadOp_LOAD),
				};
				device->RenderPassBegin(rp, arraysize(rp), cmd);

				Viewport vp;
				vp.width = (float)rtMain.GetDesc().width;
				vp.height = (float)rtMain.GetDesc().height;
				device->BindViewports(1, &vp, cmd);

				// Clear to trace result:
				{
					device->EventBegin("Clear to trace result", cmd);
					Protean::Image::Params fx;
					fx.enableFullScreen();
					fx.blendFlag = Protean::BLENDMODE_OPAQUE;
					if (denoiserResult.IsValid() && !Protean::JobSystem::IsBusy(denoiserContext))
					{
						Protean::Image::Draw(&denoiserResult, fx, cmd);
					}
					else
					{
						Protean::Image::Draw(&traceResult, fx, cmd);
					}
					device->EventEnd(cmd);
				}

				// Blend Aerial Perspective on top:
				if (scene->weather.IsRealisticSky() && scene->weather.IsRealisticSkyAerialPerspective())
				{
					device->EventBegin("Aerial Perspective Blend", cmd);
					Protean::Image::Params fx;
					fx.enableFullScreen();
					fx.blendFlag = Protean::BLENDMODE_PREMULTIPLIED;
					Protean::Image::Draw(&aerialperspectiveResources.texture_output, fx, cmd);
					device->EventEnd(cmd);
				}

				// Blend the volumetric clouds on top:
				if (scene->weather.IsVolumetricClouds())
				{
					Protean::Renderer::Postprocess_VolumetricClouds_Upsample(volumetriccloudResources, cmd);
				}

				Protean::Renderer::DrawDebugWorld(*scene, *camera, *this, cmd);
				Protean::Renderer::DrawLightVisualizers(visibility_main, cmd);
				Protean::Renderer::DrawSpritesAndFonts(*scene, *camera, false, cmd);

				if (getVolumeLightsEnabled() && visibility_main.IsRequestedVolumetricLights())
				{
					device->EventBegin("Contribute Volumetric Lights", cmd);
					Protean::Renderer::Postprocess_Upsample_Bilateral(
						rtVolumetricLights,
						depthBuffer_Copy,
						rtMain,
						cmd,
						true,
						1.5f
					);
					device->EventEnd(cmd);
				}

				XMVECTOR sunDirection = XMLoadFloat3(&scene->weather.sunDirection);
				if (getLightShaftsEnabled() && XMVectorGetX(XMVector3Dot(sunDirection, camera->GetAt())) > 0)
				{
					device->EventBegin("Contribute LightShafts", cmd);
					Protean::Image::Params fx;
					fx.enableFullScreen();
					fx.blendFlag = Protean::BLENDMODE_ADDITIVE;
					Protean::Image::Draw(&rtSun[1], fx, cmd);
					device->EventEnd(cmd);
				}
				if (getLensFlareEnabled())
				{
					Protean::Renderer::DrawLensFlares(
						visibility_main,
						cmd
					);
				}

				device->RenderPassEnd(cmd);

				if (getMeshBlendEnabled() && visibility_main.IsMeshBlendVisible())
				{
					rp[0].loadop = RenderPassImage::LoadOp_LOAD;
					Protean::Renderer::PostProcess_MeshBlend_Resolve(meshblendResources, rtMain, rp, arraysize(rp), cmd);
				}
			}

			// Post processing:
			const Texture* rt_read = &rtMain;
			const Texture* rt_write = &rtPostprocess;

			for (auto& x : custom_post_processes)
			{
				if (x.stage == CustomPostprocess::Stage_BeforeTonemap)
				{
					Protean::Renderer::Postprocess_Custom(
						x.computeshader,
						*rt_read,
						*rt_write,
						cmd,
						x.params0,
						x.params1,
						x.name.c_str()
					);

					std::swap(rt_read, rt_write);
				}
			}

			if (getEyeAdaptionEnabled())
			{
				Protean::Renderer::ComputeLuminance(
					luminanceResources,
					*rt_read,
					cmd,
					getEyeAdaptionRate(),
					getEyeAdaptionKey()
				);
			}
			if (getBloomEnabled())
			{
				Protean::Renderer::ComputeBloom(
					bloomResources,
					*rt_read,
					cmd,
					getBloomThreshold(),
					getExposure(),
					getEyeAdaptionEnabled() ? &luminanceResources.luminance : nullptr
				);
			}

			Protean::Renderer::Postprocess_Tonemap(
				*rt_read,
				*rt_write,
				cmd,
				getExposure(),
				getBrightness(),
				getContrast(),
				getSaturation(),
				getDitherEnabled(),
				getColorGradingEnabled() ? (scene->weather.colorGradingMap.IsValid() ? &scene->weather.colorGradingMap.GetTexture() : nullptr) : nullptr,
				nullptr,
				getEyeAdaptionEnabled() ? &luminanceResources.luminance : nullptr,
				getBloomEnabled() ? &bloomResources.texture_bloom : nullptr,
				colorspace,
				getTonemap(),
				&distortion_overlay,
				getHDRCalibration()
			);
			std::swap(rt_read, rt_write);

			for (auto& x : custom_post_processes)
			{
				if (x.stage == CustomPostprocess::Stage_AfterTonemap)
				{
					Protean::Renderer::Postprocess_Custom(
						x.computeshader,
						*rt_read,
						*rt_write,
						cmd,
						x.params0,
						x.params1,
						x.name.c_str()
					);

					std::swap(rt_read, rt_write);
				}
			}

			if (getSharpenFilterEnabled())
			{
				Protean::Renderer::Postprocess_Sharpen(*rt_read, *rt_write, cmd, getSharpenFilterAmount());

				std::swap(rt_read, rt_write);
			}

			if (getFXAAEnabled())
			{
				Protean::Renderer::Postprocess_FXAA(*rt_read, *rt_write, cmd);

				std::swap(rt_read, rt_write);
			}

			if (getChromaticAberrationEnabled())
			{
				Protean::Renderer::Postprocess_Chromatic_Aberration(*rt_read, *rt_write, cmd, getChromaticAberrationAmount());

				std::swap(rt_read, rt_write);
			}

			if (getCRTFilterEnabled())
			{
				Protean::Renderer::Postprocess_CRT(*rt_read, *rt_write, cmd);

				std::swap(rt_read, rt_write);
			}

			lastPostprocessRT = rt_read;


			// GUI Background blurring:
			{
				auto range = Protean::Profiler::BeginRangeGPU("GUI Background Blur", cmd);
				device->EventBegin("GUI Background Blur", cmd);
				Protean::Renderer::Postprocess_Downsample4x(rtPostprocess, rtGUIBlurredBackground[0], cmd);
				Protean::Renderer::Postprocess_Downsample4x(rtGUIBlurredBackground[0], rtGUIBlurredBackground[2], cmd);
				Protean::Renderer::Postprocess_Blur_Gaussian(rtGUIBlurredBackground[2], rtGUIBlurredBackground[1], rtGUIBlurredBackground[2], cmd, -1, -1, true);
				device->EventEnd(cmd);
				Protean::Profiler::EndRange(range);
			}

			Protean::Renderer::TextureStreamingReadbackCopy(*scene, cmd);
		});

		RenderPath2D::Render();

		Protean::JobSystem::Wait(ctx);
	}

	void RenderPath3D_PathTracing::Compose(CommandList cmd) const
	{
		GraphicsDevice* device = Protean::Graphics::GetDevice();

		device->EventBegin("RenderPath3D_PathTracing::Compose", cmd);

		Protean::Renderer::BindCommonResources(cmd);

		Protean::Image::Params fx;
		fx.enableFullScreen();
		fx.blendFlag = Protean::BLENDMODE_OPAQUE;
		fx.quality = Protean::Image::QUALITY_LINEAR;
		Protean::Image::Draw(lastPostprocessRT, fx, cmd);

		device->EventEnd(cmd);

		RenderPath2D::Compose(cmd);
	}

}
