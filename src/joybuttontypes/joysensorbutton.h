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
#pragma once

#include "joybuttontypes/joygradientbutton.h"
#include "logger.h"

class VDPad;
class SetJoystick;
class JoySensor;

class JoySensorButton : public JoyGradientButton
{
    Q_OBJECT

  public:
    explicit JoySensorButton(JoySensor *sensor, int index, int originset,
        SetJoystick *parentSet, QObject *parent);

    virtual int getRealJoyNumber() const;
    virtual QString getPartialName(
        bool forceFullFormat = false, bool displayNames = false) const;
    virtual QString getXmlName();

    virtual double getDistanceFromDeadZone();
    virtual double getMouseDistanceFromDeadZone();

    virtual bool isPartRealAxis();
    virtual bool isDefault();

    JoySensor *getSensor() const;
    QString getDirectionName() const;

  signals:
    void setAssignmentChanged(
        int current_button, int axis_index, int associated_set, int mode);

  private:
    JoySensor *m_sensor;
};
