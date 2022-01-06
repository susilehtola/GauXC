#include <math.h>
#include "../include/chebyshev_boys_computation.hpp"
#include "integral_data_types.hpp"
#include "config_obara_saika.hpp"
#include "integral_3_0.hpp"

#define PI 3.14159265358979323846

#define MIN(a,b)			\
  ({ __typeof__ (a) _a = (a);	        \
  __typeof__ (b) _b = (b);		\
  _a < _b ? _a : _b; })

void integral_3_0(size_t npts,
                  shell_pair shpair,
                  double *_points,
                  double *Xi,
                  double *Xj,
                  int ldX,
                  double *Gi,
                  double *Gj,
                  int ldG, 
                  double *weights) {
   __attribute__((__aligned__(64))) double buffer[10 * NPTS_LOCAL + 7 * NPTS_LOCAL + NPTS_LOCAL];

   double *temp = (buffer + 0);
   double *FmT = (buffer + 10 * NPTS_LOCAL);
   double *Tval = (buffer + 10 * NPTS_LOCAL + 7 * NPTS_LOCAL);

   size_t npts_upper = NPTS_LOCAL * (npts / NPTS_LOCAL);
   size_t p_outer = 0;
   for(p_outer = 0; p_outer < npts_upper; p_outer += NPTS_LOCAL) {
      double *_point_outer = (_points + p_outer);

      for(int i = 0; i < 10 * NPTS_LOCAL; i += SIMD_LENGTH) SIMD_ALIGNED_STORE((temp + i), SIMD_ZERO());

      for(int ij = 0; ij < shpair.nprim_pair; ++ij) {
         double RHO = shpair.prim_pairs[ij].gamma;
         double RHO_INV = 1.0 / RHO;
         double X_PA = shpair.prim_pairs[ij].PA.x;
         double Y_PA = shpair.prim_pairs[ij].PA.y;
         double Z_PA = shpair.prim_pairs[ij].PA.z;

         double xP = shpair.prim_pairs[ij].P.x;
         double yP = shpair.prim_pairs[ij].P.y;
         double zP = shpair.prim_pairs[ij].P.z;

         double eval = shpair.prim_pairs[ij].coeff_prod * shpair.prim_pairs[ij].K;

         // Evaluate T Values
         for(size_t p_inner = 0; p_inner < NPTS_LOCAL; p_inner += SIMD_LENGTH) {
            SIMD_TYPE xC = SIMD_UNALIGNED_LOAD((_point_outer + p_inner + 0 * npts));
            SIMD_TYPE yC = SIMD_UNALIGNED_LOAD((_point_outer + p_inner + 1 * npts));
            SIMD_TYPE zC = SIMD_UNALIGNED_LOAD((_point_outer + p_inner + 2 * npts));

            SIMD_TYPE X_PC = SIMD_SUB(SIMD_DUPLICATE(&(xP)), xC);
            SIMD_TYPE Y_PC = SIMD_SUB(SIMD_DUPLICATE(&(yP)), yC);
            SIMD_TYPE Z_PC = SIMD_SUB(SIMD_DUPLICATE(&(zP)), zC);

            X_PC = SIMD_MUL(X_PC, X_PC);
            X_PC = SIMD_FMA(Y_PC, Y_PC, X_PC);
            X_PC = SIMD_FMA(Z_PC, Z_PC, X_PC);
            X_PC = SIMD_MUL(SIMD_DUPLICATE(&(RHO)), X_PC);
            SIMD_ALIGNED_STORE((Tval + p_inner), X_PC);
         }

         // Evaluate Boys function
         GauXC::gauxc_boys_elements<0>(NPTS_LOCAL, Tval, FmT + 0 * NPTS_LOCAL);
         GauXC::gauxc_boys_elements<1>(NPTS_LOCAL, Tval, FmT + 1 * NPTS_LOCAL);
         GauXC::gauxc_boys_elements<2>(NPTS_LOCAL, Tval, FmT + 2 * NPTS_LOCAL);
         GauXC::gauxc_boys_elements<3>(NPTS_LOCAL, Tval, FmT + 3 * NPTS_LOCAL);

         // Evaluate VRR Buffer
         for(size_t p_inner = 0; p_inner < NPTS_LOCAL; p_inner += SIMD_LENGTH) {
            SIMD_TYPE xC = SIMD_UNALIGNED_LOAD((_point_outer + p_inner + 0 * npts));
            SIMD_TYPE yC = SIMD_UNALIGNED_LOAD((_point_outer + p_inner + 1 * npts));
            SIMD_TYPE zC = SIMD_UNALIGNED_LOAD((_point_outer + p_inner + 2 * npts));

            SIMD_TYPE X_PC = SIMD_SUB(SIMD_DUPLICATE(&(xP)), xC);
            SIMD_TYPE Y_PC = SIMD_SUB(SIMD_DUPLICATE(&(yP)), yC);
            SIMD_TYPE Z_PC = SIMD_SUB(SIMD_DUPLICATE(&(zP)), zC);

            SIMD_TYPE tx, ty, t00, t01, t02, t03, t10, t11, t12, t20, t21, t30;

            t00 = SIMD_ALIGNED_LOAD((FmT + p_inner + 0 * NPTS_LOCAL));
            t00 = SIMD_MUL(SIMD_DUPLICATE(&(eval)), t00);
            t01 = SIMD_ALIGNED_LOAD((FmT + p_inner + 1 * NPTS_LOCAL));
            t01 = SIMD_MUL(SIMD_DUPLICATE(&(eval)), t01);
            t02 = SIMD_ALIGNED_LOAD((FmT + p_inner + 2 * NPTS_LOCAL));
            t02 = SIMD_MUL(SIMD_DUPLICATE(&(eval)), t02);
            t03 = SIMD_ALIGNED_LOAD((FmT + p_inner + 3 * NPTS_LOCAL));
            t03 = SIMD_MUL(SIMD_DUPLICATE(&(eval)), t03);
            t10 = SIMD_MUL(SIMD_DUPLICATE(&(X_PA)), t00);
            t10 = SIMD_FNMA(X_PC, t01, t10);
            t11 = SIMD_MUL(SIMD_DUPLICATE(&(X_PA)), t01);
            t11 = SIMD_FNMA(X_PC, t02, t11);
            t12 = SIMD_MUL(SIMD_DUPLICATE(&(X_PA)), t02);
            t12 = SIMD_FNMA(X_PC, t03, t12);
            t20 = SIMD_MUL(SIMD_DUPLICATE(&(X_PA)), t10);
            t20 = SIMD_FNMA(X_PC, t11, t20);
            tx = SIMD_SUB(t00, t01);
            ty = SIMD_SET1(0.5 * 1);
            ty = SIMD_MUL(ty, SIMD_DUPLICATE(&(RHO_INV)));
            t20 = SIMD_FMA(tx, ty, t20);
            t21 = SIMD_MUL(SIMD_DUPLICATE(&(X_PA)), t11);
            t21 = SIMD_FNMA(X_PC, t12, t21);
            tx = SIMD_SUB(t01, t02);
            ty = SIMD_SET1(0.5 * 1);
            ty = SIMD_MUL(ty, SIMD_DUPLICATE(&(RHO_INV)));
            t21 = SIMD_FMA(tx, ty, t21);
            t30 = SIMD_MUL(SIMD_DUPLICATE(&(X_PA)), t20);
            t30 = SIMD_FNMA(X_PC, t21, t30);
            tx = SIMD_SUB(t10, t11);
            ty = SIMD_SET1(0.5 * 2);
            ty = SIMD_MUL(ty, SIMD_DUPLICATE(&(RHO_INV)));
            t30 = SIMD_FMA(tx, ty, t30);
            tx = SIMD_ALIGNED_LOAD((temp + 0 * NPTS_LOCAL + p_inner));
            tx = SIMD_ADD(tx, t30);
            SIMD_ALIGNED_STORE((temp + 0 * NPTS_LOCAL + p_inner), tx);
            t30 = SIMD_MUL(SIMD_DUPLICATE(&(Y_PA)), t20);
            t30 = SIMD_FNMA(Y_PC, t21, t30);
            tx = SIMD_ALIGNED_LOAD((temp + 1 * NPTS_LOCAL + p_inner));
            tx = SIMD_ADD(tx, t30);
            SIMD_ALIGNED_STORE((temp + 1 * NPTS_LOCAL + p_inner), tx);
            t30 = SIMD_MUL(SIMD_DUPLICATE(&(Z_PA)), t20);
            t30 = SIMD_FNMA(Z_PC, t21, t30);
            tx = SIMD_ALIGNED_LOAD((temp + 2 * NPTS_LOCAL + p_inner));
            tx = SIMD_ADD(tx, t30);
            SIMD_ALIGNED_STORE((temp + 2 * NPTS_LOCAL + p_inner), tx);
            t20 = SIMD_MUL(SIMD_DUPLICATE(&(Y_PA)), t10);
            t20 = SIMD_FNMA(Y_PC, t11, t20);
            t21 = SIMD_MUL(SIMD_DUPLICATE(&(Y_PA)), t11);
            t21 = SIMD_FNMA(Y_PC, t12, t21);
            t30 = SIMD_MUL(SIMD_DUPLICATE(&(Y_PA)), t20);
            t30 = SIMD_FNMA(Y_PC, t21, t30);
            tx = SIMD_SUB(t10, t11);
            ty = SIMD_SET1(0.5 * 1);
            ty = SIMD_MUL(ty, SIMD_DUPLICATE(&(RHO_INV)));
            t30 = SIMD_FMA(tx, ty, t30);
            tx = SIMD_ALIGNED_LOAD((temp + 3 * NPTS_LOCAL + p_inner));
            tx = SIMD_ADD(tx, t30);
            SIMD_ALIGNED_STORE((temp + 3 * NPTS_LOCAL + p_inner), tx);
            t30 = SIMD_MUL(SIMD_DUPLICATE(&(Z_PA)), t20);
            t30 = SIMD_FNMA(Z_PC, t21, t30);
            tx = SIMD_ALIGNED_LOAD((temp + 4 * NPTS_LOCAL + p_inner));
            tx = SIMD_ADD(tx, t30);
            SIMD_ALIGNED_STORE((temp + 4 * NPTS_LOCAL + p_inner), tx);
            t20 = SIMD_MUL(SIMD_DUPLICATE(&(Z_PA)), t10);
            t20 = SIMD_FNMA(Z_PC, t11, t20);
            t21 = SIMD_MUL(SIMD_DUPLICATE(&(Z_PA)), t11);
            t21 = SIMD_FNMA(Z_PC, t12, t21);
            t30 = SIMD_MUL(SIMD_DUPLICATE(&(Z_PA)), t20);
            t30 = SIMD_FNMA(Z_PC, t21, t30);
            tx = SIMD_SUB(t10, t11);
            ty = SIMD_SET1(0.5 * 1);
            ty = SIMD_MUL(ty, SIMD_DUPLICATE(&(RHO_INV)));
            t30 = SIMD_FMA(tx, ty, t30);
            tx = SIMD_ALIGNED_LOAD((temp + 5 * NPTS_LOCAL + p_inner));
            tx = SIMD_ADD(tx, t30);
            SIMD_ALIGNED_STORE((temp + 5 * NPTS_LOCAL + p_inner), tx);
            t10 = SIMD_MUL(SIMD_DUPLICATE(&(Y_PA)), t00);
            t10 = SIMD_FNMA(Y_PC, t01, t10);
            t11 = SIMD_MUL(SIMD_DUPLICATE(&(Y_PA)), t01);
            t11 = SIMD_FNMA(Y_PC, t02, t11);
            t12 = SIMD_MUL(SIMD_DUPLICATE(&(Y_PA)), t02);
            t12 = SIMD_FNMA(Y_PC, t03, t12);
            t20 = SIMD_MUL(SIMD_DUPLICATE(&(Y_PA)), t10);
            t20 = SIMD_FNMA(Y_PC, t11, t20);
            tx = SIMD_SUB(t00, t01);
            ty = SIMD_SET1(0.5 * 1);
            ty = SIMD_MUL(ty, SIMD_DUPLICATE(&(RHO_INV)));
            t20 = SIMD_FMA(tx, ty, t20);
            t21 = SIMD_MUL(SIMD_DUPLICATE(&(Y_PA)), t11);
            t21 = SIMD_FNMA(Y_PC, t12, t21);
            tx = SIMD_SUB(t01, t02);
            ty = SIMD_SET1(0.5 * 1);
            ty = SIMD_MUL(ty, SIMD_DUPLICATE(&(RHO_INV)));
            t21 = SIMD_FMA(tx, ty, t21);
            t30 = SIMD_MUL(SIMD_DUPLICATE(&(Y_PA)), t20);
            t30 = SIMD_FNMA(Y_PC, t21, t30);
            tx = SIMD_SUB(t10, t11);
            ty = SIMD_SET1(0.5 * 2);
            ty = SIMD_MUL(ty, SIMD_DUPLICATE(&(RHO_INV)));
            t30 = SIMD_FMA(tx, ty, t30);
            tx = SIMD_ALIGNED_LOAD((temp + 6 * NPTS_LOCAL + p_inner));
            tx = SIMD_ADD(tx, t30);
            SIMD_ALIGNED_STORE((temp + 6 * NPTS_LOCAL + p_inner), tx);
            t30 = SIMD_MUL(SIMD_DUPLICATE(&(Z_PA)), t20);
            t30 = SIMD_FNMA(Z_PC, t21, t30);
            tx = SIMD_ALIGNED_LOAD((temp + 7 * NPTS_LOCAL + p_inner));
            tx = SIMD_ADD(tx, t30);
            SIMD_ALIGNED_STORE((temp + 7 * NPTS_LOCAL + p_inner), tx);
            t20 = SIMD_MUL(SIMD_DUPLICATE(&(Z_PA)), t10);
            t20 = SIMD_FNMA(Z_PC, t11, t20);
            t21 = SIMD_MUL(SIMD_DUPLICATE(&(Z_PA)), t11);
            t21 = SIMD_FNMA(Z_PC, t12, t21);
            t30 = SIMD_MUL(SIMD_DUPLICATE(&(Z_PA)), t20);
            t30 = SIMD_FNMA(Z_PC, t21, t30);
            tx = SIMD_SUB(t10, t11);
            ty = SIMD_SET1(0.5 * 1);
            ty = SIMD_MUL(ty, SIMD_DUPLICATE(&(RHO_INV)));
            t30 = SIMD_FMA(tx, ty, t30);
            tx = SIMD_ALIGNED_LOAD((temp + 8 * NPTS_LOCAL + p_inner));
            tx = SIMD_ADD(tx, t30);
            SIMD_ALIGNED_STORE((temp + 8 * NPTS_LOCAL + p_inner), tx);
            t10 = SIMD_MUL(SIMD_DUPLICATE(&(Z_PA)), t00);
            t10 = SIMD_FNMA(Z_PC, t01, t10);
            t11 = SIMD_MUL(SIMD_DUPLICATE(&(Z_PA)), t01);
            t11 = SIMD_FNMA(Z_PC, t02, t11);
            t12 = SIMD_MUL(SIMD_DUPLICATE(&(Z_PA)), t02);
            t12 = SIMD_FNMA(Z_PC, t03, t12);
            t20 = SIMD_MUL(SIMD_DUPLICATE(&(Z_PA)), t10);
            t20 = SIMD_FNMA(Z_PC, t11, t20);
            tx = SIMD_SUB(t00, t01);
            ty = SIMD_SET1(0.5 * 1);
            ty = SIMD_MUL(ty, SIMD_DUPLICATE(&(RHO_INV)));
            t20 = SIMD_FMA(tx, ty, t20);
            t21 = SIMD_MUL(SIMD_DUPLICATE(&(Z_PA)), t11);
            t21 = SIMD_FNMA(Z_PC, t12, t21);
            tx = SIMD_SUB(t01, t02);
            ty = SIMD_SET1(0.5 * 1);
            ty = SIMD_MUL(ty, SIMD_DUPLICATE(&(RHO_INV)));
            t21 = SIMD_FMA(tx, ty, t21);
            t30 = SIMD_MUL(SIMD_DUPLICATE(&(Z_PA)), t20);
            t30 = SIMD_FNMA(Z_PC, t21, t30);
            tx = SIMD_SUB(t10, t11);
            ty = SIMD_SET1(0.5 * 2);
            ty = SIMD_MUL(ty, SIMD_DUPLICATE(&(RHO_INV)));
            t30 = SIMD_FMA(tx, ty, t30);
            tx = SIMD_ALIGNED_LOAD((temp + 9 * NPTS_LOCAL + p_inner));
            tx = SIMD_ADD(tx, t30);
            SIMD_ALIGNED_STORE((temp + 9 * NPTS_LOCAL + p_inner), tx);
         }
      }

      for(size_t p_inner = 0; p_inner < NPTS_LOCAL; p_inner += SIMD_LENGTH) {
         double *Xik = (Xi + p_outer + p_inner);
         double *Xjk = (Xj + p_outer + p_inner);
         double *Gik = (Gi + p_outer + p_inner);
         double *Gjk = (Gj + p_outer + p_inner);

         SIMD_TYPE const_value_v = SIMD_UNALIGNED_LOAD((weights + p_outer + p_inner));

         double const_value, X_ABp, Y_ABp, Z_ABp, comb_m_i, comb_n_j, comb_p_k;
         SIMD_TYPE const_value_w;
         SIMD_TYPE tx, ty, tz, tw, t0, t1, t2, t3, t4, t5, t6, t7, t8, t9;

         X_ABp = 1.0; comb_m_i = 1.0;
         Y_ABp = 1.0; comb_n_j = 1.0;
         Z_ABp = 1.0; comb_p_k = 1.0;
         const_value = comb_m_i * comb_n_j * comb_p_k * X_ABp * Y_ABp * Z_ABp;
         const_value_w = SIMD_MUL(const_value_v, SIMD_DUPLICATE(&(const_value)));
         tx = SIMD_UNALIGNED_LOAD((Xik + 0 * ldX));
         ty = SIMD_UNALIGNED_LOAD((Xjk + 0 * ldX));
         tz = SIMD_UNALIGNED_LOAD((Gik + 0 * ldG));
         tw = SIMD_UNALIGNED_LOAD((Gjk + 0 * ldG));
         t0 = SIMD_ALIGNED_LOAD((temp + 0 * NPTS_LOCAL + p_inner));
         t0 = SIMD_MUL(t0, const_value_w);
         tz = SIMD_FMA(ty, t0, tz);
         tw = SIMD_FMA(tx, t0, tw);
         SIMD_UNALIGNED_STORE((Gik + 0 * ldG), tz);
         SIMD_UNALIGNED_STORE((Gjk + 0 * ldG), tw);
         tx = SIMD_UNALIGNED_LOAD((Xik + 1 * ldX));
         ty = SIMD_UNALIGNED_LOAD((Xjk + 0 * ldX));
         tz = SIMD_UNALIGNED_LOAD((Gik + 1 * ldG));
         tw = SIMD_UNALIGNED_LOAD((Gjk + 0 * ldG));
         t1 = SIMD_ALIGNED_LOAD((temp + 1 * NPTS_LOCAL + p_inner));
         t1 = SIMD_MUL(t1, const_value_w);
         tz = SIMD_FMA(ty, t1, tz);
         tw = SIMD_FMA(tx, t1, tw);
         SIMD_UNALIGNED_STORE((Gik + 1 * ldG), tz);
         SIMD_UNALIGNED_STORE((Gjk + 0 * ldG), tw);
         tx = SIMD_UNALIGNED_LOAD((Xik + 2 * ldX));
         ty = SIMD_UNALIGNED_LOAD((Xjk + 0 * ldX));
         tz = SIMD_UNALIGNED_LOAD((Gik + 2 * ldG));
         tw = SIMD_UNALIGNED_LOAD((Gjk + 0 * ldG));
         t2 = SIMD_ALIGNED_LOAD((temp + 2 * NPTS_LOCAL + p_inner));
         t2 = SIMD_MUL(t2, const_value_w);
         tz = SIMD_FMA(ty, t2, tz);
         tw = SIMD_FMA(tx, t2, tw);
         SIMD_UNALIGNED_STORE((Gik + 2 * ldG), tz);
         SIMD_UNALIGNED_STORE((Gjk + 0 * ldG), tw);
         tx = SIMD_UNALIGNED_LOAD((Xik + 3 * ldX));
         ty = SIMD_UNALIGNED_LOAD((Xjk + 0 * ldX));
         tz = SIMD_UNALIGNED_LOAD((Gik + 3 * ldG));
         tw = SIMD_UNALIGNED_LOAD((Gjk + 0 * ldG));
         t3 = SIMD_ALIGNED_LOAD((temp + 3 * NPTS_LOCAL + p_inner));
         t3 = SIMD_MUL(t3, const_value_w);
         tz = SIMD_FMA(ty, t3, tz);
         tw = SIMD_FMA(tx, t3, tw);
         SIMD_UNALIGNED_STORE((Gik + 3 * ldG), tz);
         SIMD_UNALIGNED_STORE((Gjk + 0 * ldG), tw);
         tx = SIMD_UNALIGNED_LOAD((Xik + 4 * ldX));
         ty = SIMD_UNALIGNED_LOAD((Xjk + 0 * ldX));
         tz = SIMD_UNALIGNED_LOAD((Gik + 4 * ldG));
         tw = SIMD_UNALIGNED_LOAD((Gjk + 0 * ldG));
         t4 = SIMD_ALIGNED_LOAD((temp + 4 * NPTS_LOCAL + p_inner));
         t4 = SIMD_MUL(t4, const_value_w);
         tz = SIMD_FMA(ty, t4, tz);
         tw = SIMD_FMA(tx, t4, tw);
         SIMD_UNALIGNED_STORE((Gik + 4 * ldG), tz);
         SIMD_UNALIGNED_STORE((Gjk + 0 * ldG), tw);
         tx = SIMD_UNALIGNED_LOAD((Xik + 5 * ldX));
         ty = SIMD_UNALIGNED_LOAD((Xjk + 0 * ldX));
         tz = SIMD_UNALIGNED_LOAD((Gik + 5 * ldG));
         tw = SIMD_UNALIGNED_LOAD((Gjk + 0 * ldG));
         t5 = SIMD_ALIGNED_LOAD((temp + 5 * NPTS_LOCAL + p_inner));
         t5 = SIMD_MUL(t5, const_value_w);
         tz = SIMD_FMA(ty, t5, tz);
         tw = SIMD_FMA(tx, t5, tw);
         SIMD_UNALIGNED_STORE((Gik + 5 * ldG), tz);
         SIMD_UNALIGNED_STORE((Gjk + 0 * ldG), tw);
         tx = SIMD_UNALIGNED_LOAD((Xik + 6 * ldX));
         ty = SIMD_UNALIGNED_LOAD((Xjk + 0 * ldX));
         tz = SIMD_UNALIGNED_LOAD((Gik + 6 * ldG));
         tw = SIMD_UNALIGNED_LOAD((Gjk + 0 * ldG));
         t6 = SIMD_ALIGNED_LOAD((temp + 6 * NPTS_LOCAL + p_inner));
         t6 = SIMD_MUL(t6, const_value_w);
         tz = SIMD_FMA(ty, t6, tz);
         tw = SIMD_FMA(tx, t6, tw);
         SIMD_UNALIGNED_STORE((Gik + 6 * ldG), tz);
         SIMD_UNALIGNED_STORE((Gjk + 0 * ldG), tw);
         tx = SIMD_UNALIGNED_LOAD((Xik + 7 * ldX));
         ty = SIMD_UNALIGNED_LOAD((Xjk + 0 * ldX));
         tz = SIMD_UNALIGNED_LOAD((Gik + 7 * ldG));
         tw = SIMD_UNALIGNED_LOAD((Gjk + 0 * ldG));
         t7 = SIMD_ALIGNED_LOAD((temp + 7 * NPTS_LOCAL + p_inner));
         t7 = SIMD_MUL(t7, const_value_w);
         tz = SIMD_FMA(ty, t7, tz);
         tw = SIMD_FMA(tx, t7, tw);
         SIMD_UNALIGNED_STORE((Gik + 7 * ldG), tz);
         SIMD_UNALIGNED_STORE((Gjk + 0 * ldG), tw);
         tx = SIMD_UNALIGNED_LOAD((Xik + 8 * ldX));
         ty = SIMD_UNALIGNED_LOAD((Xjk + 0 * ldX));
         tz = SIMD_UNALIGNED_LOAD((Gik + 8 * ldG));
         tw = SIMD_UNALIGNED_LOAD((Gjk + 0 * ldG));
         t8 = SIMD_ALIGNED_LOAD((temp + 8 * NPTS_LOCAL + p_inner));
         t8 = SIMD_MUL(t8, const_value_w);
         tz = SIMD_FMA(ty, t8, tz);
         tw = SIMD_FMA(tx, t8, tw);
         SIMD_UNALIGNED_STORE((Gik + 8 * ldG), tz);
         SIMD_UNALIGNED_STORE((Gjk + 0 * ldG), tw);
         tx = SIMD_UNALIGNED_LOAD((Xik + 9 * ldX));
         ty = SIMD_UNALIGNED_LOAD((Xjk + 0 * ldX));
         tz = SIMD_UNALIGNED_LOAD((Gik + 9 * ldG));
         tw = SIMD_UNALIGNED_LOAD((Gjk + 0 * ldG));
         t9 = SIMD_ALIGNED_LOAD((temp + 9 * NPTS_LOCAL + p_inner));
         t9 = SIMD_MUL(t9, const_value_w);
         tz = SIMD_FMA(ty, t9, tz);
         tw = SIMD_FMA(tx, t9, tw);
         SIMD_UNALIGNED_STORE((Gik + 9 * ldG), tz);
         SIMD_UNALIGNED_STORE((Gjk + 0 * ldG), tw);
      }
   }

   for(; p_outer < npts; p_outer += NPTS_LOCAL) {
      size_t npts_inner = MIN((size_t) NPTS_LOCAL, npts - p_outer);
      double *_point_outer = (_points + p_outer);

      for(int i = 0; i < 10 * NPTS_LOCAL; i += SIMD_LENGTH) SIMD_ALIGNED_STORE((temp + i), SIMD_ZERO());

      for(int ij = 0; ij < shpair.nprim_pair; ++ij) {
         double RHO = shpair.prim_pairs[ij].gamma;
         double RHO_INV = 1.0 / RHO;
         double X_PA = shpair.prim_pairs[ij].PA.x;
         double Y_PA = shpair.prim_pairs[ij].PA.y;
         double Z_PA = shpair.prim_pairs[ij].PA.z;

         double xP = shpair.prim_pairs[ij].P.x;
         double yP = shpair.prim_pairs[ij].P.y;
         double zP = shpair.prim_pairs[ij].P.z;

         double eval = shpair.prim_pairs[ij].coeff_prod * shpair.prim_pairs[ij].K;

         // Evaluate T Values
         size_t npts_inner_upper = SIMD_LENGTH * (npts_inner / SIMD_LENGTH);
         size_t p_inner = 0;
         for(p_inner = 0; p_inner < npts_inner_upper; p_inner += SIMD_LENGTH) {
            SIMD_TYPE xC = SIMD_UNALIGNED_LOAD((_point_outer + p_inner + 0 * npts));
            SIMD_TYPE yC = SIMD_UNALIGNED_LOAD((_point_outer + p_inner + 1 * npts));
            SIMD_TYPE zC = SIMD_UNALIGNED_LOAD((_point_outer + p_inner + 2 * npts));

            SIMD_TYPE X_PC = SIMD_SUB(SIMD_DUPLICATE(&(xP)), xC);
            SIMD_TYPE Y_PC = SIMD_SUB(SIMD_DUPLICATE(&(yP)), yC);
            SIMD_TYPE Z_PC = SIMD_SUB(SIMD_DUPLICATE(&(zP)), zC);

            X_PC = SIMD_MUL(X_PC, X_PC);
            X_PC = SIMD_FMA(Y_PC, Y_PC, X_PC);
            X_PC = SIMD_FMA(Z_PC, Z_PC, X_PC);
            X_PC = SIMD_MUL(SIMD_DUPLICATE(&(RHO)), X_PC);
            SIMD_ALIGNED_STORE((Tval + p_inner), X_PC);
         }

         for(; p_inner < npts_inner; p_inner += SCALAR_LENGTH) {
            SCALAR_TYPE xC = SCALAR_LOAD((_point_outer + p_inner + 0 * npts));
            SCALAR_TYPE yC = SCALAR_LOAD((_point_outer + p_inner + 1 * npts));
            SCALAR_TYPE zC = SCALAR_LOAD((_point_outer + p_inner + 2 * npts));

            SCALAR_TYPE X_PC = SCALAR_SUB(SCALAR_DUPLICATE(&(xP)), xC);
            SCALAR_TYPE Y_PC = SCALAR_SUB(SCALAR_DUPLICATE(&(yP)), yC);
            SCALAR_TYPE Z_PC = SCALAR_SUB(SCALAR_DUPLICATE(&(zP)), zC);

            X_PC = SCALAR_MUL(X_PC, X_PC);
            X_PC = SCALAR_FMA(Y_PC, Y_PC, X_PC);
            X_PC = SCALAR_FMA(Z_PC, Z_PC, X_PC);
            X_PC = SCALAR_MUL(SCALAR_DUPLICATE(&(RHO)), X_PC);
            SCALAR_STORE((Tval + p_inner), X_PC);
         }

         // Evaluate Boys function
         GauXC::gauxc_boys_elements<0>(npts_inner, Tval, FmT + 0 * NPTS_LOCAL);
         GauXC::gauxc_boys_elements<1>(npts_inner, Tval, FmT + 1 * NPTS_LOCAL);
         GauXC::gauxc_boys_elements<2>(npts_inner, Tval, FmT + 2 * NPTS_LOCAL);
         GauXC::gauxc_boys_elements<3>(npts_inner, Tval, FmT + 3 * NPTS_LOCAL);

         // Evaluate VRR Buffer
         p_inner = 0;
         for(p_inner = 0; p_inner < npts_inner_upper; p_inner += SIMD_LENGTH) {
            SIMD_TYPE xC = SIMD_UNALIGNED_LOAD((_point_outer + p_inner + 0 * npts));
            SIMD_TYPE yC = SIMD_UNALIGNED_LOAD((_point_outer + p_inner + 1 * npts));
            SIMD_TYPE zC = SIMD_UNALIGNED_LOAD((_point_outer + p_inner + 2 * npts));

            SIMD_TYPE X_PC = SIMD_SUB(SIMD_DUPLICATE(&(xP)), xC);
            SIMD_TYPE Y_PC = SIMD_SUB(SIMD_DUPLICATE(&(yP)), yC);
            SIMD_TYPE Z_PC = SIMD_SUB(SIMD_DUPLICATE(&(zP)), zC);

            SIMD_TYPE tx, ty, t00, t01, t02, t03, t10, t11, t12, t20, t21, t30;

            t00 = SIMD_ALIGNED_LOAD((FmT + p_inner + 0 * NPTS_LOCAL));
            t00 = SIMD_MUL(SIMD_DUPLICATE(&(eval)), t00);
            t01 = SIMD_ALIGNED_LOAD((FmT + p_inner + 1 * NPTS_LOCAL));
            t01 = SIMD_MUL(SIMD_DUPLICATE(&(eval)), t01);
            t02 = SIMD_ALIGNED_LOAD((FmT + p_inner + 2 * NPTS_LOCAL));
            t02 = SIMD_MUL(SIMD_DUPLICATE(&(eval)), t02);
            t03 = SIMD_ALIGNED_LOAD((FmT + p_inner + 3 * NPTS_LOCAL));
            t03 = SIMD_MUL(SIMD_DUPLICATE(&(eval)), t03);
            t10 = SIMD_MUL(SIMD_DUPLICATE(&(X_PA)), t00);
            t10 = SIMD_FNMA(X_PC, t01, t10);
            t11 = SIMD_MUL(SIMD_DUPLICATE(&(X_PA)), t01);
            t11 = SIMD_FNMA(X_PC, t02, t11);
            t12 = SIMD_MUL(SIMD_DUPLICATE(&(X_PA)), t02);
            t12 = SIMD_FNMA(X_PC, t03, t12);
            t20 = SIMD_MUL(SIMD_DUPLICATE(&(X_PA)), t10);
            t20 = SIMD_FNMA(X_PC, t11, t20);
            tx = SIMD_SUB(t00, t01);
            ty = SIMD_SET1(0.5 * 1);
            ty = SIMD_MUL(ty, SIMD_DUPLICATE(&(RHO_INV)));
            t20 = SIMD_FMA(tx, ty, t20);
            t21 = SIMD_MUL(SIMD_DUPLICATE(&(X_PA)), t11);
            t21 = SIMD_FNMA(X_PC, t12, t21);
            tx = SIMD_SUB(t01, t02);
            ty = SIMD_SET1(0.5 * 1);
            ty = SIMD_MUL(ty, SIMD_DUPLICATE(&(RHO_INV)));
            t21 = SIMD_FMA(tx, ty, t21);
            t30 = SIMD_MUL(SIMD_DUPLICATE(&(X_PA)), t20);
            t30 = SIMD_FNMA(X_PC, t21, t30);
            tx = SIMD_SUB(t10, t11);
            ty = SIMD_SET1(0.5 * 2);
            ty = SIMD_MUL(ty, SIMD_DUPLICATE(&(RHO_INV)));
            t30 = SIMD_FMA(tx, ty, t30);
            tx = SIMD_ALIGNED_LOAD((temp + 0 * NPTS_LOCAL + p_inner));
            tx = SIMD_ADD(tx, t30);
            SIMD_ALIGNED_STORE((temp + 0 * NPTS_LOCAL + p_inner), tx);
            t30 = SIMD_MUL(SIMD_DUPLICATE(&(Y_PA)), t20);
            t30 = SIMD_FNMA(Y_PC, t21, t30);
            tx = SIMD_ALIGNED_LOAD((temp + 1 * NPTS_LOCAL + p_inner));
            tx = SIMD_ADD(tx, t30);
            SIMD_ALIGNED_STORE((temp + 1 * NPTS_LOCAL + p_inner), tx);
            t30 = SIMD_MUL(SIMD_DUPLICATE(&(Z_PA)), t20);
            t30 = SIMD_FNMA(Z_PC, t21, t30);
            tx = SIMD_ALIGNED_LOAD((temp + 2 * NPTS_LOCAL + p_inner));
            tx = SIMD_ADD(tx, t30);
            SIMD_ALIGNED_STORE((temp + 2 * NPTS_LOCAL + p_inner), tx);
            t20 = SIMD_MUL(SIMD_DUPLICATE(&(Y_PA)), t10);
            t20 = SIMD_FNMA(Y_PC, t11, t20);
            t21 = SIMD_MUL(SIMD_DUPLICATE(&(Y_PA)), t11);
            t21 = SIMD_FNMA(Y_PC, t12, t21);
            t30 = SIMD_MUL(SIMD_DUPLICATE(&(Y_PA)), t20);
            t30 = SIMD_FNMA(Y_PC, t21, t30);
            tx = SIMD_SUB(t10, t11);
            ty = SIMD_SET1(0.5 * 1);
            ty = SIMD_MUL(ty, SIMD_DUPLICATE(&(RHO_INV)));
            t30 = SIMD_FMA(tx, ty, t30);
            tx = SIMD_ALIGNED_LOAD((temp + 3 * NPTS_LOCAL + p_inner));
            tx = SIMD_ADD(tx, t30);
            SIMD_ALIGNED_STORE((temp + 3 * NPTS_LOCAL + p_inner), tx);
            t30 = SIMD_MUL(SIMD_DUPLICATE(&(Z_PA)), t20);
            t30 = SIMD_FNMA(Z_PC, t21, t30);
            tx = SIMD_ALIGNED_LOAD((temp + 4 * NPTS_LOCAL + p_inner));
            tx = SIMD_ADD(tx, t30);
            SIMD_ALIGNED_STORE((temp + 4 * NPTS_LOCAL + p_inner), tx);
            t20 = SIMD_MUL(SIMD_DUPLICATE(&(Z_PA)), t10);
            t20 = SIMD_FNMA(Z_PC, t11, t20);
            t21 = SIMD_MUL(SIMD_DUPLICATE(&(Z_PA)), t11);
            t21 = SIMD_FNMA(Z_PC, t12, t21);
            t30 = SIMD_MUL(SIMD_DUPLICATE(&(Z_PA)), t20);
            t30 = SIMD_FNMA(Z_PC, t21, t30);
            tx = SIMD_SUB(t10, t11);
            ty = SIMD_SET1(0.5 * 1);
            ty = SIMD_MUL(ty, SIMD_DUPLICATE(&(RHO_INV)));
            t30 = SIMD_FMA(tx, ty, t30);
            tx = SIMD_ALIGNED_LOAD((temp + 5 * NPTS_LOCAL + p_inner));
            tx = SIMD_ADD(tx, t30);
            SIMD_ALIGNED_STORE((temp + 5 * NPTS_LOCAL + p_inner), tx);
            t10 = SIMD_MUL(SIMD_DUPLICATE(&(Y_PA)), t00);
            t10 = SIMD_FNMA(Y_PC, t01, t10);
            t11 = SIMD_MUL(SIMD_DUPLICATE(&(Y_PA)), t01);
            t11 = SIMD_FNMA(Y_PC, t02, t11);
            t12 = SIMD_MUL(SIMD_DUPLICATE(&(Y_PA)), t02);
            t12 = SIMD_FNMA(Y_PC, t03, t12);
            t20 = SIMD_MUL(SIMD_DUPLICATE(&(Y_PA)), t10);
            t20 = SIMD_FNMA(Y_PC, t11, t20);
            tx = SIMD_SUB(t00, t01);
            ty = SIMD_SET1(0.5 * 1);
            ty = SIMD_MUL(ty, SIMD_DUPLICATE(&(RHO_INV)));
            t20 = SIMD_FMA(tx, ty, t20);
            t21 = SIMD_MUL(SIMD_DUPLICATE(&(Y_PA)), t11);
            t21 = SIMD_FNMA(Y_PC, t12, t21);
            tx = SIMD_SUB(t01, t02);
            ty = SIMD_SET1(0.5 * 1);
            ty = SIMD_MUL(ty, SIMD_DUPLICATE(&(RHO_INV)));
            t21 = SIMD_FMA(tx, ty, t21);
            t30 = SIMD_MUL(SIMD_DUPLICATE(&(Y_PA)), t20);
            t30 = SIMD_FNMA(Y_PC, t21, t30);
            tx = SIMD_SUB(t10, t11);
            ty = SIMD_SET1(0.5 * 2);
            ty = SIMD_MUL(ty, SIMD_DUPLICATE(&(RHO_INV)));
            t30 = SIMD_FMA(tx, ty, t30);
            tx = SIMD_ALIGNED_LOAD((temp + 6 * NPTS_LOCAL + p_inner));
            tx = SIMD_ADD(tx, t30);
            SIMD_ALIGNED_STORE((temp + 6 * NPTS_LOCAL + p_inner), tx);
            t30 = SIMD_MUL(SIMD_DUPLICATE(&(Z_PA)), t20);
            t30 = SIMD_FNMA(Z_PC, t21, t30);
            tx = SIMD_ALIGNED_LOAD((temp + 7 * NPTS_LOCAL + p_inner));
            tx = SIMD_ADD(tx, t30);
            SIMD_ALIGNED_STORE((temp + 7 * NPTS_LOCAL + p_inner), tx);
            t20 = SIMD_MUL(SIMD_DUPLICATE(&(Z_PA)), t10);
            t20 = SIMD_FNMA(Z_PC, t11, t20);
            t21 = SIMD_MUL(SIMD_DUPLICATE(&(Z_PA)), t11);
            t21 = SIMD_FNMA(Z_PC, t12, t21);
            t30 = SIMD_MUL(SIMD_DUPLICATE(&(Z_PA)), t20);
            t30 = SIMD_FNMA(Z_PC, t21, t30);
            tx = SIMD_SUB(t10, t11);
            ty = SIMD_SET1(0.5 * 1);
            ty = SIMD_MUL(ty, SIMD_DUPLICATE(&(RHO_INV)));
            t30 = SIMD_FMA(tx, ty, t30);
            tx = SIMD_ALIGNED_LOAD((temp + 8 * NPTS_LOCAL + p_inner));
            tx = SIMD_ADD(tx, t30);
            SIMD_ALIGNED_STORE((temp + 8 * NPTS_LOCAL + p_inner), tx);
            t10 = SIMD_MUL(SIMD_DUPLICATE(&(Z_PA)), t00);
            t10 = SIMD_FNMA(Z_PC, t01, t10);
            t11 = SIMD_MUL(SIMD_DUPLICATE(&(Z_PA)), t01);
            t11 = SIMD_FNMA(Z_PC, t02, t11);
            t12 = SIMD_MUL(SIMD_DUPLICATE(&(Z_PA)), t02);
            t12 = SIMD_FNMA(Z_PC, t03, t12);
            t20 = SIMD_MUL(SIMD_DUPLICATE(&(Z_PA)), t10);
            t20 = SIMD_FNMA(Z_PC, t11, t20);
            tx = SIMD_SUB(t00, t01);
            ty = SIMD_SET1(0.5 * 1);
            ty = SIMD_MUL(ty, SIMD_DUPLICATE(&(RHO_INV)));
            t20 = SIMD_FMA(tx, ty, t20);
            t21 = SIMD_MUL(SIMD_DUPLICATE(&(Z_PA)), t11);
            t21 = SIMD_FNMA(Z_PC, t12, t21);
            tx = SIMD_SUB(t01, t02);
            ty = SIMD_SET1(0.5 * 1);
            ty = SIMD_MUL(ty, SIMD_DUPLICATE(&(RHO_INV)));
            t21 = SIMD_FMA(tx, ty, t21);
            t30 = SIMD_MUL(SIMD_DUPLICATE(&(Z_PA)), t20);
            t30 = SIMD_FNMA(Z_PC, t21, t30);
            tx = SIMD_SUB(t10, t11);
            ty = SIMD_SET1(0.5 * 2);
            ty = SIMD_MUL(ty, SIMD_DUPLICATE(&(RHO_INV)));
            t30 = SIMD_FMA(tx, ty, t30);
            tx = SIMD_ALIGNED_LOAD((temp + 9 * NPTS_LOCAL + p_inner));
            tx = SIMD_ADD(tx, t30);
            SIMD_ALIGNED_STORE((temp + 9 * NPTS_LOCAL + p_inner), tx);
         }

         for(; p_inner < npts_inner; p_inner += SCALAR_LENGTH) {
            SCALAR_TYPE xC = SCALAR_LOAD((_point_outer + p_inner + 0 * npts));
            SCALAR_TYPE yC = SCALAR_LOAD((_point_outer + p_inner + 1 * npts));
            SCALAR_TYPE zC = SCALAR_LOAD((_point_outer + p_inner + 2 * npts));

            SCALAR_TYPE X_PC = SCALAR_SUB(SCALAR_DUPLICATE(&(xP)), xC);
            SCALAR_TYPE Y_PC = SCALAR_SUB(SCALAR_DUPLICATE(&(yP)), yC);
            SCALAR_TYPE Z_PC = SCALAR_SUB(SCALAR_DUPLICATE(&(zP)), zC);

            SCALAR_TYPE tx, ty, t00, t01, t02, t03, t10, t11, t12, t20, t21, t30;

            t00 = SCALAR_LOAD((FmT + p_inner + 0 * NPTS_LOCAL));
            t00 = SCALAR_MUL(SCALAR_DUPLICATE(&(eval)), t00);
            t01 = SCALAR_LOAD((FmT + p_inner + 1 * NPTS_LOCAL));
            t01 = SCALAR_MUL(SCALAR_DUPLICATE(&(eval)), t01);
            t02 = SCALAR_LOAD((FmT + p_inner + 2 * NPTS_LOCAL));
            t02 = SCALAR_MUL(SCALAR_DUPLICATE(&(eval)), t02);
            t03 = SCALAR_LOAD((FmT + p_inner + 3 * NPTS_LOCAL));
            t03 = SCALAR_MUL(SCALAR_DUPLICATE(&(eval)), t03);
            t10 = SCALAR_MUL(SCALAR_DUPLICATE(&(X_PA)), t00);
            t10 = SCALAR_FNMA(X_PC, t01, t10);
            t11 = SCALAR_MUL(SCALAR_DUPLICATE(&(X_PA)), t01);
            t11 = SCALAR_FNMA(X_PC, t02, t11);
            t12 = SCALAR_MUL(SCALAR_DUPLICATE(&(X_PA)), t02);
            t12 = SCALAR_FNMA(X_PC, t03, t12);
            t20 = SCALAR_MUL(SCALAR_DUPLICATE(&(X_PA)), t10);
            t20 = SCALAR_FNMA(X_PC, t11, t20);
            tx = SCALAR_SUB(t00, t01);
            ty = SCALAR_SET1(0.5 * 1);
            ty = SCALAR_MUL(ty, SCALAR_DUPLICATE(&(RHO_INV)));
            t20 = SCALAR_FMA(tx, ty, t20);
            t21 = SCALAR_MUL(SCALAR_DUPLICATE(&(X_PA)), t11);
            t21 = SCALAR_FNMA(X_PC, t12, t21);
            tx = SCALAR_SUB(t01, t02);
            ty = SCALAR_SET1(0.5 * 1);
            ty = SCALAR_MUL(ty, SCALAR_DUPLICATE(&(RHO_INV)));
            t21 = SCALAR_FMA(tx, ty, t21);
            t30 = SCALAR_MUL(SCALAR_DUPLICATE(&(X_PA)), t20);
            t30 = SCALAR_FNMA(X_PC, t21, t30);
            tx = SCALAR_SUB(t10, t11);
            ty = SCALAR_SET1(0.5 * 2);
            ty = SCALAR_MUL(ty, SCALAR_DUPLICATE(&(RHO_INV)));
            t30 = SCALAR_FMA(tx, ty, t30);
            tx = SCALAR_LOAD((temp + 0 * NPTS_LOCAL + p_inner));
            tx = SCALAR_ADD(tx, t30);
            SCALAR_STORE((temp + 0 * NPTS_LOCAL + p_inner), tx);
            t30 = SCALAR_MUL(SCALAR_DUPLICATE(&(Y_PA)), t20);
            t30 = SCALAR_FNMA(Y_PC, t21, t30);
            tx = SCALAR_LOAD((temp + 1 * NPTS_LOCAL + p_inner));
            tx = SCALAR_ADD(tx, t30);
            SCALAR_STORE((temp + 1 * NPTS_LOCAL + p_inner), tx);
            t30 = SCALAR_MUL(SCALAR_DUPLICATE(&(Z_PA)), t20);
            t30 = SCALAR_FNMA(Z_PC, t21, t30);
            tx = SCALAR_LOAD((temp + 2 * NPTS_LOCAL + p_inner));
            tx = SCALAR_ADD(tx, t30);
            SCALAR_STORE((temp + 2 * NPTS_LOCAL + p_inner), tx);
            t20 = SCALAR_MUL(SCALAR_DUPLICATE(&(Y_PA)), t10);
            t20 = SCALAR_FNMA(Y_PC, t11, t20);
            t21 = SCALAR_MUL(SCALAR_DUPLICATE(&(Y_PA)), t11);
            t21 = SCALAR_FNMA(Y_PC, t12, t21);
            t30 = SCALAR_MUL(SCALAR_DUPLICATE(&(Y_PA)), t20);
            t30 = SCALAR_FNMA(Y_PC, t21, t30);
            tx = SCALAR_SUB(t10, t11);
            ty = SCALAR_SET1(0.5 * 1);
            ty = SCALAR_MUL(ty, SCALAR_DUPLICATE(&(RHO_INV)));
            t30 = SCALAR_FMA(tx, ty, t30);
            tx = SCALAR_LOAD((temp + 3 * NPTS_LOCAL + p_inner));
            tx = SCALAR_ADD(tx, t30);
            SCALAR_STORE((temp + 3 * NPTS_LOCAL + p_inner), tx);
            t30 = SCALAR_MUL(SCALAR_DUPLICATE(&(Z_PA)), t20);
            t30 = SCALAR_FNMA(Z_PC, t21, t30);
            tx = SCALAR_LOAD((temp + 4 * NPTS_LOCAL + p_inner));
            tx = SCALAR_ADD(tx, t30);
            SCALAR_STORE((temp + 4 * NPTS_LOCAL + p_inner), tx);
            t20 = SCALAR_MUL(SCALAR_DUPLICATE(&(Z_PA)), t10);
            t20 = SCALAR_FNMA(Z_PC, t11, t20);
            t21 = SCALAR_MUL(SCALAR_DUPLICATE(&(Z_PA)), t11);
            t21 = SCALAR_FNMA(Z_PC, t12, t21);
            t30 = SCALAR_MUL(SCALAR_DUPLICATE(&(Z_PA)), t20);
            t30 = SCALAR_FNMA(Z_PC, t21, t30);
            tx = SCALAR_SUB(t10, t11);
            ty = SCALAR_SET1(0.5 * 1);
            ty = SCALAR_MUL(ty, SCALAR_DUPLICATE(&(RHO_INV)));
            t30 = SCALAR_FMA(tx, ty, t30);
            tx = SCALAR_LOAD((temp + 5 * NPTS_LOCAL + p_inner));
            tx = SCALAR_ADD(tx, t30);
            SCALAR_STORE((temp + 5 * NPTS_LOCAL + p_inner), tx);
            t10 = SCALAR_MUL(SCALAR_DUPLICATE(&(Y_PA)), t00);
            t10 = SCALAR_FNMA(Y_PC, t01, t10);
            t11 = SCALAR_MUL(SCALAR_DUPLICATE(&(Y_PA)), t01);
            t11 = SCALAR_FNMA(Y_PC, t02, t11);
            t12 = SCALAR_MUL(SCALAR_DUPLICATE(&(Y_PA)), t02);
            t12 = SCALAR_FNMA(Y_PC, t03, t12);
            t20 = SCALAR_MUL(SCALAR_DUPLICATE(&(Y_PA)), t10);
            t20 = SCALAR_FNMA(Y_PC, t11, t20);
            tx = SCALAR_SUB(t00, t01);
            ty = SCALAR_SET1(0.5 * 1);
            ty = SCALAR_MUL(ty, SCALAR_DUPLICATE(&(RHO_INV)));
            t20 = SCALAR_FMA(tx, ty, t20);
            t21 = SCALAR_MUL(SCALAR_DUPLICATE(&(Y_PA)), t11);
            t21 = SCALAR_FNMA(Y_PC, t12, t21);
            tx = SCALAR_SUB(t01, t02);
            ty = SCALAR_SET1(0.5 * 1);
            ty = SCALAR_MUL(ty, SCALAR_DUPLICATE(&(RHO_INV)));
            t21 = SCALAR_FMA(tx, ty, t21);
            t30 = SCALAR_MUL(SCALAR_DUPLICATE(&(Y_PA)), t20);
            t30 = SCALAR_FNMA(Y_PC, t21, t30);
            tx = SCALAR_SUB(t10, t11);
            ty = SCALAR_SET1(0.5 * 2);
            ty = SCALAR_MUL(ty, SCALAR_DUPLICATE(&(RHO_INV)));
            t30 = SCALAR_FMA(tx, ty, t30);
            tx = SCALAR_LOAD((temp + 6 * NPTS_LOCAL + p_inner));
            tx = SCALAR_ADD(tx, t30);
            SCALAR_STORE((temp + 6 * NPTS_LOCAL + p_inner), tx);
            t30 = SCALAR_MUL(SCALAR_DUPLICATE(&(Z_PA)), t20);
            t30 = SCALAR_FNMA(Z_PC, t21, t30);
            tx = SCALAR_LOAD((temp + 7 * NPTS_LOCAL + p_inner));
            tx = SCALAR_ADD(tx, t30);
            SCALAR_STORE((temp + 7 * NPTS_LOCAL + p_inner), tx);
            t20 = SCALAR_MUL(SCALAR_DUPLICATE(&(Z_PA)), t10);
            t20 = SCALAR_FNMA(Z_PC, t11, t20);
            t21 = SCALAR_MUL(SCALAR_DUPLICATE(&(Z_PA)), t11);
            t21 = SCALAR_FNMA(Z_PC, t12, t21);
            t30 = SCALAR_MUL(SCALAR_DUPLICATE(&(Z_PA)), t20);
            t30 = SCALAR_FNMA(Z_PC, t21, t30);
            tx = SCALAR_SUB(t10, t11);
            ty = SCALAR_SET1(0.5 * 1);
            ty = SCALAR_MUL(ty, SCALAR_DUPLICATE(&(RHO_INV)));
            t30 = SCALAR_FMA(tx, ty, t30);
            tx = SCALAR_LOAD((temp + 8 * NPTS_LOCAL + p_inner));
            tx = SCALAR_ADD(tx, t30);
            SCALAR_STORE((temp + 8 * NPTS_LOCAL + p_inner), tx);
            t10 = SCALAR_MUL(SCALAR_DUPLICATE(&(Z_PA)), t00);
            t10 = SCALAR_FNMA(Z_PC, t01, t10);
            t11 = SCALAR_MUL(SCALAR_DUPLICATE(&(Z_PA)), t01);
            t11 = SCALAR_FNMA(Z_PC, t02, t11);
            t12 = SCALAR_MUL(SCALAR_DUPLICATE(&(Z_PA)), t02);
            t12 = SCALAR_FNMA(Z_PC, t03, t12);
            t20 = SCALAR_MUL(SCALAR_DUPLICATE(&(Z_PA)), t10);
            t20 = SCALAR_FNMA(Z_PC, t11, t20);
            tx = SCALAR_SUB(t00, t01);
            ty = SCALAR_SET1(0.5 * 1);
            ty = SCALAR_MUL(ty, SCALAR_DUPLICATE(&(RHO_INV)));
            t20 = SCALAR_FMA(tx, ty, t20);
            t21 = SCALAR_MUL(SCALAR_DUPLICATE(&(Z_PA)), t11);
            t21 = SCALAR_FNMA(Z_PC, t12, t21);
            tx = SCALAR_SUB(t01, t02);
            ty = SCALAR_SET1(0.5 * 1);
            ty = SCALAR_MUL(ty, SCALAR_DUPLICATE(&(RHO_INV)));
            t21 = SCALAR_FMA(tx, ty, t21);
            t30 = SCALAR_MUL(SCALAR_DUPLICATE(&(Z_PA)), t20);
            t30 = SCALAR_FNMA(Z_PC, t21, t30);
            tx = SCALAR_SUB(t10, t11);
            ty = SCALAR_SET1(0.5 * 2);
            ty = SCALAR_MUL(ty, SCALAR_DUPLICATE(&(RHO_INV)));
            t30 = SCALAR_FMA(tx, ty, t30);
            tx = SCALAR_LOAD((temp + 9 * NPTS_LOCAL + p_inner));
            tx = SCALAR_ADD(tx, t30);
            SCALAR_STORE((temp + 9 * NPTS_LOCAL + p_inner), tx);
         }
      }

      size_t npts_inner_upper = SIMD_LENGTH * (npts_inner / SIMD_LENGTH);
      size_t p_inner = 0;
      for(p_inner = 0; p_inner < npts_inner_upper; p_inner += SIMD_LENGTH) {
         double *Xik = (Xi + p_outer + p_inner);
         double *Xjk = (Xj + p_outer + p_inner);
         double *Gik = (Gi + p_outer + p_inner);
         double *Gjk = (Gj + p_outer + p_inner);

         SIMD_TYPE const_value_v = SIMD_UNALIGNED_LOAD((weights + p_outer + p_inner));

         double const_value, X_ABp, Y_ABp, Z_ABp, comb_m_i, comb_n_j, comb_p_k;
         SIMD_TYPE const_value_w;
         SIMD_TYPE tx, ty, tz, tw, t0, t1, t2, t3, t4, t5, t6, t7, t8, t9;

         X_ABp = 1.0; comb_m_i = 1.0;
         Y_ABp = 1.0; comb_n_j = 1.0;
         Z_ABp = 1.0; comb_p_k = 1.0;
         const_value = comb_m_i * comb_n_j * comb_p_k * X_ABp * Y_ABp * Z_ABp;
         const_value_w = SIMD_MUL(const_value_v, SIMD_DUPLICATE(&(const_value)));
         tx = SIMD_UNALIGNED_LOAD((Xik + 0 * ldX));
         ty = SIMD_UNALIGNED_LOAD((Xjk + 0 * ldX));
         tz = SIMD_UNALIGNED_LOAD((Gik + 0 * ldG));
         tw = SIMD_UNALIGNED_LOAD((Gjk + 0 * ldG));
         t0 = SIMD_ALIGNED_LOAD((temp + 0 * NPTS_LOCAL + p_inner));
         t0 = SIMD_MUL(t0, const_value_w);
         tz = SIMD_FMA(ty, t0, tz);
         tw = SIMD_FMA(tx, t0, tw);
         SIMD_UNALIGNED_STORE((Gik + 0 * ldG), tz);
         SIMD_UNALIGNED_STORE((Gjk + 0 * ldG), tw);
         tx = SIMD_UNALIGNED_LOAD((Xik + 1 * ldX));
         ty = SIMD_UNALIGNED_LOAD((Xjk + 0 * ldX));
         tz = SIMD_UNALIGNED_LOAD((Gik + 1 * ldG));
         tw = SIMD_UNALIGNED_LOAD((Gjk + 0 * ldG));
         t1 = SIMD_ALIGNED_LOAD((temp + 1 * NPTS_LOCAL + p_inner));
         t1 = SIMD_MUL(t1, const_value_w);
         tz = SIMD_FMA(ty, t1, tz);
         tw = SIMD_FMA(tx, t1, tw);
         SIMD_UNALIGNED_STORE((Gik + 1 * ldG), tz);
         SIMD_UNALIGNED_STORE((Gjk + 0 * ldG), tw);
         tx = SIMD_UNALIGNED_LOAD((Xik + 2 * ldX));
         ty = SIMD_UNALIGNED_LOAD((Xjk + 0 * ldX));
         tz = SIMD_UNALIGNED_LOAD((Gik + 2 * ldG));
         tw = SIMD_UNALIGNED_LOAD((Gjk + 0 * ldG));
         t2 = SIMD_ALIGNED_LOAD((temp + 2 * NPTS_LOCAL + p_inner));
         t2 = SIMD_MUL(t2, const_value_w);
         tz = SIMD_FMA(ty, t2, tz);
         tw = SIMD_FMA(tx, t2, tw);
         SIMD_UNALIGNED_STORE((Gik + 2 * ldG), tz);
         SIMD_UNALIGNED_STORE((Gjk + 0 * ldG), tw);
         tx = SIMD_UNALIGNED_LOAD((Xik + 3 * ldX));
         ty = SIMD_UNALIGNED_LOAD((Xjk + 0 * ldX));
         tz = SIMD_UNALIGNED_LOAD((Gik + 3 * ldG));
         tw = SIMD_UNALIGNED_LOAD((Gjk + 0 * ldG));
         t3 = SIMD_ALIGNED_LOAD((temp + 3 * NPTS_LOCAL + p_inner));
         t3 = SIMD_MUL(t3, const_value_w);
         tz = SIMD_FMA(ty, t3, tz);
         tw = SIMD_FMA(tx, t3, tw);
         SIMD_UNALIGNED_STORE((Gik + 3 * ldG), tz);
         SIMD_UNALIGNED_STORE((Gjk + 0 * ldG), tw);
         tx = SIMD_UNALIGNED_LOAD((Xik + 4 * ldX));
         ty = SIMD_UNALIGNED_LOAD((Xjk + 0 * ldX));
         tz = SIMD_UNALIGNED_LOAD((Gik + 4 * ldG));
         tw = SIMD_UNALIGNED_LOAD((Gjk + 0 * ldG));
         t4 = SIMD_ALIGNED_LOAD((temp + 4 * NPTS_LOCAL + p_inner));
         t4 = SIMD_MUL(t4, const_value_w);
         tz = SIMD_FMA(ty, t4, tz);
         tw = SIMD_FMA(tx, t4, tw);
         SIMD_UNALIGNED_STORE((Gik + 4 * ldG), tz);
         SIMD_UNALIGNED_STORE((Gjk + 0 * ldG), tw);
         tx = SIMD_UNALIGNED_LOAD((Xik + 5 * ldX));
         ty = SIMD_UNALIGNED_LOAD((Xjk + 0 * ldX));
         tz = SIMD_UNALIGNED_LOAD((Gik + 5 * ldG));
         tw = SIMD_UNALIGNED_LOAD((Gjk + 0 * ldG));
         t5 = SIMD_ALIGNED_LOAD((temp + 5 * NPTS_LOCAL + p_inner));
         t5 = SIMD_MUL(t5, const_value_w);
         tz = SIMD_FMA(ty, t5, tz);
         tw = SIMD_FMA(tx, t5, tw);
         SIMD_UNALIGNED_STORE((Gik + 5 * ldG), tz);
         SIMD_UNALIGNED_STORE((Gjk + 0 * ldG), tw);
         tx = SIMD_UNALIGNED_LOAD((Xik + 6 * ldX));
         ty = SIMD_UNALIGNED_LOAD((Xjk + 0 * ldX));
         tz = SIMD_UNALIGNED_LOAD((Gik + 6 * ldG));
         tw = SIMD_UNALIGNED_LOAD((Gjk + 0 * ldG));
         t6 = SIMD_ALIGNED_LOAD((temp + 6 * NPTS_LOCAL + p_inner));
         t6 = SIMD_MUL(t6, const_value_w);
         tz = SIMD_FMA(ty, t6, tz);
         tw = SIMD_FMA(tx, t6, tw);
         SIMD_UNALIGNED_STORE((Gik + 6 * ldG), tz);
         SIMD_UNALIGNED_STORE((Gjk + 0 * ldG), tw);
         tx = SIMD_UNALIGNED_LOAD((Xik + 7 * ldX));
         ty = SIMD_UNALIGNED_LOAD((Xjk + 0 * ldX));
         tz = SIMD_UNALIGNED_LOAD((Gik + 7 * ldG));
         tw = SIMD_UNALIGNED_LOAD((Gjk + 0 * ldG));
         t7 = SIMD_ALIGNED_LOAD((temp + 7 * NPTS_LOCAL + p_inner));
         t7 = SIMD_MUL(t7, const_value_w);
         tz = SIMD_FMA(ty, t7, tz);
         tw = SIMD_FMA(tx, t7, tw);
         SIMD_UNALIGNED_STORE((Gik + 7 * ldG), tz);
         SIMD_UNALIGNED_STORE((Gjk + 0 * ldG), tw);
         tx = SIMD_UNALIGNED_LOAD((Xik + 8 * ldX));
         ty = SIMD_UNALIGNED_LOAD((Xjk + 0 * ldX));
         tz = SIMD_UNALIGNED_LOAD((Gik + 8 * ldG));
         tw = SIMD_UNALIGNED_LOAD((Gjk + 0 * ldG));
         t8 = SIMD_ALIGNED_LOAD((temp + 8 * NPTS_LOCAL + p_inner));
         t8 = SIMD_MUL(t8, const_value_w);
         tz = SIMD_FMA(ty, t8, tz);
         tw = SIMD_FMA(tx, t8, tw);
         SIMD_UNALIGNED_STORE((Gik + 8 * ldG), tz);
         SIMD_UNALIGNED_STORE((Gjk + 0 * ldG), tw);
         tx = SIMD_UNALIGNED_LOAD((Xik + 9 * ldX));
         ty = SIMD_UNALIGNED_LOAD((Xjk + 0 * ldX));
         tz = SIMD_UNALIGNED_LOAD((Gik + 9 * ldG));
         tw = SIMD_UNALIGNED_LOAD((Gjk + 0 * ldG));
         t9 = SIMD_ALIGNED_LOAD((temp + 9 * NPTS_LOCAL + p_inner));
         t9 = SIMD_MUL(t9, const_value_w);
         tz = SIMD_FMA(ty, t9, tz);
         tw = SIMD_FMA(tx, t9, tw);
         SIMD_UNALIGNED_STORE((Gik + 9 * ldG), tz);
         SIMD_UNALIGNED_STORE((Gjk + 0 * ldG), tw);
      }

      for(; p_inner < npts_inner; p_inner += SCALAR_LENGTH) {
         double *Xik = (Xi + p_outer + p_inner);
         double *Xjk = (Xj + p_outer + p_inner);
         double *Gik = (Gi + p_outer + p_inner);
         double *Gjk = (Gj + p_outer + p_inner);

         SCALAR_TYPE const_value_v = SCALAR_LOAD((weights + p_outer + p_inner));

         double const_value, X_ABp, Y_ABp, Z_ABp, comb_m_i, comb_n_j, comb_p_k;
         SCALAR_TYPE const_value_w;
         SCALAR_TYPE tx, ty, tz, tw, t0, t1, t2, t3, t4, t5, t6, t7, t8, t9;

         X_ABp = 1.0; comb_m_i = 1.0;
         Y_ABp = 1.0; comb_n_j = 1.0;
         Z_ABp = 1.0; comb_p_k = 1.0;
         const_value = comb_m_i * comb_n_j * comb_p_k * X_ABp * Y_ABp * Z_ABp;
         const_value_w = SCALAR_MUL(const_value_v, SCALAR_DUPLICATE(&(const_value)));
         tx = SCALAR_LOAD((Xik + 0 * ldX));
         ty = SCALAR_LOAD((Xjk + 0 * ldX));
         tz = SCALAR_LOAD((Gik + 0 * ldG));
         tw = SCALAR_LOAD((Gjk + 0 * ldG));
         t0 = SCALAR_LOAD((temp + 0 * NPTS_LOCAL + p_inner));
         t0 = SCALAR_MUL(t0, const_value_w);
         tz = SCALAR_FMA(ty, t0, tz);
         tw = SCALAR_FMA(tx, t0, tw);
         SCALAR_STORE((Gik + 0 * ldG), tz);
         SCALAR_STORE((Gjk + 0 * ldG), tw);
         tx = SCALAR_LOAD((Xik + 1 * ldX));
         ty = SCALAR_LOAD((Xjk + 0 * ldX));
         tz = SCALAR_LOAD((Gik + 1 * ldG));
         tw = SCALAR_LOAD((Gjk + 0 * ldG));
         t1 = SCALAR_LOAD((temp + 1 * NPTS_LOCAL + p_inner));
         t1 = SCALAR_MUL(t1, const_value_w);
         tz = SCALAR_FMA(ty, t1, tz);
         tw = SCALAR_FMA(tx, t1, tw);
         SCALAR_STORE((Gik + 1 * ldG), tz);
         SCALAR_STORE((Gjk + 0 * ldG), tw);
         tx = SCALAR_LOAD((Xik + 2 * ldX));
         ty = SCALAR_LOAD((Xjk + 0 * ldX));
         tz = SCALAR_LOAD((Gik + 2 * ldG));
         tw = SCALAR_LOAD((Gjk + 0 * ldG));
         t2 = SCALAR_LOAD((temp + 2 * NPTS_LOCAL + p_inner));
         t2 = SCALAR_MUL(t2, const_value_w);
         tz = SCALAR_FMA(ty, t2, tz);
         tw = SCALAR_FMA(tx, t2, tw);
         SCALAR_STORE((Gik + 2 * ldG), tz);
         SCALAR_STORE((Gjk + 0 * ldG), tw);
         tx = SCALAR_LOAD((Xik + 3 * ldX));
         ty = SCALAR_LOAD((Xjk + 0 * ldX));
         tz = SCALAR_LOAD((Gik + 3 * ldG));
         tw = SCALAR_LOAD((Gjk + 0 * ldG));
         t3 = SCALAR_LOAD((temp + 3 * NPTS_LOCAL + p_inner));
         t3 = SCALAR_MUL(t3, const_value_w);
         tz = SCALAR_FMA(ty, t3, tz);
         tw = SCALAR_FMA(tx, t3, tw);
         SCALAR_STORE((Gik + 3 * ldG), tz);
         SCALAR_STORE((Gjk + 0 * ldG), tw);
         tx = SCALAR_LOAD((Xik + 4 * ldX));
         ty = SCALAR_LOAD((Xjk + 0 * ldX));
         tz = SCALAR_LOAD((Gik + 4 * ldG));
         tw = SCALAR_LOAD((Gjk + 0 * ldG));
         t4 = SCALAR_LOAD((temp + 4 * NPTS_LOCAL + p_inner));
         t4 = SCALAR_MUL(t4, const_value_w);
         tz = SCALAR_FMA(ty, t4, tz);
         tw = SCALAR_FMA(tx, t4, tw);
         SCALAR_STORE((Gik + 4 * ldG), tz);
         SCALAR_STORE((Gjk + 0 * ldG), tw);
         tx = SCALAR_LOAD((Xik + 5 * ldX));
         ty = SCALAR_LOAD((Xjk + 0 * ldX));
         tz = SCALAR_LOAD((Gik + 5 * ldG));
         tw = SCALAR_LOAD((Gjk + 0 * ldG));
         t5 = SCALAR_LOAD((temp + 5 * NPTS_LOCAL + p_inner));
         t5 = SCALAR_MUL(t5, const_value_w);
         tz = SCALAR_FMA(ty, t5, tz);
         tw = SCALAR_FMA(tx, t5, tw);
         SCALAR_STORE((Gik + 5 * ldG), tz);
         SCALAR_STORE((Gjk + 0 * ldG), tw);
         tx = SCALAR_LOAD((Xik + 6 * ldX));
         ty = SCALAR_LOAD((Xjk + 0 * ldX));
         tz = SCALAR_LOAD((Gik + 6 * ldG));
         tw = SCALAR_LOAD((Gjk + 0 * ldG));
         t6 = SCALAR_LOAD((temp + 6 * NPTS_LOCAL + p_inner));
         t6 = SCALAR_MUL(t6, const_value_w);
         tz = SCALAR_FMA(ty, t6, tz);
         tw = SCALAR_FMA(tx, t6, tw);
         SCALAR_STORE((Gik + 6 * ldG), tz);
         SCALAR_STORE((Gjk + 0 * ldG), tw);
         tx = SCALAR_LOAD((Xik + 7 * ldX));
         ty = SCALAR_LOAD((Xjk + 0 * ldX));
         tz = SCALAR_LOAD((Gik + 7 * ldG));
         tw = SCALAR_LOAD((Gjk + 0 * ldG));
         t7 = SCALAR_LOAD((temp + 7 * NPTS_LOCAL + p_inner));
         t7 = SCALAR_MUL(t7, const_value_w);
         tz = SCALAR_FMA(ty, t7, tz);
         tw = SCALAR_FMA(tx, t7, tw);
         SCALAR_STORE((Gik + 7 * ldG), tz);
         SCALAR_STORE((Gjk + 0 * ldG), tw);
         tx = SCALAR_LOAD((Xik + 8 * ldX));
         ty = SCALAR_LOAD((Xjk + 0 * ldX));
         tz = SCALAR_LOAD((Gik + 8 * ldG));
         tw = SCALAR_LOAD((Gjk + 0 * ldG));
         t8 = SCALAR_LOAD((temp + 8 * NPTS_LOCAL + p_inner));
         t8 = SCALAR_MUL(t8, const_value_w);
         tz = SCALAR_FMA(ty, t8, tz);
         tw = SCALAR_FMA(tx, t8, tw);
         SCALAR_STORE((Gik + 8 * ldG), tz);
         SCALAR_STORE((Gjk + 0 * ldG), tw);
         tx = SCALAR_LOAD((Xik + 9 * ldX));
         ty = SCALAR_LOAD((Xjk + 0 * ldX));
         tz = SCALAR_LOAD((Gik + 9 * ldG));
         tw = SCALAR_LOAD((Gjk + 0 * ldG));
         t9 = SCALAR_LOAD((temp + 9 * NPTS_LOCAL + p_inner));
         t9 = SCALAR_MUL(t9, const_value_w);
         tz = SCALAR_FMA(ty, t9, tz);
         tw = SCALAR_FMA(tx, t9, tw);
         SCALAR_STORE((Gik + 9 * ldG), tz);
         SCALAR_STORE((Gjk + 0 * ldG), tw);
      }
   }
}
