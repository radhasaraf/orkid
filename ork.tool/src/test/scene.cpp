#include <ork/pch.h>
#include <ork/kernel/opq.h>
#include <ork/application/application.h>
#include <ork/object/Object.h>
#include <ork/rtti/downcast.h>
#include <ork/reflect/RegisterProperty.h>
#include <ork/util/Context.h>
#include <ork/lev2/gfx/gfxenv.h>
#include <pkg/ent/entity.h>
#include <pkg/ent/scene.h>
#include <pkg/ent/editor/editor.h>
#include <unittest++/UnitTest++.h>

using namespace ork;
using namespace ork::ent;

TEST(SceneManip1)
{
	auto op = []()
	{
	    SceneData *scenedata = new SceneData;

	    EntData *entdata1 = new EntData;
	    entdata1->SetName("entity1");
	    scenedata->AddSceneObject(entdata1);

	    EntData *entdata2 = new EntData;
	    entdata2->SetName("entity2");
	    scenedata->AddSceneObject(entdata2);

	    SceneInst *sceneinst = new SceneInst(scenedata, ApplicationStack::Top());
	    sceneinst->SetSceneInstMode(ESCENEMODE_RUN);

	    Entity *entity = sceneinst->FindEntity(AddPooledLiteral("entity1"));

	    printf( "entity<%p>\n", entity );
	    entity = sceneinst->FindEntity(AddPooledLiteral("entity2"));
	    printf( "entity2<%p>\n", entity );

	    delete sceneinst;
	    delete scenedata;
	    printf( "DONE\n");
	};

	UpdateSerialOpQ().push(Op(op));
	UpdateSerialOpQ().drain();

	//WaitForOpqExit();
}

