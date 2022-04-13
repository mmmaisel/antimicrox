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

#include "joysensorbuttonpushbutton.h"

//#include "joybuttoncontextmenu.h"
//#include "joybuttontypes/joycontrolstickbutton.h"
//#include "joybuttontypes/joycontrolstickmodifierbutton.h"
#include "joysensor.h"

#include <QDebug>
#include <QMenu>
#include <QWidget>

JoySensorButtonPushButton::JoySensorButtonPushButton(
    /*JoySensorButton *button,*/ bool displayNames, QWidget *parent)
    : FlashButtonWidget(displayNames, parent)//,
    //m_button(button)
{
    refreshLabel();
    enableFlashes();

    tryFlash();

    setContextMenuPolicy(Qt::CustomContextMenu);
    /*connect(this, &JoySensorButtonPushButton::customContextMenuRequested, this,
            &JoySensorButtonPushButton::showContextMenu);
    connect(button, &JoySensorButton::propertyUpdated, this, &JoySensorButtonPushButton::refreshLabel);
    connect(button, &JoySensorButton::activeZoneChanged, this, &JoySensorButtonPushButton::refreshLabel);
    connect(button->getSensor()->getModifierButton(), &JoySensorModifierButton::activeZoneChanged, this,
            &JoySensorButtonPushButton::refreshLabel);*/
}

void JoySensorButtonPushButton::disableFlashes()
{
    unflash();
}

void JoySensorButtonPushButton::enableFlashes()
{
}

/**
 * @brief Generate the string that will be displayed on the button
 * @return Display string
 */
QString JoySensorButtonPushButton::generateLabel()
{
    QString temp = QString("[NO KEY]");

    qDebug() << "Here is name of action for pushed sensor button: " << temp;

    return temp;
}

void JoySensorButtonPushButton::showContextMenu(const QPoint &point)
{
}

void JoySensorButtonPushButton::tryFlash()
{
}
