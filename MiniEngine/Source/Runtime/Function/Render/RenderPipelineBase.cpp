#include "RenderPipelineBase.h"

namespace ME
{
	void RenderPipelineBase::PreparePassData(std::shared_ptr<RenderResourceBase> render_resource)
	{
		m_main_camera_pass->PreparePassData(render_resource);
	}

	void RenderPipelineBase::ForwardRender(std::shared_ptr<RHI> rhi, std::shared_ptr<RenderResourceBase> render_resource)
	{

	}

	void RenderPipelineBase::DeferredRender(std::shared_ptr<RHI> rhi, std::shared_ptr<RenderResourceBase> render_resource)
	{

	}

	void RenderPipelineBase::InitializeUIRenderBackend(WindowUI* window_ui)
	{
		m_ui_pass->InitializeUIRenderBackend(window_ui);
	}


}