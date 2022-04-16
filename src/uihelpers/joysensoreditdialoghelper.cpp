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

#include "joysensoreditdialoghelper.h"

#include "joybuttonslot.h"
#include "joybuttontypes/joysensorbutton.h"

#include <QDebug>

JoySensorEditDialogHelper::JoySensorEditDialogHelper(JoySensor *sensor, QObject *parent)
    : QObject(parent),
    m_sensor(sensor)
{
    Q_ASSERT(m_sensor);
}

void JoySensorEditDialogHelper::setPendingSlots(QHash<JoySensorDirection, JoyButtonSlot *> *tempSlots)
{
    m_pending_slots.clear();

    QHashIterator<JoySensorDirection, JoyButtonSlot *> iter(*tempSlots);
    while (iter.hasNext())
    {
        iter.next();

        JoyButtonSlot *slot = iter.value();
        JoySensorDirection tempDir = iter.key();
        m_pending_slots.insert(tempDir, slot);
    }
}

void JoySensorEditDialogHelper::clearPendingSlots() { m_pending_slots.clear(); }

void JoySensorEditDialogHelper::setFromPendingSlots()
{
    if (!getPendingSlots().isEmpty())
    {
        QHashIterator<JoySensorDirection, JoyButtonSlot *> iter(getPendingSlots());
        while (iter.hasNext())
        {
            iter.next();

            JoyButtonSlot *slot = iter.value();
            if (slot)
            {
                JoySensorDirection tempDir = iter.key();
                JoySensorButton *button = m_sensor->getDirectionButton(tempDir);
                if (button)
                {
                    button->clearSlotsEventReset(false);
                    button->setAssignedSlot(slot->getSlotCode(), slot->getSlotCodeAlias(), slot->getSlotMode());
                }

                slot->deleteLater();
            }
        }
    }
}

void JoySensorEditDialogHelper::clearButtonsSlotsEventReset()
{
    QHash<JoySensorDirection, JoySensorButton *> *buttons = m_sensor->getButtons();
    QHashIterator<JoySensorDirection, JoySensorButton *> iter(*buttons);
    while (iter.hasNext())
    {
        JoySensorButton *button = iter.next().value();
        if (button)
            button->clearSlotsEventReset();
    }
}

void JoySensorEditDialogHelper::updateSensorDelay(int value)
{
    int temp = value * 10;
    if (m_sensor->getSensorDelay() != temp)
        m_sensor->setSensorDelay(temp);
}

QHash<JoySensorDirection, JoyButtonSlot *> const &JoySensorEditDialogHelper::getPendingSlots()
{
    return m_pending_slots;
}
