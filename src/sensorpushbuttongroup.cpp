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

#include "sensorpushbuttongroup.h"

#include "buttoneditdialog.h"
#include "inputdevice.h"
#include "joybuttontypes/joysensorbutton.h"
#include "joysensor.h"
#include "joysensorbuttonpushbutton.h"
#include "joysensoreditdialog.h"
#include "joysensorpushbutton.h"

#include <QDebug>
#include <QHash>
#include <QWidget>

SensorPushButtonGroup::SensorPushButtonGroup(
    JoySensor *sensor, bool keypadUnlocked, bool displayNames, QWidget *parent)
    : QGridLayout(parent),
    m_sensor(sensor),
    m_display_names(displayNames)
{
    generateButtons();
}

void SensorPushButtonGroup::generateButtons()
{
    if (m_sensor->getType() == JoySensor::ACCELEROMETER)
    {
        m_left_button = generateBtnToGrid(JoySensorDirection::ACCEL_LEFT, 1, 0);
        m_right_button = generateBtnToGrid(JoySensorDirection::ACCEL_RIGHT, 1, 2);
        m_up_button = generateBtnToGrid(JoySensorDirection::ACCEL_UP, 0, 1);
        m_down_button = generateBtnToGrid(JoySensorDirection::ACCEL_DOWN, 2, 1);
        m_fwd_button = generateBtnToGrid(JoySensorDirection::ACCEL_FWD, 0, 2);
        m_bwd_button = generateBtnToGrid(JoySensorDirection::ACCEL_BWD, 2, 0);
    }
    else
    {
        m_left_button = generateBtnToGrid(JoySensorDirection::GYRO_ROLL_N, 1, 0);
        m_right_button = generateBtnToGrid(JoySensorDirection::GYRO_ROLL_P, 1, 2);
        m_up_button = generateBtnToGrid(JoySensorDirection::GYRO_NICK_P, 0, 1);
        m_down_button = generateBtnToGrid(JoySensorDirection::GYRO_NICK_N, 2, 1);
        m_fwd_button = generateBtnToGrid(JoySensorDirection::GYRO_YAW_P, 0, 2);
        m_bwd_button = generateBtnToGrid(JoySensorDirection::GYRO_YAW_N, 2, 0);
    }

    m_sensor_widget = new JoySensorPushButton(m_sensor, m_display_names, parentWidget());
    m_sensor_widget->setIcon(
        QIcon::fromTheme(QString::fromUtf8("games_config_options"),
        QIcon(":/images/actions/games_config_options.png"))
    );

    connect(m_sensor_widget, &JoySensorPushButton::clicked, this,
        &SensorPushButtonGroup::showSensorDialog);

    addWidget(m_sensor_widget, 1, 1);
}

JoySensorButtonPushButton *
SensorPushButtonGroup::generateBtnToGrid(
    JoySensorDirection sensorDir, int gridRow, int gridCol)
{
    JoySensorButton *button = m_sensor->getButtons()->value(sensorDir);
    JoySensorButtonPushButton *pushbutton =
        new JoySensorButtonPushButton(button, m_display_names, parentWidget());

    connect(pushbutton, &JoySensorButtonPushButton::clicked, this,
        [this, pushbutton] { openSensorButtonDialog(pushbutton); });

    button->establishPropertyUpdatedConnections();
    connect(button, &JoySensorButton::slotsChanged, this,
        &SensorPushButtonGroup::propogateSlotsChanged);

    addWidget(pushbutton, gridRow, gridCol);
    return pushbutton;
}

void SensorPushButtonGroup::propogateSlotsChanged() { emit buttonSlotChanged(); }

JoySensor *SensorPushButtonGroup::getSensor() const { return m_sensor; }

void SensorPushButtonGroup::openSensorButtonDialog(JoySensorButtonPushButton *pushbutton)
{
    // XXX: implement
    ButtonEditDialog *dialog = new ButtonEditDialog(
        pushbutton->getButton(), m_sensor->getParentSet()->getInputDevice(),
        /*m_keypad_unlocked*/true, parentWidget());
    dialog->show();
}

void SensorPushButtonGroup::showSensorDialog()
{
    JoySensorEditDialog *dialog =
        new JoySensorEditDialog(m_sensor, parentWidget());
    dialog->show();
}

void SensorPushButtonGroup::toggleNameDisplay()
{
    m_display_names = !m_display_names;

    m_up_button->toggleNameDisplay();
    m_down_button->toggleNameDisplay();
    m_left_button->toggleNameDisplay();
    m_right_button->toggleNameDisplay();
    m_fwd_button->toggleNameDisplay();
    m_bwd_button->toggleNameDisplay();

    m_sensor_widget->toggleNameDisplay();
}

bool SensorPushButtonGroup::ifDisplayNames() const
{
    return m_display_names;
}

JoySensorButtonPushButton *SensorPushButtonGroup::getUpButton() const
{
    return m_up_button;
}

JoySensorButtonPushButton *SensorPushButtonGroup::getDownButton() const
{
    return m_down_button;
}

JoySensorButtonPushButton *SensorPushButtonGroup::getLeftButton() const
{
    return m_left_button;
}

JoySensorButtonPushButton *SensorPushButtonGroup::getRightButton() const
{
    return m_right_button;
}

JoySensorButtonPushButton *SensorPushButtonGroup::getFwdButton() const
{
    return m_fwd_button;
}

JoySensorButtonPushButton *SensorPushButtonGroup::getBwdButton() const
{
    return m_bwd_button;
}

JoySensorPushButton *SensorPushButtonGroup::getSensorWidget() const
{
    return m_sensor_widget;
}
