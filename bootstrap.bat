@echo off
rem ---------------------------------------------------------------------------
rem  Protean bootstrap: fetches premake, generates the Visual Studio solution and
rem  builds it.
rem
rem    bootstrap.bat [Debug|Development|Shipping] [--generate-only] [--vs2022|--vs2026]
rem
rem  premake 5.0.0+ is expected at build\premake5.exe and downloaded if missing.
rem  The Visual Studio version is detected with vswhere unless given explicitly.
rem
rem  Output:
rem    build\projects\Protean.slnx  generated solution (Protean.sln for --vs2022)
rem    build\temp\                  intermediate files
rem    binaries\windows-x64-<config>\  executables and libraries
rem ---------------------------------------------------------------------------
setlocal EnableDelayedExpansion

set ROOT=%~dp0
set ROOT=%ROOT:~0,-1%
set CONFIG=Development
set GENERATE_ONLY=0
set PREMAKE_VERSION=5.0.0
set PREMAKE=%ROOT%\build\premake5.exe
set VS_ACTION=
set VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe

for %%A in (%*) do (
	if /I "%%A"=="Debug" set CONFIG=Debug
	if /I "%%A"=="Development" set CONFIG=Development
	if /I "%%A"=="Shipping" set CONFIG=Shipping
	if /I "%%A"=="--generate-only" set GENERATE_ONLY=1
	if /I "%%A"=="--vs2022" set VS_ACTION=vs2022
	if /I "%%A"=="--vs2026" set VS_ACTION=vs2026
)

if not exist "%PREMAKE%" (
	echo [Protean] Downloading premake %PREMAKE_VERSION%...
	curl -fsSL -o "%ROOT%\build\premake.zip" "https://github.com/premake/premake-core/releases/download/v%PREMAKE_VERSION%/premake-%PREMAKE_VERSION%-windows.zip"
	if errorlevel 1 goto :error
	tar -xf "%ROOT%\build\premake.zip" -C "%ROOT%\build" premake5.exe
	if errorlevel 1 goto :error
	del "%ROOT%\build\premake.zip"
)

if not defined VS_ACTION (
	if exist "%VSWHERE%" (
		for /f "usebackq tokens=*" %%V in (`"%VSWHERE%" -latest -prerelease -products * -property catalog_productLineVersion`) do set VS_YEAR=%%V
	)
	if "!VS_YEAR!"=="2022" (set VS_ACTION=vs2022) else (set VS_ACTION=vs2026)
)

echo [Protean] Generating %VS_ACTION% solution...
pushd "%ROOT%\build"
"%PREMAKE%" %VS_ACTION%
set RESULT=%errorlevel%
popd
if not "%RESULT%"=="0" goto :error

set SOLUTION=%ROOT%\build\projects\Protean.slnx
if "%VS_ACTION%"=="vs2022" set SOLUTION=%ROOT%\build\projects\Protean.sln

if "%GENERATE_ONLY%"=="1" (
	echo [Protean] Solution generated: !SOLUTION!
	exit /b 0
)

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
"%MSBUILD%" "%SOLUTION%" /m /nologo /v:minimal /p:Configuration=%CONFIG% /p:Platform=x64
if errorlevel 1 goto :error

echo [Protean] Done: binaries\windows-x64-%CONFIG%\
exit /b 0

:error
echo [Protean] Bootstrap failed.
exit /b 1
