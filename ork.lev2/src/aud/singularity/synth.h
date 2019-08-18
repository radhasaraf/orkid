#pragma once

#include <ork/math/audiomath.h>
#include "krztypes.h"
#include "krzdata.h"
#include "layer.h"
#include <ork/kernel/concurrent_queue.h>
#include <ork/kernel/svariant.h>

///////////////////////////////////////////////////////////////////////////////

struct programInst
{
	programInst(synth& syn);
	~programInst();

	void keyOn( int note, const programData* pd );
	void keyOff();

	//void compute();

	const programData* _progdata;
	synth& _syn;

	std::vector<layer*> _layers;
};

///////////////////////////////////////////////////////////////////////////////

struct SynthData;

struct hudsample
{
	float _time;
	float _value;
};

struct synth
{
	synth(float sr);
	~synth();

	typedef std::vector<hudsample> hudsamples_t;

	void compute(int inumframes,const void* inputbuffer);

	programInst* keyOn(int note, const programData* pd);
	void keyOff(programInst* p);

	layer* allocLayer();
	void freeLayer(layer* l);
	void deactivateVoices();

	void onDrawHud(float w, float h);
	void onDrawHudPage1(float w, float h);
	void onDrawHudPage2(float w, float h);
	void onDrawHudPage3(float w, float h);

	void resetFenables();

    void addEvent(float time, void_lamda_t ev);
    void tick(float dt);
    float _timeaccum;

	outputBuffer _ibuf;
	outputBuffer _obuf;
	float _sampleRate;
	float _dt;

	std::set<layer*> _freeVoices;
	std::set<layer*> _activeVoices;
	std::queue<layer*> _deactiveateVoiceQ;
	std::set<programInst*> _freeProgInst;
	std::set<programInst*> _activeProgInst;
	std::map<std::string,hudsamples_t> _hudsample_map;

    std::multimap<float,void_lamda_t> _eventmap;

	int _soloLayer = -1;
	bool _fblockEnable[5] = { true, true, true, true, true };
	int _lnoteframe;
	float _lnotetime;
	float _testtonepi;
	float _testtoneph;
	float _testtoneamp;
	float _testtoneampps;
	int _hudpage;
	int _genmode = 0;
	float _ostrack = 0.0f;
	float _ostrackPH = 0.0f;
	bool _bypassDSP = false;
	bool _doModWheel = false;
	bool _doPressure = false;
	bool _doInput = false;
	float _masterGain = 1.0f/2.0f;

	layer* _hudLayer = nullptr;
	bool _clearhuddata = true;

	ork::MpMcBoundedQueue<ork::svar1024_t> _hudbuf;

	hudkframe _curhud_kframe;
	hudaframe _curhud_aframe;

    float _fftbuffer[koscopelength/2];
    float _oscopebuffer[koscopelength];
};

///////////////////////////////////////////////////////////////////////////////

//std::string formatString( const char* formatstring, ... );
//void SplitString(const std::string& s, char delim, std::vector<std::string>& tokens);
//std::vector<std::string> SplitString(const std::string& instr, char delim);

///////////////////////////////////////////////////////////////////////////////

//void drawtext( const std::string& str, float x, float y, float scale, float r, float g, float b );
