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
#include <QTimer>

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
    bool askConfirmation();
    void showGyroCalibrationValues(bool is_calibrated, double x, double y, double z);
    void showStickCalibrationValues(bool is_calibrated,
        double offsetX, double gainX, double offsetY, double gainY);
    void hideCalibrationData();
    void selectTypeIndex(unsigned int type_index);

  private:
    Ui::SensorCalibration *m_ui;
    CalibrationType m_type;
    unsigned int m_index;
    bool m_calibrated;
    JoySensor *m_sensor;
    JoyControlStick *m_stick;
    InputDevice *m_joystick;

    StatisticsProcessor m_stats[4];
    QDateTime m_end_time;
    QTimer m_rate_timer;

  public slots:
    void saveSettings();
    void startGyroscopeCalibration();
    void startGyroscopeCenterCalibration();
    void startStickCenterCalibration();
    void startStickGainCalibration();

  protected slots:
    void resetSettings(bool silentReset, bool clicked = false);
    void deviceSelectionChanged(int index);
    void onGyroscopeData(float x, float y, float z);
    void onStickCenterData(int x, int y);
    void onStickGainData(int x, int y);

  signals:
    void propertyUpdated();
};
