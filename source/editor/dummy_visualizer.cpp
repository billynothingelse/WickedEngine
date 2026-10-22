#include "stdafx.h"
#include "dummy_visualizer.h"

using namespace Protean::Graphics;

struct DummyVisualizer
{
	Protean::Graphics::GPUBuffer buffer;
	Protean::AABB aabb;

	struct Vertex
	{
		XMFLOAT4 position;
		XMFLOAT4 color;
	};
	inline static PipelineState pso[2];

	DummyVisualizer(
		const XMFLOAT3* vertices,
		uint32_t vertices_count,
		const unsigned int* indices,
		uint32_t indices_count
	)
	{
		if (!pso[0].IsValid() || !pso[1].IsValid())
		{
			static auto LoadShaders = [] {
				PipelineStateDesc desc;
				desc.vs = Protean::Renderer::GetShader(Protean::VSTYPE_VERTEXCOLOR);
				desc.ps = Protean::Renderer::GetShader(Protean::PSTYPE_VERTEXCOLOR);
				desc.il = Protean::Renderer::GetInputLayout(Protean::ILTYPE_VERTEXCOLOR);
				desc.rs = Protean::Renderer::GetRasterizerState(Protean::RSTYPE_DOUBLESIDED);
				desc.bs = Protean::Renderer::GetBlendState(Protean::BSTYPE_TRANSPARENT);
				desc.pt = PrimitiveTopology_TRIANGLELIST;
				desc.dss = Protean::Renderer::GetDepthStencilState(Protean::DSSTYPE_DEPTHDISABLED);
				Protean::Graphics::GetDevice()->CreatePipelineState(&desc, &pso[0]);
				desc.dss = Protean::Renderer::GetDepthStencilState(Protean::DSSTYPE_DEFAULT);
				Protean::Graphics::GetDevice()->CreatePipelineState(&desc, &pso[1]);
				};
			static Protean::EventHandler::Handle handle = Protean::EventHandler::Subscribe(Protean::EventHandler::EVENT_RELOAD_SHADERS, [](uint64_t userdata) { LoadShaders(); });
			LoadShaders();
		}

		if (!buffer.IsValid())
		{
			auto fill_data = [&](void* data) {
				Vertex* gpu_vertices = (Vertex*)data;
				for (size_t i = 0; i < vertices_count; ++i)
				{
					Vertex vert = {};
					vert.position.x = vertices[i].x;
					vert.position.y = vertices[i].y;
					vert.position.z = vertices[i].z;
					vert.position.w = 1;
					vert.color = XMFLOAT4(1, 1, 1, 1);
					std::memcpy(gpu_vertices + i, &vert, sizeof(vert));
					aabb.AddPoint(vertices[i]);
				}

				uint32_t* gpu_indices = (uint32_t*)(gpu_vertices + vertices_count);
				std::memcpy(gpu_indices, indices, indices_count * sizeof(uint32_t));
				};

			GPUBufferDesc desc;
			desc.size = indices_count * sizeof(uint32_t) + vertices_count * sizeof(Vertex);
			desc.bind_flags = BindFlag_INDEX_BUFFER | BindFlag_VERTEX_BUFFER;
			GraphicsDevice* device = GetDevice();
			device->CreateBuffer2(&desc, fill_data, &buffer);
			device->SetName(&buffer, "DummyVisualizer::buffer");
		}
	}

	void Draw(
		uint32_t vertices_count,
		uint32_t indices_count,
		const XMMATRIX& matrix,
		const XMFLOAT4& color,
		bool depth,
		Protean::Graphics::CommandList cmd
	)
	{
		GraphicsDevice* device = GetDevice();

		device->BindPipelineState(&pso[depth], cmd);

		device->BindStencilRef(Protean::STENCILREF_DEFAULT, cmd);

		MiscCB sb;
		XMStoreFloat4x4(&sb.g_xTransform, matrix);
		sb.g_xColor = color;
		device->BindDynamicConstantBuffer(sb, CB_GETBINDSLOT(MiscCB), cmd);

		const GPUBuffer* vbs[] = {
			&buffer,
		};
		const uint32_t strides[] = {
			sizeof(Vertex),
		};
		const uint64_t offsets[] = {
			0,
		};
		device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, offsets, cmd);
		device->BindIndexBuffer(&buffer, IndexBufferFormat_UINT32, vertices_count * sizeof(Vertex), cmd);
		device->DrawIndexed((uint32_t)indices_count, 0, 0, cmd);
	}
};

namespace dummy
{
	void draw_male(const XMMATRIX& world, const XMMATRIX& VP, const XMFLOAT4& color, bool depth, Protean::Graphics::CommandList cmd, const Protean::Frustum& frustum)
	{
#include "dummy_male.h"
		static DummyVisualizer vis(vertices, arraysize(vertices), indices, arraysize(indices));
		if (!frustum.CheckBoxFast(vis.aabb.transform(world)))
			return;
		vis.Draw(arraysize(vertices), arraysize(indices), world * VP, color, depth, cmd);
	}
	void draw_female(const XMMATRIX& world, const XMMATRIX& VP, const XMFLOAT4& color, bool depth, Protean::Graphics::CommandList cmd, const Protean::Frustum& frustum)
	{
#include "dummy_female.h"
		static DummyVisualizer vis(vertices, arraysize(vertices), indices, arraysize(indices));
		if (!frustum.CheckBoxFast(vis.aabb.transform(world)))
			return;
		vis.Draw(arraysize(vertices), arraysize(indices), world * VP, color, depth, cmd);
	}
	void draw_soldier(const XMMATRIX& world, const XMMATRIX& VP, const XMFLOAT4& color, bool depth, Protean::Graphics::CommandList cmd, const Protean::Frustum& frustum)
	{
#include "dummy_soldier.h"
		static DummyVisualizer vis(vertices, arraysize(vertices), indices, arraysize(indices));
		if (!frustum.CheckBoxFast(vis.aabb.transform(world)))
			return;
		vis.Draw(arraysize(vertices), arraysize(indices), world * VP, color, depth, cmd);
	}
	void draw_direction(const XMMATRIX& world, const XMMATRIX& VP, const XMFLOAT4& color, bool depth, Protean::Graphics::CommandList cmd, const Protean::Frustum& frustum)
	{
#include "dummy_direction.h"
		static DummyVisualizer vis(vertices, arraysize(vertices), indices, arraysize(indices));
		if (!frustum.CheckBoxFast(vis.aabb.transform(world)))
			return;
		vis.Draw(arraysize(vertices), arraysize(indices), world * VP, color, depth, cmd);
	}
	void draw_waypoint(const XMMATRIX& world, const XMMATRIX& VP, const XMFLOAT4& color, bool depth, Protean::Graphics::CommandList cmd, const Protean::Frustum& frustum)
	{
#include "dummy_waypoint.h"
		static DummyVisualizer vis(vertices, arraysize(vertices), indices, arraysize(indices));
		if (!frustum.CheckBoxFast(vis.aabb.transform(world)))
			return;
		vis.Draw(arraysize(vertices), arraysize(indices), world * VP, color, depth, cmd);
	}
	void draw_pickup(const XMMATRIX& world, const XMMATRIX& VP, const XMFLOAT4& color, bool depth, Protean::Graphics::CommandList cmd, const Protean::Frustum& frustum)
	{
#include "dummy_pickup.h"
		static DummyVisualizer vis(vertices, arraysize(vertices), indices, arraysize(indices));
		if (!frustum.CheckBoxFast(vis.aabb.transform(world)))
			return;
		vis.Draw(arraysize(vertices), arraysize(indices), world * VP, color, depth, cmd);
	}
	void draw_vehicle(const XMMATRIX& world, const XMMATRIX& VP, const XMFLOAT4& color, bool depth, Protean::Graphics::CommandList cmd, const Protean::Frustum& frustum)
	{
#include "dummy_vehicle.h"
		static DummyVisualizer vis(vertices, arraysize(vertices), indices, arraysize(indices));
		if (!frustum.CheckBoxFast(vis.aabb.transform(world)))
			return;
		vis.Draw(arraysize(vertices), arraysize(indices), world * VP, color, depth, cmd);
	}
	void draw_poi(const XMMATRIX& world, const XMMATRIX& VP, const XMFLOAT4& color, bool depth, Protean::Graphics::CommandList cmd, const Protean::Frustum& frustum)
	{
#include "dummy_poi.h"
		static DummyVisualizer vis(vertices, arraysize(vertices), indices, arraysize(indices));
		if (!frustum.CheckBoxFast(vis.aabb.transform(world)))
			return;
		vis.Draw(arraysize(vertices), arraysize(indices), world * VP, color, depth, cmd);
	}
}
