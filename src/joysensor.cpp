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

JoySensor::JoySensor(
    Type type, int originset, SetJoystick *parent_set, QObject *parent)
    : QObject(parent),
    m_type(type),
    m_originset(originset),
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
    // XXX: implement
    m_current_value[0] = values[0];
    m_current_value[1] = values[1];
    m_current_value[2] = values[2];

    emit moved(values[0], values[1], values[2]);
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
int JoySensor::getDeadZone() { return m_dead_zone; }

/**
 * @brief Get the assigned diagonal range value.
 * @return Assigned diagonal range.
 */
int JoySensor::getDiagonalRange() { return m_diagonal_range; }

int JoySensor::getMaxZone() { return m_max_zone; }

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
 * @brief Get current radial distance of the sensor past the assigned
 *   dead zone.
 * @return Distance percentage in the range of 0.0 - 1.0.
 */
double JoySensor::getDistanceFromDeadZone()
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
double JoySensor::getDistanceFromDeadZone(
    float axisXValue, float axisYValue, float axisZValue)
{
    // XXX: implement
}

/**
 * @brief Get the raw gravity vector length of the sensor.
 * @return Gravity strength in m/s^2.
 */
double JoySensor::getAbsoluteRawGravity()
{
    return getAbsoluteRawGravity(
        m_current_value[0],
        m_current_value[1],
        m_current_value[2]
    );
}

double JoySensor::getAbsoluteRawGravity(
    float axisXValue, float axisYValue, float axisZValue)
{
    // XXX: implement
}

/**
 * @brief Calculate the pitch angle (in degrees) corresponding to the current
 *   position of controller.
 * @return Pitch (in degrees)
 */
double JoySensor::calculatePitch()
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
 * @param X axis value
 * @param Y axis value
 * @param Z axis value
 * @return Pitch (in degrees)
 */
double JoySensor::calculatePitch(
    float axisXValue, float axisYValue, float axisZValue)
{
    // XXX: implement
}

/**
 * @brief Calculate the roll angle (in degrees) corresponding to the current
 *   position of controller.
 * @return Roll (in degrees)
 */
double JoySensor::calculateRoll()
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
 * @param X axis value
 * @param Y axis value
 * @param Z axis value
 * @return Roll (in degrees)
 */
double JoySensor::calculateRoll(
    float axisXValue, float axisYValue, float axisZValue)
{
    // XXX: implement
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
            if ((xml->name() == GlobalVariables::JoySensorButton::xmlName)
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
    // XXX: implement
}

void JoySensor::setDeadZone(int value)
{
    // XXX: implement
}

void JoySensor::setMaxZone(int value)
{
    // XXX: implement
}

void JoySensor::setDiagonalRange(int value)
{
    // XXX: implement
}

void JoySensor::setSensorDelay(unsigned int value)
{
    // XXX: implement
    if (((value >= 10) && (value <= 1000)) || (value == 0))
    {
        m_sensor_delay = value;
        emit sensorDelayChanged(value);
        //emit propertyUpdated();
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
