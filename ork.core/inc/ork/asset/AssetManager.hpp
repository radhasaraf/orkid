///////////////////////////////////////////////////////////////////////////////
// Orkid
// Copyright 1996-2020, Michael T. Mayers
///////////////////////////////////////////////////////////////////////////////
#pragma once

#include <ork/pch.h>

#include <ork/asset/AssetLoader.h>
#include <ork/asset/AssetSet.h>
#include <ork/util/RingLink.hpp>

namespace ork { namespace asset {

template <typename AssetType> ork::recursive_mutex AssetManager<AssetType>::gLock("AssetManagerMutex");

template <typename AssetType> VarMap AssetManager<AssetType>::novars() {
  return VarMap();
};

template <typename AssetType> bool AssetManager<AssetType>::gbAUTOLOAD = true;

template <typename AssetType>
inline typename AssetManager<AssetType>::typed_asset_ptr_t //
AssetManager<AssetType>::Create(const PieceString& asset_name, const VarMap& vmap) {
  gLock.Lock();
  auto asset       = AssetType::GetClassStatic()->DeclareAsset(asset_name, vmap);
  auto typed_asset = std::dynamic_pointer_cast<AssetType>(asset);
  gLock.UnLock();
  return typed_asset;
}

template <typename AssetType>
inline typename AssetManager<AssetType>::typed_asset_ptr_t //
AssetManager<AssetType>::Find(const PieceString& asset_name) {
  gLock.Lock();
  auto asset       = AssetType::GetClassStatic()->FindAsset(asset_name);
  auto typed_asset = std::dynamic_pointer_cast<AssetType>(asset);
  gLock.UnLock();
  return typed_asset;
}

template <typename AssetType>
inline typename AssetManager<AssetType>::typed_asset_ptr_t //
AssetManager<AssetType>::Load(const PieceString& asset_name) {
  gLock.Lock();

  typed_asset_ptr_t asset = Create(asset_name);

  if (asset) {
    if (false == asset->IsLoaded()) {
      asset->Load();
    }
    gLock.UnLock();
    return asset;
  }
  gLock.UnLock();
  return NULL;
}
template <typename AssetType>
inline typename AssetManager<AssetType>::typed_asset_ptr_t //
AssetManager<AssetType>::LoadUnManaged(const PieceString& asset_name) {
  gLock.Lock();
  auto asset       = AssetType::GetClassStatic()->LoadUnManagedAsset(asset_name);
  auto typed_asset = std::dynamic_pointer_cast<AssetType>(asset);
  gLock.UnLock();
  return typed_asset;
}

template <typename AssetType> inline bool AssetManager<AssetType>::AutoLoad(int depth) {
  if (gbAUTOLOAD) {
    gLock.Lock();
    auto asset_set = AssetType::GetClassStatic()->assetSet();
    bool brval     = asset_set->Load(depth);
    gLock.UnLock();
    return brval;
  } else {
    return false;
  }
}

#if defined(ORKCONFIG_ASSET_UNLOAD)
template <typename AssetType> inline bool AssetManager<AssetType>::AutoUnLoad(int depth) {
  auto asset_set = AssetType::GetClassStatic()->assetSet();
  return asset_set->UnLoad(depth);
}
#endif

}} // namespace ork::asset
