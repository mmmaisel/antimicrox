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
#include "inputdevice.h"
#include "joybuttontypes/joyaccelerometerbutton.h"
#include "joybuttontypes/joygyroscopebutton.h"
#include "xml/joybuttonxml.h"

#include <QDebug>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <math.h>

const double JoySensor::SHOCK_DETECT_THRESHOLD = 20.0;
const double JoySensor::SHOCK_SUPPRESS_FACTOR = 0.5;
const double JoySensor::SHOCK_TAU = 10;

JoySensor::JoySensor(
    Type type, double rate, int originset, SetJoystick *parent_set, QObject *parent)
    : QObject(parent),
    m_type(type),
    m_originset(originset),
    m_calibrated(false),
    m_shock_filter(SHOCK_TAU, rate),
    m_parent_set(parent_set)
{
    m_rate = qFuzzyIsNull(rate) ? PT1::FALLBACK_RATE : rate;
    reset();
    m_delay_timer.setSingleShot(true);
    populateButtons();

    connect(&m_delay_timer, &QTimer::timeout, this, &JoySensor::delayTimerExpired);
}

JoySensor::~JoySensor()
{
}

void JoySensor::joyEvent(float* values, bool ignoresets)
{
    m_current_value[0] = values[0];
    m_current_value[1] = values[1];
    m_current_value[2] = values[2];

    if (m_calibrated)
    {
        m_current_value[0] -= m_calibration_value[0];
        m_current_value[1] -= m_calibration_value[1];
        m_current_value[2] -= m_calibration_value[2];
    }

    JoySensorDirection pending_direction = calculateSensorDirection();

    if (pending_direction != SENSOR_CENTERED && !m_active)
    {
        m_active = true;
        emit active(m_current_value[0], m_current_value[1], m_current_value[2]);

        if (ignoresets || (m_sensor_delay == 0))
        {
            if (m_delay_timer.isActive())
                m_delay_timer.stop();
            createDeskEvent(pending_direction, ignoresets);
        } else
        {
            if (!m_delay_timer.isActive())
                m_delay_timer.start(m_sensor_delay);
        }
    } else if (pending_direction == SENSOR_CENTERED && m_active)
    {
        m_active = false;
        emit released(m_current_value[0], m_current_value[1], m_current_value[2]);

        if (ignoresets || (m_sensor_delay == 0))
        {
            if (m_delay_timer.isActive())
                m_delay_timer.stop();
            createDeskEvent(pending_direction, ignoresets);
        } else
        {
            if (!m_delay_timer.isActive())
                m_delay_timer.start(m_sensor_delay);
        }
    } else if (m_active)
    {
        if (ignoresets || (m_sensor_delay == 0))
        {
            if (m_delay_timer.isActive())
                m_delay_timer.stop();

            createDeskEvent(pending_direction, ignoresets);
        } else
        {
            if (m_current_direction != pending_direction)
            {
                if (!m_delay_timer.isActive())
                    m_delay_timer.start(m_sensor_delay);
            } else
            {
                if (m_delay_timer.isActive())
                    m_delay_timer.stop();

                createDeskEvent(pending_direction, ignoresets);
            }
        }
    }

    emit moved(m_current_value[0], m_current_value[1], m_current_value[2]);
}

void JoySensor::queuePendingEvent(float* values, bool ignoresets)
{
    m_pending_event = true;
    m_pending_value[0] = values[0];
    m_pending_value[1] = values[1];
    m_pending_value[2] = values[2];
    m_pending_ignore_sets = ignoresets;
}

void JoySensor::activatePendingEvent()
{
    if (!m_pending_event)
        return;

    joyEvent(m_pending_value, m_pending_ignore_sets);

    clearPendingEvent();
}

bool JoySensor::hasPendingEvent() { return m_pending_event; }

void JoySensor::clearPendingEvent()
{
    m_pending_event = false;
    m_pending_ignore_sets = false;
}

bool JoySensor::hasSlotsAssigned()
{
    for (auto iter = m_buttons.cbegin(); iter != m_buttons.cend(); ++iter)
    {
        JoyButton *button = iter.value();
        if (button != nullptr)
        {
            if (button->getAssignedSlots()->count() > 0)
                return true;
        }
    }
    return false;
}

QString JoySensor::getName(bool forceFullFormat, bool displayNames)
{
    QString label = getPartialName(forceFullFormat, displayNames);
    label.append(": ");
    return label;
}

QString JoySensor::getPartialName(bool forceFullFormat, bool displayNames)
{
    QString label = QString();

    if (!m_sensor_name.isEmpty() && displayNames)
    {
        if (forceFullFormat)
        {
            label.append(sensorTypeName()).append(" ");
        }

        label.append(m_sensor_name);
    } else if (!m_default_sensor_name.isEmpty())
    {
        if (forceFullFormat)
        {
            label.append(sensorTypeName()).append(" ");
        }

        label.append(m_default_sensor_name);
    } else
    {
        label.append(sensorTypeName()).append(" ");
    }

    return label;
}

JoySensorDirection JoySensor::getCurrentDirection()
{
    return m_current_direction;
}

JoySensor::Type JoySensor::getType() { return m_type; }

/**
 * @brief Get the assigned dead zone value.
 * @return Assigned dead zone value
 */
float JoySensor::getDeadZone()
{
    return m_dead_zone * 180 / M_PI;
}

/**
 * @brief Get the assigned diagonal range value.
 * @return Assigned diagonal range.
 */
float JoySensor::getDiagonalRange()
{
    return m_diagonal_range * 180 / M_PI;
}

float JoySensor::getMaxZone()
{
    return m_max_zone * 180 / M_PI;
}

/**
 * @brief Get the value for the corresponding X axis.
 * @return X axis value in m/s^2 for accelerometer or rad/s for gyroscope.
 */
float JoySensor::getXCoordinate() { return m_current_value[0]; }

/**
 * @brief Get the value for the corresponding Y axis.
 * @return X axis value in m/s^2 for accelerometer or rad/s for gyroscope.
 */
float JoySensor::getYCoordinate() { return m_current_value[1]; }

/**
 * @brief Get the value for the corresponding Z axis.
 * @return Z axis value in m/s^2 for accelerometer or rad/s for gyroscope.
 */
float JoySensor::getZCoordinate() { return m_current_value[2]; }

unsigned int JoySensor::getSensorDelay() { return m_sensor_delay; }

QString JoySensor::sensorTypeName() const {
    if (m_type == ACCELEROMETER)
        return tr("Accelerometer");
    else
        return tr("Gyroscope");
}

/**
 * @brief Reset all the properties of the sensor direction buttons.
 */
void JoySensor::resetButtons()
{
    for (auto iter = m_buttons.cbegin(); iter != m_buttons.cend(); ++iter)
    {
        JoyButton *button = iter.value();

        if (button != nullptr)
            button->reset();
    }
}

bool JoySensor::inDeadZone(float* values) const
{
    return calculateDistance(values[0], values[1], values[2]) < m_dead_zone;
}

/**
 * @brief Get current radial distance of the sensor past the assigned
 *   dead zone.
 * @return Distance between 0 and max zone in radiants.
 */
double JoySensor::getDistanceFromDeadZone() const
{
    return getDistanceFromDeadZone(
        m_current_value[0],
        m_current_value[1],
        m_current_value[2]
    );
}

/**
 * @brief Get radial distance of the sensor past the assigned dead zone
 *   based on the passed X, Y and Z axes values associated with the sensor.
 * @param X axis value
 * @param Y axis value
 * @param Z axis value
 * @return Distance between 0 and max zone in radiants.
 */
double JoySensor::getDistanceFromDeadZone(float x, float y, float z) const
{
    double distance = calculateDistance(x, y, z);
    return qBound(0.0, distance - m_dead_zone, double(m_max_zone));
}

/**
 * @brief Get current X distance of the sensor past the assigned
 *   dead zone.
 * @return Distance between 0 and max zone in radiants.
 */
double JoySensor::calculateXDistanceFromDeadZone() const
{
    return calculateXDistanceFromDeadZone(
        m_current_value[0],
        m_current_value[1],
        m_current_value[2]
    );
}

/**
 * @brief Get current X distance of the sensor past the assigned
 *   dead zone based on the passed X, Y and Z axes values associated
 *   with the sensor. The algorithm checks if axis parallel line
 *   through the current sensor position intersects with the dead zone
 *   sphere and subtracts the line segment within the sphere from the
 *   distance before normalization.
 * @param X axis value
 * @param Y axis value
 * @param Z axis value
 * @return Distance between 0 and max zone in radiants.
 */
double JoySensor::calculateXDistanceFromDeadZone(
    float x, float y, float z) const
{
    double discriminant = m_dead_zone*m_dead_zone - y*y - z*z;
    if(discriminant <= 0)
        return std::min(abs(x), m_max_zone);
    else
        return std::min(abs(x)-sqrt(discriminant), double(m_max_zone));
}

/**
 * @brief Get current Y distance of the sensor past the assigned
 *   dead zone.
 * @return Distance between 0 and max zone in radiants.
 */
double JoySensor::calculateYDistanceFromDeadZone() const
{
    return calculateYDistanceFromDeadZone(
        m_current_value[0],
        m_current_value[1],
        m_current_value[2]
    );
}

/**
 * @brief Get current Y distance of the sensor past the assigned
 *   dead zone based on the passed X, Y and Z axes values associated
 *   with the sensor. The algorithm checks if axis parallel line
 *   through the current sensor position intersects with the dead zone
 *   sphere and subtracts the line segment within the sphere from the
 *   distance before normalization.
 * @param X axis value
 * @param Y axis value
 * @param Z axis value
 * @return Distance between 0 and max zone in radiants.
 */
double JoySensor::calculateYDistanceFromDeadZone(
    float x, float y, float z) const
{
    double discriminant = m_dead_zone*m_dead_zone - x*x - z*z;
    if(discriminant <= 0)
        return std::min(abs(y), m_max_zone);
    else
        return std::min(abs(y)-sqrt(discriminant), double(m_max_zone));
}

/**
 * @brief Get current Z distance of the sensor past the assigned
 *   dead zone.
 * @return Distance between 0 and max zone in radiants.
 */
double JoySensor::calculateZDistanceFromDeadZone() const
{
    return calculateZDistanceFromDeadZone(
        m_current_value[0],
        m_current_value[1],
        m_current_value[2]
    );
}

/**
 * @brief Get current Z distance of the sensor past the assigned
 *   dead zone based on the passed X, Y and Z axes values associated
 *   with the sensor. The algorithm checks if axis parallel line
 *   through the current sensor position intersects with the dead zone
 *   sphere and subtracts the line segment within the sphere from the
 *   distance before normalization.
 * @param X axis value
 * @param Y axis value
 * @param Z axis value
 * @return Distance between 0 and max zone in radiants.
 */
double JoySensor::calculateZDistanceFromDeadZone(
    float x, float y, float z) const
{
    double discriminant = m_dead_zone*m_dead_zone - x*x - y*y;
    if(discriminant <= 0)
        return std::min(abs(z), m_max_zone);
    else
        return std::min(abs(z)-sqrt(discriminant), double(m_max_zone));
}

/**
 * @brief Get the vector length of the sensor.
 * @return Length.
 */
double JoySensor::calculateDistance() const
{
    return calculateDistance(
        m_current_value[0],
        m_current_value[1],
        m_current_value[2]
    );
}

double JoySensor::calculateDistance(float x, float y, float z) const
{
    return sqrt(x*x + y*y + z*z);
}

/**
 * @brief Calculate the pitch angle (in degrees) corresponding to the current
 *   position of controller.
 * @return Pitch (in degrees)
 */
double JoySensor::calculatePitch() const
{
    return calculatePitch(
        m_current_value[0],
        m_current_value[1],
        m_current_value[2]
    );
}

/**
 * @brief Calculate the pitch angle (in degrees) corresponding to the current
 *   passed X, Y and Z axes values associated with the sensor.
 *   position of controller.
 *   See https://www.nxp.com/files-static/sensors/doc/app_note/AN3461.pdf
 *   for a description of the used algorithm.
 * @param X axis value
 * @param Y axis value
 * @param Z axis value
 * @return Pitch (in degrees)
 */
double JoySensor::calculatePitch(float x, float y, float z) const
{
    double rad = calculateDistance(x, y, z);
    double pitch = -atan2(z/rad, y/rad) - M_PI/2;
    if (pitch < -M_PI)
        pitch += 2*M_PI;
    return pitch;
}

/**
 * @brief Calculate the roll angle (in degrees) corresponding to the current
 *   position of controller.
 * @return Roll (in degrees)
 */
double JoySensor::calculateRoll() const
{
    return calculateRoll(
        m_current_value[0],
        m_current_value[1],
        m_current_value[2]
    );
}

/**
 * @brief Calculate the roll angle (in degrees) corresponding to the current
 *   passed X, Y and Z axes values associated with the sensor.
 *   position of controller.
 *   See https://www.nxp.com/files-static/sensors/doc/app_note/AN3461.pdf
 *   for a description of the used algorithm.
 * @param X axis value
 * @param Y axis value
 * @param Z axis value
 * @return Roll (in degrees)
 */
double JoySensor::calculateRoll(float x, float y, float z) const
{
    double rad = calculateDistance(x, y, z);

    double xp, yp, zp;
    xp = x/rad; yp = y/rad; zp = z/rad;
    double roll = atan2(sqrt(yp*yp+zp*zp),-xp) - M_PI/2;
    if (roll < -M_PI)
        roll += 2*M_PI;
    return roll;
}

/**
 * @brief Used to calculate the distance value that should be used by
 *   the JoyButton in the given direction.
 * @param direction
 * @return Distance factor that should be used by JoyButton
 */
double JoySensor::calculateDirectionalDistance(
    JoySensorDirection direction) const
{
    double finalDistance = 0.0;

    switch (direction)
    {
    case JoySensorDirection::SENSOR_LEFT:
    case JoySensorDirection::SENSOR_RIGHT:
        // Yaw
        finalDistance = calculateZDistanceFromDeadZone();
        break;
    case JoySensorDirection::SENSOR_UP:
    case JoySensorDirection::SENSOR_DOWN:
        // Pitch
        finalDistance = calculateXDistanceFromDeadZone();
        break;
    case JoySensorDirection::SENSOR_FWD:
    case JoySensorDirection::SENSOR_BWD:
        // Roll
        finalDistance = calculateYDistanceFromDeadZone();
        break;
    default:
        break;
    }

    return finalDistance;
}

bool JoySensor::isCalibrated() const
{
    return m_calibrated;
}

void JoySensor::resetCalibration()
{
    m_calibrated = false;
}

void JoySensor::getCalibration(float* data)
{
    data[0] = m_calibration_value[0];
    data[1] = m_calibration_value[1];
    data[2] = m_calibration_value[2];
}

void JoySensor::setCalibration(float x0, float y0, float z0)
{
    m_calibration_value[0] = x0;
    m_calibration_value[1] = y0;
    m_calibration_value[2] = z0;
    m_calibrated = true;
}

QHash<JoySensorDirection, JoySensorButton *> *JoySensor::getButtons()
{
    return &m_buttons;
}

/**
 * @brief Get a pointer to the sensor direction button for the desired
 *     direction.
 * @param Value of the direction of the sensor.
 * @return Pointer to the sensor direction button for the sensor
 *     direction.
 */
JoySensorButton *JoySensor::getDirectionButton(JoySensorDirection direction)
{
    return m_buttons.value(direction);
}

void JoySensor::setSensorName(QString tempName)
{
    if ((tempName.length() <= 20) && (tempName != m_sensor_name))
    {
        m_sensor_name = tempName;
        emit sensorNameChanged();
    }
}

QString JoySensor::getSensorName() { return m_sensor_name; }

bool JoySensor::isDefault()
{
    bool value = true;
    value = value && qFuzzyCompare(double(getDeadZone()), GlobalVariables::JoySensor::DEFAULTDEADZONE);
    if (m_type == ACCELEROMETER)
        value = value && qFuzzyCompare(double(getMaxZone()), GlobalVariables::JoySensor::ACCEL_MAX);
    else
        value = value && qFuzzyCompare(double(getMaxZone()), GlobalVariables::JoySensor::GYRO_MAX);

    value = value && qFuzzyCompare(getDiagonalRange(), GlobalVariables::JoySensor::DEFAULTDIAGONALRANGE);
    value = value && (m_sensor_delay == GlobalVariables::JoySensor::DEFAULTSENSORDELAY);

    for (auto iter = m_buttons.cbegin(); iter != m_buttons.cend(); ++iter)
    {
        JoySensorButton *button = iter.value();
        value = value && (button->isDefault());
    }

    return value;
}
void JoySensor::setDefaultSensorName(QString tempname) { m_default_sensor_name = tempname; }

QString JoySensor::getDefaultSensorName() { return m_default_sensor_name; }

/**
 * @brief Take a XML stream and set the sensor and direction button properties
 *     according to the values contained within the stream.
 * @param QXmlStreamReader instance that will be used to read property values.
 */
void JoySensor::readConfig(QXmlStreamReader *xml)
{
    if (xml->isStartElement() && (xml->name() == "sensor"))
    {
        xml->readNextStartElement();

        while (!xml->atEnd() && (!xml->isEndElement() && (xml->name() != "sensor")))
        {
            if ((xml->name() == "deadZone") && xml->isStartElement())
            {
                QString temptext = xml->readElementText();
                float tempchoice = temptext.toFloat();
                setDeadZone(tempchoice);
            } else if ((xml->name() == "maxZone") && xml->isStartElement())
            {
                QString temptext = xml->readElementText();
                float tempchoice = temptext.toFloat();
                setMaxZone(tempchoice);
            } else if ((xml->name() == "diagonalRange") && xml->isStartElement())
            {
                QString temptext = xml->readElementText();
                int tempchoice = temptext.toInt();
                setDiagonalRange(tempchoice);
            } else if ((xml->name() == GlobalVariables::JoySensorButton::xmlName)
                && xml->isStartElement())
            {
                int index = xml->attributes().value("index").toString().toInt();
                JoySensorButton *button =
                    m_buttons.value(static_cast<JoySensorDirection>(index));
                QPointer<JoyButtonXml> joyButtonXml = new JoyButtonXml(button);

                if (button != nullptr)
                    joyButtonXml->readConfig(xml);
                else
                    xml->skipCurrentElement();

                if (!joyButtonXml.isNull())
                    delete joyButtonXml;
            } else if ((xml->name() == "sensorDelay") && xml->isStartElement())
            {
                QString temptext = xml->readElementText();
                int tempchoice = temptext.toInt();
                setSensorDelay(tempchoice);
            } else
            {
                xml->skipCurrentElement();
            }

            xml->readNextStartElement();
        }
    }
}

/**
 * @brief Write the status of the properties of a sensor and direction buttons
 *     to an XML stream.
 * @param QXmlStreamWriter instance that will be used to write a profile.
 */
void JoySensor::writeConfig(QXmlStreamWriter *xml)
{
    if (!isDefault())
    {
        xml->writeStartElement("sensor");
        xml->writeAttribute("type", QString::number(m_type));

        if (!qFuzzyCompare(double(getDeadZone()), GlobalVariables::JoySensor::DEFAULTDEADZONE))
            xml->writeTextElement("deadZone", QString::number(getDeadZone()));

        if (!qFuzzyCompare(double(m_max_zone), (m_type == ACCELEROMETER
                ? GlobalVariables::JoySensor::ACCEL_MAX
                : GlobalVariables::JoySensor::GYRO_MAX)))
            xml->writeTextElement("maxZone", QString::number(getMaxZone()));

        if (!qFuzzyCompare(double(m_diagonal_range), GlobalVariables::JoySensor::DEFAULTDIAGONALRANGE))
            xml->writeTextElement("diagonalRange", QString::number(getDiagonalRange()));

        if (m_sensor_delay > GlobalVariables::JoySensor::DEFAULTSENSORDELAY)
            xml->writeTextElement("sensorDelay", QString::number(m_sensor_delay));

        for (auto iter = m_buttons.cbegin(); iter != m_buttons.cend(); ++iter)
        {
            JoySensorButton *button = iter.value();
            JoyButtonXml *joyButtonXml = new JoyButtonXml(button);
            joyButtonXml->writeConfig(xml);
            delete joyButtonXml;
            joyButtonXml = nullptr;
        }

        xml->writeEndElement();
    }
}

/**
 * @brief Get pointer to the set that a sensor belongs to.
 * @return Pointer to the set that a sensor belongs to.
 */
SetJoystick *JoySensor::getParentSet()
{
    return m_parent_set;
}

void JoySensor::reset()
{
    m_active = false;
    for (size_t i = 0; i < ACTIVE_BUTTON_COUNT; ++i)
        m_active_button[i] = nullptr;
    m_dead_zone = GlobalVariables::JoySensor::DEFAULTDEADZONE * M_PI / 180;
    m_max_zone = m_type == ACCELEROMETER
        ? GlobalVariables::JoySensor::ACCEL_MAX * M_PI / 180
        : GlobalVariables::JoySensor::GYRO_MAX * M_PI / 180;
    m_diagonal_range = GlobalVariables::JoySensor::DEFAULTDIAGONALRANGE * M_PI / 180;
    m_pending_event = false;
    m_shock_filter.reset();
    m_shock_suppress_count = 0;

    m_current_direction = JoySensorDirection::SENSOR_CENTERED;
    m_sensor_name.clear();
    m_sensor_delay = GlobalVariables::JoySensor::DEFAULTSENSORDELAY;

    resetButtons();
}

void JoySensor::setDeadZone(float value)
{
    value = abs(value / 180 * M_PI);
    if (!qFuzzyCompare(value, m_dead_zone) && (value <= m_max_zone))
    {
        m_dead_zone = value;
        emit deadZoneChanged(value);
        emit propertyUpdated();
    }
}

void JoySensor::setMaxZone(float value)
{
    value = abs(value / 180 * M_PI);
    if (!qFuzzyCompare(value, m_max_zone) && (value > m_dead_zone))
    {
        m_max_zone = value;
        emit maxZoneChanged(value);
        emit propertyUpdated();
    }
}

/**
 * @brief Set the diagonal range value for a sensor.
 * @param Value between 1 - 90.
 */
void JoySensor::setDiagonalRange(float value)
{
    if (value < 1)
        value = 1;
    else if (value > 90)
        value = 90;

    value = value * M_PI / 180;
    if (!qFuzzyCompare(value, m_diagonal_range))
    {
        m_diagonal_range = value;
        emit diagonalRangeChanged(value);
        emit propertyUpdated();
    }
}

void JoySensor::setSensorDelay(unsigned int value)
{
    if (((value >= 10) && (value <= 1000)) || (value == 0))
    {
        m_sensor_delay = value;
        emit sensorDelayChanged(value);
        emit propertyUpdated();
    }
}

void JoySensor::establishPropertyUpdatedConnection()
{
    connect(this, &JoySensor::propertyUpdated, getParentSet()->getInputDevice(),
        &InputDevice::profileEdited);
}

/**
 * @brief Slot called when m_delay_timer has timed out. The method will
 *     call createDeskEvent.
 */
void JoySensor::delayTimerExpired()
{
    createDeskEvent(calculateSensorDirection());
}

/**
 * @brief Set buttons for current sensor direction zone.
 *
 * @param Pointer to an array of three JoySensorButton pointers in which
 *   the results are stored.
 */
void JoySensor::determineSensorEvent(JoySensorButton **eventbutton)
{
    if (m_current_direction & SENSOR_LEFT)
        eventbutton[0] = m_buttons.value(SENSOR_LEFT);
    else if (m_current_direction & SENSOR_RIGHT)
        eventbutton[0] = m_buttons.value(SENSOR_RIGHT);

    if (m_current_direction & SENSOR_UP)
        eventbutton[1] = m_buttons.value(SENSOR_UP);
    else if (m_current_direction & SENSOR_DOWN)
        eventbutton[1] = m_buttons.value(SENSOR_DOWN);

    if (m_current_direction & SENSOR_FWD)
        eventbutton[2] = m_buttons.value(SENSOR_FWD);
    else if (m_current_direction & SENSOR_BWD)
        eventbutton[2] = m_buttons.value(SENSOR_BWD);
}

JoySensorDirection JoySensor::calculateSensorDirection()
{
    if (m_type == ACCELEROMETER)
        return calculateAccelerometerDirection();
    else
        return calculateGyroscopeDirection();
}

/**
 * @brief Find the direction zone of the current sensor position.
 *
 *   First, the pitch and roll angles on the unit sphere are calculated.
 *   Then, the unit sphere is divided into direction zones with the following algorithm:
 *     - Mark a spherical layer around the X axis at +/- the diagonal zone angle
 *       divided by two (called "range" in the code)
 *     - Generate another spherical layers by rotating the first layer around the Y axis.
 *       A third layer is not necessary because there are only two degrees of freedom.
 *   Check if a point is within each layer by comparing the absolute values
 *   of pitch and roll angles against the "range".
 *   If a point is in only one layer, it is in the orthogonal zone of one axis.
 *   If a point is in both or no zones, it is diagonal to both axes.
 *   There are two cases here because the spherical layers overlap if the diagonal
 *   angle is larger then 45 degree.
 *
 *   Perform shock detection by taking the first order lag filtered absolute sum of
 *   all axes from "joyEvent" and apply a threshold. Discard some samples after
 *   the shock is over to avoid spurious pitch/roll events.
 */
JoySensorDirection JoySensor::calculateAccelerometerDirection()
{
    if (m_shock_filter.getValue() > SHOCK_DETECT_THRESHOLD)
    {
        m_shock_suppress_count = m_rate * SHOCK_SUPPRESS_FACTOR;
        return SENSOR_FWD;
    } else if (m_shock_suppress_count != 0)
    {
        --m_shock_suppress_count;
        return SENSOR_CENTERED;
    }

    double pitch = calculatePitch();
    double roll = calculateRoll();
    double pitch_abs = abs(pitch);
    double roll_abs = abs(roll);
    if (pitch_abs*pitch_abs + roll_abs*roll_abs < m_dead_zone*m_dead_zone)
        return SENSOR_CENTERED;

    double range = M_PI/4 - m_diagonal_range/2;
    bool inPitch = pitch_abs < range;
    bool inRoll = roll_abs < range;

    if (inPitch && !inRoll)
    {
        if (roll > 0)
            return SENSOR_LEFT;
        else
            return SENSOR_RIGHT;
    } else if (!inPitch && inRoll)
    {
        if (pitch > 0)
            return SENSOR_UP;
        else
            return SENSOR_DOWN;
    } else // in both or in none
    {
        if (pitch > 0)
        {
            if (roll > 0)
                return SENSOR_LEFT_UP;
            else
                return SENSOR_RIGHT_UP;
        } else
        {
            if (roll > 0)
                return SENSOR_LEFT_DOWN;
            else
                return SENSOR_RIGHT_DOWN;
        }
    }
}

/**
 * @brief Find the direction zone of the current sensor position.
 *
 *   First, the sensor axis values are normalized so they are on the unit sphere.
 *   Then, the unit sphere is divided into direction zones with the following algorithm:
 *     - Mark a spherical layer around the X axis at +/- the diagonal zone angle
 *       divided by two (called "range" in the code)
 *       Then generate two more spherical layers by rotating the
 *       first layer around the Y and Z axes.
 *   Check if a point is within each layer by comparing the absolute value
 *   of each coordinate against the "range".
 *   If a point is in only one layer, it is in the diagonal zone between two axes.
 *   If a point is in two layers, it is in the orthogonal zone of one axis.
 *   If a point is in three or zero zones, it is diagonal to all three axes.
 *   There are two cases here because the spherical layers overlap if the diagonal
 *   angle is larger then 45 degree.
 */
JoySensorDirection JoySensor::calculateGyroscopeDirection()
{
    double distance = calculateDistance();
    if (distance < m_dead_zone)
        return SENSOR_CENTERED;

    double range = sin(M_PI/4 - m_diagonal_range/2);
    double normPitch = m_current_value[0] / distance;
    double normRoll = m_current_value[1] / distance;
    double normYaw = m_current_value[2] / distance;

    bool inPitch = abs(normPitch) < range;
    bool inRoll = abs(normRoll) < range;
    bool inYaw = abs(normYaw) < range;

    if (inPitch && !inRoll && !inYaw)
    {
        if (normRoll > 0)
        {
            if (normYaw > 0)
                return SENSOR_RIGHT_FWD;
            else
                return SENSOR_LEFT_FWD;
        } else
        {
            if (normYaw > 0)
                return SENSOR_RIGHT_BWD;
            else
                return SENSOR_LEFT_BWD;
        }
    } else if (!inPitch && inRoll && !inYaw)
    {
        if (normPitch > 0)
        {
            if (normYaw > 0)
                return SENSOR_RIGHT_UP;
            else
                return SENSOR_LEFT_UP;
        } else
        {
            if (normYaw > 0)
                return SENSOR_RIGHT_DOWN;
            else
                return SENSOR_LEFT_DOWN;
        }
    } else if (!inPitch && !inRoll && inYaw)
    {
        if (normPitch > 0)
        {
            if (normRoll > 0)
                return SENSOR_UP_FWD;
            else
                return SENSOR_UP_BWD;
        } else
        {
            if (normRoll > 0)
                return SENSOR_DOWN_FWD;
            else
                return SENSOR_DOWN_BWD;
        }
    } else if (inPitch && inRoll && !inYaw)
    {
        if (normYaw > 0)
            return SENSOR_RIGHT;
        else
            return SENSOR_LEFT;
    } else if (inPitch && !inRoll && inYaw)
    {
        if (normRoll > 0)
            return SENSOR_FWD;
        else
            return SENSOR_BWD;
    } else if (!inPitch && inRoll && inYaw)
    {
        if (normPitch > 0)
            return SENSOR_UP;
        else
            return SENSOR_DOWN;
    } else // in all or in none
    {
        if (normPitch > 0)
        {
            if (normRoll > 0)
            {
                if (normYaw > 0)
                    return SENSOR_RIGHT_UP_FWD;
                else
                    return SENSOR_LEFT_UP_FWD;
            } else
            {
                if (normYaw > 0)
                    return SENSOR_RIGHT_UP_BWD;
                else
                    return SENSOR_LEFT_UP_BWD;
            }
        } else
        {
            if (normRoll > 0)
            {
                if (normYaw > 0)
                    return SENSOR_RIGHT_DOWN_FWD;
                else
                    return SENSOR_LEFT_DOWN_FWD;
            } else
            {
                if (normYaw > 0)
                    return SENSOR_RIGHT_DOWN_BWD;
                else
                    return SENSOR_LEFT_DOWN_BWD;
            }
        }
    }
}

/**
 * @brief Find the position of the three sensor axes, deactivate no longer used
 *   sensor direction button and then activate direction buttons for new
 *   direction.
 * @param Should set changing operations be ignored. Necessary in the middle
 *   of a set change.
 */
void JoySensor::createDeskEvent(JoySensorDirection direction, bool ignoresets)
{
    m_current_direction = direction;
    JoySensorButton *eventbutton[ACTIVE_BUTTON_COUNT] = {nullptr};
    determineSensorEvent(eventbutton);

    for (size_t i = 0; i < ACTIVE_BUTTON_COUNT; ++i)
    {
        if (m_active_button[i] != nullptr && m_active_button[i] != eventbutton[i])
        {
            m_active_button[i]->joyEvent(false, ignoresets);
            m_active_button[i] = nullptr;
        }

        if (eventbutton[i] != nullptr && m_active_button[i] == nullptr)
        {
            m_active_button[i] = eventbutton[i];
            m_active_button[i]->joyEvent(true, ignoresets);
        } else if (eventbutton[i] == nullptr && m_active_button[i] != nullptr)
        {
            m_active_button[i]->joyEvent(false, ignoresets);
            m_active_button[i] = nullptr;
        }
    }
}

void JoySensor::populateButtons()
{
    JoySensorButton *button = nullptr;
    if (m_type == ACCELEROMETER)
    {
        button = new JoyAccelerometerButton(this, SENSOR_LEFT, m_originset, getParentSet(), this);
        m_buttons.insert(SENSOR_LEFT, button);

        button = new JoyAccelerometerButton(this, SENSOR_RIGHT, m_originset, getParentSet(), this);
        m_buttons.insert(SENSOR_RIGHT, button);

        button = new JoyAccelerometerButton(this, SENSOR_UP, m_originset, getParentSet(), this);
        m_buttons.insert(SENSOR_UP, button);

        button = new JoyAccelerometerButton(this, SENSOR_DOWN, m_originset, getParentSet(), this);
        m_buttons.insert(SENSOR_DOWN, button);

        button = new JoyAccelerometerButton(this, SENSOR_FWD, m_originset, getParentSet(), this);
        m_buttons.insert(SENSOR_FWD, button);
    } else
    {
        button = new JoyGyroscopeButton(this, SENSOR_LEFT, m_originset, getParentSet(), this);
        m_buttons.insert(SENSOR_LEFT, button);

        button = new JoyGyroscopeButton(this, SENSOR_RIGHT, m_originset, getParentSet(), this);
        m_buttons.insert(SENSOR_RIGHT, button);

        button = new JoyGyroscopeButton(this, SENSOR_UP, m_originset, getParentSet(), this);
        m_buttons.insert(SENSOR_UP, button);

        button = new JoyGyroscopeButton(this, SENSOR_DOWN, m_originset, getParentSet(), this);
        m_buttons.insert(SENSOR_DOWN, button);

        button = new JoyGyroscopeButton(this, SENSOR_FWD, m_originset, getParentSet(), this);
        m_buttons.insert(SENSOR_FWD, button);

        button = new JoyGyroscopeButton(this, SENSOR_BWD, m_originset, getParentSet(), this);
        m_buttons.insert(SENSOR_BWD, button);
    }
}
