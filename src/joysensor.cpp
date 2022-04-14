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

#include <QDebug>

JoySensor::JoySensor(JoyAxis *axisX, JoyAxis *axisY, JoyAxis *axisZ,
    int type, int originset, SetJoystick *parent_set, QObject *parent)
    : QObject(parent),
    m_type(type),
    m_originset(originset),
    m_parent_set(parent_set),
    m_axisX(axisX),
    m_axisY(axisY),
    m_axisZ(axisZ)
{
    reset();
    populateButtons();
}

JoySensor::~JoySensor()
{
    // XXX: are axes necessary for this?
}

void JoySensor::queuePendingEvent(float* data, bool ignoresets, bool updateLastValues)
{
    // XXX: this is hacky, do not use axis, there is no guarenteed maximum
    m_axisX->queuePendingEvent(static_cast<int>(data[0] * 1000), ignoresets, updateLastValues);
    m_axisY->queuePendingEvent(static_cast<int>(data[1] * 1000), ignoresets, updateLastValues);
    m_axisZ->queuePendingEvent(static_cast<int>(data[2] * 1000), ignoresets, updateLastValues);
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

int JoySensor::getType() { return m_type; }

QHash<JoySensorDirection, JoySensorButton *> *JoySensor::getButtons() { return &m_buttons; }

void JoySensor::setDefaultSensorName(QString tempname) { m_default_sensor_name = tempname; }

QString JoySensor::getDefaultSensorName() { return m_default_sensor_name; }

/**
 * @brief Get pointer to the set that a sensor belongs to.
 * @return Pointer to the set that a sensor belongs to.
 */
SetJoystick *JoySensor::getParentSet()
{
    SetJoystick *temp = nullptr;

    if (m_axisX != nullptr)
        temp = m_axisX->getParentSet();
    else if (m_axisY != nullptr)
        temp = m_axisY->getParentSet();
    else if (m_axisZ != nullptr)
        temp = m_axisZ->getParentSet();

    return temp;
}

JoyAxis *JoySensor::getAxisX() { return m_axisX; }

JoyAxis *JoySensor::getAxisY() { return m_axisY; }

JoyAxis *JoySensor::getAxisZ() { return m_axisZ; }

void JoySensor::reset()
{
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
