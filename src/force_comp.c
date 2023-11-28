#include <math.h>
#include "types.h"
#include "utilities.h"

#ifdef _OPENMP
    #include "omp.h"
#endif

void force(mdsys_t *sys) {
    
    double epot = 0.00000;
    double c12,c6,rcsq;
    int j, tid, start, end;
    double pow6, pow12;

    /* defining pow by repeating product */
    pow6 = sys->sigma * sys->sigma * sys->sigma * sys->sigma * sys->sigma * sys->sigma;
    pow12 = sys->sigma * sys->sigma * sys->sigma * sys->sigma * sys->sigma * sys->sigma *\
                sys->sigma * sys->sigma * sys->sigma * sys->sigma * sys->sigma * sys->sigma;

    /* zero energy and forces */
    c12 = 4.000 * sys->epsilon * pow12; // pow(sys->sigma, 12.000);
    c6 = 4.000 * sys->epsilon * pow6;      // pow(sys->sigma, 6.000);
    rcsq = sys->rcut * sys->rcut;

    #ifdef _OPENMP
        #pragma omp parallel reduction(+:epot) firstprivate(c12, c6, rcsq, j, tid, start, end, pow6, pow12)
    #endif
    { 
        double *fx, *fy, *fz;
        double rx1, ry1, rz1;
        double rx, ry, rz, rsq;
        double r6, rinv, ffac;
  
        /* reading thread id and defining default tid when omp is absent */
        #ifdef _OPENMP
            tid = omp_get_thread_num();
        #else
            tid = 0;
        #endif

        fx = sys->fx + (tid * sys->natoms);
        fy = sys->fy + (tid * sys->natoms);
        fz = sys->fz + (tid * sys->natoms);

        azzero(fx, sys->natoms);
        azzero(fy, sys->natoms);
        azzero(fz, sys->natoms);

        for (int i = 0; i < (sys->natoms - 1); i += sys->tmax)
        {
            int ii = i + tid;
            if (ii >= (sys->natoms - 1)) break;

            rx1 = sys->rx[ii];
            ry1 = sys->ry[ii];
            rz1 = sys->rz[ii];

            for (j = ii + 1; j < sys->natoms; ++j)
            {
                rx = pbc(rx1 - sys->rx[j], 0.500 * sys->box);
                ry = pbc(ry1 - sys->ry[j], 0.500 * sys->box);
                rz = pbc(rz1 - sys->rz[j], 0.500 * sys->box);
                rsq = (rx * rx) + (ry * ry) + (rz * rz);
                    
                if (rsq < rcsq)
                {
                    rinv = 1.0 / rsq;
                    r6 = rinv * rinv * rinv;
                    ffac = (12.000 * c12 * r6 - 6.000 * c6) * r6 * rinv;
                    epot += r6 * (c12 * r6 - c6);
                    fx[ii] += rx * ffac;
                    fx[j] -= rx * ffac;
                    fy[ii] += ry * ffac;
                    fy[j] -= ry * ffac;
                    fz[ii] += rz * ffac;
                    fz[j] -= rz * ffac;
                }
            }
        }
        
        #ifdef _OPENMP
            #pragma omp barrier
        #endif
        
        int i = 1 + sys->natoms / sys->tmax;
        start = tid * i;
        end = start + i;

        if(end > sys->natoms) 
        {
            end = sys->natoms; 
        }

        for(int i = 1; i < sys->tmax; ++i)
        {   
            int offset = i * sys->natoms;
            for(int j = start; j < end; ++j)
            {   
                sys->fx[j] += sys->fx[offset + j];
                sys->fy[j] += sys->fy[offset + j];
                sys->fz[j] += sys->fz[offset + j];
            }
        }
    }
    sys->epot = epot;
}