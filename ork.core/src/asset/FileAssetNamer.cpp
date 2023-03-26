////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
//////////////////////////////////////////////////////////////// 


#include <ork/pch.h>
#include <ork/asset/FileAssetNamer.h>

///////////////////////////////////////////////////////////////////////////////
namespace ork { namespace asset {
///////////////////////////////////////////////////////////////////////////////

FileAssetNamer::FileAssetNamer(ConstString prefix)
	: mPrefix(prefix)
{
}

///////////////////////////////////////////////////////////////////////////////

bool FileAssetNamer::Canonicalize(MutableString result, PieceString input)
{
	// TODO: Path canonicalization here.

	result = "";

	if(input.find("://") == PieceString::npos)
	{
		result = "data://";
		result += PieceString(mPrefix);
	}

	result += input;

	return true;
}

///////////////////////////////////////////////////////////////////////////////

} }
