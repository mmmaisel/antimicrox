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
    , m_joystick(joystick)
{
    m_ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose, true);
    setWindowTitle(tr("Calibration"));

    // XXX: handle no sensor case
    // XXX: gyro status box?
    // XXX: show targets in accel status box
    m_accelerometer = m_joystick->getActiveSetJoystick()->
        getSensor(JoySensor::ACCELEROMETER);
    m_gyroscope = m_joystick->getActiveSetJoystick()->
        getSensor(JoySensor::GYROSCOPE);
    m_calibrated = m_accelerometer->isCalibrated() && m_gyroscope->isCalibrated();

    if (m_gyroscope->isCalibrated())
    {
        float data[3];
        m_gyroscope->getCalibration(data);
        showGyroCalibrationValues(true, data[0], data[1], data[2]);
    } else
    {
        showGyroCalibrationValues(false, 0.0, 0.0, 0.0);
    }

    m_ui->resetBtn->setEnabled(m_calibrated);
    m_ui->saveBtn->setEnabled(false);

    m_ui->sensorStatusBoxWidget->setFocus();
    // XXX: sensorStatusBox must not receive a nullptr
    m_ui->sensorStatusBoxWidget->setSensor(m_accelerometer);
    m_ui->sensorStatusBoxWidget->update();

    connect(m_joystick, &InputDevice::destroyed,
        this, &SensorCalibration::close);
    connect(m_ui->saveBtn, &QPushButton::clicked,
        this, &SensorCalibration::saveSettings);
    connect(m_ui->cancelBtn, &QPushButton::clicked,
        this, &SensorCalibration::close);
    connect(m_ui->startButton, &QPushButton::clicked,
        this, &SensorCalibration::startCalibration);
    connect(m_ui->resetBtn, &QPushButton::clicked,
        [this](bool clicked) { resetSettings(false, clicked); });

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

void SensorCalibration::showGyroCalibrationValues(
    bool is_calibrated, float x, float y, float z)
{
    if (is_calibrated)
    {
        QPalette palette = m_ui->labelGyroX0->palette();
        palette.setColor(m_ui->labelGyroX0->foregroundRole(), Qt::black);
        m_ui->labelGyroX0->setPalette(palette);
        m_ui->labelGyroY0->setPalette(palette);
        m_ui->labelGyroZ0->setPalette(palette);
        m_ui->labelGyroX0->setText(QString::number(x * 180 / M_PI));
        m_ui->labelGyroY0->setText(QString::number(y * 180 / M_PI));
        m_ui->labelGyroZ0->setText(QString::number(z * 180 / M_PI));
    } else
    {
        QPalette palette = m_ui->labelGyroX0->palette();
        palette.setColor(m_ui->labelGyroX0->foregroundRole(), Qt::red);
        m_ui->labelGyroX0->setPalette(palette);
        m_ui->labelGyroY0->setPalette(palette);
        m_ui->labelGyroZ0->setPalette(palette);
        m_ui->labelGyroX0->setText(QString::number(0.0));
        m_ui->labelGyroY0->setText(QString::number(0.0));
        m_ui->labelGyroZ0->setText(QString::number(0.0));
    }
}

void SensorCalibration::resetCalibrationValues()
{
    m_accelerometer->resetCalibration();
    m_gyroscope->resetCalibration();
    m_calibrated = false;

    m_ui->saveBtn->setEnabled(false);
    m_ui->resetBtn->setEnabled(false);
    m_ui->sensorStatusBoxWidget->update();

    update();
}

void SensorCalibration::onGyroscopeData(float x, float y, float z)
{
    m_gyro_center[0] += x;
    m_gyro_center[1] += y;
    m_gyro_center[2] += z;
    ++m_sample_count;

    if (m_sample_count > 1000 && QDateTime::currentDateTime() > m_end_time)
    {
        disconnect(m_gyroscope, &JoySensor::moved,
            this, &SensorCalibration::onGyroscopeData);
        disconnect(m_ui->startButton, &QPushButton::clicked, this, nullptr);
        connect(m_ui->startButton, &QPushButton::clicked, this,
            &SensorCalibration::startCalibration);
        m_ui->steps->setText(tr("Calibration completed."));
        m_ui->startButton->setEnabled(true);

        float calX = m_gyro_center[0] / double(m_sample_count);
        float calY = m_gyro_center[1] / double(m_sample_count);
        float calZ = m_gyro_center[2] / double(m_sample_count);

        showGyroCalibrationValues(true, calX, calY, calZ);
        m_ui->saveBtn->setEnabled(true);
        update();
    }
}

/**
 * @brief iSave calibration values into JoySensor object
 * @return nothing
 */
void SensorCalibration::saveSettings()
{
    float calX = m_gyro_center[0] / double(m_sample_count);
    float calY = m_gyro_center[1] / double(m_sample_count);
    float calZ = m_gyro_center[2] / double(m_sample_count);
    m_joystick->applyGyroscopeCalibration(calX, calY, calZ);
    m_ui->saveBtn->setEnabled(false);
}

/**
 * @brief Prepares first step of calibration - gyroscope center
 * @return nothing
 */
void SensorCalibration::startCalibration()
{
    bool confirmed = true;

    if (m_gyroscope->isCalibrated() || m_accelerometer->isCalibrated())
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

    if (m_gyroscope != nullptr && confirmed)
    {
        m_gyro_center[0] = 0;
        m_gyro_center[1] = 0;
        m_gyro_center[2] = 0;
        m_sample_count = 0;

        m_gyroscope->resetCalibration();
        m_calibrated = false;

        m_ui->steps->setText(tr(
            "Place the controller at rest, e.g. put it on the desk, "
            "and press continue."));
        setWindowTitle(tr("Calibrating gyroscope"));
        // XXX: implement second step for accelerometer.
        m_ui->startButton->setText(tr("Continue calibration"));
        update();

        disconnect(m_ui->startButton, &QPushButton::clicked, this, nullptr);
        connect(m_ui->startButton, &QPushButton::clicked, this,
            &SensorCalibration::startGyroscopeCenterCalibration);
    }
}

/**
 * @brief Performs gyroscope center calibration.
 * @return nothing
 */
void SensorCalibration::startGyroscopeCenterCalibration()
{
    if (m_gyroscope != nullptr)
    {
        m_end_time = QDateTime::currentDateTime().addSecs(3);
        m_ui->steps->setText(tr("Collecting gyroscope data..."));
        connect(m_gyroscope, &JoySensor::moved,
            this, &SensorCalibration::onGyroscopeData);
        update();

        m_ui->startButton->setEnabled(false);
        disconnect(m_ui->startButton, &QPushButton::clicked, this, nullptr);
        connect(m_ui->startButton, &QPushButton::clicked, this,
            &SensorCalibration::startCalibration);
    }
}
