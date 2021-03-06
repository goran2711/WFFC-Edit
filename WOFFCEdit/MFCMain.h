#pragma once

#include <afxwin.h> 
#include <afxext.h>
#include <afx.h>
#include "ToolMain.h"
#include "SelectDialogue.h"

class CMyFrame;

class MFCMain : public CWinApp
{
public:
    BOOL InitInstance() override;
    int  Run() override;

private:

    CMyFrame * m_frame;	//handle to the frame where all our UI is
    HWND m_toolHandle;	//Handle to the MFC window
    ToolMain m_ToolSystem;	//Instance of Tool System that we interface to. 
    SelectDialogue m_ToolSelectDialogue;			//for modeless dialogue, declare it here

    int m_width;
    int m_height;

    //Interface funtions for menu and toolbar etc requires
    afx_msg void MenuFileQuit();
    afx_msg void MenuFileSaveTerrain();
    afx_msg void MenuEditSelect();
    afx_msg	void ToolBarButton1();


    DECLARE_MESSAGE_MAP()	// required macro for message map functionality  One per class
};
