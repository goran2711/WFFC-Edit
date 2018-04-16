//
// Game.h
//

#pragma once

#include "pch.h"
#include "StepTimer.h"
#include "DisplayObject.h"
#include "DisplayChunk.h"
#include "DeviceResources.h"

struct ChunkObject;
struct SceneObject;

// A basic game implementation that creates a D3D11 device and
// provides a game loop.
class Game : public DX::IDeviceNotify
{
    constexpr static float DEFAULT_FOV_DEG = 75.f;
    constexpr static float NEAR_PLANE = 0.01f;
    constexpr static float FAR_PLANE = 1000.f;

    constexpr static float MOUSE_SENSITIVITY = 1.f;
    constexpr static float MOUSE_SMOOTH_FACTOR = 0.5f;

public:

    // Initialization and management
    void Initialize(HWND window, int width, int height);
    void SetGridState(bool state);

    // Basic game loop
    void Tick(DirectX::Mouse::State& mouse, DirectX::Keyboard::State& keyboard);
    void Render();

    // Rendering helpers
    void Clear();

    // IDeviceNotify
    virtual void OnDeviceLost() override;
    virtual void OnDeviceRestored() override;

    // Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowSizeChanged(int width, int height);

    //tool specific
    void BuildDisplayList(std::vector<SceneObject> * SceneGraph); //note vector passed by reference 
    void BuildDisplayChunk(ChunkObject *SceneChunk);
    void SaveDisplayChunk(ChunkObject *SceneChunk);	//saves geometry et al
    void ClearDisplayList();

    //input
    void InitialiseInput(DirectX::Mouse::ButtonStateTracker& mouseTracker, DirectX::Keyboard::KeyboardStateTracker& keyboardTracker);

#ifdef DXTK_AUDIO
    void NewAudioDevice();
#endif

private:

    void Update(DX::StepTimer const& timer, DirectX::Mouse::State& mouse, DirectX::Keyboard::State& keyboard);

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    void XM_CALLCONV DrawGrid(DirectX::FXMVECTOR xAxis, DirectX::FXMVECTOR yAxis, DirectX::FXMVECTOR origin, size_t xdivs, size_t ydivs, DirectX::GXMVECTOR color);

    //tool specific
    std::vector<DisplayObject>			m_displayList;
    DisplayChunk						m_displayChunk;

    //functionality
    float								m_movespeed = 0.3f;

    //store last frame's cursor delta movement for smoothing
    DirectX::SimpleMath::Vector3        m_smoothDelta;

    //camera
    DirectX::SimpleMath::Vector3		m_camPosition{ 0.f, 3.7f, -3.5f };
    DirectX::SimpleMath::Vector3		m_camOrientation;
    DirectX::SimpleMath::Vector3		m_camLookAt;
    DirectX::SimpleMath::Vector3		m_camLookDirection;
    DirectX::SimpleMath::Vector3		m_camRight;
    float m_camRotRate = 3.f;

    //control variables
    bool m_grid = false;						//grid rendering on / off
    // Device resources.
    std::shared_ptr<DX::DeviceResources>    m_deviceResources;

    // Rendering loop timer.
    DX::StepTimer                           m_timer;

    // DirectXTK objects.
    std::unique_ptr<DirectX::CommonStates>                                  m_states;
    std::unique_ptr<DirectX::BasicEffect>                                   m_batchEffect;
    std::unique_ptr<DirectX::EffectFactory>                                 m_fxFactory;
    std::unique_ptr<DirectX::GeometricPrimitive>                            m_shape;
    std::unique_ptr<DirectX::Model>                                         m_model;
    std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>  m_batch;
    std::unique_ptr<DirectX::SpriteBatch>                                   m_sprites;
    std::unique_ptr<DirectX::SpriteFont>                                    m_font;

#ifdef DXTK_AUDIO
    using AudioEngineDeleter = void(*)(DirectX::AudioEngine*);

    // Custom deleter to ensure the audio engine is suspended upon deletion of the Game object due to the multi-threaded nature of XAudio2
    std::unique_ptr<DirectX::AudioEngine, AudioEngineDeleter>               m_audEngine{ nullptr, [](DirectX::AudioEngine* audEngine) { audEngine->Suspend(); delete audEngine; } };
    std::unique_ptr<DirectX::WaveBank>                                      m_waveBank;
    std::unique_ptr<DirectX::SoundEffect>                                   m_soundEffect;
    std::unique_ptr<DirectX::SoundEffectInstance>                           m_effect1;
    std::unique_ptr<DirectX::SoundEffectInstance>                           m_effect2;
#endif

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>                        m_texture1;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>                        m_texture2;
    Microsoft::WRL::ComPtr<ID3D11InputLayout>                               m_batchInputLayout;

#ifdef DXTK_AUDIO
    uint32_t                                                                m_audioEvent;
    float                                                                   m_audioTimerAcc;

    bool                                                                    m_retryDefault;
#endif

    DirectX::SimpleMath::Matrix                                             m_world;
    DirectX::SimpleMath::Matrix                                             m_view;
    DirectX::SimpleMath::Matrix                                             m_projection;

    // Input
    DirectX::Mouse::ButtonStateTracker* m_mouseTracker;
    DirectX::Keyboard::KeyboardStateTracker* m_keyboardTracker;
};
