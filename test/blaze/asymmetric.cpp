/*******************************************************************************
 *
 * This file is part of ezARPACK, an easy-to-use C++ wrapper for
 * the ARPACK-NG FORTRAN library.
 *
 * Copyright (C) 2016-2019 Igor Krivenko <igor.s.krivenko@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 ******************************************************************************/

#include "common.hpp"

//////////////////////////////////////////////
// Eigenproblems with general real matrices //
//////////////////////////////////////////////

TEST_CASE("Asymmetric eigenproblem is solved", "[worker_asymmetric]") {

  using worker_t = arpack_worker<Asymmetric, blaze_storage>;
  using params_t = worker_t::params_t;

  const int N = 100;
  const double diag_coeff_shift = -0.55;
  const double diag_coeff_amp = 1.0;
  const int offdiag_offset = 3;
  const double offdiag_coeff = -1.05;
  const int nev = 10;

  auto spectrum_parts = {
      params_t::LargestMagnitude, params_t::SmallestMagnitude,
      params_t::LargestReal,      params_t::SmallestReal,
      params_t::LargestImag,      params_t::SmallestImag};

  // Asymmetric matrix A
  auto A = make_sparse_matrix<Asymmetric>(N, diag_coeff_shift, diag_coeff_amp,
                                          offdiag_offset, offdiag_coeff);
  // Inner product matrix
  auto M = make_inner_prod_matrix<Asymmetric>(N);

  auto set_init_residual_vector = [](worker_t& ar) {
    for(int i = 0; i < N; ++i)
      ar.residual_vector()[i] = double(i) / N;
  };

  using vector_view_t = worker_t::vector_view_t;
  using vector_const_view_t = worker_t::vector_const_view_t;

  SECTION("Standard eigenproblem") {
    auto Aop = [&](vector_const_view_t from, vector_view_t to) {
      to = A * from;
    };

    worker_t ar(A.rows());

    for(auto e : spectrum_parts) {
      params_t params(nev, e, params_t::Ritz);
      params.random_residual_vector = false;
      set_init_residual_vector(ar);
      ar(Aop, params);
      check_eigenvectors(ar, A);
    }
  }

  SECTION("Generalized eigenproblem: invert mode") {
    decltype(A) op_matrix = inv(M) * A;

    auto op = [&](vector_const_view_t from, vector_view_t to) {
      to = op_matrix * from;
    };
    auto Bop = [&](vector_const_view_t from, vector_view_t to) {
      to = M * from;
    };

    worker_t ar(A.rows());

    for(auto e : spectrum_parts) {
      params_t params(nev, e, params_t::Ritz);
      params.random_residual_vector = false;
      set_init_residual_vector(ar);
      ar(op, Bop, worker_t::Invert, params);
      check_eigenvectors(ar, A, M);
    }
  }

  using cmat_type = DynamicMatrix<dcomplex, columnMajor>;

  SECTION("Generalized eigenproblem: Shift-and-Invert mode (real part)") {
    dcomplex sigma(1.0, -0.1);
    decltype(A) op_matrix =
        real((inv(cmat_type(A) - sigma * cmat_type(M)) * cmat_type(M)));

    auto op = [&](vector_const_view_t from, vector_view_t to) {
      to = op_matrix * from;
    };
    auto Bop = [&](vector_const_view_t from, vector_view_t to) {
      to = M * from;
    };

    worker_t ar(A.rows());

    for(auto e : spectrum_parts) {
      params_t params(nev, e, params_t::Ritz);
      params.random_residual_vector = false;
      params.sigma = sigma;
      set_init_residual_vector(ar);
      ar(op, Bop, worker_t::ShiftAndInvertReal, params);
      check_eigenvectors_shift_and_invert(ar, A, M);
    }
  }

  SECTION("Generalized eigenproblem: Shift-and-Invert mode (imaginary part)") {
    dcomplex sigma(-0.1, 1.0);
    decltype(A) op_matrix =
        imag(inv(cmat_type(A) - sigma * cmat_type(M)) * cmat_type(M));

    auto op = [&](vector_const_view_t from, vector_view_t to) {
      to = op_matrix * from;
    };
    auto Bop = [&](vector_const_view_t from, vector_view_t to) {
      to = M * from;
    };

    worker_t ar(A.rows());

    for(auto e : spectrum_parts) {
      params_t params(nev, e, params_t::Ritz);
      params.random_residual_vector = false;
      params.sigma = sigma;
      set_init_residual_vector(ar);
      ar(op, Bop, worker_t::ShiftAndInvertImag, params);
      check_eigenvectors_shift_and_invert(ar, A, M);
    }
  }
}
