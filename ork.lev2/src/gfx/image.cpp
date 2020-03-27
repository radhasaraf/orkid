////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2020, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#include <ork/pch.h>
#include <ork/file/file.h>
#include <ork/kernel/spawner.h>
#include <ork/kernel/string/deco.inl>
#include <ork/lev2/gfx/image.h>

#include <ork/lev2/gfx/gfxenv.h>
#include <ork/gfx/dds.h>
#include <ork/lev2/gfx/texman.h>
#include <math.h>
#include <ispc_texcomp.h>
#include <OpenImageIO/imageio.h>
#include <OpenImageIO/filesystem.h>
OIIO_NAMESPACE_USING

namespace ork::lev2 {
///////////////////////////////////////////////////////////////////////////////

void Image::init(size_t w, size_t h, size_t numc) {
  _numcomponents = numc;
  _width         = w;
  _height        = h;
  _data          = std::make_shared<DataBlock>();
  _data->allocateBlock(_width * _height * _numcomponents);
}

static fvec3 _image_deco(0.1, 0.2, 0.3);

///////////////////////////////////////////////////////////////////////////////

void Image::initFromInMemoryFile(std::string fmtguess, const void* srcdata, size_t srclen) {
  ImageSpec config;                                          // ImageSpec describing input configuration options
  Filesystem::IOMemReader memreader((void*)srcdata, srclen); // I/O proxy object
  void* ptr = &memreader;
  config.attribute("oiio:ioproxy", TypeDesc::PTR, &ptr);

  auto name = std::string("inmem.") + fmtguess;

  auto in               = ImageInput::open(name, &config);
  const ImageSpec& spec = in->spec();
  _width                = spec.width;
  _height               = spec.height;
  _numcomponents        = spec.nchannels;
  _data                 = std::make_shared<DataBlock>();
  _data->allocateBlock(_width * _height * _numcomponents);
  auto pixels = (uint8_t*)_data->data();
  in->read_image(TypeDesc::UINT8, &pixels[0]);
  in->close();

  deco::printf(_image_deco, "///////////////////////////////////\n");
  deco::printf(_image_deco, "// Image::initFromInMemoryFile()\n");
  deco::printf(_image_deco, "// _width<%zu>\n", _width);
  deco::printf(_image_deco, "// _height<%zu>\n", _height);
  deco::printf(_image_deco, "// _numcomponents<%zu>\n", _numcomponents);
  deco::printf(_image_deco, "///////////////////////////////////\n");
}

///////////////////////////////////////////////////////////////////////////////

void Image::downsample(Image& imgout) const {
  OrkAssert((_width & 1) == 0);
  OrkAssert((_height & 1) == 0);
  imgout.init(_width >> 1, _height >> 1, _numcomponents);
  // todo parallelize (CPU(ISPC) or GPU(CUDA))
  auto inp_pixels = (const uint8_t*)_data->data();
  auto out_pixels = (uint8_t*)imgout._data->data();
  for (size_t y = 0; y < imgout._height; y++) {
    size_t ya = y * 2;
    size_t yb = ya + 1;
    for (size_t x = 0; x < imgout._width; x++) {
      size_t xa             = x * 2;
      size_t xb             = xa + 1;
      size_t out_index      = (y * imgout._width + x) * _numcomponents;
      size_t inp_index_xaya = (ya * _width + xa) * _numcomponents;
      size_t inp_index_xbya = (ya * _width + xb) * _numcomponents;
      size_t inp_index_xayb = (yb * _width + xa) * _numcomponents;
      size_t inp_index_xbyb = (yb * _width + xb) * _numcomponents;
      for (size_t c = 0; c < _numcomponents; c++) {
        double xaya               = double(inp_pixels[inp_index_xaya + c]);
        double xbya               = double(inp_pixels[inp_index_xbya + c]);
        double xayb               = double(inp_pixels[inp_index_xayb + c]);
        double xbyb               = double(inp_pixels[inp_index_xbyb + c]);
        double avg                = (xaya + xbya + xayb + xbyb) * 0.25;
        uint8_t uavg              = uint8_t(avg * 255.0f);
        out_pixels[out_index + c] = uavg;
      }
    }
  }
  deco::printf(_image_deco, "///////////////////////////////////\n");
  deco::printf(_image_deco, "// Image::downsample()\n");
  deco::printf(_image_deco, "// imgout._width<%zu>\n", imgout._width);
  deco::printf(_image_deco, "// imgout._height<%zu>\n", imgout._height);
  deco::printf(_image_deco, "// imgout._numcomponents<%zu>\n", imgout._numcomponents);
  deco::printf(_image_deco, "///////////////////////////////////\n");
}

///////////////////////////////////////////////////////////////////////////////

void Image::compressBC7(CompressedImage& imgout) const {
  imgout._format = EBUFFMT_RGBA_BPTC_UNORM;
  OrkAssert((_width & 3) == 0);
  OrkAssert((_height & 3) == 0);
  OrkAssert((_numcomponents == 3) or (_numcomponents == 4));
  imgout._width         = _width;
  imgout._height        = _height;
  imgout._numcomponents = 4;
  imgout._data          = std::make_shared<DataBlock>();
  auto dest             = (uint8_t*)imgout._data->allocateBlock(_width * _height);
  bc7_enc_settings settings;
  switch (_numcomponents) {
    case 3:
      GetProfile_fast(&settings);
      break;
    case 4:
      GetProfile_alpha_fast(&settings);
      break;
    default:
      OrkAssert(false);
      break;
  }
  rgba_surface surface;
  surface.width  = _width;
  surface.height = _height;
  surface.stride = _width * _numcomponents;
  surface.ptr    = (uint8_t*)_data->data();
  ork::Timer timer;
  timer.Start();
  deco::printf(_image_deco, "///////////////////////////////////\n");
  deco::printf(_image_deco, "// Image::compressBC7()\n");
  deco::printf(_image_deco, "// imgout._width<%zu>\n", imgout._width);
  deco::printf(_image_deco, "// imgout._height<%zu>\n", imgout._height);
  deco::printf(_image_deco, "// imgout._numcomponents<%zu>\n", imgout._numcomponents);
  CompressBlocksBC7(&surface, dest, &settings);
  float time = timer.SecsSinceStart();
  float MPPS = float(_width * _height) * 1e-6 / time;
  deco::printf(_image_deco, "// compression time<%g> MPPS<%g>\n", time, MPPS);
  deco::printf(_image_deco, "///////////////////////////////////\n");
}
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
} // namespace ork::lev2
