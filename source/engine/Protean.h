#ifndef WICKEDENGINE
#define WICKEDENGINE
#define WICKED_ENGINE

// NOTE:
// The purpose of this file is to expose all engine features.
// It should be included in the engine's implementing application not the engine itself!
// It should be included in the precompiled header if available.

#include "core/CommonInclude.h"

// High-level interface:
#include "core/Application.h"
#include "render/RenderPath.h"
#include "render/RenderPath2D.h"
#include "render/RenderPath3D.h"
#include "render/RenderPath3DPathTracing.h"
#include "render/LoadingScreen.h"

// Engine-level systems
#include "core/Version.h"
#include "platform/Platform.h"
#include "debug/Backlog.h"
#include "math/Primitive.h"
#include "render/Image.h"
#include "render/Font.h"
#include "render/Sprite.h"
#include "render/SpriteFont.h"
#include "scene/Scene.h"
#include "core/ECS.h"
#include "render/EmittedParticle.h"
#include "render/HairParticle.h"
#include "render/Renderer.h"
#include "math/Math.h"
#include "audio/Audio.h"
#include "resource/ResourceManager.h"
#include "core/Timer.h"
#include "core/Helper.h"
#include "input/Input.h"
#include "input/RawInput.h"
#include "input/XInput.h"
#include "input/SDLInput.h"
#include "render/TextureHelper.h"
#include "math/Random.h"
#include "math/Color.h"
#include "physics/Physics.h"
#include "render/Enums.h"
#include "core/Initializer.h"
#include "script/Lua.h"
#include "render/GraphicsAPI/Graphics.h"
#include "render/GraphicsAPI/GraphicsDevice.h"
#include "gui/GUI.h"
#include "io/Archive.h"
#include "core/SpinLock.h"
#include "core/RectPacker.h"
#include "debug/Profiler.h"
#include "render/Ocean.h"
#include "render/FFTGenerator.h"
#include "core/Arguments.h"
#include "render/GPUBVH.h"
#include "render/GPUSortLib.h"
#include "core/JobSystem.h"
#include "network/Network.h"
#include "core/EventHandler.h"
#include "render/ShaderCompiler.h"
#include "render/Canvas.h"
#include "core/UnorderedMap.h"
#include "core/UnorderedSet.h"
#include "core/Vector.h"
#include "math/Noise.h"
#include "io/Config.h"
#include "scene/Terrain.h"
#include "core/Localization.h"
#include "render/Video.h"
#include "scene/VoxelGrid.h"
#include "scene/PathQuery.h"
#include "render/TrailRenderer.h"
#include "render/GaussianSplatModel.h"


// After version 0.59.11, namespaces were refactored into nested namespaces under the wi:: root namespace.
// To allow compatibility with older user code, the backwards compatibility definitions are included below.
// If you need backwards compatibility features, define the following before including this file:
//#define WICKEDENGINE_BACKWARDS_COMPATIBILITY_0_59
#ifdef WICKEDENGINE_BACKWARDS_COMPATIBILITY_0_59

using namespace wi;
using namespace wi::enums;

namespace wiGraphics = wi::graphics;
namespace wiShaderCompiler = wi::shadercompiler;
namespace wiFFTGenerator = wi::fftgenerator;
namespace wiFont = wi::font;
namespace wiImage = wi::image;
namespace wiGPUSortLib = wi::gpusortlib;
namespace wiRenderer = wi::renderer;
namespace wiTextureHelper = wi::texturehelper;
namespace wiHelper = wi::helper;
namespace wiMath = wi::math;
namespace wiRandom = wi::random;
namespace wiRectPacker = wi::rectpacker;
namespace wiResourceManager = wi::resourcemanager;
namespace wiStartupArguments = wi::arguments;
namespace wiInput = wi::input;
namespace wiXInput = wi::input::xinput;
namespace wiRawInput = wi::input::rawinput;
namespace wiSDLInput = wi::input::sdlinput;
namespace wiAudio = wi::audio;
namespace wiNetwork = wi::network;
namespace wiPhysicsEngine = wi::physics;
namespace wiLua = wi::lua;
namespace wiECS = wi::ecs;
namespace wiEvent = wi::eventhandler;
namespace wiInitializer = wi::initializer;
namespace wiJobSystem = wi::jobsystem;
namespace wiPlatform = wi::platform;
namespace wiScene = wi::scene;
namespace wiBackLog = wi::backlog;
namespace wiProfiler = wi::profiler;
namespace wiVersion = wi::version;

using MainComponent = wi::Application;
using wiFontParams = wi::font::Params;
using wiImageParams = wi::image::Params;
using wiGPUBVH = wi::GPUBVH;
using wiEmittedParticle = wi::EmittedParticleSystem;
using wiHairParticle = wi::HairParticleSystem;
using wiOcean = wi::Ocean;
using wiSprite = wi::Sprite;
using wiSpriteFont = wi::SpriteFont;
using wiGUI = wi::gui::GUI;
using wiEventArgs = wi::gui::EventArgs;
using wiWidget = wi::gui::Widget;
using wiButton = wi::gui::Button;
using wiLabel = wi::gui::Label;
using wiTextInputField = wi::gui::TextInputField;
using wiSlider = wi::gui::Slider;
using wiCheckBox = wi::gui::CheckBox;
using wiComboBox = wi::gui::ComboBox;
using wiWindow = wi::gui::Window;
using wiColorPicker = wi::gui::ColorPicker;
using wiTreeList = wi::gui::TreeList;
using wiArchive = wi::Archive;
using wiColor = wi::Color;
using wiFadeManager = wi::FadeManager;
using wiResource = wi::Resource;
using wiSpinLock = wi::SpinLock;
using wiTimer = wi::Timer;
using wiCanvas = wi::Canvas;
using AABB = wi::primitive::AABB;
using SPHERE = wi::primitive::Sphere;
using CAPSULE = wi::primitive::Capsule;
using RAY = wi::primitive::Ray;
using Frustum = wi::primitive::Frustum;
using Hitbox2D = wi::primitive::Hitbox2D;

using wi::image::STENCILMODE;
using wi::image::STENCILREFMODE;
using wi::image::SAMPLEMODE;
using wi::image::QUALITY;
using wi::font::Alignment;

static constexpr int SYSTEM_EVENT_THREAD_SAFE_POINT = wi::eventhandler::EVENT_THREAD_SAFE_POINT;
static constexpr int SYSTEM_EVENT_RELOAD_SHADERS = wi::eventhandler::EVENT_RELOAD_SHADERS;
static constexpr int SYSTEM_EVENT_SET_VSYNC = wi::eventhandler::EVENT_SET_VSYNC;
static constexpr XMFLOAT4X4 IDENTITYMATRIX = wi::math::IDENTITY_MATRIX;

#endif // WICKEDENGINE_BACKWARDS_COMPATIBILITY_0_59



#endif // WICKEDENGINE
