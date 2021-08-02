#include "write_HS.h"
#include "../src_pw/global.h"


void HS_Matrix::saving_HS(const double *Hloc, const double* Sloc, bool bit, const int &out_hs)
{   
    if(out_hs==1)
    {
        save_HS(Hloc, Sloc, bit);
    }
    else if(out_hs==2)
    {
        save_HS(Hloc, Sloc, bit);
    }
    else if(out_hs==3)
    {
        //please call individually
    }
    else if(out_hs==0)
    {
        // do nothing.
    }
    else
    {
        WARNING("Diago_LCAO_Matrix","unrecorganized out_hs value.");
    }
    return;
}


/*
void HS_Matrix::save_HS_ccf(const int &iter, const int &Hnnz, const int *colptr_H, const int *rowind_H, 
        const double *nzval_H, const double *nzval_S, bool bit)
{
    TITLE("HS_Matrix","save_HS_ccf");

    if(GlobalV::DRANK!=0)return;
    
    stringstream ssh;
    stringstream sss;

    if(bit)
    {
        ssh << GlobalV::global_out_dir << "H_bit.ccf";
        sss << GlobalV::global_out_dir << "S_bit.ccf";
    }
    else
    {
		// mohan update 2021-02-10
        ssh << GlobalV::global_out_dir << "H" << ELEC_scf::iter << "_" << iter+1 << ".ccf";
        sss << GlobalV::global_out_dir << "S" << ELEC_scf::iter << "_" << iter+1 << ".ccf";
    }

    if(bit)
    {
        FILE *g1 = fopen(ssh.str().c_str(),"wb");
        FILE *g2 = fopen(sss.str().c_str(),"wb");

        fwrite(&GlobalV::NLOCAL,sizeof(int),1,g1);
        fwrite(&Hnnz,sizeof(int),1,g1);
        fwrite(&GlobalV::NLOCAL,sizeof(int),1,g2);
        fwrite(&Hnnz,sizeof(int),1,g2);

        fclose(g1);
        fclose(g2);
    }

        
    if(!bit)
    {
        ofstream g1(ssh.str().c_str());
        ofstream g2(sss.str().c_str());

        g1 << GlobalV::NLOCAL << " " << Hnnz << endl;
        g2 << GlobalV::NLOCAL << " " << Hnnz << endl;

        for(int i=0; i<GlobalV::NLOCAL+1; ++i)
        {
            g1 << colptr_H[i] << " ";
            g2 << colptr_H[i] << " ";
        }
        g1 << endl;
        g2 << endl;

        for(int i=0; i<Hnnz; ++i)
        {
            g1 << rowind_H[i] << " ";
            g2 << rowind_H[i] << " ";
        }
        g1 << endl;
        g2 << endl;

        for(int i=0; i<Hnnz; ++i)
        {
            g1 << nzval_H[i] << " ";
            g2 << nzval_S[i] << " ";
        }
        g1 << endl;
        g2 << endl;

        g1.close();
        g2.close();
    }
    return;
}
*/

// mohan add 2010/3/20, output H and S matrix, convinence for diagonalization
// test or save the middle information for next start.
void HS_Matrix::save_HS(const double *H, const double *S, bool bit)
{
    TITLE("HS_Matrix","save_HS_bit");
    timer::tick("HS_Matrix","save_HS_bit");
    OUT(GlobalV::ofs_running,"Dimension of H and S",GlobalV::NLOCAL);

    stringstream ssh;
    stringstream sss;

    if(bit)
    {
        ssh << GlobalV::global_out_dir << "data-H-bit";
        sss << GlobalV::global_out_dir << "data-S-bit";
    }
    else 
    {
        ssh << GlobalV::global_out_dir << "data-H";
        sss << GlobalV::global_out_dir << "data-S";
    }

    if (bit)
    {
#ifdef __MPI
        FILE *g1 = nullptr;
        FILE *g2 = nullptr;

        if (GlobalV::DRANK==0)
        {
            g1 = fopen(ssh.str().c_str(),"wb");
            g2 = fopen(sss.str().c_str(),"wb");
            fwrite(&GlobalV::NLOCAL,sizeof(int),1,g1);
            fwrite(&GlobalV::NLOCAL,sizeof(int),1,g2);
        }

        int ir,ic;
        for (int i=0; i<GlobalV::NLOCAL; i++)
        {
            double* lineH = new double[GlobalV::NLOCAL-i];
            double* lineS = new double[GlobalV::NLOCAL-i];
            ZEROS(lineH, GlobalV::NLOCAL-i);
            ZEROS(lineS, GlobalV::NLOCAL-i);

            ir = GlobalC::ParaO.trace_loc_row[i];
            if (ir>=0)
            {
                // data collection
                for (int j=i; j<GlobalV::NLOCAL; j++)
                {
                    ic = GlobalC::ParaO.trace_loc_col[j];
                    if (ic>=0)
                    {
                        int iic;
                        if(GlobalV::KS_SOLVER=="genelpa" || GlobalV::KS_SOLVER=="scalapack_gvx")  // save the matrix as column major format
                        {
                            iic=ir+ic*GlobalC::ParaO.nrow;
                        }
                        else
                        {
                            iic=ir*GlobalC::ParaO.ncol+ic;
                        }
                        //lineH[j-i] = H[ir*GlobalC::ParaO.ncol+ic];
                        //lineS[j-i] = S[ir*GlobalC::ParaO.ncol+ic];
                        lineH[j-i] = H[iic];
                        lineS[j-i] = S[iic];
                    }
                }
            }
            else
            {
                //do nothing
            }

            Parallel_Reduce::reduce_double_all(lineH,GlobalV::NLOCAL-i);
            Parallel_Reduce::reduce_double_all(lineS,GlobalV::NLOCAL-i);

            if (GlobalV::DRANK==0)
            {
                for (int j=i; j<GlobalV::NLOCAL; j++)
                {
                    fwrite(&lineH[j-i],sizeof(double),1,g1);
                    fwrite(&lineS[j-i],sizeof(double),1,g2);
                }
            }
            delete[] lineH;
            delete[] lineS;

            MPI_Barrier(DIAG_WORLD);
        }

        if (GlobalV::DRANK==0)
        {
            fclose(g1);
            fclose(g2);
        }
#else
        FILE *g1 = fopen(ssh.str().c_str(),"wb");
        FILE *g2 = fopen(sss.str().c_str(),"wb");

        fwrite(&GlobalV::NLOCAL,sizeof(int),1,g1);
        fwrite(&GlobalV::NLOCAL,sizeof(int),1,g2);

        for (int i=0; i<GlobalV::NLOCAL; i++)
        {
            for (int j=i; j<GlobalV::NLOCAL; j++)
            {
                fwrite(&H[i*GlobalV::NLOCAL+j],sizeof(double),1,g1);
                fwrite(&S[i*GlobalV::NLOCAL+j],sizeof(double),1,g2);
            }
        }
        fclose(g1);
        fclose(g2);
#endif
    } //end bit
    else
    {
#ifdef __MPI
        ofstream g1;
        ofstream g2;

        if (GlobalV::DRANK==0)
        {
            g1.open(ssh.str().c_str());
            g2.open(sss.str().c_str());
            g1 << GlobalV::NLOCAL;
            g2 << GlobalV::NLOCAL;
        }

        int ir,ic;
        for (int i=0; i<GlobalV::NLOCAL; i++)
        {
            double* lineH = new double[GlobalV::NLOCAL-i];
            double* lineS = new double[GlobalV::NLOCAL-i];
            ZEROS(lineH, GlobalV::NLOCAL-i);
            ZEROS(lineS, GlobalV::NLOCAL-i);

            ir = GlobalC::ParaO.trace_loc_row[i];
            if (ir>=0)
            {
                // data collection
                for (int j=i; j<GlobalV::NLOCAL; j++)
                {
                    ic = GlobalC::ParaO.trace_loc_col[j];
                    if (ic>=0)
                    {
                        int iic;
                        if(GlobalV::KS_SOLVER=="genelpa" || GlobalV::KS_SOLVER=="scalapack_gvx")  // save the matrix as column major format
                        {
                            iic=ir+ic*GlobalC::ParaO.nrow;
                        }
                        else
                        {
                            iic=ir*GlobalC::ParaO.ncol+ic;
                        }
                        //lineH[j-i] = H[ir*GlobalC::ParaO.ncol+ic];
                        //lineS[j-i] = S[ir*GlobalC::ParaO.ncol+ic];
                        lineH[j-i] = H[iic];
                        lineS[j-i] = S[iic];
                    }
                }
            }
            else
            {
                //do nothing
            }

            Parallel_Reduce::reduce_double_all(lineH,GlobalV::NLOCAL-i);
            Parallel_Reduce::reduce_double_all(lineS,GlobalV::NLOCAL-i);

            if (GlobalV::DRANK==0)
            {
                for (int j=i; j<GlobalV::NLOCAL; j++)
                {
                    g1 << " " << lineH[j-i];
                    g2 << " " << lineS[j-i];
                }
                g1 << endl;
                g2 << endl;
            }
            delete[] lineH;
            delete[] lineS;
        }

        //if (GlobalV::DRANK==0);
        if (GlobalV::DRANK==0)       // Peize Lin delete ; at 2020.01.31
        {
            g1.close();
            g2.close();
        }

/*LiuXH add 2015-12-17,begin
    //int nprocs,myid;
    //MPI_Status status;
    //MPI_Comm_size(DIAG_HPSEPS_WORLD,&nprocs);
    //MPI_Comm_rank(DIAG_HPSEPS_WORLD,&myid);

    string H_fn;
    stringstream H_fn2;
    H_fn2<< "data-H-"  << GlobalV::DRANK ;
    H_fn=H_fn2.str();
    ofstream ofs_H;
    ofs_H.open(H_fn.c_str());
    ofs_H<<setprecision(8) << setw(12);

    string S_fn;
    stringstream S_fn2;
    S_fn2<< "data-S-"  << GlobalV::DRANK ;
    S_fn=S_fn2.str();
    ofstream ofs_S;
    ofs_S.open(S_fn.c_str());
    ofs_S<<setprecision(8) << setw(12);

        int irr,icc;
        for (int i=0; i<GlobalV::NLOCAL; i++)
        {
            irr = GlobalC::ParaO.trace_loc_row[i];
            if (irr>=0)
            {
                // data collection
                for (int j=0; j<GlobalV::NLOCAL; j++)
                {
            icc = GlobalC::ParaO.trace_loc_col[j];
            if (icc>=0)
            {
                //if(abs(H[irr*GlobalC::ParaO.ncol+icc]) < 1.0e-10) H[irr*GlobalC::ParaO.ncol+icc] = 0.0;
                //if(abs(S[irr*GlobalC::ParaO.ncol+icc]) < 1.0e-10) S[irr*GlobalC::ParaO.ncol+icc] = 0.0;
                ofs_H << " " << H[irr*GlobalC::ParaO.ncol+icc];
                ofs_S << " " << S[irr*GlobalC::ParaO.ncol+icc];
            }
        }
        ofs_H << endl;
        ofs_S << endl;
         }
         }
//LiuXH add 2015-12-17,end*/
#else
        ofstream g1(ssh.str().c_str());
        ofstream g2(sss.str().c_str());

        g1 << GlobalV::NLOCAL;
        g2 << GlobalV::NLOCAL;

        for (int i=0; i<GlobalV::NLOCAL; i++)
        {
            for (int j=i; j<GlobalV::NLOCAL; j++)
            {
                g1 << " " << H[i*GlobalV::NLOCAL+j];
                g2 << " " << S[i*GlobalV::NLOCAL+j];
            }
            g1 << endl;
            g2 << endl;
        }
        g1.close();
        g2.close();
#endif
    }

    timer::tick("HS_Matrix","save_HS_bit");
    return;
}

//LiuXh, 2017-03-21
void HS_Matrix::saving_HS_complex(complex<double> *Hloc, complex<double>* Sloc, bool bit, const int &out_hs)
{   
    if(out_hs==1)
    {
        save_HS_complex(Hloc, Sloc, bit);
    }
    else if(out_hs==0)
    {
        // do nothing.
    }
    else
    {
        WARNING("Diago_LCAO_Matrix","unrecorganized out_hs value.");
    }
    return;
}

//LiuXh, 2017-03-21
void HS_Matrix::save_HS_complex(complex<double> *H, complex<double> *S, bool bit)
{
    TITLE("HS_Matrix","save_HS_bit");
    timer::tick("HS_Matrix","save_HS_bit");
    OUT(GlobalV::ofs_running,"Dimension of H and S",GlobalV::NLOCAL);

    stringstream ssh;
    stringstream sss;

    if(bit)
    {
    ssh << GlobalV::global_out_dir << "data-H-bit";
    sss << GlobalV::global_out_dir << "data-S-bit";
    }
    else
    {
    ssh << GlobalV::global_out_dir << "data-H";
    sss << GlobalV::global_out_dir << "data-S";
    }

    if (bit)
    {
#ifdef __MPI
        FILE *g1 = nullptr;
        FILE *g2 = nullptr;

        if (GlobalV::DRANK==0)
        {
            g1 = fopen(ssh.str().c_str(),"wb");
            g2 = fopen(sss.str().c_str(),"wb");
            fwrite(&GlobalV::NLOCAL,sizeof(int),1,g1);
            fwrite(&GlobalV::NLOCAL,sizeof(int),1,g2);
        }

        int ir,ic;
        for (int i=0; i<GlobalV::NLOCAL; i++)
        {
            complex<double>* lineH = new complex<double>[GlobalV::NLOCAL-i];
            complex<double>* lineS = new complex<double>[GlobalV::NLOCAL-i];
            ZEROS(lineH, GlobalV::NLOCAL-i);
            ZEROS(lineS, GlobalV::NLOCAL-i);

            ir = GlobalC::ParaO.trace_loc_row[i];
            if (ir>=0)
            {
                // data collection
                for (int j=i; j<GlobalV::NLOCAL; j++)
                {
                    ic = GlobalC::ParaO.trace_loc_col[j];
                    if (ic>=0)
                    {
                        int iic;
                        if(GlobalV::KS_SOLVER=="genelpa" || GlobalV::KS_SOLVER=="scalapack_gvx")  // save the matrix as column major format
                        {
                            iic=ir+ic*GlobalC::ParaO.nrow;
                        }
                        else
                        {
                            iic=ir*GlobalC::ParaO.ncol+ic;
                        }
                        //lineH[j-i] = H[ir*GlobalC::ParaO.ncol+ic];
                        //lineS[j-i] = S[ir*GlobalC::ParaO.ncol+ic];
                        lineH[j-i] = H[iic];
                        lineS[j-i] = S[iic];
                    }
                }
            }
            else
            {
                //do nothing
            }

            Parallel_Reduce::reduce_complex_double_pool(lineH,GlobalV::NLOCAL-i);
            Parallel_Reduce::reduce_complex_double_pool(lineS,GlobalV::NLOCAL-i);

            if (GlobalV::DRANK==0)
            {
                for (int j=i; j<GlobalV::NLOCAL; j++)
                {
                    fwrite(&lineH[j-i],sizeof(complex<double>),1,g1);
                    fwrite(&lineS[j-i],sizeof(complex<double>),1,g2);
                }
            }
            delete[] lineH;
            delete[] lineS;

            MPI_Barrier(DIAG_WORLD);
        }

        if (GlobalV::DRANK==0)
        {
            fclose(g1);
            fclose(g2);
        }
#else
        FILE *g1 = fopen(ssh.str().c_str(),"wb");
        FILE *g2 = fopen(sss.str().c_str(),"wb");

        fwrite(&GlobalV::NLOCAL,sizeof(int),1,g1);
        fwrite(&GlobalV::NLOCAL,sizeof(int),1,g2);

        for (int i=0; i<GlobalV::NLOCAL; i++)
        {
            for (int j=i; j<GlobalV::NLOCAL; j++)
            {
                fwrite(&H[i*GlobalV::NLOCAL+j],sizeof(complex<double>),1,g1);
                fwrite(&S[i*GlobalV::NLOCAL+j],sizeof(complex<double>),1,g2);
            }
        }
        fclose(g1);
        fclose(g2);
#endif
    } //end bit
    else
    {
#ifdef __MPI
        ofstream g1;
        ofstream g2;

        if (GlobalV::DRANK==0)
        {
            g1.open(ssh.str().c_str());
            g2.open(sss.str().c_str());
            g1 << GlobalV::NLOCAL;
            g2 << GlobalV::NLOCAL;
        }

        int ir,ic;
        for (int i=0; i<GlobalV::NLOCAL; i++)
        {
            complex<double>* lineH = new complex<double>[GlobalV::NLOCAL-i];
            complex<double>* lineS = new complex<double>[GlobalV::NLOCAL-i];
            ZEROS(lineH, GlobalV::NLOCAL-i);
            ZEROS(lineS, GlobalV::NLOCAL-i);

            ir = GlobalC::ParaO.trace_loc_row[i];
            if (ir>=0)
            {
                // data collection
                for (int j=i; j<GlobalV::NLOCAL; j++)
                {
                    ic = GlobalC::ParaO.trace_loc_col[j];
                    if (ic>=0)
                    {
                        int iic;
                        if(GlobalV::KS_SOLVER=="genelpa" || GlobalV::KS_SOLVER=="scalapack_gvx")  // save the matrix as column major format
                        {
                            iic=ir+ic*GlobalC::ParaO.nrow;
                        }
                        else
                        {
                            iic=ir*GlobalC::ParaO.ncol+ic;
                        }
                        //lineH[j-i] = H[ir*GlobalC::ParaO.ncol+ic];
                        //lineS[j-i] = S[ir*GlobalC::ParaO.ncol+ic];
                        lineH[j-i] = H[iic];
                        lineS[j-i] = S[iic];
                    }
                }
            }
            else
            {
                //do nothing
            }

            Parallel_Reduce::reduce_complex_double_pool(lineH,GlobalV::NLOCAL-i);
            Parallel_Reduce::reduce_complex_double_pool(lineS,GlobalV::NLOCAL-i);

            if (GlobalV::DRANK==0)
            {
                for (int j=i; j<GlobalV::NLOCAL; j++)
                {
                    g1 << " " << lineH[j-i];
                    g2 << " " << lineS[j-i];
                }
                g1 << endl;
                g2 << endl;
            }
            delete[] lineH;
            delete[] lineS;
        }

        //if (GlobalV::DRANK==0);
        if (GlobalV::DRANK==0)           // Peize Lin delete ; at 2020.01.31
        {
            g1.close();
            g2.close();
        }

/*LiuXH add 2015-12-17,begin
        //int nprocs,myid;
        //MPI_Status status;
        //MPI_Comm_size(DIAG_HPSEPS_WORLD,&nprocs);
        //MPI_Comm_rank(DIAG_HPSEPS_WORLD,&myid);

        string H_fn;
        stringstream H_fn2;
        H_fn2<< "data-H-"  << GlobalV::DRANK ;
        H_fn=H_fn2.str();
        ofstream ofs_H;
        ofs_H.open(H_fn.c_str());
        ofs_H<<setprecision(8) << setw(12);

        string S_fn;
        stringstream S_fn2;
        S_fn2<< "data-S-"  << GlobalV::DRANK ;
        S_fn=S_fn2.str();
        ofstream ofs_S;
        ofs_S.open(S_fn.c_str());
        ofs_S<<setprecision(8) << setw(12);

        int irr,icc;
        for (int i=0; i<GlobalV::NLOCAL; i++)
        {
            irr = GlobalC::ParaO.trace_loc_row[i];
            if (irr>=0)
            {
                // data collection
                for (int j=0; j<GlobalV::NLOCAL; j++)
                {
                        icc = GlobalC::ParaO.trace_loc_col[j];
                        if (icc>=0)
                        {
                                //if(abs(H[irr*GlobalC::ParaO.ncol+icc]) < 1.0e-10) H[irr*GlobalC::ParaO.ncol+icc] = 0.0;
                                //if(abs(S[irr*GlobalC::ParaO.ncol+icc]) < 1.0e-10) S[irr*GlobalC::ParaO.ncol+icc] = 0.0;
                                ofs_H << " " << H[irr*GlobalC::ParaO.ncol+icc];
                                ofs_S << " " << S[irr*GlobalC::ParaO.ncol+icc];
                        }
                }
                ofs_H << endl;
                ofs_S << endl;
             }
         }
//LiuXH add 2015-12-17,end*/
#else
        ofstream g1(ssh.str().c_str());
        ofstream g2(sss.str().c_str());

        g1 << GlobalV::NLOCAL;
        g2 << GlobalV::NLOCAL;

        for (int i=0; i<GlobalV::NLOCAL; i++)
        {
            for (int j=i; j<GlobalV::NLOCAL; j++)
            {
                g1 << " " << H[i*GlobalV::NLOCAL+j];
                g2 << " " << S[i*GlobalV::NLOCAL+j];
            }
            g1 << endl;
            g2 << endl;
        }
        g1.close();
        g2.close();
#endif
    }

    timer::tick("HS_Matrix","save_HS_bit");
    return;
}

//void HS_Matrix::save_HSR_tr(const int Rx, const int Ry, const int Rz, const double *H, const double *S)
void HS_Matrix::save_HSR_tr(const int current_spin)
//void HS_Matrix::save_HSR_tr(void)
{
    TITLE("HS_Matrix","save_HSR_tr");
    timer::tick("HS_Matrix","save_HSR_tr");

    stringstream ssh;
    stringstream sss;

    ssh << GlobalV::global_out_dir << "data-HR-tr_SPIN"<<current_spin;
    sss << GlobalV::global_out_dir << "data-SR-tr_SPIN"<<current_spin;
    //ssh << GlobalV::global_out_dir << "data-HR-tr_SPIN";
    //sss << GlobalV::global_out_dir << "data-SR-tr_SPIN";

#ifdef __MPI
    ofstream g1;
    ofstream g2;

    if(GlobalV::DRANK==0)
    {
        g1.open(ssh.str().c_str());
        g2.open(sss.str().c_str());
        g1 << "Matrix Dimension of H(R): "<<GlobalV::NLOCAL<<endl;
        g2 << "Matrix Dimension of S(R): "<<GlobalV::NLOCAL<<endl;
    }

    int R_x = GlobalC::GridD.getCellX();
    int R_y = GlobalC::GridD.getCellY();
    int R_z = GlobalC::GridD.getCellZ();

//cout<<"R_x: "<<R_x<<endl;
//cout<<"R_y: "<<R_y<<endl;
//cout<<"R_z: "<<R_z<<endl;

    double R_minX = GlobalC::GridD.getD_minX();
    double R_minY = GlobalC::GridD.getD_minY();
    double R_minZ = GlobalC::GridD.getD_minZ();

    //int dRx, dRy, dRz;

    for(int ix=0; ix<R_x; ix++)
    {
        int dRx = ix + R_minX;
        for(int iy=0; iy<R_y; iy++)
        {
            int dRy = iy + R_minY;
            for(int iz=0; iz<R_z; iz++)
            {
                int dRz = iz + R_minZ;
//cout<<"dRx: "<<dRx<<endl;
//cout<<"dRy: "<<dRy<<endl;
//cout<<"dRz: "<<dRz<<endl;
                int ir,ic;
                for(int i=0; i<GlobalV::NLOCAL; i++)
                {
                    //double* lineH = new double[GlobalV::NLOCAL-i];
                    //double* lineS = new double[GlobalV::NLOCAL-i];
                    double* lineH = nullptr;
                    double* lineS = nullptr;
                    complex<double>* lineH_soc = nullptr;
                    complex<double>* lineS_soc = nullptr;
                    if(GlobalV::NSPIN!=4)
                    {
                        lineH = new double[GlobalV::NLOCAL];
                        lineS = new double[GlobalV::NLOCAL];
                        ZEROS(lineH, GlobalV::NLOCAL);
                        ZEROS(lineS, GlobalV::NLOCAL);
                    }
                    else
                    {
                        lineH_soc = new complex<double>[GlobalV::NLOCAL];
                        lineS_soc = new complex<double>[GlobalV::NLOCAL];
                        ZEROS(lineH_soc, GlobalV::NLOCAL);
                        ZEROS(lineS_soc, GlobalV::NLOCAL);
                    }
                    //ZEROS(lineH, GlobalV::NLOCAL-i);
                    //ZEROS(lineS, GlobalV::NLOCAL-i);
                    //ZEROS(lineH, GlobalV::NLOCAL);
                    //ZEROS(lineS, GlobalV::NLOCAL);

                    ir = GlobalC::ParaO.trace_loc_row[i];
                    if(ir>=0)
                    {
                        //for(int j=i; j<GlobalV::NLOCAL; j++)
                        for(int j=0; j<GlobalV::NLOCAL; j++)
                        {
                            ic = GlobalC::ParaO.trace_loc_col[j];
                            if(ic>=0)
                            {
                                //lineH[j-i] = H[ir*GlobalC::ParaO.ncol+ic];
                                //lineS[j-i] = S[ir*GlobalC::ParaO.ncol+ic];
                                int iic;
                                if(GlobalV::KS_SOLVER=="genelpa" || GlobalV::KS_SOLVER=="scalapack_gvx")  // save the matrix as column major format
                                {
                                    iic=ir+ic*GlobalC::ParaO.nrow;
                                }
                                else
                                {
                                    iic=ir*GlobalC::ParaO.ncol+ic;
                                }
                                if(GlobalV::NSPIN!=4)
                                {
                                    lineH[j] = GlobalC::LM.HR_tr[ix][iy][iz][iic];
                                    lineS[j] = GlobalC::LM.SlocR_tr[ix][iy][iz][iic];
                                }
                                else
                                {
                                    lineH_soc[j] = GlobalC::LM.HR_tr_soc[ix][iy][iz][iic];
                                    lineS_soc[j] = GlobalC::LM.SlocR_tr_soc[ix][iy][iz][iic];
                                }
                            }
                        }
                    }
                    else
                    {
                        //do nothing
                    }

                    //Parallel_Reduce::reduce_double_all(lineH,GlobalV::NLOCAL-i);
                    //Parallel_Reduce::reduce_double_all(lineS,GlobalV::NLOCAL-i);
                    if(GlobalV::NSPIN!=4)
                    {
                        Parallel_Reduce::reduce_double_all(lineH,GlobalV::NLOCAL);
                        Parallel_Reduce::reduce_double_all(lineS,GlobalV::NLOCAL);
                    }
                    else
                    {
                        Parallel_Reduce::reduce_complex_double_all(lineH_soc,GlobalV::NLOCAL);
                        Parallel_Reduce::reduce_complex_double_all(lineS_soc,GlobalV::NLOCAL);
                    }

                    if(GlobalV::DRANK==0)
                    {
                        //for(int j=i; j<GlobalV::NLOCAL; j++)
                        for(int j=0; j<GlobalV::NLOCAL; j++)
                        {
                            if(i==0 && j==0)
                            {
                                g1 << dRx << " " << dRy << " " << dRz  << "    //R vector(R2 - R1,unit: lattice vector)" <<endl;
                                g2 << dRx << " " << dRy << " " << dRz  << "    //R vector(R2 - R1,unit: lattice vector)" <<endl;
                            }
                            //g1 << " " << lineH[j-i];
                            //g2 << " " << lineS[j-i];
                            if(GlobalV::NSPIN!=4)
                            {
                                if(abs(lineH[j]) < 1.0e-12) lineH[j]=0.0;
                                if(abs(lineS[j]) < 1.0e-12) lineS[j]=0.0;
                                g1 << " " << lineH[j];
                                g2 << " " << lineS[j];
                            }
                            else
                            {
                if(abs(lineH_soc[j].real()) < 1.0e-12) lineH_soc[j]= complex<double> (0.0, lineH_soc[j].imag());
                                if(abs(lineH_soc[j].imag()) < 1.0e-12) lineH_soc[j]= complex<double> (lineH_soc[j].real(), 0.0);
                                if(abs(lineS_soc[j].real()) < 1.0e-12) lineS_soc[j]= complex<double> (0.0, lineS_soc[j].imag());
                                if(abs(lineS_soc[j].imag()) < 1.0e-12) lineS_soc[j]= complex<double> (lineS_soc[j].real() , 0.0);
                                g1 << " " << lineH_soc[j];
                                g2 << " " << lineS_soc[j];
                            }
                        }
                        g1 << endl;
                        g2 << endl;
                    }
                    if(GlobalV::NSPIN!=4)
                    {
                        delete[] lineH;
                        delete[] lineS;
                    }
                    else
                    {
                        delete[] lineH_soc;
                        delete[] lineS_soc;
                    }
                }
/*
                if(GlobalV::DRANK==0);
                {
                    g1.close();
                    g2.close();
                }
*/
            }
        }
    }
    //if(GlobalV::DRANK==0);
    if(GlobalV::DRANK==0)                // Peize Lin delete ; at 2020.01.31
    {
        g1.close();
        g2.close();
    }

#else
    ofstream g1(ssh.str().c_str());
    ofstream g2(sss.str().c_str());

    g1 << GlobalV::NLOCAL;
    g2 << GlobalV::NLOCAL;

    for (int i=0; i<GlobalV::NLOCAL; i++)
    {
        for (int j=i; j<GlobalV::NLOCAL; j++)
        {
            g1 << " " << H[i*GlobalV::NLOCAL+j];
            g2 << " " << S[i*GlobalV::NLOCAL+j];
        }
    }
    g1.close();
    g2.close();
#endif

    timer::tick("HS_Matrix","save_HSR_tr");
    return;
}

void HS_Matrix::save_HSR_sparse(const int &current_spin, const double &sparse_threshold, const bool &binary)
{
    TITLE("HS_Matrix","save_HSR_sparse");
    timer::tick("HS_Matrix","save_HSR_sparse");

    auto &HR_sparse_ptr = GlobalC::LM.HR_sparse;
    auto &HR_soc_sparse_ptr = GlobalC::LM.HR_soc_sparse;
    auto &SR_sparse_ptr = GlobalC::LM.SR_sparse;
    auto &SR_soc_sparse_ptr = GlobalC::LM.SR_soc_sparse;

    int R_x = GlobalC::GridD.getCellX();
    int R_y = GlobalC::GridD.getCellY();
    int R_z = GlobalC::GridD.getCellZ();

    double R_minX = GlobalC::GridD.getD_minX();
    double R_minY = GlobalC::GridD.getD_minY();
    double R_minZ = GlobalC::GridD.getD_minZ();

    int total_R_number = R_x * R_y * R_z;
    int output_R_number = 0;
    int *H_nonzero_number = new int[total_R_number];
    int *S_nonzero_number = new int[total_R_number];
    int count_n = 0;
    for (int ix = 0; ix < R_x; ++ix)
    {
        for (int iy = 0; iy < R_y; ++iy)
        {
            for (int iz = 0; iz < R_z; ++iz)
            {
                H_nonzero_number[count_n] = 0;
                S_nonzero_number[count_n] = 0;
                if (GlobalV::NSPIN != 4)
                {
                    for (auto &iter : HR_sparse_ptr[ix][iy][iz])
                    {
                        H_nonzero_number[count_n] += iter.second.size();
                    }
                    for (auto &iter : SR_sparse_ptr[ix][iy][iz])
                    {
                        S_nonzero_number[count_n] += iter.second.size();
                    }
                }
                else
                {
                    for (auto &iter : HR_soc_sparse_ptr[ix][iy][iz])
                    {
                        H_nonzero_number[count_n] += iter.second.size();
                    }
                    for (auto &iter : SR_soc_sparse_ptr[ix][iy][iz])
                    {
                        S_nonzero_number[count_n] += iter.second.size();
                    }
                }

                count_n++;
            }
        }
    }

    Parallel_Reduce::reduce_int_all(H_nonzero_number, total_R_number);
    Parallel_Reduce::reduce_int_all(S_nonzero_number, total_R_number);

    for (int index = 0; index < total_R_number; ++index)
    {
        if (H_nonzero_number[index] == 0 && S_nonzero_number[index] == 0)
        {
            // do nothing
        } 
        else
        {
            output_R_number++;
        }
    }

    stringstream ssh;
    stringstream sss;
    ssh << GlobalV::global_out_dir << "data-HR-sparse_SPIN" << current_spin << ".csr";
    sss << GlobalV::global_out_dir << "data-SR-sparse_SPIN" << current_spin << ".csr";
    ofstream g1;
    ofstream g2;

    if(GlobalV::DRANK==0)
    {
        if (binary)
        {
            g1.open(ssh.str().c_str(), ios::binary);
            g1.write(reinterpret_cast<char *>(&GlobalV::NLOCAL), sizeof(int));
            g1.write(reinterpret_cast<char *>(&output_R_number), sizeof(int));

            g2.open(sss.str().c_str(), ios::binary);
            g2.write(reinterpret_cast<char *>(&GlobalV::NLOCAL), sizeof(int));
            g2.write(reinterpret_cast<char *>(&output_R_number), sizeof(int));
        }
        else
        {
            g1.open(ssh.str().c_str());
            g1 << "Matrix Dimension of H(R): " << GlobalV::NLOCAL <<endl;
            g1 << "Matrix number of H(R): " << output_R_number << endl;

            g2.open(sss.str().c_str());
            g2 << "Matrix Dimension of S(R): " << GlobalV::NLOCAL <<endl;
            g2 << "Matrix number of S(R): " << output_R_number << endl;
        }
    }

    count_n = 0;
    for(int ix=0; ix<R_x; ix++)
    {
        int dRx = ix + R_minX;
        for(int iy=0; iy<R_y; iy++)
        {
            int dRy = iy + R_minY;
            for(int iz=0; iz<R_z; iz++)
            {
                int dRz = iz + R_minZ;

                if (H_nonzero_number[count_n] == 0 && S_nonzero_number[count_n] == 0) 
                {
                    count_n++;
                    continue;
                }

                if (GlobalV::DRANK == 0) 
                {
                    if (binary)
                    {
                        g1.write(reinterpret_cast<char *>(&dRx), sizeof(int));
                        g1.write(reinterpret_cast<char *>(&dRy), sizeof(int));
                        g1.write(reinterpret_cast<char *>(&dRz), sizeof(int));
                        g1.write(reinterpret_cast<char *>(&H_nonzero_number[count_n]), sizeof(int));

                        g2.write(reinterpret_cast<char *>(&dRx), sizeof(int));
                        g2.write(reinterpret_cast<char *>(&dRy), sizeof(int));
                        g2.write(reinterpret_cast<char *>(&dRz), sizeof(int));
                        g2.write(reinterpret_cast<char *>(&S_nonzero_number[count_n]), sizeof(int));
                    }
                    else
                    {
                        g1 << dRx << " " << dRy << " " << dRz << " " << H_nonzero_number[count_n] << endl;
                        g2 << dRx << " " << dRy << " " << dRz << " " << S_nonzero_number[count_n] << endl;
                    }
                }

                if (H_nonzero_number[count_n] == 0)
                {
                    // if (GlobalV::DRANK == 0)
                    // {
                    //     if (!binary)
                    //     {
                    //         g1 << endl;
                    //         g1 << endl;
                    //         for (int index = 0; index < GlobalV::NLOCAL+1; ++index)
                    //         {
                    //             g1 << 0 << " ";
                    //         }
                    //         g1 << endl;
                    //     }
                    // }
                }
                else
                {
                    if (GlobalV::NSPIN != 4)
                    {
                        output_single_R(g1, HR_sparse_ptr[ix][iy][iz], sparse_threshold, binary);
                    }
                    else
                    {
                        output_soc_single_R(g1, HR_soc_sparse_ptr[ix][iy][iz], sparse_threshold, binary);
                    }
                }

                if (S_nonzero_number[count_n] == 0)
                {
                    // if (!binary)
                    // {
                    //     if (GlobalV::DRANK == 0)
                    //     {
                    //         g2 << endl;
                    //         g2 << endl;
                    //         for (int index = 0; index < GlobalV::NLOCAL+1; ++index)
                    //         {
                    //             g2 << 0 << " ";
                    //         }
                    //         g2 << endl;
                    //     }
                    // }
                }
                else
                {
                    if (GlobalV::NSPIN != 4)
                    {
                        output_single_R(g2, SR_sparse_ptr[ix][iy][iz], sparse_threshold, binary);
                    }
                    else
                    {
                        output_soc_single_R(g2, SR_soc_sparse_ptr[ix][iy][iz], sparse_threshold, binary);
                    }
                }

                count_n++;

            }
        }
    }

    if(GlobalV::DRANK==0) 
    {
        g1.close();
        g2.close();
    }

    delete[] H_nonzero_number;
    delete[] S_nonzero_number;
    H_nonzero_number = nullptr;
    S_nonzero_number = nullptr;

    timer::tick("HS_Matrix","save_HSR_sparse");
    return;
}

void HS_Matrix::output_single_R(ofstream &ofs, const map<size_t, map<size_t, double>> &XR, const double &sparse_threshold, const bool &binary)
{
    double *line = nullptr;
    vector<int> indptr;
    indptr.reserve(GlobalV::NLOCAL + 1);
    indptr.push_back(0);

    stringstream tem1;
    tem1 << GlobalV::global_out_dir << "temp_sparse_indices.dat";
    ofstream ofs_tem1;
    ifstream ifs_tem1;

    if (GlobalV::DRANK == 0)
    {
        if (binary)
        {
            ofs_tem1.open(tem1.str().c_str(), ios::binary);
        }
        else
        {
            ofs_tem1.open(tem1.str().c_str());
        }
    }

    for(int row = 0; row < GlobalV::NLOCAL; ++row)
    {
        line = new double[GlobalV::NLOCAL];
        ZEROS(line, GlobalV::NLOCAL);

        if(GlobalC::ParaO.trace_loc_row[row] >= 0)
        {
            auto iter = XR.find(row);
            if (iter != XR.end())
            {
                for (auto &value : iter->second)
                {
                    line[value.first] = value.second;
                }
            }
        }

        Parallel_Reduce::reduce_double_all(line, GlobalV::NLOCAL);

        if(GlobalV::DRANK == 0)
        {
            int nonzeros_count = 0;
            for (int col = 0; col < GlobalV::NLOCAL; ++col)
            {
                if (abs(line[col]) > sparse_threshold)
                {
                    if (binary)
                    {
                        ofs.write(reinterpret_cast<char *>(&line[col]), sizeof(double));
                        ofs_tem1.write(reinterpret_cast<char *>(&col), sizeof(int));
                    }
                    else
                    {
                        ofs << " " << fixed << scientific << setprecision(8) << line[col];
                        ofs_tem1 << " " << col;
                    }

                    nonzeros_count++;

                }

            }
            nonzeros_count += indptr.back();
            indptr.push_back(nonzeros_count);
        }

        delete[] line;
        line = nullptr;

    }

    if (GlobalV::DRANK == 0)
    {
        if (binary)
        {
            ofs_tem1.close();
            ifs_tem1.open(tem1.str().c_str(), ios::binary);
            ofs << ifs_tem1.rdbuf();
            ifs_tem1.close();
            for (auto &i : indptr)
            {
                ofs.write(reinterpret_cast<char *>(&i), sizeof(int));
            }
        }
        else
        {
            ofs << endl;
            ofs_tem1 << endl;
            ofs_tem1.close();
            ifs_tem1.open(tem1.str().c_str());
            ofs << ifs_tem1.rdbuf();
            ifs_tem1.close();
            for (auto &i : indptr)
            {
                ofs << " " << i;
            }
            ofs << endl;
        }

        std::remove(tem1.str().c_str());

    }

}

void HS_Matrix::output_soc_single_R(ofstream &ofs, const map<size_t, map<size_t, complex<double>>> &XR, const double &sparse_threshold, const bool &binary)
{
    complex<double> *line = nullptr;
    vector<int> indptr;
    indptr.reserve(GlobalV::NLOCAL + 1);
    indptr.push_back(0);

    stringstream tem1;
    tem1 << GlobalV::global_out_dir << "temp_sparse_indices.dat";
    ofstream ofs_tem1;
    ifstream ifs_tem1;

    if (GlobalV::DRANK == 0)
    {
        if (binary)
        {
            ofs_tem1.open(tem1.str().c_str(), ios::binary);
        }
        else
        {
            ofs_tem1.open(tem1.str().c_str());
        }
    }

    for(int row = 0; row < GlobalV::NLOCAL; ++row)
    {
        line = new complex<double>[GlobalV::NLOCAL];
        ZEROS(line, GlobalV::NLOCAL);

        if(GlobalC::ParaO.trace_loc_row[row] >= 0)
        {
            auto iter = XR.find(row);
            if (iter != XR.end())
            {
                for (auto &value : iter->second)
                {
                    line[value.first] = value.second;
                }
            }
        }

        Parallel_Reduce::reduce_complex_double_all(line, GlobalV::NLOCAL);

        if (GlobalV::DRANK == 0)
        {
            int nonzeros_count = 0;
            for (int col = 0; col < GlobalV::NLOCAL; ++col)
            {
                if (abs(line[col]) > sparse_threshold)
                {
                    if (binary)
                    {
                        ofs.write(reinterpret_cast<char *>(&line[col]), sizeof(complex<double>));
                        ofs_tem1.write(reinterpret_cast<char *>(&col), sizeof(int));
                    }
                    else
                    {
                        ofs << " (" << fixed << scientific << setprecision(8) << line[col].real() << "," 
                                    << fixed << scientific << setprecision(8) << line[col].imag() << ")";
                        ofs_tem1 << " " << col;
                    }

                    nonzeros_count++;

                }

            }
            nonzeros_count += indptr.back();
            indptr.push_back(nonzeros_count);
        }

        delete[] line;
        line = nullptr;

    }

    if (GlobalV::DRANK == 0)
    {
        if (binary)
        {
            ofs_tem1.close();
            ifs_tem1.open(tem1.str().c_str(), ios::binary);
            ofs << ifs_tem1.rdbuf();
            ifs_tem1.close();
            for (auto &i : indptr)
            {
                ofs.write(reinterpret_cast<char *>(&i), sizeof(int));
            }
        }
        else
        {
            ofs << endl;
            ofs_tem1 << endl;
            ofs_tem1.close();
            ifs_tem1.open(tem1.str().c_str());
            ofs << ifs_tem1.rdbuf();
            ifs_tem1.close();
            for (auto &i : indptr)
            {
                ofs << " " << i;
            }
            ofs << endl;
        }

        std::remove(tem1.str().c_str());
    }

} 