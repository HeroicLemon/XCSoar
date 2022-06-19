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

#include "UnitsConfigPanel.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/Listener.hpp"
#include "Units/Units.hpp"
#include "Units/UnitsStore.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Widget/RowFormWidget.hpp"
#include "UIGlobals.hpp"

enum ControlIndex {
  UnitsPreset,
  spacer_1,
  UnitsSpeed,
  UnitsDistance,
  UnitsLift,
  UnitsAltitude,
  UnitsTemperature,
  UnitsTaskSpeed,
  UnitsPressure,
  UnitsMass,
  UnitsWingLoading,
  UnitsDistanceFromDatum,
  spacer_2,
  UnitsLatLon
};

class UnitsConfigPanel final
  : public RowFormWidget, DataFieldListener {
public:
  UnitsConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  void UpdateUnitFields(const UnitSetting &units);
  void PresetCheck();

  /* methods from Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;

private:
  /* methods from DataFieldListener */
  void OnModified(DataField &df) noexcept override;
};

void
UnitsConfigPanel::UpdateUnitFields(const UnitSetting &units)
{
  LoadValueEnum(UnitsSpeed, units.speed_unit);
  LoadValueEnum(UnitsDistance, units.distance_unit);
  LoadValueEnum(UnitsLift, units.vertical_speed_unit);
  LoadValueEnum(UnitsAltitude, units.altitude_unit);
  LoadValueEnum(UnitsTemperature, units.temperature_unit);
  LoadValueEnum(UnitsTaskSpeed, units.task_speed_unit);
  LoadValueEnum(UnitsPressure, units.pressure_unit);
  LoadValueEnum(UnitsMass, units.mass_unit);
  LoadValueEnum(UnitsWingLoading, units.wing_loading_unit);
  LoadValueEnum(UnitsDistanceFromDatum, units.distance_from_datum_unit);

  // Ignore the coord.format for the preset selection.
}

void
UnitsConfigPanel::PresetCheck()
{
  UnitSetting current_dlg_set;
  current_dlg_set.speed_unit = (Unit)GetValueEnum(UnitsSpeed);
  current_dlg_set.wind_speed_unit = current_dlg_set.speed_unit;
  current_dlg_set.distance_unit = (Unit)GetValueEnum(UnitsDistance);
  current_dlg_set.vertical_speed_unit = (Unit)GetValueEnum(UnitsLift);
  current_dlg_set.altitude_unit = (Unit)GetValueEnum(UnitsAltitude);
  current_dlg_set.temperature_unit = (Unit)GetValueEnum(UnitsTemperature);
  current_dlg_set.task_speed_unit = (Unit)GetValueEnum(UnitsTaskSpeed);
  current_dlg_set.pressure_unit = (Unit)GetValueEnum(UnitsPressure);
  current_dlg_set.mass_unit = (Unit)GetValueEnum(UnitsMass);
  current_dlg_set.wing_loading_unit = (Unit)GetValueEnum(UnitsWingLoading);
  current_dlg_set.distance_from_datum_unit = (Unit)GetValueInteger(UnitsDistanceFromDatum);

  LoadValueEnum(UnitsPreset, Units::Store::EqualsPresetUnits(current_dlg_set));
}

void
UnitsConfigPanel::OnModified(DataField &df) noexcept
{
  if (IsDataField(UnitsPreset, df)) {
    const DataFieldEnum &dfe = (const DataFieldEnum &)df;
    int result = dfe.GetValue();
    if (result > 0) {
      // First selection means not to load any preset.
      const UnitSetting &units = Units::Store::Read(result - 1);
      UpdateUnitFields(units);
    }
  } else
    PresetCheck();
}

void
UnitsConfigPanel::Prepare(ContainerWindow &parent,
                          const PixelRect &rc) noexcept
{
  const UnitSetting &config = CommonInterface::GetUISettings().format.units;
  const CoordinateFormat coordinate_format =
      CommonInterface::GetUISettings().format.coordinate_format;

  RowFormWidget::Prepare(parent, rc);

  WndProperty *wp = AddEnum(_("Preset"), _("Load a set of units."));
  DataFieldEnum &df = *(DataFieldEnum *)wp->GetDataField();

  df.addEnumText(_("Custom"), (unsigned)0, _("My individual set of units."));
  unsigned len = Units::Store::Count();
  for (unsigned i = 0; i < len; i++)
    df.addEnumText(Units::Store::GetName(i), i+1);

  LoadValueEnum(UnitsPreset, Units::Store::EqualsPresetUnits(config));
  wp->GetDataField()->SetListener(this);

  AddSpacer();
  SetExpertRow(spacer_1);

  static constexpr StaticEnumChoice units_speed_list[] = {
    { Unit::STATUTE_MILES_PER_HOUR, _T("mph") },
    { Unit::KNOTS, N_("knots") },
    { Unit::KILOMETER_PER_HOUR, _T("km/h") },
    { Unit::METER_PER_SECOND, _T("m/s") },
    nullptr
  };
  AddEnum(_("Aircraft/Wind speed"),
          _("Units used for airspeed and ground speed.  "
            "A separate unit is available for task speeds."),
          units_speed_list,
          (unsigned int)config.speed_unit, this);
  SetExpertRow(UnitsSpeed);

  static constexpr StaticEnumChoice units_distance_list[] = {
    { Unit::STATUTE_MILES, _T("sm") },
    { Unit::NAUTICAL_MILES, _T("nm") },
    { Unit::KILOMETER, _T("km") },
    nullptr
  };
  AddEnum(_("Distance"),
          _("Units used for horizontal distances e.g. "
            "range to waypoint, distance to go."),
          units_distance_list,
          (unsigned)config.distance_unit, this);
  SetExpertRow(UnitsDistance);

  static constexpr StaticEnumChoice units_lift_list[] = {
    { Unit::KNOTS, N_("knots") },
    { Unit::METER_PER_SECOND, _T("m/s") },
    { Unit::FEET_PER_MINUTE, _T("ft/min") },
    nullptr
  };
  AddEnum(_("Lift"), _("Units used for vertical speeds (variometer)."),
          units_lift_list,
          (unsigned)config.vertical_speed_unit, this);
  SetExpertRow(UnitsLift);

  static constexpr StaticEnumChoice units_altitude_list[] = {
    { Unit::FEET,  N_("feet") },
    { Unit::METER, N_("meters") },
    nullptr
  };
  AddEnum(_("Altitude"), _("Units used for altitude and heights."),
          units_altitude_list,
          (unsigned)config.altitude_unit, this);
  SetExpertRow(UnitsAltitude);

  static constexpr StaticEnumChoice units_temperature_list[] = {
    { Unit::DEGREES_CELCIUS, _T(DEG "C") },
    { Unit::DEGREES_FAHRENHEIT, _T(DEG "F") },
    nullptr
  };
  AddEnum(_("Temperature"), _("Units used for temperature."),
          units_temperature_list,
          (unsigned)config.temperature_unit, this);
  SetExpertRow(UnitsTemperature);

  static constexpr StaticEnumChoice units_taskspeed_list[] = {
    { Unit::STATUTE_MILES_PER_HOUR, _T("mph") },
    { Unit::KNOTS, N_("knots") },
    { Unit::KILOMETER_PER_HOUR, _T("km/h") },
    { Unit::METER_PER_SECOND, _T("m/s") },
    nullptr
  };
  AddEnum(_("Task speed"), _("Units used for task speeds."),
          units_taskspeed_list,
          (unsigned)config.task_speed_unit, this);
  SetExpertRow(UnitsTaskSpeed);

  static constexpr StaticEnumChoice pressure_labels_list[] = {
    { Unit::HECTOPASCAL, _T("hPa") },
    { Unit::MILLIBAR, _T("mb") },
    { Unit::INCH_MERCURY, _T("inHg") },
    nullptr
  };
  AddEnum(_("Pressure"), _("Units used for pressures."),
          pressure_labels_list,
          (unsigned)config.pressure_unit, this);
  SetExpertRow(UnitsPressure);

  static constexpr StaticEnumChoice mass_labels_list[] = {
    { Unit::KG, _T("kg") },
    { Unit::LB, _T("lb") },
    nullptr
  };
  AddEnum(_("Mass"), _("Units used for mass."),
          mass_labels_list,
          (unsigned)config.mass_unit, this);
  SetExpertRow(UnitsMass);

  static constexpr StaticEnumChoice wing_loading_labels_list[] = {
    { Unit::KG_PER_M2, _T("kg/m²") },
    { Unit::LB_PER_FT2, _T("lb/ft²") },
    nullptr
  };
  AddEnum(_("Wing loading"), _("Units used for wing loading."),
          wing_loading_labels_list,
          (unsigned)config.wing_loading_unit, this);
  SetExpertRow(UnitsWingLoading);

  static constexpr StaticEnumChoice distance_from_datum_labels_list[] = {
      { (unsigned)Unit::MM, _T("mm") },
      { (unsigned)Unit::INCHES, _T("inches") },
      { 0 }
  };
  AddEnum(_("Distance from Datum"), _("Units used for CG distance from datum."),
          distance_from_datum_labels_list,
          (unsigned)config.distance_from_datum_unit, this);
  SetExpertRow(UnitsDistanceFromDatum);

  AddSpacer();
  SetExpertRow(spacer_2);

  static constexpr StaticEnumChoice units_lat_lon_list[] = {
    { CoordinateFormat::DDMMSS, _T("DDMMSS") },
    { CoordinateFormat::DDMMSS_S, _T("DDMMSS.s") },
    { CoordinateFormat::DDMM_MMM, _T("DDMM.mmm") },
    { CoordinateFormat::DD_DDDDD, _T("DD.ddddd") },
    { CoordinateFormat::UTM, _T("UTM") },
    nullptr
  };
  AddEnum(_("Lat./Lon."), _("Units used for latitude and longitude."),
          units_lat_lon_list,
          (unsigned)coordinate_format);
  SetExpertRow(UnitsLatLon);
}

bool
UnitsConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  UnitSetting &config = CommonInterface::SetUISettings().format.units;
  CoordinateFormat &coordinate_format =
      CommonInterface::SetUISettings().format.coordinate_format;

  /* the Units settings affect how other form values are read and translated
   * so changes to Units settings should be processed after all other form settings
   */
  changed |= SaveValueEnum(UnitsSpeed, ProfileKeys::SpeedUnitsValue, config.speed_unit);
  config.wind_speed_unit = config.speed_unit; // Mapping the wind speed to the speed unit

  changed |= SaveValueEnum(UnitsDistance, ProfileKeys::DistanceUnitsValue, config.distance_unit);

  changed |= SaveValueEnum(UnitsLift, ProfileKeys::LiftUnitsValue, config.vertical_speed_unit);

  changed |= SaveValueEnum(UnitsAltitude, ProfileKeys::AltitudeUnitsValue, config.altitude_unit);

  changed |= SaveValueEnum(UnitsTemperature, ProfileKeys::TemperatureUnitsValue, config.temperature_unit);

  changed |= SaveValueEnum(UnitsTaskSpeed, ProfileKeys::TaskSpeedUnitsValue, config.task_speed_unit);

  changed |= SaveValueEnum(UnitsPressure, ProfileKeys::PressureUnitsValue, config.pressure_unit);

  changed |= SaveValueEnum(UnitsMass, ProfileKeys::MassUnitValue,
                           config.mass_unit);

  changed |= SaveValueEnum(UnitsWingLoading, ProfileKeys::WingLoadingUnitValue,
                           config.wing_loading_unit);

  changed |= SaveValueEnum(UnitsDistanceFromDatum, ProfileKeys::DistanceFromDatumUnitsValue, 
                           config.distance_from_datum_unit);

  changed |= SaveValueEnum(UnitsLatLon, ProfileKeys::LatLonUnits, coordinate_format);  

  _changed |= changed;

  return true;
}

std::unique_ptr<Widget>
CreateUnitsConfigPanel()
{
  return std::make_unique<UnitsConfigPanel>();
}

