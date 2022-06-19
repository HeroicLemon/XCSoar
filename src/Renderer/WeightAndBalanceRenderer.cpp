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


#include "WeightAndBalanceRenderer.hpp"
#include "WeightAndBalanceManager.hpp"
#include "Look/ChartLook.hpp"
#include "ChartRenderer.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Units/Units.hpp"
#include "Language/Language.hpp"
#include "Formatter/UserUnits.hpp"
#include "util/StaticString.hxx"

#include <stdio.h>

void
RenderWeightAndBalance(Canvas &canvas, const PixelRect rc,
                 const ChartLook &chart_look,
                 const WeightAndBalanceManager &weight_and_balance_manager_const)
{
  // TODO: Hardcoding this for now.
  WeightAndBalanceLimit minimum_limit;
  minimum_limit.forward_limit = 200;
  minimum_limit.aft_limit = 300;
  minimum_limit.weight = 100;

  WeightAndBalanceLimit maximum_limit;
  maximum_limit.forward_limit = 220;
  maximum_limit.aft_limit = 300;
  maximum_limit.weight = 200;

  DryWeightAndBalanceStation emptyStation("EMPTY", 250, 100, StationType::EMPTY);
  DryWeightAndBalanceStation pilotStation("PILOT", 250, 50, StationType::PILOT);
  WetWeightAndBalanceStation ballastStation("MAIN BALLAST", 1000, 20, 120);
  ballastStation.Fill(20);  
  WeightAndBalanceManager weight_and_balance_manager;
  weight_and_balance_manager.SetMaxLimit(maximum_limit);
  weight_and_balance_manager.SetMinLimit(minimum_limit);
  weight_and_balance_manager.AddStation(emptyStation);
  weight_and_balance_manager.AddStation(pilotStation);
  weight_and_balance_manager.AddStation(ballastStation);

  ChartRenderer chart(chart_look, canvas, rc);  
  chart.SetXLabel(_T("CG"), Units::GetDistanceFromDatumName());
  chart.SetYLabel(_T("M"), Units::GetMassName());
  chart.Begin();

  if (!weight_and_balance_manager.AreLimitsComplete()) {
    chart.DrawNoData();
    return;
  }

  const auto min_limit = weight_and_balance_manager.GetMinLimit();
  const auto max_limit = weight_and_balance_manager.GetMaxLimit();
  const auto forwardmost_limit = std::min(min_limit.forward_limit, 
                                      max_limit.forward_limit);
  const auto aftmost_limit = std::max(min_limit.aft_limit, 
                                      max_limit.aft_limit);

  double padding = 5;
  chart.ScaleYFromValue(min_limit.weight - padding);
  chart.ScaleYFromValue(max_limit.weight + padding);
  chart.ScaleXFromValue(forwardmost_limit - padding);
  chart.ScaleXFromValue(aftmost_limit + padding);

  chart.DrawXGrid(Units::ToSysDistanceFromDatum(5), 5, ChartRenderer::UnitFormat::NUMERIC);
  chart.DrawYGrid(Units::ToSysMass(5), 5, ChartRenderer::UnitFormat::NUMERIC);  

  /**
   * Draw the bounds of the valid CG ranges.
   */ 
  ChartLook::Style limit_style = ChartLook::STYLE_BLACK;
  chart.DrawLine(min_limit.forward_limit, min_limit.weight, max_limit.forward_limit, max_limit.weight, limit_style);
  chart.DrawLine(max_limit.forward_limit, max_limit.weight, max_limit.aft_limit, max_limit.weight, limit_style);
  chart.DrawLine(max_limit.aft_limit, max_limit.weight, min_limit.aft_limit, min_limit.weight, limit_style);
  chart.DrawLine(min_limit.aft_limit, min_limit.weight, min_limit.forward_limit, min_limit.weight, limit_style);

  if (weight_and_balance_manager.IsCenterOfGravityComplete()) {
    double cg = weight_and_balance_manager.GetTotalCenterOfGravity();
    double mass = weight_and_balance_manager.GetTotalMass();
    chart.ScaleYFromValue(mass + padding);
    chart.ScaleXFromValue(cg + padding);

    double nonexpendable_cg = weight_and_balance_manager.GetNonExpendableCenterOfGravity();
    double nonexpendable_mass = weight_and_balance_manager.GetNonExpendableMass();    

    Brush dotColor = Brush(COLOR_BLACK);
    /**
     * If the non-expendable mass is different from the total mass (indicating that dumpable ballast is aboard)
     * draw the non-expendable data with a black dot and the total data with a blue dot so that the user
     * can see where their CG will be after dumping all ballast.
     */
    if (nonexpendable_mass != mass) {
      chart.ScaleYFromValue(nonexpendable_mass + padding);
      chart.ScaleXFromValue(nonexpendable_cg + padding);      
      chart.DrawReferenceLines(nonexpendable_cg, nonexpendable_mass, ChartLook::STYLE_BLUETHINDASH);

      Brush indicatorColor = Brush(COLOR_RED);
      if (weight_and_balance_manager.NonExpendableWithinEnvelope()) {
        indicatorColor = Brush(COLOR_GREEN);
      }
      chart.GetCanvas().Select(indicatorColor);
      chart.DrawDot(nonexpendable_cg, nonexpendable_mass, Layout::Scale(4));
      chart.GetCanvas().SelectBlackBrush();
      chart.DrawDot(nonexpendable_cg, nonexpendable_mass, Layout::Scale(2));

      dotColor = Brush(COLOR_CYAN);
    }

    chart.DrawReferenceLines(cg, mass, ChartLook::STYLE_BLUETHINDASH);
    Brush indicatorColor = Brush(COLOR_RED);
    if (weight_and_balance_manager.TotalWithinEnvelope()) {
      indicatorColor = Brush(COLOR_GREEN);
    }
    chart.GetCanvas().Select(indicatorColor);
    chart.DrawDot(cg, mass, Layout::Scale(4));    
    chart.GetCanvas().Select(dotColor);
    chart.DrawDot(cg, mass, Layout::Scale(2));
  }

  chart.Finish();
}
