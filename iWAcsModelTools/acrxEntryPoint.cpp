// (C) Copyright 2002-2005 by Autodesk, Inc. 
//
// Permission to use, copy, modify, and distribute this software in
// object code form for any purpose and without fee is hereby granted, 
// provided that the above copyright notice appears in all copies and 
// that both that copyright notice and the limited warranty and
// restricted rights notice below appear in all supporting 
// documentation.
//
// AUTODESK PROVIDES THIS PROGRAM "AS IS" AND WITH ALL FAULTS. 
// AUTODESK SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTY OF
// MERCHANTABILITY OR FITNESS FOR A PARTICULAR USE.  AUTODESK, INC. 
// DOES NOT WARRANT THAT THE OPERATION OF THE PROGRAM WILL BE
// UNINTERRUPTED OR ERROR FREE.
//
// Use, duplication, or disclosure by the U.S. Government is subject to 
// restrictions set forth in FAR 52.227-19 (Commercial Computer
// Software - Restricted Rights) and DFAR 252.227-7013(c)(1)(ii)
// (Rights in Technical Data and Computer Software), as applicable.
//

//-----------------------------------------------------------------------------
//----- acrxEntryPoint.h
//-----------------------------------------------------------------------------
#include "StdAfx.h"
#include "resource.h"
#include "CiWAcsModelTools.h"
//#include "MdiTabMain.h"
#include "CAcDocMgrReator.h"

//#include "tab_src_files/CustomTabCtrl.h"
//-----------------------------------------------------------------------------
#define szRDS _RXST("IBR")


//CRTabCtrl *gpTabCtrl = NULL;
//CAcDocMgrReator *gpDcoMgrReactor = NULL;
//-----------------------------------------------------------------------------
//----- ObjectARX EntryPoint
class CiWAcsModelToolsApp : public AcRxArxApp {

public:
	CiWAcsModelToolsApp () : AcRxArxApp () {}

	virtual AcRx::AppRetCode On_kInitAppMsg (void *pkt) {
		// TODO: Load dependencies here

		// You *must* call On_kInitAppMsg here
		AcRx::AppRetCode retCode =AcRxArxApp::On_kInitAppMsg (pkt) ;
		
		// TODO: Add your initialization code here

// 		acrxDynamicLinker->registerAppMDIAware( pkt );
// 		gpDcoMgrReactor = new CAcDocMgrReator();
// 		acDocManagerPtr()->addReactor(gpDcoMgrReactor);
// 
// 		CAcModuleResourceOverride res;
// 
// 		gpTabCtrl = new CRTabCtrl();
// 		gpTabCtrl->Create( WS_VISIBLE|WS_CHILD|CTCS_AUTOHIDEBUTTONS|CTCS_TOOLTIPS|CTCS_TOP|TAB_SHAPE5 |TAB_SHAPE3 | CTCS_DRAGMOVE, CRect(0,0,100,20), acedGetAcadFrame(), WM_USER + 2 );
// 
// 		if ( acDocManagerPtr()->documentCount() <=1 )
// 		{
// 			gpTabCtrl->MoveWindow(CRect(0,0,0.1,0.1));
// 			gpTabCtrl->GetParentFrame()->RecalcLayout();
// 		}
// 		else
// 		{
// 			gpTabCtrl->MoveWindow(CRect(0,0,100,20));
// 			gpTabCtrl->GetParentFrame()->RecalcLayout();
// 		}
// 		acedGetAcadFrame()->RecalcLayout();
		return (retCode) ;
	}

	virtual AcRx::AppRetCode On_kUnloadAppMsg (void *pkt) {
		// TODO: Add your code here

		// You *must* call On_kUnloadAppMsg here
		AcRx::AppRetCode retCode =AcRxArxApp::On_kUnloadAppMsg (pkt) ;

		// TODO: Unload dependencies here
// 		if ( gpDcoMgrReactor )
// 		{
// 			acDocManagerPtr()->removeReactor( gpDcoMgrReactor );
// 			delete gpDcoMgrReactor;
// 			gpDcoMgrReactor = NULL;
// 		}
// 
// 		if ( gpTabCtrl )
// 		{
// 			gpTabCtrl->MoveWindow(CRect(0,0,0.1,0.1));
// 			gpTabCtrl->DestroyWindow();
// 
// 			delete gpTabCtrl;
// 			gpTabCtrl = NULL;
// 
// 			acedGetAcadFrame()->RecalcLayout();
// 		}

		return (retCode) ;
	}

	virtual void RegisterServerComponents () {
	}

	// 将提资内容复制到提资文件
	static void iW_iwCopyToProvideFile(void)
	{
		if (!CiWAcsModelTools::Normalize())
		{
			CiWAcsModelTools::iwCopyToProvideFile();

		}
	}
} ;

//-----------------------------------------------------------------------------
IMPLEMENT_ARX_ENTRYPOINT(CiWAcsModelToolsApp)
ACED_ARXCOMMAND_ENTRY_AUTO(CiWAcsModelToolsApp, iW_, iwCopyToProvideFile, iwCopyToProvideFile, ACRX_CMD_TRANSPARENT, NULL)
