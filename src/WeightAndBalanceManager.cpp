/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include <math.h>
#include "WeightAndBalanceManager.hpp"

/**
 * Checks to see if all of the parameters have been filled for this limit.
 */
bool WeightAndBalanceLimit::IsComplete() const {
  return !std::isnan(weight) && !std::isnan(forward_limit) && !std::isnan(aft_limit);
}

/**
 * Returns true if the total mass is within the min and max limits and 
 * the total CG is within the forward and aft limits.
 */
bool WeightAndBalanceManager::TotalWithinEnvelope() const {
  double cg = GetTotalCenterOfGravity();
  double mass = GetTotalMass();

  return WithinWeightLimits(mass) && WithinCGLimits(cg, mass);
}

/**
 * Returns true if the non-expendable mass is within the min and max limits and 
 * the total CG is within the forward and aft limits.
 */
bool WeightAndBalanceManager::NonExpendableWithinEnvelope() const {
  double cg = GetNonExpendableCenterOfGravity();
  double mass = GetNonExpendableMass();

  return WithinWeightLimits(mass) && WithinCGLimits(cg, mass);
}

bool WeightAndBalanceManager::WithinCGLimits(double cg, double weight) const {
  double forwardLimit = GetSlopeInterceptLimit(weight, min_limit.forward_limit, min_limit.weight, max_limit.forward_limit, max_limit.weight);
  double aftLimit = GetSlopeInterceptLimit(weight, min_limit.aft_limit, min_limit.weight, max_limit.aft_limit, max_limit.weight);

  // TODO: I ought to do some epsilon comparisons for these floating point ops.
  return cg >= forwardLimit && cg <= aftLimit;
}

/**
 * Calculates CG limits. Because the envelope is a convex quadrilateral (most likely a rectangle or trapezoid), 
 * slope-intercept equations can be used to find the CG limits for a particular weight.
 */
double WeightAndBalanceManager::GetSlopeInterceptLimit(double weight, double x1, double y1, double x2, double y2) const {
  double limit;
  if (x1 == x2) {
    limit = x1;
  } else {
    double slope = (y2 - y1) / (x2 - x1);
    double intercept = y1 - slope * x1;  
    limit = (weight - intercept) / slope;
  }

  return limit;
}

/**
 * Gets the mass for all configured stations.
 */
double WeightAndBalanceManager::GetTotalMass() const {
  double expendable_mass = 0;

  for (const auto &station : expendable_station_list) {
    expendable_mass += station->GetMass();
  }

  return expendable_mass + GetNonExpendableMass();
}

/**
 * Gets the center of gravity for all configured stations.
 */
double WeightAndBalanceManager::GetTotalCenterOfGravity() const {
  double total_mass = 0;
  double total_moment = 0;

  for (const auto &station : expendable_station_list) {
    total_mass += station->GetMass();
    total_moment += station->GetMoment();
  }

  for (const auto &station : non_expendable_station_list) {
    total_mass += station->GetMass();
    total_moment += station->GetMoment();
  }

  return total_moment / total_mass;
}

/**
 * Gets the mass of all stations that will not be drained during ballast dumps.
 */
double WeightAndBalanceManager::GetNonExpendableMass() const {
  double nonexpendable_mass = 0;
  for (const auto &station : non_expendable_station_list) {
    nonexpendable_mass += station->GetMass();
  }

  return nonexpendable_mass;
}

/**
 * Gets the center of gravity for all stations that will not be drained during ballast dumps.
 */
double WeightAndBalanceManager::GetNonExpendableCenterOfGravity() const {
  double nonexpendable_mass = 0;
  double nonexpendable_moment = 0;
  for (const auto &station : non_expendable_station_list) {
    nonexpendable_mass += station->GetMass();
    nonexpendable_moment += station->GetMoment();
  }

  return nonexpendable_moment / nonexpendable_mass;
}

/**
 * Returns true if all data required for drawing the limits boxes on the 
 * chart is available.
 */
bool WeightAndBalanceManager::AreLimitsComplete() const {
  return min_limit.IsComplete() && max_limit.IsComplete();
}

/**
 * Returns true if the minimums of empty station and pilot station are complete.
 * TODO: Does this requirement make sense? Should CG be returned if there is at least one complete station?
 * Or should I go into the other end of the spectrum and require that ALL stations are complete in order to get CG?
 */
bool WeightAndBalanceManager::IsCenterOfGravityComplete() const {

  /**
   * First check to see if there is a complete empty station.
   */
  auto const& empty_station = std::find_if(non_expendable_station_list.begin(), non_expendable_station_list.end(), 
                        [](const std::unique_ptr<WeightAndBalanceStation> &station) { return station.get()->GetStationType() == StationType::EMPTY; });
  if (empty_station != non_expendable_station_list.end()) {
    if(empty_station->get()->IsComplete()) {
      /**
       * Next check to see if there is a complete pilot station.
       */
      auto const& pilot_station = std::find_if(non_expendable_station_list.begin(), non_expendable_station_list.end(), 
                        [](const std::unique_ptr<WeightAndBalanceStation> &station) { return station.get()->GetStationType() == StationType::PILOT; }); 
      if(pilot_station != non_expendable_station_list.end()) {
        return pilot_station->get()->IsComplete();
      }
    }
  }

  return false;
}

WeightAndBalanceStation::WeightAndBalanceStation(tstring name, double arm, StationType station_type)
  : name(name), arm(arm), station_type(station_type) {
}

DryWeightAndBalanceStation::DryWeightAndBalanceStation(tstring name, double arm, double mass, StationType station_type)
  : WeightAndBalanceStation(name, arm, station_type), mass(mass) {
}

bool DryWeightAndBalanceStation::IsComplete() const {
  return true;
}

WetWeightAndBalanceStation::WetWeightAndBalanceStation(tstring name, double arm, double max_capacity, unsigned dump_time, LiquidType liquid_type)
  : WeightAndBalanceStation(name, arm, StationType::WET), max_capacity(max_capacity), dump_time(dump_time) {
}

bool WetWeightAndBalanceStation::IsComplete() const {
  return true;
}

double WetWeightAndBalanceStation::GetMass() const {
  double density;
  switch (liquid_type) {
  case LiquidType::FUEL:
    /** 
     * Following LXNAV process for now, which seems to use a density of 0.755 which falls between AVGAS and MOGAS. 
     * Do gliders carry enough fuel to justify explicitly setting a fuel type or is this estimation OK? 
     */
    density = 0.755;
    break;    
  case LiquidType::WATER:
  default:
    density = 1;
    break;
  }

  return current_capacity * density;
}

/**
 * Fills this station with the amount specified.
 * If the amount specified is greater than the max capacity 
 * of the station, the overflow amount will be returned.
 */
double WetWeightAndBalanceStation::Fill(double ballast_to_add) {
  double overflow = 0;

  if (ballast_to_add > max_capacity) {
    overflow = max_capacity - ballast_to_add;
    ballast_to_add = max_capacity;
  }

  current_capacity = ballast_to_add;

  return overflow;
}
