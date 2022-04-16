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
#include "joysensordirection.h"

class SetJoystick;
class JoySensorButton;

class JoySensor : public QObject
{
    Q_OBJECT

  public:
    explicit JoySensor(JoyAxis *axisX, JoyAxis *axisY, JoyAxis *axisZ,
        int type, int originset, SetJoystick *parent_set, QObject *parent);
    ~JoySensor();

    void queuePendingEvent(float* data, bool ignoresets = false, bool updateLastValues = true);

    bool hasSlotsAssigned();

    virtual QString getName(bool forceFullFormat = false, bool displayNames = false);
    virtual QString getPartialName(bool forceFullFormat = false, bool displayNames = false);

    JoySensorDirection getCurrentDirection();

    int getType();
    int getDeadZone();
    int getDiagonalRange();
    int getMaxZone();
    int getXCoordinate();
    int getYCoordinate();
    int getZCoordinate();
    unsigned int getSensorDelay();

    double getDistanceFromDeadZone();
    double getDistanceFromDeadZone(int axisXValue, int axisYValue, int axisZValue);
    double getAbsoluteRawGravity();
    double getAbsoluteRawGravity(int axisXValue, int axisYValue, int axisZValue);
    double calculatePitch();
    double calculatePitch(int axisXValue, int axisYValue, int axisZValue);
    double calculateRoll();
    double calculateRoll(int axisXValue, int axisYValue, int axisZValue);

    QHash<JoySensorDirection, JoySensorButton *> *getButtons();
    JoySensorButton *getDirectionButton(JoySensorDirection direction);

    QString getSensorName();

    bool isDefault();
    virtual void setDefaultSensorName(QString tempname);
    virtual QString getDefaultSensorName();
    void readConfig(QXmlStreamReader *xml);
    void writeConfig(QXmlStreamWriter *xml);

    SetJoystick *getParentSet();
    JoyAxis *getAxisX();
    JoyAxis *getAxisY();
    JoyAxis *getAxisZ();

    enum {
        ACCELEROMETER,
        GYROSCOPE
    };

  signals:
    void moved(int xaxis, int yaxis, int zaxis);
    void active(int value);
    void released(int value);
    void deadZoneChanged(int value);
    void diagonalRangeChanged(int value);
    void maxZoneChanged(int value);
    void sensorDelayChanged(int value);
    void sensorNameChanged();
    void joyModeChanged();

  public slots:
    void reset();
    void setDeadZone(int value);
    void setMaxZone(int value);
    void setDiagonalRange(int value);
    void setSensorName(QString tempName);
    void setSensorDelay(unsigned int value);

  protected:
    void populateButtons();
    QString sensorTypeName() const;

    int m_type;
    int m_originset;
    int m_dead_zone;
    int m_diagonal_range;
    int m_max_zone;
    unsigned int m_sensor_delay;

    QString m_sensor_name;
    QString m_default_sensor_name;

  private:
    JoySensorDirection m_current_direction;
    SetJoystick *m_parent_set;
    QPointer<JoyAxis> m_axisX;
    QPointer<JoyAxis> m_axisY;
    QPointer<JoyAxis> m_axisZ;
    QHash<JoySensorDirection, JoySensorButton *> m_buttons;
};
