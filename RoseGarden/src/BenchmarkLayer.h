#pragma once

#include "Hazel.h"
#include "Hazel/ComponentSystem/GameObject.h"
#include "Hazel/ComponentSystem/Component.h"
#include "Hazel/Renderer/PerspectiveCameraController.h"
#include "Hazel/Scene/Scene.h"

#include "Platform/D3D12/D3D12Buffer.h"
#include "Platform/D3D12/FrameBuffer.h"

#include <future>
#include <atomic>
class BenchmarkLayer : public Hazel::Layer
{
public:

	struct CreationOptions {
        int32_t UpdateRate = 0;
        uint32_t CaptureRate = 0;
        uint32_t CaptureLimit = 0;
	};

	BenchmarkLayer(const std::string& name, CreationOptions options = {});
    virtual ~BenchmarkLayer() = default;

	virtual void OnAttach() override;
	virtual void OnDetach() override;

	void OnUpdate(Hazel::Timestep ts) override;
	virtual void OnImGuiRender() override;
	virtual void OnFrameEnd() override;
	void OnEvent(Hazel::Event& e) override;

	bool OnMouseButtonPressed(Hazel::MouseButtonPressedEvent& event);
	bool OnResize(Hazel::WindowResizeEvent& event);

protected:
	void CaptureLastFramebuffer();
	
	virtual void OnRefresh() {};
	virtual void OnCapture() {}

	void AppendCapturePath(std::string& suffix);
	
	Hazel::Scene m_Scene;
	Hazel::PerspectiveCameraController m_CameraController;

    Hazel::Ref<Hazel::GameObject> m_Selection = nullptr;
    Hazel::Ref<Hazel::FrameBuffer> m_LastFrameBuffer = nullptr;
    Hazel::Ref<Hazel::ReadbackBuffer> m_ReadbackBuffer = nullptr;

    int m_EnvironmentLevel = 0;
	float m_LastFrameTime;
    uint32_t m_RefreshCounter = 0;

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

