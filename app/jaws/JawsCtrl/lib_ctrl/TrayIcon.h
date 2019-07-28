////////////////////////////////////////////////////////////////
// CTrayIcon Copyright 1996 Microsoft Systems Journal.
//
// If this code works, it was written by Paul DiLascia.
// If not, I don't know who wrote it.

#ifndef _TRAYICON_H
#define _TRAYICON_H

////////////////
// CTrayIcon manages an icon in the Windows 95 system tray. 
// 
class  AFX_EXT_CLASS CTrayIcon : public CCmdTarget {
protected:
	DECLARE_DYNAMIC(CTrayIcon)
	NOTIFYICONDATA m_nid;			// struct for Shell_NotifyIcon args

public:
	CTrayIcon(UINT uID);
	~CTrayIcon();

	// Call this to receive tray notifications
	void SetNotificationWnd(CWnd* pNotifyWnd, UINT uCbMsg);

	// SetIcon functions. To remove icon, call SetIcon(0)
	//
	BOOL SetIcon(UINT uID); // main variant you want to use
	BOOL SetIcon(HICON hicon, LPCSTR lpTip);
	BOOL SetIcon(LPCTSTR lpResName, LPCSTR lpTip)
		{ return SetIcon(lpResName ? 
			AfxGetApp()->LoadIcon(lpResName) : NULL, lpTip); }
	BOOL SetStandardIcon(LPCTSTR lpszIconName, LPCSTR lpTip)
		{ return SetIcon(::LoadIcon(NULL, lpszIconName), lpTip); }

	virtual LRESULT OnTrayNotification(WPARAM uID, LPARAM lEvent);
};

#endif
