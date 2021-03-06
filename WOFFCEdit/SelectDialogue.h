#pragma once
#include "afxdialogex.h"
#include <vector>

struct SceneObject;

// SelectDialogue dialog

class SelectDialogue : public CDialogEx
{
    DECLARE_DYNAMIC(SelectDialogue)

public:
    SelectDialogue(CWnd* pParent, std::vector<SceneObject>* SceneGraph);   // modal // takes in out scenegraph in the constructor
    explicit SelectDialogue(CWnd* pParent = NULL);

    virtual ~SelectDialogue() = default;

    void SetObjectData(std::vector<SceneObject>* SceneGraph, int * Selection);	//passing in pointers to the data the class will operate on.

// Dialog Data
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_DIALOG1 };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
    afx_msg void End();		//kill the dialogue
    afx_msg void Select();	//Item has been selected

    std::vector<SceneObject> * m_sceneGraph;
    int * m_currentSelection;


    DECLARE_MESSAGE_MAP()
public:
    // Control variable for more efficient access of the listbox
    CListBox m_listBox;
    virtual BOOL OnInitDialog() override;
    virtual void PostNcDestroy();
    afx_msg void OnBnClickedOk();
    afx_msg void OnLbnSelchangeList1();
};


INT_PTR CALLBACK SelectProc(HWND   hwndDlg, UINT   uMsg, WPARAM wParam, LPARAM lParam);