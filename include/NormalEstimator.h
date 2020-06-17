/**
* This file is part of DefSLAM.
*
* Copyright (C) 2017-2020 Jose Lamarca Peiro <jlamarca at unizar dot es> (University
*of Zaragoza)
* For more information see <https://github.com/unizar/DefSLAM>
*
* DefSLAM is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* DefSLAM is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with DefSLAM. If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef NORMALESTIMATOR_H
#define NORMALESTIMATOR_H

#include "PolySolver.h"
#include "Surface.h"
#include "SurfacePoint.h"
#include "WarpDatabase.h"
#include "ceres/ceres.h"

namespace defSLAM {

class PolySolver;
class SchwarpDatabase;
class Surface;
class SurfacePoint;
using ORB_SLAM2::KeyFrame;

class NormalEstimator {
public:
  NormalEstimator() = delete;

  NormalEstimator(WarpDatabase *WarpDatabase);

  ~NormalEstimator();

  void ObtainK1K2();

public:
  WarpDatabase *WarpDB;
};

} // namespace ORB_SLAM2

#endif // DEFORMATION MAPPOINT_H
