#pragma once

#include <afxext.h>
#include "pch.h"
#include "Game.h"
#include "sqlite3.h"
#include "SceneObject.h"
#include "UserInput.h"
#include <vector>


class ToolMain
{
public: //methods
    ToolMain() = default;

    // Rule of three
    ToolMain(const ToolMain&) = delete;
    ToolMain& operator=(const ToolMain&) = delete;
    ~ToolMain();

    //onAction - These are the interface to MFC
    int		getCurrentSelectionID() const;									//returns the selection number of currently selected object so that It can be displayed.
    void	onActionInitialise(HWND handle, int width, int height);			//Passes through handle and hieght and width and initialises DirectX renderer and SQL LITE
    void	onActionFocusCamera();
    void	onActionLoad();													//load the current chunk
    afx_msg	void	onActionSave();											//save the current chunk
    afx_msg void	onActionSaveTerrain();									//save chunk geometry

    void OnWindowSizeChanged(int width, int height);

    void	Tick(MSG *msg);
    void	UpdateInput(MSG *msg);
    UserInput    HandleInput();

public:	//variables
    std::vector<SceneObject>    m_sceneGraph;	//our scenegraph storing all the objects in the current chunk
    ChunkObject					m_chunk;		//our landscape chunk
    int m_selectedObject = 0;					//ID of current Selection

private:	//methods
    void	onContentAdded();



private:	//variables
    HWND	m_toolHandle;		//Handle to the  window
    Game	m_d3dRenderer;		//Instance of D3D rendering system for our tool
    CRect	WindowRECT;		//Window area rectangle. 
    sqlite3 *m_databaseConnection = nullptr;	//sqldatabase handle

    int m_width;		//dimensions passed to directX
    int m_height;
    int m_currentChunk = 0;			//the current chunk of thedatabase that we are operating on.  Dictates loading and saving. 

    // Input devices.
    std::unique_ptr<DirectX::GamePad>       m_gamePad;
    std::unique_ptr<DirectX::Keyboard>      m_keyboard;
    std::unique_ptr<DirectX::Mouse>         m_mouse;
    std::unique_ptr<DirectX::Mouse::ButtonStateTracker> m_mouseStateTracker;
};
