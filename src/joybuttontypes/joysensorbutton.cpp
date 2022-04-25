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

#include "joysensorbutton.h"

#include "event.h"
#include "globalvariables.h"
#include "joysensor.h"
#include "joybutton.h"
#include "setjoystick.h"
#include "vdpad.h"

#include <cmath>

#include <QDebug>

JoySensorButton::JoySensorButton(JoySensor *sensor,
    int index, int originset, SetJoystick *parentSet, QObject *parent)
    : JoyGradientButton(index, originset, parentSet, parent),
    m_sensor(sensor)
{
    // XXX: these are overwritten by resetAllProperties
    setEasingDuration(0);
    setMouseCurve(LinearCurve);
}

/**
 * @brief Get a 0 indexed number of button
 * @return 0 indexed button index number
 */
int JoySensorButton::getRealJoyNumber() const { return m_index; }

QString JoySensorButton::getPartialName(bool forceFullFormat, bool displayNames) const
{
    QString temp = m_sensor->getPartialName(forceFullFormat, displayNames);
    temp.append(": ");

    if (!buttonName.isEmpty() && displayNames)
    {
        if (forceFullFormat)
            temp.append(tr("Button")).append(" ");
        temp.append(buttonName);
    } else if (!defaultButtonName.isEmpty())
    {
        if (forceFullFormat)
            temp.append(tr("Button")).append(" ");
        temp.append(defaultButtonName);
    } else
    {
        temp.append(tr("Button")).append(" ");
        temp.append(getDirectionName());
    }

    return temp;
}

QString JoySensorButton::getXmlName() { return GlobalVariables::JoySensorButton::xmlName; }

/**
 * @brief Get the distance that an element is away from its assigned
 *     dead zone
 * @return Normalized distance away from dead zone
 */
double JoySensorButton::getDistanceFromDeadZone()
{
    return m_sensor->calculateDirectionalDistance(
        static_cast<JoySensorDirection>(m_index));
}

/**
 * @brief Get the distance factor that should be used for mouse movement
 * @return Distance factor that should be used for mouse movement
 */
double JoySensorButton::getMouseDistanceFromDeadZone()
{
    return m_sensor->calculateDirectionalDistance(
        static_cast<JoySensorDirection>(m_index));
}

/**
 * @brief Check if button should be considered a part of a real controller
 *     axis. Needed for some dialogs so the program won't have to resort to
 *     type checking.
 * @return Status of being part of a real controller axis
 */
bool JoySensorButton::isPartRealAxis() { return false; }

/**
 * @brief Check if button properties are at their default values
 * @return Status of possible property edits
 */
bool JoySensorButton::isDefault()
{
    bool value = true;

    value = value && (getToggleState() == GlobalVariables::JoyButton::DEFAULTTOGGLE);
    value = value && (getTurboInterval() == GlobalVariables::JoyButton::DEFAULTTURBOINTERVAL);
    value = value && (getTurboMode() == NormalTurbo);
    value = value && (isUsingTurbo() == GlobalVariables::JoyButton::DEFAULTUSETURBO);
    value = value && (getMouseSpeedX() == GlobalVariables::JoyButton::DEFAULTMOUSESPEEDX);
    value = value && (getMouseSpeedY() == GlobalVariables::JoyButton::DEFAULTMOUSESPEEDY);
    value = value && (getSetSelection() == GlobalVariables::JoyButton::DEFAULTSETSELECTION);
    value = value && (getChangeSetCondition() == DEFAULTSETCONDITION);
    value = value && (getAssignedSlots()->isEmpty());
    value = value && (getMouseMode() == MouseCursor);
    value = value && (getMouseCurve() == LinearCurve);
    value = value && (getSpringWidth() == GlobalVariables::JoyButton::DEFAULTSPRINGWIDTH);
    value = value && (getSpringHeight() == GlobalVariables::JoyButton::DEFAULTSPRINGHEIGHT);
    value = value && qFuzzyCompare(getSensitivity(), GlobalVariables::JoyButton::DEFAULTSENSITIVITY);
    value = value && (getActionName().isEmpty());
    value = value && (getWheelSpeedX() == GlobalVariables::JoyButton::DEFAULTWHEELX);
    value = value && (getWheelSpeedY() == GlobalVariables::JoyButton::DEFAULTWHEELY);
    value = value && (isCycleResetActive() == GlobalVariables::JoyButton::DEFAULTCYCLERESETACTIVE);
    value = value && (getCycleResetTime() == GlobalVariables::JoyButton::DEFAULTCYCLERESET);
    value = value && (isRelativeSpring() == GlobalVariables::JoyButton::DEFAULTRELATIVESPRING);
    value = value && qFuzzyCompare(getEasingDuration(), GlobalVariables::JoyButton::DEFAULTEASINGDURATION);
    value = value && !isExtraAccelerationEnabled();
    value = value && qFuzzyCompare(getExtraAccelerationMultiplier(), GlobalVariables::JoyButton::DEFAULTEXTRACCELVALUE);
    value = value && qFuzzyCompare(getMinAccelThreshold(), GlobalVariables::JoyButton::DEFAULTMINACCELTHRESHOLD);
    value = value && qFuzzyCompare(getMaxAccelThreshold(), GlobalVariables::JoyButton::DEFAULTMAXACCELTHRESHOLD);
    value = value && qFuzzyCompare(getStartAccelMultiplier(), GlobalVariables::JoyButton::DEFAULTSTARTACCELMULTIPLIER);
    value = value && qFuzzyCompare(getAccelExtraDuration(), GlobalVariables::JoyButton::DEFAULTACCELEASINGDURATION);
    value = value && (getSpringDeadCircleMultiplier() == GlobalVariables::JoyButton::DEFAULTSPRINGRELEASERADIUS);
    value = value && (getExtraAccelerationCurve() == DEFAULTEXTRAACCELCURVE);

    return value;
}

JoySensor *JoySensorButton::getSensor() const { return m_sensor; }

QString JoySensorButton::getDirectionName() const
{
    QString label = QString();

    switch (m_index)
    {
    case JoySensorDirection::ACCEL_UP:
        label.append(tr("Up"));
        break;

    case JoySensorDirection::ACCEL_DOWN:
        label.append(tr("Down"));
        break;

    case JoySensorDirection::ACCEL_LEFT:
        label.append(tr("Left"));
        break;

    case JoySensorDirection::ACCEL_RIGHT:
        label.append(tr("Right"));
        break;

    case JoySensorDirection::ACCEL_FWD:
        label.append(tr("Forward"));
        break;

    case JoySensorDirection::ACCEL_BWD:
        label.append(tr("Backward"));
        break;

    case JoySensorDirection::GYRO_YAW_P:
        label.append(tr("Yaw+"));
        break;

    case JoySensorDirection::GYRO_YAW_N:
        label.append(tr("Yaw-"));
        break;

    case JoySensorDirection::GYRO_ROLL_P:
        label.append(tr("Roll+"));
        break;

    case JoySensorDirection::GYRO_ROLL_N:
        label.append(tr("Roll-"));
        break;

    case JoySensorDirection::GYRO_NICK_P:
        label.append(tr("Nick+"));
        break;

    case JoySensorDirection::GYRO_NICK_N:
        label.append(tr("Nick-"));
        break;
    }

    return label;
}
