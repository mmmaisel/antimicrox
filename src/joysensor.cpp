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

#include <QDebug>

JoySensor::JoySensor(JoyAxis *axisX, JoyAxis *axisY, JoyAxis *axisZ,
    int index, int originset, SetJoystick *parentSet, QObject *parent)
    : QObject(parent),
    m_index(index),
    m_axisX(axisX),
    m_axisY(axisY),
    m_axisZ(axisZ)
{

    reset();
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

void JoySensor::setDefaultSensorName(QString tempname) { m_default_sensor_name = tempname; }

QString JoySensor::getDefaultSensorName() { return m_default_sensor_name; }

JoyAxis *JoySensor::getAxisX() { return m_axisX; }

JoyAxis *JoySensor::getAxisY() { return m_axisY; }

JoyAxis *JoySensor::getAxisZ() { return m_axisZ; }

void JoySensor::reset()
{
}
