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

#include <QObject>

#include "joyaxis.h"

class SetJoystick;

class JoySensor : public QObject
{
    Q_OBJECT

  public:
    explicit JoySensor(JoyAxis *axisX, JoyAxis *axisY, JoyAxis *axisZ,
        int type, int originset, SetJoystick *parentSet, QObject *parent);
    ~JoySensor();

    void queuePendingEvent(float* data, bool ignoresets = false, bool updateLastValues = true);

    bool hasSlotsAssigned();

    virtual QString getName(bool forceFullFormat = false, bool displayNames = false);
    virtual QString getPartialName(bool forceFullFormat = false, bool displayNames = false);

    int getType();

    virtual void setDefaultSensorName(QString tempname);
    virtual QString getDefaultSensorName();

    JoyAxis *getAxisX();
    JoyAxis *getAxisY();
    JoyAxis *getAxisZ();

    enum {
        ACCELEROMETER,
        GYROSCOPE
    };

  public slots:
    void reset();

  protected:
    int m_type;

    QString m_sensor_name;
    QString m_default_sensor_name;

    QString sensorTypeName() const;

  private:
    QPointer<JoyAxis> m_axisX;
    QPointer<JoyAxis> m_axisY;
    QPointer<JoyAxis> m_axisZ;
};
