#!/usr/bin/env python3
################################################################################
# lev2 sample which renders to an offscreen buffer
# Copyright 1996-2020, Michael T. Mayers.
# Distributed under the Boost Software License - Version 1.0 - August 17, 2003
# see http://www.boost.org/LICENSE_1_0.txt
################################################################################

import numpy, time
from orkcore import *
from orklev2 import *
from PIL import Image

#lev2appinit()
#gfxenv = GfxEnv.ref
#ctx = gfxenv.loadingContext()
#print(ctx)
#ctx.makeCurrent()

class MyApp:
  ###########################
  def __init__(self):
    pass
  ###########################
  def gpuInit(self,ctx):
    FBI = ctx.FBI()
    GBI = ctx.GBI()
    self.material = FreestyleMaterial()
    self.material.gpuInit(ctx,Path("orkshader://solid"))
    self.tek = self.material.shader.technique("vtxcolor")
    self.par_mvp = self.material.shader.param("MatMVP")

    print(self.material)
    print(self.tek)
    print(self.par_mvp)

    ###################################

    fpmtx = ctx.perspective(45,1,0.1,3)
    fvmtx = ctx.lookAt(vec3(0,0,-1),vec3(0,0,0),vec3(0,1,0))
    frust = Frustum()
    frust.set(fvmtx,fpmtx)

    self.prim = primitives.FrustumPrimitive()
    self.prim.topColor = vec4(0.5,1.0,0.5,1)
    self.prim.bottomColor = vec4(0.5,0.0,0.5,1)
    self.prim.leftColor = vec4(0.0,0.5,0.5,1)
    self.prim.rightColor = vec4(1.0,0.5,0.5,1)
    self.prim.frontColor = vec4(0.5,0.5,1.0,1)
    self.prim.backColor = vec4(0.5,0.5,0.0,1)
    self.prim.frustum = frust
    self.prim.gpuInit(ctx)

    print(self.prim)
    ###################################
    # rtg setup
    ###################################

    FBI.autoclear = True
    FBI.autoclear = True
    #self.rtg = ctx.defaultRTG()
    #ctx.resize(WIDTH,HEIGHT)
    #capbuf = CaptureBuffer()

    ###################################

  ###########################

  def draw(self,drawev):
    ctx = drawev.context
    RCFD = RenderContextFrameData(ctx)

    WIDTH = ctx.mainSurfaceWidth()
    HEIGHT = ctx.mainSurfaceHeight()

    pmatrix = ctx.perspective(70,WIDTH/HEIGHT,0.01,100.0)
    vmatrix = ctx.lookAt(vec3(1,0.8,1),
                         vec3(0,0,0),
                         vec3(0,1,0))

    mvp_matrix = vmatrix*pmatrix

    FBI = ctx.FBI()
    GBI = ctx.GBI()

    FBI.clearcolor = vec4(1,0,0,1)
    ctx.beginFrame()
    #FBI.clear(vec4(0.6,0.6,0.7,1),1.0)
    #ctx.debugMarker("yo")
    self.material.bindTechnique(self.tek)
    self.material.begin(RCFD)
    self.material.bindParamMatrix4(self.par_mvp,mvp_matrix)
    self.prim.draw(ctx)
    self.material.end(RCFD)
    ctx.endFrame()

##############################################

myapp = MyApp()

def onGpuInit(ctx):
  myapp.gpuInit(ctx)

def onDraw(drawev):
  myapp.draw(drawev)
  pass

qtapp = OrkEzQtApp.create( onGpuInit, onDraw)
qtapp.exec()
