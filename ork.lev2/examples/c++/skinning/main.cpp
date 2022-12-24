////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2022, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#include <iostream>

#include <ork/application/application.h>
#include <ork/kernel/environment.h>
#include <ork/kernel/string/deco.inl>
#include <ork/kernel/timer.h>
#include <ork/lev2/ezapp.h>
#include <ork/lev2/gfx/renderer/drawable.h>
#include <ork/lev2/lev2_asset.h>
#include <ork/lev2/gfx/gfxmodel.h>
#include <ork/lev2/gfx/lighting/gfx_lighting.h>
#include <ork/lev2/gfx/renderer/compositor.h>
#include <ork/lev2/gfx/renderer/NodeCompositor/NodeCompositorScreen.h>
#include <ork/lev2/gfx/material_freestyle.h>

///////////////////////////////////////////////////////////////////////////////
#include <ork/lev2/gfx/renderer/NodeCompositor/pbr_node_deferred.h>
#include <ork/lev2/gfx/renderer/NodeCompositor/pbr_node_forward.h>
#include <ork/lev2/gfx/renderer/NodeCompositor/unlit_node.h>
///////////////////////////////////////////////////////////////////////////////
#include <ork/lev2/imgui/imgui.h>
#include <ork/lev2/imgui/imgui_impl_glfw.h>
#include <ork/lev2/imgui/imgui_impl_opengl3.h>
#include <ork/lev2/imgui/imgui_ged.inl>
#include <ork/lev2/imgui/imgui_internal.h>
#include <ork/lev2/gfx/camera/uicam.h>
///////////////////////////////////////////////////////////////////////////////

using namespace std::string_literals;
using namespace ork;
using namespace ork::lev2;
using namespace ork::lev2::pbr::deferrednode;
typedef SVtxV12C4T16 vtx_t; // position, vertex color, 2 UV sets

///////////////////////////////////////////////////////////////////

struct GpuResources {

  GpuResources(
      appinitdata_ptr_t init_data, //
      Context* ctx,                //
      bool use_forward ) {         //

    auto vars = *init_data->parse();

    //////////////////////////////////////////////////////////
    int testnum = vars["testnum"].as<int>();
    //////////////////////////////////////////////////////////

    _camlut                = std::make_shared<CameraDataLut>();
    _camdata               = std::make_shared<CameraData>();
    (*_camlut)["spawncam"] = _camdata;

    _char_drawable = std::make_shared<ModelDrawable>();

    //////////////////////////////////////////////
    // create scenegraph
    //////////////////////////////////////////////

    _sg_params                                         = std::make_shared<varmap::VarMap>();
    _sg_params->makeValueForKey<std::string>("preset") = use_forward ? "ForwardPBR" : "DeferredPBR";

    _sg_scene        = std::make_shared<scenegraph::Scene>(_sg_params);
    auto sg_layer    = _sg_scene->createLayer("default");
    auto sg_compdata = _sg_scene->_compositorData;
    auto nodetek     = sg_compdata->tryNodeTechnique<NodeCompositingTechnique>("scene1"_pool, "item1"_pool);
    auto rendnode    = nodetek->tryRenderNodeAs<ork::lev2::pbr::deferrednode::DeferredCompositingNodePbr>();
    auto pbrcommon   = rendnode->_pbrcommon;

    pbrcommon->_depthFogDistance = 4000.0f;
    pbrcommon->_depthFogPower    = 5.0f;
    pbrcommon->_skyboxLevel      = 0.25;
    pbrcommon->_diffuseLevel     = 0.2;
    pbrcommon->_specularLevel    = 3.2;

    //////////////////////////////////////////////////////////

    ctx->debugPushGroup("main.onGpuInit");

    auto model_load_req = std::make_shared<asset::LoadRequest>();
    auto anim_load_req  = std::make_shared<asset::LoadRequest>();

    switch (testnum) {
      case 0:
        model_load_req->_asset_path = "data://tests/blender-rigtest/blender-rigtest-mesh";
        anim_load_req->_asset_path  = "data://tests/blender-rigtest/blender-rigtest-anim1";
        break;
      case 1:
        model_load_req->_asset_path = "data://tests/misc_gltf_samples/RiggedFigure/RiggedFigure";
        anim_load_req->_asset_path  = "data://tests/misc_gltf_samples/RiggedFigure/RiggedFigure";
        break;
      case 2:
        model_load_req->_asset_path = "data://tests/chartest/char_mesh";
        anim_load_req->_asset_path  = "data://tests/chartest/char_testanim1";
        break;
      case 3:
        model_load_req->_asset_path = "data://tests/hfstest/hfs_rigtest";
        anim_load_req->_asset_path  = "data://tests/hfstest/hfs_rigtest_anim";
        break;
      default:
        OrkAssert(false);
        break;
    }

    auto mesh_override = vars["mesh"].as<std::string>();
    auto anim_override = vars["anim"].as<std::string>();

    if (mesh_override.length()) {
      model_load_req->_asset_path = mesh_override;
    }
    if (anim_override.length()) {
      anim_load_req->_asset_path = anim_override;
    }

    _char_modelasset = asset::AssetManager<XgmModelAsset>::load(model_load_req);
    OrkAssert(_char_modelasset);
    model_load_req->waitForCompletion();

    auto model    = _char_modelasset->getSharedModel();
    auto skeldump = model->mSkeleton.dump(fvec3(1, 1, 1));
    printf("skeldump<%s>\n", skeldump.c_str());

    _char_animasset = asset::AssetManager<XgmAnimAsset>::load(anim_load_req);
    OrkAssert(_char_animasset);
    anim_load_req->waitForCompletion();

    _char_drawable->bindModel(model);
    _char_drawable->_name = "char";
    auto modelinst        = _char_drawable->_modelinst;
    // modelinst->setBlenderZup(true);
    modelinst->enableSkinning();
    modelinst->enableAllMeshes();
    modelinst->_drawSkeleton = true;

    // model->mSkeleton.mTopNodesMatrix.compose(fvec3(),fquat(),0.0001);

    auto anim      = _char_animasset->GetAnim();
    _char_animinst = std::make_shared<XgmAnimInst>();
    _char_animinst->bindAnim(anim);
    _char_animinst->SetWeight(1.0f);
    _char_animinst->RefMask().EnableAll();
    //_char_animinst->RefMask().Disable(model->mSkeleton,"mixamorig.RightShoulder");
    //_char_animinst->RefMask().Disable(model->mSkeleton,"mixamorig.RightArm");
    //_char_animinst->RefMask().Disable(model->mSkeleton,"mixamorig.RightForeArm");
    //_char_animinst->RefMask().Disable(model->mSkeleton,"mixamorig.RightHand");
    _char_animinst->_use_temporal_lerp = true;
    _char_animinst->bindToSkeleton(model->mSkeleton);

    _char_animinst2 = std::make_shared<XgmAnimInst>();
    _char_animinst2->bindAnim(anim);
    _char_animinst2->SetWeight(0.5);
    _char_animinst2->RefMask().DisableAll();
    _char_animinst2->RefMask().Enable(model->mSkeleton, "mixamorig.RightShoulder");
    _char_animinst2->RefMask().Enable(model->mSkeleton, "mixamorig.RightArm");
    _char_animinst2->RefMask().Enable(model->mSkeleton, "mixamorig.RightForeArm");
    _char_animinst2->RefMask().Enable(model->mSkeleton, "mixamorig.RightHand");
    _char_animinst2->_use_temporal_lerp = true;
    _char_animinst2->bindToSkeleton(model->mSkeleton);

    _char_animinst3 = std::make_shared<XgmAnimInst>();
    _char_animinst3->bindAnim(anim);
    _char_animinst3->SetWeight(0.5);
    _char_animinst3->RefMask().DisableAll();
    _char_animinst3->RefMask().Enable(model->mSkeleton, "mixamorig.RightShoulder");
    _char_animinst3->RefMask().Enable(model->mSkeleton, "mixamorig.RightArm");
    _char_animinst3->RefMask().Enable(model->mSkeleton, "mixamorig.RightForeArm");
    _char_animinst3->RefMask().Enable(model->mSkeleton, "mixamorig.RightHand");
    _char_animinst3->_use_temporal_lerp = true;
    _char_animinst3->bindToSkeleton(model->mSkeleton);

    _char_applicatorL = std::make_shared<lev2::XgmSkelApplicator>(model->mSkeleton);
    _char_applicatorL->bindToBone("mixamorig.LeftShoulder");
    _char_applicatorL->bindToBone("mixamorig.LeftArm");
    _char_applicatorL->bindToBone("mixamorig.LeftForeArm");
    _char_applicatorL->bindToBone("mixamorig.LeftHand");
    _char_applicatorL->bindToBone("mixamorig.LeftHandIndex1");
    _char_applicatorL->bindToBone("mixamorig.LeftHandIndex2");
    _char_applicatorL->bindToBone("mixamorig.LeftHandIndex3");
    _char_applicatorL->bindToBone("mixamorig.LeftHandIndex4");
    _char_applicatorL->bindToBone("mixamorig.LeftHandThumb1");
    _char_applicatorL->bindToBone("mixamorig.LeftHandThumb2");
    _char_applicatorL->bindToBone("mixamorig.LeftHandThumb3");
    _char_applicatorL->bindToBone("mixamorig.LeftHandThumb4");

    _char_applicatorR1 = std::make_shared<lev2::XgmSkelApplicator>(model->mSkeleton);
    _char_applicatorR1->bindToBone("mixamorig.RightArm");

    _char_applicatorR2 = std::make_shared<lev2::XgmSkelApplicator>(model->mSkeleton);
    _char_applicatorR2->bindToBone("mixamorig.RightForeArm");

    // OrkAssert(false);
    auto& localpose = modelinst->_localPose;
    auto& worldpose = modelinst->_worldPose;

    localpose.bindPose();
    _char_animinst->_current_frame = 0;
    _char_animinst->applyToPose(localpose);
    localpose.blendPoses();
    localpose.concatenate();
    worldpose.apply(fmtx4(), localpose);
    // OrkAssert(false);

    auto rarm  = model->mSkeleton.bindMatrixByName("mixamorig.RightArm");
    auto rfarm = model->mSkeleton.bindMatrixByName("mixamorig.RightForeArm");
    auto rhand = model->mSkeleton.bindMatrixByName("mixamorig.RightHand");

    fvec3 rarm_pos, rfarm_pos;
    fquat rarm_quat, rfarm_quat;
    float rarm_scale, rfarm_scale;

    rarm.decompose(rarm_pos, rarm_quat, rarm_scale);
    rfarm.decompose(rfarm_pos, rfarm_quat, rfarm_scale);

    auto v_farm_arm  = rfarm.translation() - rarm.translation();
    auto v_hand_farm = rhand.translation() - rfarm.translation();

    _rarm_len    = v_farm_arm.length();
    _rfarm_len   = v_hand_farm.length();
    _rarm_scale  = rarm_scale;
    _rfarm_scale = rfarm_scale;

    //////////////////////////////////////////////
    // scenegraph nodes
    //////////////////////////////////////////////

    _char_node = sg_layer->createDrawableNode("mesh-node", _char_drawable);

    _uicamera = std::make_shared<EzUiCam>();
    _uicamera->_constrainZ = true;
    _uicamera->_base_zmoveamt = 2.0f;
    _uicamera->mfLoc = 100.0f;
    ctx->debugPopGroup();
  }

  lev2::xgmanimmask_ptr_t _char_animmask;
  lev2::xgmaniminst_ptr_t _char_animinst;
  lev2::xgmaniminst_ptr_t _char_animinst2;
  lev2::xgmaniminst_ptr_t _char_animinst3;
  lev2::xgmanimassetptr_t _char_animasset;   // retain anim
  lev2::xgmmodelassetptr_t _char_modelasset; // retain model
  lev2::xgmskelapplicator_ptr_t _char_applicatorL;
  lev2::xgmskelapplicator_ptr_t _char_applicatorR1;
  lev2::xgmskelapplicator_ptr_t _char_applicatorR2;
  lev2::xgmskelapplicator_ptr_t _char_applicatorR3;
  lev2::ezuicam_ptr_t _uicamera;
  model_drawable_ptr_t _char_drawable;
  scenegraph::node_ptr_t _char_node;

  varmap::varmap_ptr_t _sg_params;
  scenegraph::scene_ptr_t _sg_scene;

  cameradata_ptr_t _camdata;
  cameradatalut_ptr_t _camlut;

  float _rarm_len    = 0.0f;
  float _rfarm_len   = 0.0f;
  float _rarm_scale  = 0.0f;
  float _rfarm_scale = 0.0f;
};

///////////////////////////////////////////////////////////////////

int main(int argc, char** argv, char** envp) {

  float camera_distance = 10.0f;
  fquat camROT;

  auto init_data = std::make_shared<ork::AppInitData>(argc, argv, envp);

  auto desc = init_data->commandLineOptions("model3dpbr example Options");
  desc->add_options()                  //
      ("help", "produce help message") //
      ("msaa", po::value<int>()->default_value(1), "msaa samples(*1,4,9,16,25)")(
          "ssaa", po::value<int>()->default_value(1), "ssaa samples(*1,4,9,16,25)")(
          "forward", po::bool_switch()->default_value(false), "forward renderer")(
          "fullscreen", po::bool_switch()->default_value(false), "fullscreen mode")(
          "left", po::value<int>()->default_value(100), "left window offset")(
          "top", po::value<int>()->default_value(100), "top window offset")(
          "width", po::value<int>()->default_value(1280), "window width")(
          "height", po::value<int>()->default_value(720), "window height")(
          "usevr", po::bool_switch()->default_value(false), "use vr output")(
          "testnum", po::value<int>()->default_value(0), "animation test level")(
          "fbase", po::value<std::string>()->default_value(""), "set user fbase")(
          "mesh", po::value<std::string>()->default_value(""), "mesh file override")(
          "anim", po::value<std::string>()->default_value(""), "animation file override");

  auto vars = *init_data->parse();

  if (vars.count("help")) {
    std::cout << (*desc) << "\n";
    exit(0);
  }
  //////////////////////////////////////////////////////////
  int testnum       = vars["testnum"].as<int>();
  std::string fbase = vars["fbase"].as<std::string>();
  //////////////////////////////////////////////////////////
  if (fbase.length()) {
    auto fdevctx = FileEnv::createContextForUriBase("fbase://", fbase);
  }
  //////////////////////////////////////////////////////////

  init_data->_fullscreen = vars["fullscreen"].as<bool>();
  ;
  init_data->_top          = vars["top"].as<int>();
  init_data->_left         = vars["left"].as<int>();
  init_data->_width        = vars["width"].as<int>();
  init_data->_height       = vars["height"].as<int>();
  init_data->_msaa_samples = vars["msaa"].as<int>();
  init_data->_ssaa_samples = vars["ssaa"].as<int>();

  printf("_msaa_samples<%d>\n", init_data->_msaa_samples);
  bool use_forward = vars["forward"].as<bool>();
  //////////////////////////////////////////////////////////
  init_data->_imgui            = true;
  init_data->_application_name = "ork.model3dpbr";
  //////////////////////////////////////////////////////////
  auto ezapp = OrkEzApp::create(init_data);
  std::shared_ptr<GpuResources> gpurec;
  //////////////////////////////////////////////////////////
  // gpuInit handler, called once on main(rendering) thread
  //  at startup time
  //////////////////////////////////////////////////////////
  ezapp->onGpuInit([&](Context* ctx) { gpurec = std::make_shared<GpuResources>(init_data, ctx, use_forward); });
  //////////////////////////////////////////////////////////
  // update handler (called on update thread)
  //  it will never be called before onGpuInit() is complete...
  //////////////////////////////////////////////////////////
  ork::Timer timer;
  timer.Start();
  auto dbufcontext = std::make_shared<DrawBufContext>();
  auto sframe      = std::make_shared<StandardCompositorFrame>();
  float animspeed  = 1.0f;
  ezapp->onUpdate([&](ui::updatedata_ptr_t updata) {


    double dt      = updata->_dt;
    double abstime = updata->_abstime + dt + .016;
    ///////////////////////////////////////
    // compute camera data
    ///////////////////////////////////////
    float phase = 4; // PI * abstime * 0.05;

    fvec3 tgt(0, 0, 0);
    fvec3 up(0, 1, 0);
    auto eye   = fvec3(sinf(phase), 0.3f, -cosf(phase));
    float wsca = 1.0f;
    float near = 1;
    float far  = 500.0f;
    float fovy = 45.0f;

    switch (testnum) {
      case 0:
        far = 50.0f;
        eye *= 10.0f;
        wsca = 0.15f;
        break;
      case 1:
        eye += fvec3(0, -1, 0);
        eye *= 10.0f;
        wsca = 10.0f;
        break;
      case 2:
        tgt += fvec3(0, 50, 0);
        eye *= 100.0f;
        eye += fvec3(0, -50, 0);
        wsca = 6.0f;
        break;
      case 3:
        eye *= 100.0f;
        wsca = 100.1f;
        break;
      default:
        break;
    }

    gpurec->_uicamera->aper = fovy * DTOR;
    gpurec->_uicamera->updateMatrices();

    (*gpurec->_camdata) = gpurec->_uicamera->_camcamdata;

    ////////////////////////////////////////
    // set character node's world transform
    ////////////////////////////////////////

    fvec3 wpos(0, 0, 0);
    fquat wori; // fvec3(0,1,0),phase+PI);

    gpurec->_char_node->_dqxfdata._worldTransform->set(wpos, wori, wsca);

    ////////////////////////////////////////
    // enqueue scenegraph to renderer
    ////////////////////////////////////////

    gpurec->_sg_scene->enqueueToRenderer(gpurec->_camlut);

    ////////////////////////////////////////
  });
  //////////////////////////////////////////////////////////
  // draw handler (called on main(rendering) thread)
  //////////////////////////////////////////////////////////
  ezapp->onDraw([&](ui::drawevent_constptr_t drwev) {
    float time  = timer.SecsSinceStart();
    float frame = (time * 30.0f * animspeed);

    auto anim = gpurec->_char_animasset->GetAnim();

    gpurec->_char_animinst->_current_frame = fmod(frame, float(anim->_numframes));
    gpurec->_char_animinst->SetWeight(0.5f);
    gpurec->_char_animinst2->_current_frame = fmod(frame * 1.3, float(anim->_numframes));
    gpurec->_char_animinst2->SetWeight(0.5);
    gpurec->_char_animinst3->_current_frame = fmod(frame, float(anim->_numframes));
    gpurec->_char_animinst3->SetWeight(0.75);

    auto modelinst  = gpurec->_char_drawable->_modelinst;
    auto& localpose = modelinst->_localPose;
    auto& worldpose = modelinst->_worldPose;

    localpose.bindPose();
    gpurec->_char_animinst->applyToPose(localpose);
    // gpurec->_char_animinst2->applyToPose(localpose);
    // gpurec->_char_animinst3->applyToPose(localpose);
    localpose.blendPoses();

    // auto lpdump = localpose.dump();
    // printf( "%s\n", lpdump.c_str() );

    localpose.concatenate();

    ///////////////////////////////////////////////////////////
    // use skel applicator on post concatenated bones
    ///////////////////////////////////////////////////////////

    auto model = gpurec->_char_modelasset->getSharedModel();
    auto& skel = model->skeleton();
    if (0) { // fmod(time, 10) < 5) {

      int ji_lshoulder     = skel.jointIndex("mixamorig.LeftShoulder");
      auto lshoulder_base  = localpose._concat_matrices[ji_lshoulder];
      auto lshoulder_basei = lshoulder_base.inverse();

      fmtx4 rotmtx;
      rotmtx.setRotateY((sinf(time * 5) * 7.5) * DTOR);
      rotmtx = lshoulder_basei * rotmtx * lshoulder_base;

      gpurec->_char_applicatorL->apply([&](int index) {
        auto& ci = localpose._concat_matrices[index];
        ci       = (rotmtx * ci);
      });
    }
    if (1) { // else{

      int ji_rshoulder = skel.jointIndex("mixamorig.RightShoulder");
      int ji_rarm      = skel.jointIndex("mixamorig.RightArm");
      int ji_rfarm     = skel.jointIndex("mixamorig.RightForeArm");
      int ji_rhand     = skel.jointIndex("mixamorig.RightHand");

      const auto& rshoulder = localpose._concat_matrices[ji_rshoulder];
      auto rshoulder_i      = rshoulder.inverse();

      auto rarm    = localpose._concat_matrices[ji_rarm];
      auto rarm_i  = rarm.inverse();
      auto rfarm   = localpose._concat_matrices[ji_rfarm];
      auto rfarm_i = rfarm.inverse();

      localpose._boneprops[ji_rarm] = 1;

      ///////////////////////

      auto rhand         = localpose._concat_matrices[ji_rhand];
      auto wrist_xnormal = rhand.xNormal();
      auto wrist_ynormal = rhand.yNormal();
      auto wrist_znormal = rhand.zNormal();

      auto elbow_pos    = rhand.translation() - (wrist_ynormal * gpurec->_rfarm_len);
      auto elbow_normal = (elbow_pos - rshoulder.translation()).normalized();

      fmtx4 elbowR, elbowS, elbowT;
      elbowR.fromNormalVectors(
          wrist_xnormal,  //
          -wrist_ynormal, //
          wrist_znormal);
      elbowS.setScale(gpurec->_rfarm_scale);
      elbowT.setTranslation(elbow_pos);

      rfarm = elbowT * (elbowR * elbowS);

      fmtx4 MM, MS;
      MM.correctionMatrix(rshoulder, rfarm);
      MS.setScale(gpurec->_rarm_scale);

      ///////////////////////

      // localpose._concat_matrices[ji_rfarm] = (MS*MM)*rshoulder;
      // localpose._concat_matrices[ji_rfarm] = rhand;
    }

    ///////////////////////////////////////////////////////////

    auto context = drwev->GetTarget();
    RenderContextFrameData RCFD(context); // renderer per/frame data
    RCFD.setUserProperty("vrcam"_crc, (const CameraData*)gpurec->_camdata.get());
    gpurec->_sg_scene->renderOnContext(context, RCFD);
  });
  //////////////////////////////////////////////////////////
  ezapp->onResize([&](int w, int h) { //
    gpurec->_sg_scene->_compositorImpl->compositingContext().Resize(w, h); 
    gpurec->_uicamera->_vpdim      = fvec2(w,h);
  });
  ezapp->onGpuExit([&](Context* ctx) { gpurec = nullptr; });
  //////////////////////////////////////////////////////////
  ezapp->onUiEvent([&](ui::event_constptr_t ev) -> ui::HandlerResult {
    bool handled =  gpurec->_uicamera->UIEventHandler(ev);
    return ui::HandlerResult();
  });
  //////////////////////////////////////////////////////////
  ezapp->setRefreshPolicy({EREFRESH_FASTEST, -1});
  return ezapp->mainThreadLoop();
}
