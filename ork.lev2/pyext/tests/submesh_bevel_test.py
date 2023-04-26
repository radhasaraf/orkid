#!/usr/bin/env python3
################################################################################
# Copyright 1996-2023, Michael T. Mayers.
# Distributed under the MIT License
# see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
################################################################################
import ork.path
import math, random, argparse, sys
import trimesh, pickle
from orkengine.core import *
from orkengine.lev2 import *
from _boilerplate import *
################################################################################
# apply transform to trimesh
################################################################################

################################################################################

class SceneGraphApp(BasicUiCamSgApp):
  ##############################################
  def __init__(self):
    super().__init__()
  ##############################################
  def cutWithPlane(self,inpsubmesh,n,o):
    nn = n.normalized()
    tp = dplane(nn,0)
    d = tp.distanceToPoint(o) * -1.0
    new_plane = dplane(nn,d)
    return clipMeshWithPlane(inpsubmesh,new_plane,debug=True)
  ##############################################
  def onGpuInit(self,ctx):
    super().onGpuInit(ctx)
    ##################################
    # solid wire pipeline
    ##################################
    solid_wire_pipeline = self.createBaryWirePipeline()
    material = solid_wire_pipeline.sharedMaterial
    solid_wire_pipeline.bindParam( material.param("m"), tokens.RCFD_M)

    #################################################################
    # source mesh paths
    #################################################################

    cub_path = "data://tests/simple_obj/box.obj"

      # load source meshes

    cub_orkmesh = meshutil.Mesh()
    cub_orkmesh.readFromWavefrontObj(cub_path)
    cub_submesh = cub_orkmesh.submesh_list[0]

    cub_submesh = self.cutWithPlane(cub_submesh,dvec3(0,1,1),dvec3(0,0,0))

    self.barysubmesh = cub_submesh.withBarycentricUVs()

    self.bary_prim = meshutil.RigidPrimitive(self.barysubmesh,ctx)
    self.bary_sgnode = self.bary_prim.createNode("union",self.layer1,solid_wire_pipeline)
    self.bary_sgnode.enabled = True

###############################################################################
sgapp = SceneGraphApp()
sgapp.ezapp.mainThreadLoop(on_iter=lambda: sgapp.onGpuIter() )
