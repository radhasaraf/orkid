////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2022, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#include "pyext.h"
#include <ork/lev2/input/inputdevice.h>
#include <ork/lev2/gfx/terrain/terrain_drawable.h>
#include <ork/lev2/gfx/camera/cameradata.h>
#include <ork/lev2/gfx/scenegraph/sgnode_grid.h>

///////////////////////////////////////////////////////////////////////////////

namespace ork::lev2 {

void pyinit_gfx_renderer(py::module& module_lev2) {
  auto type_codec = python::TypeCodec::instance();

  /////////////////////////////////////////////////////////////////////////////////
  auto rcfd_type_t = py::class_<RenderContextFrameData, rcfd_ptr_t>(
                         module_lev2,                                 //
                         "RenderContextFrameData")                    //
                         .def(py::init([](ctx_t& ctx) -> rcfd_ptr_t { //
                           return std::make_shared<RenderContextFrameData>(ctx.get());
                         }))
                         .def_property(
                             "cimpl",
                             [](rcfd_ptr_t the_rcfd) -> compositorimpl_ptr_t { return the_rcfd->_cimpl; },
                             [](rcfd_ptr_t the_rcfd, compositorimpl_ptr_t c) { the_rcfd->_cimpl = c; })
                         .def("setRenderingModel", [](rcfd_ptr_t the_rcfd, std::string rendermodel) { //
                           auto as_crc               = CrcString(rendermodel.c_str());
                           the_rcfd->_renderingmodel = (uint32_t)as_crc._hashed;
                         });
  type_codec->registerStdCodec<rcfd_ptr_t>(rcfd_type_t);
  /////////////////////////////////////////////////////////////////////////////////
  auto rcid_type_t = py::class_<RenderContextInstData, rcid_ptr_t>(
                         module_lev2,                                          //
                         "RenderContextInstData")                              //
                         .def(py::init([](rcfd_ptr_t the_rcfd) -> rcid_ptr_t { //
                           return RenderContextInstData::create(the_rcfd);
                         }))
                         .def(
                             "pipeline",                                                         //
                             [](rcid_ptr_t the_rcid, material_ptr_t mtl) -> fxpipeline_ptr_t { //
                               auto cache = mtl->pipelineCache();
                               return cache->findPipeline(*the_rcid);
                             })
                         .def(
                             "genMatrix",                                 //
                             [](rcid_ptr_t the_rcid, py::object method) { //
                               auto py_callback     = method.cast<pybind11::object>();
                               the_rcid->_genMatrix = [py_callback]() -> fmtx4 {
                                 py::gil_scoped_acquire acquire;
                                 py::object mtx_attempt = py_callback();
                                 printf("YAY..\n");
                                 return mtx_attempt.cast<fmtx4>();
                               };
                             })
                         .def(
                             "forceTechnique",                                  //
                             [](rcid_ptr_t the_rcid, pyfxtechnique_ptr_t tek) { //
                               the_rcid->forceTechnique(tek.get());
                             });
  /*.def_property("fxcache",
      [](rcid_ptr_t the_rcid) -> fxpipelinecache_constptr_t { //
        return the_rcid->_pipeline_cache;
      },
      [](rcid_ptr_t the_rcid, fxpipelinecache_constptr_t cache) { //
        the_rcid->_pipeline_cache = cache;
      }
  )*/
  type_codec->registerStdCodec<rcid_ptr_t>(rcid_type_t);

  /////////////////////////////////////////////////////////////////////////////////
  py::class_<LightData, lightdata_ptr_t>(module_lev2, "LightData")
      .def_property(
          "color",                                       //
          [](lightdata_ptr_t lightdata) -> fvec3 { //
            return lightdata->mColor;
          },
          [](lightdata_ptr_t lightdata, fvec3 color) { //
            lightdata->mColor = color;
          });
  py::class_<PointLightData, LightData, pointlightdata_ptr_t>(module_lev2, "PointLightData")
      .def(py::init<>())
      .def(
          "createNode",                      //
          [](pointlightdata_ptr_t lightdata, //
             std::string named,
             scenegraph::layer_ptr_t layer) -> scenegraph::lightnode_ptr_t { //
            auto xfgen = []() -> fmtx4 { return fmtx4(); };
            auto light = std::make_shared<PointLight>(xfgen, lightdata.get());
            return layer->createLightNode(named, light);
          });
  py::class_<SpotLightData, LightData, spotlightdata_ptr_t>(module_lev2, "SpotLightData");
  /////////////////////////////////////////////////////////////////////////////////
  py::class_<Light, light_ptr_t>(module_lev2, "Light")
      .def_property(
          "matrix",                              //
          [](light_ptr_t light) -> fmtx4_ptr_t { //
            auto copy = std::make_shared<fmtx4>(light->worldMatrix());
            return copy;
          },
          [](light_ptr_t light, fmtx4_ptr_t mtx) { //
            light->worldMatrix() = *mtx.get();
          });
  py::class_<PointLight, Light, pointlight_ptr_t>(module_lev2, "PointLight");
  py::class_<SpotLight, Light, spotlight_ptr_t>(module_lev2, "SpotLight");
  /////////////////////////////////////////////////////////////////////////////////
  auto camdattype = //
      py::class_<CameraData, cameradata_ptr_t>(module_lev2, "CameraData")
          .def(py::init([]() -> cameradata_ptr_t { return std::make_shared<CameraData>(); }))
          .def(
              "perspective",                                                    //
              [](cameradata_ptr_t camera, float near, float ffar, float fovy) { //
                camera->Persp(near, ffar, fovy);
              })
          .def(
              "lookAt",                                                        //
              [](cameradata_ptr_t camera, fvec3& eye, fvec3& tgt, fvec3& up) { //
                camera->Lookat(eye, tgt, up);
              })
          .def(
              "copyFrom",                                                        //
              [](cameradata_ptr_t camera, cameradata_ptr_t src_camera) { //
                *camera = *src_camera;
              })
          .def(
              "projectDepthRay",                                              //
              [](cameradata_ptr_t camera, fvec2_ptr_t pos2d) -> fray3_ptr_t { //
                auto cammat = camera->computeMatrices(1280.0 / 720.0);
                auto rval   = std::make_shared<fray3>();
                cammat.projectDepthRay(*pos2d.get(), *rval.get());
                return rval;
              });
  type_codec->registerStdCodec<cameradata_ptr_t>(camdattype);
  /////////////////////////////////////////////////////////////////////////////////
  auto camdatluttype = //
      py::class_<CameraDataLut, cameradatalut_ptr_t>(module_lev2, "CameraDataLut")
          .def(py::init<>())
          .def("addCamera", [](cameradatalut_ptr_t lut, std::string key, cameradata_constptr_t camera) { (*lut)[key] = camera; })
          .def("create", [](cameradatalut_ptr_t lut, std::string key) -> cameradata_ptr_t {
            auto camera = lut->create(key);
            return camera;
          });
  type_codec->registerStdCodec<cameradatalut_ptr_t>(camdatluttype);
  /////////////////////////////////////////////////////////////////////////////////
  auto cammatstype = //
      py::class_<CameraMatrices, cameramatrices_ptr_t>(module_lev2, "CameraMatrices")
          .def(py::init([]() -> cameramatrices_ptr_t { //
            return std::make_shared<CameraMatrices>();
          }))
          .def(
              "setCustomProjection",                           //
              [](cameramatrices_ptr_t cammats, fmtx4 matrix) { //
                cammats->setCustomProjection(matrix);
              })
          .def(
              "setCustomView",                                 //
              [](cameramatrices_ptr_t cammats, fmtx4 matrix) { //
                cammats->setCustomView(matrix);
              });
  type_codec->registerStdCodec<cameramatrices_ptr_t>(cammatstype);
}
} // namespace ork::lev2
