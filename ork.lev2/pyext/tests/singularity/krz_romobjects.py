#!/usr/bin/env python3

################################################################################
# lev2 sample which renders a UI with four views to the same scenegraph to a window
# Copyright 1996-2023, Michael T. Mayers.
# Distributed under the MIT License
# see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
################################################################################

import sys, math, random, numpy, obt.path, time, bisect
import plotly.graph_objects as go
from collections import defaultdict
import re
from orkengine.core import *
from orkengine.lev2 import *
################################################################################
sys.path.append((thisdir()/"..").normalized.as_string) # add parent dir to path
from _boilerplate import *
from singularity._harness import SingulTestApp, find_index
################################################################################

class KrzApp(SingulTestApp):

  def __init__(self):
    super().__init__()
  
  ##############################################

  def onGpuInit(self,ctx):
    super().onGpuInit(ctx)
    self.syn_data_base = singularity.baseDataPath()/"kurzweil"
    self.krzdata = singularity.KrzSynthData(base_objects=True)
    
    self.soundbank = self.krzdata.bankData
    self.krzprogs = self.soundbank.programsByName
    self.sorted_progs = sorted(self.krzprogs.keys())
    ok_list = [
    "20's_Trumpet",
    "2000_Odyssey",
    "2nd_Oboe",
    "2nd_Violin",
    "3rd_World_Order",
    "40_Something",
    "5_8ve_Percussion",
    "7th_World_String",
    "AC_Dream",
    "A.Bass+Ride_Pno",
    "AcousGtr+Strings",
    "Acous_12_String",
    "Acoustic_Bass",
    "All_In_The_Fader",
    "Ambient_Bells",
    "Analog__Brazz",
    "Arco_Dbl_Bass",
    "Attack_Stack",
    "Bad_Clav",
    "Bamboo_Voices",
    "BaroqueOrchestra",
    "Baroque_Strings",
    "Baroque_Strg_Ens",
    "Batman_Strings",
    "Big_Drum_Corp",
    "Bell_Players",
    "BrazKnuckles",
    "Bright_Piano",
    "Brite_Klav",
    "Cartoon_Perc",
    "Chamber_Section",
    "Chimes",
    "Choral_Sleigh",
    "Chorus_Gtr",
    "Classical_Gtr",
    "Click",
    "Clock_S+H_Lead",
    "Copland_Sft_Trp",
    "Crowd_Stomper_",
    "CowGogiBell",
    "Default_Program",
    "DigThat_DC_Lead",
    "DigiBass",
    "Dog_Chases_Tail",
    "Doomsday",
    "Dual_Bass",
    "Dual_Tri_Bass",
    "Dubb_Bass",
    "Dyn_Brass_+_Horn",
    "Dynamic_Harp",
    "Dyno_EP_Lead",
    "Econo_Kit_",
    "EDrum_Kit_2_",
    "Elect_12_String",
    "Ethereal_Echos",
    "Ethnoo_Lead",
    "Fast_Strings_MW",
    "Fat_Traps_",
    "Finger_Bass",
    "French_Horn_Sec1",
    "F_Horn_Con_Sord",
    "Fluid_Koto",
    "Fun_Delay_Square",
    "General_MIDI_kit",
    "Glass_Web",
    "Glassy_Eyes",
    "Glistener",
    "Grand_Strings",
    "Gtr_Jazz_Band",
    "Guitar_Mutes_1",
    "Guitar_Mutes_2",
    "Guitar___Flute",
    "Hammeron",
    "Harmon_Section",
    "Harp_On_It",
    "Harp_w_8ve_CTL",
    "Hi_Res_Sweeper",
    "Hip_Brass",
    "Horn+Flute_w_Str",
    "Hot_Tamali_Kit",
    "Hybrid_Breath",
    "Hybrid_Sweep",
    "Islanders",
    "India_Jam",
    "Jam_Corp",
    "Jazz_Lab_Band",
    "Jazz_Muted_Trp",
    "Jazz_Quartet",
    "Jethro's_Flute",
    "Klakran",
    "Klarinet",
    "Koto_Followers",
    "Kotobira",
    "Kotolin",
    "Mallet_Voice",
    "MarcatoViolin_MW",
    "Marcato_Cello_MW",
    "Marcato_S.Strngs",
    "Mbira",
    "Mbira_Stack",
    "Medicine_Man",
    "Mello_Hyb_Brass",
    "Metal_Garden",
    "Miles_Unmuted",
    "Mixed_Choir",
    "Mogue_Bass",
    "My_JayDee",
    "Native_Drum",
    "Oboe+Flute_w_Str",
    "Oh_Bee!!!",
    "Orch_Bassoon",
    "Orch_Clarinet",
    "Orch_EnglishHorn",
    "Orch_Percussion1",
    "Orch_Trumpet",
    "Orch_Viola",
    "Orchestral_Pad",
    "Orchestral_Winds",
    "Ostinato_Bass",
    "Outside_L_A",
    "ParaKoto",
    "Pedal_Pipes",
    "Pedal_Steel",
    "Perc_Voices",
    "Piano___S_String",
    "Pipes",
    "Pizzicato_String",
    "Preview_Drums",
    "Prs_Koto",
    "Punch_Gate_Kit_",
    "Ravitar",
    "Real_Drums",
    "Ritual_Metals",
    "Rock_Axe",
    "Rock_Axe_mono",
    "Rock_Kit",
    "Shimmerling",
    "Shudder_Kit_",
    "Sizzl_E_Pno",
    "Skinny_Lead",
    "Slo_Chorus_Gtr",
    "Slo_Solo_Strings",
    "Slo_SynthOrch",
    "Slow_Arco_Bass",
    "Slow_Cello",
    "Slow_Horn",
    "Soaring_Brass",
    "Soft_Trumpet",
    "Solo_Bassoon",
    "Solo_Trombone",
    "Solo_Trumpet",
    "SpaceStation",
    "Stackoid",
    "Steel_Str_Guitar",
    "Stereo_Grand",
    "StrataClav",
    "Strataguitar",
    "Straight_Strat",
    "StreetCorner_Sax",
    "Sweeper",
    "Sweetar",
    "Syncro_Taps",
    "Talk_Talk",
    "Tenor_Sax",
    "Timpani_Chimes",
    "Tine_Elec_Piano",
    "Too_Bad_Bass",
    "TotalCntrl_Orch1",
    "TotalCntrl_Orch2",
    "Touch_Clav",
    "Touch_MiniBass",
    "Touch_StringOrch",
    "Tranquility",
    "Trumpet+Bone",
    "Tuba",
    "Vibratone",
    "Walking_Bass",
    "Waterflute",
    "West_Room_Kit",
    "Wet_Pizz_",
    "Williamsong",
    "Wind_Vox",
    "WonderSynth_Bass",
    "Wood_Bars",
    "Woodwinds_1",
    "Woodwinds_2",
    "Woody_Jam_Rack",
    "World_Rave_Kit",
    "Xylophone"
    ]
    self.sorted_progs = sorted(ok_list)
    #print("krzprogs<%s>" % self.krzprogs)    
    PRG = "Doomsday" # "Stereo_Grand"
    #self.prog_index = find_index(self.sorted_progs, "Stereo_Grand")
    self.synth.masterGain = singularity.decibelsToLinear(-36.0)
    main = self.synth.outputBus("main")
    aux8 = self.synth.outputBus("aux8")
    self.setBusProgram(aux8,self.soundbank.programByName("Chamber_Section"))
    self.setBusProgram(main,self.soundbank.programByName(PRG))
    self.prog_index = find_index(self.sorted_progs, PRG)
    self.prog = self.soundbank.programByName(PRG)
    self.setUiProgram(self.prog)

###############################################################################

app = KrzApp()
app.ezapp.mainThreadLoop()