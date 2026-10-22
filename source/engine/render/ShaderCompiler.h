#pragma once
#include "render/GraphicsAPI/Graphics.h"
#include "core/Vector.h"
#include "core/Allocator.h"

#include <string>

namespace Protean::ShaderCompiler
{

	enum class Flags
	{
		NONE = 0,
		DISABLE_OPTIMIZATION = 1 << 0,
		STRIP_REFLECTION = 1 << 1,
		KEEP_DEBUG_INFORMATION = 1 << 2,
	};
	struct CompilerInput
	{
		Flags flags = Flags::NONE;
		Protean::Graphics::ShaderFormat format = Protean::Graphics::ShaderFormat::NONE;
		Protean::Graphics::ShaderStage stage = Protean::Graphics::ShaderStage::Count;
		// if the shader relies on a higher shader model feature, it must be declared here.
		//	But the compiler can also choose a higher one internally, if needed
		Protean::Graphics::ShaderModel minshadermodel = Protean::Graphics::ShaderModel::SM_5_0;
		std::string shadersourcefilename;
		std::string entrypoint = "main";
		Protean::vector<std::string> include_directories;
		Protean::vector<std::string> defines;
	};
	struct CompilerOutput
	{
		Protean::Allocator::shared_ptr<void> internal_state;
		constexpr bool IsValid() const { return internal_state.IsValid(); }

		const uint8_t* shaderdata = nullptr;
		size_t shadersize = 0;
		Protean::vector<uint8_t> shaderhash;
		std::string error_message;
		Protean::vector<std::string> dependencies;
	};
	void Compile(const CompilerInput& input, CompilerOutput& output);

	bool SaveShaderAndMetadata(const std::string& shaderfilename, const CompilerOutput& output);
	bool IsShaderOutdated(const std::string& shaderfilename);

	void RegisterShader(const std::string& shaderfilename);
	size_t GetRegisteredShaderCount();
	bool CheckRegisteredShadersOutdated();
}

template<>
struct enable_bitmask_operators<Protean::ShaderCompiler::Flags> : std::true_type {};
