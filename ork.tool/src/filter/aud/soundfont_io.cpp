////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2012, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#include <orktool/orktool_pch.h>

#include <ork/math/audiomath.h>
#include <ork/file/riff.h>
#include "soundfont.h"
#include <ork/lev2/aud/audiobank.h>
#if defined(_USE_SOUNDFONT)
////////////////////////////////////////////////////////////////////////////////

namespace ork { namespace tool {

U32 CSoundFontConversionEngine::GetSBFK( CRIFFChunk *pROOT )
{

	U32 rval =0;
	CRIFFChunk *nchnk = new CRIFFChunk;
	nchnk->chunkID = pROOT->GetU32( 0 );
	U8* chunkdata = pROOT->GetChunkData();
	nchnk->chunklen = CRIFFChunk::GetChunkLen( chunkdata[1] );
	nchnk->chunkdata = & pROOT->chunkdata[4];
	nchnk->DumpHeader();
	//children.push_back( nchnk );
	
	mDynamicChunks.push_back( nchnk );

	U32 offs = 0;
	U32 len = 0;
	
	len = GetINFOList( nchnk, 0 );
	len = GetSDTAList( nchnk, len );
	len = GetPDTAList( nchnk, len );
	
	return rval;
}

////////////////////////////////////////////////////////////////////////////////

U32 CSoundFontConversionEngine::GetINFOChunk( CRIFFChunk *ParChunk, U32 offset )
{
	//orkmessageh( "get_chunk( 0x%x )\n", offset );
	U32 rval =0;
	U8 *parchkdata = ParChunk->GetChunkData();
	CRIFFChunk *nchnk = new CRIFFChunk;
	nchnk->chunkID = ParChunk->GetU32( offset );
	nchnk->chunklen = ParChunk->GetU32( offset+4 );
	nchnk->chunkdata = & parchkdata[ offset+8 ];
	//children.push_back( nchnk );
	rval = 8+nchnk->chunklen;

	mDynamicChunks.push_back( nchnk );


#if 0
	orkmessageh( "//////////////////////////\n" );
	if( nchnk->chunkID == CRIFFChunk::ChunkName( 'i', 'f', 'i', 'l' ) )
	{	orkmessageh( "ifil chunk found!\n" );
		U32 version = nchnk->GetU32( 0 );
		orkmessageh( "SoundFont Version: 0x%08x\n", version );
	}	
	else if( nchnk->chunkID == CRIFFChunk::ChunkName( 'i', 's', 'n', 'g' ) )
	{	orkmessageh( "isng chunk found!\n" );
		orkmessageh( "SoundEngine: %s\n", nchnk->chunkdata );
	}	
	else if( nchnk->chunkID == CRIFFChunk::ChunkName( 'I', 'N', 'A', 'M' ) )
	{	orkmessageh( "INAM chunk found!\n" );
		orkmessageh( "BANKName: %s\n", nchnk->chunkdata );
	}	
	else if( nchnk->chunkID == CRIFFChunk::ChunkName( 'I', 'S', 'F', 'T' ) )
	{	orkmessageh( "ISFT chunk found!\n" );
		orkmessageh( "Application: %s\n", nchnk->chunkdata );
	}	
	else if( nchnk->chunkID == CRIFFChunk::ChunkName( 'I', 'E', 'N', 'G' ) )
	{	orkmessageh( "IENG chunk found!\n" );
		orkmessageh( "Creator: %s\n", nchnk->chunkdata );
	}	
	else if( nchnk->chunkID == CRIFFChunk::ChunkName( 'I', 'C', 'O', 'P' ) )
	{	orkmessageh( "ICOP chunk found!\n" );
		orkmessageh( "Copyright: %s\n", nchnk->chunkdata );
	}	
	else if( nchnk->chunkID == CRIFFChunk::ChunkName( 'I', 'C', 'M', 'T' ) )
	{	orkmessageh( "ICMT chunk found!\n" );
		orkmessageh( "Comment: %s\n", nchnk->chunkdata );
	}	
	else
	{	orkmessageh( "UNKNOWN CHUNK!\n" );
		nchnk->DumpHeader();
	}
	orkmessageh( "//////////////////////////\n" );
#endif

	return rval;
}


////////////////////////////////////////////////////////////////////////////////

U32 CSoundFontConversionEngine::GetINFOList( CRIFFChunk* ParChunk, U32 offset )
{
	orkmessageh( "get_info_list()\n" );
	U32 rval =0;
	U8 *parchkdata = ParChunk->GetChunkData();
	CRIFFChunk *nchnk = new CRIFFChunk;
	nchnk->chunkID = ParChunk->GetU32( offset );
	nchnk->subID = ParChunk->GetU32( offset+8 );
	nchnk->chunklen = CRIFFChunk::GetChunkLen( ParChunk->GetU32( offset+4 ) );
	nchnk->chunkdata = & parchkdata[(3*4)];
	nchnk->DumpListHeader();

	mDynamicChunks.push_back( nchnk );

	list_info = nchnk;
	
	//children.push_back( nchnk );
	rval = 0;
	
	U32 i = 0;
	while( rval < (nchnk->chunklen-8) )
	{	rval += GetINFOChunk( nchnk, rval);
	}
	return (rval+12);
}

////////////////////////////////////////////////////////////////////////////////

U32 CSoundFontConversionEngine::GetSDTAChunk( CRIFFChunk *ParChunk, U32 offset )
{
	//orkmessageh( "get_chunk( 0x%x )\n", offset );
	U32 rval =0;
	U8 *parchkdata = ParChunk->GetChunkData();
	CRIFFChunk *nchnk = new CRIFFChunk;
	nchnk->chunkID = ParChunk->GetU32( offset );
	nchnk->chunklen = ParChunk->GetU32( offset+4 );
	nchnk->chunkdata = & parchkdata[ offset+8 ];
	//children.push_back( nchnk );
	rval = 8+nchnk->chunklen;

	mDynamicChunks.push_back( nchnk );

	orkmessageh( "//////////////////////////\n" );
	if( nchnk->chunkID == CRIFFChunk::ChunkName( 's', 'm', 'p', 'l' ) )
	{	orkmessageh( "smpl chunk found! (offset: 0x%x len: %d\n", offset, nchnk->chunklen );
		sampledata = (S16 *) & nchnk->chunkdata[ 2 ];
		int *pbdlen = const_cast<int*>(& misampleblockdatalen);
		*pbdlen = nchnk->chunklen;

	}	
	else
	{	orkmessageh( "//////////////////////////\n" );
		orkmessageh( "UNKNOWN CHUNK at offset 0x%x!\n", offset );
		nchnk->DumpHeader();
		orkmessageh( "//////////////////////////\n" );
	}

	return rval;
}

////////////////////////////////////////////////////////////////////////////////

U32 CSoundFontConversionEngine::GetSDTAList( CRIFFChunk* ParChunk, U32 offset )
{
	orkmessageh( "get_sdta_list( 0x%x )\n", offset );
	U32 rval =0;
	U8 *parchkdata = ParChunk->GetChunkData();

	CRIFFChunk *nchnk = new CRIFFChunk;
	nchnk->chunkID = ParChunk->GetU32( offset );
	nchnk->subID = ParChunk->GetU32( offset+8 );
	nchnk->chunklen = ParChunk->GetU32( offset+4 );
	nchnk->chunkdata = & parchkdata[(3*4)];
	nchnk->DumpListHeader();

	mDynamicChunks.push_back( nchnk );

	list_sdta = nchnk;

	sampledata = (S16 *) nchnk->chunkdata;

	rval = offset;
	
	U32 i = 0;
	while( rval < (nchnk->chunklen-8) )
	{	rval += GetSDTAChunk( nchnk, rval ); //->get_sdta_chunk(rval);
	}
	return rval+12;
}

////////////////////////////////////////////////////////////////////////////////

U32 CSoundFontConversionEngine::GetPDTAList( CRIFFChunk* ParChunk, U32 offset )
{
	orkmessageh( "get_pdta_list( 0x%x )\n", offset );
	U32 rval =0;
	U8 *parchkdata = ParChunk->GetChunkData();
	CRIFFChunk *nchnk = new CRIFFChunk;
	nchnk->chunkID = ParChunk->GetU32( offset );
	nchnk->subID = ParChunk->GetU32( offset+8 );
	nchnk->chunklen = CRIFFChunk::GetChunkLen( ParChunk->GetU32( offset+4 ) );
	nchnk->chunkdata = & parchkdata[(3*4)];
	nchnk->DumpListHeader();
	
	mDynamicChunks.push_back( nchnk );

	list_sdta = nchnk;
	
	rval = offset;
	
	U32 i = 0;
	orkmessageh( "rval: %d clm8: %d\n", (rval-offset), (nchnk->chunklen-8) );
	while( (rval-offset) < (nchnk->chunklen-8) )
	{	rval += GetPDTAChunk( nchnk, rval );
	}
	
	//post_process();
	
	return rval+12;
}

////////////////////////////////////////////////////////////////////////////////

U32 CSoundFontConversionEngine::GetPDTAChunk( CRIFFChunk* ParChunk, U32 offset )
{
	orkmessageh( "get_chunk( 0x%x )\n", offset );
	U32 rval =0;
	U8 *parchkdata = ParChunk->GetChunkData();

	CRIFFChunk *nchnk = new CRIFFChunk;
	nchnk->chunkID = ParChunk->GetU32( offset );
	nchnk->chunklen = CRIFFChunk::GetChunkLen( ParChunk->GetU32( offset+4 ) );
	nchnk->chunkdata = & parchkdata[ offset+8 ];
	//children.push_back( nchnk );
	rval = 8+nchnk->chunklen;

	mDynamicChunks.push_back( nchnk );

	orkmessageh( "//////////////////////////\n" );
	if( nchnk->chunkID == CRIFFChunk::ChunkName( 'p', 'h', 'd', 'r' ) )
	{	orkmessageh( "phdr chunk found!\n" );
		U32 sizofpre = sizeof( Ssfontpreset );	
		U32 numpresets = (nchnk->chunklen / 38)-1;
		orkmessageh( "chunklen: %d sizofpre: %d numpresets: %d\n", nchnk->chunklen, sizofpre, numpresets );

		for( U32 i=0; i<numpresets; i++ )
		{
			U32 pnum = 0xffffffff;
		
			Ssfontpreset *preset = (Ssfontpreset *) & nchnk->chunkdata[ (38*i) ];
			//SF2Presets.push_back( preset );

			AddProgram( preset );
		}

	}
	else if( nchnk->chunkID == CRIFFChunk::ChunkName( 's', 'h', 'd', 'r' ) )
	{	orkmessageh( "//////////////////////////\n" );
		orkmessageh( "shdr chunk found!\n" );
		sizofsmp = sizeof( Ssfontsample );	
		numsamples = (nchnk->chunklen / 46)-1;
		orkmessageh( "chunklen: %d sizofsmp: %d numsamples: %d\n", nchnk->chunklen, sizofsmp, numsamples );

		for( U32 i=0; i<U32(numsamples); i++ )
		{
			Ssfontsample *osample = (Ssfontsample *) & nchnk->chunkdata[ (46*i) ];
			//SF2Samples.push_back( osample );
			AddSample( osample );
		}
	}
	else if( nchnk->chunkID == CRIFFChunk::ChunkName( 'i', 'g', 'e', 'n' ) )
	{	orkmessageh( "//////////////////////////\n" );
		orkmessageh( "igen chunk found!\n" );
		U32 sizofign = sizeof( SSoundFontGenerator );	
		U32 numinstgens = (nchnk->chunklen / 4)-1;
		orkmessageh( "chunklen: %d sizofign: %d numinstgens: %d\n", nchnk->chunklen, sizofign, numinstgens );

		for( U32 i=0; i<numinstgens; i++ )
		{
			SSoundFontGenerator *ign = (SSoundFontGenerator *) & nchnk->chunkdata[ (4*i) ];
			//SF2InstGens.push_back( (SSoundFontGenerator*)ign );
			AddInstrumentGen( ign );
			
		}
	}
	else if( nchnk->chunkID == CRIFFChunk::ChunkName( 'p', 'g', 'e', 'n' ) )
	{	orkmessageh( "//////////////////////////\n" );
		orkmessageh( "pgen chunk found!\n" );
		U32 sizofpgn = sizeof( SSoundFontGenerator );	
		U32 numpregens = (nchnk->chunklen / 4)-1;
		orkmessageh( "chunklen: %d sizofpgn: %d numpregens: %d\n", nchnk->chunklen, sizofpgn, numpregens );

		for( U32 i=0; i<numpregens; i++ )
		{	SSoundFontGenerator *pgn = (SSoundFontGenerator *) & nchnk->chunkdata[ (4*i) ];
			//SF2PreGens.push_back( (SSoundFontGenerator*)pgn );
			//orkmessageh( "ign: %d	genID: 0x%x	genVAL: 0x%x\n", i, ign->gen_ID, ign->value );
			AddPresetGen( pgn );
		}
	}
	else if( nchnk->chunkID == CRIFFChunk::ChunkName( 'i', 'b', 'a', 'g' ) )
	{	orkmessageh( "//////////////////////////\n" );
		orkmessageh( "ibag chunk found!\n" );
		U32 sizofibg = sizeof( Ssfontinstbag );	
		U32 numinstbags = (nchnk->chunklen / 4)-1;
		orkmessageh( "chunklen: %d sizofibg: %d numinstbags: %d\n", nchnk->chunklen, sizofibg, numinstbags );

		for( U32 i=0; i<numinstbags; i++ )
		{
			Ssfontinstbag *ibg = (Ssfontinstbag *) & nchnk->chunkdata[ (4*i) ];
			//SF2InstBags.push_back( ibg );

			//orkmessageh( "InstBag [%d] [ibagndx %d] [imodndx %d]\n", i, ibg->wInstGenNdx, ibg->wInstModNdx );

			AddInstrumentZone( ibg );

		}
	}
	else if( nchnk->chunkID == CRIFFChunk::ChunkName( 'i', 'n', 's', 't' ) )
	{	orkmessageh( "//////////////////////////\n" );
		orkmessageh( "inst chunk found!\n" );
		U32 sizofinst = sizeof( Ssfontinst );	
		U32 numinsts = (nchnk->chunklen / 22)-1;
		orkmessageh( "chunklen: %d sizofinst: %d numinsts: %d\n", nchnk->chunklen, sizofinst, numinsts );

		for( U32 i=0; i<numinsts; i++ )
		{
			U32 j=i+1;
			Ssfontinst *inst = (Ssfontinst *) & nchnk->chunkdata[ (22*i) ];
			//SF2Instruments.push_back( inst );

			//orkmessageh( "Instrument [%d] %s :[InstGenBase %d]\n", i, inst->achInstName, inst->wInstBagNdx );
			
			AddInstrument( inst );

		}
	}
	else if( nchnk->chunkID == CRIFFChunk::ChunkName( 'p', 'b', 'a', 'g' ) )
	{	orkmessageh( "//////////////////////////\n" );
		orkmessageh( "pbag chunk found!\n" );
		U32 sizofpbg = sizeof( Ssfontprebag );	
		numprebags = (nchnk->chunklen / 4)-1;
		orkmessageh( "chunklen: %d sizofpbg: %d numprebags: %d\n", nchnk->chunklen, sizofpbg, numprebags );

		for( U32 i=0; i<numprebags; i++ )
		{
			Ssfontprebag *pbg = (Ssfontprebag *) & nchnk->chunkdata[ (4*i) ];
			//SF2PreBags.push_back( pbg );
			//orkmessageh( "ibg: %d	genidx: 0x%x	modidx: 0x%x\n", i, ibg->wInstGenNdx, ibg->wInstModNdx );
			
			AddProgramZone( pbg );
			
		}
	}

	else
	{	orkmessageh( "UNKNOWN CHUNK at offset 0x%x!\n", offset );
		nchnk->DumpHeader();
	}
	orkmessageh( "//////////////////////////\n" );
	
	return rval;
}

} }
#endif
