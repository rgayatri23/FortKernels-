#include <iostream>
#include <cstdlib>

#include <iomanip>
#include <cmath>
#include <complex>
#include <omp.h>

#include <Kokkos_Core.hpp>
#include <Kokkos_Complex.hpp>
using namespace std;


#define CUDASPACE 0
#define OPENMPSPACE 1
#define CUDAUVM 0
#define SERIAL 0
#define THREADS 0

#if OPENMPSPACE
        typedef Kokkos::OpenMP   ExecSpace;
        typedef Kokkos::OpenMP        MemSpace;
        typedef Kokkos::LayoutRight  Layout;
#endif

#if CUDASPACE
        typedef Kokkos::Cuda     ExecSpace;
        typedef Kokkos::CudaSpace     MemSpace;
        typedef Kokkos::LayoutLeft   Layout;
#endif

#if SERIAL
        typedef Kokkos::Serial   ExecSpace;
        typedef Kokkos::HostSpace     MemSpace;
#endif

#if THREADS
        typedef Kokkos::Threads  ExecSpace;
        typedef Kokkos::HostSpace     MemSpace;
#endif

#if CUDAUVM
        typedef Kokkos::Cuda     ExecSpace;
        typedef Kokkos::CudaUVMSpace  MemSpace;
        typedef Kokkos::LayoutLeft   Layout;
#endif

typedef Kokkos::RangePolicy<ExecSpace>  range_policy;

typedef Kokkos::View<Kokkos::complex<double>, Layout, MemSpace>   ViewScalarTypeComplex;
typedef Kokkos::View<Kokkos::complex<double>*, Layout, MemSpace>   ViewVectorTypeComplex;
typedef Kokkos::View<Kokkos::complex<double>**, Layout, MemSpace>  ViewMatrixTypeComplex;
typedef Kokkos::View<int*, Layout, MemSpace>   ViewVectorTypeInt;
typedef Kokkos::View<double*, Layout, MemSpace>   ViewVectorTypeDouble;


KOKKOS_INLINE_FUNCTION
void flagOCC_solver(double wxt, Kokkos::View<Kokkos::complex<double>** > wtilde_array, int my_igp, int n1, Kokkos::View<Kokkos::complex<double>** > aqsmtemp, Kokkos::View<Kokkos::complex<double>** > aqsntemp, Kokkos::View<Kokkos::complex<double>** > I_eps_array, Kokkos::complex<double> &ssxt, Kokkos::complex<double> &scht, int ncouls, int igp);

KOKKOS_INLINE_FUNCTION
void reduce_achstemp(int n1, Kokkos::View<int*> inv_igp_index, int ncouls, Kokkos::View<Kokkos::complex<double>** > aqsmtemp, Kokkos::View<Kokkos::complex<double>** > aqsntemp, Kokkos::View<Kokkos::complex<double>** > I_eps_array, Kokkos::complex<double>& achstemp, Kokkos::View<int*> indinv, int ngpown, Kokkos::View<double*> vcoul);

KOKKOS_INLINE_FUNCTION
Kokkos::complex<double> doubleMultKokkosComplex(double op1, Kokkos::complex<double> op2);

KOKKOS_INLINE_FUNCTION
Kokkos::complex<double> doublePlusKokkosComplex(double op1, Kokkos::complex<double> op2);

KOKKOS_INLINE_FUNCTION
Kokkos::complex<double> doubleMinusKokkosComplex(double op1, Kokkos::complex<double> op2);

KOKKOS_INLINE_FUNCTION
Kokkos::complex<double> kokkos_square(Kokkos::complex<double> compl_num, int n);
