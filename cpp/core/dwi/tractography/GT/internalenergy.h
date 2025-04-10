/* Copyright (c) 2008-2024 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#pragma once

#include "types.h"

#include "math/math.h"
#include "math/rng.h"

#include "dwi/tractography/GT/energy.h"
#include "dwi/tractography/GT/gt.h"
#include "dwi/tractography/GT/particle.h"
#include "dwi/tractography/GT/particlegrid.h"

namespace MR::DWI::Tractography::GT {

class InternalEnergyComputer : public EnergyComputer {
public:
  InternalEnergyComputer(Stats &s, ParticleGrid &pgrid)
      : EnergyComputer(s), pGrid(pgrid), cpot(1.0), dEint(0.0), neighbourhood(), normalization(1.0), rng_uniform() {
    DEBUG("Initialise computation of internal energy.");
    neighbourhood.reserve(1000);
    ParticleEnd pe;
    pe.par = NULL;
    pe.alpha = 0;
    pe.p_suc = 1.0;
    pe.e_conn = 0.0;
    neighbourhood.push_back(pe);
  }

  double stageShift(const Particle *par, const Point_t &pos, const Point_t &dir) {
    dEint = 0.0;
    if (par->hasPredecessor()) {
      int a = (par->getPredecessor()->getPredecessor() == par) ? -1 : 1;
      dEint -= calcEnergy(par, -1, par->getPredecessor(), a);
      Point_t ep(pos);
      ep -= Particle::L * dir;
      dEint += calcEnergy(pos, ep, par->getPredecessor()->getPosition(), par->getPredecessor()->getEndPoint(a));
    }
    if (par->hasSuccessor()) {
      int a = (par->getSuccessor()->getPredecessor() == par) ? -1 : 1;
      dEint -= calcEnergy(par, 1, par->getSuccessor(), a);
      Point_t ep(pos);
      ep += Particle::L * dir;
      dEint += calcEnergy(pos, ep, par->getSuccessor()->getPosition(), par->getSuccessor()->getEndPoint(a));
    }
    return dEint / stats.getTint();
  }

  double stageRemove(const Particle *par) {
    dEint = 0.0;
    if (par->hasPredecessor()) {
      int a = (par->getPredecessor()->getPredecessor() == par) ? -1 : 1;
      dEint -= calcEnergy(par, -1, par->getPredecessor(), a);
    }
    if (par->hasSuccessor()) {
      int a = (par->getSuccessor()->getPredecessor() == par) ? -1 : 1;
      dEint -= calcEnergy(par, 1, par->getSuccessor(), a);
    }
    return dEint / stats.getTint();
  }

  double stageConnect(const ParticleEnd &pe1, ParticleEnd &pe2);

  void acceptChanges() { stats.incEintTotal(dEint); }

  EnergyComputer *clone() const { return new InternalEnergyComputer(*this); }

  double getConnPot() const { return cpot; }

  void setConnPot(const double connpot) { cpot = connpot; }

protected:
  ParticleGrid &pGrid;
  double cpot, dEint;
  std::vector<ParticleEnd> neighbourhood;
  double normalization;
  Math::RNG::Uniform<double> rng_uniform;

  double calcEnergy(const Particle *P1, const int ep1, const Particle *P2, const int ep2) {
    return calcEnergy(P1->getPosition(), P1->getEndPoint(ep1), P2->getPosition(), P2->getEndPoint(ep2));
  }

  double calcEnergy(const Point_t &pos1, const Point_t &ep1, const Point_t &pos2, const Point_t &ep2) {
    Point_t Xm = (pos1 + pos2) * 0.5; // midpoint between both segments
    double Ucon = ((ep1 - Xm).squaredNorm() + (ep2 - Xm).squaredNorm()) / (Particle::L * Particle::L);
    return Ucon - cpot;
  }

  void scanNeighbourhood(const Particle *p, const int alpha0, const double currTemp);

  ParticleEnd pickNeighbour();
};

} // namespace MR::DWI::Tractography::GT
