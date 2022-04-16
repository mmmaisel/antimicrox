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

#include "joysensoreditdialog.h"
#include "ui_joysensoreditdialog.h"

#include "antkeymapper.h"
#include "buttoneditdialog.h"
#include "common.h"
#include "event.h"
#include "inputdevice.h"
#include "joybuttontypes/joysensorbutton.h"
#include "joysensor.h"
#include "mousedialog/mousecontrolsticksettingsdialog.h"
#include "setjoystick.h"

#include <QDebug>
#include <QHash>
#include <QHashIterator>
#include <QList>
#include <QWidget>

JoySensorEditDialog::JoySensorEditDialog(JoySensor *sensor, QWidget *parent)
    : QDialog(parent, Qt::Window),
    m_ui(new Ui::JoySensorEditDialog),
    m_sensor(sensor),
    m_helper(sensor)
{
    m_ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    getHelperLocal().moveToThread(m_sensor->thread());

    PadderCommon::inputDaemonMutex.lock();

    updateWindowTitleSensorName();

    m_ui->deadZoneSlider->setValue(m_sensor->getDeadZone());
    m_ui->deadZoneSpinBox->setValue(m_sensor->getDeadZone());

    m_ui->maxZoneSlider->setValue(m_sensor->getMaxZone());
    m_ui->maxZoneSpinBox->setValue(m_sensor->getMaxZone());

    m_ui->diagonalRangeSlider->setValue(m_sensor->getDiagonalRange());
    m_ui->diagonalRangeSpinBox->setValue(m_sensor->getDiagonalRange());

    QString xCoorString = QString::number(m_sensor->getXCoordinate());
    m_ui->xCoordinateValue->setText(xCoorString);

    QString yCoorString = QString::number(m_sensor->getYCoordinate());
    m_ui->yCoordinateValue->setText(yCoorString);

    QString zCoorString = QString::number(m_sensor->getZCoordinate());
    m_ui->zCoordinateValue->setText(zCoorString);

    if (m_sensor->getType() == JoySensor::ACCELEROMETER)
    {
        m_ui->gravityValue->setText(QString::number(m_sensor->getAbsoluteRawGravity()));
        m_ui->pitchValue->setText(QString::number(m_sensor->calculatePitch()));
        m_ui->rollValue->setText(QString::number(m_sensor->calculateRoll()));
    } else
    {
        m_ui->xCoordinateLabel->setText(tr("Roll"));
        m_ui->yCoordinateLabel->setText(tr("Pitch"));
        m_ui->zCoordinateLabel->setText(tr("Yaw"));
        m_ui->gravityLabel->setVisible(false);
        m_ui->gravityValue->setVisible(false);
        m_ui->pitchLabel->setVisible(false);
        m_ui->pitchValue->setVisible(false);
        m_ui->rollLabel->setVisible(false);
        m_ui->rollValue->setVisible(false);
    }

    m_ui->sensorStatusBoxWidget->setSensor(m_sensor);

    selectCurrentPreset();

    m_ui->sensorNameLineEdit->setText(m_sensor->getSensorName());
    double validDistance = m_sensor->getDistanceFromDeadZone() * 100.0;
    m_ui->fromSafeZoneValueLabel->setText(QString::number(validDistance));

    int sensorDelay = m_sensor->getSensorDelay();
    m_ui->sensorDelaySlider->setValue(sensorDelay * .1);
    m_ui->sensorDelayDoubleSpinBox->setValue(sensorDelay * .001);

    update();
    updateGeometry();

    PadderCommon::inputDaemonMutex.unlock();

    connect(m_ui->presetsComboBox,
        static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
        this, &JoySensorEditDialog::implementPresets);

    connect(m_ui->deadZoneSlider, &QSlider::valueChanged,
        m_ui->deadZoneSpinBox, &QSpinBox::setValue);
    connect(m_ui->maxZoneSlider, &QSlider::valueChanged,
        m_ui->maxZoneSpinBox, &QSpinBox::setValue);
    connect(m_ui->diagonalRangeSlider, &QSlider::valueChanged,
        m_ui->diagonalRangeSpinBox, &QSpinBox::setValue);

    connect(m_ui->deadZoneSpinBox,
        static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
        m_ui->deadZoneSlider, &QSlider::setValue);
    connect(m_ui->maxZoneSpinBox,
        static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
        m_ui->maxZoneSlider, &QSlider::setValue);
    connect(m_ui->maxZoneSpinBox,
        static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
        this, &JoySensorEditDialog::checkMaxZone);
    connect(m_ui->diagonalRangeSpinBox,
        static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
        m_ui->diagonalRangeSlider, &QSlider::setValue);

    connect(m_ui->deadZoneSpinBox,
        static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
        m_sensor, &JoySensor::setDeadZone);
    connect(m_ui->diagonalRangeSpinBox,
        static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
        m_sensor, &JoySensor::setDiagonalRange);
    connect(m_sensor, &JoySensor::sensorDelayChanged, this,
        &JoySensorEditDialog::updateSensorDelaySpinBox);
    connect(m_ui->sensorDelayDoubleSpinBox,
        static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
        this, &JoySensorEditDialog::updateSensorDelaySlider);

    connect(m_sensor, &JoySensor::moved, this, &JoySensorEditDialog::refreshSensorStats);
    connect(m_ui->mouseSettingsPushButton, &QPushButton::clicked, this,
        &JoySensorEditDialog::openMouseSettingsDialog);

    connect(m_ui->sensorNameLineEdit, &QLineEdit::textEdited,
        m_sensor, &JoySensor::setSensorName);
    connect(m_sensor, &JoySensor::sensorNameChanged, this,
        &JoySensorEditDialog::updateWindowTitleSensorName);
}

JoySensorEditDialog::~JoySensorEditDialog() { delete m_ui; }

void JoySensorEditDialog::implementPresets(int index)
{
    // XXX: implement
}

void JoySensorEditDialog::refreshSensorStats(int x, int y, int z)
{
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(z);

    PadderCommon::inputDaemonMutex.lock();

    QString xCoorString = QString::number(m_sensor->getXCoordinate());
    m_ui->xCoordinateLabel->setText(xCoorString);

    QString yCoorString = QString::number(m_sensor->getYCoordinate());
    m_ui->yCoordinateLabel->setText(yCoorString);

    QString zCoorString = QString::number(m_sensor->getZCoordinate());
    m_ui->zCoordinateLabel->setText(zCoorString);

    if (m_sensor->getType() == JoySensor::ACCELEROMETER)
    {
        m_ui->gravityValue->setText(QString::number(m_sensor->getAbsoluteRawGravity()));
        m_ui->pitchValue->setText(QString::number(m_sensor->calculatePitch()));
        m_ui->rollValue->setText(QString::number(m_sensor->calculateRoll()));
    }

    double validDistance = m_sensor->getDistanceFromDeadZone() * 100.0;
    m_ui->fromSafeZoneValueLabel->setText(QString::number(validDistance));

    PadderCommon::inputDaemonMutex.unlock();
}

void JoySensorEditDialog::checkMaxZone(int value)
{
    if (value > m_ui->deadZoneSpinBox->value())
        QMetaObject::invokeMethod(m_sensor, "setMaxZone", Q_ARG(int, value));
}

void JoySensorEditDialog::selectCurrentPreset()
{
    // XXX: implement
}

void JoySensorEditDialog::openMouseSettingsDialog()
{
    // XXX: implement
}

void JoySensorEditDialog::enableMouseSettingButton()
{
    m_ui->mouseSettingsPushButton->setEnabled(true);
}

void JoySensorEditDialog::updateWindowTitleSensorName()
{
    QString temp = QString(tr("Set")).append(" ");

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
        {
            temp.append(": ").append(setName);
        }

        temp.append("]");
    }

    setWindowTitle(temp);
}

/**
 * @brief Update QDoubleSpinBox value based on updated sensor delay value.
 * @param Delay value obtained from JoySensor.
 */
void JoySensorEditDialog::updateSensorDelaySpinBox(int value)
{
    double temp = value * 0.001;
    m_ui->sensorDelayDoubleSpinBox->setValue(temp);
}

/**
 * @brief Update QSlider value based on value from QDoubleSpinBox.
 * @param Value from QDoubleSpinBox.
 */
void JoySensorEditDialog::updateSensorDelaySlider(double value)
{
    int temp = value * 100;

    if (m_ui->sensorDelaySlider->value() != temp)
        m_ui->sensorDelaySlider->setValue(temp);
}

JoySensorEditDialogHelper &JoySensorEditDialog::getHelperLocal() { return m_helper; }
