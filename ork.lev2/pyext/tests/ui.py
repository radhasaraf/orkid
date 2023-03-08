#!/usr/bin/env python3

################################################################################
# lev2 sample which renders a UI to a window
# Copyright 1996-2020, Michael T. Mayers.
# Distributed under the Boost Software License - Version 1.0 - August 17, 2003
# see http://www.boost.org/LICENSE_1_0.txt
################################################################################

from orkengine.core import *
from orkengine.lev2 import *

################################################################################

class UiTestApp(object):

  def __init__(self):
    super().__init__()
    self.ezapp = OrkEzApp.create(self)
    self.ezapp.setRefreshPolicy(RefreshFastest, 0)
    self.ezapp.topWidget.enableUiDraw()
    lg_group = self.ezapp.topLayoutGroup
    griditems = lg_group.makeGrid( width = 2,
                                   height = 2,
                                   margin = 1,
                                   uiclass = ui.UiLambdaBox,
                                   args = ["box",vec4(1,0,1,1)] )

    griditems[0].widget.onPressed(lambda: print("GRIDITEM0 PUSHED"))
    griditems[1].widget.onPressed(lambda: print("GRIDITEM1 PUSHED"))
    griditems[2].widget.onPressed(lambda: print("GRIDITEM2 PUSHED"))
    griditems[3].widget.onPressed(lambda: print("GRIDITEM3 PUSHED"))

###############################################################################

UiTestApp().ezapp.mainThreadLoop()
lbox