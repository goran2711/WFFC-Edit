#include "ToolMain.h"
#include "resource.h"
#include <vector>
#include <sstream>

using DirectX::GamePad;
using DirectX::Keyboard;
using DirectX::Mouse;

ToolMain::~ToolMain()
{
    sqlite3_close(m_databaseConnection);
}


int ToolMain::getCurrentSelectionID() const
{
    return m_selectedObject;
}

void ToolMain::onActionInitialise(HWND handle, int width, int height)
{
    //window size, handle etc for directX
    m_toolHandle = handle;
    m_width = width;
    m_height = height;

    m_d3dRenderer.Initialize(handle, m_width, m_height);

    // Initialise user input devices
    m_gamePad = std::make_unique<GamePad>();

    m_keyboard = std::make_unique<Keyboard>();
    m_kbTracker = std::make_unique<Keyboard::KeyboardStateTracker>();

    m_mouse = std::make_unique<Mouse>();
    m_mouse->SetWindow(handle);
    m_mouseTracker = std::make_unique<Mouse::ButtonStateTracker>();

    m_d3dRenderer.InitialiseInput(*m_mouseTracker, *m_kbTracker);

    //database connection establish
    int rc;
    rc = sqlite3_open("database/test.db", &m_databaseConnection);

    if (rc)
    {
        //if the database cant open. Perhaps a more catastrophic error would be better here
        TRACE("Can't open database");
    }
    else
    {
        TRACE("Opened database successfully");
    }

    onActionLoad();
}

void ToolMain::onActionLoad()
{
    //load current chunk and objects into lists
    if (!m_sceneGraph.empty())
    {
        m_sceneGraph.clear();
    }

    //SQL
    int rc;
    char *sqlCommand;
    char *ErrMSG = 0;
    //results of the query
    sqlite3_stmt *pResults;
    sqlite3_stmt *pResultsChunk;

    //OBJECTS IN THE WORLD
    //prepare SQL Text
    sqlCommand = "SELECT * from Objects";   //sql command which will return all records from the objects table.
    //Send Command and fill result object
    rc = sqlite3_prepare_v2(m_databaseConnection, sqlCommand, -1, &pResults, 0);

    //loop for each row in results until there are no more rows.  ie for every row in the results. We create and object
    while (sqlite3_step(pResults) == SQLITE_ROW)
    {
        SceneObject newSceneObject;
        newSceneObject.ID = sqlite3_column_int(pResults, 0);
        newSceneObject.chunk_ID = sqlite3_column_int(pResults, 1);
        newSceneObject.model_path = reinterpret_cast<const char*>(sqlite3_column_text(pResults, 2));
        newSceneObject.tex_diffuse_path = reinterpret_cast<const char*>(sqlite3_column_text(pResults, 3));
        newSceneObject.posX = static_cast<float>(sqlite3_column_double(pResults, 4));
        newSceneObject.posY = static_cast<float>(sqlite3_column_double(pResults, 5));
        newSceneObject.posZ = static_cast<float>(sqlite3_column_double(pResults, 6));
        newSceneObject.rotX = static_cast<float>(sqlite3_column_double(pResults, 7));
        newSceneObject.rotY = static_cast<float>(sqlite3_column_double(pResults, 8));
        newSceneObject.rotZ = static_cast<float>(sqlite3_column_double(pResults, 9));
        newSceneObject.scaX = static_cast<float>(sqlite3_column_double(pResults, 10));
        newSceneObject.scaY = static_cast<float>(sqlite3_column_double(pResults, 11));
        newSceneObject.scaZ = static_cast<float>(sqlite3_column_double(pResults, 12));
        newSceneObject.render = (sqlite3_column_int(pResults, 13) != 0);
        newSceneObject.collision = (sqlite3_column_int(pResults, 14) != 0);
        newSceneObject.collision_mesh = reinterpret_cast<const char*>(sqlite3_column_text(pResults, 15));
        newSceneObject.collectable = (sqlite3_column_int(pResults, 16) != 0);
        newSceneObject.destructable = (sqlite3_column_int(pResults, 17) != 0);
        newSceneObject.health_amount = sqlite3_column_int(pResults, 18);
        newSceneObject.editor_render = (sqlite3_column_int(pResults, 19) != 0);
        newSceneObject.editor_texture_vis = (sqlite3_column_int(pResults, 20) != 0);
        newSceneObject.editor_normals_vis = (sqlite3_column_int(pResults, 21) != 0);
        newSceneObject.editor_collision_vis = (sqlite3_column_int(pResults, 22) != 0);
        newSceneObject.editor_pivot_vis = (sqlite3_column_int(pResults, 23) != 0);
        newSceneObject.pivotX = static_cast<float>(sqlite3_column_double(pResults, 24));
        newSceneObject.pivotY = static_cast<float>(sqlite3_column_double(pResults, 25));
        newSceneObject.pivotZ = static_cast<float>(sqlite3_column_double(pResults, 26));
        newSceneObject.snapToGround = (sqlite3_column_int(pResults, 27) != 0);
        newSceneObject.AINode = (sqlite3_column_int(pResults, 28) != 0);
        newSceneObject.audio_path = reinterpret_cast<const char*>(sqlite3_column_text(pResults, 29));
        newSceneObject.volume = static_cast<float>(sqlite3_column_double(pResults, 30));
        newSceneObject.pitch = static_cast<float>(sqlite3_column_double(pResults, 31));
        newSceneObject.pan = static_cast<float>(sqlite3_column_int(pResults, 32));
        newSceneObject.one_shot = (sqlite3_column_int(pResults, 33) != 0);
        newSceneObject.play_on_init = (sqlite3_column_int(pResults, 34) != 0);
        newSceneObject.play_in_editor = (sqlite3_column_int(pResults, 35) != 0);
        newSceneObject.min_dist = static_cast<int>(sqlite3_column_double(pResults, 36));
        newSceneObject.max_dist = static_cast<int>(sqlite3_column_double(pResults, 37));
        newSceneObject.camera = (sqlite3_column_int(pResults, 38) != 0);
        newSceneObject.path_node = (sqlite3_column_int(pResults, 39) != 0);
        newSceneObject.path_node_start = (sqlite3_column_int(pResults, 40) != 0);
        newSceneObject.path_node_end = (sqlite3_column_int(pResults, 41) != 0);
        newSceneObject.parent_id = sqlite3_column_int(pResults, 42);
        newSceneObject.editor_wireframe = (sqlite3_column_int(pResults, 43) != 0);
        newSceneObject.name = reinterpret_cast<const char*>(sqlite3_column_text(pResults, 44));

        //send completed object to scenegraph
        m_sceneGraph.push_back(newSceneObject);
    }

    //THE WORLD CHUNK
    //prepare SQL Text
    sqlCommand = "SELECT * from Chunks";    //sql command which will return all records from  chunks table. There is only one tho.

    //Send Command and fill result object
    rc = sqlite3_prepare_v2(m_databaseConnection, sqlCommand, -1, &pResultsChunk, 0);


    sqlite3_step(pResultsChunk);
    m_chunk.ID = sqlite3_column_int(pResultsChunk, 0);
    m_chunk.name = reinterpret_cast<const char*>(sqlite3_column_text(pResultsChunk, 1));
    m_chunk.chunk_x_size_metres = sqlite3_column_int(pResultsChunk, 2);
    m_chunk.chunk_y_size_metres = sqlite3_column_int(pResultsChunk, 3);
    m_chunk.chunk_base_resolution = sqlite3_column_int(pResultsChunk, 4);
    m_chunk.heightmap_path = reinterpret_cast<const char*>(sqlite3_column_text(pResultsChunk, 5));
    m_chunk.tex_diffuse_path = reinterpret_cast<const char*>(sqlite3_column_text(pResultsChunk, 6));
    m_chunk.tex_splat_alpha_path = reinterpret_cast<const char*>(sqlite3_column_text(pResultsChunk, 7));
    m_chunk.tex_splat_1_path = reinterpret_cast<const char*>(sqlite3_column_text(pResultsChunk, 8));
    m_chunk.tex_splat_2_path = reinterpret_cast<const char*>(sqlite3_column_text(pResultsChunk, 9));
    m_chunk.tex_splat_3_path = reinterpret_cast<const char*>(sqlite3_column_text(pResultsChunk, 10));
    m_chunk.tex_splat_4_path = reinterpret_cast<const char*>(sqlite3_column_text(pResultsChunk, 11));
    m_chunk.render_wireframe = (sqlite3_column_int(pResultsChunk, 12) != 0);
    m_chunk.render_normals = (sqlite3_column_int(pResultsChunk, 13) != 0);
    m_chunk.tex_diffuse_tiling = sqlite3_column_int(pResultsChunk, 14);
    m_chunk.tex_splat_1_tiling = sqlite3_column_int(pResultsChunk, 15);
    m_chunk.tex_splat_2_tiling = sqlite3_column_int(pResultsChunk, 16);
    m_chunk.tex_splat_3_tiling = sqlite3_column_int(pResultsChunk, 17);
    m_chunk.tex_splat_4_tiling = sqlite3_column_int(pResultsChunk, 18);


    //Process REsults into renderable
    m_d3dRenderer.BuildDisplayList(&m_sceneGraph);
    //build the renderable chunk 
    m_d3dRenderer.BuildDisplayChunk(&m_chunk);

}

void ToolMain::onActionSave()
{
    //SQL
    int rc;
    char *sqlCommand;
    char *ErrMSG = 0;
    //results of the query
    sqlite3_stmt *pResults;


    //OBJECTS IN THE WORLD Delete them all
    //prepare SQL Text
    sqlCommand = "DELETE FROM Objects";	 //will delete the whole object table.   Slightly risky but hey.
    rc = sqlite3_prepare_v2(m_databaseConnection, sqlCommand, -1, &pResults, 0);
    sqlite3_step(pResults);

    //Populate with our new objects
    std::wstring sqlCommand2;
    int numObjects = m_sceneGraph.size();	//Loop thru the scengraph.

    for (int i = 0; i < numObjects; i++)
    {
        std::stringstream command;
        command << "INSERT INTO Objects "
            << "VALUES(" << m_sceneGraph.at(i).ID << ","
            << m_sceneGraph.at(i).chunk_ID << ","
            << "'" << m_sceneGraph.at(i).model_path << "'" << ","
            << "'" << m_sceneGraph.at(i).tex_diffuse_path << "'" << ","
            << m_sceneGraph.at(i).posX << ","
            << m_sceneGraph.at(i).posY << ","
            << m_sceneGraph.at(i).posZ << ","
            << m_sceneGraph.at(i).rotX << ","
            << m_sceneGraph.at(i).rotY << ","
            << m_sceneGraph.at(i).rotZ << ","
            << m_sceneGraph.at(i).scaX << ","
            << m_sceneGraph.at(i).scaY << ","
            << m_sceneGraph.at(i).scaZ << ","
            << m_sceneGraph.at(i).render << ","
            << m_sceneGraph.at(i).collision << ","
            << "'" << m_sceneGraph.at(i).collision_mesh << "'" << ","
            << m_sceneGraph.at(i).collectable << ","
            << m_sceneGraph.at(i).destructable << ","
            << m_sceneGraph.at(i).health_amount << ","
            << m_sceneGraph.at(i).editor_render << ","
            << m_sceneGraph.at(i).editor_texture_vis << ","
            << m_sceneGraph.at(i).editor_normals_vis << ","
            << m_sceneGraph.at(i).editor_collision_vis << ","
            << m_sceneGraph.at(i).editor_pivot_vis << ","
            << m_sceneGraph.at(i).pivotX << ","
            << m_sceneGraph.at(i).pivotY << ","
            << m_sceneGraph.at(i).pivotZ << ","
            << m_sceneGraph.at(i).snapToGround << ","
            << m_sceneGraph.at(i).AINode << ","
            << "'" << m_sceneGraph.at(i).audio_path << "'" << ","
            << m_sceneGraph.at(i).volume << ","
            << m_sceneGraph.at(i).pitch << ","
            << m_sceneGraph.at(i).pan << ","
            << m_sceneGraph.at(i).one_shot << ","
            << m_sceneGraph.at(i).play_on_init << ","
            << m_sceneGraph.at(i).play_in_editor << ","
            << m_sceneGraph.at(i).min_dist << ","
            << m_sceneGraph.at(i).max_dist << ","
            << m_sceneGraph.at(i).camera << ","
            << m_sceneGraph.at(i).path_node << ","
            << m_sceneGraph.at(i).path_node_start << ","
            << m_sceneGraph.at(i).path_node_end << ","
            << m_sceneGraph.at(i).parent_id << ","
            << m_sceneGraph.at(i).editor_wireframe << ","
            << "'" << m_sceneGraph.at(i).name << "'"
            << ")";
        std::string sqlCommand2 = command.str();
        rc = sqlite3_prepare_v2(m_databaseConnection, sqlCommand2.c_str(), -1, &pResults, 0);
        sqlite3_step(pResults);
    }
    MessageBox(NULL, L"Objects Saved", L"Notification", MB_OK);
}

void ToolMain::onActionSaveTerrain()
{
    m_d3dRenderer.SaveDisplayChunk(&m_chunk);
}

void ToolMain::OnWindowSizeChanged(int width, int height)
{
    m_d3dRenderer.OnWindowSizeChanged(width, height);
}

void ToolMain::Tick(MSG *msg)
{
    // Inputs
    auto mouse = m_mouse->GetState();
    m_mouseTracker->Update(mouse);

    auto keyboard = m_keyboard->GetState();
    m_kbTracker->Update(keyboard);

    if (m_kbTracker->IsKeyPressed(Keyboard::Space))
    {
        m_fpsCameraActive = !m_fpsCameraActive;
        m_mouse->SetMode(m_fpsCameraActive ? Mouse::MODE_RELATIVE : Mouse::MODE_ABSOLUTE);
    }

    //Renderer Update Call
    m_d3dRenderer.Tick(mouse, keyboard);
}

void ToolMain::UpdateInput(MSG * msg)
{
    UINT message = msg->message;
    WPARAM wParam = msg->wParam;
    LPARAM lParam = msg->lParam;

    switch (message)
    {
        case WM_ACTIVATEAPP:
            Mouse::ProcessMessage(message, wParam, lParam);
            Keyboard::ProcessMessage(message, wParam, lParam);
            break;

        case WM_INPUT:
        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_MOUSEWHEEL:
        case WM_XBUTTONDOWN:
        case WM_XBUTTONUP:
        case WM_MOUSEHOVER:
            Mouse::ProcessMessage(message, wParam, lParam);
            break;

        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP:
        case WM_SYSKEYDOWN:
            Keyboard::ProcessMessage(message, wParam, lParam);
            break;
    }
}