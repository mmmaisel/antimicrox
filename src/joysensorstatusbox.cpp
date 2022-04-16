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

#include "joysensorstatusbox.h"

#include "common.h"
#include "globalvariables.h"
#include "joyaxis.h"
#include "joysensor.h"

#include <qdrawutil.h>

#include <QDebug>
#include <QLinearGradient>
#include <QList>
#include <QPaintEvent>
#include <QPainter>
#include <QSizePolicy>

JoySensorStatusBox::JoySensorStatusBox(QWidget *parent)
    : QWidget(parent),
    m_sensor(nullptr)
{
}

JoySensorStatusBox::JoySensorStatusBox(JoySensor *sensor, QWidget *parent)
    : QWidget(parent),
    m_sensor(sensor)
{

    connect(m_sensor, SIGNAL(deadZoneChanged(int)), this, SLOT(update()));
    connect(m_sensor, SIGNAL(moved(float, float, float)), this, SLOT(update()));
    connect(m_sensor, SIGNAL(diagonalRangeChanged(int)), this, SLOT(update()));
    connect(m_sensor, SIGNAL(maxZoneChanged(int)), this, SLOT(update()));
    connect(m_sensor, SIGNAL(joyModeChanged()), this, SLOT(update()));
    connect(m_sensor, SIGNAL(circleAdjustChange(double)), this, SLOT(update()));
}

void JoySensorStatusBox::setSensor(JoySensor *sensor)
{
    if (m_sensor != nullptr)
    {
        disconnect(m_sensor, SIGNAL(deadZoneChanged(int)), this, nullptr);
        disconnect(m_sensor, SIGNAL(moved(float, float, float)), this, nullptr);
        disconnect(m_sensor, SIGNAL(diagonalRangeChanged(int)), this, nullptr);
        disconnect(m_sensor, SIGNAL(maxZoneChanged(int)), this, nullptr);
        disconnect(m_sensor, SIGNAL(joyModeChanged()), this, nullptr);
    }

    m_sensor = sensor;
    connect(m_sensor, SIGNAL(deadZoneChanged(int)), this, SLOT(update()));
    connect(m_sensor, SIGNAL(moved(float, float, float)), this, SLOT(update()));
    connect(m_sensor, SIGNAL(diagonalRangeChanged(int)), this, SLOT(update()));
    connect(m_sensor, SIGNAL(maxZoneChanged(int)), this, SLOT(update()));
    connect(m_sensor, SIGNAL(joyModeChanged()), this, SLOT(update()));

    update();
}

JoySensor *JoySensorStatusBox::getSensor() const { return m_sensor; }

int JoySensorStatusBox::heightForWidth(int width) const { return width; }

QSize JoySensorStatusBox::sizeHint() const { return QSize(-1, -1); }

void JoySensorStatusBox::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    PadderCommon::inputDaemonMutex.lock();
    if (m_sensor->getType() == JoySensor::ACCELEROMETER)
        drawAccelerometerBox();
    else
        drawGyroscopeBox();
    PadderCommon::inputDaemonMutex.unlock();
}

void JoySensorStatusBox::drawAccelerometerBox()
{
    // XXX: implement
}

void JoySensorStatusBox::drawGyroscopeBox()
{
    // XXX: implement
}
