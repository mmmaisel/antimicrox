/* antimicrox Gamepad to KB+M event mapper
 * Copyright (C) 2022 Max Maisel <max.maisel@posteo.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include "uihelpers/joysensoriothreadhelper.h"

#include <QDialog>

class JoySensor;
class QWidget;

namespace Ui {
class JoySensorEditDialog;
}

class JoySensorEditDialog : public QDialog
{
    Q_OBJECT

  public:
    explicit JoySensorEditDialog(JoySensor *sensor, QWidget *parent = nullptr);
    ~JoySensorEditDialog();

  protected:
    void selectCurrentPreset();

  private:
    Ui::JoySensorEditDialog *m_ui;
    bool m_keypad_unlocked;

    JoySensor *m_sensor;
    JoySensorIoThreadHelper m_helper;

  private slots:
    void implementPresets(int index);

    void refreshSensorStats(float x, float y, float z);

    void checkMaxZone(float value);
    void openMouseSettingsDialog();
    void enableMouseSettingButton();
    void updateWindowTitleSensorName();
    void updateSensorDelaySpinBox(int value);
    void updateSensorDelaySlider(double value);
};
