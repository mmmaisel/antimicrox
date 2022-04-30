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

#include "sensorcalibration.h"
#include "ui_sensorcalibration.h"

#include "globalvariables.h"
#include "inputdevice.h"
#include "joysensor.h"
#include "joytabwidget.h"

#include <SDL2/SDL_joystick.h>

#include <QDebug>
#include <QFuture>
#include <QLayoutItem>
#include <QMessageBox>
#include <QPointer>
#include <QTabWidget>
#include <QtConcurrent>

SensorCalibration::SensorCalibration(InputDevice *joystick, QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::SensorCalibration)
    , m_type(CAL_NONE)
    , m_joystick(joystick)
    , m_calibrated(false)
{
    m_ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose, true);
    setWindowTitle(tr("Calibration"));

    m_ui->gainXLabel->setVisible(false);
    m_ui->gainYLabel->setVisible(false);
    m_ui->gainZLabel->setVisible(false);
    m_ui->gainXValue->setVisible(false);
    m_ui->gainYValue->setVisible(false);
    m_ui->gainZValue->setVisible(false);
    m_ui->steps->clear();

    int device_count = 0;

    if (m_joystick->getActiveSetJoystick()->hasSensor(GYROSCOPE))
    {
        m_ui->deviceComboBox->addItem(tr("Gyroscope"), QVariant(CAL_GYROSCOPE));
        ++device_count;
    }

    connect(m_joystick, &InputDevice::destroyed,
        this, &SensorCalibration::close);
    connect(m_ui->saveBtn, &QPushButton::clicked,
        this, &SensorCalibration::saveSettings);
    connect(m_ui->cancelBtn, &QPushButton::clicked,
        this, &SensorCalibration::close);
    connect(m_ui->deviceComboBox,
        static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
        this, &SensorCalibration::deviceSelectionChanged);

    if (device_count == 0)
    {
        m_ui->steps->setText(tr(
            "Selected device doesn't have any inputs to calibrate."));
    } else
    {
        int index = m_ui->deviceComboBox->currentIndex();
        auto type = static_cast<CalibrationType>(m_ui->deviceComboBox->itemData(index).toInt());
        selectType(type);
    }

    showCalibrationValues(true, 0, 0, 0);
    update();
}

SensorCalibration::~SensorCalibration() { delete m_ui; }

/**
 * @brief Resets memory of all variables to default, updates window and shows message
 * @return Nothing
 */
void SensorCalibration::resetSettings(bool silentReset, bool)
{
    if (!silentReset)
    {
        QMessageBox msgBox;
        msgBox.setText(tr(
            "Do you really want to reset settings of current sensors?"));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);

        switch (msgBox.exec())
        {
        case QMessageBox::Yes:
            resetCalibrationValues();
            m_ui->steps->clear();
            break;

        case QMessageBox::No:
            break;

        default:
            break;
        }
    } else
    {
        resetCalibrationValues();
        m_ui->steps->clear();
    }
}

void SensorCalibration::showCalibrationValues(
    bool is_calibrated, double x, double y, double z)
{
    if (is_calibrated)
    {
        QPalette palette = m_ui->centerXValue->palette();
        palette.setColor(m_ui->centerXValue->foregroundRole(), Qt::black);
        m_ui->centerXValue->setPalette(palette);
        m_ui->centerYValue->setPalette(palette);
        m_ui->centerZValue->setPalette(palette);
        m_ui->centerXValue->setText(QString::number(x * 180 / M_PI));
        m_ui->centerYValue->setText(QString::number(y * 180 / M_PI));
        m_ui->centerZValue->setText(QString::number(z * 180 / M_PI));
    } else
    {
        QPalette palette = m_ui->centerXValue->palette();
        palette.setColor(m_ui->centerXValue->foregroundRole(), Qt::red);
        m_ui->centerXValue->setPalette(palette);
        m_ui->centerYValue->setPalette(palette);
        m_ui->centerZValue->setPalette(palette);
        m_ui->centerXValue->setText(QString::number(0.0));
        m_ui->centerYValue->setText(QString::number(0.0));
        m_ui->centerZValue->setText(QString::number(0.0));
    }
}

void SensorCalibration::selectType(SensorCalibration::CalibrationType type)
{
    if (m_type == type)
        return;

    disconnect(m_ui->startBtn, &QPushButton::clicked, this, nullptr);
    disconnect(m_ui->resetBtn, &QPushButton::clicked, this, nullptr);
    m_type = type;

    if (m_type == CAL_GYROSCOPE)
    {
        m_ui->statusStack->setCurrentIndex(0);
        m_sensor = m_joystick->getActiveSetJoystick()->
            getSensor(GYROSCOPE);
        m_calibrated = m_sensor->isCalibrated();

        if (m_calibrated)
        {
            double data[3];
            m_sensor->getCalibration(data);
            showCalibrationValues(true, data[0], data[1], data[2]);
        } else
        {
            showCalibrationValues(false, 0.0, 0.0, 0.0);
        }

        m_ui->resetBtn->setEnabled(m_calibrated);
        m_ui->saveBtn->setEnabled(false);

        m_ui->sensorStatusBoxWidget->setFocus();
        m_ui->sensorStatusBoxWidget->setSensor(m_sensor);
        m_ui->sensorStatusBoxWidget->update();

        connect(m_ui->startBtn, &QPushButton::clicked,
            this, &SensorCalibration::startGyroscopeCalibration);
        connect(m_ui->resetBtn, &QPushButton::clicked,
            [this](bool clicked) { resetSettings(false, clicked); });
        m_ui->startBtn->setEnabled(true);
        m_ui->resetBtn->setEnabled(true);
    }
}

void SensorCalibration::resetCalibrationValues()
{
    if (m_type == CAL_GYROSCOPE && m_sensor != nullptr)
    {
        m_sensor->resetCalibration();
        m_calibrated = false;

        m_ui->saveBtn->setEnabled(false);
        m_ui->resetBtn->setEnabled(false);
        m_ui->sensorStatusBoxWidget->update();
        showCalibrationValues(false, 0, 0, 0);

        update();
    }
}

void SensorCalibration::deviceSelectionChanged(int index)
{
    auto type = static_cast<CalibrationType>(m_ui->deviceComboBox->itemData(index).toInt());
    selectType(type);
}

void SensorCalibration::onGyroscopeData(float x, float y, float z)
{
    // Calculate mean and variance using Welford's algorithm
    // as well as 3 sigma interval width.
    ++m_sample_count;
    double dx = x - m_mean[0];
    double dy = y - m_mean[1];
    double dz = z - m_mean[2];

    m_mean[0] += dx / m_sample_count;
    m_mean[1] += dy / m_sample_count;
    m_mean[2] += dz / m_sample_count;

    double dx2 = x - m_mean[0];
    double dy2 = y - m_mean[1];
    double dz2 = z - m_mean[2];

    m_var[0] += dx * dx2;
    m_var[1] += dy * dy2;
    m_var[2] += dz * dz2;

    double varx = m_var[0] / (m_sample_count - 1);
    double vary = m_var[1] / (m_sample_count - 1);
    double varz = m_var[2] / (m_sample_count - 1);

    showCalibrationValues(true, m_mean[0], m_mean[1], m_mean[2]);

    double wx = 9*varx/m_sample_count;
    double wy = 9*vary/m_sample_count;
    double wz = 9*varz/m_sample_count;

    // Abort when end time is reached to avoid infinite loop
    // in case of noisy sensors.
    if ((wx < 1e-7 && wy < 1e-7 && wz < 1e-7 && m_sample_count > 10) ||
        (QDateTime::currentDateTime() > m_end_time))
    {
        disconnect(m_sensor, &JoySensor::moved,
            this, &SensorCalibration::onGyroscopeData);
        disconnect(m_ui->startBtn, &QPushButton::clicked, this, nullptr);
        connect(m_ui->startBtn, &QPushButton::clicked, this,
            &SensorCalibration::startGyroscopeCalibration);
        m_ui->steps->setText(tr("Calibration completed."));
        m_ui->startBtn->setText(tr("Start calibration"));
        m_ui->startBtn->setEnabled(true);
        m_ui->saveBtn->setEnabled(true);
        m_ui->deviceComboBox->setEnabled(true);
        update();
    }
}

/**
 * @brief iSave calibration values into JoySensor object
 * @return nothing
 */
void SensorCalibration::saveSettings()
{
    if (m_type == CAL_GYROSCOPE)
    {
        m_joystick->applyGyroscopeCalibration(
            m_mean[0], m_mean[1], m_mean[2]);
        m_calibrated = true;
        m_ui->saveBtn->setEnabled(false);
        m_ui->resetBtn->setEnabled(true);
    }
}

/**
 * @brief Prepares first step of calibration - gyroscope center
 * @return nothing
 */
void SensorCalibration::startGyroscopeCalibration()
{
    bool confirmed = true;

    if (m_calibrated)
    {
        QMessageBox msgBox;
        msgBox.setText(tr(
            "Calibration was saved for the preset. Do you really want to reset settings?"));
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);

        switch (msgBox.exec())
        {
        case QMessageBox::Ok:
            confirmed = true;
            m_ui->resetBtn->setEnabled(false);
            break;

        case QMessageBox::Cancel:
            confirmed = false;
            break;

        default:
            confirmed = true;
            break;
        }
    }

    if (m_sensor != nullptr && confirmed)
    {
        m_mean[0] = 0;
        m_mean[1] = 0;
        m_mean[2] = 0;
        m_var[0] = 0;
        m_var[1] = 0;
        m_var[2] = 0;
        m_sample_count = 0;

        m_sensor->resetCalibration();
        m_calibrated = false;

        m_ui->steps->setText(tr(
            "Place the controller at rest, e.g. put it on the desk, "
            "and press continue."));
        setWindowTitle(tr("Calibrating gyroscope"));
        m_ui->startBtn->setText(tr("Continue calibration"));
        update();

        m_ui->deviceComboBox->setEnabled(false);
        disconnect(m_ui->startBtn, &QPushButton::clicked, this, nullptr);
        connect(m_ui->startBtn, &QPushButton::clicked, this,
            &SensorCalibration::startGyroscopeCenterCalibration);
    }
}

/**
 * @brief Performs gyroscope center calibration.
 * @return nothing
 */
void SensorCalibration::startGyroscopeCenterCalibration()
{
    if (m_sensor != nullptr)
    {
        m_end_time = QDateTime::currentDateTime().addSecs(3);
        m_ui->steps->setText(tr("Collecting gyroscope data..."));
        connect(m_sensor, &JoySensor::moved,
            this, &SensorCalibration::onGyroscopeData);
        update();

        m_ui->startBtn->setEnabled(false);
        disconnect(m_ui->startBtn, &QPushButton::clicked, this, nullptr);
    }
}
