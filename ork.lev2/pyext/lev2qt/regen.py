#!/usr/bin/env python3

import os
from ork import command, log, path, dep

log.marker( "Generating lev2 shiboken bindings")

this_dir = os.path.dirname(os.path.realpath(__file__))
orkid_dir = path.Path(os.environ["ORKID_WORKSPACE_DIR"])
qt_dir = path.Path(os.environ["QTDIR"])
os.chdir(this_dir)

# todo - autodetect and run only when changes occured

env = {
  "CLANG_INSTALL_DIR": "/usr/include/clang/10"
}

#os.system("rm -rf %s"%output_dir)
"""
command.run([
  "shiboken2",
  #"--silent",
  "--output-directory=%s"%output_dir,
  "-I%s"%this_dir,
  "-I%s"%(qt_dir/"include"),
  "-I%s"%(orkid_dir/"ork.core"/"inc"),
  "-I%s"%(orkid_dir/"ork.lev2"/"inc"),
  "--enable-pyside-extensions",
  "--avoid-protected-hack",
  "--enable-parent-ctor-heuristic",
  "--use-isnull-as-nb_nonzero",
  "--enable-return-value-heuristic",
  "--generator-set",
  "--language-level=c++17",
  "test.h",
  "test.xml"
], env)
"""



import os
#import sipconfig
from PyQt5 import QtCore
################################################################################
qt_inc_dir = dep.instance("qt5").include_dir

target_dir = path.stage()/"orkid"/"sip-bindings"/"lev2-qtui"
#command.run(["rm","-rf",target_dir])
#command.run(["mkdir","-p",target_dir])

# The name of the SIP build file generated by SIP and used by the build
# system.
build_file = "test.sbf"
sip_flags = QtCore.PYQT_CONFIGURATION["sip_flags"]
# Get the PyQt configuration information.
#config = sipconfig.Configuration()

# Run SIP to generate the code.  Note that we tell SIP where to find the qt
# module's specification files using the -I flag.

cmd = ["sip-install","--verbose","--tracing",
       "--build-dir",target_dir
      ]
#       "-I" + str(dep.instance("pyqt5").pysite_dir/"bindings")]
#cmd += sip_flags.split(" ")
#cmd += ["test.sip"]
#command.run(cmd)


#cmd = ["sip5","-c", target_dir,
#       "-I" + str(dep.instance("pyqt5").pysite_dir/"bindings")]
#cmd += sip_flags.split(" ")
#cmd += ["test.sip"]
#command.run(cmd)

# Create the Makefile.
#makefile = sipconfig.SIPModuleMakefile(config, build_file)

# Add the library we are wrapping.  The name doesn't include any platform
# specific prefixes or extensions (e.g. the "lib" prefix on UNIX, or the
# ".dll" extension on Windows).
#extraFlags = "-std=c++11 -I%s -I%s/QtCore -I%s/QtGui" % (qt_inc_dir, qt_inc_dir, qt_inc_dir)
#makefile.extra_cflags = [extraFlags]
#makefile.extra_cxxflags = [extraFlags]
#makefile.extra_lflags = ["-Wl,-R" + target_dir]
#makefile.extra_lib_dirs = [target_dir]
#makefile.extra_libs = ["hello"]
# Generate the Makefile itself.
#makefile.generate()
