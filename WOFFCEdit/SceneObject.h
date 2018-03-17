#pragma once

#include <string>


//This object should accurately and totally reflect the information stored in the object table


struct SceneObject
{

    int ID = 0;
    int chunk_ID = 0;

    std::string model_path = "";
    std::string tex_diffuse_path = "";

    float posX = 0.f;
    float posY = 0.f;
    float posZ = 0.f;

    float rotX = 0.f;
    float rotY = 0.f;
    float rotZ = 0.f;

    float scaX = 0.f;
    float scaY = 0.f;
    float scaZ = 0.f;

    bool render = true;
    bool collision = false;
    std::string collision_mesh = "";

    bool collectable = false;
    bool destructable = false;
    int health_amount = 0;

    bool editor_render = true;
    bool editor_texture_vis = true;
    bool editor_normals_vis = false;
    bool editor_collision_vis = false;
    bool editor_pivot_vis = false;

    float pivotX = 0.f;
    float pivotY = 0.f;
    float pivotZ = 0.f;

    bool snapToGround = false;

    bool AINode = false;
    std::string audio_path = "";
    float volume = 0.f;
    float pitch = 0.f;
    float pan = 0.f;
    bool one_shot = false;
    bool play_on_init = false;
    bool play_in_editor = false;
    int min_dist = 0;
    int max_dist = 0;

    bool camera = false;
    bool path_node = false;
    bool path_node_start = false;
    bool path_node_end = false;

    int parent_id = 0;
    bool editor_wireframe = false;
    std::string name = "";

};

