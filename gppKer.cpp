#include <iostream>
#include <cstdlib>
#include <memory>

#include <iomanip>
#include <cmath>
#include <complex>
#include <omp.h>
#include <ctime>
#include <chrono>

using namespace std;
int debug = 0;
#pragma omp declare target
inline void flagOCC_solver(double , std::complex<double>* , int , int , std::complex<double>* , std::complex<double>* , std::complex<double>* , std::complex<double>& , std::complex<double>& , int , int , int , int , int );

void reduce_achstemp(int , int , int* , int , std::complex<double>*, std::complex<double>* , std::complex<double>* , std::complex<double>* ,  int* , int , double* , int );

void ssxt_scht_solver(double , int , int , int , std::complex<double> , std::complex<double> , std::complex<double> , std::complex<double> , std::complex<double> , std::complex<double> , std::complex<double> , std::complex<double>& , std::complex<double>& , std::complex<double> );
#pragma omp end declare target

double local_abs(std::complex<double> compl_num)
{
    double re = real(compl_num) * real(compl_num);
    double im = imag(compl_num) * imag(compl_num);

    double result = sqrt(re+im);
    return result;
}

void ssxt_scht_solver(double wxt, int igp, int my_igp, int ig, std::complex<double> wtilde, std::complex<double> wtilde2, std::complex<double> Omega2, std::complex<double> matngmatmgp, std::complex<double> matngpmatmg, std::complex<double> mygpvar1, std::complex<double> mygpvar2, std::complex<double>& ssxa, std::complex<double>& scha, std::complex<double> I_eps_array_igp_myIgp)
{
    std::complex<double> expr0( 0.0 , 0.0);
    double delw2, scha_mult, ssxcutoff;
    double to1 = 1e-6;
    double sexcut = 4.0;
    double gamma = 0.5;
    double limitone = 1.0/(to1*4.0);
    double limittwo = pow(0.5,2);
    std::complex<double> sch, ssx;

    std::complex<double> wdiff = wxt - wtilde;

    std::complex<double> cden = wdiff;
    double rden = 1/real(cden * conj(cden));
    std::complex<double> delw = wtilde * conj(cden) * rden;
    double delwr = real(delw * conj(delw));
    double wdiffr = real(wdiff * conj(wdiff));

    if((wdiffr > limittwo) && (delwr < limitone))
    {
        sch = delw * I_eps_array_igp_myIgp;
        cden = wxt*wxt;
        rden = real(cden * conj(cden));
        rden = 1.00 / rden;
        ssx = Omega2 * conj(cden) * rden;
    }
    else if (delwr > to1)
    {
        sch = expr0;
        cden = (double) 4.00 * wtilde2 * (delw + (double)0.50);
        rden = real(cden * conj(cden));
        rden = 1.00/rden;
        ssx = -Omega2 * conj(cden) * rden * delw;
    }
    else
    {
        sch = expr0;
        ssx = expr0;
    }

    ssxcutoff = sexcut*std::abs(I_eps_array_igp_myIgp);
    if((std::abs(ssx) > ssxcutoff) && (std::abs(wxt) < 0.00)) ssx = 0.00;

//    ssxa = matngmatmgp*ssx;
//    scha = matngmatmgp*sch;
}

void reduce_achstemp(int n1, int number_bands, int* inv_igp_index, int ncouls, std::complex<double> *aqsmtemp, std::complex<double> *aqsntemp, std::complex<double> *I_eps_array, std::complex<double>* achstemp,  int* indinv, int ngpown, double* vcoul, int numThreads)
{
    double to1 = 1e-6;
    std::complex<double> schstemp(0.0, 0.0);;
    std::complex<double> achtemp_loc(0.00, 0.00);

//#pragma omp parallel for default(shared) schedule(dynamic)
    for(int my_igp = 0; my_igp< ngpown; my_igp++)
    {
        int tid = omp_get_thread_num();
        std::complex<double> schs(0.0, 0.0);
        std::complex<double> matngmatmgp(0.0, 0.0);
        std::complex<double> matngpmatmg(0.0, 0.0);
        std::complex<double> halfinvwtilde, delw, ssx, sch, wdiff, cden , eden, mygpvar1, mygpvar2;
        int indigp = inv_igp_index[my_igp];
        int igp = indinv[indigp];
        if(indigp == ncouls)
            igp = ncouls-1;

        if(!(igp > ncouls || igp < 0)){


            std::complex<double> mygpvar1 = std::conj(aqsmtemp[n1*ncouls+igp]);
            std::complex<double> mygpvar2 = aqsntemp[n1*ncouls+igp];

            schs = I_eps_array[my_igp*ncouls+igp];
            matngmatmgp = aqsntemp[n1*ncouls+igp] * mygpvar1;

            if(std::abs(schs) > to1)
                schstemp = schstemp + matngmatmgp * schs;
        }
        else
        {
            for(int ig=1; ig<ncouls; ++ig)
                schstemp = schstemp - aqsntemp[n1*ncouls+igp] * I_eps_array[my_igp*ncouls+ig] * mygpvar1;
        }

        achtemp_loc += schstemp * vcoul[igp] *(double) 0.5;
    }

//    achstemp = achtemp_loc;
}

inline void flagOCC_solver(double wxt, std::complex<double> *wtilde_array, int my_igp, int n1, std::complex<double> *aqsmtemp, std::complex<double> *aqsntemp, std::complex<double> *I_eps_array, std::complex<double> &ssxt, std::complex<double> &scht,int ncouls, int igp, int number_bands, int ngpown)
{
    std::complex<double> matngmatmgp = std::complex<double>(0.0, 0.0);
    std::complex<double> matngpmatmg = std::complex<double>(0.0, 0.0);
//    std::complex<double> ssxa[ncouls];
//    std::complex<double> scha[ncouls];

    for(int ig=0; ig<ncouls; ++ig)
    {
        std::complex<double> wtilde = wtilde_array[my_igp*ncouls+ig];
        std::complex<double> wtilde2 = wtilde*wtilde;
        std::complex<double> Omega2 = wtilde2*I_eps_array[my_igp*ncouls+ig];
        std::complex<double> mygpvar1 = std::conj(aqsmtemp[n1*ncouls+igp]);
        std::complex<double> mygpvar2 = aqsmtemp[n1*ncouls+igp];
        std::complex<double> matngmatmgp = aqsntemp[n1*ncouls+ig] * mygpvar1;
        if(ig != igp) matngpmatmg = std::conj(aqsmtemp[n1*ncouls+ig]) * mygpvar2;
        std::complex<double> expr0( 0.0 , 0.0);
        double delw2, scha_mult, ssxcutoff;
        double to1 = 1e-6;
        double sexcut = 4.0;
        double gamma = 0.5;
        double limitone = 1.0/(to1*4.0);
        double limittwo = pow(0.5,2);
        std::complex<double> sch, ssx;
        std::complex<double> wdiff = wxt - wtilde;

        std::complex<double> cden = wdiff;
        double rden = 1/real(cden * conj(cden));
        std::complex<double> delw = wtilde * conj(cden) * rden;
        double delwr = real(delw * conj(delw));
        double wdiffr = real(wdiff * conj(wdiff));
        if((wdiffr > limittwo) && (delwr < limitone))
        {
            sch = delw * I_eps_array[my_igp*ncouls+ig];
            cden = wxt*wxt;
            rden = real(cden * conj(cden));
            rden = 1.00 / rden;
            ssx = Omega2 * conj(cden) * rden;
        }
        else if (delwr > to1)
        {
            sch = expr0;
            cden = (double) 4.00 * wtilde2 * (delw + (double)0.50);
            rden = real(cden * conj(cden));
            rden = 1.00/rden;
            ssx = -Omega2 * conj(cden) * rden * delw;
        }
        else
        {
            sch = expr0;
            ssx = expr0;
        }

        ssxcutoff = sexcut*std::abs(I_eps_array[my_igp*ncouls+ig]);
        if((std::abs(ssx) > ssxcutoff) && (std::abs(wxt) < 0.00)) ssx = 0.00;

//        ssxt_scht_solver(wxt, igp, my_igp, ig, wtilde, wtilde2, Omega2, matngmatmgp, matngpmatmg, mygpvar1, mygpvar2, ssxa[ig], scha[ig], I_eps_array[my_igp*ncouls+ig]);
//
//        ssxt += matngmatmgp*ssx;
//        scht += matngmatmgp*sch;

    }
}

int main(int argc, char** argv)
{

    if (argc != 5)
    {
        std::cout << "The correct form of input is : " << endl;
        std::cout << " ./a.out <number_bands> <number_valence_bands> <number_plane_waves> <nodes_per_mpi_group> " << endl;
        exit (0);
    }
    int number_bands = atoi(argv[1]);
    int nvband = atoi(argv[2]);
    int ncouls = atoi(argv[3]);
    int nodes_per_group = atoi(argv[4]);

    int npes = 1; //Represents the number of ranks per node
    int ngpown = ncouls / (nodes_per_group * npes); //Number of gvectors per mpi task

    double e_lk = 10;
    double dw = 1;
    int nstart = 0, nend = 3;

    int inv_igp_index[ngpown];
    int indinv[ncouls];
    //OpenMP Printing of threads on Host and Device
    int tid, numThreads, numTeams;
#pragma omp parallel shared(numThreads) private(tid)
    {
        tid = omp_get_thread_num();
        if(tid == 0)
            numThreads = omp_get_num_threads();
    }
    std::cout << "Number of OpenMP Threads = " << numThreads << endl;

#pragma omp target map(tofrom: numTeams, numThreads)
#pragma omp teams shared(numTeams) private(tid)
    {
        tid = omp_get_team_num();
        if(tid == 0)
        {
            numTeams = omp_get_num_teams();
#pragma omp parallel 
            {
                int ttid = omp_get_thread_num();
                if(ttid == 0)
                    numThreads = omp_get_num_threads();
            }
        }
    }
    std::cout << "Number of OpenMP Teams = " << numTeams << std::endl;
    std::cout << "Number of OpenMP DEVICE Threads = " << numThreads << std::endl;


    double to1 = 1e-6, \
    gamma = 0.5, \
    sexcut = 4.0;
    double limitone = 1.0/(to1*4.0), \
    limittwo = pow(0.5,2);

    double e_n1kq= 6.0; //This in the fortran code is derived through the double dimenrsion array ekq whose 2nd dimension is 1 and all the elements in the array have the same value

    //Printing out the params passed.
    std::cout << "number_bands = " << number_bands \
        << "\t nvband = " << nvband \
        << "\t ncouls = " << ncouls \
        << "\t nodes_per_group  = " << nodes_per_group \
        << "\t ngpown = " << ngpown \
        << "\t nend = " << nend \
        << "\t nstart = " << nstart \
        << "\t gamma = " << gamma \
        << "\t sexcut = " << sexcut \
        << "\t limitone = " << limitone \
        << "\t limittwo = " << limittwo << endl;

    std::complex<double> expr0( 0.0 , 0.0);
    std::complex<double> expr( 0.5 , 0.5);

    std::complex<double> *acht_n1_loc = new std::complex<double>[number_bands];
//    std::complex<double> *acht_n1_loc_threadArr = new std::complex<double> [numThreads*number_bands];

    std::complex<double> *achtemp = new std::complex<double>[nend-nstart];
//    std::complex<double> *achtemp_threadArr = new std::complex<double> [numThreads*(nend-nstart)];

    std::complex<double> *aqsmtemp = new std::complex<double> [number_bands*ncouls];

    std::complex<double> *aqsntemp = new std::complex<double> [number_bands*ncouls];

    std::complex<double> *I_eps_array = new std::complex<double> [ngpown*ncouls];

    std::complex<double> *wtilde_array = new std::complex<double> [ngpown*ncouls];

    std::complex<double> *asxtemp = new std::complex<double>[nend-nstart];
//    std::complex<double> *asxtemp_threadArr = new std::complex<double> [numThreads*(nend-nstart)];

    double *vcoul = new double[ncouls];
    double wx_array[3];

//    std::complex<double> achstemp;
//    std::complex<double> *achstemp = new std::complex<double>;
    std::complex<double> *ssx_array = new std::complex<double>[3];

    std::complex<double> sch_array[3];

    double occ=1.0;
    bool flag_occ;

    cout << "Size of wtilde_array = " << (ncouls*ngpown*2.0*8) / pow(1024,2) << " Mbytes" << endl;
    cout << "Size of aqsntemp = " << (ncouls*number_bands*2.0*8) / pow(1024,2) << " Mbytes" << endl;
    cout << "Size of I_eps_array array = " << (ncouls*ngpown*2.0*8) / pow(1024,2) << " Mbytes" << endl;

   for(int i=0; i<number_bands; i++)
       for(int j=0; j<ncouls; j++)
       {
           aqsmtemp[i*ncouls+j] = expr;
           aqsntemp[i*ncouls+j] = expr;
       }

   for(int i=0; i<ngpown; i++)
       for(int j=0; j<ncouls; j++)
       {
           I_eps_array[i*ncouls+j] = expr;
           wtilde_array[i*ncouls+j] = expr;
       }

   for(int i=0; i<ncouls; i++)
       vcoul[i] = 1.0;


    for(int ig=0, tmp=1; ig < ngpown; ++ig,tmp++)
        inv_igp_index[ig] = (ig+1) * ncouls / ngpown;

    //Do not know yet what this array represents
    for(int ig=0, tmp=1; ig<ncouls; ++ig,tmp++)
        indinv[ig] = ig;

    auto start_chrono = std::chrono::high_resolution_clock::now();

#pragma omp target enter data map(alloc:acht_n1_loc[0:number_bands], achtemp[0:(nend-nstart)], aqsmtemp[0:number_bands*ncouls],aqsntemp[0:number_bands*ncouls], I_eps_array[0:ngpown*ncouls], wtilde_array[0:ngpown*ncouls], vcoul[0:ncouls], inv_igp_index[0:ngpown], indinv[0:ncouls], asxtemp[0:(nend-nstart)])

#pragma omp target update to(acht_n1_loc[0:number_bands], achtemp[0:(nend-nstart)], aqsmtemp[0:number_bands*ncouls],aqsntemp[0:number_bands*ncouls], I_eps_array[0:ngpown*ncouls], wtilde_array[0:ngpown*ncouls], vcoul[0:ncouls], inv_igp_index[0:ngpown], indinv[0:ncouls], asxtemp[0:(nend-nstart)])

#pragma omp target map(to:inv_igp_index[0:ngpown], indinv[0:ncouls], vcoul[0:ncouls],ssx_array[0:3], aqsmtemp[0:number_bands*ncouls], wtilde_array[0:ngpown*ncouls], aqsntemp[0:number_bands*ncouls], I_eps_array[0:ngpown*ncouls]) map(tofrom:achtemp[0:(nend-nstart)], asxtemp[0:(nend-nstart)], acht_n1_loc[0:number_bands] )
{
#pragma omp teams distribute shared(vcoul, aqsntemp, aqsmtemp, I_eps_array) //private(sch_array, ssx_array) 
    for(int n1 = 0; n1<number_bands; ++n1) // This for loop at the end cheddam
    {
        flag_occ = n1 < nvband;
        std::complex<double> achstemp;

//        reduce_achstemp(n1, number_bands, inv_igp_index, ncouls,aqsmtemp, aqsntemp, I_eps_array, achstemp, indinv, ngpown, vcoul, numThreads);
        {
            double to1 = 1e-6;
            std::complex<double> schstemp(0.0, 0.0);;
            std::complex<double> achtemp_loc(0.00, 0.00);
        
        //#pragma omp parallel for default(shared) schedule(dynamic)
            for(int my_igp = 0; my_igp< ngpown; my_igp++)
            {
                int tid = omp_get_thread_num();
                std::complex<double> schs(0.0, 0.0);
                std::complex<double> matngmatmgp(0.0, 0.0);
                std::complex<double> matngpmatmg(0.0, 0.0);
                std::complex<double> halfinvwtilde, delw, ssx, sch, wdiff, cden , eden, mygpvar1, mygpvar2;
                int indigp = inv_igp_index[my_igp];
                int igp = indinv[indigp];
                if(indigp == ncouls)
                    igp = ncouls-1;
        
                if(!(igp > ncouls || igp < 0)){
        
        
                    std::complex<double> mygpvar1 = std::conj(aqsmtemp[n1*ncouls+igp]);
                    std::complex<double> mygpvar2 = aqsntemp[n1*ncouls+igp];
        
                    schs = I_eps_array[my_igp*ncouls+igp];
                    matngmatmgp = aqsntemp[n1*ncouls+igp] * mygpvar1;
        
                    if(abs(schs) > to1)
                        schstemp = schstemp + matngmatmgp * schs;
                }
                else
                {
                    for(int ig=1; ig<ncouls; ++ig)
                        schstemp = schstemp - aqsntemp[n1*ncouls+igp] * I_eps_array[my_igp*ncouls+ig] * mygpvar1;
                }
        
                achstemp += schstemp * vcoul[igp] *(double) 0.5;
            }
        }

        for(int iw=nstart; iw<nend; ++iw)
        {
            wx_array[iw] = e_lk - e_n1kq + dw*((iw+1)-2);
            if(abs(wx_array[iw]) < to1) wx_array[iw] = to1;
        }

#pragma omp parallel for firstprivate(sch_array) schedule(static)
        for(int my_igp=0; my_igp<ngpown; ++my_igp)
        {
            tid = omp_get_thread_num();
            std::complex<double> scht, ssxt;
            int indigp = inv_igp_index[my_igp];
            int igp = indinv[indigp];
            if(indigp == ncouls)
                igp = ncouls-1;
            double wxt;

            if(!(igp > ncouls || igp < 0)) {

           for(int i=0; i<3; i++)
           {
               ssx_array[i] = expr0;
               sch_array[i] = expr0;
           }

           if(flag_occ)
           {
               for(int iw=nstart; iw<nend; iw++)
               {
                   scht = ssxt = expr0;
                   wxt = wx_array[iw];
//                   flagOCC_solver(wxt, wtilde_array, my_igp, n1, aqsmtemp, aqsntemp, I_eps_array, ssxt, scht, ncouls, igp, number_bands, ngpown);
                    {
                        std::complex<double> matngmatmgp = std::complex<double>(0.0, 0.0);
                        std::complex<double> matngpmatmg = std::complex<double>(0.0, 0.0);
                        for(int ig=0; ig<ncouls; ++ig)
                        {
                            std::complex<double> wtilde = wtilde_array[my_igp*ncouls+ig];
                            std::complex<double> wtilde2 = wtilde*wtilde;
                            std::complex<double> Omega2 = wtilde2*I_eps_array[my_igp*ncouls+ig];
                            std::complex<double> mygpvar1 = std::conj(aqsmtemp[n1*ncouls+igp]);
                            std::complex<double> mygpvar2 = aqsmtemp[n1*ncouls+igp];
                            std::complex<double> matngmatmgp = aqsntemp[n1*ncouls+ig] * mygpvar1;
                            if(ig != igp) matngpmatmg = std::conj(aqsmtemp[n1*ncouls+ig]) * mygpvar2;
                            std::complex<double> expr0( 0.0 , 0.0);
                            double delw2, scha_mult, ssxcutoff;
                            double to1 = 1e-6;
                            double sexcut = 4.0;
                            double gamma = 0.5;
                            double limitone = 1.0/(to1*4.0);
                            double limittwo = pow(0.5,2);
                            std::complex<double> sch, ssx;
                            std::complex<double> wdiff = wxt - wtilde;
                    
                            std::complex<double> cden = wdiff;
                            double rden = 1/real(cden * conj(cden));
                            std::complex<double> delw = wtilde * conj(cden) * rden;
                            double delwr = real(delw * conj(delw));
                            double wdiffr = real(wdiff * conj(wdiff));
                            if((wdiffr > limittwo) && (delwr < limitone))
                            {
                                sch = delw * I_eps_array[my_igp*ncouls+ig];
                                cden = wxt*wxt;
                                rden = real(cden * conj(cden));
                                rden = 1.00 / rden;
                                ssx = Omega2 * conj(cden) * rden;
                            }
                            else if (delwr > to1)
                            {
                                sch = expr0;
                                cden = (double) 4.00 * wtilde2 * (delw + (double)0.50);
                                rden = real(cden * conj(cden));
                                rden = 1.00/rden;
                                ssx = -Omega2 * conj(cden) * rden * delw;
                            }
                            else
                            {
                                sch = expr0;
                                ssx = expr0;
                            }
                    
                            ssxcutoff = sexcut*std::abs(I_eps_array[my_igp*ncouls+ig]);
                            if((std::abs(ssx) > ssxcutoff) && (std::abs(wxt) < 0.00)) ssx = 0.00;
                    
                            ssxt += matngmatmgp*ssx;
                            scht += matngmatmgp*sch;
                        }
                    }

                   ssx_array[iw] += ssxt;
                   sch_array[iw] +=(double) 0.5*scht;
               }
           }
           else
           {
               int igblk = 512, numBlock=0;;
               std::complex<double> mygpvar1 = std::conj(aqsmtemp[n1*ncouls+igp]);
               std::complex<double> cden, wdiff, delw;
               double delwr, wdiffr, rden; //rden

            for(int igbeg=0; igbeg<ncouls; igbeg+=igblk)
            {
                scht = ssxt = expr0;
                int igend = min(igblk, ncouls-igbeg);
 //               int igend = min(igbeg+igblk, ncouls);
                for(int iw=nstart; iw<nend; ++iw)
                {
                    wxt = wx_array[iw];
                    std::complex<double> scha[igblk];
                    int sch_cntr = 0;
#pragma omp simd
                    for(int ig = 0; ig<igend; ++ig)
                    {
                       wdiff = wxt - wtilde_array[my_igp*ncouls+(ig + numBlock*igblk)];
                       cden = wdiff;
                       rden = std::real(cden * std::conj(cden));
                       rden = 1/rden;
                       delw = wtilde_array[my_igp*ncouls+(ig + numBlock*igblk)] * conj(cden) * rden ; //*rden
                       delwr = std::real(delw*std::conj(delw));
                       wdiffr = std::real(wdiff*std::conj(wdiff));

//                        if ((wdiffr > limittwo) && (delwr < limitone))
                        if ((wdiffr > limittwo) )
                            scha[ig] = mygpvar1 * aqsntemp[n1*ncouls+(ig + numBlock*igblk)] * delw * I_eps_array[my_igp*ncouls+(ig + numBlock*igblk)];
                            else 
                                scha[ig] = expr0;
                    }
                    for(int ig = 0; ig < igblk; ++ig)
                        scht += scha[ig];
                   
                   sch_array[iw] +=(double) 0.5*scht;
                }
            }


//               std::complex<double> scha[ncouls];
//               for(int iw=nstart; iw<nend; ++iw)
//               {
//                   scht = ssxt = expr0;
//                   wxt = wx_array[iw];
//
//#pragma omp simd
//                   for(int ig = 0; ig<ncouls; ++ig)
//                   { 
//                       wdiff = wxt - wtilde_array[my_igp*ncouls+ig];
//                       cden = wdiff;
//                       rden = std::real(cden * std::conj(cden));
//                       rden = 1/rden;
//                       delw = wtilde_array[my_igp*ncouls+ig] * conj(cden) * rden ; //*rden
//                       delwr = std::real(delw*std::conj(delw));
//                       wdiffr = std::real(wdiff*std::conj(wdiff));
//
////                        if ((wdiffr > limittwo) && (delwr < limitone))
//                        if ((wdiffr > limittwo) )
//                            scha[ig] = mygpvar1 * aqsntemp[n1*ncouls+ig] * delw * I_eps_array[my_igp*ncouls+ig];
//                            else 
//                                scha[ig] = expr0;
//                   }
//                    for(int ig = 0; ig<ncouls; ++ig)
//                            scht += scha[ig];
//
//                   sch_array[iw] +=(double) 0.5*scht;
//                }
           }

           if(flag_occ)
               for(int iw=nstart; iw<nend; ++iw)
                   asxtemp[iw] += ssx_array[iw] * occ * vcoul[igp];

#pragma omp critical
{
            for(int iw=nstart; iw<nend; ++iw)
                achtemp[iw] += sch_array[iw] * vcoul[igp];

            acht_n1_loc[n1] += sch_array[2] * vcoul[igp];
}

            } //for the if-loop to avoid break inside an openmp pragma statment
        } //ngpown
    } // number-bands
} // Target
#pragma omp target update from (acht_n1_loc[0:number_bands], achtemp[0:(nend-nstart)], asxtemp[0:(nend-nstart)])


    std::chrono::duration<double> elapsed_chrono = std::chrono::high_resolution_clock::now() - start_chrono;

    for(int iw=nstart; iw<nend; ++iw)
        cout << "achtemp[" << iw << "] = " << std::setprecision(15) << achtemp[iw] << endl;

    cout << "********** Chrono Time Taken **********= " << elapsed_chrono.count() << " secs" << endl;



    free(acht_n1_loc);
    free(achtemp);
    free(aqsmtemp);
    free(aqsntemp);
    free(I_eps_array);
    free(wtilde_array);
    free(asxtemp);
    free(vcoul);
    free(ssx_array);

    return 0;
}

//Almost done code
