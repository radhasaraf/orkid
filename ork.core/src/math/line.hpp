////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2012, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
//////////////////////////////////////////////////////////////// 


#ifndef _ORK_MATH_LINE_HPP
#define _ORK_MATH_LINE_HPP

///////////////////////////////////////////////////////////////////////////////
namespace ork {
///////////////////////////////////////////////////////////////////////////////

template< typename T >
TLineSegment2Helper<T>::TLineSegment2Helper()
{

}
template< typename T >
TLineSegment2Helper<T>::TLineSegment2Helper( const vec2_type& s, const vec2_type& e ) : mStart(s), mEnd(e)
{
   mOrigin = mEnd - mStart;
   mMag = mOrigin.Mag();

}

template< typename T >
void TLineSegment2Helper<T>::SetStartEnd( const vec2_type& s, const vec2_type& e )
{

   mStart.SetX(s.GetX());
   mStart.SetY(s.GetY());
   mEnd.SetX(e.GetX());
   mEnd.SetY(e.GetY());
   mOrigin = mEnd - mStart;
   mMag = mOrigin.Mag();

}

template< typename T >
float TLineSegment2Helper<T>::GetPointDistanceSquared( const vec2_type  &pt ) const
{
	vec2_type pt_origin = pt - mStart;
	T U_num = (pt_origin).Dot(mOrigin);
    T U_den = ( mMag * mMag );

	T U_num2 =  ((pt.GetX() - mStart.GetX()) * (mEnd.GetX() - mStart.GetX()) + ((pt.GetY() - mStart.GetY()) * (mEnd.GetY() - mStart.GetY())));

	if(std::fabs(U_num) < U_den)
	{
		T U = U_num / U_den;
		return(pt_origin - (mOrigin * U)).MagSquared();
	}
	return T(0.0f);
}


template< typename T >
float TLineSegment2Helper<T>::GetPointDistancePercent( const vec2_type  &pt ) const
{

	/*
	 float U_num = ( ( ( pt.GetX() - mStart.GetX() ) * ( mEnd.GetX() - mStart.GetX() ) ) +
        ( ( pt.GetY() - mStart.GetY() ) * ( mEnd.GetY() - mStart.GetY() ))) ;

	*/
	vec2_type pt_origin = pt - mStart;
	T U_num = (pt_origin).Dot(mOrigin);
    T U_den = ( mMag * mMag );
	if(std::fabs(U_num) < U_den)
	{
		return(U_num / U_den);
	}
	return T(0.0f);
}

///////////////////////////////////////////////////////////////////////////////
}
///////////////////////////////////////////////////////////////////////////////

#endif
