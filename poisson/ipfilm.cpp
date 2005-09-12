//**************************************************************************
// File:    ipdrops.cpp                                                    *
// Content: test program for the instat. poisson-problem                   *
// Author:  Sven Gross, Joerg Peters, Volker Reichelt, Marcus Soemers      *
//          IGPM RWTH Aachen                                               *
// Version: 0.1                                                            *
// History: begin - Nov, 19 2002                                           *
//**************************************************************************


#include "poisson/instatpoisson.h"
#include "poisson/integrTime.h"
#include "geom/multigrid.h"
#include "num/solver.h"
#include "out/ensightOut.h"

class ParamCL
{
  public:
    double dx, dy, dz;
    int    nx, ny, nz;
    double Heat;
    double rho, mu, cp, lambda;
    std::string EnsDir, EnsCase;
    
    ParamCL()
      : dx(100), dy(0.3), dz(1), nx(8), ny(2), nz(2),  // in mm
        Heat(5960), rho(866), mu(1.732e-3), cp(1500), lambda(0.26), // in SI
        EnsDir("ensight"), EnsCase("FilmTemp")
      {}
} C;

// du/dt - nu*laplace u + Vel grad u + q*u = f

class PoissonCoeffCL
{
  public:
    static double q(const DROPS::Point3DCL&) { return 0.0; }
    static double f(const DROPS::Point3DCL& p, double t) 
    { 
//        return 0;
        const double u= C.rho*9.81*C.dy*C.dy/2/C.mu*1e-3;
    	return cos((p[0] + t*u)/C.dx*2*M_PI); 
    }
    static DROPS::Point3DCL Vel(const DROPS::Point3DCL& p, double)
    { 
    	DROPS::Point3DCL ret; 
        const double d= p[1]/C.dy,
            u= C.rho*9.81*C.dy*C.dy/2/C.mu*1e-3;
        ret[0]= u*(2-d)*d; // Nusselt
        return ret; 
    } 
};


double Zero(const DROPS::Point2DCL&, double) { return 0.0; }
double Heat(const DROPS::Point2DCL&, double) { return C.Heat/C.lambda*1e-3; }


namespace DROPS 
{

template<class Coeff>
void Strategy(InstatPoissonP1CL<Coeff>& Poisson, double dt, int time_steps,
  double nu, double theta, double tol, int maxiter)
{
  
  MultiGridCL& MG= Poisson.GetMG();
  IdxDescCL& idx= Poisson.idx;
  VecDescCL& x= Poisson.x;
  VecDescCL& b= Poisson.b;
  MatDescCL& A= Poisson.A;
  MatDescCL& M= Poisson.M;
  MatDescCL& U= Poisson.U;

  idx.Set(1);
  Poisson.CreateNumbering(MG.GetLastLevel(), &idx);

  x.SetIdx(&idx); 
  b.SetIdx(&idx);
  A.SetIdx(&idx, &idx);
  M.SetIdx(&idx, &idx);
  U.SetIdx(&idx, &idx);

  std::cerr << "Anzahl der Unbekannten: " <<  Poisson.x.Data.size()
    << std::endl;
  Poisson.SetupInstatSystem(A, M);
  
  SSORPcCL pc(1.0);
  typedef GMResSolverCL<SSORPcCL> SolverT;
  SolverT solver(pc, 50, maxiter, tol);
  InstatPoissonThetaSchemeCL<InstatPoissonP1CL<Coeff>, SolverT>
    ThetaScheme(Poisson, solver, theta, true);

  ThetaScheme.SetTimeStep(dt, nu);

  IdxDescCL ens_idx( 1, 1);
  CreateNumb( MG.GetLastLevel(), ens_idx, MG, NoBndDataCL<>());
  EnsightP2SolOutCL ens( MG, &ens_idx);
  const std::string filename= C.EnsDir + "/" + C.EnsCase;
  const std::string datgeo= filename+".geo", 
                    datT=   filename+".tp";
  ens.CaseBegin( std::string(C.EnsCase+".case").c_str(), time_steps+1);
  ens.DescribeGeom( "Film", datgeo);
  ens.DescribeScalar( "Temperatur", datT, true); 
  ens.putGeom( datgeo);
  ens.putScalar( datT,  Poisson.GetSolution(), 0);
  ens.Commit();

  double average= 0.0;
  for (int step=1;step<=time_steps;step++)
  {
    ThetaScheme.DoStep(x);
    std::cerr << "t= " << Poisson.t << std::endl;
    std::cerr << "Iterationen: " << solver.GetIter() 
      << "\tNorm des Residuums: " << solver.GetResid() << std::endl;
    average+= solver.GetIter();
    ens.putScalar( datT,  Poisson.GetSolution(), step*dt);
    ens.Commit();
  }
  average/= time_steps;
  std::cerr << "Anzahl der Iterationen im Durchschnitt: " << average
            << std::endl;
}

} // end of namespace DROPS



int main()
{
  try
  {
    DROPS::Point3DCL null(0.0);
    DROPS::Point3DCL e1(0.0), e2(0.0), e3(0.0);
    e1[0]= C.dx;
    e2[1]= C.dy;
    e3[2]= C.dz;

    typedef DROPS::InstatPoissonP1CL<PoissonCoeffCL> 
      InstatPoissonOnBrickCL;
    typedef InstatPoissonOnBrickCL MyPoissonCL;
    
    DROPS::BrickBuilderCL brick(null, e1, e2, e3, C.nx, C.ny, C.nz);
  
    double dt;
    int time_steps, brick_ref;
    
    std::cerr << "\nDelta t = "; std::cin >> dt;
    std::cerr << "\nAnzahl der Zeitschritte = "; std::cin >> time_steps;
    std::cerr << "\nAnzahl der Verfeinerungen = "; std::cin >> brick_ref;

    // bnd cond: x=0/dx, y=0/dy, z=0/dz
    const bool isneumann[6]= 
      { false, true, true, true, true, true };
    const DROPS::InstatPoissonBndDataCL::bnd_val_fun bnd_fun[6]=
      { &Zero, &Zero, &Heat, &Zero, &Zero, &Zero};
 
    DROPS::InstatPoissonBndDataCL bdata(6, isneumann, bnd_fun);
    MyPoissonCL prob(brick, PoissonCoeffCL(), bdata);
    DROPS::MultiGridCL& mg = prob.GetMG();
    
    for (int count=1; count<=brick_ref; count++)
    {
      MarkAll(mg);
      mg.Refine();
    }
    mg.SizeInfo(std::cerr);

    // Diffusionskoeffizient
    //double nu= 6.303096;
    double nu= C.lambda/C.rho/C.cp*1e6;
//    double nu= 1e-3;
    // one-step-theta scheme 
    double theta= 1;
  
    // Daten fuer den Loeser
    double tol= 1.0e-10;
    int maxiter= 5000; 

    DROPS::Strategy(prob, dt, time_steps, nu, theta, tol, maxiter);
   
    return 0;
  }
  catch (DROPS::DROPSErrCL err) { err.handle(); }
  
}


