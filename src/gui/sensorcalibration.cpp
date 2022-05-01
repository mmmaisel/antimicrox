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

const double SensorCalibration::STICK_CAL_TAU = 0.045;
const int SensorCalibration::STICK_RATE_SAMPLES = 100;
const int SensorCalibration::STICK_THRESHOLD = 5000;

SensorCalibration::SensorCalibration(InputDevice *joystick, QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::SensorCalibration)
    , m_type(CAL_NONE)
    , m_calibrated(false)
    , m_joystick(joystick)
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
    bool offsetCalibrated, bool gainCalibrated,
    double offsetX, double gainX, double offsetY, double gainY)
{
    QPalette paletteCenter = m_ui->centerXValue->palette();
    QPalette paletteGain = m_ui->centerXValue->palette();

    if (offsetCalibrated)
        paletteCenter.setColor(m_ui->centerXValue->foregroundRole(), Qt::black);
    else
        paletteCenter.setColor(m_ui->centerXValue->foregroundRole(), Qt::red);

    if (gainCalibrated)
        paletteGain.setColor(m_ui->centerXValue->foregroundRole(), Qt::black);
    else
        paletteGain.setColor(m_ui->centerXValue->foregroundRole(), Qt::red);

    m_ui->centerXValue->setPalette(paletteCenter);
    m_ui->gainXValue->setPalette(paletteGain);
    m_ui->centerYValue->setPalette(paletteCenter);
    m_ui->gainYValue->setPalette(paletteGain);

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
            showStickCalibrationValues(true, true, data[0], data[1], data[2], data[3]);
        } else
        {
            showStickCalibrationValues(false, false, 0.0, 1.0, 0.0, 1.0);
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
        showStickCalibrationValues(false, false, 0, 0, 0, 0);

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
    m_stats[0].process(x);
    m_stats[1].process(y);
    m_stats[2].process(z);

    showGyroCalibrationValues(
        true, m_stats[0].getMean(), m_stats[1].getMean(), m_stats[2].getMean());

    // Abort when end time is reached to avoid infinite loop
    // in case of noisy sensors.
    if ((m_stats[0].getRelativeErrorSq() < 1e-6
        && m_stats[1].getRelativeErrorSq() < 1e-6
        && m_stats[2].getRelativeErrorSq() < 1e-6 && m_stats[0].getCount() > 10)
        || (QDateTime::currentDateTime() > m_end_time))
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

// XXX: describe calibration algorithm
void SensorCalibration::onStickCenterData(int x, int y)
{
    if (m_phase == 0)
    {
        m_rate_timer.start();
        m_sample_count = 0;
        m_phase = 1;
    } else if (m_phase == 1)
    {
        if (++m_sample_count == STICK_RATE_SAMPLES)
        {
            int delta_t = m_rate_timer.elapsed();
            m_rate_timer.invalidate();
            m_stick_filter[0] = PT1(STICK_CAL_TAU, STICK_RATE_SAMPLES * 1000.0 / delta_t);
            m_stick_filter[1] = PT1(STICK_CAL_TAU, STICK_RATE_SAMPLES * 1000.0 / delta_t);
            m_sample_count = 0;
            m_end_time = QDateTime::currentDateTime().addSecs(10);
            m_phase = 2;
        }
    } else if (m_phase == 2)
    {
        double slopex = m_stick_filter[0].getValue() - m_stick_filter[0].process(x);
        double slopey = m_stick_filter[1].getValue() - m_stick_filter[1].process(y);

        if (((m_last_slope[0] < 0 && slopex > 0) || (m_last_slope[0] > 0 && slopex < 0))
            && abs(x) < STICK_THRESHOLD)
        {
            m_stats[0].process(x);
        }
        if (((m_last_slope[1] < 0 && slopey > 0) || (m_last_slope[1] > 0 && slopey < 0))
            && abs(y) < STICK_THRESHOLD)
        {
            m_stats[2].process(y);
        }

        // there are two events with one value changed each
        if(slopex != 0)
            m_last_slope[0] = slopex;
        if(slopey != 0)
            m_last_slope[1] = slopey;

        showStickCalibrationValues(true, false,
            m_stats[0].getMean(), m_stats[1].getMean(),
            m_stats[2].getMean(), m_stats[3].getMean());

        if ((m_stats[0].getRelativeErrorSq() < 0.01
            && m_stats[2].getRelativeErrorSq() < 0.01 && m_stats[0].getCount() > 10)
            || QDateTime::currentDateTime() > m_end_time)
        {
            m_phase = 3;
        }
    } else if (m_phase == 3)
    {
        disconnect(m_stick, &JoyControlStick::moved,
            this, &SensorCalibration::onStickCenterData);
        m_ui->steps->setText(tr(
            "Offset calibration completed. Click next to continue with gain calibration."));
        m_ui->startBtn->setEnabled(true);
        update();
    }
}

void SensorCalibration::onStickGainData(int x, int y)
{
    double slopex = m_stick_filter[0].getValue() - m_stick_filter[0].process(x);
    double slopey = m_stick_filter[1].getValue() - m_stick_filter[1].process(y);

    // XXX: use different stat processor for min and max ?
    if (m_last_slope[0] > 0 && slopex < 0 && m_stick_filter[0].getValue() < -STICK_THRESHOLD)
    {
        // minimum
        m_stats[1].process(-m_stick_filter[0].getValue());
    } else if (m_last_slope[0] < 0 && slopex > 0 && m_stick_filter[0].getValue() > STICK_THRESHOLD)
    {
        // maximum
        m_stats[1].process(m_stick_filter[0].getValue());
    }

    if (m_last_slope[1] > 0 && slopey < 0 && m_stick_filter[1].getValue() < -STICK_THRESHOLD)
    {
        // minimum
        m_stats[3].process(-m_stick_filter[1].getValue());
    } else if (m_last_slope[1] < 0 && slopey > 0 && m_stick_filter[1].getValue() > STICK_THRESHOLD)
    {
        // maximum
        m_stats[3].process(m_stick_filter[1].getValue());
    }

    // there are two events with one value changed each
    if(slopex != 0)
        m_last_slope[0] = slopex;
    if(slopey != 0)
        m_last_slope[1] = slopey;

    showStickCalibrationValues(true, true,
        m_stats[0].getMean(), GlobalVariables::JoyAxis::AXISMAX / (m_stats[1].getMean()),
        m_stats[2].getMean(), GlobalVariables::JoyAxis::AXISMAX / (m_stats[3].getMean()));

    // XXX: configure cal time with constant?
    if ((m_stats[1].getRelativeErrorSq() < 0.01
        && m_stats[3].getRelativeErrorSq() < 0.01 && m_stats[1].getCount() > 10)
        || QDateTime::currentDateTime() > m_end_time)
    {
        disconnect(m_stick, &JoyControlStick::moved,
            this, &SensorCalibration::onStickGainData);
        disconnect(m_ui->startBtn, &QPushButton::clicked, this, nullptr);
        connect(m_ui->startBtn, &QPushButton::clicked, this,
            &SensorCalibration::startGyroscopeCalibration);
        m_ui->steps->setText("Calibration completed.");
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
            m_stats[0].getMean(), m_stats[1].getMean(), m_stats[2].getMean());
    } else if (m_type == CAL_STICK)
    {
        m_joystick->applyStickCalibration(m_index,
            m_stats[0].getMean(),
            GlobalVariables::JoyAxis::AXISMAX / (m_stats[1].getMean()),
            m_stats[2].getMean(),
            GlobalVariables::JoyAxis::AXISMAX / (m_stats[3].getMean())
        );
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
        m_stats[0].reset();
        m_stats[1].reset();
        m_stats[2].reset();

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
        m_stats[0].reset();
        m_stats[2].reset();
        m_last_slope[0] = 0;
        m_last_slope[1] = 0;

        m_stick->resetCalibration();
        m_calibrated = false;
        m_phase = 0;

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

    m_stats[1].reset();
    m_stats[3].reset();
    m_stick_filter[0].reset();
    m_stick_filter[1].reset();

    m_ui->steps->setText(tr(
        "Now move the stick in full circles for several times."));
    m_ui->startBtn->setEnabled(false);
    update();

    disconnect(m_ui->startBtn, &QPushButton::clicked, this, nullptr);
    connect(m_ui->startBtn, &QPushButton::clicked, this,
        &SensorCalibration::startStickCenterCalibration);
    connect(m_stick, &JoyControlStick::moved,
        this, &SensorCalibration::onStickGainData);
    m_end_time = QDateTime::currentDateTime().addSecs(10);
}
