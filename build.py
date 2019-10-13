#!/usr/bin/env python3

import os, argparse
import ork.host
import ork.dep
from ork.path import Path
from ork.command import Command

parser = argparse.ArgumentParser(description='orkid build')
parser.add_argument('--clean', action="store_true", help='force clean build' )
parser.add_argument('--verbose', action="store_true", help='verbose build' )
parser.add_argument('--serial',action="store_true", help="non-parallel-build")
parser.add_argument('--debug',action="store_true", help=" debug build")
parser.add_argument('--xcode',action="store_true", help=" xcode debug build")
_args = vars(parser.parse_args())

ork.dep.require(["yarl","bullet","luajit","openexr","oiio","openvr","fcollada"])

build_dest = ork.path.stage()/"orkid"
debug = _args["debug"]!=False

if _args["xcode"]!=False:
    build_dest = ork.path.stage()/"orkid-xcode"

build_dest.mkdir(parents=True,exist_ok=True)
build_dest.chdir()

prj_root = Path(os.environ["ORKID_WORKSPACE_DIR"])
cmd = ["cmake"]

if _args["xcode"]!=False:
    debug = True
    cmd += ["-G","Xcode"]

if debug:
    cmd += ["-DCMAKE_BUILD_TYPE=Debug"]
else:
    cmd += ["-DCMAKE_BUILD_TYPE=Release"]

cmd += ["-DCMAKE_FIND_DEBUG_MODE=ON"]

cmd += [prj_root]

Command(cmd).exec()



if _args["clean"]!=False:
    Command(["make","clean"]).exec()

cmd = ["make"]
if _args["verbose"]!=False:
    cmd += ["VERBOSE=1"]

if _args["serial"]==False:
    cmd += ["-j",ork.host.NumCores]

cmd += ["install"]

Command(cmd).exec()
