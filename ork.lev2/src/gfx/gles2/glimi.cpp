////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2012, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#include <ork/pch.h>
#if defined( ORK_CONFIG_OPENGL )
#include <ork/lev2/gfx/gfxenv.h>
#include <ork/lev2/gfx/gfxmaterial.h>
#include "gl.h"

#include <ork/lev2/ui/ui.h>

namespace ork { namespace lev2 {

GlImiInterface::GlImiInterface( GfxTargetGL& target )
	: ImmInterface( target )
{
}

///////////////////////////////////////////////////////////////////////////////

void GlImiInterface::DrawPrim( const CVector4 *Points, int inumpoints, EPrimitiveType eType )
{
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	int inumpasses = mTarget.GetCurMaterial()->BeginBlock(&mTarget);

	for( int ipass=0; ipass<inumpasses; ipass++ )
	{
		bool bDRAW = mTarget.GetCurMaterial()->BeginPass( &mTarget,ipass );

		if( bDRAW )
		{
			switch( eType )
			{
				case EPRIM_QUADS:
				{
					/*glBegin( GL_QUADS );
					for( int itri=0; itri<inumpoints; itri++ )
					{
						int ibase = itri*4;
						glVertex3fv( (float*)Points[ibase+0].GetArray() );
						glVertex3fv( (float*)Points[ibase+1].GetArray() );
						glVertex3fv( (float*)Points[ibase+2].GetArray() );
						glVertex3fv( (float*)Points[ibase+3].GetArray() );

					}
					glEnd();*/
					break;
				}

				case EPRIM_TRIANGLES:
				{
					/*glBegin( GL_TRIANGLES );
					for( int itri=0; itri<inumpoints; itri++ )
					{
						int ibase = itri*3;
						glVertex3fv( (float*)Points[ibase+0].GetArray() );
						glVertex3fv( (float*)Points[ibase+1].GetArray() );
						glVertex3fv( (float*)Points[ibase+2].GetArray() );

					}
					glEnd();
*/
					break;
				}
			}
		}
	}
}

void GlImiInterface::DrawLine( const CVector4 &From, const CVector4 &To )
{
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	int inumpasses = mTarget.GetCurMaterial()->BeginBlock(&mTarget);

	for( int ipass=0; ipass<inumpasses; ipass++ )
	{
		bool bDRAW = mTarget.GetCurMaterial()->BeginPass( &mTarget,ipass );

		if( bDRAW )
		{
			//glBegin( GL_LINES );
			//	glVertex3fv( (float*)const_cast<CVector4&>(From).GetArray() );
			//	glVertex3fv( (float*)const_cast<CVector4&>(To).GetArray() );
			//glEnd();
		}
		mTarget.GetCurMaterial()->EndPass(&mTarget);

	}

	mTarget.GetCurMaterial()->EndBlock(&mTarget);
}

void GlImiInterface::DrawPoint( F32 fx, F32 fy, F32 fz )
{
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	int inumpasses = mTarget.GetCurMaterial()->BeginBlock(&mTarget);

	for( int ipass=0; ipass<inumpasses; ipass++ )
	{
		bool bDRAW = mTarget.GetCurMaterial()->BeginPass( &mTarget, ipass );

		if( bDRAW )
		{
			//glPointSizef( 3.0f );
			//glBegin( GL_POINTS );
			//	glVertex3f( fx, fy, fz );
			//glEnd();
		}

		mTarget.GetCurMaterial()->EndPass(&mTarget);
	}

	mTarget.GetCurMaterial()->EndBlock(&mTarget);
}

} }

#endif
