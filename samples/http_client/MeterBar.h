#pragma once
#include "afxcmn.h"
#include "MeterCtrl.h"

// CMeterBar

class CMeterBar : public CStatusBarCtrl
{
	DECLARE_DYNAMIC(CMeterBar)

public:
	CMeterBar();
	virtual ~CMeterBar();

	BOOL SetParts(int nParts, int* pWidths);

	CMeterCtrl& GetProgressCtrl() {
		return m_meter;
	}

protected:
	DECLARE_MESSAGE_MAP()
	//CProgressCtrl m_meter;
	CMeterCtrl m_meter;
public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
private:
	int *m_pWidths;
	int  m_nParts;
};


