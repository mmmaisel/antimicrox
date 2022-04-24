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

#include "joysensorcontextmenu.h"

#include "common.h"
#include "inputdevice.h"
#include "joybuttontypes/joysensorbutton.h"
#include "joysensor.h"
#include "mousedialog/mousesensorsettingsdialog.h"

#include <QDebug>
#include <QList>
#include <QWidget>

JoySensorContextMenu::JoySensorContextMenu(JoySensor *sensor, QWidget *parent)
    : QMenu(parent),
    m_sensor(sensor),
    m_preset(sensor)
{
    connect(this, &JoySensorContextMenu::aboutToHide, this, &JoySensorContextMenu::deleteLater);
}

void JoySensorContextMenu::buildMenu()
{
    QAction *action;
    QActionGroup *presetGroup = new QActionGroup(this);
    JoySensorPreset::Preset currentPreset = m_preset.getIndex();
    QList<JoySensorPreset::Preset> presets = m_preset.getAvailablePresets();

    for (auto iter = presets.cbegin(); iter != presets.cend(); ++iter)
    {
        action = addAction(m_preset.getPresetName(*iter));
        action->setCheckable(true);
        action->setChecked(currentPreset == *iter);
        action->setData(QVariant(*iter));
        connect(action, &QAction::triggered, this,
            [this, action] { m_preset.setSensorPreset(
                static_cast<JoySensorPreset::Preset>(action->data().toInt())); });
        presetGroup->addAction(action);
    }

    if (m_sensor->getType() == JoySensor::GYROSCOPE)
    {
        addSeparator();

        action = addAction(tr("Mouse Settings"));
        action->setCheckable(false);
        connect(action, &QAction::triggered, this, &JoySensorContextMenu::openMouseSettingsDialog);
    }
}

void JoySensorContextMenu::openMouseSettingsDialog()
{
    MouseSensorSettingsDialog *dialog = new MouseSensorSettingsDialog(m_sensor, parentWidget());
    dialog->show();
}
