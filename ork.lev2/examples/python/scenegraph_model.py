#!/usr/bin/env python3
################################################################################
# lev2 sample which renders a scenegraph to a window
# Copyright 1996-2020, Michael T. Mayers.
# Distributed under the Boost Software License - Version 1.0 - August 17, 2003
# see http://www.boost.org/LICENSE_1_0.txt
################################################################################
import math, random
from orkengine.core import *
from orkengine.lev2 import *
################################################################################
class modelinst(object):
  def __init__(self,model,layer):
    super().__init__()
    self.model = model
    self.sgnode = model.createNode("node1",layer)
    Z = random.uniform(-2.5,-125)
    self.pos = vec3(random.uniform(-1,1)*Z,
                    random.uniform(-1,1)*Z,
                    Z)
    self.rot = quat(vec3(0,1,0),0)
    incraxis = vec3(random.uniform(-1,1),
                    random.uniform(-1,1),
                    random.uniform(-1,1)).normal()
    incrmagn = random.uniform(-0.01,0.01)
    self.rotincr = quat(incraxis,incrmagn)
    self.scale = random.uniform(0.5,0.7)
  def update(self,deltatime):
    self.rot = self.rot*self.rotincr
    self.sgnode\
        .worldMatrix\
        .compose( self.pos, # pos
                  self.rot, # rot
                  self.scale) # scale
################################################################################
class SceneGraphApp(object):
  ################################################
  def __init__(self):
    super().__init__()
    self.sceneparams = VarMap()
    self.sceneparams.preset = "PBR"
    self.qtapp = OrkEzQtApp.create(self)
    self.qtapp.setRefreshPolicy(RefreshFastest, 0)
    self.modelinsts=[]
  ##############################################
  def onGpuInit(self,ctx):
    layer = self.scene.createLayer("layer1")
    models = []
    models += [Model("data://tests/pbr1/pbr1")]
    models += [Model("data://tests/pbr_calib.gltf")]
    models += [Model("src://environ/objects/misc/headwalker.obj")]
    models += [Model("src://environ/objects/misc/ref/torus.glb")]
    ###################################
    for i in range(200):
      model = models[i%4]
      self.modelinsts += [modelinst(model,layer)]
    ###################################
    self.camera = CameraData()
    self.cameralut = CameraDataLut()
    self.cameralut.addCamera("spawncam",self.camera)
    ###################################
    self.camera.perspective(0.1, 150.0, 45.0)
    self.camera.lookAt(vec3(0,0,5), # eye
                       vec3(0, 0, 0), # tgt
                       vec3(0, 1, 0)) # up
  ################################################
  def onUpdate(self,updinfo):
    ###################################
    for minst in self.modelinsts:
      minst.update(updinfo.deltatime)
    ###################################
    self.scene.updateScene(self.cameralut) # update and enqueue all scenenodes
################################################
app = SceneGraphApp()
app.qtapp.exec()
