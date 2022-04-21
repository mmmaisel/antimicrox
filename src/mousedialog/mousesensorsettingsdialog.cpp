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

#include "mousesensorsettingsdialog.h"

#include "common.h"
#include "inputdevice.h"
#include "joybuttontypes/joysensorbutton.h"
#include "joysensor.h"
#include "setjoystick.h"

#include <QComboBox>
#include <QDebug>
#include <QHashIterator>
#include <QSpinBox>

MouseSensorSettingsDialog::MouseSensorSettingsDialog(JoySensor *sensor, QWidget *parent)
    : MouseSettingsDialog(parent),
    m_sensor(sensor)
{
    setAttribute(Qt::WA_DeleteOnClose);

    ui->topGroupBox->setVisible(false);
    ui->springGroupBox->setVisible(false);
    ui->extraAccelerationGroupBox->setVisible(false);
    ui->sensLabel->setVisible(false);
    ui->sensitivityDoubleSpinBox->setVisible(false);
    ui->easingDurationLabel->setVisible(false);
    ui->easingDoubleSpinBox->setVisible(false);

    calculateMouseSpeedPreset();

    updateWindowTitleSensorName();
    calculateWheelSpeedPreset();

    changeSettingsWidgetStatus(ui->accelerationComboBox->currentIndex());

    connect(ui->horizontalSpinBox,
        static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
        this, &MouseSensorSettingsDialog::updateConfigHorizontalSpeed);
    connect(ui->verticalSpinBox,
        static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
        this, &MouseSensorSettingsDialog::updateConfigVerticalSpeed);

    connect(ui->sensitivityDoubleSpinBox,
        static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
        this, &MouseSensorSettingsDialog::updateSensitivity);

    connect(ui->wheelHoriSpeedSpinBox,
        static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
        this, &MouseSensorSettingsDialog::updateWheelSpeedHorizontalSpeed);
    connect(ui->wheelVertSpeedSpinBox,
        static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
        this, &MouseSensorSettingsDialog::updateWheelSpeedVerticalSpeed);
}

void MouseSensorSettingsDialog::updateConfigHorizontalSpeed(int value)
{
    QHashIterator<JoySensorDirection, JoySensorButton *>
        iter(*m_sensor->getButtons());
    while (iter.hasNext())
    {
        JoySensorButton *button = iter.next().value();
        button->setMouseSpeedX(value);
    }
}

void MouseSensorSettingsDialog::updateConfigVerticalSpeed(int value)
{
    QHashIterator<JoySensorDirection, JoySensorButton *>
        iter(*m_sensor->getButtons());
    while (iter.hasNext())
    {
        JoySensorButton *button = iter.next().value();
        button->setMouseSpeedY(value);
    }
}

void MouseSensorSettingsDialog::calculateMouseSpeedPreset()
{
    QHashIterator<JoySensorDirection, JoySensorButton *>
        iter(*m_sensor->getButtons());
    int mouseSpeedX = 0;
    int mouseSpeedY = 0;
    while (iter.hasNext())
    {
        JoySensorButton *button = iter.next().value();
        mouseSpeedX = qMax(mouseSpeedX, button->getMouseSpeedX());
        mouseSpeedY = qMax(mouseSpeedY, button->getMouseSpeedY());
    }

    ui->horizontalSpinBox->setValue(mouseSpeedX);
    ui->verticalSpinBox->setValue(mouseSpeedY);
}

void MouseSensorSettingsDialog::updateSensitivity(double value)
{
    QHashIterator<JoySensorDirection, JoySensorButton *>
        iter(*m_sensor->getButtons());

    while (iter.hasNext())
    {
        JoySensorButton *button = iter.next().value();
        button->setSensitivity(value);
    }
}

void MouseSensorSettingsDialog::calculateWheelSpeedPreset()
{
    QHashIterator<JoySensorDirection, JoySensorButton *>
        iter(*m_sensor->getButtons());
    int tempWheelSpeedX = 0;
    int tempWheelSpeedY = 0;
    while (iter.hasNext())
    {
        JoySensorButton *button = iter.next().value();
        tempWheelSpeedX = qMax(tempWheelSpeedX, button->getWheelSpeedX());
        tempWheelSpeedY = qMax(tempWheelSpeedY, button->getWheelSpeedY());
    }

    ui->wheelHoriSpeedSpinBox->setValue(tempWheelSpeedX);
    ui->wheelVertSpeedSpinBox->setValue(tempWheelSpeedY);
}

void MouseSensorSettingsDialog::updateWheelSpeedHorizontalSpeed(int value)
{
    QHashIterator<JoySensorDirection, JoySensorButton *>
        iter(*m_sensor->getButtons());

    while (iter.hasNext())
    {
        JoySensorButton *button = iter.next().value();
        button->setWheelSpeed(value, 'X');
    }
}

void MouseSensorSettingsDialog::updateWheelSpeedVerticalSpeed(int value)
{
    QHashIterator<JoySensorDirection, JoySensorButton *>
        iter(*m_sensor->getButtons());

    while (iter.hasNext())
    {
        JoySensorButton *button = iter.next().value();
        button->setWheelSpeed(value, 'Y');
    }
}

void MouseSensorSettingsDialog::changeMouseMode(int index)
{
    Q_UNUSED(index);
}

void MouseSensorSettingsDialog::changeMouseCurve(int index)
{
    Q_UNUSED(index);
}

void MouseSensorSettingsDialog::updateWindowTitleSensorName()
{
    QString temp = QString(tr("Mouse Settings")).append(" - ");

    if (!m_sensor->getSensorName().isEmpty())
        temp.append(m_sensor->getPartialName(false, true));
    else
        temp.append(m_sensor->getPartialName());

    if (m_sensor->getParentSet()->getIndex() != 0)
    {
        int setIndex = m_sensor->getParentSet()->getRealIndex();
        temp.append(" [").append(tr("Set %1").arg(setIndex));

        QString setName = m_sensor->getParentSet()->getName();
        if (!setName.isEmpty())
            temp.append(": ").append(setName);

        temp.append("]");
    }

    setWindowTitle(temp);
}

JoySensor *MouseSensorSettingsDialog::getSensor() const { return m_sensor; }
