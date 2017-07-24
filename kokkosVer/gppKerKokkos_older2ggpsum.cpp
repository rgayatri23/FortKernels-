#include <iostream>
#include <cstdlib>

#include <iomanip>
#include <cmath>
#include <complex>
#include <omp.h>

#include <Kokkos_Core.hpp>
#include <Kokkos_Complex.hpp>
using namespace std;

KOKKOS_INLINE_FUNCTION
Kokkos::complex<double> kokkos_square(Kokkos::complex<double> compl_num, int n)
{
    std::complex<double> powerExpr(Kokkos::real(compl_num), Kokkos::imag(compl_num));
    powerExpr = std::pow(powerExpr,n);

    return powerExpr;
}

KOKKOS_INLINE_FUNCTION
Kokkos::complex<double> doubleMinusKokkosComplex(double op1, Kokkos::complex<double> op2)
{
    Kokkos::complex<double> expr((op1 - Kokkos::real(op2)), (0 - Kokkos::imag(op2)));
    return expr;
}

KOKKOS_INLINE_FUNCTION
Kokkos::complex<double> doublePlusKokkosComplex(double op1, Kokkos::complex<double> op2)
{
    Kokkos::complex<double> expr((op1 + Kokkos::real(op2)), (0 + Kokkos::imag(op2)));
    return expr;
}

KOKKOS_INLINE_FUNCTION
Kokkos::complex<double> doubleMultKokkosComplex(double op1, Kokkos::complex<double> op2)
{
    Kokkos::complex<double> expr((op1 * Kokkos::real(op2)), (0 * Kokkos::imag(op2)));
    return expr;
}
int main(int argc, char** argv)
{
    Kokkos::initialize(argc, argv);
    {

    if (argc != 6)
    {
        std::cout << "The correct form of input is : " << endl;
        std::cout << " ./a.out <number_bands> <number_valence_bands> <number_plane_waves> <nodes_per_mpi_group> <gppsum> " << endl;
        exit (0);
    }
    int number_bands = atoi(argv[1]);
    int nvband = atoi(argv[2]);
    int ncouls = atoi(argv[3]);
    int nodes_per_group = atoi(argv[4]);
    int gppsum = atoi(argv[5]);

    int igmax = ncouls;

    int npes = 1; //Represents the number of ranks per node
    int ngpown = ncouls / (nodes_per_group * npes); //Number of gvectors per mpi task

    double e_lk = 10;
    double dw = 1;
    int nstart = 0, nend = 3;

//    int inv_igp_index[ngpown];
//    int indinv[ncouls];


    double to1 = 1e-6;
//    std::cout << setprecision(16) << "to1 = " << to1 << endl;

    double gamma = 0.5;
    double sexcut = 4.0;
    double limitone = 1.0/(to1*4.0);
    double limittwo = pow(0.5,2);

    double e_n1kq= 6.0; //This in the fortran code is derived through the double dimenrsion array ekq whose 2nd dimension is 1 and all the elements in the array have the same value


    //Printing out the params passed.
    std::cout << "number_bands = " << number_bands \
        << "\t nvband = " << nvband \
        << "\t ncouls = " << ncouls \
        << "\t nodes_per_group  = " << nodes_per_group \
        << "\t gppsum = " << gppsum \
        << "\t ngpown = " << ngpown \
        << "\t nend = " << nend \
        << "\t nstart = " << nstart \
        << "\t gamma = " << gamma \
        << "\t sexcut = " << sexcut \
        << "\t limitone = " << limitone \
        << "\t limittwo = " << limittwo << endl;


    double ssxcutoff;
    double wxt, delw2, delwr, wdiffr, scha_mult, rden;
    double occ=1.0;
    bool flag_occ;

    /********************KOKKOS RELATED VARS AND VIEWS ***************************/
    Kokkos::complex<double> expr(0.5 , 0.5);
    Kokkos::complex<double> expr0(0.0 , 0.0);
    Kokkos::complex<double> achstemp = expr0;
    Kokkos::complex<double> achstemp_tmp = expr0;

    Kokkos::View<int*> inv_igp_index("inv_igp_index", ngpown);
    Kokkos::View<int*> indinv("indinv", ncouls);
    Kokkos::View<double*> vcoul ("vcoul", ncouls);

    Kokkos::View<Kokkos::complex<double>** > aqsntemp ("aqsntemp", ncouls, number_bands);
    Kokkos::View<Kokkos::complex<double>** > aqsmtemp ("aqsmtemp", ncouls, number_bands);
    Kokkos::View<complex<double>** > I_eps_array ("I_eps_array", ncouls, ngpown);
    Kokkos::View<complex<double>** > wtilde_array ("wtilde_array", ncouls, ngpown);

    Kokkos::View<complex<double> *> asxtemp ("asxtemp", nend-nstart);
    Kokkos::View<complex<double> *> acht_n1_loc(" acht_n1_loc", number_bands);
    Kokkos::View<complex<double> *> ssxa(" ssxa", ncouls);
    
   for(int i=0; i<ncouls; i++)
   {
       for(int j=0; j<number_bands; j++)
       {
           aqsmtemp(i,j) = expr;
           aqsntemp(i,j) = expr;
       }

       for(int j=0; j<ngpown; j++)
       {
           I_eps_array(i,j) = expr;
           wtilde_array(i,j) = expr;
       }

       vcoul(i) = 1.0;
   }

    cout << "Size of wtilde_array = " << (ncouls*ngpown*2.0*8) / pow(1024,2) << " Mbytes" << endl;
    cout << "Size of aqsntemp = " << (ncouls*number_bands*2.0*8) / pow(1024,2) << " Mbytes" << endl;
    cout << "Size of I_eps_array array = " << (ncouls*ngpown*2.0*8) / pow(1024,2) << " Mbytes" << endl;

    //For MPI Work distribution
    for(int ig=0; ig < ngpown; ++ig)
        inv_igp_index(ig) = (ig+1) * ncouls / ngpown;

    //Do not know yet what this array represents
    for(int ig=0; ig<ncouls; ++ig)
        indinv(ig) =ig;

//********************** Structures to update arrays inside Kokkos parallel calls ************************************************
//****** achtemp **********
    Kokkos::complex<double>  achtemp[nend-nstart];
    struct achtempStruct 
    {
         Kokkos::complex<double> value[3];
        void operator+=(achtempStruct const& other) 
        {
            for (int i = 0; i < 3; ++i) 
                value[i] += other.value[i];
        }
        void operator+=(achtempStruct const volatile& other) volatile 
        {
            for (int i = 0; i < 3; ++i) 
                value[i] += other.value[i];
        }
    };
    achtempStruct achtempVar = {{achtemp[0],achtemp[1],achtemp[2]}}; 


//****** acht_n1_loc **********
//    Kokkos::complex<double>  acht_n1_loc[number_bands];
//    struct acht_n1_locStruct 
//    {
//         Kokkos::complex<double> value[number_bands];
//        void operator+=(acht_n1_locStruct const& other) 
//        {
//            for (int i = 0; i < 3; ++i) 
//                value[i] += other.value[i];
//        }
//        void operator+=(acht_n1_locStruct const volatile& other) volatile 
//        {
//            for (int i = 0; i < 3; ++i) 
//                value[i] += other.value[i];
//        }
//    }acht_n1_locVar;
//    acht_n1_locStruct acht_n1_locVar = {{[0],achtemp[1],achtemp[2]}}; 

//**********************************************************************************************************************************

    double start_time = omp_get_wtime(); //Start timing here.


//    Kokkos::parallel_reduce(Kokkos::TeamPolicy<>(number_bands, Kokkos::AUTO), KOKKOS_LAMBDA (const Kokkos::TeamPolicy<>:: member_type teamMember, achtempStruct& achtempVarUpdate)
    for(int n1 = 0; n1<number_bands; ++n1) // This for loop at the end cheddam
  //  for(int n1 = 0; n1<1; ++n1) // This for loop at the end cheddam
    {
 //       const int n1 = teamMember.league_rank();
        double wx_array[3];

//        Kokkos::parallel_reduce(Kokkos::TeamThreadRange(teamMember, ngpown), [&] (int my_igp, Kokkos::complex<double> &achstempUpdate)
//        for(int my_igp = 0; my_igp< ngpown; my_igp++)
        Kokkos::parallel_reduce(ngpown, KOKKOS_LAMBDA (int my_igp, Kokkos::complex<double> &achstempUpdate)
        {
            Kokkos::complex<double> halfinvwtilde, delw, ssx, sch, wdiff, cden , eden, mygpvar1, mygpvar2;
            Kokkos::complex<double> schstemp = expr0;
            Kokkos::complex<double> matngmatmgp = expr;
            Kokkos::complex<double> matngpmatmg = expr;
            Kokkos::complex<double> schs = expr0;
            Kokkos::complex<double> wtilde;
            int igmax = ncouls;

            int indigp = inv_igp_index(my_igp);
            int igp = indinv(indigp);
            if(indigp == ncouls)
                igp = ncouls-1;

            if(!(igp > ncouls || igp < 0)){

            if(gppsum == 1)
                igmax = igp;
            else
              igmax = ncouls;

            mygpvar1 = Kokkos::conj(aqsntemp(igp,n1));
            mygpvar2 = aqsntemp(igp,n1);

            if(gppsum == 1)
            {
                //Aggregating results in schstemp
                for(int ig=0; ig<igmax; ++ig)
                {
                    schs = -I_eps_array(ig,my_igp);
                    matngmatmgp = aqsntemp(ig,n1) * mygpvar1;
                    matngpmatmg = Kokkos::conj(aqsmtemp(ig,n1)) * mygpvar2;
                    schstemp += matngmatmgp*schs + matngpmatmg*(Kokkos::conj(schs));
                }
                schs = -I_eps_array(igp,my_igp);
                matngmatmgp = aqsntemp(igp,n1) * mygpvar1;

                if(abs(schs) > to1)
                    schstemp += matngmatmgp * schs;
            }
            else
            {
                for(int ig=1; ig<igmax; ++ig)
                {
                    schstemp = schstemp - aqsntemp(ig,n1);
                    schstemp *= I_eps_array(ig,my_igp);
                    schstemp *= mygpvar1;
                }
            }

            }
          //  achstemp += 0.5 * vcoul(igp) * schstemp;
            achstempUpdate += 0.5 * vcoul(igp) * schstemp;
        },achstemp_tmp);
        achstemp += achstemp_tmp;
//        std::cout << "achstemp = " << achstemp << std::endl;


        for(int iw=nstart; iw<nend; ++iw)
        {
            wx_array[iw] = e_lk - e_n1kq + dw*((iw+1)-2);
            if(wx_array[iw] < to1) wx_array[iw] = to1;
        }


//      Kokkos::parallel_reduce(ngpown, KOKKOS_LAMBDA (int my_igp, achtempStruct& achtempVarUpdate)
      Kokkos::parallel_reduce(ngpown, KOKKOS_LAMBDA (int my_igp, achtempStruct& achtempVarUpdate)
//          for(int my_igp=0; my_igp<ngpown; ++my_igp)
        {
            Kokkos::complex<double> wtilde2, Omega2;
            bool flag_occ = n1 < nvband;
            double wxt, delw2, delwr, wdiffr, scha_mult, rden, \
            ssxcutoff;
            Kokkos::complex<double> halfinvwtilde, delw, ssx, sch, wdiff, cden , eden, mygpvar1, mygpvar2;
            Kokkos::complex<double> schstemp = expr0;
            Kokkos::complex<double> matngmatmgp = expr;
            Kokkos::complex<double> matngpmatmg = expr;
            Kokkos::complex<double> schs = expr0;
            Kokkos::complex<double> wtilde;
            Kokkos::complex<double> scht, ssxt;
            int indigp = inv_igp_index(my_igp);
            int igp = indinv(indigp);
            if(indigp == ncouls)
                igp = ncouls-1;
            int igmax;
            Kokkos::complex<double> scha[ncouls];
            Kokkos::complex<double> ssx_array[3];
            Kokkos::complex<double> sch_array[3];

            if(gppsum == 1)
                igmax = igp;
            else
              igmax = ncouls;

            if(!(igp > ncouls || igp < 0)){
            
            for(int i=0; i<3; i++)
            {
                ssx_array[i] = expr0;
                sch_array[i] = expr0;
            }

            mygpvar1 = Kokkos::conj(aqsmtemp(igp,n1));
            mygpvar2 = aqsmtemp(igp,n1);

            if(flag_occ)
            {
                for(int iw=nstart; iw<nend; ++iw)
                {
                        scht = ssxt = expr0;
                        wxt = wx_array[iw];

                        if(gppsum == 1)
                        {
                            for(int ig=0; ig<igmax; ++ig)
                            {
                                wtilde = wtilde_array(ig, my_igp);
                                wtilde2 = kokkos_square(wtilde,2);
                                Omega2 = I_eps_array(ig,my_igp) * wtilde2 ;

                                if(Kokkos::abs(Omega2) > to1) 
                                {
                                    matngmatmgp = aqsntemp(ig,n1) * mygpvar1;
                                    if(ig != igp) matngpmatmg = Kokkos::conj(aqsmtemp(ig,n1)) * mygpvar2;

                                    halfinvwtilde = (Kokkos::complex<double>) 0.5/wtilde; 
                                    Kokkos::complex<double> wxt_wtilde = doubleMinusKokkosComplex(wxt , wtilde);
                                    delw = wxt_wtilde * halfinvwtilde;
                                    delw2 = std::pow(abs(delw),2);


                                    if((Kokkos::abs(wxt_wtilde) < gamma) || (delw2 < to1))
                                    {
                                        sch = expr0;
                                        if(Kokkos::abs(wtilde) > to1)
                                            ssx = -Omega2 / (4.0 * doublePlusKokkosComplex(1.0 , delw) * wtilde2);
                                        else
                                            ssx = expr0;
                                    }
                                    else
                                    {
                                        sch = I_eps_array(ig,my_igp) * wtilde / wxt_wtilde;
                                        ssx = Omega2 / doubleMinusKokkosComplex(std::pow(wxt,2) , wtilde2);
                                    }

                                    Kokkos::complex<double> ssxcutoffComplex = I_eps_array(ig,my_igp);
                                    ssxcutoff = sexcut*Kokkos::abs(ssxcutoffComplex);
                                    if((Kokkos::abs(ssx) > ssxcutoff) && (wxt < 0.0)) ssx = expr0;

                                    if(ig != igp-1)
                                    {
                                        ssxa[ig] = matngmatmgp*ssx + matngpmatmg * Kokkos::conj(ssx);
                                        scha[ig] = matngmatmgp*sch + matngpmatmg * Kokkos::conj(sch);
                                    }
                                    else
                                    {
                                        ssxa[ig] = matngmatmgp * ssx;
                                        scha[ig] = matngmatmgp * sch;
                                    }

                                    ssxt += ssxa[ig];
                                    scht += scha[ig];
//                                    cout << "ssxa[" << ig << "] = " << ssxa[ig] << endl;
                                }
                            }
//                            std::cout << "ssxt = " << ssxt << "\t scht = " << scht << endl;
                        }
                        else
                        {
                            //344-394
                            for(int ig=0; ig<igmax; ++ig)
                            {
                                wtilde = wtilde_array(ig,my_igp);
                                wtilde2 = kokkos_square(wtilde,2);
                                Omega2 = I_eps_array(ig,my_igp) * wtilde;

                                matngmatmgp = aqsntemp(ig,n1) * mygpvar1;
                                wdiff = doubleMinusKokkosComplex(wxt , wtilde);

                                cden = wdiff;
                                rden = Kokkos::real(cden * Kokkos::conj(cden));
                                rden = 1.00 / rden;
                                delw = rden * wtilde * Kokkos::conj(cden);
                                delwr = Kokkos::real(delw * Kokkos::conj(delw)); //This is diff from original ...
                                wdiffr = Kokkos::real(wdiff * Kokkos::conj(wdiff));

                                if((wdiffr > limittwo) && (delwr < limitone))
                                {
                                    sch = delw;
                                    sch *= I_eps_array(ig,my_igp);
                                    cden = kokkos_square(wxt,2);
                                    rden = Kokkos::real(cden * Kokkos::conj(cden));
                                    rden = 1.00 / rden;
                                    ssx = rden * Omega2 * Kokkos::conj(cden);
                                }
                                else if (delwr > to1)
                                {
                                    sch = 0.00;
                                    cden = 4.00 * wtilde2 * doublePlusKokkosComplex(0.5, delw );
                                    rden = Kokkos::real(cden * Kokkos::conj(cden));
                                    rden = 1.00/rden;
                                    ssx = rden * -Omega2 * conj(cden) * delw;
                                }
                                else
                                {
                                    sch = expr0;
                                    ssx = expr0;
                                }

                                ssxcutoff = sexcut*abs(I_eps_array(ig,my_igp));
                                if((Kokkos::abs(ssx) > ssxcutoff) && (wxt < 0.00)) ssx = expr0;

                                ssxa(ig) = matngmatmgp*ssx;
                                scha[ig] = matngmatmgp*sch;

                                ssxt += ssxa(ig);
                                scht += scha[ig];
                          }
                        }

                        ssx_array[iw] += ssxt;
                        sch_array[iw] += 0.5*scht;
//                        cout << "sch_array[" << iw << "] = " << sch_array[iw] << endl;
//                        cout << "scht " << scht << endl;
                    }
                }
                else
                {
                    int igblk = 512;
                    //403 - 479
                    for(int igbeg=0; igbeg<igmax; igbeg+=igblk)
                    {
                        int igend = min(igbeg+igblk-1, igmax);
                        for(int iw=nstart; iw<nend; ++iw)
                        {
                            scht = ssxt = expr0;
                            wxt = wx_array[iw];

                            if(gppsum == 1)
                            {
                                for(int ig= igbeg; ig<min(igend,igmax-1); ++ig)
                                {
                                    wtilde = wtilde_array(ig,my_igp);
                                    matngmatmgp = aqsntemp(ig,n1) * mygpvar1;
                                    wdiff = doubleMinusKokkosComplex(wxt , wtilde);
                                    delw = wtilde / wdiff;
                                    delw2 = Kokkos::real(delw * Kokkos::conj(delw));
                                    wdiffr = Kokkos::real(wdiff * Kokkos::conj(wdiff));
                                    if((abs(wdiffr) < limittwo) || (delw2 > limitone))
                                        scha_mult = 1.0;
                                    else 
                                        scha_mult = 0.0;

                                    sch = scha_mult * delw ;
                                    sch *= I_eps_array(ig, my_igp);
                                    scha[ig] = matngmatmgp*sch + Kokkos::conj(aqsmtemp(ig,n1)) * mygpvar2 * Kokkos::conj(sch);
                                    scht += scha[ig];
                                }
                                if(igend == (igmax-1))
                                {
                                    int ig = igmax;
                                    wtilde = wtilde_array(ig,my_igp);
                                    matngmatmgp = aqsntemp(ig,n1) * mygpvar1;
                                    wdiff = doubleMinusKokkosComplex(wxt,wtilde);
                                    delw = wtilde / wdiff;
                                    delw2 = Kokkos::real(delw * Kokkos::conj(delw));
                                    wdiffr = Kokkos::real(wdiff * Kokkos::conj(wdiff));
                                    if((wdiffr < limittwo) || (delw2 > limitone))
                                        scha_mult = 1.0;
                                    else scha_mult = 0.0;

                                    sch = scha_mult * delw ;
                                    sch *= I_eps_array(ig, my_igp);
                                    scha[ig] = matngmatmgp * sch;
                                    scht += scha[ig];
                                }
                            }
                            else
                            {
                                for(int ig = igbeg; ig<min(igend,igmax); ++ig)
                                {
                                    wdiff = doubleMinusKokkosComplex(wxt , wtilde_array(ig,my_igp));
                                    cden = wdiff;
                                    rden = Kokkos::real(cden * Kokkos::conj(cden));
                                    rden = 1.00/rden;
                                    delw = rden * wtilde_array(ig,my_igp) * Kokkos::conj(cden);
                                    delwr = Kokkos::real(delw * Kokkos::conj(delw));
                                    wdiffr = Kokkos::real(wdiff * Kokkos::conj(wdiff));

                                    scha[ig] = mygpvar1 * delw * aqsntemp(ig,n1);
                                    scha[ig] *= I_eps_array(ig, my_igp);

                                    if((wdiffr > limittwo) && (delwr < limitone)) scht += scha[ig];
                                }
                            }
                            Kokkos::View<complex<double>> schtView("schtView");
                            schtView() = scht;
                            sch_array[iw] += 0.5*scht;
//                            sch_array(iw) += 0.5*scht;
                        }
                    }
                }
                }

            if(flag_occ)
            {
                for(int iw=nstart; iw<nend; ++iw)
                {
                    Kokkos::View<complex<double>> addVal("addVal");
                    addVal() = occ * ssx_array[iw];
                    asxtemp(iw) += addVal() ; //occ does not change and is 1.00 so why not remove it.
                }
            }

            for(int iw=nstart; iw<nend; ++iw)
            {
                Kokkos::complex<double> addVal = vcoul(igp) * sch_array[iw];
//                achtempVar.value[iw] += addVal;
                achtempVarUpdate.value[iw] += addVal;
            }
            Kokkos::View<complex<double>> addVal("addVal");
            addVal() = sch_array[2];
            acht_n1_loc(n1) += addVal() * vcoul(igp);
//            acht_n1_loc(n1) += sch_array[2] * vcoul(igp);
        },achtempVar); // for - ngpown 

        //Rahul - have to copy it into a diff buffer not related to kokkos-views so that the value is not modified at the start of each iteration.
        for(int iw=nstart; iw<nend; ++iw)
            achtemp[iw] += achtempVar.value[iw];
    } // for - number_bands

   
    double end_time = omp_get_wtime(); //End timing here

    for(int iw=nstart; iw<nend; ++iw)
        cout << "Final achtemp[" << iw << "] = " << achtemp[iw] << endl;
//        cout << "achtemp[" << iw << "] = " << achtemp[iw] << endl;

    cout << "********** Time Taken **********= " << end_time - start_time << " secs" << endl;

    }
    Kokkos::finalize();

    return 0;
}

//Almost done code