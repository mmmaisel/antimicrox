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

#include "joysensor.h"

class JoyButtonSlot;

class JoySensorEditDialogHelper : public QObject
{
    Q_OBJECT
  public:
    explicit JoySensorEditDialogHelper(JoySensor *sensor, QObject *parent = nullptr);
    void setPendingSlots(QHash<JoySensorDirection, JoyButtonSlot *> *tempSlots);
    void clearPendingSlots();
    QHash<JoySensorDirection, JoyButtonSlot *> const &getPendingSlots();

  public slots:
    void setFromPendingSlots();
    void clearButtonsSlotsEventReset();
    void updateSensorDelay(int value);

  private:
    JoySensor *m_sensor;
    QHash<JoySensorDirection, JoyButtonSlot *> m_pending_slots;
};
