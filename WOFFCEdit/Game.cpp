//
// Game.cpp
//

#include "Game.h"
#include "SceneObject.h"
#include <string>
#include <locale>
#include <codecvt>


using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;


// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window, int width, int height)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_deviceResources->RegisterDeviceNotify(this);

    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

#ifdef DXTK_AUDIO
    // Create DirectXTK for Audio objects
    AUDIO_ENGINE_FLAGS eflags = AudioEngine_Default;
#ifdef _DEBUG
    eflags = eflags | AudioEngine_Debug;
#endif

    m_audEngine.reset(new AudioEngine(eflags));

    m_audioEvent = 0;
    m_audioTimerAcc = 10.f;
    m_retryDefault = false;

    m_waveBank = std::make_unique<WaveBank>(m_audEngine.get(), L"adpcmdroid.xwb");

    m_soundEffect = std::make_unique<SoundEffect>(m_audEngine.get(), L"MusicMono_adpcm.wav");
    m_effect1 = m_soundEffect->CreateInstance();
    m_effect2 = m_waveBank->CreateInstance(10);

    m_effect1->Play(true);
    m_effect2->Play();
#endif
}

void Game::SetGridState(bool state)
{
    m_grid = state;
}

#pragma region Frame Update
// Executes the basic game loop.
void Game::Tick(DirectX::Mouse::State& mouse, DirectX::Keyboard::State& keyboard)
{
    //copy over the input commands so we have a local version to use elsewhere.
    m_timer.Tick([&]()
    {
        Update(m_timer, mouse, keyboard);
    });

#ifdef DXTK_AUDIO
    // Only update audio engine once per frame
    if (!m_audEngine->IsCriticalError() && m_audEngine->Update())
    {
        // Setup a retry in 1 second
        m_audioTimerAcc = 1.f;
        m_retryDefault = true;
    }
#endif

    Render();
}

void Game::Update(DX::StepTimer const & timer, Mouse::State& mouse, Keyboard::State& keyboard)
{
    //TODO  any more complex than this, and the camera should be abstracted out to somewhere else
    //camera motion is on a plane, so kill the 7 component of the look direction
    if (mouse.positionMode == Mouse::MODE_RELATIVE)
    {
        Vector3 delta = Vector3(float(mouse.x), float(mouse.y), 0.f) * MOUSE_SENSITIVITY;

        // Mouse smoothing
        m_smoothDelta = ((1.f - MOUSE_SMOOTH_FACTOR) * delta) + (MOUSE_SMOOTH_FACTOR * m_smoothDelta);

        m_camOrientation.y -= m_smoothDelta.x;
        m_camOrientation.x -= m_smoothDelta.y;

        // Prevent gimbal lock
        constexpr float LIMIT = 89.f;
        m_camOrientation.x = std::fminf(m_camOrientation.x, +LIMIT);
        m_camOrientation.x = std::fmaxf(m_camOrientation.x, -LIMIT);
    }
    else
    {
        if (keyboard.E)
        {
            m_camOrientation.y -= m_camRotRate;
        }
        if (keyboard.Q)
        {
            m_camOrientation.y += m_camRotRate;
        }
    }

    //create look direction from Euler angles in m_camOrientation
    float ry = XMConvertToRadians(m_camOrientation.y);
    float rx = XMConvertToRadians(m_camOrientation.x);

    float cosY = cosf(ry);
    float cosP = cosf(rx);
    float sinY = sinf(ry);
    float sinP = sinf(rx);

    m_camLookDirection.x = sinY * cosP;
    m_camLookDirection.y = sinP;
    m_camLookDirection.z = cosP * cosY;
    m_camLookDirection.Normalize();

    //create right vector from look Direction
    m_camLookDirection.Cross(Vector3::UnitY, m_camRight);
    m_camRight.Normalize();

    //process input and update stuff
    if (keyboard.W)
    {
        m_camPosition += m_camLookDirection*m_movespeed;
    }
    if (keyboard.S)
    {
        m_camPosition -= m_camLookDirection*m_movespeed;
    }
    if (keyboard.D)
    {
        m_camPosition += m_camRight*m_movespeed;
    }
    if (keyboard.A)
    {
        m_camPosition -= m_camRight*m_movespeed;
    }

    //update lookat point
    m_camLookAt = m_camPosition + m_camLookDirection;

    //apply camera vectors
    m_view = Matrix::CreateLookAt(m_camPosition, m_camLookAt, Vector3::UnitY);

    m_batchEffect->SetView(m_view);
    m_batchEffect->SetWorld(Matrix::Identity);
    m_displayChunk.m_terrainEffect->SetView(m_view);

    m_displayChunk.m_terrainEffect->SetWorld(Matrix::Identity);

#ifdef DXTK_AUDIO
    m_audioTimerAcc -= (float) timer.GetElapsedSeconds();
    if (m_audioTimerAcc < 0)
    {
        if (m_retryDefault)
        {
            m_retryDefault = false;
            if (m_audEngine->Reset())
            {
                // Restart looping audio
                m_effect1->Play(true);
            }
        }
        else
        {
            m_audioTimerAcc = 4.f;

            m_waveBank->Play(m_audioEvent++);

            if (m_audioEvent >= 11)
                m_audioEvent = 0;
        }
    }
#endif
}

#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Game::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    Clear();

    m_deviceResources->PIXBeginEvent(L"Render");
    auto context = m_deviceResources->GetD3DDeviceContext();

    if (m_grid)
    {
        // Draw procedurally generated dynamic grid
        const XMVECTORF32 xaxis = { 512.f, 0.f, 0.f };
        const XMVECTORF32 yaxis = { 0.f, 0.f, 512.f };
        DrawGrid(xaxis, yaxis, g_XMZero, 512, 512, Colors::Gray);
    }

    //RENDER OBJECTS FROM SCENEGRAPH
    int numRenderObjects = m_displayList.size();
    for (int i = 0; i < numRenderObjects; i++)
    {
        m_deviceResources->PIXBeginEvent(L"Draw model");
        const XMVECTORF32 scale = { m_displayList[i].m_scale.x, m_displayList[i].m_scale.y, m_displayList[i].m_scale.z };
        const XMVECTORF32 translate = { m_displayList[i].m_position.x, m_displayList[i].m_position.y, m_displayList[i].m_position.z };

        //convert degrees into radians for rotation matrix
        XMVECTOR rotate = Quaternion::CreateFromYawPitchRoll(XMConvertToRadians(m_displayList[i].m_orientation.y),
                                                             XMConvertToRadians(m_displayList[i].m_orientation.x),
                                                             XMConvertToRadians(m_displayList[i].m_orientation.z));

        XMMATRIX local = m_world * XMMatrixTransformation(g_XMZero, Quaternion::Identity, scale, g_XMZero, rotate, translate);

        m_displayList[i].m_model->Draw(context, *m_states, local, m_view, m_projection, false);	//last variable in draw,  make TRUE for wireframe

        m_deviceResources->PIXEndEvent();
    }
    m_deviceResources->PIXEndEvent();

    //RENDER TERRAIN
    context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
    context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
    context->RSSetState(m_states->CullNone());
    //	context->RSSetState(m_states->Wireframe());		//uncomment for wireframe

    ID3D11SamplerState* sampler[] = { m_states->AnisotropicWrap() };
    context->PSSetSamplers(0, 1, sampler);

    //Render the batch,  This is handled in the Display chunk becuase it has the potential to get complex
    m_displayChunk.RenderBatch(context);

    //CAMERA POSITION ON HUD
    m_sprites->Begin();
    std::wstring var = L"Cam X: " + std::to_wstring(m_camPosition.x) + L"Cam Z: " + std::to_wstring(m_camPosition.z);
    m_font->DrawString(m_sprites.get(), var.c_str(), XMFLOAT2(100, 10), Colors::Yellow);
    m_sprites->End();

    m_deviceResources->Present();
}

// Helper method to clear the back buffers.
void Game::Clear()
{
    m_deviceResources->PIXBeginEvent(L"Clear");

    // Clear the views.
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto renderTarget = m_deviceResources->GetBackBufferRenderTargetView();
    auto depthStencil = m_deviceResources->GetDepthStencilView();

    context->ClearRenderTargetView(renderTarget, Colors::CornflowerBlue);
    context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    context->OMSetRenderTargets(1, &renderTarget, depthStencil);

    // Set the viewport.
    auto viewport = m_deviceResources->GetScreenViewport();
    context->RSSetViewports(1, &viewport);

    m_deviceResources->PIXEndEvent();
}

void XM_CALLCONV Game::DrawGrid(FXMVECTOR xAxis, FXMVECTOR yAxis, FXMVECTOR origin, size_t xdivs, size_t ydivs, GXMVECTOR color)
{
    m_deviceResources->PIXBeginEvent(L"Draw grid");

    auto context = m_deviceResources->GetD3DDeviceContext();
    context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
    context->OMSetDepthStencilState(m_states->DepthNone(), 0);
    context->RSSetState(m_states->CullCounterClockwise());

    m_batchEffect->Apply(context);

    context->IASetInputLayout(m_batchInputLayout.Get());

    m_batch->Begin();

    xdivs = std::max<size_t>(1, xdivs);
    ydivs = std::max<size_t>(1, ydivs);

    for (size_t i = 0; i <= xdivs; ++i)
    {
        float fPercent = float(i) / float(xdivs);
        fPercent = (fPercent * 2.0f) - 1.0f;
        XMVECTOR vScale = XMVectorScale(xAxis, fPercent);
        vScale = XMVectorAdd(vScale, origin);

        VertexPositionColor v1(XMVectorSubtract(vScale, yAxis), color);
        VertexPositionColor v2(XMVectorAdd(vScale, yAxis), color);
        m_batch->DrawLine(v1, v2);
    }

    for (size_t i = 0; i <= ydivs; i++)
    {
        float fPercent = float(i) / float(ydivs);
        fPercent = (fPercent * 2.0f) - 1.0f;
        XMVECTOR vScale = XMVectorScale(yAxis, fPercent);
        vScale = XMVectorAdd(vScale, origin);

        VertexPositionColor v1(XMVectorSubtract(vScale, xAxis), color);
        VertexPositionColor v2(XMVectorAdd(vScale, xAxis), color);
        m_batch->DrawLine(v1, v2);
    }

    m_batch->End();

    m_deviceResources->PIXEndEvent();
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void Game::OnActivated()
{
}

void Game::OnDeactivated()
{
}

void Game::OnSuspending()
{
#ifdef DXTK_AUDIO
    m_audEngine->Suspend();
#endif
}

void Game::OnResuming()
{
    m_timer.ResetElapsedTime();

#ifdef DXTK_AUDIO
    m_audEngine->Resume();
#endif
}

void Game::OnWindowSizeChanged(int width, int height)
{
    if (!m_deviceResources->WindowSizeChanged(width, height))
        return;

    CreateWindowSizeDependentResources();
}

void Game::BuildDisplayList(std::vector<SceneObject> * SceneGraph)
{
    auto device = m_deviceResources->GetD3DDevice();
    auto devicecontext = m_deviceResources->GetD3DDeviceContext();

    if (!m_displayList.empty())		//is the vector empty
    {
        m_displayList.clear();		//if not, empty it
    }

    //for every item in the scenegraph
    const int numObjects = SceneGraph->size();
    for (int i = 0; i < numObjects; i++)
    {
        const SceneObject& sceneObject = (*SceneGraph)[i];

        //create a temp display object that we will populate then append to the display list.
        DisplayObject newDisplayObject;
        newDisplayObject.m_ID = sceneObject.ID;

        //load model
        std::wstring_convert<std::codecvt_utf8<wchar_t>> convertToWide;
        std::wstring modelwstr = convertToWide.from_bytes(sceneObject.model_path);							//convect string to Wchar
        newDisplayObject.m_model = Model::CreateFromCMO(device, modelwstr.c_str(), *m_fxFactory, true);	//get DXSDK to load model "False" for LH coordinate system (maya)

        //Load Texture
        std::wstring texturewstr = convertToWide.from_bytes(sceneObject.tex_diffuse_path);								//convect string to Wchar
        ID3D11ShaderResourceView* texture_diffuse = nullptr;
        HRESULT rs = CreateDDSTextureFromFile(device, texturewstr.c_str(), nullptr, &texture_diffuse);	//load tex into Shader resource

        //if texture fails.  load error default
        if (rs)
        {
            CreateDDSTextureFromFile(device, L"database/data/Error.dds", nullptr, &texture_diffuse);	//load tex into Shader resource
        }

        // Texture is handled by RAII
        newDisplayObject.m_texture_diffuse = texture_diffuse;

        //apply new texture to models effect
        newDisplayObject.m_model->UpdateEffects([&](IEffect* effect) //This uses a Lambda function,  if you dont understand it: Look it up.
        {
            auto lights = dynamic_cast<BasicEffect*>(effect);
            if (lights)
            {
                lights->SetTexture(newDisplayObject.m_texture_diffuse.Get());
            }
        });

        //set position
        newDisplayObject.m_position.x = sceneObject.posX;
        newDisplayObject.m_position.y = sceneObject.posY;
        newDisplayObject.m_position.z = sceneObject.posZ;

        //setorientation
        newDisplayObject.m_orientation.x = sceneObject.rotX;
        newDisplayObject.m_orientation.y = sceneObject.rotY;
        newDisplayObject.m_orientation.z = sceneObject.rotZ;

        //set scale
        newDisplayObject.m_scale.x = sceneObject.scaX;
        newDisplayObject.m_scale.y = sceneObject.scaY;
        newDisplayObject.m_scale.z = sceneObject.scaZ;

        //set wireframe / render flags
        newDisplayObject.m_render = sceneObject.editor_render;
        newDisplayObject.m_wireframe = sceneObject.editor_wireframe;

        m_displayList.push_back(newDisplayObject);
    }
}

void Game::BuildDisplayChunk(ChunkObject * SceneChunk)
{
    //populate our local DISPLAYCHUNK with all the chunk info we need from the object stored in toolmain
    //which, to be honest, is almost all of it. Its mostly rendering related info so...
    m_displayChunk.PopulateChunkData(SceneChunk);		//migrate chunk data
    m_displayChunk.LoadHeightMap(m_deviceResources->GetD3DDevice());
    m_displayChunk.InitialiseBatch();
    // Initialise rendering after batch, because we need to know how large the index buffer needs to be
    m_displayChunk.InitialiseRendering(m_deviceResources.get());
    m_displayChunk.m_terrainEffect->SetProjection(m_projection);
}

void Game::SaveDisplayChunk(ChunkObject * SceneChunk)
{
    m_displayChunk.SaveHeightMap();			//save heightmap to file.
}

void Game::InitialiseInput(Mouse::ButtonStateTracker& mouseTracker, Keyboard::KeyboardStateTracker& keyboardTracker)
{
    m_mouseTracker = &mouseTracker;
    m_keyboardTracker = &keyboardTracker;
}

#ifdef DXTK_AUDIO
void Game::NewAudioDevice()
{
    if (m_audEngine && !m_audEngine->IsAudioDevicePresent())
    {
        // Setup a retry in 1 second
        m_audioTimerAcc = 1.f;
        m_retryDefault = true;
    }
}
#endif

#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Game::CreateDeviceDependentResources()
{
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto device = m_deviceResources->GetD3DDevice();

    m_states = std::make_unique<CommonStates>(device);

    m_fxFactory = std::make_unique<EffectFactory>(device);
    m_fxFactory->SetDirectory(L"database/data/"); //fx Factory will look in the database directory
    m_fxFactory->SetSharing(false);	//we must set this to false otherwise it will share effects based on the initial tex loaded (When the model loads) rather than what we will change them to.

    m_sprites = std::make_unique<SpriteBatch>(context);

    m_batch = std::make_unique<PrimitiveBatch<VertexPositionColor>>(context);

    m_batchEffect = std::make_unique<BasicEffect>(device);
    m_batchEffect->SetVertexColorEnabled(true);

    {
        void const* shaderByteCode;
        size_t byteCodeLength;

        m_batchEffect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

        DX::ThrowIfFailed(
            device->CreateInputLayout(VertexPositionColor::InputElements,
                                      VertexPositionColor::InputElementCount,
                                      shaderByteCode, byteCodeLength,
                                      m_batchInputLayout.ReleaseAndGetAddressOf())
        );
    }

    m_font = std::make_unique<SpriteFont>(device, L"SegoeUI_18.spritefont");

    //    m_shape = GeometricPrimitive::CreateTeapot(context, 4.f, 8);

        // SDKMESH has to use clockwise winding with right-handed coordinates, so textures are flipped in U
    m_model = Model::CreateFromSDKMESH(device, L"tiny.sdkmesh", *m_fxFactory);


    // Load textures
    DX::ThrowIfFailed(
        CreateDDSTextureFromFile(device, L"seafloor.dds", nullptr, m_texture1.ReleaseAndGetAddressOf())
    );

    DX::ThrowIfFailed(
        CreateDDSTextureFromFile(device, L"windowslogo.dds", nullptr, m_texture2.ReleaseAndGetAddressOf())
    );

}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    auto size = m_deviceResources->GetOutputSize();
    float aspectRatio = float(size.right) / float(size.bottom);
    float fovAngleY = XMConvertToRadians(DEFAULT_FOV_DEG);

    // This is a simple example of change that can be made when the app is in
    // portrait or snapped view.
    if (aspectRatio < 1.0f)
    {
        fovAngleY *= 2.0f;
    }

    // This sample makes use of a right-handed coordinate system using row-major matrices.
    m_projection = Matrix::CreatePerspectiveFieldOfView(
        fovAngleY,
        aspectRatio,
        NEAR_PLANE,
        FAR_PLANE
    );

    m_batchEffect->SetProjection(m_projection);

    if (m_displayChunk.m_terrainEffect)
        m_displayChunk.m_terrainEffect->SetProjection(m_projection);

}

void Game::OnDeviceLost()
{
    m_states.reset();
    m_fxFactory.reset();
    m_sprites.reset();
    m_batch.reset();
    m_batchEffect.reset();
    m_font.reset();
    m_shape.reset();
    m_model.reset();
    m_texture1.Reset();
    m_texture2.Reset();
    m_batchInputLayout.Reset();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
