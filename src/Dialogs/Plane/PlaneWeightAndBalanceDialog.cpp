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

#include "PlaneDialogs.hpp"
#include "Dialogs/ComboPicker.hpp"
#include "Dialogs/FilePicker.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Form/DataField/ComboList.hpp"
#include "Form/DataField/Listener.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Widget/TextWidget.hpp"
#include "ui/canvas/Color.hpp"
#include "Polar/Polar.hpp"
#include "Polar/PolarStore.hpp"
#include "Polar/PolarFileGlue.hpp"
#include "Plane/Plane.hpp"
#include "system/Path.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"
#include "util/ConvertString.hpp"

class PlaneWeightAndBalanceWidget final
  : public RowFormWidget, DataFieldListener {
  enum Controls {
    EMPTY_MASS,
    MAX_BALLAST,
    DUMP_TIME,
  };

  Plane plane;

public:
  PlaneWeightAndBalanceWidget(const Plane &_plane, const DialogLook &_look)
    :RowFormWidget(_look), plane(_plane) {}

  const Plane &GetValue() const {
    return plane;
  }

  void CreateButtons(WidgetDialog &buttons) {
  }

private:
  void Update();


  /* virtual methods from Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;

  /* methods from DataFieldListener */
  void OnModified(DataField &df) noexcept override;
};

void
PlaneWeightAndBalanceWidget::Update()
{  
  LoadValue(EMPTY_MASS, plane.empty_mass, UnitGroup::MASS);  
  LoadValue(MAX_BALLAST, plane.max_ballast);
}

void
PlaneWeightAndBalanceWidget::Prepare(ContainerWindow &parent,
                          const PixelRect &rc) noexcept
{
  AddFloat(_("Empty Mass"), _("Net mass of the rigged plane."),
           _T("%.0f %s"), _T("%.0f"),
           0, 1000, 5, false,
           UnitGroup::MASS, plane.empty_mass);
  AddFloat(_("Max. Ballast"), nullptr,
           _T("%.0f l"), _T("%.0f"),
           0, 500, 5,
           false, plane.max_ballast);
  AddInteger(_("Dump Time"), nullptr,
             _T("%u s"), _T("%u"),
             10, 300, 5,
             plane.dump_time);
}

void
PlaneWeightAndBalanceWidget::Show(const PixelRect &rc) noexcept
{
  RowFormWidget::Show(rc);
}

bool
PlaneWeightAndBalanceWidget::Save(bool &_changed) noexcept
{
  bool changed = false;

  changed |= SaveValue(EMPTY_MASS, UnitGroup::MASS, plane.empty_mass);
  changed |= SaveValue(MAX_BALLAST, plane.max_ballast);
  changed |= SaveValue(DUMP_TIME, plane.dump_time);

  _changed |= changed;
  return true;
}

void
PlaneWeightAndBalanceWidget::OnModified(DataField &df) noexcept
{
}

bool
dlgPlaneWeightAndBalanceShowModal(Plane &_plane)
{
  StaticString<128> caption;
  caption.Format(_T("%s: %s"), _("Plane Weight and Balance"), _plane.registration.c_str());

  const DialogLook &look = UIGlobals::GetDialogLook();
  TWidgetDialog<PlaneWeightAndBalanceWidget>
    dialog(WidgetDialog::Auto{}, UIGlobals::GetMainWindow(), look, caption);
  dialog.AddButton(_("OK"), mrOK);
  dialog.AddButton(_("Cancel"), mrCancel);
  dialog.SetWidget(_plane, look);
  dialog.GetWidget().CreateButtons(dialog);
  const int result = dialog.ShowModal();

  if (result != mrOK)
    return false;

  _plane = dialog.GetWidget().GetValue();
  return true;
}
