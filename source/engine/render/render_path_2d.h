#pragma once
#include "render/render_path.h"
#include "gui/gui.h"
#include "core/vector.h"
#include "render/video.h"

#include <string>

namespace Protean
{
	class Sprite;
	class SpriteFont;

	class RenderPath2D : public RenderPath
	{
	protected:
		Protean::Graphics::Texture rtStencilExtracted;
		Protean::Graphics::Texture stencilScaled;

		Protean::Graphics::Texture rtFinal;
		Protean::Graphics::Texture rtFinal_MSAA;

		Protean::Gui::GUI GUI;

		XMUINT2 current_buffersize{};
		float current_layoutscale{};

		float hdr_scaling = 9.0f;

		uint32_t msaaSampleCount = 1;
		uint32_t msaaSampleCount2D = 1;

		Protean::vector<Protean::Video::VideoInstance*> video_decodes;

	public:
		// Delete GPU resources and initialize them to default
		virtual void DeleteGPUResources();
		// create resolution dependent resources, such as render targets
		virtual void ResizeBuffers();
		// update DPI dependent elements, such as GUI elements, sprites
		virtual void ResizeLayout();

		void Update(float dt) override;
		void FixedUpdate() override;
		void PreRender() override;
		void Render() const override;
		void Compose(Protean::Graphics::CommandList cmd) const override;

		// MSAA sample count of main rendering
		//	This is the minimum sample count that is used for 3D and 2D rendering
		virtual void setMSAASampleCount(uint32_t value) { msaaSampleCount = value; }
		constexpr uint32_t getMSAASampleCount() const { return msaaSampleCount; }

		// MSAA sample count of overlay rendering
		//	This can be larger than 3D rendering if required since MSAA for 2D overlay can be cheaper to render
		virtual void setMSAASampleCount2D(uint32_t value) { msaaSampleCount2D = value; }
		constexpr uint32_t getMSAASampleCount2D() const { return msaaSampleCount2D; }

		// The 2D scene rendering that is used in Compose() to present the final image. In RenderPath3D this is only the 2D overlay with transparency
		const Protean::Graphics::Texture& GetRenderResult2D() const { return rtFinal; }

		// The depth stencil is not used in pure 2D rendering, but when it's also a RenderPath3D it is available (not texture compatible, only depth-stencil)
		virtual const Protean::Graphics::Texture* GetDepthStencil() const { return nullptr; }

		// The blurred GUI background is not available in pure 2D rendering, but other render path can make it available, such as RenderPath3D
		virtual const Protean::Graphics::Texture* GetGUIBlurredBackground() const { return nullptr; }

		// Adds a sprite to be rendered. The pointer lifetime is managed by the user, it must be valid as long as it is used by the RenderPath2D
		//	layer is optional, it can be used to manage overlapping render groups
		void AddSprite(Protean::Sprite* sprite, const std::string& layer = "");
		void RemoveSprite(Protean::Sprite* sprite);
		void ClearSprites();
		int GetSpriteOrder(Protean::Sprite* sprite);

		// Adds a SpriteFont to be rendered. The pointer lifetime is managed by the user, it must be valid as long as it is used by the RenderPath2D
		//	layer is optional, it can be used to manage overlapping render groups
		void AddFont(Protean::SpriteFont* font, const std::string& layer = "");
		void RemoveFont(Protean::SpriteFont* font);
		void ClearFonts();
		int GetFontOrder(Protean::SpriteFont* font);

		// Adds a sprite showing a video, the pointer lifetimes are managed by the user, they must be valid as long as they are used by the RenderPath2D
		//	layer is optional, it can be used to manage overlapping render groups
		void AddVideoSprite(Protean::Video::VideoInstance* videoinstance, Protean::Sprite* sprite, const std::string& layer = "");

		struct RenderItem2D
		{
			Protean::Sprite* sprite = nullptr;
			Protean::SpriteFont* font = nullptr;
			Protean::Video::VideoInstance* videoinstance = nullptr;
			int order = 0;
		};
		struct RenderLayer2D
		{
			Protean::vector<RenderItem2D> items;
			std::string name;
			int order = 0;
		};
		Protean::vector<RenderLayer2D> layers{ 1 };

		// Creates a new layer, for managing overlapping render groups
		void AddLayer(const std::string& name);

		// Set the render order of a layer (smallest layer renders earliest, greatest layer renders latest)
		void SetLayerOrder(const std::string& name, int order);

		// Set the render order of a specific sprite within its layer
		void SetSpriteOrder(Protean::Sprite* sprite, int order);

		// Set the render order of a specific font within its layer
		void SetFontOrder(Protean::SpriteFont* font, int order);

		// Sorts all layers based on their order
		void SortLayers();

		// Removes null items from all layers
		void CleanLayers();

		// Deletes all layers
		void DeleteLayers();

		const Protean::Gui::GUI& GetGUI() const { return GUI; }
		Protean::Gui::GUI& GetGUI() { return GUI; }

		float resolutionScale = 1.0f;
		XMUINT2 GetInternalResolution() const
		{
			return XMUINT2(
				uint32_t((float)GetPhysicalWidth() * resolutionScale),
				uint32_t((float)GetPhysicalHeight() * resolutionScale)
			);
		}

		float GetHDRScaling() const { return hdr_scaling; }
		void SetHDRScaling(float value) { hdr_scaling = value; }

		// This is an identifier of RenderPath subtype that is used for lua binding.
		static constexpr const auto script_check_identifier = relative_path_storage(__FILE__);
		const char* GetScriptBindingID() const override { return script_check_identifier.c_str(); }
	};

}
