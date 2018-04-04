#pragma once
#include <memory>
#include <wrl/client.h>
#include <d3d11_1.h>
#include <SimpleMath.h>

namespace DirectX
{
    class Model;
}

struct DisplayObject
{
    std::shared_ptr<DirectX::Model>						m_model = NULL;						//main Mesh
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	m_texture_diffuse = NULL;			//diffuse texture

    int m_ID;
    DirectX::SimpleMath::Vector3			m_position;
    DirectX::SimpleMath::Vector3			m_orientation;
    DirectX::SimpleMath::Vector3			m_scale;
    bool									m_render = true;
    bool									m_wireframe = false;
};

