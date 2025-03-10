////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////

#pragma once

#include <ork/pch.h>

#include <ork/object/ObjectClass.h>
#include <ork/asset/AssetLoader.h>
#include <ork/asset/AssetSet.h>
#include <ork/util/RingLink.hpp>
#include <ork/asset/AssetManager.h>
///////////////////////////////////////////////////////////////////////////////
namespace ork::asset {
///////////////////////////////////////////////////////////////////////////////
template <typename AssetType> ork::recursive_mutex AssetManager<AssetType>::gLock("AssetManagerMutex");
///////////////////////////////////////////////////////////////////////////////
template <typename AssetType>
inline typename AssetManager<AssetType>::typed_asset_ptr_t //
AssetManager<AssetType>::load(loadrequest_ptr_t loadreq) {
  auto loader = getLoader<AssetType>();
  gLock.Lock();
  auto asset = loader->load(loadreq);
  gLock.UnLock();
  return std::dynamic_pointer_cast<AssetType>(asset);
}
///////////////////////////////////////////////////////////////////////////////
template <typename AssetType>
inline typename AssetManager<AssetType>::typed_asset_ptr_t //
AssetManager<AssetType>::load(const AssetPath& pth) {
  auto loader = getLoader<AssetType>();
  gLock.Lock();
  auto loadreq = std::make_shared<LoadRequest>(pth);
  auto asset = loader->load(loadreq);
  gLock.UnLock();
  return std::dynamic_pointer_cast<AssetType>(asset);
}
///////////////////////////////////////////////////////////////////////////////
} // namespace ork::asset
