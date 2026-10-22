#pragma once
#include "render/RenderPath3D.h"
#include "core/Vector.h"

namespace Protean
{

	class RenderPath3D_PathTracing : public RenderPath3D
	{
	protected:
		int sam = -1;
		int target = 1024;
		Protean::Graphics::Texture traceResult;
		Protean::Graphics::Texture traceDepth;
		Protean::Graphics::Texture traceStencil;

		Protean::vector<uint8_t> texturedata_src;
		Protean::vector<uint8_t> texturedata_dst;
		Protean::vector<uint8_t> texturedata_albedo;
		Protean::vector<uint8_t> texturedata_normal;
		Protean::Graphics::Texture denoiserAlbedo;
		Protean::Graphics::Texture denoiserNormal;
		Protean::Graphics::Texture denoiserResult;
		Protean::JobSystem::context denoiserContext;

		void ResizeBuffers() override;

	public:

		~RenderPath3D_PathTracing();

		void Update(float dt) override;
		void Render() const override;
		void Compose(Protean::Graphics::CommandList cmd) const override;

		int getCurrentSampleCount() const { return sam; }
		void setTargetSampleCount(int value) { target = value; }
		float getProgress() const { return (float)sam / (float)target; }

		float denoiserProgress = 0;
		float getDenoiserProgress() const { return denoiserProgress; }
		bool isDenoiserAvailable() const;

		void resetProgress() { Protean::JobSystem::Wait(denoiserContext); sam = -1; denoiserProgress = 0; volumetriccloudResources.ResetFrame(); }

		uint8_t instanceInclusionMask_PathTrace = 0xFF;

		// This is an identifier of RenderPath subtype that is used for lua binding.
		static constexpr const auto script_check_identifier = relative_path_storage(__FILE__);
		const char* GetScriptBindingID() const override { return script_check_identifier.c_str(); }
	};

}
