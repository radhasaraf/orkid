#!/usr/bin/env python3

################################################################################
# lev2 sample which renders a scenegraph, optionally in VR mode
# Copyright 1996-2020, Michael T. Mayers.
# Distributed under the Boost Software License - Version 1.0 - August 17, 2003
# see http://www.boost.org/LICENSE_1_0.txt
################################################################################

import math, random, argparse, sys
from orkengine.core import *
from orkengine.lev2 import *

################################################################################

sys.path.append((thisdir()/".."/".."/"examples"/"python").normalized.as_string) # add parent dir to path
from common.cameras import *
from common.shaders import *
from common.primitives import createGridData
from common.scenegraph import createSceneGraph

################################################################################

parser = argparse.ArgumentParser(description='scenegraph example')

################################################################################

args = vars(parser.parse_args())

################################################################################

class NODE(object):

  def __init__(self,model,layer, index):

    super().__init__()
    self.model = model
    self.sgnode = model.createNode("node%d"%index,layer)
    self.modelinst = self.sgnode.user.pyext_retain_modelinst
    self.sgnode.worldTransform.scale = 1

################################################################################

class SceneGraphApp(object):

  def __init__(self):
    super().__init__()
    self.ezapp = OrkEzApp.create(self)
    self.ezapp.setRefreshPolicy(RefreshFastest, 0)
    self.materials = set()
    self.nodes=[]
    self.camera = CameraData()
    self.cameralut = CameraDataLut()
    self.cameralut.addCamera("spawncam",self.camera)

  ##############################################

  def onGpuInit(self,ctx):

    params_dict = {
      "SkyboxIntensity": float(2),
      "SpecularIntensity": float(1),
      "DepthFogDistance": float(10000)
    }
    createSceneGraph(app=self,
                     rendermodel="DeferredPBR",
                     params_dict=params_dict)

    ###################################

    model = XgmModel("data://tests/pbr_calib.glb")

    for mesh in model.meshes:
      for submesh in mesh.submeshes:
        copy = submesh.material.clone()
        copy.texColor = Texture.load("src://effect_textures/white.dds")
        copy.texNormal = Texture.load("src://effect_textures/default_normal.dds")
        copy.texMtlRuf = Texture.load("src://effect_textures/white.dds")
        submesh.material = copy

    for i in range(81):
      node = NODE(model,self.layer1,i)

      x = (i % 9)
      z = int(i/9)

      ######################
      # set transform
      ######################

      node.sgnode.worldTransform.translation = vec3((x-4)*2,1,(z-4)*2)

      ######################
      # override material for submeshinst
      ######################

      subinst = node.modelinst.submeshinsts[0]
      mtl_cloned = subinst.material.clone()
      mtl_cloned.metallicFactor = float(x/8.0)
      mtl_cloned.roughnessFactor = float(z/8.0)
      mtl_cloned.baseColor = vec4(1,0,0,1)
      subinst.overrideMaterial(mtl_cloned)

      ######################


      self.nodes += [node]

    ###################################

    self.grid_data = createGridData()
    self.grid_node = self.layer1.createGridNode("grid",self.grid_data)
    self.grid_node.sortkey = 1

  ################################################

  def onUpdate(self,updinfo):
    phase = updinfo.absolutetime * 0.2
    x =  math.sin(phase)*10    
    z = -math.cos(phase)*10    
    ###################################
    self.camera.perspective(0.1, 50.0, 35.0*constants.DTOR)
    self.camera.lookAt(vec3(x,5,z)*2.5, # eye
                       vec3(0, 0, 0), # tgt
                       vec3(0, 1, 0)) # up
    self.scene.updateScene(self.cameralut) 

###############################################################################

SceneGraphApp().ezapp.mainThreadLoop()