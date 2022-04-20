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

#include "joysensor.h"

#include "setjoystick.h"
#include "joybuttontypes/joysensorbutton.h"
#include "xml/joybuttonxml.h"

#include <QDebug>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <math.h>

JoySensor::JoySensor(
    Type type, int originset, SetJoystick *parent_set, QObject *parent)
    : QObject(parent),
    m_type(type),
    m_originset(originset),
    m_calibrated(false),
    m_parent_set(parent_set)
{
    reset();
    populateButtons();
}

JoySensor::~JoySensor()
{
}

void JoySensor::joyEvent(float* values, bool ignoresets)
{
    bool safezone = false;
    m_current_value[0] = values[0];
    m_current_value[1] = values[1];
    m_current_value[2] = values[2];

    if (m_calibrated)
    {
        m_current_value[0] -= m_calibration_value[0];
        m_current_value[1] -= m_calibration_value[1];
        m_current_value[2] -= m_calibration_value[2];
    }

    double distance = calculateDistance();
    // XXX: gravity threshold
    if (m_type == ACCELEROMETER) {
        double pitch = calculatePitch();
        double roll = calculateRoll();
        safezone = abs(pitch) > m_dead_zone || abs(roll) > m_dead_zone
            || distance > 20;
    }
    else {
        safezone = distance > m_dead_zone;
    }

    if (safezone && !m_active)
    {
        m_active = true;
        emit active(m_current_value[0], m_current_value[1], m_current_value[2]);
        createDeskEvent(safezone, ignoresets);
        // XXX: implement delay
    } else if (!safezone && m_active)
    {
        m_active = false;
        emit released(m_current_value[0], m_current_value[1], m_current_value[2]);
        createDeskEvent(safezone, ignoresets);
        // XXX: implement delay
    } else if (m_active)
    {
        createDeskEvent(safezone, ignoresets);
        // XXX: implement delay
    }

    emit moved(m_current_value[0], m_current_value[1], m_current_value[2]);
}

void JoySensor::queuePendingEvent(float* values, bool ignoresets)
{
    m_pending_event = true;
    m_pending_value[0] = values[0];
    m_pending_value[1] = values[1];
    m_pending_value[2] = values[2];
    m_pending_ignore_sets = ignoresets;
}

void JoySensor::activatePendingEvent()
{
    if (!m_pending_event)
        return;

    joyEvent(m_pending_value, m_pending_ignore_sets);

    clearPendingEvent();
}

bool JoySensor::hasPendingEvent() { return m_pending_event; }

void JoySensor::clearPendingEvent()
{
    m_pending_event = false;
    m_pending_ignore_sets = false;
}

bool JoySensor::hasSlotsAssigned()
{
    bool hasSlots = false;
    // XXX: implement
    return hasSlots;
}

QString JoySensor::getName(bool forceFullFormat, bool displayNames)
{
    QString label = getPartialName(forceFullFormat, displayNames);
    label.append(": ");
    return label;
}

QString JoySensor::getPartialName(bool forceFullFormat, bool displayNames)
{
    QString label = QString();

    if (!m_sensor_name.isEmpty() && displayNames)
    {
        if (forceFullFormat)
        {
            label.append(sensorTypeName()).append(" ");
        }

        label.append(m_sensor_name);
    } else if (!m_default_sensor_name.isEmpty())
    {
        if (forceFullFormat)
        {
            label.append(sensorTypeName()).append(" ");
        }

        label.append(m_default_sensor_name);
    } else
    {
        label.append(sensorTypeName()).append(" ");
        //label.append(QString::number(getRealJoyIndex()));
    }

    return label;
}

JoySensorDirection JoySensor::getCurrentDirection()
{
    return m_current_direction;
}

JoySensor::Type JoySensor::getType() { return m_type; }

/**
 * @brief Get the assigned dead zone value.
 * @return Assigned dead zone value
 */
float JoySensor::getDeadZone() { return m_dead_zone; }

/**
 * @brief Get the assigned diagonal range value.
 * @return Assigned diagonal range.
 */
int JoySensor::getDiagonalRange() { return m_diagonal_range; }

float JoySensor::getMaxZone() { return m_max_zone; }

/**
 * @brief Get the value for the corresponding X axis.
 * @return X axis value in m/s^2 for accelerometer or rad/s for gyroscope.
 */
float JoySensor::getXCoordinate() { return m_current_value[0]; }

/**
 * @brief Get the value for the corresponding Y axis.
 * @return X axis value in m/s^2 for accelerometer or rad/s for gyroscope.
 */
float JoySensor::getYCoordinate() { return m_current_value[1]; }

/**
 * @brief Get the value for the corresponding Z axis.
 * @return Z axis value in m/s^2 for accelerometer or rad/s for gyroscope.
 */
float JoySensor::getZCoordinate() { return m_current_value[2]; }

unsigned int JoySensor::getSensorDelay() { return m_sensor_delay; }

QHash<JoySensorDirection, JoySensorButton *> *JoySensor::getButtons() { return &m_buttons; }

/**
 * @brief Reset all the properties of the sensor direction buttons.
 */
void JoySensor::resetButtons()
{
    QHashIterator<JoySensorDirection, JoySensorButton *> iter(m_buttons);

    while (iter.hasNext())
    {
        JoyButton *button = iter.next().value();

        if (button != nullptr)
            button->reset();
    }
}

bool JoySensor::inDeadZone() const
{
    return calculateDistance() < m_dead_zone;
}

/**
 * @brief Get current radial distance of the sensor past the assigned
 *   dead zone.
 * @return Distance percentage in the range of 0.0 - 1.0.
 */
double JoySensor::getDistanceFromDeadZone() const
{
    return getDistanceFromDeadZone(
        m_current_value[0],
        m_current_value[1],
        m_current_value[2]
    );
}

/**
 * @brief Get radial distance of the sensor past the assigned dead zone
 *   based on the passed X, Y and Z axes values associated with the sensor.
 * @param X axis value
 * @param Y axis value
 * @param Z axis value
 * @return Distance percentage in the range of 0.0 - 1.0.
 */
double JoySensor::getDistanceFromDeadZone(float x, float y, float z) const
{
    double distance = calculateDistance(x, y, z);
    double distance_outside = std::max(0.0, distance - m_dead_zone);

    return distance_outside / m_max_zone;
}

/**
 * @brief Get current X distance of the sensor past the assigned
 *   dead zone.
 * @return Distance percentage in the range of 0.0 - 1.0.
 */
double JoySensor::calculateXDistanceFromDeadZone(bool interpolate) const
{
    return calculateXDistanceFromDeadZone(
        m_current_value[0],
        m_current_value[1],
        m_current_value[2],
        interpolate
    );
}

/**
 * @brief Get current X distance of the sensor past the assigned
 *   dead zone based on the passed X, Y and Z axes values associated
 *   with the sensor. The algorithm checks if axis parallel line
 *   through the current sensor position intersects with the dead zone
 *   sphere and subtracts the line segment within the sphere from the
 *   distance before normalization.
 * @param X axis value
 * @param Y axis value
 * @param Z axis value
 * @return Distance percentage in the range of 0.0 - 1.0.
 */
double JoySensor::calculateXDistanceFromDeadZone(
    float x, float y, float z, bool interpolate) const
{
    double x_abs = abs(x);
    if(x_abs > m_dead_zone || !interpolate)
    {
        return std::max(0.0, x_abs - m_dead_zone) / m_max_zone;
    } else
    {
        double intersection = x-sqrt(m_dead_zone*m_dead_zone-y*y-z*z);
        return std::max(0.0, x-intersection) / m_max_zone;
    }
}

/**
 * @brief Get current Y distance of the sensor past the assigned
 *   dead zone.
 * @return Distance percentage in the range of 0.0 - 1.0.
 */
double JoySensor::calculateYDistanceFromDeadZone(bool interpolate) const
{
    return calculateYDistanceFromDeadZone(
        m_current_value[0],
        m_current_value[1],
        m_current_value[2],
        interpolate
    );
}

/**
 * @brief Get current Y distance of the sensor past the assigned
 *   dead zone based on the passed X, Y and Z axes values associated
 *   with the sensor. The algorithm checks if axis parallel line
 *   through the current sensor position intersects with the dead zone
 *   sphere and subtracts the line segment within the sphere from the
 *   distance before normalization.
 * @param X axis value
 * @param Y axis value
 * @param Z axis value
 * @return Distance percentage in the range of 0.0 - 1.0.
 */
double JoySensor::calculateYDistanceFromDeadZone(
    float x, float y, float z, bool interpolate) const
{
    double y_abs = abs(y);
    if(y_abs > m_dead_zone || !interpolate)
    {
        return std::max(0.0, y_abs - m_dead_zone) / m_max_zone;
    } else
    {
        double intersection = y-sqrt(m_dead_zone*m_dead_zone-x*x-z*z);
        return std::max(0.0, y-intersection) / m_max_zone;
    }
}

/**
 * @brief Get current Z distance of the sensor past the assigned
 *   dead zone.
 * @return Distance percentage in the range of 0.0 - 1.0.
 */
double JoySensor::calculateZDistanceFromDeadZone(bool interpolate) const
{
    return calculateZDistanceFromDeadZone(
        m_current_value[0],
        m_current_value[1],
        m_current_value[2],
        interpolate
    );
}

/**
 * @brief Get current Z distance of the sensor past the assigned
 *   dead zone based on the passed X, Y and Z axes values associated
 *   with the sensor. The algorithm checks if axis parallel line
 *   through the current sensor position intersects with the dead zone
 *   sphere and subtracts the line segment within the sphere from the
 *   distance before normalization.
 * @param X axis value
 * @param Y axis value
 * @param Z axis value
 * @return Distance percentage in the range of 0.0 - 1.0.
 */
double JoySensor::calculateZDistanceFromDeadZone(
    float x, float y, float z, bool interpolate) const
{
    double z_abs = abs(z);
    if(z_abs > m_dead_zone || !interpolate)
    {
        return std::max(0.0, z_abs - m_dead_zone) / m_max_zone;
    } else
    {
        double intersection = z-sqrt(m_dead_zone*m_dead_zone-x*x-y*y);
        return std::max(0.0, z-intersection) / m_max_zone;
    }
}

/**
 * @brief Get the vector length of the sensor.
 * @return Length.
 */
double JoySensor::calculateDistance() const
{
    return calculateDistance(
        m_current_value[0],
        m_current_value[1],
        m_current_value[2]
    );
}

double JoySensor::calculateDistance(float x, float y, float z) const
{
    return sqrt(x*x + y*y + z*z);
}

/**
 * @brief Calculate the pitch angle (in degrees) corresponding to the current
 *   position of controller.
 * @return Pitch (in degrees)
 */
double JoySensor::calculatePitch() const
{
    return calculatePitch(
        m_current_value[0],
        m_current_value[1],
        m_current_value[2]
    );
}

/**
 * @brief Calculate the pitch angle (in degrees) corresponding to the current
 *   passed X, Y and Z axes values associated with the sensor.
 *   position of controller.
 *   See https://www.nxp.com/files-static/sensors/doc/app_note/AN3461.pdf
 *   for a description of the used algorithm.
 * @param X axis value
 * @param Y axis value
 * @param Z axis value
 * @return Pitch (in degrees)
 */
double JoySensor::calculatePitch(float x, float y, float z) const
{
    double rad = calculateDistance(x, y, z);
    double pitch = -atan2(z/rad, y/rad) - M_PI/2;
    if (pitch < -M_PI)
        pitch += 2*M_PI;
    return pitch;
}

/**
 * @brief Calculate the roll angle (in degrees) corresponding to the current
 *   position of controller.
 * @return Roll (in degrees)
 */
double JoySensor::calculateRoll() const
{
    return calculateRoll(
        m_current_value[0],
        m_current_value[1],
        m_current_value[2]
    );
}

/**
 * @brief Calculate the roll angle (in degrees) corresponding to the current
 *   passed X, Y and Z axes values associated with the sensor.
 *   position of controller.
 *   See https://www.nxp.com/files-static/sensors/doc/app_note/AN3461.pdf
 *   for a description of the used algorithm.
 * @param X axis value
 * @param Y axis value
 * @param Z axis value
 * @return Roll (in degrees)
 */
double JoySensor::calculateRoll(float x, float y, float z) const
{
    double rad = calculateDistance(x, y, z);

    double xp, yp, zp;
    xp = x/rad; yp = y/rad; zp = z/rad;
    double roll = atan2(sqrt(yp*yp+zp*zp),-xp) - M_PI/2;
    if (roll < -M_PI)
        roll += 2*M_PI;
    return roll;
}

/**
 * @brief Used to calculate the distance value that should be used by
 *   the JoyButton in the given direction.
 * @param direction
 * @return Distance factor that should be used by JoyButton
 */
double JoySensor::calculateDirectionalDistance(
    JoySensorDirection direction) const
{
    double finalDistance = 0.0;

    switch (direction)
    {
    case JoySensorDirection::GYRO_NICK_P:
    case JoySensorDirection::GYRO_NICK_N:
        finalDistance = calculateXDistanceFromDeadZone(true);
        break;
    case JoySensorDirection::GYRO_ROLL_P:
    case JoySensorDirection::GYRO_ROLL_N:
        finalDistance = calculateYDistanceFromDeadZone(true);
        break;
    case JoySensorDirection::GYRO_YAW_P:
    case JoySensorDirection::GYRO_YAW_N:
        finalDistance = calculateZDistanceFromDeadZone(true);
        break;
    default:
        break;
    }

    return finalDistance;
}

bool JoySensor::isCalibrated() const
{
    return m_calibrated;
}

void JoySensor::resetCalibration()
{
    m_calibrated = false;
}

void JoySensor::getCalibration(float* data)
{
    data[0] = m_calibration_value[0];
    data[1] = m_calibration_value[1];
    data[2] = m_calibration_value[2];
}

void JoySensor::setCalibration(float x0, float y0, float z0)
{
    m_calibration_value[0] = x0;
    m_calibration_value[1] = y0;
    m_calibration_value[2] = z0;
    m_calibrated = true;
}

/**
 * @brief Get a pointer to the sensor direction button for the desired
 *     direction.
 * @param Value of the direction of the sensor.
 * @return Pointer to the sensor direction button for the sensor
 *     direction.
 */
JoySensorButton *JoySensor::getDirectionButton(JoySensorDirection direction)
{
    return m_buttons.value(direction);
}

void JoySensor::setSensorName(QString tempName)
{
    if ((tempName.length() <= 20) && (tempName != m_sensor_name))
    {
        m_sensor_name = tempName;
        emit sensorNameChanged();
    }
}

QString JoySensor::getSensorName() { return m_sensor_name; }

bool JoySensor::isDefault()
{
    bool value = false;
    // XXX: implement
    return value;
}
void JoySensor::setDefaultSensorName(QString tempname) { m_default_sensor_name = tempname; }

QString JoySensor::getDefaultSensorName() { return m_default_sensor_name; }

/**
 * @brief Take a XML stream and set the sensor and direction button properties
 *     according to the values contained within the stream.
 * @param QXmlStreamReader instance that will be used to read property values.
 */
void JoySensor::readConfig(QXmlStreamReader *xml)
{
    if (xml->isStartElement() && (xml->name() == "sensor"))
    {
        xml->readNextStartElement();

        while (!xml->atEnd() && (!xml->isEndElement() && (xml->name() != "sensor")))
        {
            if ((xml->name() == "deadZone") && xml->isStartElement())
            {
                QString temptext = xml->readElementText();
                float tempchoice = temptext.toFloat();
                setDeadZone(tempchoice);
            } else if ((xml->name() == "maxZone") && xml->isStartElement())
            {
                QString temptext = xml->readElementText();
                float tempchoice = temptext.toFloat();
                setMaxZone(tempchoice);
            } else if ((xml->name() == "diagonalRange") && xml->isStartElement())
            {
                QString temptext = xml->readElementText();
                int tempchoice = temptext.toInt();
                setDiagonalRange(tempchoice);
            } else if ((xml->name() == GlobalVariables::JoySensorButton::xmlName)
                && xml->isStartElement())
            {
                int index = xml->attributes().value("index").toString().toInt();
                JoySensorButton *button =
                    m_buttons.value(static_cast<JoySensorDirection>(index));
                QPointer<JoyButtonXml> joyButtonXml = new JoyButtonXml(button);

                if (button != nullptr)
                    joyButtonXml->readConfig(xml);
                else
                    xml->skipCurrentElement();

                if (!joyButtonXml.isNull())
                    delete joyButtonXml;
            } else if ((xml->name() == "sensorDelay") && xml->isStartElement())
            {
                QString temptext = xml->readElementText();
                int tempchoice = temptext.toInt();
                setSensorDelay(tempchoice);
            } else
            {
                xml->skipCurrentElement();
            }

            xml->readNextStartElement();
        }
    }
}

/**
 * @brief Write the status of the properties of a sensor and direction buttons
 *     to an XML stream.
 * @param QXmlStreamWriter instance that will be used to write a profile.
 */
void JoySensor::writeConfig(QXmlStreamWriter *xml)
{
    if (!isDefault())
    {
        xml->writeStartElement("sensor");
        xml->writeAttribute("type", QString::number(m_type));

        if (m_dead_zone != GlobalVariables::JoySensor::DEFAULTDEADZONE)
            xml->writeTextElement("deadZone", QString::number(m_dead_zone));

        if (m_max_zone != (m_type == ACCELEROMETER
                ? GlobalVariables::JoySensor::ACCEL_MAX
                : GlobalVariables::JoySensor::GYRO_MAX))
            xml->writeTextElement("maxZone", QString::number(m_max_zone));

        if (m_diagonal_range != GlobalVariables::JoySensor::DEFAULTDIAGONALRANGE)
            xml->writeTextElement("diagonalRange", QString::number(m_diagonal_range));

        if (m_sensor_delay > GlobalVariables::JoySensor::DEFAULTSENSORDELAY)
            xml->writeTextElement("sensorDelay", QString::number(m_sensor_delay));

        QHashIterator<JoySensorDirection, JoySensorButton *> iter(m_buttons);

        while (iter.hasNext())
        {
            JoySensorButton *button = iter.next().value();
            JoyButtonXml *joyButtonXml = new JoyButtonXml(button);
            joyButtonXml->writeConfig(xml);
            delete joyButtonXml;
            joyButtonXml = nullptr;
        }

        xml->writeEndElement();
    }
}

/**
 * @brief Get pointer to the set that a sensor belongs to.
 * @return Pointer to the set that a sensor belongs to.
 */
SetJoystick *JoySensor::getParentSet()
{
    return m_parent_set;
}

void JoySensor::reset()
{
    m_active = false;
    for (size_t i = 0; i < ACTIVE_BUTTON_COUNT; ++i)
        m_active_button[i] = nullptr;
    m_dead_zone = GlobalVariables::JoySensor::DEFAULTDEADZONE;
    m_max_zone = m_type == ACCELEROMETER
        ? GlobalVariables::JoySensor::ACCEL_MAX
        : GlobalVariables::JoySensor::GYRO_MAX;
    m_diagonal_range = GlobalVariables::JoySensor::DEFAULTDIAGONALRANGE;
    m_pending_event = false;

    m_current_direction = JoySensorDirection::CENTERED;
    m_sensor_name.clear();
    m_sensor_delay = GlobalVariables::JoySensor::DEFAULTSENSORDELAY;

    resetButtons();
}

void JoySensor::setDeadZone(float value)
{
    // XXX: do not compare floats
    if ((value != m_dead_zone) && (value <= m_max_zone))
    {
        m_dead_zone = value;
        emit deadZoneChanged(value);
        emit propertyUpdated();
    }
}

void JoySensor::setMaxZone(float value)
{
    value = abs(value);

    // XXX: implement calibration

    // XXX: do not compare floats
    if ((value != m_max_zone) && (value > m_dead_zone))
    {
        m_max_zone = value;
        emit maxZoneChanged(value);
        emit propertyUpdated();
    }
}

/**
 * @brief Set the diagonal range value for a sensor.
 * @param Value between 1 - 90.
 */
void JoySensor::setDiagonalRange(int value)
{
    if (value < 1)
        value = 1;
    else if (value > 90)
        value = 90;

    if (value != m_diagonal_range)
    {
        m_diagonal_range = value;
        emit diagonalRangeChanged(value);
        emit propertyUpdated();
    }
}

void JoySensor::setSensorDelay(unsigned int value)
{
    if (((value >= 10) && (value <= 1000)) || (value == 0))
    {
        m_sensor_delay = value;
        emit sensorDelayChanged(value);
        emit propertyUpdated();
    }
}

void JoySensor::createDeskEvent(bool safezone, bool ignoresets)
{
    JoySensorButton *eventbutton[ACTIVE_BUTTON_COUNT] = {nullptr};

    if (m_type == ACCELEROMETER)
    {
        // XXX: only calculate them once
        // XXX: debounce shock
        double distance = calculateDistance();
        double pitch = calculatePitch();
        double roll = calculateRoll();
        if (safezone)
        {
            if (pitch > M_PI/4)
                eventbutton[0] = m_buttons.value(JoySensorDirection::ACCEL_UP);
            else if(pitch < -M_PI/4)
                eventbutton[0] = m_buttons.value(JoySensorDirection::ACCEL_DOWN);

            if (roll > M_PI/4)
                eventbutton[1] = m_buttons.value(JoySensorDirection::ACCEL_LEFT);
            else if (roll < -M_PI/4)
                eventbutton[1] = m_buttons.value(JoySensorDirection::ACCEL_RIGHT);

            if (distance > 20)
                eventbutton[2] = m_buttons.value(JoySensorDirection::ACCEL_FWD);
        }
        // XXX: implement diagonal zone
    } else
    {
        if (safezone)
        {
            if (m_current_value[0] > 0)
                eventbutton[0] = m_buttons.value(JoySensorDirection::GYRO_NICK_P);
            else
                eventbutton[0] = m_buttons.value(JoySensorDirection::GYRO_NICK_N);

            if (m_current_value[1] > 0)
                eventbutton[1] = m_buttons.value(JoySensorDirection::GYRO_YAW_N);
            else
                eventbutton[1] = m_buttons.value(JoySensorDirection::GYRO_YAW_P);

            if (m_current_value[2] > 0)
                eventbutton[2] = m_buttons.value(JoySensorDirection::GYRO_ROLL_P);
            else
                eventbutton[2] = m_buttons.value(JoySensorDirection::GYRO_ROLL_N);
        }
    }

    for (size_t i = 0; i < ACTIVE_BUTTON_COUNT; ++i)
    {
        // XXX: gyro mouse needs filtering!!!
        if (m_active_button[i] != nullptr && m_active_button[i] != eventbutton[i])
        {
            m_active_button[i]->joyEvent(false, ignoresets);
            m_active_button[i] = nullptr;
        }

        if (eventbutton[i] != nullptr && m_active_button[i] == nullptr)
        {
            m_active_button[i] = eventbutton[i];
            m_active_button[i]->joyEvent(true, ignoresets);
        } else if (eventbutton[i] == nullptr && m_active_button[i] != nullptr)
        {
            m_active_button[i]->joyEvent(false, ignoresets);
            m_active_button[i] = nullptr;
        }
    }
}

void JoySensor::populateButtons()
{
    JoySensorButton *button = nullptr;
    if (m_type == ACCELEROMETER)
    {
        button = new JoySensorButton(
            this, JoySensorDirection::ACCEL_UP, m_originset, getParentSet(), this);
        m_buttons.insert(JoySensorDirection::ACCEL_UP, button);

        button = new JoySensorButton(
            this, JoySensorDirection::ACCEL_DOWN, m_originset, getParentSet(), this);
        m_buttons.insert(JoySensorDirection::ACCEL_DOWN, button);

        button = new JoySensorButton(
            this, JoySensorDirection::ACCEL_LEFT, m_originset, getParentSet(), this);
        m_buttons.insert(JoySensorDirection::ACCEL_LEFT, button);

        button = new JoySensorButton(
            this, JoySensorDirection::ACCEL_RIGHT, m_originset, getParentSet(), this);
        m_buttons.insert(JoySensorDirection::ACCEL_RIGHT, button);

        button = new JoySensorButton(
            this, JoySensorDirection::ACCEL_FWD, m_originset, getParentSet(), this);
        m_buttons.insert(JoySensorDirection::ACCEL_FWD, button);

        button = new JoySensorButton(
            this, JoySensorDirection::ACCEL_BWD, m_originset, getParentSet(), this);
        m_buttons.insert(JoySensorDirection::ACCEL_BWD, button);
    }
    else
    {
        button = new JoySensorButton(
            this, JoySensorDirection::GYRO_NICK_P, m_originset, getParentSet(), this);
        m_buttons.insert(JoySensorDirection::GYRO_NICK_P, button);

        button = new JoySensorButton(
            this, JoySensorDirection::GYRO_NICK_N, m_originset, getParentSet(), this);
        m_buttons.insert(JoySensorDirection::GYRO_NICK_N, button);

        button = new JoySensorButton(
            this, JoySensorDirection::GYRO_ROLL_P, m_originset, getParentSet(), this);
        m_buttons.insert(JoySensorDirection::GYRO_ROLL_P, button);

        button = new JoySensorButton(
            this, JoySensorDirection::GYRO_ROLL_N, m_originset, getParentSet(), this);
        m_buttons.insert(JoySensorDirection::GYRO_ROLL_N, button);

        button = new JoySensorButton(
            this, JoySensorDirection::GYRO_YAW_P, m_originset, getParentSet(), this);
        m_buttons.insert(JoySensorDirection::GYRO_YAW_P, button);

        button = new JoySensorButton(
            this, JoySensorDirection::GYRO_YAW_N, m_originset, getParentSet(), this);
        m_buttons.insert(JoySensorDirection::GYRO_YAW_N, button);
    }
}

QString JoySensor::sensorTypeName() const {
    if (m_type == ACCELEROMETER)
        return tr("Accelerometer");
    else
        return tr("Gyroscope");
}
