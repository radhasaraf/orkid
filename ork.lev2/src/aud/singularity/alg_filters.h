#pragma once

#include "dspblocks.h"

///////////////////////////////////////////////////////////////////////////////
// filter blocks
///////////////////////////////////////////////////////////////////////////////

struct BANDPASS_FILT : public DspBlock
{
    BANDPASS_FILT( const DspBlockData& dbd );
    TrapSVF _filter;
    BiQuad _biquad;
    void compute(DspBuffer& dspbuf) final;
    void doKeyOn(const DspKeyOnInfo& koi) final;
};
struct BAND2 : public DspBlock
{
    BAND2( const DspBlockData& dbd );
    TrapSVF _filter;
    void compute(DspBuffer& dspbuf) final;
    void doKeyOn(const DspKeyOnInfo& koi) final;
};
struct NOTCH_FILT : public DspBlock
{
    NOTCH_FILT( const DspBlockData& dbd );
    TrapSVF _filter;
    void compute(DspBuffer& dspbuf) final;
    void doKeyOn(const DspKeyOnInfo& koi) final;
};
struct NOTCH2 : public DspBlock
{
    NOTCH2( const DspBlockData& dbd );
    TrapSVF _filter1;
    void compute(DspBuffer& dspbuf) final;
    void doKeyOn(const DspKeyOnInfo& koi) final;
};
struct DOUBLE_NOTCH_W_SEP : public DspBlock
{
    DOUBLE_NOTCH_W_SEP( const DspBlockData& dbd );
    TrapSVF _filter1;
    TrapSVF _filter2;
    void compute(DspBuffer& dspbuf) final;
    void doKeyOn(const DspKeyOnInfo& koi) final;
};
struct TWOPOLE_LOWPASS : public DspBlock
{
    TWOPOLE_LOWPASS( const DspBlockData& dbd );
    TrapSVF _filter;
    float _smoothFC;
    void compute(DspBuffer& dspbuf) final;
    void doKeyOn(const DspKeyOnInfo& koi) final;
};
struct LOPAS2 : public DspBlock
{   LOPAS2( const DspBlockData& dbd );
    TrapSVF _filter;
    void compute(DspBuffer& dspbuf) final;
    void doKeyOn(const DspKeyOnInfo& koi) final;
};
struct LP2RES : public DspBlock
{   LP2RES( const DspBlockData& dbd );
    TrapSVF _filter;
    void compute(DspBuffer& dspbuf) final;
    void doKeyOn(const DspKeyOnInfo& koi) final;
};
struct LPGATE : public DspBlock
{   LPGATE( const DspBlockData& dbd );
    TrapSVF _filter;
    void compute(DspBuffer& dspbuf) final;
    void doKeyOn(const DspKeyOnInfo& koi) final;
};
struct FOURPOLE_LOPASS_W_SEP : public DspBlock
{
    FOURPOLE_LOPASS_W_SEP( const DspBlockData& dbd );
    TrapSVF _filter1;
    TrapSVF _filter2;
    float _filtFC;
    void compute(DspBuffer& dspbuf) final;
    void doKeyOn(const DspKeyOnInfo& koi) final;
};
struct FOURPOLE_HIPASS_W_SEP : public DspBlock
{
    FOURPOLE_HIPASS_W_SEP( const DspBlockData& dbd );
    TrapSVF _filter1;
    TrapSVF _filter2;
    float _filtFC;
    void compute(DspBuffer& dspbuf) final;
    void doKeyOn(const DspKeyOnInfo& koi) final;
};
struct LOPASS : public DspBlock
{   LOPASS( const DspBlockData& dbd );
    OnePoleLoPass _lpf;
    void compute(DspBuffer& dspbuf) final;
    void doKeyOn(const DspKeyOnInfo& koi) final;
};
struct LPCLIP : public DspBlock
{   LPCLIP( const DspBlockData& dbd );
    OnePoleLoPass _lpf;
    void compute(DspBuffer& dspbuf) final;
    void doKeyOn(const DspKeyOnInfo& koi) final;
};
struct HIPASS : public DspBlock
{   HIPASS( const DspBlockData& dbd );
    OnePoleHiPass _hpf;
    void compute(DspBuffer& dspbuf) final;
    void doKeyOn(const DspKeyOnInfo& koi) final;
};
struct ALPASS : public DspBlock
{   
    ALPASS( const DspBlockData& dbd );
    TrapAllpass _filter;
    void compute(DspBuffer& dspbuf) final;
    void doKeyOn(const DspKeyOnInfo& koi) final;
};
struct TWOPOLE_ALLPASS : public DspBlock
{
    TWOPOLE_ALLPASS( const DspBlockData& dbd );
    TrapAllpass _filterL;
    TrapAllpass _filterH;
    void compute(DspBuffer& dspbuf) final;
    void doKeyOn(const DspKeyOnInfo& koi) final;
};
struct HIFREQ_STIMULATOR : public DspBlock
{
    HIFREQ_STIMULATOR( const DspBlockData& dbd );
    TrapSVF _filter1;
    TrapSVF _filter2;
    float _smoothFC;
    void compute(DspBuffer& dspbuf) final;
    void doKeyOn(const DspKeyOnInfo& koi) final;
};