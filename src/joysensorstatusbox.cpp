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

    connect(m_sensor, SIGNAL(deadZoneChanged(float)), this, SLOT(update()));
    connect(m_sensor, SIGNAL(moved(float, float, float)), this, SLOT(update()));
    connect(m_sensor, SIGNAL(diagonalRangeChanged(int)), this, SLOT(update()));
    connect(m_sensor, SIGNAL(maxZoneChanged(float)), this, SLOT(update()));
    connect(m_sensor, SIGNAL(joyModeChanged()), this, SLOT(update()));
    connect(m_sensor, SIGNAL(circleAdjustChange(double)), this, SLOT(update()));
}

void JoySensorStatusBox::setSensor(JoySensor *sensor)
{
    if (m_sensor != nullptr)
    {
        disconnect(m_sensor, SIGNAL(deadZoneChanged(float)), this, nullptr);
        disconnect(m_sensor, SIGNAL(moved(float, float, float)), this, nullptr);
        disconnect(m_sensor, SIGNAL(diagonalRangeChanged(int)), this, nullptr);
        disconnect(m_sensor, SIGNAL(maxZoneChanged(float)), this, nullptr);
        disconnect(m_sensor, SIGNAL(joyModeChanged()), this, nullptr);
    }

    m_sensor = sensor;
    connect(m_sensor, SIGNAL(deadZoneChanged(float)), this, SLOT(update()));
    connect(m_sensor, SIGNAL(moved(float, float, float)), this, SLOT(update()));
    connect(m_sensor, SIGNAL(diagonalRangeChanged(int)), this, SLOT(update()));
    connect(m_sensor, SIGNAL(maxZoneChanged(float)), this, SLOT(update()));
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
        drawSensorBox(GlobalVariables::JoySensor::ACCEL_MAX);
    else
        drawSensorBox(GlobalVariables::JoySensor::GYRO_MAX);
    PadderCommon::inputDaemonMutex.unlock();
}

void JoySensorStatusBox::drawSensorBox(float scale)
{
    QPainter paint(this);
    paint.setRenderHint(QPainter::Antialiasing, true);

    int side = qMin(width() - 2, height() - 2);

    QPixmap pix(side, side);
    pix.fill(Qt::transparent);
    QPainter painter(&pix);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Draw box outline
    QPen pen;
    pen.setColor(Qt::black);
    pen.setWidth(1);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(0, 0, side - 1, side - 1);
    painter.save();

    // Switch coordinate system
    float window_scale = (scale * 2.0 + 1.0);
    painter.scale(side / window_scale, side / window_scale);
    painter.translate(window_scale/2.0, window_scale/2.0);

    // Draw max zone and initial inner clear circle
    float maxzone = m_sensor->getMaxZone();
    pen.setWidthF(scale/10.0);
    painter.setPen(pen);
    painter.setOpacity(0.5);
    painter.setBrush(Qt::darkGreen);
    painter.drawEllipse(QPointF(0.0, 0.0), scale, scale);
    painter.setCompositionMode(QPainter::CompositionMode_Clear);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(QPointF(0.0, 0.0), maxzone, maxzone);

    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.setOpacity(1.0);

    // Draw diagonal zones
    pen.setWidth(0);
    pen.setColor(Qt::black);
    painter.setPen(pen);
    painter.setOpacity(0.5);
    painter.setBrush(QBrush(Qt::green));

    for(int i = 0; i < 4; ++i)
    {
        painter.drawPie(
            QRectF(-scale, -scale, scale * 2.0, scale * 2.0),
            (45 + 90*i - m_sensor->getDiagonalRange()/2) * 16,
            m_sensor->getDiagonalRange() * 16);
    }
    painter.setOpacity(1.0);

    // Draw deadzone circle
    pen.setWidth(0);
    painter.setPen(pen);
    painter.setBrush(QBrush(Qt::red));
    painter.drawEllipse(QPointF(0.0, 0.0),
        m_sensor->getDeadZone(), m_sensor->getDeadZone());

    pen.setWidth(0);
    painter.setBrush(QBrush(Qt::blue));
    pen.setColor(Qt::blue);
    painter.setPen(pen);

    // Draw raw crosshair
    float xsens = m_sensor->getXCoordinate();
    float ysens = m_sensor->getYCoordinate();
    float zsens = m_sensor->getZCoordinate();

    float xp = xsens + 0.5 * zsens;
    float yp = ysens - 0.5 * zsens;

    pen.setWidthF(scale/20.0);
    painter.setPen(pen);
    painter.drawLine(QPointF(xsens, ysens), QPointF(xp, yp));
    painter.setPen(Qt::NoPen);
    painter.drawRect(
        QRectF(xp-scale/20.0, yp-scale/20.0, scale/10.0, scale/10.0));

    painter.setBrush(QBrush(Qt::darkBlue));
    pen.setColor(Qt::darkBlue);
    painter.setPen(pen);

    // Back to window coordinate system.
    painter.restore();
    pen.setWidth(0);
    pen.setColor(Qt::black);
    painter.setPen(pen);
    painter.scale(side / 2, side / 2);
    painter.translate(1, 1);
    painter.setPen(pen);
    painter.setOpacity(0.5);
    // Draw Y line
    painter.drawLine(QPointF(0.0, -1.0), QPointF(0.0, 1.0));
    // Draw X line
    painter.drawLine(QPointF(-1.0, 0.0), QPointF(1.0, 0.0));
    // Draw Z line
    painter.setOpacity(0.25);
    painter.drawLine(QPointF(-0.5, 0.5), QPointF(0.5, -0.5));

    // Draw to window
    paint.setCompositionMode(QPainter::CompositionMode_SourceOver);
    paint.drawPixmap(pix.rect(), pix);
}
