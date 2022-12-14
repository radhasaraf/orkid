////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2022, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#include <ork/pch.h>

#include <ork/application/application.h>
#include <ork/asset/Asset.h>
#include <ork/asset/FileAssetLoader.h>
#include <ork/kernel/string/PieceString.h>
#include <ork/kernel/string/ArrayString.h>
#include <ork/kernel/string/PoolString.h>
#include <ork/file/file.h>
#include <ork/kernel/Array.h>
#include <ork/kernel/string/string.h>
#include <ork/file/path.h>

///////////////////////////////////////////////////////////////////////////////
namespace ork::asset {
///////////////////////////////////////////////////////////////////////////////

std::set<file::Path> FileAssetLoader::EnumerateExisting() {
  std::set<file::Path> rval;
  // GetLoaders
  for (auto& item : mLocations) {
    auto wild = file_ext_t("*") + item.mExt;
    auto ctx  = item.mPathBase;
    auto dir  = ctx->getFilesystemBaseAbs();
    auto pid  = ctx->_protoid;
    if (ctx->_vars.hasKey("disablechoices"))
      continue;
    // printf("FileAssetLoader<%p> searching<%s> for pattern<%s>\n", this, dir.c_str(), wild.c_str());

    auto files    = FileEnv::filespec_search(wild.c_str(), dir);
    int inumfiles = (int)files.size();

    printf("FileAssetLoader<%p> searching<%s> for<%s> inumfiles<%d>\n", (void*) this, dir.c_str(), wild.c_str(), inumfiles);

    file::Path::NameType searchdir(dir.toAbsolute().c_str());
    searchdir.replace_in_place("\\", "/");
    for (int ifile = 0; ifile < inumfiles; ifile++) {
      auto the_file                  = files[ifile];
      auto the_stripped              = FileEnv::filespec_strip_base(the_file, "./");
      file::Path::NameType ObjPtrStr = FileEnv::filespec_no_extension(the_stripped);
      file::Path::NameType ObjPtrStrA;
      ObjPtrStrA.replace(ObjPtrStr.c_str(), searchdir.c_str(), "");
      // OldStlSchoolFindAndReplace( ObjPtrStrA, searchdir, file::Path::NameType("") );
      file::Path::NameType ObjPtrStr2 = file::Path::NameType(pid.c_str()) + ObjPtrStrA;
      file::Path OutPath(ObjPtrStr2.c_str());
      // orkprintf( "FOUND ASSET<%s>\n", the_file.c_str() );

      rval.insert(OutPath);
    }
  }
  // printf("found <%zu> files\n", rval.size());
  return rval;
}

void FileAssetLoader::addLocation(filedevctx_constptr_t b, file_ext_t e) {
  OrkAssert(b);
  FileSet fset;
  fset.mExt      = e;
  fset.mPathBase = b;
  mLocations.push_back(fset);
  if (0) {
    auto loc = b->getFilesystemBaseAbs().c_str();
    printf(
        "FileAssetLoader<%p> added set ext<%s> base<%s>\n", //
        (void*) this,
        fset.mExt.c_str(),
        loc);
  }
}

///////////////////////////////////////////////////////////////////////////////

bool FileAssetLoader::_find(
    const AssetPath& name, //
    AssetPath& result_out,
    int first_extension) {
  //////////////////////////////////////////
  // do we already have an extension
  //////////////////////////////////////////

  file::Path pathobjnoq(name);
  file::Path pathobj(name);
  AssetPath::NameType pathsp, qrysp;
  pathobj.splitQuery(pathsp, qrysp);
  pathobjnoq.set(pathsp.c_str());

  file::Path::NameType preext;
  preext.format(".%s", pathobjnoq.getExtension().c_str());
  bool has_extension       = pathobjnoq.getExtension().length() != 0;
  bool has_valid_extension = false;
  if (has_extension) {
    for (auto l : mLocations) {
      if (0 == strcmp(l.mExt.c_str(), preext.c_str())) {
        has_valid_extension = true;
      }
    }
  }

   //printf("FindAsset<%s> has_valid_extension<%d>\n", ork::Application::AddPooledString(name).c_str(), int(has_valid_extension));

  //////////////////////////////////////////
  // check Munged Paths first (Munged path is a path run thru 1 or more path converters)
  //////////////////////////////////////////

  auto url = pathobjnoq.getUrlBase();

  auto filedevctx = ork::FileEnv::contextForUriProto(url.c_str());

   //printf("filedevctx<%p>\n", filedevctx.get());

    if (has_valid_extension) // path already have an extension ?
    {
      if (FileEnv::DoesFileExist(pathobjnoq)) {
        result_out = pathobjnoq.c_str();
        return true;
      }
    } else // no extension test the registered extensions
    {
      for (auto l : mLocations) {
        pathobjnoq.setExtension(l.mExt.c_str());

        // printf("munged_ext<%s>\n", pathobjnoq.c_str());

        if (FileEnv::DoesFileExist(pathobjnoq)) {
          // pathobj.SetExtension( extension.c_str() );

          result_out = pathobjnoq.c_str();
          return true;
        }
      }
    }

  //////////////////////////////////////////
  // if we got here then munged paths do not exist
  // try the original path
  //////////////////////////////////////////

  AssetPath thename = name;

  if (has_valid_extension) {
    printf("TESTPTH3<%s>\n", pathobjnoq.c_str());
    if (FileEnv::DoesFileExist(pathobjnoq)) {
      result_out = pathobjnoq.c_str();
      printf("PTH3<%s>\n", pathobjnoq.c_str());
      return true;
    }
  } else {
    for (auto l : mLocations) {
      pathobjnoq.setExtension(l.mExt.c_str());
      bool exists = FileEnv::DoesFileExist(pathobjnoq);
      printf("TESTPTH4<%s> exists<%d>\n", pathobjnoq.c_str(), int(exists));
      if (exists) {
        result_out = pathobjnoq.c_str();
        printf("PTH4<%s>\n", pathobjnoq.c_str());
        return true;
      }
    }
  }

  printf("NOTFOUND\n");
  return false;
}

///////////////////////////////////////////////////////////////////////////////

bool FileAssetLoader::doesExist(const AssetPath& name) {
  AssetPath null_result;
  return _find(name, null_result);
}

///////////////////////////////////////////////////////////////////////////////

bool FileAssetLoader::resolvePath(
    const AssetPath& name,      //
    AssetPath& resolved_path) { // override
  return _find(name, resolved_path);
}

///////////////////////////////////////////////////////////////////////////////

asset_ptr_t FileAssetLoader::load(loadrequest_ptr_t loadreq) {
  ///////////////////////////////////////////////////////////////////////////////
  auto orig_path = loadreq->_asset_path;
  ///////////////////////////////////////////////////////////////////////////////
  // resolve extension / search path, etc..
  ///////////////////////////////////////////////////////////////////////////////
  if (not _find(orig_path, loadreq->_asset_path)) {
    printf("Error Loading File Asset %s\n", orig_path.c_str());
    return nullptr;
  }
  ///////////////////////////////////////////////////////////////////////////////
  loadreq->incrementPartialLoadCount();
  auto asset = _doLoadAsset(loadreq);
  if (asset){
    asset->_name = orig_path;
    asset->_load_request = loadreq;
    loadreq->_asset = asset;
  }
  loadreq->decrementPartialLoadCount();
  return asset;
}

///////////////////////////////////////////////////////////////////////////////
} // namespace ork::asset
///////////////////////////////////////////////////////////////////////////////
