//======================================================================================
/* Athena++ astrophysical MHD code
 * Copyright (C) 2014 James M. Stone  <jmstone@princeton.edu>
 *
 * This program is free software: you can redistribute and/or modify it under the terms
 * of the GNU General Public License (GPL) as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
 * PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of GNU GPL in the file LICENSE included in
 * the code distribution.  If not see <http://www.gnu.org/licenses/>.
 *====================================================================================*/

#include <iostream>
#include <string>
#include <math.h>
#include <float.h>

#include "athena.hpp"
#include "athena_arrays.hpp"
#include "parameter_input.hpp"
#include "mesh.hpp"
#include "fluid.hpp"
#include "convert_var/convert_var.hpp"

//======================================================================================
//! \file fluid.cpp
//  \brief implementation of functions in class Fluid
//======================================================================================

// constructor, initializes data structures and parameters, calls problem generator

Fluid::Fluid(ParameterInput *pin, Block *pb)
{
  pmy_block = pb;

// Read some parameters from input file

  gamma_ = pin->GetReal("fluid","gamma");

// Allocate memory for primitive/conserved variables

  int ncells1 = pmy_block->block_size.nx1 + 2*(NGHOST);
  int ncells2 = pmy_block->block_size.nx2 + 2*(NGHOST);
  int ncells3 = pmy_block->block_size.nx3 + 2*(NGHOST);

  u.NewAthenaArray(NVAR,ncells3,ncells2,ncells1);
  w.NewAthenaArray(NVAR,ncells3,ncells2,ncells1);

// Allocate memory for primitive/conserved variables at half-time step

  u1.NewAthenaArray(NVAR,ncells3,ncells2,ncells1);
  w1.NewAthenaArray(NVAR,ncells3,ncells2,ncells1);

// Allocate memory for scratch arrays

  dt1_.NewAthenaArray(ncells1);
  dt2_.NewAthenaArray(ncells1);
  dt3_.NewAthenaArray(ncells1);
}

// destructor

Fluid::~Fluid()
{
  u.DeleteAthenaArray();
  w.DeleteAthenaArray();
  u1.DeleteAthenaArray();
  w1.DeleteAthenaArray();
}

//--------------------------------------------------------------------------------------
// \!fn 
// \brief

void Fluid::NewTimeStep(Block *pb)
{
  int is = pb->is; int js = pb->js; int ks = pb->ks;
  int ie = pb->ie; int je = pb->je; int ke = pb->ke;
  Real gam = GetGamma();
  Real min_dt;

  AthenaArray<Real> w = pb->pfluid->w.ShallowCopy();
  AthenaArray<Real> dt1 = dt1_.ShallowCopy();
  AthenaArray<Real> dt2 = dt2_.ShallowCopy();
  AthenaArray<Real> dt3 = dt3_.ShallowCopy();

  for (int k=ks; k<=ke; ++k){
  for (int j=js; j<=je; ++j){
    Real& dx2 = pb->dx2f(j);
    Real& dx3 = pb->dx3f(k);
#pragma simd
    for (int i=is; i<=ie; ++i){
      Real& w_d  = w(IDN,k,j,i);
      Real& w_v1 = w(IVX,k,j,i);
      Real& w_v2 = w(IVY,k,j,i);
      Real& w_v3 = w(IVZ,k,j,i);
      Real& w_p  = w(IEN,k,j,i);
      Real& dx1  = pb->dx1f(i);
      Real& d_t1 = dt1(i);
      Real& d_t2 = dt2(i);
      Real& d_t3 = dt3(i);

      Real cs = sqrt(gam*w_p/((gam-1.0)*w_d));

      d_t1 = dx1/(fabs(w_v1) + cs);
      d_t2 = dx2/(fabs(w_v2) + cs);
      d_t3 = dx3/(fabs(w_v3) + cs);
    }

// compute minimum of (v1 +/- C)

    min_dt = dt1(is);
    for (int i=is+1; i<=ie; ++i){
      min_dt = std::min(min_dt,dt1(i));
    }
    
// if grid is 2D/3D, compute minimum of (v2 +/- C)

    if (pb->block_size.nx2 > 1) {
      for (int i=is; i<=ie; ++i){
        min_dt = std::min(min_dt,dt2(i));
      }
    }

// if grid is 3D, compute minimum of (v3 +/- C)

    if (pb->block_size.nx3 > 1) {
      for (int i=is; i<=ie; ++i){
        min_dt = std::min(min_dt,dt3(i));
      }
    }

  }}

  Real old_dt = pb->pmy_domain->pmy_mesh->dt;
  Real cfl = pb->pmy_domain->pmy_mesh->cfl_number;
  pb->pmy_domain->pmy_mesh->dt = std::min((cfl*min_dt), (2.0*old_dt));

  return;

}