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

JoyButton::JoyMouseCurve JoySensorButton::getDefaultMouseCurve() const
{
    return LinearCurve;
}

JoySensor *JoySensorButton::getSensor() const { return m_sensor; }
