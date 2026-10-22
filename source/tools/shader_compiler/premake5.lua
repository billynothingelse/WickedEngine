-- Offline shader compiler
project "ShaderCompiler"
	kind "ConsoleApp"
	location (PROJECTS .. "/Tools")
	debugdir (SOURCE .. "/engine")
	files { SOURCE .. "/tools/shader_compiler/**.cpp" }
	UseEngine()
