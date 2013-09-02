////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2012, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#include <orktool/orktool_pch.h>

//#include <IL/il.h>
//#include <squish.h>
//#include "nvtt_main.h"
#include <orktool/filter/filter.h>
/*
The only precision issue (regarding S3TC on GC) has to do with the accuracy
of the interpolation factors.  S3TC specifies 1/3 and 2/3.  GC uses 3/8 and 5/8.
This gives a 4% error vs. the spec, or specifically:
*/



///////////////////////////////////////////////////////////////////////////////
namespace ork { namespace tool {
///////////////////////////////////////////////////////////////////////////////
bool NvttCompress( const ork::tool::FilterOptMap& options ) ;

bool Tga2DdsFilterDriver( const tokenlist& toklist )
{
	ork::tool::FilterOptMap	OptionsMap;
	OptionsMap.SetDefault( "-in", "yo.tga" );
	OptionsMap.SetDefault( "-out", "yo.dds" );
	OptionsMap.SetDefault( "-mipfilter", "kaiser" );
	OptionsMap.SetDefault( "-platform", "pc" );
	OptionsMap.SetOptions( toklist );

	//flipy wiis3

	//std::string tex_in = OptionsMap.GetOption( "-in" );
	//std::string tex_out = OptionsMap.GetOption( "-out" );
	//std::string tex_pla = OptionsMap.GetOption( "-out" );
	/////////////////////////////////////////////
	//ork::file::Path InPath( tex_in.c_str() );
	//ork::file::Path OutPath( tex_out.c_str() );
	
	/////////////////////////////////////////////
	OrkAssert(false);
	//return false;
	return NvttCompress( OptionsMap );
	
}

/*
static void setColorToNormalMap(nvtt::InputOptions & inputOptions)
{
	inputOptions.setNormalMap(false);
	inputOptions.setConvertToNormalMap(true);
	inputOptions.setHeightEvaluation(1.0f/3.0f, 1.0f/3.0f, 1.0f/3.0f, 0.0f);
	//inputOptions.setNormalFilter(1.0f, 0, 0, 0);
	//inputOptions.setNormalFilter(0.0f, 0, 0, 1);
	inputOptions.setGamma(1.0f, 1.0f);
	inputOptions.setNormalizeMipmaps(true);
}

///////////////////////////////////////////////////////////////////////////////
// Set options for normal maps.
///////////////////////////////////////////////////////////////////////////////
static void setNormalMap(nvtt::InputOptions & inputOptions)
{
	inputOptions.setNormalMap(true);
	inputOptions.setConvertToNormalMap(false);
	inputOptions.setGamma(1.0f, 1.0f);
	inputOptions.setNormalizeMipmaps(true);
}

///////////////////////////////////////////////////////////////////////////////
// Set options for color maps.
///////////////////////////////////////////////////////////////////////////////
static void setColorMap(nvtt::InputOptions & inputOptions)
{
	inputOptions.setNormalMap(false);
	inputOptions.setConvertToNormalMap(false);
	inputOptions.setGamma(2.2f, 2.2f);
	inputOptions.setNormalizeMipmaps(false);
}

///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////

static inline void rgbToHsv( float r, float g, float b, float& h, float& s, float& v )
{
    //r = r/255, g = g/255, b = b/255;
	float fmax = r; if( g>fmax ) fmax=g; if( b>fmax ) fmax=b;
	//ork::CFloat::Max(r, g, b);
	float fmin = r; if( g<fmin ) fmin=g; if( b<fmin ) fmin=b;
	//float fmin = ork::min(r, g, b);
    h = fmax;
    s = fmax;
    v = fmax;

    float d = fmax - fmin;
    s = fmax == 0 ? 0 : d / fmax;

    if(fmax == fmin){
        h = 0; // achromatic
    }else
	{
		if( fmax == r )
		{
			h = (g - b) / d + (g < b ? 6 : 0);
		}
		else if( fmax == g )
		{
			h = (b - r) / d + 2;
		}
		else if( fmax==b )
		{
            h = (r - g) / d + 4;
        }
        h /= 6;
    }
};

static inline void hsvToRgb(float h, float s, float v, float& r, float& g, float& b)
{
	int i = int(std::floor(h * 6));
    float f = h * 6 - i;
    float p = v * (1 - s);
    float q = v * (1 - f * s);
    float t = v * (1 - (1 - f) * s);

    switch(i){
        case 0: r = v, g = t, b = p; break;
        case 1: r = q, g = v, b = p; break;
        case 2: r = p, g = v, b = t; break;
        case 3: r = p, g = q, b = v; break;
        case 4: r = t, g = p, b = v; break;
        case 5: r = v, g = p, b = q; break;
    }
};

///////////////////////////////////////////////////////////////////////////////
*/
bool NvttCompress( const ork::tool::FilterOptMap& OptionsMap ) 
{
/*	MyAssertHandler assertHandler;
	MyMessageHandler messageHandler;

	std::string tex_in = OptionsMap.GetOption( "-in" )->GetValue();
	std::string tex_out = OptionsMap.GetOption( "-out" )->GetValue();
	file::Path inpath( tex_in.c_str() );
	file::Path outpath( tex_out.c_str() );

	if( strcmp( inpath.GetExtension().c_str(), "dds" ) == 0 )
	{
		const char* frompath = inpath.ToAbsolute().c_str();
		const char* topath = outpath.ToAbsolute().c_str();

		orkprintf( "Copy from<%s> to<%s>\n", frompath, topath );
		BOOL ret = CopyFile( frompath, topath, FALSE );
		return bool(ret);		
	}

	nv::Path InNvPath( inpath.ToAbsolute().c_str() );
	nv::Path OutNvPath( outpath.ToAbsolute().c_str() );

	nvtt::InputOptions inputOptions;
	inputOptions.setAlphaMode( nvtt::AlphaMode_Transparency );

	nv::Image image;
	if (!image.load(InNvPath))
	{
		orkerrorlog("ERROR: The file '%s' is not a supported image type.\n", InNvPath.str());
		return false;
	}

	bool bflipy = OptionsMap.HasOption( "-flipy" );

	std::string platform_str = OptionsMap.GetOption( "-platform" )->GetValue();

	bool bwii = ( 0 == strcmp( platform_str.c_str(), "wii" ) );
	bool bxb360 = ( 0 == strcmp( platform_str.c_str(), "xb360" ) );
	
	bool bnomips = OptionsMap.HasOption( "-nomips" );

	//////////////////////////////////
	//////////////////////////////////
	nvtt::CompressionOptions CompressionOptions;
	CompressionOptions.setFormat(nvtt::Format_DXT1a);
	//compressionOptions.enableHardwareCompression(true);
	CompressionOptions.setQuality(nvtt::Quality_Fastest);
	//////////////////////////////////
	//////////////////////////////////

	if( bflipy )
	{
		int ih = image.height();
		int iw = image.width();

		nv::Color32* c32ary = new nv::Color32[ iw ];

		int ilinesize = iw*sizeof(nv::Color32);

		int imaxy = (ih>>1);

		for( int iy=0; iy<imaxy; iy++ )
		{
			int iothy = ih-(iy+1);

			nv::Color32* scan0 = image.scanline(iy);
			nv::Color32* scan1 = image.scanline(iothy);

			memcpy( (void*) c32ary, (const void *) scan0, ilinesize );
			memcpy( (void*) scan0, (const void *) scan1, ilinesize );
			memcpy( (void*) scan1, (const void *) c32ary, ilinesize );
		}
	}

	//////////////////////////////////
	//////////////////////////////////
	// Wii S3 ?
	// alternate set of interpolation alphas, pre-swizzled(for wii texture cache) and endian-swapped dxt blocks
	// otherwise the same as standard dxt1
	
	if( bwii )
	{
		int ih = image.height();
		int iw = image.width();

		int ilinesize = iw*sizeof(nv::Color32);

		const float fgamma = 1.3f;
		const float fpow = 1.0f / fgamma;

		for( int iy=0; iy<ih; iy++ )
		{
			nv::Color32* scanline = image.scanline(iy);
			for( int ix=0; ix<iw; ix++ )
			{
				nv::Color32& refcolor = scanline[ix];

				uint8 red8 = refcolor.r;
				uint8 grn8 = refcolor.g;
				uint8 blu8 = refcolor.b;
				uint8 alp8 = refcolor.a;

				float redf = float(red8)/255.0f;
				float grnf = float(grn8)/255.0f;
				float bluf = float(blu8)/255.0f;
				float alpf = float(alp8)/255.0f;
				
				float huef, satf, valf;

				rgbToHsv( redf, grnf, bluf, huef, satf, valf );
				satf *= 0.80f;
				hsvToRgb( huef, satf, valf, redf, grnf, bluf );

				redf = std::powf( redf, fpow );
				grnf = std::powf( grnf, fpow );
				bluf = std::powf( bluf, fpow );
				//alpf = alpf;

				red8 = uint8(redf*255.0f);
				grn8 = uint8(grnf*255.0f);
				blu8 = uint8(bluf*255.0f);
				//alp8 = uint8(alpf*255.0f);

				refcolor.r = red8;
				refcolor.g = grn8;
				refcolor.b = blu8;
				//refcolor.a = alp8;
				
			}

			//memcpy( (void*) c32ary, (const void *) scan0, ilinesize );
			//memcpy( (void*) scan0, (const void *) scan1, ilinesize );
			//memcpy( (void*) scan1, (const void *) c32ary, ilinesize );
		}
		CompressionOptions.setFormat(nvtt::Format_WIIS3a);
	}

	//////////////////////////////////
	// check for full alpha (non compressed)
	//////////////////////////////////

	bool b_binary_alpha = true;

	if( image.format() == nv::Image::Format_ARGB )
	{
		int iamin = image.GetAlphaMin( );
		int iamax = image.GetAlphaMax( );
		int iaavg = image.GetAlphaAvg( );
		int iaiavg = image.GetAlphaInteriorAvg( );

		//orkprintf( "INFO: alphas %d %d %d %d\n", iamin, iamax, iaavg, iaiavg );

		if( iaavg==0xffffffff )
		{
			CompressionOptions.setFormat(nvtt::Format_DXT1);
		}
		else if( iaiavg != 0 )
		{
			CompressionOptions.setFormat(nvtt::Format_RGBA);
			//orkprintf( "INFO: non-binary alpha channel found\n" );
			b_binary_alpha = false;
		}
		else
		{
			CompressionOptions.setFormat(nvtt::Format_DXT5);
		}
	}

	if( b_binary_alpha )
	{
		CompressionOptions.setQuantization( true, false, true, 127 );
	}

	//////////////////////////////////
	//////////////////////////////////

	inputOptions.setTextureLayout(nvtt::TextureType_2D, image.width(), image.height());
	inputOptions.setMipmapData(image.pixels(), image.width(), image.height());

	inputOptions.setMipmapFilter( nvtt::MipmapFilter_Kaiser );
	inputOptions.setMipmapGeneration(bnomips==false);

	inputOptions.setRoundMode( nvtt::RoundMode_ToNearestPowerOfTwo );

	//inputOptions.setQuantization( true, false, true, 127 );

	//////////////////////////////////
	// wrap mode (im guessing if the texture is tiled, use Repeat, otherwise use clamp)

	//inputOptions.setWrapMode(nvtt::WrapMode_Repeat); // WrapMode_Clamp
	inputOptions.setWrapMode(nvtt::WrapMode_Repeat); // WrapMode_Clamp

	//if( bwii )
	//{
	//	inputOptions.setGamma( 1.0f, 2.0f );
	//}

	//////////////////////////////////

	setColorMap(inputOptions);

	//////////////////////////////////
	//////////////////////////////////

	MyErrorHandler errorHandler;
	nvtt::OutputOptions OutputOptions;

	OutputOptions.setFileName( OutNvPath );

	//////////////////////////////////
	//////////////////////////////////

	nvtt::Compressor compressor; //  compress(inputOptions, outputOptions, compressionOptions);

	compressor.process( inputOptions, CompressionOptions, OutputOptions );*/
	
	//ilSetInteger(IL_DXTC_FORMAT, IL_DXT5);

	return true;
}


///////////////////////////////////////////////////////////////////////////////
}}
///////////////////////////////////////////////////////////////////////////////
