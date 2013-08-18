////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2012, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#ifndef _ENT3D_EDMAINWIN_H 
#define _ENT3D_EDMAINWIN_H 

#include <pkg/ent/editor/editor.h>
#include <orktool/ged/ged.h>
#include <orktool/qtui/qtmainwin.h>
#include <orktool/qtui/qtdataflow.h>

///////////////////////////////////////////////////////////////////////////
namespace ork { namespace ent {
///////////////////////////////////////////////////////////////////////////


class EditorMainWindow 	: public tool::MiniorkMainWindow
						, public ork::AutoConnector
{
	RttiDeclareAbstract( EditorMainWindow, ork::AutoConnector );

	//////////////////////////////////////////////////////////

	DeclarePublicSignal( NewObject );

	DeclarePublicAutoSlot( UpdateAll );
	DeclarePublicAutoSlot( OnTimer );
	DeclarePublicAutoSlot( SceneInstInvalidated );
	DeclarePublicAutoSlot( ObjectSelected );
	DeclarePublicAutoSlot( ObjectDeSelected );
	DeclarePublicAutoSlot( SpawnNewGed );
	DeclarePublicAutoSlot( ClearSelection );
	DeclarePublicAutoSlot( PostNewObject );

	//////////////////////////////////////////////////////////

	DeclareMoc( EditorMainWindow, tool::MiniorkMainWindow );

	//////////////////////////////////////////////////////////

	void AddBuiltInActions();

	//////////////////////////////////////////////////////////

public://

	SceneEditorBase					mEditorBase;

	typedef void(EditorMainWindow::*IntMethodType)(int);
	QString							mCurrentFileName;
	QWidget*						mpCTQTMain;
	QDirModel*						mDirModel;
	tool::ged::ObjModel				mGedModelObj;
//	tool::ged::ObjModel				mApplicationModelObj;
	tool::DataFlowEditor			mDataflowEditor;

	QTimer							mQtTimer;

	QTreeView*						mpEntityView;
	QTreeView*						mpArchView;
	QSplashScreen*					mpSplashScreen;
	orkvector<std::string>			mScripts;
	QApplication&					mQtApplication;

	///////////////////////////////////////////////////////////////////////////

	void SlotUpdateAll();
	void SlotOnTimer();
	void SlotSceneInstInvalidated( ork::Object* pSI );
	void SlotObjectSelected( ork::Object* pobj );
	void SlotObjectDeSelected( ork::Object* pobj );
	void SlotSpawnNewGed( ork::Object* pobj );
	void SlotClearSelection();
	void SlotPostNewObject( ork::Object* pobj );

	void SigNewObject( ork::Object* pobj );

	bool event(QEvent *qevent); /*virtual*/

	///////////////////////////////////////////////////////////////////////////

	QDockWidget *  NewCamView( bool bfloat );

	void NewOutliner2View();
	void NewDataflowView();
	
	//QDockWidget * NewToolView( bool bfloat );
	//QDockWidget * NewDataflowView( bool bfloat );
	//QDockWidget * NewPyConView( bool bfloat );
	
	/////////////////////////////////////////////////
	// this Ged is special it is the "master" ged, it cannot be closed
	// and has some permanent connections to:
	// the scene editor
	// the editormainwindow
	/////////////////////////////////////////////////

	void SceneObjPropEdit();

	///////////////////////////////////////////////////////////////////////////

	void NewHierView( bool bfloat );
	void NewCamViewFloating();
	void NewToolViewFloating();
	void NewDataflowViewFloating();

	///////////////////////////////////////////////////////////////////////////

	void NewDirView();
	void LightingHeadLightMode();
	void LightingSceneMode();
	void LightingSetLightPos();
	void ViewToggleCollisionSpheres();
	void OpenSceneFile();
	void SaveSceneFile();
	void MergeFile();
	void SaveSelected();
	void RunGame();
	void RunLevel();
	void Group();
	void ArchExport();
	void ArchImport();
	void ArchMakeReferenced();
	void ArchMakeLocal();
	void NewEntity();
	void NewEntities();
	void NewScene();
	void Dupe();
	void RefreshModels();
	void RefreshAnims();
	void RefreshTextures();
	void RefreshHFSMs();
	void SaveCubeMap();
	void RunLocal();
	void StopLocal();
	void SaveLayout();
	void LoadLayout();
	
	///////////////////////////////////////////////////////////////////////////

	EditorMainWindow(QWidget *parent, const std::string& applicationClassName, QApplication & App);
	~EditorMainWindow();
};

extern ent::EditorMainWindow *gEditorMainWindow;

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

inline void EditorMainWindow::NewCamViewFloating()
{
//	NewCamView(true);
}

inline void EditorMainWindow::NewToolViewFloating()
{
//	NewToolView(true);
}

inline void EditorMainWindow::NewDataflowViewFloating()
{
//	NewDataflowView(true);
}

///////////////////////////////////////////////////////////////////////////
} // ent
} // ork
///////////////////////////////////////////////////////////////////////////
#endif
