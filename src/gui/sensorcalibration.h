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

#include "joysensoreditdialog.h"

#include <SDL2/SDL_joystick.h>

#include <QDateTime>

class JoyControlStick;
class InputDevice;

namespace Ui {
class SensorCalibration;
}

class SensorCalibration : public QWidget
{
    Q_OBJECT

  public:
    enum CalibrationType
    {
        CAL_NONE,
        CAL_GYROSCOPE,
        CAL_STICK,

        CAL_TYPE_MASK = 0x0000FFFF,
        CAL_INDEX_MASK = 0xFFFF0000,
        CAL_INDEX_POS = 16
    };

    explicit SensorCalibration(InputDevice *joystick, QWidget *parent = 0);
    ~SensorCalibration();

  protected:
    void resetCalibrationValues();
    void showCalibrationValues(bool is_calibrated, double x, double y, double z);
    void selectTypeIndex(unsigned int type_index);

  private:
    Ui::SensorCalibration *m_ui;
    CalibrationType m_type;
    unsigned int m_index;
    JoySensor *m_sensor;
    JoyControlStick *m_stick;
    InputDevice *m_joystick;
    double m_mean[3];
    double m_var[3];
    bool m_calibrated;
    QDateTime m_end_time;
    unsigned int m_sample_count;

  public slots:
    void saveSettings();
    void startGyroscopeCalibration();
    void startGyroscopeCenterCalibration();
    void startStickCalibration();
    void startStickCenterCalibration();
    void startStickGainCalibration();

  protected slots:
    void resetSettings(bool silentReset, bool clicked = false);
    void deviceSelectionChanged(int index);
    void onGyroscopeData(float x, float y, float z);
    void onStickData(int x, int y);

  signals:
    void propertyUpdated();
};
