cmake_minimum_required (VERSION 3.13.4)
include (GenerateExportHeader)
project (Orkid)

################################################################################

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED on)

################################################################################

set(CMAKE_INSTALL_RPATH "$ENV{OBT_STAGE}/lib")
set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)

################################################################################

set(CMAKE_FIND_DEBUG_MODE OFF)
set(PYTHON_EXECUTABLE $ENV{PYTHONHOME}/bin/python3)
set(PYTHON_LIBRARY $ENV{PYTHONHOME}/lib/libpython3.9d.so)

set(Python3_FIND_STRATEGY "LOCATION")
set(Python3_ROOT_DIR $ENV{PYTHONHOME} )

find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
find_package(pybind11 REQUIRED)

################################################################################

IF(${APPLE})
    set(XCODE_SDKBASE /Library/Developer/CommandLineTools/SDKs)
    set(CMAKE_OSX_SYSROOT ${XCODE_SDKBASE}/MacOSX10.15.sdk)
    set(CMAKE_MACOSX_RPATH 1)
    LIST(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "$ENV{OBT_STAGE}/lib" isSystemDir)
    IF("${isSystemDir}" STREQUAL "-1")
       SET(CMAKE_INSTALL_RPATH "$ENV{OBT_STAGE}/lib")
    ENDIF("${isSystemDir}" STREQUAL "-1")
    add_definitions(-DOSX -DORK_OSX)

    macro(ADD_OSX_FRAMEWORK fwname target)
        find_library(FRAMEWORK_${fwname}
        NAMES ${fwname}
        PATHS ${CMAKE_OSX_SYSROOT}/System/Library
        PATH_SUFFIXES Frameworks
        NO_DEFAULT_PATH)
        if( ${FRAMEWORK_${fwname}} STREQUAL FRAMEWORK_${fwname}-NOTFOUND)
            MESSAGE(ERROR ": Framework ${fwname} not found")
        else()
            TARGET_LINK_LIBRARIES(${target} PUBLIC "${FRAMEWORK_${fwname}}/${fwname}")
            MESSAGE(STATUS "Framework ${fwname} found at ${FRAMEWORK_${fwname}}")
        endif()
    endmacro(ADD_OSX_FRAMEWORK)

    set(CMAKE_MACOSX_RPATH 1)
    LIST(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "$ENV{OBT_STAGE}/lib" isSystemDir)
    IF("${isSystemDir}" STREQUAL "-1")
       SET(CMAKE_INSTALL_RPATH "$ENV{OBT_STAGE}/lib")
    ENDIF("${isSystemDir}" STREQUAL "-1")
    add_definitions(-DOSX -DORK_OSX)
ELSE()
    add_definitions(-DORK_CONFIG_IX -DLINUX -DGCC)
    add_compile_options(-D_REENTRANT -D_LARGEFILE64_SOURCE -D_LARGEFILE_SOURCE)
ENDIF()

add_compile_options(-Wno-deprecated -Wno-register -fexceptions)
add_compile_options(-Wno-unused-command-line-argument)
add_compile_options(-fPIE -fPIC -fno-common -fno-strict-aliasing -g -Wno-switch-enum)
add_compile_options(-fextended-identifiers)

##############################

IF( "${ARCHITECTURE}" STREQUAL "x86_64" )
    add_compile_options(-mavx)
    add_definitions(-DORK_ARCHITECTURE_X86_64 )
ELSEIF( "${ARCHITECTURE}" STREQUAL "AARCH64" )
    add_definitions(-DORK_ARCHITECTURE_ARM_64 )
    #add_compile_options(-fvectorize)
ENDIF()

##############################

set( ORKROOT $ENV{ORKID_WORKSPACE_DIR} )

##############################

#set( destinc $ENV{ORKID_WORKSPACE_DIR}/ork/inc )
#include_directories(BEFORE ${destinc})
link_directories(${destlib})

include_directories(AFTER $ENV{OBT_STAGE}/include)
include_directories(AFTER $ENV{OBT_STAGE}/include/tuio/oscpack)
link_directories($ENV{OBT_STAGE}/lib/ $ENV{OBT_STAGE}/lib64 )

link_directories($ENV{OBT_STAGE}/orkid/ork.tuio )
link_directories($ENV{OBT_STAGE}/orkid/ork.utpp )
#link_directories($ENV{OBT_STAGE}/orkid/ork.core )
#link_directories($ENV{OBT_STAGE}/orkid/ork.lev2 )
#link_directories($ENV{OBT_STAGE}/orkid/ork.ent )
#link_directories($ENV{OBT_STAGE}/orkid/ork.tool )


set( ORK_CORE_INCD ${ORKROOT}/ork.core/inc )
set( ORK_LEV2_INCD ${ORKROOT}/ork.lev2/inc )
set( ORK_ECS_INCD ${ORKROOT}/ork.ecs/inc )

#set( ORK_CORE_LIBD ${OBT_STAGE}/orkid/ork.lev2 )
#set( ORK_LEV2_LIBD ${OBT_STAGE}/orkid/ork.lev2 )
#set( ORK_ECS_LIBD ${OBT_STAGE}/orkid/ork.ent )
#set( ORK_TOOL_LIBD ${OBT_STAGE}/orkid/ork.tool )

################################################################################
# IGL (its a beast, needs a cmake update)
################################################################################

include_directories (AFTER $ENV{OBT_STAGE}/include/eigen3 )

IF( "${ARCHITECTURE}" STREQUAL "x86_64" )

  list(APPEND CMAKE_MODULE_PATH "${LIBIGL_INCLUDE_DIR}/../cmake")
  option(LIBIGL_USE_STATIC_LIBRARY "Use libigl as static library" ON)
  #option(LIBIGL_WITH_ANTTWEAKBAR      "Use AntTweakBar"    OFF)
  option(LIBIGL_WITH_CGAL             "Use CGAL"           ON)
  option(LIBIGL_WITH_COMISO           "Use CoMiso"         ON)
  option(LIBIGL_WITH_CORK             "Use Cork"           ON)
  option(LIBIGL_WITH_EMBREE           "Use Embree"         ON)
  #option(LIBIGL_WITH_LIM              "Use LIM"            OFF)
  #option(LIBIGL_WITH_MATLAB           "Use Matlab"         OFF)
  #option(LIBIGL_WITH_MOSEK            "Use MOSEK"          OFF)
  #option(LIBIGL_WITH_OPENGL           "Use OpenGL"         ON)
  #option(LIBIGL_WITH_OPENGL_GLFW      "Use GLFW"           ON)
  #option(LIBIGL_WITH_PNG              "Use PNG"            OFF)
  #option(LIBIGL_WITH_PYTHON           "Use Python"         OFF)
  option(LIBIGL_WITH_TETGEN           "Use Tetgen"         ON)
  option(LIBIGL_WITH_TRIANGLE         "Use Triangle"       ON)
  #option(LIBIGL_WITH_VIEWER           "Use OpenGL viewer"  ON)
  #option(LIBIGL_WITH_XML              "Use XML"            OFF)
  find_package(LIBIGL REQUIRED)
  #include($ENV{OBT_BUILDS}/igl/cmake/libigl.cmake )

  link_directories($ENV{OBT_BUILDS}/igl/.build)
  include_directories (AFTER $ENV{OBT_BUILDS}/igl/include )
  include_directories (AFTER $ENV{OBT_BUILDS}/igl/external/triangle )

ENDIF()

##############################

function(ork_post_lib_paths TARGET)
  target_link_directories(${TARGET} PRIVATE /usr/local/lib) # homebrew
endfunction()

##############################

function(ork_std_target_opts_compiler TARGET)
  target_include_directories( ${TARGET} PRIVATE ${Python3_INCLUDE_DIRS} ${PYBIND11_INCLUDE_DIRS} )
  IF(${APPLE})
    target_include_directories (${TARGET} PRIVATE /usr/local/include)
  ELSEIF(${UNIX})
  ENDIF()
endfunction()

##############################

function(ork_std_target_opts_linker TARGET)
  IF(${APPLE})
      target_link_libraries(${TARGET} LINK_PRIVATE m pthread)
      target_link_libraries(${TARGET} LINK_PRIVATE
          "-framework AppKit"
          "-framework IOKit"
      )
      target_link_libraries(${TARGET} LINK_PRIVATE objc boost_filesystem  boost_system)
  ELSEIF(${UNIX})
      target_link_libraries(${TARGET} LINK_PRIVATE rt dl pthread boost_filesystem boost_system)
  ENDIF()
  target_link_directories(${TARGET} PUBLIC $ENV{OBT_PYTHON_LIB_PATH} )
  target_link_libraries(${TARGET} LINK_PRIVATE $ENV{OBT_PYTHON_DECOD_NAME} )
  #target_link_options(${TARGET} PRIVATE ${Python3_LINK_OPTIONS})
endfunction()

##############################

function(ork_std_target_opts TARGET)
  ork_post_lib_paths(${TARGET})
  ork_std_target_opts_compiler(${TARGET})
  ork_std_target_opts_linker(${TARGET})
endfunction()

##############################
# ISPC compile option
##############################
function(declare_ispc_source_object src obj dep)
  add_custom_command(OUTPUT ${obj}
                     MAIN_DEPENDENCY ${src}
                     COMMENT "ISPC-Compile ${src}"
                     COMMAND ispc -O3 --target=avx ${src} -g -o ${obj} --colored-output
                     DEPENDS ${dep})
endfunction()
##############################
# ISPC convenience method
#  I really dislike cmake as a language...
##############################
function(gen_ispc_object_list
         ISPC_GLOB_SPEC
         ISPC_SUBDIR
         ISPC_OUTPUT_OBJECT_LIST )
  set(ISPC_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_FILES_DIRECTORY}/${ISPC_SUBDIR})
  #message(${ISPC_GLOB_SPEC})
  #message(${ISPC_SUBDIR})
  #message(${ISPC_OUTPUT_DIR})
  file(GLOB_RECURSE SRC_ISPC ${ISPC_GLOB_SPEC} )
  foreach(SRC_ITEM ${SRC_ISPC})
    file(RELATIVE_PATH SRC_ITEM_RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/src ${SRC_ITEM} )
    get_filename_component(ispc_name_we ${SRC_ITEM_RELATIVE} NAME_WE)
    get_filename_component(ispc_dir ${SRC_ITEM_RELATIVE} DIRECTORY)
    set(OBJ_ITEM ${ISPC_OUTPUT_DIR}/${ispc_name_we}.o)
    #message(${SRC_ITEM})
    #message(${ispc_name_we})
    #message(${ispc_dir})
    #message(${OBJ_ITEM})
    declare_ispc_source_object(${SRC_ITEM} ${OBJ_ITEM} ${SRC_ITEM} )
    list(APPEND _INTERNAL_ISPC_OUTPUT_OBJECT_LIST ${OBJ_ITEM} )
  endforeach(SRC_ITEM)
  #message(${_INTERNAL_ISPC_OUTPUT_OBJECT_LIST})
  set(${ISPC_OUTPUT_OBJECT_LIST} ${_INTERNAL_ISPC_OUTPUT_OBJECT_LIST} PARENT_SCOPE)
endfunction()

##############################
