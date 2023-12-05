/**
 * GauXC Copyright (c) 2020-2023, The Regents of the University of California,
 * through Lawrence Berkeley National Laboratory (subject to receipt of
 * any required approvals from the U.S. Dept. of Energy). All rights reserved.
 *
 * See LICENSE.txt for details
 */
#include "device/common/zmat_vxc.hpp"
#include <gauxc/util/div_ceil.hpp>
#include "device_specific/cuda_util.hpp"
#include "device_specific/cuda_device_constants.hpp"

namespace GauXC {


__global__ void zmat_lda_vxc_rks_kernel( size_t        ntasks,
                                     XCDeviceTask* tasks_device ) {

  const int batch_idx = blockIdx.z;
  if( batch_idx >= ntasks ) return;

  auto& task = tasks_device[ batch_idx ];
  const auto npts            = task.npts;
  const auto nbf             = task.bfn_screening.nbe;
  const auto* vrho_device    = task.vrho;

  const auto* basis_eval_device = task.bf;

  auto* z_matrix_device = task.zmat;

  const int tid_x = blockIdx.x * blockDim.x + threadIdx.x;
  const int tid_y = blockIdx.y * blockDim.y + threadIdx.y;

  if( tid_x < npts and tid_y < nbf ) {

    const size_t ibfoff = tid_y * npts + tid_x;
    const double fact = 0.5 * vrho_device[tid_x];

    z_matrix_device[ ibfoff ] = fact * basis_eval_device[ ibfoff ];

  }

}




void zmat_lda_vxc_rks( size_t            ntasks,
                   int32_t           max_nbf,
                   int32_t           max_npts,
                   XCDeviceTask*     tasks_device,
                   device_queue queue ) {

  cudaStream_t stream = queue.queue_as<util::cuda_stream>() ;


  dim3 threads(cuda::warp_size,cuda::max_warps_per_thread_block,1);
  dim3 blocks( util::div_ceil( max_npts, threads.x ),
               util::div_ceil( max_nbf,  threads.y ),
               ntasks );

  zmat_lda_vxc_rks_kernel<<< blocks, threads, 0, stream >>>( ntasks, tasks_device );

}




















__global__ void zmat_gga_vxc_rks_kernel( size_t        ntasks,
                                     XCDeviceTask* tasks_device ) {

  const int batch_idx = blockIdx.z;
  if( batch_idx >= ntasks ) return;

  auto& task = tasks_device[ batch_idx ];
  const auto npts            = task.npts;
  const auto nbf             = task.bfn_screening.nbe;
  const auto* vrho_device    = task.vrho;
  const auto* vgamma_device  = task.vgamma;
  const auto* den_x_eval_device = task.ddenx;
  const auto* den_y_eval_device = task.ddeny;
  const auto* den_z_eval_device = task.ddenz;

  const auto* basis_eval_device = task.bf;
  const auto* dbasis_x_eval_device = task.dbfx;
  const auto* dbasis_y_eval_device = task.dbfy;
  const auto* dbasis_z_eval_device = task.dbfz;

  auto* z_matrix_device = task.zmat;

  const int tid_x = blockIdx.x * blockDim.x + threadIdx.x;
  const int tid_y = blockIdx.y * blockDim.y + threadIdx.y;

  if( tid_x < npts and tid_y < nbf ) {

    const size_t ibfoff = tid_y * npts + tid_x;
    const double fact_1 = 0.5 * vrho_device[tid_x]  ;
    const double fact_2 = 2.0 * vgamma_device[tid_x];

    const double dx = den_x_eval_device[ tid_x ] * dbasis_x_eval_device[ ibfoff ];
    const double dy = den_y_eval_device[ tid_x ] * dbasis_y_eval_device[ ibfoff ];
    const double dz = den_z_eval_device[ tid_x ] * dbasis_z_eval_device[ ibfoff ];

    z_matrix_device[ ibfoff ] = 
      fact_1 * basis_eval_device[ ibfoff ] + fact_2 * ( dx + dy + dz ); 

  }
}

void zmat_gga_vxc_rks( size_t            ntasks,
                   int32_t           max_nbf,
                   int32_t           max_npts,
                   XCDeviceTask*     tasks_device,
                   device_queue queue ) {

  cudaStream_t stream = queue.queue_as<util::cuda_stream>() ;


  dim3 threads(cuda::warp_size,cuda::max_warps_per_thread_block,1);
  dim3 blocks( util::div_ceil( max_npts, threads.x ),
               util::div_ceil( max_nbf,  threads.y ),
               ntasks );

  zmat_gga_vxc_rks_kernel<<< blocks, threads, 0, stream >>>( ntasks, tasks_device );

}
              









template<density_id den_selector>
__global__ void zmat_lda_vxc_uks_kernel( size_t        ntasks,
                                     XCDeviceTask* tasks_device ) {

  const int batch_idx = blockIdx.z;
  if( batch_idx >= ntasks ) return;

  auto& task = tasks_device[ batch_idx ];
  const auto npts            = task.npts;
  const auto nbf             = task.bfn_screening.nbe;
  const double* vrho_pos_device    = task.vrho_pos;
  const double* vrho_neg_device    = task.vrho_neg;


  const auto* basis_eval_device = task.bf;


  auto* z_matrix_device = task.zmat;

  const int tid_x = blockIdx.x * blockDim.x + threadIdx.x;
  const int tid_y = blockIdx.y * blockDim.y + threadIdx.y;

  if( tid_x < npts and tid_y < nbf ) {

    const size_t ibfoff = tid_y * npts + tid_x;
    const double factp = 0.5 * vrho_pos_device[tid_x];
    const double factm = 0.5 * vrho_neg_device[tid_x];
    if constexpr ( den_selector == DEN_S ) // positive density
      z_matrix_device[ ibfoff ] = 0.5*(factp * basis_eval_device[ ibfoff ] + factm * basis_eval_device[ ibfoff ]);
    if constexpr ( den_selector == DEN_Z ) // negative density
      z_matrix_device[ ibfoff ] = 0.5*(factp * basis_eval_device[ ibfoff ] - factm * basis_eval_device[ ibfoff ]);
  }

}



void zmat_lda_vxc_uks( size_t            ntasks,
                   int32_t           max_nbf,
                   int32_t           max_npts,
                   XCDeviceTask*     tasks_device,
                   density_id sel,
                   device_queue queue ) {

  cudaStream_t stream = queue.queue_as<util::cuda_stream>() ;


  dim3 threads(cuda::warp_size,cuda::max_warps_per_thread_block,1);
  dim3 blocks( util::div_ceil( max_npts, threads.x ),
               util::div_ceil( max_nbf,  threads.y ),
               ntasks );

  if ( sel == DEN_S )       zmat_lda_vxc_uks_kernel<DEN_S><<< blocks, threads, 0, stream >>>( ntasks, tasks_device );
  else if ( sel == DEN_Z )  zmat_lda_vxc_uks_kernel<DEN_Z><<< blocks, threads, 0, stream >>>( ntasks, tasks_device );

}











template<density_id den_selector>
__global__ void zmat_gga_vxc_uks_kernel( size_t        ntasks,
                                     XCDeviceTask* tasks_device ) {

  const int batch_idx = blockIdx.z;
  if( batch_idx >= ntasks ) return;

  auto& task = tasks_device[ batch_idx ];
  const auto npts            = task.npts;
  const auto nbf             = task.bfn_screening.nbe;

  const double* vrho_pos_device    = task.vrho_pos;
  const double* vrho_neg_device    = task.vrho_neg;

  const auto* den_pos_x_eval_device = task.dden_posx;
  const auto* den_pos_y_eval_device = task.dden_posy;
  const auto* den_pos_z_eval_device = task.dden_posz;
  const auto* den_neg_x_eval_device = task.dden_negx;
  const auto* den_neg_y_eval_device = task.dden_negy;
  const auto* den_neg_z_eval_device = task.dden_negz;


  const auto* basis_eval_device = task.bf;
  const auto* dbasis_x_eval_device = task.dbfx;
  const auto* dbasis_y_eval_device = task.dbfy;
  const auto* dbasis_z_eval_device = task.dbfz;

  auto* z_matrix_device = task.zmat;

  const int tid_x = blockIdx.x * blockDim.x + threadIdx.x;
  const int tid_y = blockIdx.y * blockDim.y + threadIdx.y;

  if( tid_x < npts and tid_y < nbf ) {

    const size_t ibfoff = tid_y * npts + tid_x;

    const double factp = 0.25 * vrho_pos_device[tid_x];
    const double factm = 0.25 * vrho_neg_device[tid_x];
    
    const auto gga_fact_pp  = task.vgamma_pp[ tid_x ];
    const auto gga_fact_pm  = task.vgamma_pm[ tid_x ];
    const auto gga_fact_mm  = task.vgamma_mm[ tid_x ];
    
    const auto gga_fact_1 = 0.5*(gga_fact_pp + gga_fact_pm + gga_fact_mm);
    const auto gga_fact_2 = 0.5*(gga_fact_pp - gga_fact_mm);
    const auto gga_fact_3 = 0.5*(gga_fact_pp - gga_fact_pm + gga_fact_mm);

    if constexpr ( den_selector == DEN_S ) {
      const auto x_fact = gga_fact_1 * den_pos_x_eval_device[ ibfoff ] + gga_fact_2 * den_pos_x_eval_device[ ibfoff ];
      const auto y_fact = gga_fact_1 * den_pos_y_eval_device[ ibfoff ] + gga_fact_2 * den_pos_y_eval_device[ ibfoff ];
      const auto z_fact = gga_fact_1 * den_pos_z_eval_device[ ibfoff ] + gga_fact_2 * den_pos_z_eval_device[ ibfoff ];
      
      z_matrix_device[ ibfoff ] =   x_fact * dbasis_x_eval_device[ ibfoff ]      
                                  + y_fact * dbasis_y_eval_device[ ibfoff ]
                                  + z_fact * dbasis_z_eval_device[ ibfoff ] 
                                  + (factp + factm) * basis_eval_device[ ibfoff ];

    }
    if constexpr ( den_selector == DEN_Z ) {
      const auto x_fact = gga_fact_3 * den_neg_x_eval_device[ ibfoff ] + gga_fact_2 * den_neg_x_eval_device[ ibfoff ];
      const auto y_fact = gga_fact_3 * den_neg_y_eval_device[ ibfoff ] + gga_fact_2 * den_neg_y_eval_device[ ibfoff ];
      const auto z_fact = gga_fact_3 * den_neg_z_eval_device[ ibfoff ] + gga_fact_2 * den_neg_z_eval_device[ ibfoff ];

      z_matrix_device[ ibfoff ] =   x_fact * dbasis_x_eval_device[ ibfoff ] 
                                  + y_fact * dbasis_y_eval_device[ ibfoff ]
                                  + z_fact * dbasis_z_eval_device[ ibfoff ]
                                  + (factp - factm) * basis_eval_device[ ibfoff ];
    }
      
    



    
  }


}



void zmat_gga_vxc_uks( size_t            ntasks,
                   int32_t           max_nbf,
                   int32_t           max_npts,
                   XCDeviceTask*     tasks_device,
                   density_id sel,
                   device_queue queue ) {

  cudaStream_t stream = queue.queue_as<util::cuda_stream>() ;


  dim3 threads(cuda::warp_size,cuda::max_warps_per_thread_block,1);
  dim3 blocks( util::div_ceil( max_npts, threads.x ),
               util::div_ceil( max_nbf,  threads.y ),
               ntasks );

  if ( sel == DEN_S )       zmat_gga_vxc_uks_kernel<DEN_S><<< blocks, threads, 0, stream >>>( ntasks, tasks_device );
  else if ( sel == DEN_Z )  zmat_gga_vxc_uks_kernel<DEN_Z><<< blocks, threads, 0, stream >>>( ntasks, tasks_device );

}







}

