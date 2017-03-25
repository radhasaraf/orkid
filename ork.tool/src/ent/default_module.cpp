////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2012, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#include <orktool/qtui/qtui_tool.h>
#include <ork/lev2/qtui/qtui.hpp>
#include <orktool/qtui/qtmainwin.h>
///////////////////////////////////////////////////////////////////////////
#include <pkg/ent/editor/edmainwin.h>
#include <orktool/toolcore/FunctionManager.h>
#include <QtGui/qmessagebox.h>
#include <ork/util/hotkey.h>
#include <ork/kernel/opq.h>
#include <ork/kernel/debug.h>

///////////////////////////////////////////////////////////////////////////////
namespace ork { namespace ent {
///////////////////////////////////////////////////////////////////////////
class MainWinDefaultModule : public tool::EditorModule
{
	DeclareMoc( MainWinDefaultModule, EditorModule );
	void OnAction( const char* pact ) final;
	void Activate( QMenuBar* qmb ) final;
	void DeActivate( QMenuBar* qmb ) final;

	EditorMainWindow& mEditWin;

public:
	MainWinDefaultModule(EditorMainWindow& emw)
		: mEditWin( emw )
	{
	}
};
///////////////////////////////////////////////////////////////////////////////
ImplementMoc( MainWinDefaultModule, tool::EditorModule );
///////////////////////////////////////////////////////////////////////////////
void MainWinDefaultModule::Activate( QMenuBar* qmb )
{
	mMenuBar = qmb;

	AddAction( "/Scene/NewScene"  ,QKeySequence(tr("Ctrl+N")) );
	AddAction( "/Scene/OpenScene" ,QKeySequence(tr(HotKeyManager::GetAcceleratorCode("open").c_str())));
	AddAction( "/Scene/SaveScene" ,QKeySequence(tr(HotKeyManager::GetAcceleratorCode("save").c_str())));
	AddAction( "/Scene/ExportArchetype" );
	AddAction( "/Scene/ImportArchetype" );

	//AddAction( "/View/Outliner" );
	//AddAction( "/View/Outliner2" );
	//AddAction( "/View/DataflowEditor" );
	//AddAction( "/View/ToolEditor" );
	//AddAction( "/View/PropEditor" );
	AddAction( "/View/SaveLayout" );
	AddAction( "/View/LoadLayout" );

	AddAction( "/Game/Local/Run", QKeySequence(tr("Ctrl+.")) );
	AddAction( "/Game/Local/Stop", QKeySequence(tr("Ctrl+,")) );

	AddAction( "/Entity/New Entity" ,QKeySequence(tr("Ctrl+E")) );
	AddAction( "/Entity/New Entities..." ,QKeySequence(tr("Ctrl+Shift+E")) );
	AddAction( "/Entity/Group" );

	AddAction( "/&Archetype/E&xport" ,QKeySequence(tr("Ctrl+Shift+X")) );
	AddAction( "/&Archetype/I&mport" ,QKeySequence(tr("Ctrl+Shift+M")) );
	//AddAction( "/&Archetype/Make &Referenced" ,QKeySequence(tr("Ctrl+Shift+R")) );
	//AddAction( "/&Archetype/Make &Local" ,QKeySequence(tr("Ctrl+Shift+L")) );

	AddAction( "/Refresh/Models" );
	AddAction( "/Refresh/Anims" );
	AddAction( "/Refresh/Textures" );
	AddAction( "/Refresh/Chsms" );


	//mEditWin.NewToolView(false);
}
///////////////////////////////////////////////////////////////////////////////
void MainWinDefaultModule::DeActivate( QMenuBar* qmb )
{
	OrkAssert( qmb==mMenuBar );
	mMenuBar = 0;
}
///////////////////////////////////////////////////////////////////////////////
void MainWinDefaultModule::OnAction( const char* pact )
{
	     if( 0 == strcmp( "/Scene/NewScene", pact ) )			{	mEditWin.NewScene();		}
	else if( 0 == strcmp( "/Scene/OpenScene", pact ) )			{	mEditWin.OpenSceneFile();	}
	else if( 0 == strcmp( "/Scene/ExportArchetype", pact))		{	mEditWin.SaveSelected();	}
	else if( 0 == strcmp( "/Scene/ImportArchetype", pact ) )	{	mEditWin.MergeFile();		}
	///////////////////////////////////////////////////////
//	else if( 0 == strcmp( "/View/Outliner",pact) )				{	mEditWin.NewOutlinerView(false); }
//	else if( 0 == strcmp( "/View/Outliner2",pact) )				{	mEditWin.NewOutliner2View(false); }
//	else if( 0 == strcmp( "/View/PyCon",pact) )					{	mEditWin.NewPyConView(false); }
//	else if( 0 == strcmp( "/View/DataflowEditor",pact) )		{	mEditWin.NewDataflowView(); }
//	else if( 0 == strcmp( "/View/ToolEditor",pact) )			{	mEditWin.NewToolView(false); }
	else if( 0 == strcmp( "/View/SaveLayout",pact) )			{	mEditWin.SaveLayout(); }
	else if( 0 == strcmp( "/View/LoadLayout",pact) )			{	mEditWin.LoadLayout(); }
	///////////////////////////////////////////////////////
	else if( 0 == strcmp( "/Game/Local/Run",pact) )				{	mEditWin.RunLocal(); }
	else if( 0 == strcmp( "/Game/Local/Stop",pact) )			{	mEditWin.StopLocal(); }
	///////////////////////////////////////////////////////
	else if( 0 == strcmp( "/Entity/New Entity",pact) )			{	mEditWin.NewEntity(); }
	else if( 0 == strcmp( "/Entity/New Entities...",pact) )		{	mEditWin.NewEntities(); }
	else if( 0 == strcmp( "/Entity/Group",pact) )				{	mEditWin.Group(); }
	///////////////////////////////////////////////////////
	else if( 0 == strcmp( "/&Archetype/E&xport",pact) )			{	mEditWin.ArchExport(); }
	else if( 0 == strcmp( "/&Archetype/I&mport",pact) )			{	mEditWin.ArchImport(); }
	//else if( 0 == strcmp( "/&Archetype/Make &Referenced",pact) ){	mEditWin.ArchMakeReferenced(); }
	//else if( 0 == strcmp( "/&Archetype/Make &Local",pact) )		{	mEditWin.ArchMakeLocal(); }
	///////////////////////////////////////////////////////
	else if( 0 == strcmp( "/Refresh/Models",pact) )				{	mEditWin.RefreshModels(); }
	else if( 0 == strcmp( "/Refresh/Anims",pact) )				{	mEditWin.RefreshAnims(); }
	else if( 0 == strcmp( "/Refresh/Textures",pact) )			{	mEditWin.RefreshTextures(); }
////////////////////////////////////////////////////////////////////////////////////////////////
	else if( 0 == strcmp( "/Scene/SaveScene", pact ) )	{
		mEditWin.SaveSceneFile();

	}

}

///////////////////////////////////////////////////////////////////////////////
void MainWinDefaultModule::MocInit()
{
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void RegisterMainWinDefaultModule( EditorMainWindow& emw )
{
	emw.ModuleMgr().AddModule( "Default", new MainWinDefaultModule( emw ) );
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
void EditorMainWindow::NewScene()
{
	mEditorBase.QueueOpASync(NewSceneReq());
	SetSceneFile(QString("UNTITLED"));
}
///////////////////////////////////////////////////////////////////////////
void EditorMainWindow::RunLocal()
{
	mEditorBase.QueueOpASync(RunLocalReq());
}
///////////////////////////////////////////////////////////////////////////
void EditorMainWindow::StopLocal()
{
	mEditorBase.QueueOpASync(StopLocalReq());
}
///////////////////////////////////////////////////////////////////////////
void EditorMainWindow::RunGame()
{
}
///////////////////////////////////////////////////////////////////////////
void EditorMainWindow::RunLevel()
{
}
///////////////////////////////////////////////////////////////////////////
void EditorMainWindow::RefreshAnims()
{
	mEditorBase.EditorRefreshAnims();
}
///////////////////////////////////////////////////////////////////////////
void EditorMainWindow::RefreshModels()
{
	mEditorBase.EditorRefreshModels();
}
///////////////////////////////////////////////////////////////////////////
void EditorMainWindow::RefreshHFSMs()
{
	//mEditorBase.EditorRefreshHFSMs();
}
///////////////////////////////////////////////////////////////////////////
void EditorMainWindow::RefreshTextures()
{
	mEditorBase.EditorRefreshTextures();
}}}
///////////////////////////////////////////////////////////////////////////////
