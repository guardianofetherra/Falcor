/***************************************************************************
# Copyright (c) 2018, NVIDIA CORPORATION. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of NVIDIA CORPORATION nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************************************************************************/
#include "GodRaysSample.h"

using namespace Falcor;

const char* GodRaysSample::kDefaultSceneName = "testScenes/tree.fscene";

void GodRaysSample::onLoad(SampleCallbacks* pSample, const RenderContext::SharedPtr& pRenderContext)
{
    mpGodRaysPass = GodRays::create(1.0f);
    Scene::SharedPtr pScene = Scene::loadFromFile(kDefaultSceneName);
    mCamControl.attachCamera(pScene->getCamera(0));
    mpSceneRenderer = SceneRenderer::create(pScene);
    mpSceneRenderer->setCameraControllerType(SceneRenderer::CameraControllerType::FirstPerson);

    GraphicsProgram::SharedPtr pProgram = GraphicsProgram::createFromFile("RenderPasses/SceneLightingPass.slang", "", "ps");
    mpGraphicsState = GraphicsState::create();
    mpGraphicsState->setProgram(pProgram);
    mpVars = GraphicsVars::create(pProgram->getReflector());

    mpGodRaysPass = GodRays::create();
    mpGodRaysPass->setScene(pScene);
    mpToneMappingPass = ToneMapping::create(ToneMapping::Operator::Aces);
    mpToneMappingPass->setScene(pScene);

    Scene::UserVariable var = pScene->getUserVariable("sky_box");
    assert(var.type == Scene::UserVariable::Type::String);
    std::string skyBox = getDirectoryFromFile(kDefaultSceneName) + '/' + var.str;
    Sampler::Desc samplerDesc;
    samplerDesc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Linear);
    mpSkyBoxPass = SkyBox::createFromTexture(skyBox, true, Sampler::create(samplerDesc));
}

void GodRaysSample::onGuiRender(SampleCallbacks* pSample, Gui* pGui)
{
    std::string name;
    if (pGui->addButton("Load Scene"))
    {
        if (openFileDialog("", name))
        {
            Scene::SharedPtr pScene = Scene::loadFromFile(name);
            mpSceneRenderer = SceneRenderer::create(pScene);

            mpGodRaysPass->setScene(pScene);
            mpToneMappingPass->setScene(pScene);
        }
    }
    pGui->addCheckBox("Enable God Rays", mEnableGodRays);
    if (mEnableGodRays)
    {
        mpGodRaysPass->renderUI(pGui, "God Rays");
    }
}

void GodRaysSample::onFrameRender(SampleCallbacks* pSample, const RenderContext::SharedPtr& pRenderContext, const Fbo::SharedPtr& pTargetFbo)
{
    const glm::vec4 clearColor(0.38f, 0.52f, 0.10f, 1);
    mpSceneRenderer->getScene()->update(pSample->getCurrentTime(), &mCamControl);
    pRenderContext->clearFbo(pTargetFbo.get(), clearColor, 1.0f, 0, FboAttachmentType::All);
    mpGraphicsState->pushFbo(pTargetFbo);
    pRenderContext->setGraphicsState(mpGraphicsState);

    pRenderContext->pushGraphicsVars(mpVars);
    mpSkyBoxPass->render(pRenderContext.get(), mpSceneRenderer->getScene()->getActiveCamera().get(), pTargetFbo);
    mpSceneRenderer->renderScene(pRenderContext.get());
    pRenderContext->popGraphicsVars();
    if (mEnableGodRays)
    {
        mpGodRaysPass->execute(pRenderContext.get(), pTargetFbo);
    }
    mpToneMappingPass->execute(pRenderContext.get(), pTargetFbo->getColorTexture(0), pTargetFbo);

    pRenderContext->popGraphicsState();
    mpGraphicsState->popFbo();
}

void GodRaysSample::onResizeSwapChain(SampleCallbacks* pSample, uint32_t width, uint32_t height)
{
    ResourceFormat format = ResourceFormat::RGBA32Float;
    Fbo::Desc desc;
    desc.setDepthStencilTarget(ResourceFormat::D16Unorm);
    desc.setColorTarget(0u, format);

    mpFbo = FboHelper::create2D(width, height, desc);
}

bool GodRaysSample::onKeyEvent(SampleCallbacks* pSample, const KeyboardEvent& keyEvent)
{
    return mCamControl.onKeyEvent(keyEvent);
}

bool GodRaysSample::onMouseEvent(SampleCallbacks* pSample, const MouseEvent& mouseEvent)
{
    return mCamControl.onMouseEvent(mouseEvent);
}

void GodRaysSample::onInitializeTesting(SampleCallbacks* pSample)
{
}

void GodRaysSample::onEndTestFrame(SampleCallbacks* pSample, SampleTest* pSampleTest)
{

}

#ifdef _WIN32
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
#else
int main(int argc, char** argv)
#endif
{
    GodRaysSample::UniquePtr pRenderer = std::make_unique<GodRaysSample>();
    SampleConfig config;
    config.windowDesc.title = "God Rays Sample";
#ifdef _WIN32
    Sample::run(config, pRenderer);
#else
    config.argc = (uint32_t)argc;
    config.argv = argv;
    Sample::run(config, pRenderer);
#endif
    return 0;
}
