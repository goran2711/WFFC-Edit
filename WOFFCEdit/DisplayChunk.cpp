#include "DisplayChunk.h"
#include "ChunkObject.h"
#include "DeviceResources.h"
#include "pch.h"
#include <string>
#include <locale>
#include <codecvt>


using namespace DirectX;
using namespace DirectX::SimpleMath;

void DisplayChunk::PopulateChunkData(ChunkObject * SceneChunk)
{
    m_name = SceneChunk->name;
    m_chunk_x_size_metres = SceneChunk->chunk_x_size_metres;
    m_chunk_y_size_metres = SceneChunk->chunk_y_size_metres;
    m_chunk_base_resolution = SceneChunk->chunk_base_resolution;
    m_heightmap_path = SceneChunk->heightmap_path;
    m_tex_diffuse_path = SceneChunk->tex_diffuse_path;
    m_tex_splat_alpha_path = SceneChunk->tex_splat_alpha_path;
    m_tex_splat_1_path = SceneChunk->tex_splat_1_path;
    m_tex_splat_2_path = SceneChunk->tex_splat_2_path;
    m_tex_splat_3_path = SceneChunk->tex_splat_3_path;
    m_tex_splat_4_path = SceneChunk->tex_splat_4_path;
    m_render_wireframe = SceneChunk->render_wireframe;
    m_render_normals = SceneChunk->render_normals;
    m_tex_diffuse_tiling = SceneChunk->tex_diffuse_tiling;
    m_tex_splat_1_tiling = SceneChunk->tex_splat_1_tiling;
    m_tex_splat_2_tiling = SceneChunk->tex_splat_2_tiling;
    m_tex_splat_3_tiling = SceneChunk->tex_splat_3_tiling;
    m_tex_splat_4_tiling = SceneChunk->tex_splat_4_tiling;
}

void DisplayChunk::RenderBatch(ID3D11DeviceContext* context)
{
    m_terrainEffect->Apply(context);
    context->IASetInputLayout(m_terrainInputLayout.Get());

    m_batch->Begin();
    m_batch->DrawIndexed(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, m_indices.data(), m_indices.size(), m_terrainGeometry, NUM_VERTICES);
    m_batch->End();
}

void DisplayChunk::InitialiseBatch()
{
    //build geometry for our terrain array
    //iterate through all the vertices of our required resolution terrain.
    const float terrainSizeH = m_terrainSize * 0.5f;

    for (size_t z = 0; z < TERRAINRESOLUTION; z++)
    {
        for (size_t x = 0; x < TERRAINRESOLUTION; x++)
        {
            int index = (TERRAINRESOLUTION * z) + x;

            //This will create a terrain going from -64->64.  rather than 0->128.  So the center of the terrain is on the origin
            m_terrainGeometry[index].position = {
                (x * m_terrainPositionScalingFactor) - terrainSizeH,
                float(m_heightMap[index]) * m_terrainHeightScale,
                (z * m_terrainPositionScalingFactor) - terrainSizeH
            };

            m_terrainGeometry[index].normal = Vector3::UnitY;
            //Spread tex coords so that its distributed evenly across the terrain from 0-1
            m_terrainGeometry[index].textureCoordinate = { x * TEXCOORD_STEP * m_tex_diffuse_tiling, z * TEXCOORD_STEP * m_tex_diffuse_tiling };
        }
    }

    // Initialise indices
    for (size_t z = 0; z < TERRAINRESOLUTION - 1; z++)
    {
        for (size_t x = 0; x < TERRAINRESOLUTION - 1; x++)
        {
            int bottomL = (TERRAINRESOLUTION * z) + x;
            int bottomR = (TERRAINRESOLUTION * z) + x + 1;
            int topR = (TERRAINRESOLUTION * (z + 1)) + x + 1;
            int topL = (TERRAINRESOLUTION * (z + 1)) + x;

            // First triangle
            m_indices.push_back(bottomL);
            m_indices.push_back(bottomR);
            m_indices.push_back(topR);

            // Second triangle
            m_indices.push_back(bottomL);
            m_indices.push_back(topR);
            m_indices.push_back(topL);
        }
    }

    CalculateTerrainNormals();
}

void DisplayChunk::InitialiseRendering(DX::DeviceResources* deviceResources)
{
    ID3D11Device* device = deviceResources->GetD3DDevice();
    ID3D11DeviceContext* context = deviceResources->GetD3DDeviceContext();

    //setup terrain effect
    m_terrainEffect = std::make_unique<BasicEffect>(device);
    m_terrainEffect->EnableDefaultLighting();
    m_terrainEffect->SetLightingEnabled(true);
    m_terrainEffect->SetTextureEnabled(true);
    m_terrainEffect->SetTexture(m_texture_diffuse);

    void const* shaderByteCode;
    size_t byteCodeLength;

    m_terrainEffect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

    //setup batch
    DX::ThrowIfFailed(
        device->CreateInputLayout(VertexPositionNormalTexture::InputElements,
                                  VertexPositionNormalTexture::InputElementCount,
                                  shaderByteCode,
                                  byteCodeLength,
                                  m_terrainInputLayout.GetAddressOf())
    );

    m_batch = std::make_unique<PrimitiveBatch<VertexPositionNormalTexture>>(context, m_indices.size() + 1, NUM_VERTICES + 1);
}

void DisplayChunk::LoadHeightMap(ID3D11Device* device)
{
    //load in heightmap .raw
    FILE *pFile = NULL;

    // Open The File In Read / Binary Mode.
    errno_t ret = fopen_s(&pFile, m_heightmap_path.c_str(), "rb");
    // Check To See If We Found The File And Could Open It
    if (ret != 0 || pFile == NULL)
    {
        // Display Error Message And Stop The Function
        MessageBox(NULL, L"Can't Find The Height Map!", L"Error", MB_OK);
        return;
    }

    // Here We Load The .RAW File Into Our pHeightMap Data Array
    // We Are Only Reading In '1', And The Size Is (Width * Height)
    fread(m_heightMap, 1, TERRAINRESOLUTION*TERRAINRESOLUTION, pFile);

    fclose(pFile);

    //load the diffuse texture
    std::wstring_convert<std::codecvt_utf8<wchar_t>> convertToWide;
    std::wstring texturewstr = convertToWide.from_bytes(m_tex_diffuse_path);
    HRESULT rs = CreateDDSTextureFromFile(device, texturewstr.c_str(), NULL, &m_texture_diffuse);	//load tex into Shader resource	view and resource
}

void DisplayChunk::SaveHeightMap()
{
    FILE *pFile = NULL;

    // Open The File In Read / Binary Mode.
    errno_t ret = fopen_s(&pFile, m_heightmap_path.c_str(), "wb+");;
    // Check To See If We Found The File And Could Open It
    if (ret != 0 || pFile == NULL)
    {
        // Display Error Message And Stop The Function
        MessageBox(NULL, L"Can't Find The Height Map!", L"Error", MB_OK);
        return;
    }

    fwrite(m_heightMap, 1, TERRAINRESOLUTION*TERRAINRESOLUTION, pFile);
    fclose(pFile);
}

void DisplayChunk::UpdateTerrain()
{
    //all this is doing is transferring the height from the heigtmap into the terrain geometry.
    for (size_t i = 0; i < NUM_VERTICES; ++i)
        m_terrainGeometry[i].position.y = float(m_heightMap[i]) * m_terrainHeightScale;

    CalculateTerrainNormals();
}

void DisplayChunk::GenerateHeightmap()
{
    //insert how YOU want to update the heigtmap here! :D
}

void DisplayChunk::CalculateTerrainNormals()
{
    // Lambda for testing if two indices are on the same terrain row
    static const auto OnSameRow = [](int i0, int i1)
    {
        return (i0 / TERRAINRESOLUTION) == (i1 / TERRAINRESOLUTION);
    };

    for (int i = 0; i < NUM_QUADS; i++)
    {
        // Neighbour index calculation and range checks
        int upIdx = (i + TERRAINRESOLUTION >= (TERRAINRESOLUTION * TERRAINRESOLUTION) ? i : i + TERRAINRESOLUTION);
        int downIdx = (i - TERRAINRESOLUTION < 0 ? i : i - TERRAINRESOLUTION);

        int leftIdx = (OnSameRow(i, i - 1) ? i - 1 : i);
        int rightIdx = (OnSameRow(i, i + 1) ? i + 1 : i);

        // Normal calculation
        Vector3 upDownVector(
            (m_terrainGeometry[upIdx].position.x - m_terrainGeometry[downIdx].position.x),
            (m_terrainGeometry[upIdx].position.y - m_terrainGeometry[downIdx].position.y),
            (m_terrainGeometry[upIdx].position.z - m_terrainGeometry[downIdx].position.z)
        );

        Vector3 leftRightVector(
            (m_terrainGeometry[leftIdx].position.x - m_terrainGeometry[rightIdx].position.x),
            (m_terrainGeometry[leftIdx].position.y - m_terrainGeometry[rightIdx].position.y),
            (m_terrainGeometry[leftIdx].position.z - m_terrainGeometry[rightIdx].position.z)
        );

        Vector3 normalVector = leftRightVector.Cross(upDownVector);
        normalVector.Normalize();

        m_terrainGeometry[i].normal = normalVector;
    }
}
