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
#include "joycontrolstick.h"
#include "joysensor.h"

#include <QDebug>
#include <QMessageBox>

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
    hideCalibrationData();

    int device_count = 0;

    QHash<int, JoyControlStick *> sticks = m_joystick->getActiveSetJoystick()->getSticks();
    for (auto iter = sticks.cbegin(); iter != sticks.cend(); ++iter)
    {
        m_ui->deviceComboBox->addItem(iter.value()->getPartialName(),
            QVariant(int(CAL_STICK | (iter.key() << CAL_INDEX_POS))));
        ++device_count;
    }

    if (m_joystick->getActiveSetJoystick()->hasSensor(GYROSCOPE))
    {
        m_ui->deviceComboBox->addItem(tr("Gyroscope"), QVariant(int(CAL_GYROSCOPE)));
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
        unsigned int data = m_ui->deviceComboBox->itemData(index).toInt();
        selectTypeIndex(data);
    }

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
    bool is_calibrated, double x, double y, double z)
{
    QPalette palette = m_ui->centerXValue->palette();
    if (is_calibrated)
        palette.setColor(m_ui->centerXValue->foregroundRole(), Qt::black);
    else
        palette.setColor(m_ui->centerXValue->foregroundRole(), Qt::red);
    m_ui->centerXValue->setPalette(palette);
    m_ui->centerYValue->setPalette(palette);
    m_ui->centerZValue->setPalette(palette);

    m_ui->centerXValue->setText(QString::number(x * 180 / M_PI));
    m_ui->centerYValue->setText(QString::number(y * 180 / M_PI));
    m_ui->centerZValue->setText(QString::number(z * 180 / M_PI));
}

void SensorCalibration::showStickCalibrationValues(
    bool is_calibrated, double offsetX, double gainX, double offsetY, double gainY)
{
    QPalette palette = m_ui->centerXValue->palette();
    if (is_calibrated)
        palette.setColor(m_ui->centerXValue->foregroundRole(), Qt::black);
    else
        palette.setColor(m_ui->centerXValue->foregroundRole(), Qt::red);
    m_ui->centerXValue->setPalette(palette);
    m_ui->gainXValue->setPalette(palette);
    m_ui->centerYValue->setPalette(palette);
    m_ui->gainYValue->setPalette(palette);

    m_ui->centerXValue->setText(QString::number(offsetX));
    m_ui->gainXValue->setText(QString::number(gainX));
    m_ui->centerYValue->setText(QString::number(offsetY));
    m_ui->gainYValue->setText(QString::number(gainY));
}

void SensorCalibration::hideCalibrationData()
{
    m_ui->xAxisLabel->setVisible(false);
    m_ui->yAxisLabel->setVisible(false);
    m_ui->zAxisLabel->setVisible(false);
    m_ui->centerXLabel->setVisible(false);
    m_ui->centerYLabel->setVisible(false);
    m_ui->centerZLabel->setVisible(false);
    m_ui->centerXValue->setVisible(false);
    m_ui->centerYValue->setVisible(false);
    m_ui->centerZValue->setVisible(false);
    m_ui->gainXLabel->setVisible(false);
    m_ui->gainYLabel->setVisible(false);
    m_ui->gainZLabel->setVisible(false);
    m_ui->gainXValue->setVisible(false);
    m_ui->gainYValue->setVisible(false);
    m_ui->gainZValue->setVisible(false);
    m_ui->steps->clear();
}

void SensorCalibration::selectTypeIndex(unsigned int type_index)
{
    CalibrationType type = static_cast<CalibrationType>(type_index & CAL_TYPE_MASK);
    unsigned int index = (type_index & CAL_INDEX_MASK) >> CAL_INDEX_POS;

    if (m_type == type && m_index == index)
        return;

    disconnect(m_ui->startBtn, &QPushButton::clicked, this, nullptr);
    disconnect(m_ui->resetBtn, &QPushButton::clicked, this, nullptr);
    m_type = type;
    m_index = index;
    hideCalibrationData();

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
            showGyroCalibrationValues(true, data[0], data[1], data[2]);
        } else
        {
            showGyroCalibrationValues(false, 0.0, 0.0, 0.0);
        }

        m_ui->xAxisLabel->setVisible(true);
        m_ui->yAxisLabel->setVisible(true);
        m_ui->zAxisLabel->setVisible(true);
        m_ui->centerXLabel->setVisible(true);
        m_ui->centerYLabel->setVisible(true);
        m_ui->centerZLabel->setVisible(true);
        m_ui->centerXValue->setVisible(true);
        m_ui->centerYValue->setVisible(true);
        m_ui->centerZValue->setVisible(true);

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
    } else if (m_type == CAL_STICK)
    {
        m_ui->statusStack->setCurrentIndex(1);
        m_stick = m_joystick->getActiveSetJoystick()->getSticks().value(m_index);
        m_calibrated = m_stick->isCalibrated();

        if (m_calibrated)
        {
            double data[4] = {0};
            m_stick->getCalibration(data);
            showStickCalibrationValues(true, data[0], data[1], data[2], data[3]);
        } else
        {
            showStickCalibrationValues(false, 0.0, 1.0, 0.0, 1.0);
        }

        m_ui->xAxisLabel->setVisible(true);
        m_ui->yAxisLabel->setVisible(true);
        m_ui->centerXLabel->setVisible(true);
        m_ui->centerYLabel->setVisible(true);
        m_ui->centerXValue->setVisible(true);
        m_ui->centerYValue->setVisible(true);
        m_ui->gainXLabel->setVisible(true);
        m_ui->gainYLabel->setVisible(true);
        m_ui->gainXValue->setVisible(true);
        m_ui->gainYValue->setVisible(true);

        m_ui->resetBtn->setEnabled(m_calibrated);
        m_ui->saveBtn->setEnabled(false);

        m_ui->stickStatusBoxWidget->setFocus();
        m_ui->stickStatusBoxWidget->setStick(m_stick);
        m_ui->stickStatusBoxWidget->update();

        connect(m_ui->startBtn, &QPushButton::clicked,
            this, &SensorCalibration::startStickCenterCalibration);
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
        showGyroCalibrationValues(false, 0, 0, 0);
    } else if (m_type == CAL_STICK && m_stick != nullptr)
    {
        m_stick->resetCalibration();
        m_calibrated = false;

        m_ui->saveBtn->setEnabled(false);
        m_ui->resetBtn->setEnabled(false);
        m_ui->stickStatusBoxWidget->update();
        showStickCalibrationValues(false, 0, 0, 0, 0);

    }
    update();
}

bool SensorCalibration::askConfirmation()
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
    return confirmed;
}

void SensorCalibration::deviceSelectionChanged(int index)
{
    int data = m_ui->deviceComboBox->itemData(index).toInt();
    selectTypeIndex(data);
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

    showGyroCalibrationValues(true, m_mean[0], m_mean[1], m_mean[2]);

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

void SensorCalibration::onStickCenterData(int x, int y)
{
    m_ui->steps->setText(tr(
        "Offset calibration completed. Click next to continue with gain calibration."));
    m_ui->startBtn->setEnabled(true);
    update();
}

void SensorCalibration::onStickGainData(int x, int y)
{
    m_ui->steps->setText("Calibration completed.");
    m_ui->startBtn->setText(tr("Start calibration"));
    m_ui->startBtn->setEnabled(true);
    update();
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
    } else if (m_type == CAL_STICK)
    {
        m_joystick->applyStickCalibration(m_index,
            m_mean[0], m_mean[1], m_mean[2], m_mean[3]);
    }
    m_calibrated = true;
    m_ui->saveBtn->setEnabled(false);
    m_ui->resetBtn->setEnabled(true);
}

/**
 * @brief Prepares first step of calibration - gyroscope center
 * @return nothing
 */
void SensorCalibration::startGyroscopeCalibration()
{
    if(m_sensor == nullptr)
        return;

    if (askConfirmation())
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

void SensorCalibration::startStickCenterCalibration()
{
    if (m_stick == nullptr)
        return;

    if (askConfirmation())
    {
        m_mean[0] = 0;
        m_mean[2] = 0;
        m_var[0] = 0;
        m_var[2] = 0;
        m_sample_count = 0;

        m_stick->resetCalibration();
        m_calibrated = false;

        m_ui->steps->setText(tr(
            "Now move the stick several times to the maximum and back to center."));
        setWindowTitle(tr("Calibrating stick"));
        m_ui->startBtn->setText(tr("Continue calibration"));
        m_ui->startBtn->setEnabled(false);

        m_ui->deviceComboBox->setEnabled(false);
        disconnect(m_ui->startBtn, &QPushButton::clicked, this, nullptr);
        connect(m_ui->startBtn, &QPushButton::clicked, this,
            &SensorCalibration::startStickGainCalibration);
        connect(m_stick, &JoyControlStick::moved,
            this, &SensorCalibration::onStickCenterData);
        update();
    }
}

void SensorCalibration::startStickGainCalibration()
{
    if (m_stick == nullptr)
        return;

    if (askConfirmation())
    {
        m_mean[1] = 0;
        m_mean[3] = 0;
        m_var[1] = 0;
        m_var[3] = 0;
        m_sample_count = 0;

        m_ui->steps->setText(tr(
            "Now move the stick slowly in full circly for several times."));
        m_ui->startBtn->setEnabled(false);

        disconnect(m_ui->startBtn, &QPushButton::clicked, this, nullptr);
        connect(m_ui->startBtn, &QPushButton::clicked, this,
            &SensorCalibration::startStickCenterCalibration);
        connect(m_stick, &JoyControlStick::moved,
            this, &SensorCalibration::onStickGainData);
        update();
    }
}
