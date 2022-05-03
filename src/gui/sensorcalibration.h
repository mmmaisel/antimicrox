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
#include "pt1.h"
#include "statisticsprocessor.h"

#include <QDialog>
#include <QDateTime>
#include <QElapsedTimer>

class JoyControlStick;
class InputDevice;

namespace Ui {
class SensorCalibration;
}

class SensorCalibration : public QDialog
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

    explicit SensorCalibration(InputDevice *joystick, QDialog *parent = 0);
    ~SensorCalibration();

  protected:
    void resetCalibrationValues();
    bool askConfirmation(QString message, bool confirmed);
    void showGyroCalibrationValues(bool xvalid, double x, bool yvalid, double y, bool zvalid, double z);
    void showStickCalibrationValues(bool offsetXvalid, double offsetX, bool gainXvalid, double gainX,
        bool offsetYvalid, double offsetY, bool gainYvalid, double gainY);
    void hideCalibrationData();
    void selectTypeIndex(unsigned int type_index);
    static void stickRegression(double* offset, double* gain, double xoffset, double xmin, double xmax);

  private:
    Ui::SensorCalibration *m_ui;
    CalibrationType m_type;
    unsigned int m_index;
    bool m_calibrated;
    bool m_changed;
    JoySensor *m_sensor;
    JoyControlStick *m_stick;
    InputDevice *m_joystick;

    StatisticsProcessor m_offset[3];
    StatisticsProcessor m_min[2];
    StatisticsProcessor m_max[2];
    PT1 m_stick_filter[2];
    double m_last_slope[2];
    QDateTime m_end_time;
    QElapsedTimer m_rate_timer;
    int m_sample_count;
    int m_phase;

    static const int CAL_MIN_SAMPLES;
    static const double CAL_ACCURACY_SQ;
    static const double STICK_CAL_TAU;
    static const int STICK_RATE_SAMPLES;
    static const int CAL_TIMEOUT;

  public slots:
    void saveSettings();
    void startGyroscopeCalibration();
    void startGyroscopeOffsetCalibration();
    void startStickOffsetCalibration();
    void startStickGainCalibration();

  protected slots:
    void closeEvent(QCloseEvent *event) override;
    void resetSettings();
    void deviceSelectionChanged(int index);
    void onGyroscopeData(float x, float y, float z);
    void onStickOffsetData(int x, int y);
    void onStickGainData(int x, int y);

  signals:
    void propertyUpdated();
};
