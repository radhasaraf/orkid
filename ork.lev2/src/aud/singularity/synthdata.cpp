#include <audiofile.h>
#include <string>
#include <assert.h>
#include <unistd.h>
#include <math.h>

#include "krzdata.h"
#include "synth.h"
#include "krzobjects.h"

///////////////////////////////////////////////////////////////////////////////
std::string kbasepath = "/usr/local/share/singularity";
///////////////////////////////////////////////////////////////////////////////

using namespace ork::sf2;

float SynthData::seqTime(float dur)
{
	float rval = _seqCursor;
	_seqCursor += dur;
	return rval;

}

///////////////////////////////////////////////////////////////////////////////

layerData::layerData()
{
	_pchBlock = nullptr;
	
	for( int i=0; i<kmaxdspblocksperlayer; i++ )
		_dspBlocks[i] = nullptr; 
}

///////////////////////////////////////////////////////////////////////////////

Sf2TestSynthData::Sf2TestSynthData(const std::string& filename, synth* syn, const std::string& bankname)
	: SynthData(syn)
{
   _staticBankName = bankname;

   std::string sfbasedir = kbasepath+"/soundfonts/";
   _sfont = new SoundFont(sfbasedir+filename,bankname);



}
Sf2TestSynthData::~Sf2TestSynthData()
{
	delete _sfont;
}

const programData* Sf2TestSynthData::getProgram(int progID) const
{
	auto ObjDB = _sfont->_zpmDB;
	return ObjDB->findProgram(progID);
}

///////////////////////////////////////////////////////////////////////////////

static const std::string krzbasedir = kbasepath+"/kurzweil/krz/";

VastObjectsDB* KrzSynthData::baseObjects()
{
	static VastObjectsDB* objdb = nullptr;
	if(nullptr==objdb)
	{
		objdb = new VastObjectsDB;
		objdb->loadJson("k2v3base",0);
	}

	return objdb;

}

KrzSynthData::KrzSynthData(synth* syn)
	: SynthData(syn)
{

}
const programData* KrzSynthData::getProgram(int progID) const
{
	auto ObjDB = baseObjects();
	return ObjDB->findProgram(progID);
}

///////////////////////////////////////////////////////////////////////////////

KrzKmTestData::KrzKmTestData(synth* syn)
	: SynthData(syn)
{

}
const programData* KrzKmTestData::getProgram(int kmID) const
{
	programData* rval = nullptr;
	auto it = _testKmPrograms.find(kmID);
	if( it == _testKmPrograms.end() )
	{
		auto ObjDB = KrzSynthData::baseObjects();
		rval = new programData;
		rval->_role = "KmTest";
		auto km = ObjDB->findKeymap(kmID);
		if( km )
		{
			auto lyr = rval->newLayer();
			lyr->_keymap = km;
			//lyr->_useNatEnv = false;
			rval->_name = formatString("%s", km->_name.c_str());
		}
		else
			rval->_name = formatString("\?\?\?\?");

	}
	return rval;
}

///////////////////////////////////////////////////////////////////////////////

KrzTestData::KrzTestData(synth* syn)
	: SynthData(syn)
{
	genTestPrograms();	
}

const programData* KrzTestData::getProgram(int progid) const
{
	int inumtests = _testPrograms.size();
	int testid = progid%inumtests;
	printf( "test<%d>\n", testid );
	auto test = _testPrograms[testid];
	return test;
}


///////////////////////////////////////////////////////////////////////////////

SynthData::SynthData(synth* syn)
	: _syn(syn)
	, _seqCursor(0.0f)
{
	_synsr = syn->_sampleRate;

	//auto ObjDB = syn->_objectDB;
	//ObjDB->loadJson("krzfiles/krzdump.json");
	//_objects = ObjDB;

	// C4 = 72
	float t1, t2;

	///////////////////////////////////////
	// timbreshift scan
	///////////////////////////////////////

	#if 0
	if( false ) for( int i=1; i<128; i++ )
	{
		auto prg = getKmTestProgram(1);
		auto l0 = prg->getLayer(0);

		for( int n=0; n<72; n+=1 )
		{
			for( int tst=0; tst<18; tst++ )
			{
				t1 = seqTime(.15f);

				addEvent( t1 ,[=]()
				{
					//l0->_km_timbreshift = (tst%9)*4;
					//l0->_km_transposeTS = (tst/9);
					auto pi = _syn->keyOn(12+n,prg);

					addEvent( t1+1.5 ,[=]()
					{
						_syn->keyOff(pi);
					} );
				});
			}
		}
	}

	///////////////////////////////////////
	// keymap scan
	///////////////////////////////////////

	if( false ) for( int i=1; i<128; i++ )
	{
		auto prg = getKmTestProgram(i);

		for( int n=0; n<48; n++ )
		{
			t1 = seqTime(0.15f);

			addEvent( t1 ,[=]()
			{
				auto pi = _syn->keyOn(24+n,prg);

				addEvent( t1+2 ,[=]()
				{
					_syn->keyOff(pi);
				} );
			});
		}
	}

	///////////////////////////////////////
	// program scan
	///////////////////////////////////////

	if( false ) for( int i=1; i<128; i++ )
	{
		for( int n=0; n<36; n+=1 )
		{
			t1 = seqTime(0.5f);
			//t2 = seqTime(0.1f);

			addEvent( t1 ,[=]()
			{
				auto prg = getProgram(i);
				auto pi = _syn->keyOn(48+n,prg);

				addEvent( t1+0.5 ,[=]()
				{
					_syn->keyOff(pi);
				} );
			});
		}
	}
	#endif

	///////////////////////////////////////

	//_lpf.setup(330.0f,_synsr);
}

///////////////////////////////////////////////////////////////////////////////

void KrzTestData::genTestPrograms()
{
	/*
	auto t1 = new programData;
	t1->_role = "PrgTest";
	_testPrograms.push_back(t1);
	t1->_name = "YO";

	auto l1 = t1->newLayer();
	const int keymap_sine = 163;
	const int keymap_saw = 151;

	l1->_keymap = KrzSynthData::baseObjects()->findKeymap(keymap_sine);

	auto& EC = l1->_envCtrlData;
	EC._useNatEnv = false;

    auto CB0 = new controlBlockData;
    auto CB1 = new controlBlockData;
    l1->_ctrlBlocks[0] = CB0;
    l1->_ctrlBlocks[1] = CB1;

	auto ampenv = new RateLevelEnvData();
	ampenv->_name = "AMPENV";
	CB0->_cdata[0] = ampenv;
	auto& aesegs = ampenv->_segments;
	aesegs.push_back(EnvPoint{0,1});
	aesegs.push_back(EnvPoint{0,0});
	aesegs.push_back(EnvPoint{0,0});
	aesegs.push_back(EnvPoint{0,1});
	aesegs.push_back(EnvPoint{0,0});
	aesegs.push_back(EnvPoint{0,0});
	aesegs.push_back(EnvPoint{1,0});

	auto env2 = new RateLevelEnvData();
	env2->_name = "ENV2";
	CB0->_cdata[1] = env2;
	auto& e2segs = env2->_segments;
	e2segs.push_back(EnvPoint{2,1});
	e2segs.push_back(EnvPoint{0,1});
	e2segs.push_back(EnvPoint{0,1});
	e2segs.push_back(EnvPoint{0,0.5});
	e2segs.push_back(EnvPoint{0,0});
	e2segs.push_back(EnvPoint{0,0});
	e2segs.push_back(EnvPoint{1,0});

	auto env3 = new RateLevelEnvData();
	env3->_name = "ENV3";
	CB0->_cdata[2] = env3;
	auto& e3segs = env3->_segments;
	e3segs.push_back(EnvPoint{8,1});
	e3segs.push_back(EnvPoint{0,1});
	e3segs.push_back(EnvPoint{0,1});
	e3segs.push_back(EnvPoint{0,0.5});
	e3segs.push_back(EnvPoint{0,0});
	e3segs.push_back(EnvPoint{0,0});
	e3segs.push_back(EnvPoint{1,0});

	auto lfo1 = new LfoData;
	CB1->_cdata[2] = (const LfoData*) lfo1;
	lfo1->_name = "LFO1";
	lfo1->_controller = "ON";
	lfo1->_maxRate = 0.1;

	auto& ALGD = l1->_algData;

	ALGD._name = "ALG1";
	ALGD._algID = 1;

	if( 0 )
	{
		auto& F1 = l1->_fBlock[1];
		F1._dspBlock = "2PARAM SHAPER";
		F1._paramScheme = "EVN";
		F1._blockIndex = 0;
		F1._units = "dB";
		F1._coarse = -60.0;
	}
	else if( 0 )
	{
		auto& F2 = l1->_fBlock[2];
		F2._blockIndex = 0;
		F2._dspBlock = "SHAPER";
		F2._paramScheme = "AMT";
		F2._units = "x";
		F2._coarse = 0.1;
	}
	if( 0 )
	{
		auto& F3 = l1->_fBlock[3];
		F3._paramScheme = "ODD";
		F3._blockIndex = 1;
		F3._units = "dB";
		F3._coarse = -96.0;
	}*/
}



