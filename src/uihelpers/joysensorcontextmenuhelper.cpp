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

#include "joysensorcontextmenuhelper.h"

#include "joybuttonslot.h"
#include "joybuttontypes/joysensorbutton.h"

#include <QDebug>
#include <QHashIterator>

JoySensorContextMenuHelper::JoySensorContextMenuHelper(JoySensor *sensor, QObject *parent)
    : QObject(parent)
    , m_sensor(sensor)
{
    Q_ASSERT(m_sensor);
}

void JoySensorContextMenuHelper::setPendingSlots(
    QHash<JoySensorDirection, JoyButtonSlot *> *tempSlots)
{
    m_pending_slots.clear();
    for(auto iter = tempSlots->cbegin(); iter != tempSlots->cend(); ++iter)
    {
        JoyButtonSlot *slot = iter.value();
        JoySensorDirection tempDir = iter.key();
        m_pending_slots.insert(tempDir, slot);
    }
}

void JoySensorContextMenuHelper::clearPendingSlots() { m_pending_slots.clear(); }

void JoySensorContextMenuHelper::setFromPendingSlots()
{
    if (!m_pending_slots.isEmpty())
    {
        for (auto iter = m_pending_slots.cbegin(); iter != m_pending_slots.cend(); ++iter)
        {
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

void JoySensorContextMenuHelper::clearButtonsSlotsEventReset()
{
    QHash<JoySensorDirection, JoySensorButton *> *buttons = m_sensor->getButtons();
    for (auto iter = buttons->cbegin(); iter != buttons->cend(); ++iter)
    {
        JoySensorButton *button = iter.value();
        if (button)
            button->clearSlotsEventReset();
    }
}
