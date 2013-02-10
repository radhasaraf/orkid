////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2012, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#include <orktool/orktool_pch.h>
#include <ork/math/plane.h>
#include <orktool/filter/gfx/meshutil/meshutil.h>

///////////////////////////////////////////////////////////////////////////////
namespace ork { namespace MeshUtil {
///////////////////////////////////////////////////////////////////////////////

void submesh::Triangulate( submesh *poutmesh ) const
{
	int inump = GetNumPolys();
	
	for( int ip=0; ip<inump; ip++ )
	{
		const poly & ply = mMergedPolys[ ip ];

		int inumv = ply.GetNumSides();

		switch( inumv )
		{
			case 3:
			{
				int idx0 = ply.miVertices[ 0 ];
				int idx1 = ply.miVertices[ 1 ];
				int idx2 = ply.miVertices[ 2 ];
				const vertex & v0 = mvpool.VertexPool[ idx0 ];
				const vertex & v1 = mvpool.VertexPool[ idx1 ];
				const vertex & v2 = mvpool.VertexPool[ idx2 ];
				int imerged0 = poutmesh->mvpool.MergeVertex( v0 );
				int imerged1 = poutmesh->mvpool.MergeVertex( v1 );
				int imerged2 = poutmesh->mvpool.MergeVertex( v2 );
				poutmesh->MergePoly( poly(imerged0, imerged1, imerged2) );
				break;
			}
			case 4:
			{
				int idx0 = ply.miVertices[ 0 ];
				int idx1 = ply.miVertices[ 1 ];
				int idx2 = ply.miVertices[ 2 ];
				int idx3 = ply.miVertices[ 3 ];
				const vertex & v0 = mvpool.VertexPool[ idx0 ];
				const vertex & v1 = mvpool.VertexPool[ idx1 ];
				const vertex & v2 = mvpool.VertexPool[ idx2 ];
				const vertex & v3 = mvpool.VertexPool[ idx3 ];
				int imerged0 = poutmesh->mvpool.MergeVertex( v0 );
				int imerged1 = poutmesh->mvpool.MergeVertex( v1 );
				int imerged2 = poutmesh->mvpool.MergeVertex( v2 );
				int imerged3 = poutmesh->mvpool.MergeVertex( v3 );
				poutmesh->MergePoly( poly(imerged0, imerged1, imerged2) );
				poutmesh->MergePoly( poly(imerged2, imerged3, imerged0) );
				break;
			}
			default:
				OrkAssert( false );
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

void submesh::TrianglesToQuads( submesh *poutmesh ) const
{
	///////////////////////////////////////

	CPlane P0, P1;
	int ici[6];
	CVector4 VPos[6];

	///////////////////////////////////////

	int inumtri = GetNumPolys(3);

	for ( int ip=0; ip<inumtri; ip++ )
	{
		bool basquad = false;

		const MeshUtil::poly & inpoly = RefPoly( ip );

		ici[0] = inpoly.miVertices[ 0 ];
		ici[1] = inpoly.miVertices[ 1 ];
		ici[2] = inpoly.miVertices[ 2 ];

		VPos[0] = mvpool.VertexPool[ ici[0] ].mPos;
		VPos[1] = mvpool.VertexPool[ ici[1] ].mPos;
		VPos[2] = mvpool.VertexPool[ ici[2] ].mPos;
		P0.CalcPlaneFromTriangle( VPos[0], VPos[1], VPos[2] );
		//CVector4 VArea012[3] = { VPos[0],VPos[1],VPos[2] };
		
		CReal fArea012 = CVector4::CalcTriArea( VPos[0],VPos[1],VPos[2], P0.GetNormal() );

		/*if( 0 != _isnan( fArea012 ) )
		{
			fArea012 = 0.0f;
		}*/

		/////////////////////////
		// get other triangles connected to this via any of its edges
		
		orkset<int> ConnectedPolySet;
		orkset<int> ConnectedPolySetA;
		orkset<int> ConnectedPolySetB;
		orkset<int> ConnectedPolySetC;
		GetConnectedPolys( MeshUtil::edge( ici[0], ici[1] ), ConnectedPolySetA );
		GetConnectedPolys( MeshUtil::edge( ici[1], ici[2] ), ConnectedPolySetB );
		GetConnectedPolys( MeshUtil::edge( ici[2], ici[0] ), ConnectedPolySetC );

		for( orkset<int>::iterator it=ConnectedPolySetA.begin(); it!=ConnectedPolySetA.end(); it++ ) { ConnectedPolySet.insert( *it ); }
		for( orkset<int>::iterator it=ConnectedPolySetB.begin(); it!=ConnectedPolySetB.end(); it++ ) { ConnectedPolySet.insert( *it ); }
		for( orkset<int>::iterator it=ConnectedPolySetC.begin(); it!=ConnectedPolySetC.end(); it++ ) { ConnectedPolySet.insert( *it ); }

		/////////////////////////////////////////////////////////////////////////////
		// for each connected poly, test if it matches our quad critera
		
		for( orkset<int>::iterator it=ConnectedPolySet.begin(); it!=ConnectedPolySet.end(); it++ )
		{
			int iotherpoly = *it;

			const MeshUtil::poly & ply = RefPoly( iotherpoly );

			int inumsides = ply.GetNumSides();

			if( (inumsides==3) && (iotherpoly != ip) ) // if not the same triangle
			{
				////////////////////////////////////////

				IndexTestContext itestctx;

				ici[3] = ply.miVertices[0];
				ici[4] = ply.miVertices[1];
				ici[5] = ply.miVertices[2];

				VPos[3] = mvpool.VertexPool[ ici[3] ].mPos;
				VPos[4] = mvpool.VertexPool[ ici[4] ].mPos;
				VPos[5] = mvpool.VertexPool[ ici[5] ].mPos;

				P1.CalcPlaneFromTriangle( VPos[3], VPos[4], VPos[5] );
				//CVector4 VArea345[3] = { VPos[3],VPos[4],VPos[5] };

				CReal fArea345 = CVector4::CalcTriArea( VPos[3],VPos[4],VPos[5], P1.GetNormal() );

				/*if( 0 != _isnan( fArea345 ) )
				{
					fArea345 = 0.0f;
				}*/

				CReal DelArea = CFloat::Abs(fArea012-fArea345);
				CReal AvgArea = (fArea012+fArea345)*CReal(0.5f);

				if( P0.IsCoPlanar( P1 ) )
				{
					if( (DelArea/AvgArea)<CReal(0.01f) ) // are the areas within 1% of each other ?
					{
						////////////////////////////////////////////
						// count number of unique indices in the 2 triangles, for quads this should be 4

						orkset<int>			TestForQuadIdxSet;
						std::multimap<int,int>	TestForQuadIdxMap;

						for( int it=0; it<6; it++ )
						{
							int idx = ici[it];

							TestForQuadIdxMap.insert( std::make_pair( idx, it ) );
							TestForQuadIdxSet.insert( idx );
						}

						int inumidxinset = (int) TestForQuadIdxSet.size();

						////////////////////////////////////////////

						if( 4 == inumidxinset ) // its a quad with a shared edge
						{
							////////////////////////////////////////////
							// find the shared edges

							int imc = 0;

							for( orkset<int>::iterator it=TestForQuadIdxSet.begin(); it!=TestForQuadIdxSet.end(); it++ )
							{
								static int idx;

								struct testmap : std::binary_function<std::pair<int,int>, IndexTestContext * , bool>
								{
									static bool testmatch( const std::pair<int,int> & pr )
									{
										bool bmatch = (pr.first == idx);
										return bmatch;
									}

									bool operator ()( std::pair<int,int> pr, IndexTestContext * itctx ) const
									{
										bool bmatch = (pr.first == itctx->itest);

										if( bmatch )
										{
											int iset = itctx->iset;
											itctx->PairedIndices[iset].insert( pr.second );
											itctx->PairedIndicesCombined.insert( pr.second );
										}
										return bmatch;
									}
								};

								idx = *it;

								int imatch = (int) std::count_if( TestForQuadIdxMap.begin(), TestForQuadIdxMap.end(), testmap::testmatch );

								if( 2 == imatch )
								{
									OrkAssert( imc<2 );
									itestctx.iset = imc++;
									itestctx.itest = idx;
									int icnt = (int) std::count_if( TestForQuadIdxMap.begin(), TestForQuadIdxMap.end(), bind2nd( testmap(), & itestctx ) );
								}

							}
	
							////////////////////////////////////////////
							// find corner edges (unshared)

							for( int ii=0; ii<6; ii++ )
							{
								if( itestctx.PairedIndicesCombined.find(ii) == itestctx.PairedIndicesCombined.end() )
								{
									itestctx.CornerIndices.insert( ii );
								}
							}
						
							////////////////////////////////////////////
							// test if the quad is rectangular, if so add it

							if( (itestctx.CornerIndices.size()==2) && (itestctx.PairedIndicesCombined.size()==4) )
							{
								orkset<int>::iterator itcorner = itestctx.CornerIndices.begin();
								orkset<int>::iterator itpair0 = itestctx.PairedIndices[0].begin();
								orkset<int>::iterator itpair1 = itestctx.PairedIndices[1].begin();

								int icorner0	= (*itcorner++);
								int ilo0		= (*itpair0++);
								int ihi0		= (*itpair0);

								int icorner1	= (*itcorner);
								int ilo1		= (*itpair1++);
								int ihi1		= (*itpair1);

								// AD are corners

								CVector4 VDelAC = (VPos[icorner0]-VPos[ilo0]).Normal();
								CVector4 VDelAB = (VPos[icorner0]-VPos[ilo1]).Normal();
								CVector4 VDelDC = (VPos[icorner1]-VPos[ilo0]).Normal();
								CVector4 VDelBD = (VPos[ilo1]-VPos[icorner1]).Normal();

								CReal fdotACBD = VDelAC.Dot( VDelBD );		// quad is at least a parallelogram if ang(V02) == ang(V31)
								CReal fdotACAB = VDelAC.Dot( VDelAB );		// quad is rectangular if V01 is perpendicular to V02
								CReal fdotDCBD = VDelDC.Dot( VDelBD );		// quad is rectangular if V01 is perpendicular to V02

								// make sure its a rectangular quad by comparing edge directions

								if( (fdotACBD > CReal(0.999f) ) && (CFloat::Abs(fdotACAB)<CReal(0.02f)) && (CFloat::Abs(fdotDCBD)<CReal(0.02f)) )
								{
									int i0 = ici[ icorner0 ];
									int i1 = ici[ ilo0 ];
									int i2 = ici[ ilo1 ];
									int i3 = ici[ icorner1 ];

									//////////////////////////////////////
									// ensure good winding order

									CPlane P3;
									P3.CalcPlaneFromTriangle( VPos[icorner0], VPos[ilo0], VPos[ilo1] );

									CReal fdot = P3.n.Dot( P0.n );

									//////////////////////////////////////

									if( (CReal(1.0f)-fdot) < CReal(0.001f) )
									{
										poutmesh->MergePoly( MeshUtil::poly( i0, i1, i3, i2 ) );
										basquad = true;
									}
									else if( (CReal(1.0f)+fdot) < CReal(0.001f) )
									{
										poutmesh->MergePoly( MeshUtil::poly( i0, i2, i3, i1 ) );
										basquad = true;
									}
								}
							}
						}
					}
				}
			}

		}	// for( set<int>::iterator it=ConnectedPolySet.begin(); it!=ConnectedPolySet.end(); it++ )
						
		if( false == basquad )
		{
			poutmesh->MergePoly( MeshUtil::poly( ici[0], ici[1], ici[2] ) );
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
}}
///////////////////////////////////////////////////////////////////////////////
