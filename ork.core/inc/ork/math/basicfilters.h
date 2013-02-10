////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2012, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
//////////////////////////////////////////////////////////////// 


#ifndef _ORK_MATH_BASICFILTERS_H
#define _ORK_MATH_BASICFILTERS_H

///////////////////////////////////////////////////////////////////////////////
namespace ork {
///////////////////////////////////////////////////////////////////////////////

template <int stages>
struct avg_filter
{
	float fsamples[stages];
	float fdtsamples[stages];
	float mfWindow;
	bool mbEnable;
	//////////////////////////////////////////////////////////////////////////////
	inline float compute( float fin, float fdt=1.0f )
	{	float foutacc = fin;
		float fdtacc = 1.0f;
		//////////////////////////////
		// make sure we have enough samples for the window
		// if this assert hits, then increase stages
		float frate = 1.0f / fdt;
		float freqsamps = frate*mfWindow;
		//if(freqsamps < float(stages) return(1.0f);
		//OrkAssert( freqsamps < float(stages)  ); 
		//////////////////////////////
			
		if( mbEnable )
		{	int icount = (stages-1);
			//////////////////////////////
			// rotate loop (ya, we could use the circular technique, but we are iterating anyway)
			//////////////////////////////
			for( int i=0; i<icount; i++ )
			{	fsamples[i] = fsamples[i+1];
				fdtsamples[i] = fdtsamples[i+1];
			}
			//////////////////////////////////////////
			// record new samples
			//////////////////////////////////////////
			fsamples[stages-1] = fin;
			fdtsamples[stages-1] = fdt;
			//////////////////////////////
			// init accumulators
			//////////////////////////////
			fdtacc = fdt;
			foutacc = fin*fdt;
			//////////////////////////////
			// accumulation loop
			//////////////////////////////
			for( int i=icount; i>=0; i-- )
			{	if( fdtacc < mfWindow ) // window check
				{	float fdti = fdtsamples[i];
					float fsi = fsamples[i];
					//////////////////////////////////
					// fractional windowing 
					//////////////////////////////////
					if( (fdtacc+fdti) > mfWindow )
					{
						fdti = mfWindow - fdtacc;
					}
					//////////////////////////////////
					// accumulate
					//////////////////////////////////
					fdtacc += fdti;
					foutacc += fsi*fdti;
				}
			}
		}
		//////////////////////////////////////////
		// constraints
		//////////////////////////////////////////
		if( fdtacc<=0.0f )
		{
			foutacc = 0.0f;
			fdtacc = 1.0f;
		}
		//////////////////////////////////////////
		// final output
		//////////////////////////////////////////
		float fout = foutacc/fdtacc;
		return fout;
	}
	//////////////////////////////////////////////////////////////////////////////
	void SetWindow( float ftime )
	{
		mfWindow = ftime;
	}
	//////////////////////////////////////////////////////////////////////////////
	avg_filter()
		: mbEnable( true )
		, mfWindow( float(stages) )
	{
		for( int i=0; i<stages; i++ )
		{
			fsamples[i] = 0.0f;
			fdtsamples[i] = 0.0f;
		}
	}
};

///////////////////////////////////////////////////////////////////////////////
}
///////////////////////////////////////////////////////////////////////////////

#endif
