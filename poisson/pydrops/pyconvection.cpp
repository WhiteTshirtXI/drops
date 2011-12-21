#include "Python.h"
#include "numpy/arrayobject.h"	/* NumPy header */

#include <boost/python.hpp>
#include <boost/python/numeric.hpp>
#include <boost/python/tuple.hpp>
#include <boost/python/extract.hpp>
#include <boost/shared_ptr.hpp>

#include "misc/params.h"

#include "pyconnect.h"
#include "convection_diffusion.cpp"
#include "pypdefunction.h"

bool check_dimension(const PdeFunction& f, int Nx, int Ny, int Nz, int Nt)
{
  return f.get_dimensions(Nx, Ny, Nz, Nt);
}

bool check_dimensions(int Nx, int Ny, int Nz, int Nt, const PdeFunction& C0, const PdeFunction& b_in, const PdeFunction& b_interface, const PdeFunction& source, const PdeFunction& Dw)
{
  if (check_dimension(C0, Nx, Ny, Nz, 1) &&
      check_dimension(b_in, 1, Ny, Nz, Nt) &&
      check_dimension(b_interface, Nx, 1, Nz, Nt) &&
      check_dimension(source, Nx, Ny, Nz, Nt) &&
      check_dimension(Dw, Nx, Ny, Nz, Nt)) {
    return true;
  }
  return false;
}


using namespace boost::python::numeric;

array numpy_convection_diffusion(array& C0, array& b_in, array& source, array& Dw, array& b_interface, string json_filename) {
  // 1. Read parameter file
  std::ifstream param;
  param.open(json_filename.c_str());
  /*    else
	param.open( argv[1]);
	if (!param){
	std::cerr << "error while opening parameter file\n";
	return 1;
	}*/
  param >> P;
  param.close();
  using namespace boost::python;
  tuple t = extract<tuple>(source.attr("shape"));
  int nx = extract<int>(t[0]);
  int ny = extract<int>(t[1]);
  int nz = extract<int>(t[2]);
  int nt = extract<int>(t[3]);
  P.put<std::string>("PoissonCoeff.Reaction", "Zero");
  P.put<int>("Poisson.SolutionIsKnown",0);
  P.put<int>("DomainCond.RefineSteps", 0);
  P.put<std::string>("DomainCond.BoundaryType","2!21!21!2!21!21");
  P.put<int>("DomainCond.GeomType", 1);

  int nt_test = P.get<int>("Time.NumSteps"); // again, number of intervals
  assert(nt_test==nt-1);

  /* Print out parameters */
  std::cout << P << std::endl;

  /* Convert numpy arrays to PyPdeFunctions */
  typedef const PdeFunction* PdeFunPtr;
  PdeFunPtr C0f(new PyPdeBoundaryFunction(C0,3));
  PdeFunPtr b_inf(new PyPdeBoundaryFunction(b_in,0));
  PdeFunPtr b_interfacef(new PyPdeBoundaryFunction(b_interface,1));
  PdeFunPtr sourcef(new PyPdeFunction(source));
  PdeFunPtr Dwf(new PyPdeFunction(Dw));

  if (!check_dimensions(nx,ny,nz,nt,*C0f,*b_inf,*b_interfacef,*sourcef,*Dwf)) {
    throw DROPS::DROPSErrCL("Error in setting up DROPS: Wrong dimensions in inputs!");
  }
  npy_intp* c_sol_dim = new npy_intp[4];
  c_sol_dim[0] = nx; c_sol_dim[1] = ny; c_sol_dim[2] = nz; c_sol_dim[3] = nt;
  PyArrayObject* newarray = (PyArrayObject*) PyArray_New(&PyArray_Type, 4, c_sol_dim, PyArray_DOUBLE, NULL, NULL, 0, NPY_F_CONTIGUOUS, NULL);
  boost::python::object obj(handle<>((PyObject*)newarray));
  double* solution_ptr = (double*)newarray->data;
  for (int k=0; k<nx*ny*nz*nt; ++k) { solution_ptr[k] = -1.2345;} // for testing purposes
  array solution = extract<boost::python::numeric::array>(obj);
  convection_diffusion(P, C0f, b_inf, b_interfacef, sourcef, Dwf, solution_ptr);

  delete C0f;
  delete b_inf;
  delete b_interfacef;
  delete sourcef;
  delete Dwf;
  return solution;
}
/*
bool py_convection_diffusion(PdeFunction& C0, PdeFunction& b_in, PdeFunction& b_interface, PdeFunction& source, PdeFunction& Dw)
{
  try {
    using namespace DROPS;

    std::ifstream param;
    std::cout << "Using default parameter file: poissonex1.json\n";
    param.open( "poissonex1.json");
        else
	  param.open( argv[1]);
	  if (!param){
	  std::cerr << "error while opening parameter file\n";
	  return 1;
	  }
    param >> P;
    param.close();
    std::cout << P << std::endl;

    // set up data structure to represent a poisson problem
    // ---------------------------------------------------------------------
    std::cout << line << "Set up data structure to represent a Poisson problem ...\n";
    int Nx, Ny, Nz, Ns, Nt, N;

    Nx = P.get<int>("DomainCond.nx")+1;
    Ny = P.get<int>("DomainCond.ny")+1;
    Nz = P.get<int>("DomainCond.nz")+1;
    Ns = Nx*Ny*Nz;
    Nt = P.get<int>("Time.NumSteps")+1;
    N  = Ns*Nt;

    if (check_dimensions(Nx,Ny,Nz,Nt,C0,b_in,b_interface,source,Dw)) {
      convection_diffusion(P, &C0, &b_in, &b_interface, &source, &Dw, NULL);
    }
    else {
      return false;
    }
    return true;
  }
  catch (DROPS::DROPSErrCL err) { err.handle(); }
  return false;
}
*/

#include "prepy_product.cpp"


BOOST_PYTHON_MODULE(drops)
{
  import_array();		/* Initialize the Numarray module. */
  using namespace boost::python;
  boost::python::numeric::array::set_module_and_type("numpy", "ndarray");
  def("convection_diffusion", numpy_convection_diffusion);
  //def("get_array", &get_array);
  //def("setArray", &setArray);

  def("setup_scalar_product_matrices", setup_sp_matrices);
  def("scalar_product", numpy_scalar_product);

  class_<PyPdeFunction>("PyPdeFunction", init<numeric::array&>(args("x"), "__init__ docstring"))
    .def(init<numeric::array&>())
    .def("at",&PyPdeFunction::at);
}
