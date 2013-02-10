////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2012, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#include <orktool/qtui/qtui_tool.h>
#include <ork/lev2/qtui/qtui.hpp>
///////////////////////////////////////////////////////////////////////////////
#include <orktool/qtui/qtmainwin.h>
#include <orktool/qtui/uitoolhandler.h>
#include <orktool/toolcore/FunctionManager.h>
#include <ork/util/hotkey.h>
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
using namespace ork::lev2;
extern bool gbheadlight;
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
namespace ork { namespace tool {
///////////////////////////////////////////////////////////////////////////////
//ImplementMoc( MiniorkMainWindow, QMainWindow );
//ImplementMoc( EditorModule, QObject );
///////////////////////////////////////////////////////////////////////////
#ifndef MINIORK_REPOS
#define MINIORK_REPOS		"UNKNOWN MINIORK REPOS"
#endif
//////////////////////////////////////////////////////////////////////////////
#ifndef GAME_REPOS
#define GAME_REPOS			"UNKNOWN GAME REPOS"
#endif
//////////////////////////////////////////////////////////////////////////////
#ifndef MINIORK_REVISION
#define MINIORK_REVISION	"UNKNOWN MINIORK REVISION"
#endif
//////////////////////////////////////////////////////////////////////////////
#ifndef GAME_REVISION
#define GAME_REVISION		"UNKNOWN GAME REVISION"
#endif
///////////////////////////////////////////////////////////////////////////
//const QEvent::Type QQedRefreshEvent::gevtype ;
///////////////////////////////////////////////////////////////////////////
QQedRefreshEvent::QQedRefreshEvent()
	: QEvent( QQedRefreshEvent::gevtype )
{
}
//////////////////////////////////////////////////////////////////////////////
MiniorkMainWindow::MiniorkMainWindow(QWidget* parent, Qt::WindowFlags flags) 
	: QMainWindow(parent, flags)
	, mEditorModuleMgr( menuBar() )
{
	SetMainTitle(QString("Miniork Tool"));
	SetMiniorkRepos(QString(MINIORK_REPOS));
	SetMiniorkRev(QString(MINIORK_REVISION));
	SetGameRepos(QString(GAME_REPOS));
	SetGameRev(QString(GAME_REVISION));
	SetSceneFile(QString("UNTITLED"));
	SetReadOnly(false);
}
//////////////////////////////////////////////////////////////////////////////
const QString& MiniorkMainWindow::GetMainTitle() const
{
	return mMainTitle;
}
//////////////////////////////////////////////////////////////////////////////
void MiniorkMainWindow::SetMainTitle(const QString& str)
{
	mMainTitle = str;
	UpdateTitle();
}
//////////////////////////////////////////////////////////////////////////////
const QString& MiniorkMainWindow::GetMiniorkRepos() const
{
	return mMiniorkRepos;
}
//////////////////////////////////////////////////////////////////////////////
void MiniorkMainWindow::SetMiniorkRepos(const QString& str)
{
	mMiniorkRepos = str;
	UpdateTitle();
}
//////////////////////////////////////////////////////////////////////////////
const QString& MiniorkMainWindow::GetMiniorkRev() const
{
	return mMiniorkRev;
}
//////////////////////////////////////////////////////////////////////////////
void MiniorkMainWindow::SetMiniorkRev(const QString& str)
{
	mMiniorkRev = str;
	UpdateTitle();
}
//////////////////////////////////////////////////////////////////////////////
const QString& MiniorkMainWindow::GetGameRepos() const
{
	return mGameRepos;
}
//////////////////////////////////////////////////////////////////////////////
void MiniorkMainWindow::SetGameRepos(const QString& str)
{
	mGameRepos = str;
	UpdateTitle();
}
//////////////////////////////////////////////////////////////////////////////
const QString& MiniorkMainWindow::GetGameRev() const
{
	return mGameRev;
}
//////////////////////////////////////////////////////////////////////////////
void MiniorkMainWindow::SetGameRev(const QString& str)
{
	mGameRev = str;
	UpdateTitle();
}
//////////////////////////////////////////////////////////////////////////////
const QString& MiniorkMainWindow::GetSceneFile() const
{
	return mSceneFile;
}
//////////////////////////////////////////////////////////////////////////////
void MiniorkMainWindow::SetSceneFile(const QString& str)
{
	mSceneFile = str;
	UpdateTitle();
}
//////////////////////////////////////////////////////////////////////////////
bool MiniorkMainWindow::IsReadOnly() const
{
	return mReadOnly;
}
//////////////////////////////////////////////////////////////////////////////
void MiniorkMainWindow::SetReadOnly(bool value)
{
	mReadOnly = value;
	UpdateTitle();
}
//////////////////////////////////////////////////////////////////////////////
void MiniorkMainWindow::FunctorAction()
{
	QAction *action = (QAction *)sender();
	reflect::IFunctor *functor = (reflect::IFunctor *)action->data().toUInt();
	reflect::IInvokation *invokation = functor->CreateInvokation();
	functor->invoke(invokation);
	delete invokation;
}

///////////////////////////////////////////////////////////////////////////

void MiniorkMainWindow::CreateFunctionMenu(QMenu *menu, const std::string &name, ork::reflect::IFunctor *functor)
{
	OrkAssert(menu);
	OrkAssert(functor);

	std::string::size_type pos = name.find('/');
	if(pos == std::string::npos)
	{
		QAction *action = menu->findChild<QAction *>(name.c_str());
		if(NULL == action)
		{
			action = menu->addAction(name.c_str());
			action->setObjectName(name.c_str());
		}

		action->setData(QVariant((qulonglong)functor));
		bool result = connect(action, SIGNAL(triggered()), this, SLOT(FunctorAction()));
		OrkAssert(result);
	}
	else
	{
		std::string childname(name.substr(0, pos));
		QMenu *child = menu->findChild<QMenu *>(childname.c_str());
		if(NULL == child || child->parent() != menu)
		{
			child = menu->addMenu(childname.c_str());
			child->setObjectName(childname.c_str());
		}

		CreateFunctionMenu(child, name.substr(pos + 1), functor);
	}
}

///////////////////////////////////////////////////////////////////////////

void MiniorkMainWindow::CreateFunctionMenu(const std::string &name, ork::reflect::IFunctor *functor)
{
	OrkAssert(menuBar());
	OrkAssert(functor);

	std::string::size_type pos = name.find('/');
	if(pos == std::string::npos)
	{
		QAction *action = menuBar()->findChild<QAction *>(name.c_str());
		if(NULL == action)
		{
			action = menuBar()->addAction(name.c_str());
			action->setObjectName(name.c_str());
		}

		action->setData(QVariant((qulonglong)functor));
		bool result = connect(action, SIGNAL(triggered()), SLOT(FunctorAction()));
		OrkAssert(result);
	}
	else
	{
		std::string childname(name.substr(0, pos));
		QMenu *child = menuBar()->findChild<QMenu *>(childname.c_str());
		if(NULL == child || child->parent() != menuBar())
		{
			child = menuBar()->addMenu(childname.c_str());
			child->setObjectName(childname.c_str());
		}

		CreateFunctionMenu(child, name.substr(pos + 1), functor);
	}
}

///////////////////////////////////////////////////////////////////////////

void MiniorkMainWindow::CreateFunctionMenus()
{
	const ork::FunctionManager::FunctionsByNameMap &functions = ork::FunctionManager::GetRef().GetFunctionsByName();
	for(ork::FunctionManager::FunctionsByNameMap::const_iterator it = functions.begin(); it != functions.end(); it++)
	{
		std::string name(it->first);
		std::string prefix("MainMenu/");
		if(name.substr(0, prefix.length()) == prefix)
			CreateFunctionMenu(name.substr(prefix.length()), it->second);
	}
}
//////////////////////////////////////////////////////////////////////////////
void MiniorkMainWindow::UpdateTitle()
{
	QString title = mMainTitle + QString(" - ") + mMiniorkRepos + QString(":") + mMiniorkRev + QString(" - ") + mGameRepos + QString(":") + mGameRepos;

	if(mSceneFile.length() > 0)
		title += QString(" - ") + mSceneFile;
	if(mReadOnly)
		title += QString(" - ") + QString("Read Only");

	//setWindowTitle(title);
}
//////////////////////////////////////////////////////////////////////////////
//void MiniorkMainWindow::MocInit()
//{
//	Moc.AddSlot0( "FunctorAction()", & MiniorkMainWindow::FunctorAction );
//}
///////////////////////////////////////////////////////////////////////////////
void EditorModuleMgr::AddModule( std::string name, EditorModule*mod )
{
	orkmap<std::string,EditorModule*>::const_iterator it = mModules.find(name);
	OrkAssert(it==mModules.end());
	mod->Activate( mMenuBar );
	mModules[name] = mod;
}
///////////////////////////////////////////////////////////////////////////////
void EditorModuleMgr::RemoveModule( std::string name )
{
	orkmap<std::string,EditorModule*>::iterator it = mModules.find(name);
	OrkAssert(it!=mModules.end());
	it->second->DeActivate( mMenuBar );
	mModules.erase(it);
}
///////////////////////////////////////////////////////////////////////////////
void EditorModule::AddAction( QMenu* pmenu, const char* pname)
{
	QAction * action = new QAction(tr(pname), this);
	pmenu->addAction( action );
	connect(action, SIGNAL(triggered()), this, SLOT(ActionSlot()));
}
///////////////////////////////////////////////////////////////////////////////
void EditorModule::AddAction( const char* pname )
{
	AddAction(   pname,0);
}
///////////////////////////////////////////////////////////////////////////////
void EditorModule::AddAction(  const char* pname,QKeySequence ks)
{
	ork::SlashNode* pnode = mSlashHier.add_node( pname, 0 );

	orkvector< const SlashNode * > path;

	pnode->GetPath( path );
	std::string pthstr = pnode->getfullpath();
	int inumnodes = int(path.size());
	QMenu* pcurmenu = 0;
	for(int i=0; i<inumnodes; i++ )
	{
		bool end = ((i+1)==inumnodes);
		const SlashNode *node = path[i];
	
		if( node != mSlashHier.GetRoot() )
		{
			SlashNode * nonconst = const_cast<SlashNode *>( node );

			const std::string& nm = node->GetNodeName();

			if( nm != "/" )
			{
				void* pdata = const_cast<void*>( nonconst->GetData() );

				if( end )
				{
					QAction* pact = pcurmenu->addAction( nm.c_str() );
					if(ks.count()) pact->setShortcut(ks);
					pact->setData( QVariant( tr(pthstr.c_str()) ) );
					connect(pact, SIGNAL(triggered()), this, SLOT(ActionSlot()));
				}
				else
				{
					if( 0 == pcurmenu )
					{	
						pcurmenu = (0==pdata) 
								 ? mMenuBar->addMenu( nm.c_str() )
								 : (QMenu*) pdata;
					}
					else
					{
						pcurmenu = (0==pdata) 
								 ? pcurmenu->addMenu( nm.c_str() )
								 : (QMenu*) pdata;
					}
					if( 0 == pdata )
					{
						nonconst->SetData( (void*) pcurmenu );
					}
				}
			}
		}
	}
}
///////////////////////////////////////////////////////////////////////////////
EditorModule::EditorModule()
	: mMenuBar( 0 )
{
}
///////////////////////////////////////////////////////////////////////////////
//void EditorModule::MocInit()
//{
//	Moc.AddSlot0( "ActionSlot()", & EditorModule::ActionSlot );
//}
///////////////////////////////////////////////////////////////////////////////
void EditorModule::ActionSlot()
{
	QObject* psender = sender();
	QAction *pact = qobject_cast<QAction *>( psender );
	
	if( pact )
	{
		QVariant data = pact->data();	
		QString text = pact->text();
		QString datatext = data.toString();
		//std::string std_text = text.toAscii().data();
		QByteArray qb = datatext.toUtf8();
		const char* pt = qb.data();
		OnAction( pt );
	}
}
///////////////////////////////////////////////////////////////////////////////
}} // namespace ork/tool
///////////////////////////////////////////////////////////////////////////
#include "qtmainwin.moc"
