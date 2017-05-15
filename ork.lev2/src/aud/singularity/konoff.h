#pragma once

#include <ork/kernel/svariant.h>

struct DspBlock;
struct layerData;
struct layer;

///////////////////////////////////////////////////////////////////////////////

struct KeyOnInfo
{
    int _key = 0;
    int _vel = 0;
    layer* _layer = nullptr;
    const layerData* _layerData = nullptr;
    ork::svarp_t _extdata;
};

///////////////////////////////////////////////////////////////////////////////

struct DspKeyOnInfo : public KeyOnInfo
{
    DspBlock* _prv = nullptr;
};
