#pragma once
#include <afxwin.h> 
#include <afxext.h>

class ToolMain;

// CChildView window

class CChildRender : public CWnd
{
	// Construction
public:

	// Attributes
public:
    ToolMain* toolSystem = nullptr;

	// Operations
public:

	// Overrides
protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs) override;

	// Implementation
public:
	virtual ~CChildRender() = default;

	// Generated message map functions
protected:
	afx_msg void OnPaint();
    afx_msg void OnSize(UINT nType, int width, int height);

	DECLARE_MESSAGE_MAP()
};

