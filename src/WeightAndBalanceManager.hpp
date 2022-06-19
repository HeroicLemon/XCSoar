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
#if !defined(XCSOAR_WEIGHT_AND_BALANCE_MANAGER_HPP)
#define XCSOAR_WEIGHT_AND_BALANCE_MANAGER_HPP

#include <cstdint>
#include <vector>
#include <memory>
#include "util/tstring.hpp"

/**
 * The type of station. Empty, pilot, and copilot are dry stations
 * but are handled differently for configuration purposes.
 */
enum class StationType : uint8_t {
  EMPTY,
  PILOT,
  COPILOT,
  DRY,
  WET,
};

/**
 * The type of liquid contained in a wet station.
 */
enum class LiquidType : uint8_t {
  WATER,
  FUEL,
};

class WeightAndBalanceStation 
{
  public:  
    WeightAndBalanceStation(tstring name, double arm, StationType station_type);
    virtual bool IsComplete() const = 0;
    virtual double GetMass() const = 0;
    double GetMoment() const { return GetMass() * arm; }
    StationType GetStationType() const { return station_type; }
  protected:  
    tstring name;
    double arm;
    StationType station_type;
};

/**
 * Defines a dry station, where the mass is explicitly set. Use for pilot stations, batteries, equipment, etc.
 */
class DryWeightAndBalanceStation : public WeightAndBalanceStation
{
  public:
    DryWeightAndBalanceStation(tstring name, double arm, double mass, StationType station_type = StationType::DRY);
    virtual bool IsComplete() const;
    virtual double GetMass() const { return mass; }
  private:
    double mass;

};

/**
 * Defines a wet station, where the mass is determined by the current fill level and the type of liquid (fuel or water) that it holds.
 * dump_time greater than zero indicates a wet station that can be drained during ballast dumps.
 * dump_time equal to zero indicates a wet station that cannot be drained during ballast dumps, such as tail trim ballast.
 */
class WetWeightAndBalanceStation : public WeightAndBalanceStation
{
  public: 
    WetWeightAndBalanceStation(tstring name, double arm, double max_capacity, unsigned dump_time, LiquidType liquid_type = LiquidType::WATER);
    virtual bool IsComplete() const;
    virtual double GetMass() const;
    bool IsExpendable() const { return dump_time > 0; }
    double Fill(double ballast_to_add);
  private:
    LiquidType liquid_type;
    double max_capacity;
    double current_capacity;
    unsigned dump_time;
};

struct WeightAndBalanceLimit 
{
  public:
    bool IsComplete() const;
    double weight = std::numeric_limits<double>::quiet_NaN();
    double forward_limit = std::numeric_limits<double>::quiet_NaN();
    double aft_limit = std::numeric_limits<double>::quiet_NaN();    
};

class WeightAndBalanceManager
{
  public:  
    bool TotalWithinEnvelope() const;
    bool NonExpendableWithinEnvelope() const;
    bool WithinCGLimits(double cg, double weight) const;
    bool WithinWeightLimits(double weight) const {
      return weight <= max_limit.weight && weight >= min_limit.weight;
    }
    double GetTotalMass() const;  
    double GetTotalCenterOfGravity() const;  
    double GetNonExpendableMass() const;
    double GetNonExpendableCenterOfGravity() const;
    bool AreLimitsComplete() const;
    bool IsCenterOfGravityComplete() const;
    WeightAndBalanceLimit GetMaxLimit() const { return max_limit; }
    WeightAndBalanceLimit GetMinLimit() const { return min_limit; }
    void SetMaxLimit(const WeightAndBalanceLimit& max_limit) {
      this->max_limit = max_limit;
    }
    void SetMinLimit(const WeightAndBalanceLimit& min_limit) {
      this->min_limit = min_limit;
    }
    void AddStation(const DryWeightAndBalanceStation& station) {
      non_expendable_station_list.push_back(std::make_unique<DryWeightAndBalanceStation>(station));
    }
    void AddStation(const WetWeightAndBalanceStation& station) {
      if(station.IsExpendable())
        expendable_station_list.push_back(std::make_unique<WetWeightAndBalanceStation>(station));
      else
        non_expendable_station_list.push_back(std::make_unique<WetWeightAndBalanceStation>(station));
    }
  private:    
    /**
     *  Track expendable and non-expendable stations seperately. This allows both defining an order that 
     *  expendable ballast is dumped and simplifies CG and mass calculations.
     */
    std::vector<std::unique_ptr<WeightAndBalanceStation>> non_expendable_station_list;
    std::vector<std::unique_ptr<WetWeightAndBalanceStation>> expendable_station_list;
    // TODO: LXNAV allows for defining an intermediate limit.
    WeightAndBalanceLimit max_limit;
    WeightAndBalanceLimit min_limit;
    double GetSlopeInterceptLimit(double weight, double x1, double y1, double x2, double y2) const;
};

#endif
