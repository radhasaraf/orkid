////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
//////////////////////////////////////////////////////////////// 

#pragma once

///////////////////////////////////////////////////////////////////////////////

#include <ork/kernel/string/PoolString.h>

#include <ork/orkstl.h>
#include <ork/kernel/mutex.h>

///////////////////////////////////////////////////////////////////////////////
namespace ork {
///////////////////////////////////////////////////////////////////////////////

class StringPool
{
public:
	typedef orkvector<const char * > VecType;

	explicit StringPool(const StringPool *parent = NULL);
	PoolString String(const PieceString &);
	PoolString Literal(const ConstString &);
	PoolString Find(const PieceString &) const;

	int LiteralIndex(const ConstString &);
	int StringIndex(const PieceString &);
	int FindIndex(const PieceString &) const;
	int Size() const;
	PoolString FromIndex(int) const;
private:
	const char *FindRecursive(const PieceString &) const;
	const char *FindFirst(const PieceString &, VecType::iterator &);
	VecType::size_type BinarySearch(const PieceString &, bool &) const;

	VecType										mStringPool;
	mutable ork::recursive_mutex				mStringPoolMutex;

	void Lock() const { mStringPoolMutex.Lock(); }
	void UnLock() const { mStringPoolMutex.UnLock(); }

protected:
	const StringPool *_parent;
};

///////////////////////////////////////////////////////////////////////////////
}
///////////////////////////////////////////////////////////////////////////////
