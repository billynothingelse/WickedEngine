-- Writes the export definitions of the engine DLL from its object files (Windows)
project "DefGenerator"
	kind "ConsoleApp"
	location (PROJECTS .. "/Tools")
	targetname "def_generator"
	exceptionhandling "Default"
	rtti "Default"
	files { SOURCE .. "/tools/def_generator/**.cpp" }
