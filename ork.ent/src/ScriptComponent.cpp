////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2012, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#include <ork/pch.h>
#include <ork/reflect/RegisterProperty.h>
#include <pkg/ent/entity.h>
#include <pkg/ent/entity.hpp>
#include <pkg/ent/scene.h>
#include <pkg/ent/scene.hpp>
#include <pkg/ent/ScriptComponent.h>
#include <ork/kernel/any.h>

#include <sstream>
#include <iostream>
#include <cxxabi.h>

#include "LuaBindings.h"

///////////////////////////////////////////////////////////////////////////////

using namespace luabind;

static const bool kUSEEXECTABUPDATE = true;

///////////////////////////////////////////////////////////////////////////////

INSTANTIATE_TRANSPARENT_RTTI( ork::ent::ScriptComponentData, "ScriptComponentData" );
INSTANTIATE_TRANSPARENT_RTTI( ork::ent::ScriptComponentInst, "ScriptComponentInst" );
INSTANTIATE_TRANSPARENT_RTTI( ork::ent::ScriptManagerComponentData, "ScriptManagerComponentData" );
INSTANTIATE_TRANSPARENT_RTTI( ork::ent::ScriptManagerComponentInst, "ScriptManagerComponentInst" );


///////////////////////////////////////////////////////////////////////////////
namespace ork { namespace ent {
///////////////////////////////////////////////////////////////////////////////


void ScriptComponentData::Describe()
{
	ork::ent::RegisterFamily<ScriptComponentData>(ork::AddPooledLiteral("control"));

	ork::reflect::RegisterProperty( "ScriptFile", & ScriptComponentData::mScriptPath );
	ork::reflect::AnnotatePropertyForEditor<ScriptComponentData>("ScriptFile", "editor.class", "ged.factory.filelist");
	ork::reflect::AnnotatePropertyForEditor<ScriptComponentData>("ScriptFile", "editor.filetype", "lua");
	ork::reflect::AnnotatePropertyForEditor<ScriptComponentData>("ScriptFile", "editor.filebase", "src://scripts/");
}
ScriptComponentData::ScriptComponentData()
{
	printf( "ScriptComponentData::ScriptComponentData() this: %p\n", this );
}

ent::ComponentInst* ScriptComponentData::CreateComponent(ent::Entity* pent) const
{
	return new ScriptComponentInst( *this, pent );
}
void ScriptComponentData::DoRegisterWithScene( ork::ent::SceneComposer& sc )
{
	sc.Register<ork::ent::ScriptManagerComponentData>();
}

///////////////////////////////////////////////////////////////////////////////

void ScriptComponentInst::Describe()
{
}

ScriptComponentInst::ScriptComponentInst( const ScriptComponentData& data, ent::Entity* pent )
	: ork::ent::ComponentInst( & data, pent )
	, mCD( data )
	, mScriptRef(LUA_NOREF)
{
	auto path = mCD.GetPath();
	auto abspath = path.ToAbsolute();

	if( abspath.DoesPathExist() )
	{
		mPrefix = path.GetName().c_str();

		CFile scriptfile(abspath,EFM_READ);
		size_t filesize = 0;
		scriptfile.GetLength(filesize);
		char* scripttext = (char*) malloc(filesize+1);
		scriptfile.Read(scripttext,filesize);
		scripttext[filesize] = 0;
		mScriptText = scripttext;
		//printf( "%s\n", scripttext);
		free(scripttext);
	}

}

bool ScriptComponentInst::DoLink(ork::ent::SceneInst *psi)
{

	auto scm = psi->FindTypedSceneComponent<ScriptManagerComponentInst>();

	if( scm && (mScriptText.length()>0) )
	{
		auto ent = this->GetEntity();
		auto name = ent->GetEntData().GetName().c_str();

		auto asluasys = scm->GetLuaManager().Get<LuaSystem*>();
		OrkAssert(asluasys);
		auto luast = asluasys->mLuaState;

		int ret = luaL_loadstring(luast,mScriptText.c_str());
		mScriptRef = luaL_ref(luast, LUA_REGISTRYINDEX);

		assert(mScriptRef!=LUA_NOREF);

		LuaProtectedCallByRef( luast, mScriptRef );

	    auto wrap_table =luabind::newtable(luast);
	    wrap_table["x"]=0.0;
	    wrap_table["y"]=6.0;
		luabind::globals(luast)["arch"] = wrap_table;
		//wrap_table.push(luast);

	    //lua_getfenv(luast,-1);
		//assert(lua_setfenv(luast, -1) != 0);

		printf( "LINKING SCRIPTCOMPONENT<%p> of ent<%s> into Lua exec list\n", this, name );

		luabind::object o( luast, ent );
		LuaProtectedCallByName(luast,mScriptRef,"OnEntityLink",o);

		printf( "done LINKING SCRIPTCOMPONENT<%p> of ent<%s> into Lua exec list\n", this, name );
	}
	return true;
}
void ScriptComponentInst::DoUnLink(ork::ent::SceneInst *psi)
{
	auto scm = psi->FindTypedSceneComponent<ScriptManagerComponentInst>();

	if( scm )
	{
		auto asluasys = scm->GetLuaManager().Get<LuaSystem*>();
		OrkAssert(asluasys);
	}
}

bool ScriptComponentInst::DoStart(SceneInst *psi, const CMatrix4 &world)
{
	auto scm = psi->FindTypedSceneComponent<ScriptManagerComponentInst>();

	if( scm )
	{
		auto asluasys = scm->GetLuaManager().Get<LuaSystem*>();
		OrkAssert(asluasys);
		auto L = asluasys->mLuaState;

		auto ent = this->GetEntity();
		auto name = ent->GetEntData().GetName().c_str();

		//printf( "Starting SCRIPTCOMPONENT<%p> of ent<%p:%s> into Lua exec list\n", this, ent, name );

		//DoString( L, "printf(\"oeu: %s\",inspect(OnEntityUpdate))");
		
		if( kUSEEXECTABUPDATE )
		{
			luabind::object osu_fn = luabind::globals(L)["OnEntityUpdate"];
			assert(osu_fn.is_valid());
			assert(luabind::type(osu_fn)!=LUA_TNIL); // make sure fn was found!
			luabind::object osu_ent(L,ent);
			auto wrap_table =luabind::newtable(L);
		    wrap_table["fn"]=osu_fn;
		    wrap_table["ent"]=osu_ent;

			//luabind::call_function<void>(osu_fn,osu_arg);

			//assert(false);

			//fxstring<256> autoreg;
			//autoreg.format("entity_exec_table[e:name()]= function() OnScriptUpdate(e) end")
			//luaL_dostring(lua, "return 'derp'");
			luabind::object exectab = luabind::globals(L)["entity_exec_table"];// = exec_table;
			exectab[name]=wrap_table;

			LuaProtectedCallByName(L,mScriptRef,"OnEntityStart",osu_ent);

		}
		else
		{
			SCILuaData luadat;

			//luabind::object mOSUfn, mOSUarg;

			luadat.mFN = luabind::globals(L)["OnEntityUpdate"];
			assert(luadat.mFN.is_valid());
			assert(luabind::type(luadat.mFN)!=LUA_TNIL); // make sure fn was found!
			luadat.mARG = luabind::object(L,ent);

			mLuaData.Set<SCILuaData>(luadat);
		}


	}
	return true;
}
void ScriptComponentInst::DoStop(SceneInst *psi)
{
	auto scm = psi->FindTypedSceneComponent<ScriptManagerComponentInst>();

	if( scm )
	{
		auto asluasys = scm->GetLuaManager().Get<LuaSystem*>();
		OrkAssert(asluasys);
		auto L = asluasys->mLuaState;

		auto ent = this->GetEntity();
		auto name = ent->GetEntData().GetName().c_str();

		//printf( "LINKING SCRIPTCOMPONENT<%p> of ent<%s> into Lua exec list\n", this, name );

		luabind::object exectab = luabind::globals(L)["entity_exec_table"];// = exec_table;
		exectab[name]=luabind::nil;

		LuaProtectedCallByNameT<Entity*>(asluasys->mLuaState,mScriptRef,"OnEntityStop",ent);

	}
}


void ScriptComponentInst::DoUpdate(ork::ent::SceneInst* psi)
{

	if( false == kUSEEXECTABUPDATE )
	{
		auto scm = psi->FindTypedSceneComponent<ScriptManagerComponentInst>();

		if( scm )
		{
			auto asluasys = scm->GetLuaManager().Get<LuaSystem*>();
			OrkAssert(asluasys);
			auto L = asluasys->mLuaState;
			auto ent = this->GetEntity();
		
				//DoString( L, "printf(\"oeu: %s\",inspect(OnEntityUpdate))");
			//luabind::object osu_fn = luabind::globals(L)["OnEntityUpdate"];
			//assert(osu_fn.is_valid());
			//assert(luabind::type(osu_fn)!=LUA_TNIL); // make sure fn was found!
			//luabind::object osu_arg(L,ent);

			auto& as_luadata = mLuaData.Get<SCILuaData>();
			auto& fn = as_luadata.mFN;
			auto& arg = as_luadata.mARG;
			luabind::call_function<void>(fn,arg);


		}
	}
	//printf( "NOP\n");
	// NOP (scriptmanager will execute)
}

///////////////////////////////////////////////////////////////////////////////

void ScriptManagerComponentData::Describe()
{
}
ScriptManagerComponentData::ScriptManagerComponentData()
{

}

SceneComponentInst* ScriptManagerComponentData::CreateComponentInst(ork::ent::SceneInst *pinst) const
{
	return new ScriptManagerComponentInst( *this, pinst );
}

///////////////////////////////////////////////////////////////////////////////

void ScriptManagerComponentInst::Describe()
{
}

ScriptManagerComponentInst::ScriptManagerComponentInst( const ScriptManagerComponentData& data, ork::ent::SceneInst *pinst )
	: ork::ent::SceneComponentInst( &data, pinst )
	, mScriptRef(LUA_NOREF)
{
	auto luasys = new LuaSystem(pinst);
	mLuaManager.Set<LuaSystem*>(luasys);

	///////////////////////////////////////////////

	auto AppendPath = [&]( const char* pth )
	{
	    lua_getglobal( luasys->mLuaState, "package" );
	    lua_getfield( luasys->mLuaState, -1, "path" );

	    fxstring<256> lua_path;
	    lua_path.format(	"%s;%s", 
	    					lua_tostring( luasys->mLuaState, -1 ),
	    					pth );

	    lua_pop( luasys->mLuaState, 1 );
	    lua_pushstring( luasys->mLuaState, lua_path.c_str() );
	    lua_setfield( luasys->mLuaState, -2, "path" );
	    lua_pop( luasys->mLuaState, 1 );
	};

	///////////////////////////////////////////////
	// Set Lua Search Path
	///////////////////////////////////////////////

	auto searchpath = file::Path("src://scripts/");
	auto abssrchpath = searchpath.ToAbsolute();

	if( abssrchpath.DoesPathExist() )
	{
	    fxstring<256> lua_path;
	    lua_path.format( "%s?.lua", abssrchpath.c_str() );
	    AppendPath( lua_path.c_str() );
	}


	///////////////////////////////////////////////
	// find & init scene file
	///////////////////////////////////////////////

	const auto& scenedata = pinst->GetData();
	auto path = scenedata.GetScriptPath();
	auto abspath = path.ToAbsolute();

	if( abspath.DoesPathExist() )
	{
		CFile scriptfile(abspath,EFM_READ);
		size_t filesize = 0;
		scriptfile.GetLength(filesize);
		char* scripttext = (char*) malloc(filesize+1);
		scriptfile.Read(scripttext,filesize);
		scripttext[filesize] = 0;
		mScriptText = scripttext;
		//printf( "%s\n", scripttext);
		free(scripttext);


		auto asluasys = mLuaManager.Get<LuaSystem*>();
		OrkAssert(asluasys);

		int ret = luaL_loadstring(asluasys->mLuaState,mScriptText.c_str());

		mScriptRef = luaL_ref(asluasys->mLuaState, LUA_REGISTRYINDEX);
		//printf( "mScriptRef<%d>\n", mScriptRef );
		//lua_pop(asluasys->mLuaState, 1); // dont call, just reference


		LuaProtectedCallByRef( asluasys->mLuaState, mScriptRef );

		LuaProtectedCallByName( asluasys->mLuaState, mScriptRef, "OnSceneCompose");

	}

}

ScriptManagerComponentInst::~ScriptManagerComponentInst()
{
	auto asluasys = mLuaManager.Get<LuaSystem*>();
	OrkAssert(asluasys);
	delete asluasys;
}

bool ScriptManagerComponentInst::DoLink(SceneInst* psi) // final
{
	//printf( "ScriptManagerComponentInst::DoLink()\n" );
	auto asluasys = mLuaManager.Get<LuaSystem*>();
	OrkAssert(asluasys);
	LuaProtectedCallByName( asluasys->mLuaState, mScriptRef, "OnSceneLink");

	return true;
}
void ScriptManagerComponentInst::DoUnLink(SceneInst* psi) // final
{
	printf( "ScriptManagerComponentInst::DoUnLink()\n" );
	auto asluasys = mLuaManager.Get<LuaSystem*>();
	OrkAssert(asluasys);
	LuaProtectedCallByName( asluasys->mLuaState, mScriptRef, "OnSceneUnLink");
}
void ScriptManagerComponentInst::DoStart(SceneInst *psi) // final
{
	//printf( "ScriptManagerComponentInst::DoStart()\n" );
	auto asluasys = mLuaManager.Get<LuaSystem*>();
	OrkAssert(asluasys);
	LuaProtectedCallByName( asluasys->mLuaState, mScriptRef, "OnSceneStart");
}
void ScriptManagerComponentInst::DoStop(SceneInst *inst) // final
{
	//printf( "ScriptManagerComponentInst::DoStop()\n" );
	auto asluasys = mLuaManager.Get<LuaSystem*>();
	OrkAssert(asluasys);
	LuaProtectedCallByName( asluasys->mLuaState, mScriptRef, "OnSceneStop");
}

void ScriptManagerComponentInst::DoUpdate(SceneInst* psi) // final
{
	auto asluasys = mLuaManager.Get<LuaSystem*>();
	OrkAssert(asluasys);
	LuaProtectedCallByName( asluasys->mLuaState, mScriptRef, "OnSceneUpdate");

	double dt = psi->GetDeltaTime();
	double gt = psi->GetGameTime();

	if( kUSEEXECTABUPDATE )
	{
		luabind::object o(asluasys->mLuaState, dt );
		LuaProtectedCallByName( asluasys->mLuaState, mScriptRef, "UpdateSceneEntities",o);
	}
}

///////////////////////////////////////////////////////////////////////////////

} }  // ork::ent3d

