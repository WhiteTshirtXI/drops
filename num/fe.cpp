/// \file fe.cpp
/// \brief Description of various finite-element functions
/// \author LNM RWTH Aachen: Joer Grande, Sven Gross, Volker Reichelt ; SC RWTH Aachen:

/*
 * This file is part of DROPS.
 *
 * DROPS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * DROPS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with DROPS. If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * Copyright 2009 LNM/SC RWTH Aachen, Germany
*/

#include "num/fe.h"

namespace DROPS
{

const double FE_P1CL::_gradient[4][3]=
    { {-1., -1., -1.}, {1., 0., 0.}, {0., 1., 0.}, {0., 0., 1.} };

const double FE_P1DCL::_gradient[4][3]=
    { {3., 3., 3.}, {-3., 0., 0.}, {0., -3., 0.}, {0., 0., -3.} };

const double FE_P1DCL::_vertexvalue[4][4]=
    { {-2., 1., 1., 1.}, {1., -2., 1., 1.}, {1., 1., -2., 1.}, {1., 1., 1., -2.} };

const double FE_P2CL::_D2H[10][3][3]= {
    { {4., 4., 4.}, {4., 4., 4.}, {4., 4., 4.} },
    { {4., 0., 0.}, {0., 0., 0.}, {0., 0., 0.} },
    { {0., 0., 0.}, {0., 4., 0.}, {0., 0., 0.} },
    { {0., 0., 0.}, {0., 0., 0.}, {0., 0., 4.} },
    { {-8., -4., -4.}, {-4., 0., 0.}, {-4., 0., 0.} },
    { {0., -4., 0.}, {-4., -8., -4.}, {0., -4., 0.} },
    { {0., 4., 0.}, {4., 0., 0.}, {0., 0., 0.} },
    { {0., 0., -4.}, {0., 0., -4.}, {-4., -4., -8.} },
    { {0., 0., 4.}, {0., 0., 0.}, {4., 0., 0.} },
    { {0., 0., 0.}, {0., 0., 4.}, {0., 4., 0.} } };

void
FE_P2CL::ApplyAll(Uint numpt, const BaryCoordCL* const pt, std::valarray<double>* v)
{
    for (Uint i= 0; i < 10; ++i)
        v[i].resize( numpt);
    for (Uint i= 0; i < numpt; ++i) {
        v[0][i]= H0( pt[i]);
        v[1][i]= H1( pt[i]);
        v[2][i]= H2( pt[i]);
        v[3][i]= H3( pt[i]);
        v[4][i]= H4( pt[i]);
        v[5][i]= H5( pt[i]);
        v[6][i]= H6( pt[i]);
        v[7][i]= H7( pt[i]);
        v[8][i]= H8( pt[i]);
        v[9][i]= H9( pt[i]);
    }
}


//**************************************************************************
// P1-Prolongation                                                         *
//**************************************************************************
void SetupP1ProlongationMatrix(const MultiGridCL& mg, MatrixCL& P,
                               IdxDescCL& cIdx, IdxDescCL& fIdx)
{
    const Uint c_level= cIdx.TriangLevel();
    const Uint c_idx= cIdx.GetIdx();
    const Uint f_idx= fIdx.GetIdx();
    MatrixBuilderCL mat( &P, fIdx.NumUnknowns(), cIdx.NumUnknowns());
    IdxT i;

    // setup index part of matrix
    // Iterate over all edges, interpolate values on new mid vertices
    for (MultiGridCL::const_EdgeIterator sit= mg.GetAllEdgeBegin( c_level),
         theend= mg.GetAllEdgeEnd( c_level); sit!=theend; ++sit)
        if ( sit->IsRefined() && sit->GetMidVertex()->Unknowns.Exist()
             && !sit->GetMidVertex()->Unknowns.Exist(c_idx) )  {
            // only new non-boundary vertices are interpolated
// if(!(*sit->GetMidVertex()->Unknowns.Exist(idx)) std::cout << "no unknown index in mid vertex" << std::endl;
            i= sit->GetMidVertex()->Unknowns(f_idx);
            if (sit->GetVertex(0)->Unknowns.Exist())
                mat(i,sit->GetVertex(0)->Unknowns(c_idx))= 0.5;
            if (sit->GetVertex(1)->Unknowns.Exist())
                mat(i,sit->GetVertex(1)->Unknowns(c_idx))= 0.5;
        }
    // Iterate over the vertices of the coarse triangulation and copy values
    for (MultiGridCL::const_TriangVertexIteratorCL sit= mg.GetTriangVertexBegin( c_level),
         theend= mg.GetTriangVertexEnd( c_level); sit!=theend; ++sit)
        if ( sit->Unknowns.Exist() && sit->Unknowns.Exist(c_idx) ) {
            mat( sit->Unknowns(f_idx), sit->Unknowns(c_idx) )= 1.0;
        }

    mat.Build();
}

void SetupP1ProlongationMatrix(const MultiGridCL& mg, MLMatrixCL& P, MLIdxDescCL* ColIdx, MLIdxDescCL* RowIdx)
{
    P.resize( ColIdx->size());
    MLMatrixCL::iterator itProlong = ++P.begin();
    MLIdxDescCL::iterator itcIdx = ColIdx->begin();
    MLIdxDescCL::iterator itfIdx = ++RowIdx->begin();
    for (size_t lvl=1; lvl < P.size(); ++lvl, ++itProlong, ++itcIdx, ++itfIdx)
        SetupP1ProlongationMatrix( mg, *itProlong, *itcIdx, *itfIdx);
}

void SetupP1ProlongationMatrix(const MultiGridCL& mg, MLMatDescCL& P)
{
    SetupP1ProlongationMatrix( mg, P.Data, P.ColIdx, P.RowIdx);
}

/// \name Coefficient-tables for P2-prolongation.
/// For each refinement rule the local P2 prolongation matrix is stored
/// as a sequence of matrix rows in P2_local_prolongation_row_idx.
/// Beginning and one-past-the-end of each sequence are stored in
/// P2_local_prolongation_mat_beg.
/// Conceptually, rows of the local prolongation matrices are represented
/// by an index into an array of the 35 possible different rows.
/// Considered as a matrix, this array is sparse. Consequently, it is
/// stored in compressed row storage format.
/// \{
const unsigned int P2_local_prolongation_mat_beg[65]= {
    0, 10, 24, 38, 56, 70, 88, 106,
    128, 142, 160, 178, 200, 219, 241, 263,
    289, 303, 321, 340, 362, 380, 402, 425,
    451, 469, 491, 514, 541, 564, 591, 618,
    649, 663, 682, 700, 723, 741, 764, 786,
    812, 830, 853, 875, 902, 925, 952, 979,
    1010, 1028, 1051, 1074, 1101, 1123, 1150, 1177,
    1208, 1230, 1256, 1283, 1314, 1341, 1372, 1403,
    1438
};

const unsigned char P2_local_prolongation_rows[1438]= {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
    12, 13, 0, 1, 2, 3, 5, 4, 6, 7, 8, 9, 14, 15, 16, 17, 0, 1, 2, 3, 4, 5, 6, 7,
    8, 9, 10, 11, 14, 15, 16, 12, 13, 18, 0, 1, 2, 3, 6, 4, 5, 7, 8, 9, 19, 20, 21,
    18, 0, 1, 2, 3, 4, 6, 5, 7, 8, 9, 10, 11, 19, 20, 21, 12, 13, 17, 0, 1, 2, 3,
    5, 6, 4, 7, 8, 9, 14, 15, 19, 20, 21, 16, 17, 13, 0, 1, 2, 3, 4, 5, 6, 7, 8,
    9, 10, 11, 14, 15, 19, 20, 21, 16, 12, 18, 17, 13, 0, 1, 2, 3, 7, 4, 5, 6, 8,
    9, 22, 23, 24, 25, 0, 1, 2, 3, 4, 7, 5, 6, 8, 9, 10, 11, 22, 23, 24, 12, 13,
    26, 0, 1, 2, 3, 5, 7, 4, 6, 8, 9, 14, 15, 22, 23, 16, 25, 17, 27, 0, 1, 2, 3,
    4, 5, 7, 6, 8, 9, 10, 11, 14, 15, 22, 23, 16, 12, 13, 27, 26, 18, 0, 1, 2, 3,
    6, 7, 4, 5, 8, 9, 19, 20, 22, 23, 21, 24, 25, 18, 28, 0, 1, 2, 3, 4, 6, 7, 5,
    8, 9, 10, 11, 19, 20, 22, 23, 21, 24, 12, 13, 26, 17, 0, 1, 2, 3, 5, 6, 7, 4,
    8, 9, 14, 15, 19, 20, 22, 23, 21, 16, 25, 17, 27, 13, 0, 1, 2, 3, 4, 5, 6, 7,
    8, 9, 10, 11, 14, 15, 19, 20, 22, 23, 21, 16, 12, 27, 26, 18, 17, 13, 0, 1, 2,
    3, 8, 4, 5, 6, 7, 9, 29, 30, 31, 26, 0, 1, 2, 3, 4, 8, 5, 6, 7, 9, 10, 11, 29,
    30, 31, 12, 13, 25, 0, 1, 2, 3, 5, 8, 4, 6, 7, 9, 14, 15, 29, 30, 31, 16, 26,
    17, 28, 0, 1, 2, 3, 4, 5, 8, 6, 7, 9, 10, 11, 14, 15, 29, 30, 31, 16, 12, 13,
    25, 18, 0, 1, 2, 3, 6, 8, 4, 5, 7, 9, 19, 20, 29, 30, 21, 26, 18, 32, 0, 1, 2,
    3, 4, 6, 8, 5, 7, 9, 10, 11, 19, 20, 29, 30, 21, 12, 13, 32, 25, 17, 0, 1, 2,
    3, 5, 6, 8, 4, 7, 9, 14, 15, 19, 20, 29, 30, 21, 16, 26, 17, 32, 13, 28, 0, 1,
    2, 3, 4, 5, 6, 8, 7, 9, 10, 11, 14, 15, 19, 20, 29, 30, 21, 16, 12, 32, 25, 18,
    17, 13, 0, 1, 2, 3, 7, 8, 4, 5, 6, 9, 22, 23, 29, 30, 31, 24, 25, 12, 0, 1,
    2, 3, 4, 7, 8, 5, 6, 9, 10, 11, 22, 23, 29, 30, 31, 24, 13, 26, 25, 12, 0, 1,
    2, 3, 5, 7, 8, 4, 6, 9, 14, 15, 22, 23, 29, 30, 31, 16, 25, 17, 27, 12, 28, 0,
    1, 2, 3, 4, 5, 7, 8, 6, 9, 10, 11, 14, 15, 22, 23, 29, 30, 31, 16, 13, 27, 26,
    25, 12, 18, 28, 0, 1, 2, 3, 6, 7, 8, 4, 5, 9, 19, 20, 22, 23, 29, 30, 21, 24,
    25, 18, 32, 12, 28, 0, 1, 2, 3, 4, 6, 7, 8, 5, 9, 10, 11, 19, 20, 22, 23, 29,
    30, 21, 24, 13, 32, 26, 25, 12, 17, 28, 0, 1, 2, 3, 5, 6, 7, 8, 4, 9, 14, 15,
    19, 20, 22, 23, 29, 30, 21, 16, 25, 17, 32, 27, 12, 13, 28, 0, 1, 2, 3, 4, 5,
    6, 7, 8, 9, 10, 11, 14, 15, 19, 20, 22, 23, 29, 30, 21, 16, 32, 27, 26, 25, 12,
    18, 17, 13, 28, 0, 1, 2, 3, 9, 4, 5, 6, 7, 8, 33, 34, 32, 27, 0, 1, 2, 3, 4,
    9, 5, 6, 7, 8, 10, 11, 33, 34, 32, 27, 12, 13, 28, 0, 1, 2, 3, 5, 9, 4, 6, 7,
    8, 14, 15, 33, 34, 32, 16, 17, 24, 0, 1, 2, 3, 4, 5, 9, 6, 7, 8, 10, 11, 14,
    15, 33, 34, 32, 16, 12, 13, 24, 18, 28, 0, 1, 2, 3, 6, 9, 4, 5, 7, 8, 19, 20,
    33, 34, 21, 27, 18, 31, 0, 1, 2, 3, 4, 6, 9, 5, 7, 8, 10, 11, 19, 20, 33, 34,
    21, 27, 12, 13, 31, 17, 28, 0, 1, 2, 3, 5, 6, 9, 4, 7, 8, 14, 15, 19, 20, 33,
    34, 21, 16, 17, 31, 24, 13, 0, 1, 2, 3, 4, 5, 6, 9, 7, 8, 10, 11, 14, 15, 19,
    20, 33, 34, 21, 16, 12, 31, 24, 18, 17, 13, 0, 1, 2, 3, 7, 9, 4, 5, 6, 8, 22,
    23, 33, 34, 32, 24, 25, 16, 0, 1, 2, 3, 4, 7, 9, 5, 6, 8, 10, 11, 22, 23, 33,
    34, 32, 24, 12, 13, 16, 26, 28, 0, 1, 2, 3, 5, 7, 9, 4, 6, 8, 14, 15, 22, 23,
    33, 34, 32, 25, 17, 27, 24, 16, 0, 1, 2, 3, 4, 5, 7, 9, 6, 8, 10, 11, 14, 15,
    22, 23, 33, 34, 32, 12, 13, 27, 24, 16, 26, 18, 28, 0, 1, 2, 3, 6, 7, 9, 4, 5,
    8, 19, 20, 22, 23, 33, 34, 21, 24, 25, 18, 31, 16, 28, 0, 1, 2, 3, 4, 6, 7, 9,
    5, 8, 10, 11, 19, 20, 22, 23, 33, 34, 21, 24, 12, 13, 31, 16, 26, 17, 28, 0, 1,
    2, 3, 5, 6, 7, 9, 4, 8, 14, 15, 19, 20, 22, 23, 33, 34, 21, 25, 17, 31, 27,
    24, 16, 13, 28, 0, 1, 2, 3, 4, 5, 6, 7, 9, 8, 10, 11, 14, 15, 19, 20, 22, 23,
    33, 34, 21, 12, 31, 27, 24, 16, 26, 18, 17, 13, 28, 0, 1, 2, 3, 8, 9, 4, 5, 6,
    7, 29, 30, 33, 34, 31, 27, 26, 21, 0, 1, 2, 3, 4, 8, 9, 5, 6, 7, 10, 11, 29,
    30, 33, 34, 31, 27, 12, 13, 21, 25, 28, 0, 1, 2, 3, 5, 8, 9, 4, 6, 7, 14, 15,
    29, 30, 33, 34, 31, 16, 26, 17, 21, 24, 28, 0, 1, 2, 3, 4, 5, 8, 9, 6, 7, 10,
    11, 14, 15, 29, 30, 33, 34, 31, 16, 12, 13, 21, 24, 25, 18, 28, 0, 1, 2, 3, 6,
    8, 9, 4, 5, 7, 19, 20, 29, 30, 33, 34, 27, 26, 18, 32, 31, 21, 0, 1, 2, 3, 4,
    6, 8, 9, 5, 7, 10, 11, 19, 20, 29, 30, 33, 34, 27, 12, 13, 32, 31, 21, 25, 17,
    28, 0, 1, 2, 3, 5, 6, 8, 9, 4, 7, 14, 15, 19, 20, 29, 30, 33, 34, 16, 26, 17,
    32, 31, 21, 24, 13, 28, 0, 1, 2, 3, 4, 5, 6, 8, 9, 7, 10, 11, 14, 15, 19, 20,
    29, 30, 33, 34, 16, 12, 32, 31, 21, 24, 25, 18, 17, 13, 28, 0, 1, 2, 3, 7, 8,
    9, 4, 5, 6, 22, 23, 29, 30, 33, 34, 31, 24, 25, 21, 16, 12, 0, 1, 2, 3, 4, 7,
    8, 9, 5, 6, 10, 11, 22, 23, 29, 30, 33, 34, 31, 24, 13, 21, 16, 26, 25, 12, 0,
    1, 2, 3, 5, 7, 8, 9, 4, 6, 14, 15, 22, 23, 29, 30, 33, 34, 31, 25, 17, 21, 27,
    24, 16, 12, 28, 0, 1, 2, 3, 4, 5, 7, 8, 9, 6, 10, 11, 14, 15, 22, 23, 29, 30,
    33, 34, 31, 13, 21, 27, 24, 16, 26, 25, 12, 18, 28, 0, 1, 2, 3, 6, 7, 8, 9, 4,
    5, 19, 20, 22, 23, 29, 30, 33, 34, 24, 25, 18, 32, 31, 21, 16, 12, 28, 0, 1, 2,
    3, 4, 6, 7, 8, 9, 5, 10, 11, 19, 20, 22, 23, 29, 30, 33, 34, 24, 13, 32, 31,
    21, 16, 26, 25, 12, 17, 28, 0, 1, 2, 3, 5, 6, 7, 8, 9, 4, 14, 15, 19, 20, 22,
    23, 29, 30, 33, 34, 25, 17, 32, 31, 21, 27, 24, 16, 12, 13, 28, 0, 1, 2, 3, 4,
    5, 6, 7, 8, 9, 10, 11, 14, 15, 19, 20, 22, 23, 29, 30, 33, 34, 32, 31, 21, 27,
    24, 16, 26, 25, 12, 18, 17, 13, 28
};

/// Column index for compressed row format storage.
const unsigned char P2_prolongation_col_ind[116]= {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 4, 0, 1, 4, 0, 1, 4, 7,
    8, 0, 1, 4, 5, 6, 0, 2, 5, 0, 2, 5, 0, 2, 5, 7, 9, 0, 2, 4,
    5, 6, 1, 2, 4, 5, 6, 1, 2, 6, 1, 2, 6, 1, 2, 6, 8, 9, 0, 3,
    7, 0, 3, 7, 0, 3, 5, 7, 9, 0, 3, 4, 7, 8, 1, 3, 4, 7, 8, 2,
    3, 5, 7, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 3, 8, 1, 3, 8,
    1, 3, 6, 8, 9, 2, 3, 6, 8, 9, 2, 3, 9, 2, 3, 9
};

/// Beginning of rows for compressed row format storage.
const unsigned char P2_prolongation_row_beg[36]= {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 13, 16, 21, 26, 29, 32, 37, 42,
    47, 50, 53, 58, 61, 64, 69, 74, 79, 84, 94, 97, 100, 105, 110, 113, 116
};

/// Coefficients of the prolongation matrices as index into coeff.
const unsigned char P2_prolongation_coeff_idx[116]= {
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 2, 0, 4, 0, 2, 4, 0, 0, 1, 3, 3,
    0, 0, 1, 3, 3, 2, 0, 4, 0, 2, 4, 0, 0, 1, 3, 3, 0, 0, 3, 1, 3, 0, 0, 3, 3, 1,
    2, 0, 4, 0, 2, 4, 0, 0, 1, 3, 3, 2, 0, 4, 0, 2, 4, 0, 0, 3, 1, 3, 0, 0, 3, 1,
    3, 0, 0, 3, 3, 1, 0, 0, 3, 3, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 0, 4, 0, 2,
    4, 0, 0, 3, 1, 3, 0, 0, 3, 3, 1, 2, 0, 4, 0, 2, 4
};

/// The coefficients in the prolongation matrices.
const double P2_prolongation_coeff[6]= {
    -0.125, 0.25, 0.375, 0.5, 0.75, 1.0
};
/// \}

std::vector<IdxT> CollectChildUnknownsP2(const TetraCL& t, const Uint f_idx)
{
    const RefRuleCL& rule= t.GetRefData();
    typedef std::map<Ubyte, IdxT>::value_type map_entryT;
    std::map<Ubyte, IdxT> childvertex;
    std::map<Ubyte, IdxT> childedge;

    for (Uint j= 0; j < rule.ChildNum; ++j) {
        const ChildDataCL& child= GetChildData( rule.Children[j]);
        const TetraCL* const childp= t.GetChild( j);
        for (Ubyte k= 0; k<4; ++k) {
            const VertexCL* const p= childp->GetVertex( k);
            childvertex[child.Vertices[k]]= p->Unknowns.Exist()
                                            && p->Unknowns.Exist( f_idx)
                                            ? p->Unknowns( f_idx) : NoIdx;
        }
        for (Ubyte k= 0; k<6; ++k) {
            const EdgeCL* const p= childp->GetEdge( k);
            childedge[child.Edges[k]]= p->Unknowns.Exist()
                                       && p->Unknowns.Exist( f_idx)
                                       ? p->Unknowns( f_idx) : NoIdx;
        }
    }
    std::vector<IdxT> ret( childvertex.size() + childedge.size());
    std::transform( childvertex.begin(), childvertex.end(), ret.begin(),
                    select2nd<map_entryT>());
    std::transform( childedge.begin(), childedge.end(), ret.begin() + childvertex.size(),
                    select2nd<map_entryT>());
    return ret;
}


void SetupP2ProlongationMatrix(const MultiGridCL& mg, MatrixCL& P,
                               IdxDescCL& cIdx, IdxDescCL& fIdx)
{
    const Uint c_level= cIdx.TriangLevel();
    const Uint f_level= fIdx.TriangLevel();
    const Uint c_idx= cIdx.GetIdx();
    const Uint f_idx= fIdx.GetIdx();
// It is only a copy of the 1D case 'ndofs' times
    const Uint ndofs= fIdx.NumUnknownsVertex();

    MatrixBuilderCL mat( &P, fIdx.NumUnknowns(), cIdx.NumUnknowns());
/*    std::cout << "    alt                    cIdx "
              << cIdx->TriangLevel        << ", "
              << cIdx->NumUnknownsVertex_ << ", "
              << cIdx->NumUnknownsEdge_   << ", "
              << cIdx->NumUnknownsFace_   << ", "
              << cIdx->NumUnknownsTetra_  << ", "
              << cIdx->NumUnknowns
              << std::endl;
    std::cout << "    neu                    fIdx "
              << fIdx->TriangLevel        << ", "
              << fIdx->NumUnknownsVertex_ << ", "
              << fIdx->NumUnknownsEdge_   << ", "
              << fIdx->NumUnknownsFace_   << ", "
              << fIdx->NumUnknownsTetra_  << ", "
              << fIdx->NumUnknowns
              << std::endl;
*/
    for (MultiGridCL::const_TriangTetraIteratorCL sit= mg.GetTriangTetraBegin( c_level),
         theend= mg.GetTriangTetraEnd( c_level); sit != theend; ++sit) {
        if (!sit->IsInTriang( f_level)) {
            IdxT cUnknowns[10];
            // Collect the coarse indices.
            for (Uint i=0; i<4; ++i)
                cUnknowns[i]= (sit->GetVertex( i)->Unknowns.Exist()
                               && sit->GetVertex( i)->Unknowns.Exist( c_idx))
                              ? sit->GetVertex( i)->Unknowns( c_idx) : NoIdx;
            for (Uint i=0; i<6; ++i)
                cUnknowns[i+4]= (sit->GetEdge( i)->Unknowns.Exist()
                                 && sit->GetEdge( i)->Unknowns.Exist( c_idx))
                                ? sit->GetEdge( i)->Unknowns( c_idx) : NoIdx;
            // Collect the fine indices.
            const std::vector<IdxT> fUnknowns( CollectChildUnknownsP2( *sit, f_idx));
            // Here, the green rule for regular refinement and the
            // regular rule are treated in the same way: & 63 cuts off
            // the bit that distinguishes them.
            const Uint rule= sit->GetRefRule() & 63;
            for (Uint i= P2_local_prolongation_mat_beg[rule]; // Construct the rows into mat.
                 i != P2_local_prolongation_mat_beg[rule+1]; ++i) {
                const IdxT thefUnknown= fUnknowns[i-P2_local_prolongation_mat_beg[rule]];
                if (thefUnknown != NoIdx) {
                    const Uint row= P2_local_prolongation_rows[i];
                    for (Uint j= P2_prolongation_row_beg[row]; // Construct a single row into mat.
                         j != P2_prolongation_row_beg[row+1]; ++j) {
                        const IdxT thecUnknown= cUnknowns[P2_prolongation_col_ind[j]];
                        if (thecUnknown != NoIdx)
                            for (Uint k=0; k<ndofs; k++)
                                mat(thefUnknown+k, thecUnknown+k)=
                                P2_prolongation_coeff[P2_prolongation_coeff_idx[j]];
                    }
                }
            }
        }
        else { // coarse and fine tetra are identical; the prolongation is trivial.
            for (Uint i=0; i<4; ++i)
                if (sit->GetVertex( i)->Unknowns.Exist()
                    && sit->GetVertex( i)->Unknowns.Exist( c_idx)
                    && sit->GetVertex( i)->Unknowns.Exist( f_idx))
                    for (Uint k=0; k<ndofs; k++)
                        mat(sit->GetVertex( i)->Unknowns( f_idx)+k,
                            sit->GetVertex( i)->Unknowns( c_idx)+k)= 1.0;
            for (Uint i=0; i<6; ++i)
                if (sit->GetEdge( i)->Unknowns.Exist()
                    && sit->GetEdge( i)->Unknowns.Exist( c_idx)
                    && sit->GetEdge( i)->Unknowns.Exist( f_idx))
                    for (Uint k=0; k<ndofs; k++)
                        mat(sit->GetEdge( i)->Unknowns( f_idx)+k,
                            sit->GetEdge( i)->Unknowns( c_idx)+k)= 1.0;
        }
    }
    mat.Build();
}

void SetupP2ProlongationMatrix(const MultiGridCL& mg, MLMatDescCL& P)
{
    SetupP2ProlongationMatrix( mg, P.Data, P.ColIdx, P.RowIdx);
}

void SetupP2ProlongationMatrix(const MultiGridCL& mg, MLMatrixCL& P, MLIdxDescCL* ColIdx, MLIdxDescCL* RowIdx)
{
    P.resize( ColIdx->size());
    MLMatrixCL::iterator itProlong = ++P.begin();
    MLIdxDescCL::iterator itcIdx = ColIdx->begin();
    MLIdxDescCL::iterator itfIdx = ++RowIdx->begin();
    for (size_t lvl=1; lvl < P.size(); ++lvl, ++itProlong, ++itcIdx, ++itfIdx)
        SetupP2ProlongationMatrix( mg, *itProlong, *itcIdx, *itfIdx);
}

} // end of namespace DROPS
