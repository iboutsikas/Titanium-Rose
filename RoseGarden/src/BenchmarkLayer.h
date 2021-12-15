#pragma once

#include "Roses.h"
#include "TitaniumRose/ComponentSystem/GameObject.h"
#include "TitaniumRose/ComponentSystem/Component.h"
#include "TitaniumRose/Renderer/PerspectiveCameraController.h"
#include "TitaniumRose/Scene/Scene.h"

#include "Platform/D3D12/D3D12Buffer.h"
#include "Platform/D3D12/FrameBuffer.h"

#include <future>
#include <atomic>
class BenchmarkLayer : public Roses::Layer
{
public:

	struct CreationOptions {
        int32_t UpdateRate = -1;
        uint32_t CaptureRate = 0;
        int32_t CaptureLimit = -1;
		bool CaptureTiming = false;
	};

	BenchmarkLayer(const std::string& name, CreationOptions options = {});
    virtual ~BenchmarkLayer() = default;

	virtual void OnAttach() override;
	virtual void OnDetach() override;

	void OnUpdate(Roses::Timestep ts) override;
	void OnRender(Roses::Timestep ts, Roses::GraphicsContext& gfxContext) override;

	virtual void OnImGuiRender(Roses::GraphicsContext& uiContext) override;
	virtual void OnFrameEnd() override;
	void OnEvent(Roses::Event& e) override;

	bool OnMouseButtonPressed(Roses::MouseButtonPressedEvent& event);
	bool OnResize(Roses::WindowResizeEvent& event);

protected:
	void CaptureLastFramebuffer();
	
	virtual void OnRefresh() {};
	virtual void OnCapture() {}

	void AppendCapturePath(std::string& suffix);
	
	Roses::Scene m_Scene;
	Roses::PerspectiveCameraController m_CameraController;

    Roses::Ref<Roses::HGameObject> m_Selection = nullptr;
    Roses::Ref<Roses::FrameBuffer> m_LastFrameBuffer = nullptr;
    Roses::Ref<Roses::ReadbackBuffer> m_ReadbackBuffer = nullptr;

    int m_EnvironmentLevel = 0;
	float m_LastFrameTime;

	bool m_EnableCapture = true;
    uint32_t m_CaptureCounter = 0;
	uint64_t m_FrameCounter = 0;
	
	std::vector<std::shared_future<void>> m_CaptureTasks;
	CreationOptions m_CreationOptions;
private:
	glm::vec4 m_ClearColor;
	std::string m_CaptureFolder;
	std::atomic_uint32_t m_CapturedFrames = 0;
};

