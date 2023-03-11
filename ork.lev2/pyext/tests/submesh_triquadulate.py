#!/usr/bin/env python3
################################################################################
# Copyright 1996-2020, Michael T. Mayers.
# Distributed under the Boost Software License - Version 1.0 - August 17, 2003
# see http://www.boost.org/LICENSE_1_0.txt
################################################################################
import ork.path
from orkengine.core import *
from orkengine.lev2 import *

coreappinit() # setup filesystem
mesh = meshutil.Mesh()
mesh.readFromWavefrontObj("data://tests/simple_obj/monkey.obj")

submesh = mesh.submesh_list[0]

as_tris = submesh.triangulate()
as_quads = as_tris.quadulate(area_tolerance=100.0, #
                             exclude_non_coplanar=False, #
                             exclude_non_rectangular=False, #
                             )

print(submesh)
print(as_tris)
print(as_quads)

as_quads.writeWavefrontObj(str(ork.path.temp()/"monkey_quadulated_out.obj"));