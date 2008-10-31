/// \file
/// \brief creates several standard Stokes-solver
/// \author Sven Gross, Joerg Grande, Patrick Esser, IGPM

#include "num/stokessolver.h"

namespace DROPS {

/*******************************************************************
*   S t o k e s S o l v e r F a c t o r y  C L                     *
*******************************************************************/
/// \brief Creates a StokesSolverCL* and manages the preconditioner
/**
<table border="1">
    <tr> <th>no</th><th>Solver       </th><th>APc      </th><th>SPc     </th><th>PC structure</th></td><td>
         <th>no</th><th>Solver       </th><th>APc      </th><th>SPc     </th><th>PC structure</th></tr>
    <tr><td> 11</td><td>GCR          </td><td>MG       </td><td>BBT     </td><td>lower block</td></td><td>
        <td> 12</td><td>GCR          </td><td>MG       </td><td>MinComm </td><td>lower block</td></tr>
    <tr><td> 13</td><td>GCR          </td><td>GMRes    </td><td>BBT     </td><td>lower block</td></td><td>
        <td> 14</td><td>GCR          </td><td>GMRes    </td><td>MinComm </td><td>lower block</td></tr>
    <tr><td> 15</td><td>GCR          </td><td>BiCGStab </td><td>BBT     </td><td>lower block</td></td><td>
        <td> 16</td><td>GCR          </td><td>BiCGStab </td><td>MinComm </td><td>lower block</td></tr>
    <tr><td> 21</td><td>Inexact Uzawa</td><td>MG       </td><td>BBT     </td><td> -  </td></td><td>
        <td> 22</td><td>Inexact Uzawa</td><td>MG       </td><td>MinComm </td><td> -  </td></tr>
    <tr><td> 23</td><td>Inexact Uzawa</td><td>GMRes    </td><td>BBT     </td><td> -  </td></td><td>
        <td> 24</td><td>Inexact Uzawa</td><td>GMRes    </td><td>MinComm </td><td> -  </td></tr>
    <tr><td> 25</td><td>Inexact Uzawa</td><td>BiCGStab </td><td>BBT     </td><td> -  </td></td><td>
        <td> 26</td><td>Inexact Uzawa</td><td>BiCGStab </td><td>MinComm </td><td> -  </td></tr>
    <tr><td>221</td><td>Inexact Uzawa</td><td>symm. MG </td><td>BBT     </td><td> -  </td></td><td>
        <td>222</td><td>Inexact Uzawa</td><td>symm. MG </td><td>MinComm </td><td> -  </td></tr>
    <tr><td>225</td><td>Inexact Uzawa</td><td>SSORPCG  </td><td>BBT     </td><td> -  </td></td><td>
        <td>226</td><td>Inexact Uzawa</td><td>SSORPCG  </td><td>MinComm </td><td> -  </td></tr>
    <tr><td> 31</td><td>PMinRes      </td><td>MG       </td><td>BBT     </td><td>diag</td></td><td>
        <td> 32</td><td>PMinRes      </td><td>MG       </td><td>MinComm </td><td>diag</td></tr>
    <tr><td> 33</td><td>PMinRes      </td><td>SSORPCG  </td><td>BBT     </td><td>diag</td></td><td>
        <td> 34</td><td>PMinRes      </td><td>SSORPCG  </td><td>MinComm </td><td>diag</td></tr>
    <tr><td> 41</td><td>GMRes        </td><td>MG       </td><td>BBT     </td><td>lower block</td></td><td>
        <td> 42</td><td>GMRes        </td><td>MG       </td><td>MinComm </td><td>lower block</td></tr>
    <tr><td> 43</td><td>GMRes        </td><td>GMRes    </td><td>BBT     </td><td>lower block</td></td><td>
        <td> 44</td><td>GMRes        </td><td>GMRes    </td><td>MinComm </td><td>lower block</td></tr>
    <tr><td> 45</td><td>GMRes        </td><td>BiCGStab </td><td>BBT     </td><td>lower block</td></td><td>
        <td> 46</td><td>GMRes        </td><td>BiCGStab </td><td>MinComm </td><td>lower block</td></tr>
    <tr><td> 51</td><td>GMResR       </td><td>MG       </td><td>BBT     </td><td>lower block</td></td><td>
        <td> 52</td><td>GMResR       </td><td>MG       </td><td>MinComm </td><td>lower block</td></tr>
    <tr><td> 53</td><td>GMResR       </td><td>GMRes    </td><td>BBT     </td><td>lower block</td></td><td>
        <td> 54</td><td>GMResR       </td><td>GMRes    </td><td>MinComm </td><td>lower block</td></tr>
    <tr><td> 55</td><td>GMResR       </td><td>BiCGStab </td><td>BBT     </td><td>lower block</td></td><td>
        <td> 56</td><td>GMResR       </td><td>BiCGStab </td><td>MinComm </td><td>lower block</td></tr>
    <tr><td> 81</td><td>StokesMGM    </td><td>PVanka   </td><td> </td><td> </td></td><td>
        <td> 82</td><td>StokesMGM    </td><td>BraessSarazin</td><td> </td><td> </td></tr>
</table>*/
/*******************************************************************
*   S t o k e s S o l v e r F a c t o r y  C L                     *
********************************************************************/
template <class StokesT, class ParamsT>
class StokesSolverFactoryCL
{
  private:
    StokesT& Stokes_;
    ParamsT& C_;
    double kA_, kM_;
    bool   mgused_;

// PC for instat. Schur complement
    ISBBTPreCL bbtispc_;
    MinCommPreCL mincommispc_;

// PC for A-block
    // MultiGrid symm.
    SSORsmoothCL smoother_;
    PCG_SsorCL   coarsesolversymm_;
    MGSolverCL<SSORsmoothCL, PCG_SsorCL> MGSolversymm_;
    typedef SolverAsPreCL<MGSolverCL<SSORsmoothCL, PCG_SsorCL> > MGsymmPcT;
    MGsymmPcT MGPcsymm_;

    // Multigrid nonsymm.
    GMResSolverCL<JACPcCL> coarsesolver_;
    MGSolverCL<SSORsmoothCL, GMResSolverCL<JACPcCL> > MGSolver_;
    typedef SolverAsPreCL<MGSolverCL<SSORsmoothCL, GMResSolverCL<JACPcCL> > > MGPcT;
    MGPcT MGPc_;

    //JAC-GMRes
    JACPcCL  JACPc_;
    typedef GMResSolverCL<JACPcCL> GMResSolverT;
    GMResSolverT GMResSolver_;
    typedef SolverAsPreCL<GMResSolverT> GMResPcT;
    GMResPcT GMResPc_;

    //JAC-BiCGStab
    typedef BiCGStabSolverCL<JACPcCL> BiCGStabSolverT;
    BiCGStabSolverT BiCGStabSolver_;
    typedef SolverAsPreCL<BiCGStabSolverT> BiCGStabPcT;
    BiCGStabPcT BiCGStabPc_;

    //PCG
    SSORPcCL SSORPc_;
    typedef PCGSolverCL<SSORPcCL> PCGSolverT;
    PCGSolverT PCGSolver_;
    typedef SolverAsPreCL<PCGSolverT> PCGPcT;
    PCGPcT PCGPc_;

// PC for Oseen problem
    typedef BlockPreCL<MGsymmPcT, ISBBTPreCL>   DiagMGBBTOseenPcT;
    typedef BlockPreCL<MGsymmPcT, MinCommPreCL> DiagMGMinCommOseenPcT;
    typedef BlockPreCL<GMResPcT,  MinCommPreCL> DiagGMResMinCommOseenPcT;
    typedef BlockPreCL<PCGPcT,    ISBBTPreCL>   DiagPCGBBTOseenPcT;
    typedef BlockPreCL<PCGPcT,    MinCommPreCL> DiagPCGMinCommOseenPcT;

    typedef BlockPreCL<MGPcT,       ISBBTPreCL,   LowerBlockPreCL> LBlockMGBBTOseenPcT;
    typedef BlockPreCL<MGPcT,       MinCommPreCL, LowerBlockPreCL> LBlockMGMinCommOseenPcT;
    typedef BlockPreCL<GMResPcT,    ISBBTPreCL,   LowerBlockPreCL> LBlockGMResBBTOseenPcT;
    typedef BlockPreCL<GMResPcT,    MinCommPreCL, LowerBlockPreCL> LBlockGMResMinCommOseenPcT;
    typedef BlockPreCL<BiCGStabPcT, ISBBTPreCL,   LowerBlockPreCL> LBlockBiCGBBTOseenPcT;
    typedef BlockPreCL<BiCGStabPcT, MinCommPreCL, LowerBlockPreCL> LBlockBiCGMinCommOseenPcT;

    DiagMGBBTOseenPcT         DiagMGBBTOseenPc_;
    DiagMGMinCommOseenPcT     DiagMGMinCommOseenPc_;
    DiagGMResMinCommOseenPcT  DiagGMResMinCommPc_;
    DiagPCGBBTOseenPcT        DiagPCGBBTOseenPc_;
    DiagPCGMinCommOseenPcT    DiagPCGMinCommOseenPc_;

    LBlockMGBBTOseenPcT        LBlockMGBBTOseenPc_;
    LBlockMGMinCommOseenPcT    LBlockMGMinCommOseenPc_;
    LBlockGMResBBTOseenPcT     LBlockGMResBBTOseenPc_;
    LBlockGMResMinCommOseenPcT LBlockGMResMinCommOseenPc_;
    LBlockBiCGBBTOseenPcT      LBlockBiCGBBTOseenPc_;
    LBlockBiCGMinCommOseenPcT  LBlockBiCGMinCommOseenPc_;

//GCR solver
    GCRSolverCL<LBlockMGBBTOseenPcT>        GCRMGBBT_;
    GCRSolverCL<LBlockMGMinCommOseenPcT>    GCRMGMinComm_;
    GCRSolverCL<LBlockGMResBBTOseenPcT>     GCRGMResBBT_;
    GCRSolverCL<LBlockGMResMinCommOseenPcT> GCRGMResMinComm_;
    GCRSolverCL<LBlockBiCGBBTOseenPcT>      GCRBiCGStabBBT_;
    GCRSolverCL<LBlockBiCGMinCommOseenPcT>  GCRBiCGStabMinComm_;

//GMRes solver
    GMResSolverCL<LBlockMGBBTOseenPcT>        GMResMGBBT_;
    GMResSolverCL<LBlockMGMinCommOseenPcT>    GMResMGMinComm_;
    GMResSolverCL<LBlockGMResBBTOseenPcT>     GMResGMResBBT_;
    GMResSolverCL<LBlockGMResMinCommOseenPcT> GMResGMResMinComm_;
    GMResSolverCL<LBlockBiCGBBTOseenPcT>      GMResBiCGStabBBT_;
    GMResSolverCL<LBlockBiCGMinCommOseenPcT>  GMResBiCGStabMinComm_;

// GMResR solver
    GMResRSolverCL<LBlockMGBBTOseenPcT>        GMResRMGBBT_;
    GMResRSolverCL<LBlockMGMinCommOseenPcT>    GMResRMGMinComm_;
    GMResRSolverCL<LBlockGMResBBTOseenPcT>     GMResRGMResBBT_;
    GMResRSolverCL<LBlockGMResMinCommOseenPcT> GMResRGMResMinComm_;
    GMResRSolverCL<LBlockBiCGBBTOseenPcT>      GMResRBiCGStabBBT_;
    GMResRSolverCL<LBlockBiCGMinCommOseenPcT>  GMResRBiCGStabMinComm_;

// Lanczos
    typedef PLanczosONBCL<BlockMatrixCL, VectorCL, DiagMGBBTOseenPcT>      Lanczos1T;
    typedef PLanczosONBCL<BlockMatrixCL, VectorCL, DiagMGMinCommOseenPcT>  Lanczos2T;
    typedef PLanczosONBCL<BlockMatrixCL, VectorCL, DiagPCGBBTOseenPcT>     Lanczos3T;
    typedef PLanczosONBCL<BlockMatrixCL, VectorCL, DiagPCGMinCommOseenPcT> Lanczos4T;

    Lanczos1T lanczos1_;
    Lanczos2T lanczos2_;
    Lanczos3T lanczos3_;
    Lanczos4T lanczos4_;

// MinRes solver
    PMResSolverCL<Lanczos1T> PMinResMGBBT_;
    PMResSolverCL<Lanczos2T> PMinResMGMinComm_;
    PMResSolverCL<Lanczos3T> PMinResPCGBBT_;
    PMResSolverCL<Lanczos4T> PMinResPCGMinComm_;

// coarse grid solver
    //MinRes solver
    PMResSolverCL<Lanczos3T> minressolver;
    BlockMatrixSolverCL<PMResSolverCL<Lanczos3T> > blockminressolver;

    //GCR solver
    GCRSolverCL<DiagGMResMinCommOseenPcT> gcrsolver;
    BlockMatrixSolverCL<GCRSolverCL<DiagGMResMinCommOseenPcT> > blockgcrsolver;

    PVankaSmootherCL vankasmoother;
    BSSmootherCL bssmoother;

  public:
    StokesSolverFactoryCL(StokesT& Stokes, ParamsT& C);
    ~StokesSolverFactoryCL() {}

    /// Set the A-block in the minimal commutator
    void       SetMatrixA (const MatrixCL** A) { mincommispc_.SetMatrixA(A); }
    /// Set all matrices in Schur complement preconditioner (only for StokesMGM)
    void       SetMatrices(const MatrixCL** A, const MatrixCL* B, const MatrixCL* Mvel, const MatrixCL* M);
    /// Returns true if a multigrid solver/preconditioner is used
    bool       MGUsed()   {return mgused_;}

    /// Returns a stokes solver with specifications from ParamsT C
    StokesSolverBaseCL* CreateStokesSolver();
};

template <class StokesT, class ParamsT>
StokesSolverFactoryCL<StokesT, ParamsT>::StokesSolverFactoryCL(StokesT& Stokes, ParamsT& C)
    : Stokes_(Stokes), C_(C), kA_(1.0/C_.dt), kM_(C_.theta), mgused_(false),
        // schur complement preconditioner
        bbtispc_    ( &Stokes_.B.Data, &Stokes_.prM.Data, &Stokes_.M.Data, kA_, kM_, C_.pcS_tol, C_.pcS_tol),
        mincommispc_( 0, &Stokes_.B.Data, &Stokes_.M.Data, &Stokes_.prM.Data, C_.pcS_tol),
        // preconditioner for A
        smoother_( 1.0), coarsesolversymm_( SSORPcCL(1.0), 500, 1e-6, true),
        MGSolversymm_ ( Stokes.GetMGData(), smoother_, coarsesolversymm_, C_.pcA_iter, C_.pcA_tol, false),
        MGPcsymm_( MGSolversymm_),
        coarsesolver_( JACPcCL(1.0), 500, 500, 1e-6, true),
        MGSolver_ ( Stokes.GetMGData(), smoother_, coarsesolver_, C_.pcA_iter, C_.pcA_tol, false), MGPc_( MGSolver_),
        GMResSolver_( JACPc_, C_.pcA_iter, /*restart*/ 100, C_.pcA_tol, /*rel*/ true), GMResPc_( GMResSolver_),
        BiCGStabSolver_( JACPc_, C_.pcA_iter, C_.pcA_tol, /*rel*/ true),BiCGStabPc_( BiCGStabSolver_),
        PCGSolver_( SSORPc_, C_.pcA_iter, C_.pcA_tol, true), PCGPc_( PCGSolver_),
        // block precondtioner
        DiagMGBBTOseenPc_        ( MGPcsymm_,    bbtispc_), DiagMGMinCommOseenPc_     ( MGPcsymm_,    mincommispc_),
        DiagGMResMinCommPc_      ( GMResPc_, mincommispc_),
        DiagPCGBBTOseenPc_       ( PCGPc_,   bbtispc_), DiagPCGMinCommOseenPc_    ( PCGPc_,   mincommispc_),
        LBlockMGBBTOseenPc_      ( MGPc_,    bbtispc_), LBlockMGMinCommOseenPc_   ( MGPc_,    mincommispc_),
        LBlockGMResBBTOseenPc_   ( GMResPc_, bbtispc_), LBlockGMResMinCommOseenPc_( GMResPc_, mincommispc_),
        LBlockBiCGBBTOseenPc_    ( BiCGStabPc_, bbtispc_), LBlockBiCGMinCommOseenPc_( BiCGStabPc_, mincommispc_),
        // GCR solver
        GCRMGBBT_           ( LBlockMGBBTOseenPc_,        C_.outer_iter, C_.outer_iter, C_.outer_tol, /*rel*/ false),
        GCRMGMinComm_       ( LBlockMGMinCommOseenPc_,    C_.outer_iter, C_.outer_iter, C_.outer_tol, /*rel*/ false),
        GCRGMResBBT_        ( LBlockGMResBBTOseenPc_,     C_.outer_iter, C_.outer_iter, C_.outer_tol, /*rel*/ false),
        GCRGMResMinComm_    ( LBlockGMResMinCommOseenPc_, C_.outer_iter, C_.outer_iter, C_.outer_tol, /*rel*/ false),
        GCRBiCGStabBBT_     ( LBlockBiCGBBTOseenPc_,      C_.outer_iter, C_.outer_iter, C_.outer_tol, /*rel*/ false),
        GCRBiCGStabMinComm_ ( LBlockBiCGMinCommOseenPc_,  C_.outer_iter, C_.outer_iter, C_.outer_tol, /*rel*/ false),
        // GMRes solver
        GMResMGBBT_           ( LBlockMGBBTOseenPc_,        C_.outer_iter, C_.outer_iter, C_.outer_tol,
                                /*rel*/ false, false, RightPreconditioning),
        GMResMGMinComm_       ( LBlockMGMinCommOseenPc_,    C_.outer_iter, C_.outer_iter, C_.outer_tol,
                                /*rel*/ false, false, RightPreconditioning),
        GMResGMResBBT_        ( LBlockGMResBBTOseenPc_,     C_.outer_iter, C_.outer_iter, C_.outer_tol,
                                /*rel*/ false, false, RightPreconditioning),
        GMResGMResMinComm_    ( LBlockGMResMinCommOseenPc_, C_.outer_iter, C_.outer_iter, C_.outer_tol,
                                /*rel*/ false, false, RightPreconditioning),
        GMResBiCGStabBBT_     ( LBlockBiCGBBTOseenPc_,      C_.outer_iter, C_.outer_iter, C_.outer_tol,
                                /*rel*/ false, false, RightPreconditioning),
        GMResBiCGStabMinComm_ ( LBlockBiCGMinCommOseenPc_,  C_.outer_iter, C_.outer_iter, C_.outer_tol,
                                /*rel*/ false, false, RightPreconditioning),
        GMResRMGBBT_          ( LBlockMGBBTOseenPc_,        C_.outer_iter, C_.outer_iter, C_.inner_iter,
                                    C_.outer_tol, C_.inner_tol, /*rel*/ false),
        GMResRMGMinComm_      ( LBlockMGMinCommOseenPc_,    C_.outer_iter, C_.outer_iter, C_.inner_iter,
                                    C_.outer_tol, C_.inner_tol, /*rel*/ false),
        GMResRGMResBBT_       ( LBlockGMResBBTOseenPc_,     C_.outer_iter, C_.outer_iter, C_.inner_iter,
                                    C_.outer_tol, C_.inner_tol, /*rel*/ false),
        GMResRGMResMinComm_   ( LBlockGMResMinCommOseenPc_, C_.outer_iter, C_.outer_iter, C_.inner_iter,
                                    C_.outer_tol, C_.inner_tol, /*rel*/ false),
        GMResRBiCGStabBBT_    ( LBlockBiCGBBTOseenPc_,      C_.outer_iter, C_.outer_iter, C_.inner_iter,
                                    C_.outer_tol, C_.inner_tol, /*rel*/ false),
        GMResRBiCGStabMinComm_( LBlockBiCGMinCommOseenPc_,  C_.outer_iter, C_.outer_iter, C_.inner_iter,
                                    C_.outer_tol, C_.inner_tol, /*rel*/ false),
        // lanczos objects
        lanczos1_ (DiagMGBBTOseenPc_),  lanczos2_ (DiagMGMinCommOseenPc_),
        lanczos3_ (DiagPCGBBTOseenPc_), lanczos4_ (DiagPCGMinCommOseenPc_),
        // PMinRes solver
        PMinResMGBBT_     ( lanczos1_, C_.outer_iter, C_.outer_tol, /*relative*/ false),
        PMinResMGMinComm_ ( lanczos2_, C_.outer_iter, C_.outer_tol, /*relative*/ false),
        PMinResPCGBBT_    ( lanczos3_, C_.outer_iter, C_.outer_tol, /*relative*/ false),
        PMinResPCGMinComm_( lanczos4_, C_.outer_iter, C_.outer_tol, /*relative*/ false),
        // coarse grid/direct solver for StokesMGM
        minressolver( lanczos3_, 500, 1e-6, true), blockminressolver(minressolver),
        gcrsolver( DiagGMResMinCommPc_, 500, 500, 1e-6, true), blockgcrsolver(gcrsolver)
        {}

template <class StokesT, class ParamsT>
StokesSolverBaseCL* StokesSolverFactoryCL<StokesT, ParamsT>::CreateStokesSolver()
{
    StokesSolverBaseCL* stokessolver = 0;
    switch (C_.StokesMethod)
    {
        case 11 :
            stokessolver = new BlockMatrixSolverCL<GCRSolverCL<LBlockMGBBTOseenPcT> >       ( GCRMGBBT_);
        break;
        case 12 :
            stokessolver = new BlockMatrixSolverCL<GCRSolverCL<LBlockMGMinCommOseenPcT> >   ( GCRMGMinComm_);
        break;
        case 13 :
            stokessolver = new BlockMatrixSolverCL<GCRSolverCL<LBlockGMResBBTOseenPcT> >    ( GCRGMResBBT_);
        break;
        case 14 :
            stokessolver = new BlockMatrixSolverCL<GCRSolverCL<LBlockGMResMinCommOseenPcT> >( GCRGMResMinComm_);
        break;
        case 15 :
            stokessolver = new BlockMatrixSolverCL<GCRSolverCL<LBlockBiCGBBTOseenPcT> >     ( GCRBiCGStabBBT_);
        break;
        case 16 :
            stokessolver = new BlockMatrixSolverCL<GCRSolverCL<LBlockBiCGMinCommOseenPcT> > ( GCRBiCGStabMinComm_);
        break;
        case 211 :
            stokessolver = new InexactUzawaCL<MGsymmPcT, ISBBTPreCL, APC_SYM>
                        ( MGPcsymm_, bbtispc_, C_.outer_iter, C_.outer_tol, C_.inner_tol);
        break;
        case 222 :
            stokessolver = new InexactUzawaCL<MGsymmPcT, MinCommPreCL, APC_SYM>
                        ( MGPcsymm_, mincommispc_, C_.outer_iter, C_.outer_tol, C_.inner_tol);
        break;
        case 21 :
            stokessolver = new InexactUzawaCL<MGPcT, ISBBTPreCL, APC_OTHER>
                        ( MGPc_, bbtispc_, C_.outer_iter, C_.outer_tol, C_.inner_tol);
        break;
        case 22 :
            stokessolver = new InexactUzawaCL<MGPcT, MinCommPreCL, APC_OTHER>
                        ( MGPc_, mincommispc_, C_.outer_iter, C_.outer_tol, C_.inner_tol);
        break;
        case 23 :
            stokessolver = new InexactUzawaCL<GMResPcT, ISBBTPreCL, APC_OTHER>
                        ( GMResPc_, bbtispc_, C_.outer_iter, C_.outer_tol, C_.inner_tol);
        break;
        case 24 :
            stokessolver = new InexactUzawaCL<GMResPcT, MinCommPreCL, APC_OTHER>
                        ( GMResPc_, mincommispc_, C_.outer_iter, C_.outer_tol, C_.inner_tol);
        break;
        case 225 :
            stokessolver = new InexactUzawaCL<PCGPcT, ISBBTPreCL, APC_SYM>
                        ( PCGPc_, bbtispc_, C_.outer_iter, C_.outer_tol, C_.inner_tol);
        break;
        case 226 :
            stokessolver = new InexactUzawaCL<PCGPcT, MinCommPreCL, APC_SYM>
                        ( PCGPc_, mincommispc_, C_.outer_iter, C_.outer_tol, C_.inner_tol);
        break;
        case 25 :
            stokessolver = new InexactUzawaCL<BiCGStabPcT, ISBBTPreCL, APC_OTHER>
                        ( BiCGStabPc_, bbtispc_, C_.outer_iter, C_.outer_tol, C_.inner_tol);
        break;
        case 26 :
            stokessolver = new InexactUzawaCL<BiCGStabPcT, MinCommPreCL, APC_OTHER>
                        ( BiCGStabPc_, mincommispc_, C_.outer_iter, C_.outer_tol, C_.inner_tol);
        break;
        case 31 :
            stokessolver = new BlockMatrixSolverCL<PMResSolverCL<Lanczos1T> >( PMinResMGBBT_);
        break;
        case 32 :
            stokessolver = new BlockMatrixSolverCL<PMResSolverCL<Lanczos2T> >( PMinResMGMinComm_);
        break;
        case 33 :
            stokessolver = new BlockMatrixSolverCL<PMResSolverCL<Lanczos3T> >( PMinResPCGBBT_);
        break;
        case 34 :
            stokessolver = new BlockMatrixSolverCL<PMResSolverCL<Lanczos4T> >( PMinResPCGMinComm_);
        break;
        case 41 :
            stokessolver = new BlockMatrixSolverCL<GMResSolverCL<LBlockMGBBTOseenPcT> >        ( GMResMGBBT_);
        break;
        case 42 :
            stokessolver = new BlockMatrixSolverCL<GMResSolverCL<LBlockMGMinCommOseenPcT> >    ( GMResMGMinComm_);
        break;
        case 43 :
            stokessolver = new BlockMatrixSolverCL<GMResSolverCL<LBlockGMResBBTOseenPcT> >     ( GMResGMResBBT_);
        break;
        case 44 :
            stokessolver = new BlockMatrixSolverCL<GMResSolverCL<LBlockGMResMinCommOseenPcT> > ( GMResGMResMinComm_);
        break;
        case 45 :
            stokessolver = new BlockMatrixSolverCL<GMResSolverCL<LBlockBiCGBBTOseenPcT> >      ( GMResBiCGStabBBT_);
        break;
        case 46 :
            stokessolver = new BlockMatrixSolverCL<GMResSolverCL<LBlockBiCGMinCommOseenPcT> >  ( GMResBiCGStabMinComm_);
        break;
        case 51 :
            stokessolver = new BlockMatrixSolverCL<GMResRSolverCL<LBlockMGBBTOseenPcT> >       ( GMResRMGBBT_);
        break;
        case 52 :
            stokessolver = new BlockMatrixSolverCL<GMResRSolverCL<LBlockMGMinCommOseenPcT> >   ( GMResRMGMinComm_);
        break;
        case 53 :
            stokessolver = new BlockMatrixSolverCL<GMResRSolverCL<LBlockGMResBBTOseenPcT> >    ( GMResRGMResBBT_);
        break;
        case 54 :
            stokessolver = new BlockMatrixSolverCL<GMResRSolverCL<LBlockGMResMinCommOseenPcT> >( GMResRGMResMinComm_);
        break;
        case 55 :
            stokessolver = new BlockMatrixSolverCL<GMResRSolverCL<LBlockBiCGBBTOseenPcT> >     ( GMResRBiCGStabBBT_);
        break;
        case 56 :
            stokessolver = new BlockMatrixSolverCL<GMResRSolverCL<LBlockBiCGMinCommOseenPcT> > ( GMResRBiCGStabMinComm_);
        break;
        case 81 : {
            if (C_.XFEMStab >= 0) // P1X
                throw DROPSErrCL("StokesMGM not implemented for P1X-elements");
            Stokes_.GetMGData().SetStokesMG(true);
            vankasmoother.SetVankaMethod(0);
            if (C_.nonlinear==0.0) // stokes
                stokessolver = new StokesMGSolverCL<PVankaSmootherCL>
                        ( Stokes_.GetMGData(), vankasmoother, blockminressolver, C_.outer_iter, C_.outer_tol, false, 2);
            else
                stokessolver = new StokesMGSolverCL<PVankaSmootherCL>
                        ( Stokes_.GetMGData(), vankasmoother, blockgcrsolver,    C_.outer_iter, C_.outer_tol, false, 2);
        }
        break;
        case 82 : {
            if (C_.XFEMStab >= 0) // P1X
                throw DROPSErrCL("StokesMGM not implemented for P1X-elements");
            Stokes_.GetMGData().SetStokesMG(true);
            vankasmoother.SetVankaMethod(0);
            if (C_.nonlinear==0.0) // stokes
                stokessolver = new StokesMGSolverCL<BSSmootherCL>
                        ( Stokes_.GetMGData(), bssmoother, blockminressolver, C_.outer_iter, C_.outer_tol, false, 2);
            else
                stokessolver = new StokesMGSolverCL<BSSmootherCL>
                        ( Stokes_.GetMGData(), bssmoother, blockgcrsolver,    C_.outer_iter, C_.outer_tol, false, 2);
        }
        break;
        default: throw DROPSErrCL("Unknown StokesMethod");
    }
    mgused_ = (C_.StokesMethod ==  11 || C_.StokesMethod ==  12 || C_.StokesMethod ==  21 ||
               C_.StokesMethod ==  22 || C_.StokesMethod ==  31 || C_.StokesMethod ==  32 ||
               C_.StokesMethod ==  41 || C_.StokesMethod ==  42 || C_.StokesMethod ==  51 ||
               C_.StokesMethod ==  52 || C_.StokesMethod == 211 || C_.StokesMethod == 222 ||
               C_.StokesMethod ==  81 || C_.StokesMethod ==  82);
    return stokessolver;
}

template <class StokesT, class ParamsT>
void StokesSolverFactoryCL<StokesT, ParamsT>::SetMatrices(const MatrixCL** A, const MatrixCL* B, const MatrixCL* Mvel, const MatrixCL* M) {
    if (C_.StokesMethod == 81 || C_.StokesMethod == 82) {
        mincommispc_.SetMatrices(A, B, Mvel, M);
        bbtispc_.SetMatrices(B, Mvel, M);
    }
}


} // end of namespace DROPS