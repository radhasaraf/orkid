#!/usr/bin/env python3

################################################################################
# lev2 sample which renders a scenegraph to a window
# Copyright 1996-2023, Michael T. Mayers.
# Distributed under the MIT License
# see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
################################################################################

import math, sys, os, random, numpy, argparse
from obt import path
from pathlib import Path
from orkengine.core import *
from orkengine.lev2 import *
sys.path.append(str(path.orkid()/"ork.lev2"/"examples"/"python")) # add parent dir to path
from common.cameras import *
from common.scenegraph import createSceneGraph
from signal import signal, SIGINT

tokens = CrcStringProxy()

from _ptc_harness import *

################################################################################
parser = argparse.ArgumentParser(description='scenegraph particles example')

args = vars(parser.parse_args())

################################################################################

class ParticlesApp(object):

  def __init__(self):
    super().__init__()
    self.ezapp = OrkEzApp.create(self)
    self.ezapp.setRefreshPolicy(RefreshFastest, 0)

    #self.materials = set()

    setupUiCamera( app=self, #
                   eye = vec3(0,100,150), #
                   constrainZ=True, #
                   up=vec3(0,1,0))

  ################################################
  # gpu data init:
  #  called on main thread when graphics context is
  #   made available
  ##############################################

  def onGpuInit(self,ctx):

    ###################################
    # create scenegraph
    ###################################
    sceneparams = VarMap() 
    sceneparams.preset = "ForwardPBR"
    sceneparams.SkyboxIntensity = float(1)
    sceneparams.SpecularIntensity = float(1)
    sceneparams.DiffuseIntensity = float(1)
    sceneparams.AmbientLight = vec3(0.1)
    sceneparams.DepthFogDistance = float(1e6)
    sceneparams.SkyboxTexPathStr = "src://envmaps/tozenv_caustic1.png"
    ###################################
    # post fx node
    ###################################
    postNode = DecompBlurPostFxNode()
    postNode.threshold = 0.99
    postNode.blurwidth = 16.0
    postNode.blurfactor = 0.1
    postNode.amount = 0.1
    postNode.gpuInit(ctx,8,8);
    postNode.addToVarMap(sceneparams,"PostFxNode")
    #self.post_node = postNode
    ###################################
    self.scene = self.ezapp.createScene(sceneparams)
    self.layer1 = self.scene.createLayer("layer1")
    ###################################
    # create particle drawable 
    ###################################
    ptc_data = {
      "POOL":particles.Pool,
      "EMITN":particles.NozzleEmitter,
      "EMITR":particles.RingEmitter,
      "GLOB":particles.Globals,
      #"PNTA":particles.PointAttractor,
      "GRAV":particles.Gravity,
      "TURB":particles.Turbulence,
      "VORT":particles.Vortex,
      "DRAG":particles.Drag,
      "LITE":particles.LightRenderer,
      "SPRI":particles.SpriteRenderer,
    }
    ptc_connections = [
      ("POOL","EMITN"),
      ("EMITN","EMITR"),
      ("EMITR","GRAV"),
      ("GRAV","TURB"),
      ("TURB","VORT"),
      ("VORT","DRAG"),
      ("DRAG","LITE"),
      ("LITE","SPRI"),
    ]
    #######################################
    def gen_sys(grad,frq,radius):
      ptc = ParticleContainer(self.scene,self.layer1)
      createParticleData(ptc,ptc_data,ptc_connections)
      ptc.POOL.pool_size = 8192 # max number of particles in pool
      ptc.SPRI.inputs.Size = 0.1
      ptc.SPRI.inputs.GradientIntensity = 1
      ptc.SPRI.material = presetMaterial(grad=grad)
      ptc.EMITN.inputs.EmissionVelocity = 0.1
      #presetPOOL1(ptc.POOL)
      presetEMITN1(ptc.EMITN)
      presetEMITR1(ptc.EMITR)
      ptc.EMITN.inputs.EmissionRate = 100
      ptc.EMITR.inputs.EmissionRate = 100
      ptc.EMITN.inputs.LifeSpan = 20
      ptc.EMITR.inputs.LifeSpan = 20
      presetTURB1(ptc.TURB)
      presetVORT1(ptc.VORT)
      ptc.VORT.inputs.VortexStrength = 0.0
      ptc.VORT.inputs.OutwardStrength = 0.0
      presetGRAV1(ptc.GRAV)
      ptc.particlenode.worldTransform.translation = vec3(50,10,0)    
      ptc.TURB.inputs.Amount = vec3(1,1,1)*5
      ptc.frq = frq
      ptc.radius = radius
      ptc.DRAG.inputs.drag = 0.999
      ptc.drawable_data.emitterIntensity = 5.0
      return ptc
    #######################################
    self.ptc_systems = [
      gen_sys(presetGRAD1(),0.5,40),
      gen_sys(presetGRAD2(),0.6,40),
      gen_sys(presetGRAD3(),0.7,45),
      gen_sys(presetGRAD4(),0.8,45),
      gen_sys(presetGRAD5(),0.9,50),
      gen_sys(presetGRAD6(),1.1,50),
      gen_sys(presetGRAD7(),1.15,55),
      gen_sys(presetGRAD8(),1.3,55)
    ]
    #self.ptc_systems[0].EMITN.inputs.EmissionVelocity = 0.1
    #######################################
    gmtl = PBRMaterial() 
    gmtl.texColor = Texture.load("src://effect_textures/white.dds")
    gmtl.texNormal = Texture.load("src://effect_textures/default_normal.dds")
    gmtl.texMtlRuf = Texture.load("src://effect_textures/white.dds")
    gmtl.metallicFactor = 1
    gmtl.roughnessFactor = 1
    gmtl.baseColor = vec4(0.8,0.8,1,1)
    gmtl.gpuInit(ctx)
    gdata = GroundPlaneDrawableData()
    gdata.pbrmaterial = gmtl
    gdata.extent = 1000.0
    self.gdata = gdata
    self.drawable_ground = gdata.createSGDrawable(self.scene)
    self.groundnode = self.layer1.createDrawableNode("partgroundicle-node",self.drawable_ground)
    self.groundnode.worldTransform.translation = vec3(0,-5,0)
    #######################################
    self.model = XgmModel("data://tests/misc_gltf_samples/DamagedHelmet.glb")
    self.sgnode = self.model.createNode("model-node",self.layer1)
    self.sgnode.worldTransform.scale = 35
    self.sgnode.worldTransform.translation = vec3(0,25,0)
    #######################################


  ################################################

  def onUpdate(self,updinfo):
    self.scene.updateScene(self.cameralut) # update and enqueue all scenenodes
    for item in self.ptc_systems:

      prv_trans = item.particlenode.worldTransform.translation

      x = item.radius*math.cos(updinfo.absolutetime*item.frq)
      y = 30+math.sin(updinfo.absolutetime*item.frq)*30
      z = item.radius*math.sin(updinfo.absolutetime*item.frq)*-1.0
      
      new_trans = vec3(x,y,z)

      #delta_dir = (new_trans-prv_trans).normalized()
      #item.EMITN.inputs.Offset = new_trans
      #item.EMITR.inputs.Offset = new_trans
      #item.PNTA.inputs.position = new_trans
      
      item.particlenode.worldTransform.translation = new_trans
    
  ##############################################

  def onUiEvent(self,uievent):
    handled = self.uicam.uiEventHandler(uievent)
    if handled:
      self.camera.copyFrom( self.uicam.cameradata )
    return ui.HandlerResult()

###############################################################################

def sig_handler(signal_received, frame):
  print('SIGINT or CTRL-C detected. Exiting gracefully')
  sys.exit(0)

###############################################################################

signal(SIGINT, sig_handler)

ParticlesApp().ezapp.mainThreadLoop()
