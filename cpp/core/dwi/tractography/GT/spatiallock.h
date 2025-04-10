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

#include <Eigen/Dense>
#include <mutex>

#include "types.h"

namespace MR::DWI::Tractography::GT {

/**
 * @brief SpatialLock manages a mutex lock on n positions in 3D space.
 */
template <typename T = float> class SpatialLock {
public:
  using value_type = T;
  using point_type = Eigen::Matrix<value_type, 3, 1>;

  SpatialLock() : _tx(0), _ty(0), _tz(0) {}
  SpatialLock(const value_type t) : _tx(t), _ty(t), _tz(t) {}
  SpatialLock(const value_type tx, const value_type ty, const value_type tz) : _tx(tx), _ty(ty), _tz(tz) {}

  ~SpatialLock() { lockcentres.clear(); }

  void setThreshold(const value_type t) { _tx = _ty = _tz = t; }

  void setThreshold(const value_type tx, const value_type ty, const value_type tz) {
    _tx = tx;
    _ty = ty;
    _tz = tz;
  }

  struct Guard {
  public:
    Guard(SpatialLock &l) : lock(l), idx(-1) {}

    ~Guard() {
      if (idx >= 0)
        lock.unlock(idx);
    }

    bool try_lock(const point_type &pos) { return lock.try_lock(pos, idx); }

    bool operator!() const { return (idx == -1); }

  private:
    SpatialLock &lock;
    ssize_t idx;
  };

protected:
  std::mutex mutex;
  std::vector<std::pair<point_type, bool>> lockcentres;
  value_type _tx, _ty, _tz;

  bool try_lock(const point_type &pos, ssize_t &idx) {
    std::lock_guard<std::mutex> lock(mutex);
    idx = -1;
    ssize_t i = 0;
    point_type d;
    for (auto &x : lockcentres) {
      if (x.second) {
        d = x.first - pos;
        if ((std::fabs(d[0]) < _tx) && (std::fabs(d[1]) < _ty) && (std::fabs(d[2]) < _tz))
          return false;
      } else {
        idx = i;
      }
      i++;
    }
    if (idx == -1) {
      idx = lockcentres.size();
      lockcentres.emplace_back(pos, true);
    } else {
      lockcentres[idx].first = pos;
      lockcentres[idx].second = true;
    }
    return true;
  }

  void unlock(const size_t idx) { lockcentres[idx].second = false; }
};

} // namespace MR::DWI::Tractography::GT
