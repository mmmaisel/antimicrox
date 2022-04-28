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

#include "joysensorpreset.h"

#include "antkeymapper.h"
#include "common.h"
#include "joybuttontypes/joysensorbutton.h"

JoySensorPreset::JoySensorPreset(JoySensor *sensor, QObject *parent)
    : QObject(parent),
    m_sensor(sensor),
    m_helper(sensor)
{
    m_helper.moveToThread(m_sensor->thread());
}

QList<JoySensorPreset::Preset> JoySensorPreset::getAvailablePresets()
{
    QList<Preset> result;
    result.append(PRESET_NONE);

    if(m_sensor->getType() == JoySensor::ACCELEROMETER)
    {
        result.append(PRESET_ARROWS);
        result.append(PRESET_WASD);
        result.append(PRESET_NUMPAD);
    } else
    {
        result.append(PRESET_MOUSE);
        result.append(PRESET_MOUSE_INV_H);
        result.append(PRESET_MOUSE_INV_V);
        result.append(PRESET_MOUSE_INV_HV);
    }

    return result;
}

JoySensorPreset::Preset JoySensorPreset::currentPreset()
{
    Preset result = PRESET_NONE;
    QList<JoyButtonSlot *> *upslots, *downslots, *leftslots, *rightslots, *fwdslots, *bwdslots;
    JoySensorButton *upButton, *downButton, *leftButton, *rightButton, *fwdButton, *bwdButton;

    PadderCommon::inputDaemonMutex.lock();

    if (m_sensor->getType() == JoySensor::GYROSCOPE)
    {
        upButton = m_sensor->getDirectionButton(JoySensorDirection::GYRO_NICK_P);
        upslots = upButton->getAssignedSlots();
        downButton = m_sensor->getDirectionButton(JoySensorDirection::GYRO_NICK_N);
        downslots = downButton->getAssignedSlots();
        leftButton = m_sensor->getDirectionButton(JoySensorDirection::GYRO_YAW_N);
        leftslots = leftButton->getAssignedSlots();
        rightButton = m_sensor->getDirectionButton(JoySensorDirection::GYRO_YAW_P);
        rightslots = rightButton->getAssignedSlots();
        fwdButton = m_sensor->getDirectionButton(JoySensorDirection::GYRO_ROLL_P);
        fwdslots = fwdButton->getAssignedSlots();
        bwdButton = m_sensor->getDirectionButton(JoySensorDirection::GYRO_ROLL_N);
        bwdslots = bwdButton->getAssignedSlots();

        if (upslots->length() == 1 && downslots->length() == 1 &&
            leftslots->length() == 1 && rightslots->length() == 1 &&
            fwdslots->length() == 0 && bwdslots->length() == 0)
        {
            JoyButtonSlot *upslot = upslots->at(0);
            JoyButtonSlot *downslot = downslots->at(0);
            JoyButtonSlot *leftslot = leftslots->at(0);
            JoyButtonSlot *rightslot = rightslots->at(0);

            if ((upslot->getSlotMode() == JoyButtonSlot::JoyMouseMovement) &&
                (upslot->getSlotCode() == JoyButtonSlot::MouseUp) &&
                (downslot->getSlotMode() == JoyButtonSlot::JoyMouseMovement) &&
                (downslot->getSlotCode() == JoyButtonSlot::MouseDown) &&
                (leftslot->getSlotMode() == JoyButtonSlot::JoyMouseMovement) &&
                (leftslot->getSlotCode() == JoyButtonSlot::MouseLeft) &&
                (rightslot->getSlotMode() == JoyButtonSlot::JoyMouseMovement) &&
                (rightslot->getSlotCode() == JoyButtonSlot::MouseRight))
            {
                result = PRESET_MOUSE;
            } else if ((upslot->getSlotMode() == JoyButtonSlot::JoyMouseMovement) &&
                       (upslot->getSlotCode() == JoyButtonSlot::MouseUp) &&
                       (downslot->getSlotMode() == JoyButtonSlot::JoyMouseMovement) &&
                       (downslot->getSlotCode() == JoyButtonSlot::MouseDown) &&
                       (leftslot->getSlotMode() == JoyButtonSlot::JoyMouseMovement) &&
                       (leftslot->getSlotCode() == JoyButtonSlot::MouseRight) &&
                       (rightslot->getSlotMode() == JoyButtonSlot::JoyMouseMovement) &&
                       (rightslot->getSlotCode() == JoyButtonSlot::MouseLeft))
            {
                result = PRESET_MOUSE_INV_H;
            } else if ((upslot->getSlotMode() == JoyButtonSlot::JoyMouseMovement) &&
                       (upslot->getSlotCode() == JoyButtonSlot::MouseDown) &&
                       (downslot->getSlotMode() == JoyButtonSlot::JoyMouseMovement) &&
                       (downslot->getSlotCode() == JoyButtonSlot::MouseUp) &&
                       (leftslot->getSlotMode() == JoyButtonSlot::JoyMouseMovement) &&
                       (leftslot->getSlotCode() == JoyButtonSlot::MouseLeft) &&
                       (rightslot->getSlotMode() == JoyButtonSlot::JoyMouseMovement) &&
                       (rightslot->getSlotCode() == JoyButtonSlot::MouseRight))
            {
                result = PRESET_MOUSE_INV_V;
            } else if ((upslot->getSlotMode() == JoyButtonSlot::JoyMouseMovement) &&
                       (upslot->getSlotCode() == JoyButtonSlot::MouseDown) &&
                       (downslot->getSlotMode() == JoyButtonSlot::JoyMouseMovement) &&
                       (downslot->getSlotCode() == JoyButtonSlot::MouseUp) &&
                       (leftslot->getSlotMode() == JoyButtonSlot::JoyMouseMovement) &&
                       (leftslot->getSlotCode() == JoyButtonSlot::MouseRight) &&
                       (rightslot->getSlotMode() == JoyButtonSlot::JoyMouseMovement) &&
                       (rightslot->getSlotCode() == JoyButtonSlot::MouseLeft))
            {
                result = PRESET_MOUSE_INV_HV;
            }
        }
    } else
    {
        upButton = m_sensor->getDirectionButton(JoySensorDirection::ACCEL_UP);
        upslots = upButton->getAssignedSlots();
        downButton = m_sensor->getDirectionButton(JoySensorDirection::ACCEL_DOWN);
        downslots = downButton->getAssignedSlots();
        leftButton = m_sensor->getDirectionButton(JoySensorDirection::ACCEL_LEFT);
        leftslots = leftButton->getAssignedSlots();
        rightButton = m_sensor->getDirectionButton(JoySensorDirection::ACCEL_RIGHT);
        rightslots = rightButton->getAssignedSlots();
        fwdButton = m_sensor->getDirectionButton(JoySensorDirection::ACCEL_FWD);
        fwdslots = fwdButton->getAssignedSlots();

        if (upslots->length() == 1 && downslots->length() == 1 &&
            leftslots->length() == 1 && rightslots->length() == 1 &&
            fwdslots->length() == 0)
        {
            JoyButtonSlot *upslot = upslots->at(0);
            JoyButtonSlot *downslot = downslots->at(0);
            JoyButtonSlot *leftslot = leftslots->at(0);
            JoyButtonSlot *rightslot = rightslots->at(0);

            if ((upslot->getSlotMode() == JoyButtonSlot::JoyKeyboard) &&
               (upslot->getSlotCode() == AntKeyMapper::getInstance()->returnVirtualKey(Qt::Key_Up)) &&
               (downslot->getSlotMode() == JoyButtonSlot::JoyKeyboard) &&
               (downslot->getSlotCode() == AntKeyMapper::getInstance()->returnVirtualKey(Qt::Key_Down)) &&
               (leftslot->getSlotMode() == JoyButtonSlot::JoyKeyboard) &&
               (leftslot->getSlotCode() == AntKeyMapper::getInstance()->returnVirtualKey(Qt::Key_Left)) &&
               (rightslot->getSlotMode() == JoyButtonSlot::JoyKeyboard) &&
               (rightslot->getSlotCode() == AntKeyMapper::getInstance()->returnVirtualKey(Qt::Key_Right)))
            {
                result = PRESET_ARROWS;
            } else if ((upslot->getSlotMode() == JoyButtonSlot::JoyKeyboard) &&
                       (upslot->getSlotCode() == AntKeyMapper::getInstance()->returnVirtualKey(Qt::Key_W)) &&
                       (downslot->getSlotMode() == JoyButtonSlot::JoyKeyboard) &&
                       (downslot->getSlotCode() == AntKeyMapper::getInstance()->returnVirtualKey(Qt::Key_S)) &&
                       (leftslot->getSlotMode() == JoyButtonSlot::JoyKeyboard) &&
                       (leftslot->getSlotCode() == AntKeyMapper::getInstance()->returnVirtualKey(Qt::Key_A)) &&
                       (rightslot->getSlotMode() == JoyButtonSlot::JoyKeyboard) &&
                       (rightslot->getSlotCode() == AntKeyMapper::getInstance()->returnVirtualKey(Qt::Key_D)))
            {
                result = PRESET_WASD;
            } else if ((upslot->getSlotMode() == JoyButtonSlot::JoyKeyboard) &&
                       (upslot->getSlotCode() == AntKeyMapper::getInstance()->returnVirtualKey(QtKeyMapperBase::AntKey_KP_8)) &&
                       (downslot->getSlotMode() == JoyButtonSlot::JoyKeyboard) &&
                       (downslot->getSlotCode() ==
                        AntKeyMapper::getInstance()->returnVirtualKey(QtKeyMapperBase::AntKey_KP_2)) &&
                       (leftslot->getSlotMode() == JoyButtonSlot::JoyKeyboard) &&
                       (leftslot->getSlotCode() ==
                        AntKeyMapper::getInstance()->returnVirtualKey(QtKeyMapperBase::AntKey_KP_4)) &&
                       (rightslot->getSlotMode() == JoyButtonSlot::JoyKeyboard) &&
                       (rightslot->getSlotCode() == AntKeyMapper::getInstance()->returnVirtualKey(QtKeyMapperBase::AntKey_KP_6)))
            {
                result = PRESET_NUMPAD;
            }
        }
    }

    PadderCommon::inputDaemonMutex.unlock();
    return result;
}

QString JoySensorPreset::getPresetName(Preset preset)
{
    QString result;
    switch(preset)
    {
        case PRESET_NONE:
            result = tr("None");
            break;
        case PRESET_MOUSE:
            result = tr("Mouse (Normal)");
            break;
        case PRESET_MOUSE_INV_H:
            result = tr("Mouse (Inverted Horizontal)");
            break;
        case PRESET_MOUSE_INV_V:
            result = tr("Mouse (Inverted Vertical)");
            break;
        case PRESET_MOUSE_INV_HV:
            result = tr("Mouse (Inverted Horizontal + Vertical)");
            break;
        case PRESET_ARROWS:
            result = tr("Arrows");
            break;
        case PRESET_WASD:
            result = tr("Keys: W | A | S | D");
            break;
        case PRESET_NUMPAD:
            result = tr("NumPad");
            break;
    }
    return result;
}

void JoySensorPreset::setSensorPreset(Preset preset)
{
    JoyButtonSlot *upButtonSlot = nullptr;
    JoyButtonSlot *downButtonSlot = nullptr;
    JoyButtonSlot *leftButtonSlot = nullptr;
    JoyButtonSlot *rightButtonSlot = nullptr;
    JoyButtonSlot *fwdButtonSlot = nullptr;
    JoyButtonSlot *bwdButtonSlot = nullptr;

    switch (preset)
    {
    case PRESET_NONE:
        QMetaObject::invokeMethod(&m_helper, "clearButtonsSlotsEventReset");
        QMetaObject::invokeMethod(m_sensor, "setDiagonalRange", Q_ARG(int, 45));
        break;
    case PRESET_MOUSE:
        PadderCommon::inputDaemonMutex.lock();

        upButtonSlot = new JoyButtonSlot(JoyButtonSlot::MouseUp, JoyButtonSlot::JoyMouseMovement, this);
        downButtonSlot = new JoyButtonSlot(JoyButtonSlot::MouseDown, JoyButtonSlot::JoyMouseMovement, this);
        leftButtonSlot = new JoyButtonSlot(JoyButtonSlot::MouseLeft, JoyButtonSlot::JoyMouseMovement, this);
        rightButtonSlot = new JoyButtonSlot(JoyButtonSlot::MouseRight, JoyButtonSlot::JoyMouseMovement, this);
        m_sensor->setDeadZone(0);
        m_sensor->setDiagonalRange(90);

        PadderCommon::inputDaemonMutex.unlock();
        break;
    case PRESET_MOUSE_INV_H:
        PadderCommon::inputDaemonMutex.lock();

        upButtonSlot = new JoyButtonSlot(JoyButtonSlot::MouseUp, JoyButtonSlot::JoyMouseMovement, this);
        downButtonSlot = new JoyButtonSlot(JoyButtonSlot::MouseDown, JoyButtonSlot::JoyMouseMovement, this);
        leftButtonSlot = new JoyButtonSlot(JoyButtonSlot::MouseRight, JoyButtonSlot::JoyMouseMovement, this);
        rightButtonSlot = new JoyButtonSlot(JoyButtonSlot::MouseLeft, JoyButtonSlot::JoyMouseMovement, this);
        m_sensor->setDeadZone(0);
        m_sensor->setDiagonalRange(90);

        PadderCommon::inputDaemonMutex.unlock();
        break;
    case PRESET_MOUSE_INV_V:
        PadderCommon::inputDaemonMutex.lock();

        upButtonSlot = new JoyButtonSlot(JoyButtonSlot::MouseDown, JoyButtonSlot::JoyMouseMovement, this);
        downButtonSlot = new JoyButtonSlot(JoyButtonSlot::MouseUp, JoyButtonSlot::JoyMouseMovement, this);
        leftButtonSlot = new JoyButtonSlot(JoyButtonSlot::MouseLeft, JoyButtonSlot::JoyMouseMovement, this);
        rightButtonSlot = new JoyButtonSlot(JoyButtonSlot::MouseRight, JoyButtonSlot::JoyMouseMovement, this);
        m_sensor->setDeadZone(0);
        m_sensor->setDiagonalRange(90);

        PadderCommon::inputDaemonMutex.unlock();
        break;
    case PRESET_MOUSE_INV_HV:
        PadderCommon::inputDaemonMutex.lock();

        upButtonSlot = new JoyButtonSlot(JoyButtonSlot::MouseDown, JoyButtonSlot::JoyMouseMovement, this);
        downButtonSlot = new JoyButtonSlot(JoyButtonSlot::MouseUp, JoyButtonSlot::JoyMouseMovement, this);
        leftButtonSlot = new JoyButtonSlot(JoyButtonSlot::MouseRight, JoyButtonSlot::JoyMouseMovement, this);
        rightButtonSlot = new JoyButtonSlot(JoyButtonSlot::MouseLeft, JoyButtonSlot::JoyMouseMovement, this);
        m_sensor->setDeadZone(0);
        m_sensor->setDiagonalRange(90);

        PadderCommon::inputDaemonMutex.unlock();
        break;
    case PRESET_ARROWS:
        PadderCommon::inputDaemonMutex.lock();

        upButtonSlot = new JoyButtonSlot(AntKeyMapper::getInstance()->returnVirtualKey(Qt::Key_Up), Qt::Key_Up,
                                         JoyButtonSlot::JoyKeyboard, this);
        downButtonSlot = new JoyButtonSlot(AntKeyMapper::getInstance()->returnVirtualKey(Qt::Key_Down), Qt::Key_Down,
                                           JoyButtonSlot::JoyKeyboard, this);
        leftButtonSlot = new JoyButtonSlot(AntKeyMapper::getInstance()->returnVirtualKey(Qt::Key_Left), Qt::Key_Left,
                                           JoyButtonSlot::JoyKeyboard, this);
        rightButtonSlot = new JoyButtonSlot(AntKeyMapper::getInstance()->returnVirtualKey(Qt::Key_Right), Qt::Key_Right,
                                            JoyButtonSlot::JoyKeyboard, this);
        m_sensor->setDeadZone(15);
        m_sensor->setDiagonalRange(45);

        PadderCommon::inputDaemonMutex.unlock();
        break;
    case PRESET_WASD:
        PadderCommon::inputDaemonMutex.lock();

        upButtonSlot = new JoyButtonSlot(AntKeyMapper::getInstance()->returnVirtualKey(Qt::Key_W), Qt::Key_W,
                                         JoyButtonSlot::JoyKeyboard, this);
        downButtonSlot = new JoyButtonSlot(AntKeyMapper::getInstance()->returnVirtualKey(Qt::Key_S), Qt::Key_S,
                                           JoyButtonSlot::JoyKeyboard, this);
        leftButtonSlot = new JoyButtonSlot(AntKeyMapper::getInstance()->returnVirtualKey(Qt::Key_A), Qt::Key_A,
                                           JoyButtonSlot::JoyKeyboard, this);
        rightButtonSlot = new JoyButtonSlot(AntKeyMapper::getInstance()->returnVirtualKey(Qt::Key_D), Qt::Key_D,
                                            JoyButtonSlot::JoyKeyboard, this);
        m_sensor->setDeadZone(15);
        m_sensor->setDiagonalRange(45);

        PadderCommon::inputDaemonMutex.unlock();
        break;
    case PRESET_NUMPAD:
        PadderCommon::inputDaemonMutex.lock();

        upButtonSlot = new JoyButtonSlot(AntKeyMapper::getInstance()->returnVirtualKey(QtKeyMapperBase::AntKey_KP_8),
                                         QtKeyMapperBase::AntKey_KP_8, JoyButtonSlot::JoyKeyboard, this);
        downButtonSlot = new JoyButtonSlot(AntKeyMapper::getInstance()->returnVirtualKey(QtKeyMapperBase::AntKey_KP_2),
                                           QtKeyMapperBase::AntKey_KP_2, JoyButtonSlot::JoyKeyboard, this);
        leftButtonSlot = new JoyButtonSlot(AntKeyMapper::getInstance()->returnVirtualKey(QtKeyMapperBase::AntKey_KP_4),
                                           QtKeyMapperBase::AntKey_KP_4, JoyButtonSlot::JoyKeyboard, this);
        rightButtonSlot = new JoyButtonSlot(AntKeyMapper::getInstance()->returnVirtualKey(QtKeyMapperBase::AntKey_KP_6),
                                            QtKeyMapperBase::AntKey_KP_6, JoyButtonSlot::JoyKeyboard, this);
        m_sensor->setDeadZone(15);
        m_sensor->setDiagonalRange(45);

        PadderCommon::inputDaemonMutex.unlock();
        break;
    }

    QHash<JoySensorDirection, JoyButtonSlot *> tempHash;
    if (m_sensor->getType() == JoySensor::GYROSCOPE)
    {
        tempHash.insert(JoySensorDirection::GYRO_NICK_P, upButtonSlot);
        tempHash.insert(JoySensorDirection::GYRO_NICK_N, downButtonSlot);
        tempHash.insert(JoySensorDirection::GYRO_YAW_P, rightButtonSlot);
        tempHash.insert(JoySensorDirection::GYRO_YAW_N, leftButtonSlot);
        tempHash.insert(JoySensorDirection::GYRO_ROLL_P, fwdButtonSlot);
        tempHash.insert(JoySensorDirection::GYRO_ROLL_N, bwdButtonSlot);
    } else
    {
        tempHash.insert(JoySensorDirection::ACCEL_UP, upButtonSlot);
        tempHash.insert(JoySensorDirection::ACCEL_DOWN, downButtonSlot);
        tempHash.insert(JoySensorDirection::ACCEL_LEFT, leftButtonSlot);
        tempHash.insert(JoySensorDirection::ACCEL_RIGHT, rightButtonSlot);
        tempHash.insert(JoySensorDirection::ACCEL_FWD, fwdButtonSlot);
    }

    m_helper.setPendingSlots(&tempHash);
    QMetaObject::invokeMethod(&m_helper, "setFromPendingSlots", Qt::BlockingQueuedConnection);
}

JoySensorIoThreadHelper &JoySensorPreset::getHelper()
{
    return m_helper;
}
