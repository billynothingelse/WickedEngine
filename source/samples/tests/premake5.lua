-- Engine feature tests
project "Tests"
	kind "WindowedApp"
	location (PROJECTS .. "/Samples")
	debugdir (SOURCE .. "/samples/tests")
	files { SOURCE .. "/samples/tests/*.h", SOURCE .. "/samples/tests/*.cpp" }
	removefiles { SOURCE .. "/samples/tests/main_*", SOURCE .. "/samples/tests/custom_shader_lifecycle_test.cpp" }
	UseEngine()
	filter "system:windows"
		files { SOURCE .. "/samples/tests/main_windows.cpp", SOURCE .. "/samples/tests/tests.rc" }
	filter "system:linux"
		files { SOURCE .. "/samples/tests/main_sdl2.cpp" }
	filter {}

-- Headless unit test, run it from any directory
project "CustomShaderLifecycleTest"
	kind "ConsoleApp"
	location (PROJECTS .. "/Samples")
	files { SOURCE .. "/samples/tests/custom_shader_lifecycle_test.cpp" }
	UseEngine()
