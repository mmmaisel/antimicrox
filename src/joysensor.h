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
#include <QHash>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "joysensordirection.h"

class SetJoystick;
class JoySensorButton;

class JoySensor : public QObject
{
    Q_OBJECT

  public:
    enum Type {
        ACCELEROMETER,
        GYROSCOPE,
        SENSOR_COUNT
    };

    explicit JoySensor(Type type, int originset, SetJoystick *parent_set, QObject *parent);
    ~JoySensor();

    void joyEvent(float* values, bool ignoresets = false);
    void queuePendingEvent(float* values, bool ignoresets = false);
    void activatePendingEvent();
    bool hasPendingEvent();
    void clearPendingEvent();

    bool hasSlotsAssigned();

    virtual QString getName(bool forceFullFormat = false, bool displayNames = false);
    virtual QString getPartialName(bool forceFullFormat = false, bool displayNames = false);

    JoySensorDirection getCurrentDirection();

    Type getType();
    float getDeadZone();
    int getDiagonalRange();
    float getMaxZone();
    float getXCoordinate();
    float getYCoordinate();
    float getZCoordinate();
    unsigned int getSensorDelay();

    void resetButtons();

    bool inDeadZone(float* values) const;
    double getDistanceFromDeadZone() const;
    double getDistanceFromDeadZone(float x, float y, float z) const;
    double calculateXDistanceFromDeadZone() const;
    double calculateXDistanceFromDeadZone(float x, float y, float z) const;
    double calculateYDistanceFromDeadZone() const;
    double calculateYDistanceFromDeadZone(float x, float y, float z) const;
    double calculateZDistanceFromDeadZone() const;
    double calculateZDistanceFromDeadZone(float x, float y, float z) const;
    double calculateDistance() const;
    double calculateDistance(float x, float y, float z) const;
    double calculatePitch() const;
    double calculatePitch(float x, float y, float z) const;
    double calculateRoll() const;
    double calculateRoll(float x, float y, float z) const;
    double calculateDirectionalDistance(JoySensorDirection direction) const;

    bool isCalibrated() const;
    void resetCalibration();
    void getCalibration(float* data);
    void setCalibration(float x0, float y0, float z0);

    QHash<JoySensorDirection, JoySensorButton *> *getButtons();
    JoySensorButton *getDirectionButton(JoySensorDirection direction);

    QString getSensorName();

    bool isDefault();
    virtual void setDefaultSensorName(QString tempname);
    virtual QString getDefaultSensorName();
    void readConfig(QXmlStreamReader *xml);
    void writeConfig(QXmlStreamWriter *xml);

    SetJoystick *getParentSet();

  signals:
    void moved(float xaxis, float yaxis, float zaxis);
    void active(float xaxis, float yaxis, float zaxis);
    void released(float xaxis, float yaxis, float zaxis);
    void deadZoneChanged(float value);
    void diagonalRangeChanged(int value);
    void maxZoneChanged(float value);
    void sensorDelayChanged(int value);
    void sensorNameChanged();
    void joyModeChanged();
    void propertyUpdated();

  public slots:
    void reset();
    void setDeadZone(float value);
    void setMaxZone(float value);
    void setDiagonalRange(int value);
    void setSensorName(QString tempName);
    void setSensorDelay(unsigned int value);

  protected:
    void populateButtons();
    void createDeskEvent(bool safezone, bool ignoresets = false);
    QString sensorTypeName() const;

    Type m_type;
    int m_originset;
    bool m_active;
    static const size_t ACTIVE_BUTTON_COUNT = 3;
    JoySensorButton *m_active_button[ACTIVE_BUTTON_COUNT];

    float m_last_known_raw_value[3];
    float m_current_value[3];
    float m_pending_value[3];
    bool m_calibrated;
    float m_calibration_value[3];
    bool m_pending_event;
    bool m_pending_ignore_sets;
    float m_dead_zone;
    int m_diagonal_range;
    float m_max_zone;
    unsigned int m_sensor_delay;

    QString m_sensor_name;
    QString m_default_sensor_name;

  private:
    JoySensorDirection m_current_direction;
    SetJoystick *m_parent_set;
    QHash<JoySensorDirection, JoySensorButton *> m_buttons;
};
