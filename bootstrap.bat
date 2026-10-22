@echo off
rem ---------------------------------------------------------------------------
rem  Protean bootstrap: fetches premake, generates the Visual Studio solution and
rem  builds it.
rem
rem    bootstrap.bat [Debug|Development|Shipping] [--generate-only]
rem
rem  Output:
rem    build\projects\Protean.sln   generated solution
rem    build\temp\                  intermediate files
rem    build\bin\<Config>\          executables
rem ---------------------------------------------------------------------------
setlocal EnableDelayedExpansion

set ROOT=%~dp0
set ROOT=%ROOT:~0,-1%
set CONFIG=Development
set GENERATE_ONLY=0
set PREMAKE_VERSION=5.0.0-beta7
set PREMAKE_DIR=%ROOT%\build\tools
set PREMAKE=%PREMAKE_DIR%\premake5.exe
set VS_ACTION=vs2022

for %%A in (%*) do (
	if /I "%%A"=="Debug" set CONFIG=Debug
	if /I "%%A"=="Development" set CONFIG=Development
	if /I "%%A"=="Shipping" set CONFIG=Shipping
	if /I "%%A"=="--generate-only" set GENERATE_ONLY=1
)

if not exist "%PREMAKE%" (
	echo [Protean] Downloading premake %PREMAKE_VERSION%...
	if not exist "%PREMAKE_DIR%" mkdir "%PREMAKE_DIR%"
	curl -fsSL -o "%PREMAKE_DIR%\premake.zip" "https://github.com/premake/premake-core/releases/download/v%PREMAKE_VERSION%/premake-%PREMAKE_VERSION%-windows.zip"
	if errorlevel 1 goto :error
	tar -xf "%PREMAKE_DIR%\premake.zip" -C "%PREMAKE_DIR%"
	if errorlevel 1 goto :error
	del "%PREMAKE_DIR%\premake.zip"
)

echo [Protean] Generating %VS_ACTION% solution...
pushd "%ROOT%\build"
"%PREMAKE%" %VS_ACTION%
set RESULT=%errorlevel%
popd
if not "%RESULT%"=="0" goto :error

if "%GENERATE_ONLY%"=="1" (
	echo [Protean] Solution generated: build\projects\Protean.sln
	exit /b 0
)

set VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe
if not exist "%VSWHERE%" (
	echo [Protean] vswhere.exe not found, is Visual Studio installed?
	goto :error
)
for /f "usebackq tokens=*" %%I in (`"%VSWHERE%" -latest -prerelease -products * -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe`) do set MSBUILD=%%I
if not defined MSBUILD (
	echo [Protean] MSBuild not found.
	goto :error
)

echo [Protean] Building %CONFIG%^|x64...
"%MSBUILD%" "%ROOT%\build\projects\Protean.sln" /m /nologo /v:minimal /p:Configuration=%CONFIG% /p:Platform=x64
if errorlevel 1 goto :error

echo [Protean] Done: build\bin\%CONFIG%\
exit /b 0

:error
echo [Protean] Bootstrap failed.
exit /b 1
