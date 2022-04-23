/* antimicrox Gamepad to KB+M event mapper
 * Copyright (C) 2015 Travis Nickles <nickles.travis@gmail.com>
 * Copyright (C) 2020 Jagoda Górska <juliagoda.pl@protonmail>
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

#ifndef INPUTDEVICE_H
#define INPUTDEVICE_H

#include "setjoystick.h"

#include <SDL2/SDL_joystick.h>

class AntiMicroSettings;
class SetJoystick;
class QXmlStreamReader;
class QXmlStreamWriter;
class QSettings;

class InputDevice : public QObject
{
    Q_OBJECT

  public:
    explicit InputDevice(SDL_Joystick *joystick, int deviceIndex, AntiMicroSettings *settings, QObject *parent);
    virtual ~InputDevice();

    virtual int getNumberButtons();
    virtual int getNumberAxes();
    virtual int getNumberHats();
    virtual int hasAccelerometer();
    virtual int hasGyroscope();
    virtual int getNumberSticks();
    virtual int getNumberVDPads();

    int getJoyNumber();
    int getRealJoyNumber();
    int getActiveSetNumber();
    SetJoystick *getActiveSetJoystick();
    SetJoystick *getSetJoystick(int index);
    void removeControlStick(int index);
    bool isActive();
    int getButtonDownCount();

    virtual QString getXmlName() = 0;
    virtual QString getName() = 0;
    virtual QString getSDLName() = 0;
    virtual QString getDescription();

    // GUID only available on SDL 2.
    virtual QString getGUIDString() = 0;
    virtual QString getUniqueIDString() = 0;
    virtual QString getVendorString() = 0;
    virtual QString getProductIDString() = 0;
    virtual QString getProductVersion() = 0;
    virtual QString getRawGUIDString();
    virtual QString getRawVendorString();
    virtual QString getRawProductIDString();
    virtual QString getRawProductVersion();
    virtual QString getRawUniqueIDString();
    virtual void setCounterUniques(int counter) = 0;

    virtual QString getStringIdentifier();
    virtual void closeSDLDevice() = 0;
    virtual SDL_JoystickID getSDLJoystickID() = 0;
    QString getSDLPlatform();
    virtual bool isGameController();
    virtual bool isKnownController();

    void setButtonName(int index, QString tempName);                            // InputDeviceXml class
    void setAxisButtonName(int axisIndex, int buttonIndex, QString tempName);   // InputDeviceXml class
    void setStickButtonName(int stickIndex, int buttonIndex, QString tempName); // InputDeviceXml class
    void setDPadButtonName(int dpadIndex, int buttonIndex, QString tempName);   // InputDeviceXml class
    void setVDPadButtonName(int vdpadIndex, int buttonIndex, QString tempName); // InputDeviceXml class

    void setAxisName(int axisIndex, QString tempName);   // InputDeviceAxis class
    void setStickName(int stickIndex, QString tempName); // InputDeviceStick class
    void setDPadName(int dpadIndex, QString tempName);   // InputDeviceHat class
    void setVDPadName(int vdpadIndex, QString tempName); // InputDeviceVDPad class

    virtual int getNumberRawButtons() = 0;
    virtual int getNumberRawAxes() = 0;
    virtual int getNumberRawHats() = 0;
    virtual int hasRawAccelerometer() = 0;
    virtual int hasRawGyroscope() = 0;

    int getDeviceKeyPressTime(); // unsigned

    void setIndex(int index);
    bool isDeviceEdited();
    void revertProfileEdited();

    void setKeyRepeatStatus(bool enabled);
    void setKeyRepeatDelay(int delay);
    void setKeyRepeatRate(int rate);

    bool isKeyRepeatEnabled();
    int getKeyRepeatDelay();
    int getKeyRepeatRate();

    QString getProfileName();
    bool hasCalibrationThrottle(int axisNum);
    JoyAxis::ThrottleTypes getCalibrationThrottle(int axisNum);
    void setCalibrationThrottle(int axisNum, JoyAxis::ThrottleTypes throttle);
    void setCalibrationStatus(int axisNum, JoyAxis::ThrottleTypes throttle);
    void removeCalibrationStatus(int axisNum);

    void sendLoadProfileRequest(QString location);
    AntiMicroSettings *getSettings();

    void activatePossiblePendingEvents();
    void activatePossibleControlStickEvents(); // InputDeviceStick class
    void activatePossibleAxisEvents();         // InputDeviceAxis class
    void activatePossibleSensorEvents();       // InputDeviceSensorclass
    void activatePossibleDPadEvents();         // InputDeviceHat class
    void activatePossibleVDPadEvents();        // InputDeviceVDPad class
    void activatePossibleButtonEvents();       // InputDeviceButton class
    void convertToUniqueMappSett(QSettings *sett, QString gUIDmappGroupSett, QString uniqueIDGroupSett);

    // bool isEmptyGUID(QString tempGUID);
    bool isEmptyUniqueID(QString tempUniqueID);
    // bool isRelevantGUID(QString tempGUID);
    bool isRelevantUniqueID(QString tempUniqueID);

    void setRawAxisDeadZone(int deadZone);   // InputDeviceAxis class
    int getRawAxisDeadZone();                // InputDeviceAxis class
    void rawAxisEvent(int index, int value); // InputDeviceAxis class
    bool elementsHaveNames();

    QHash<int, SetJoystick *> &getJoystick_sets();
    SDL_Joystick *getJoyHandle() const;

  protected:
    void enableSetConnections(SetJoystick *setstick);

    QHash<int, JoyAxis::ThrottleTypes> &getCali();
    SDL_JoystickID *getJoystickID();

    int rawAxisDeadZone;
    int keyPressTime; // unsigned
    QString profileName;

  signals:
    void setChangeActivated(int index);
    void setAxisThrottleActivated(int index); // InputDeviceAxis class
    void clicked(int index);
    void released(int index);

    void rawButtonClick(int index);                       // InputDeviceButton class
    void rawButtonRelease(int index);                     // InputDeviceButton class
    void rawAxisButtonClick(int axis, int buttonindex);   // InputDeviceAxisBtn class
    void rawAxisButtonRelease(int axis, int buttonindex); // InputDeviceAxisBtn class
    void rawDPadButtonClick(int dpad, int buttonindex);   // InputDeviceHat class
    void rawDPadButtonRelease(int dpad, int buttonindex); // InputDeviceHat class
    void rawAxisActivated(int axis, int value);           // InputDeviceAxis class
    void rawAxisReleased(int axis, int value);            // InputDeviceAxis class
    void rawAxisMoved(int axis, int value);               // InputDeviceAxis class
    void profileUpdated();
    void propertyUpdated();
    void profileNameEdited(QString text);
    void requestProfileLoad(QString location);
    void requestWait();

  public slots:
    void reset();
    void transferReset();
    void reInitButtons();
    void resetButtonDownCount();
    void setActiveSetNumber(int index);
    void changeSetButtonAssociation(int button_index, int originset, int newset, int mode); // InputDeviceButton class
    void changeSetAxisButtonAssociation(int button_index, int axis_index, int originset, int newset,
                                        int mode); // InputDeviceAxisBtn class
    void changeSetStickButtonAssociation(int button_index, int stick_index, int originset, int newset,
                                         int mode); // InputDeviceStick class
    void changeSetDPadButtonAssociation(int button_index, int dpad_index, int originset, int newset,
                                        int mode); // InputDeviceHat class
    void changeSetVDPadButtonAssociation(int button_index, int dpad_index, int originset, int newset,
                                         int mode); // InputDeviceVDPad class
    void setDeviceKeyPressTime(int newPressTime);   // .., unsigned
    void profileEdited();
    void setProfileName(QString value);
    void haltServices();
    void finalRemoval();

    virtual void buttonClickEvent(int buttonindex);       // InputDeviceButton class
    virtual void buttonReleaseEvent(int buttonindex);     // InputDeviceButton class
    virtual void dpadButtonClickEvent(int buttonindex);   // InputDeviceHat class
    virtual void dpadButtonReleaseEvent(int buttonindex); // InputDeviceHat class

    void establishPropertyUpdatedConnection();
    void disconnectPropertyUpdatedConnection();

  protected slots:
    void propogateSetChange(int index);
    void propogateSetAxisThrottleChange(int index, int originset);
    void buttonDownEvent(int setindex, int buttonindex);                              // InputDeviceButton class
    void buttonUpEvent(int setindex, int buttonindex);                                // InputDeviceButton class
    virtual void axisActivatedEvent(int setindex, int axisindex, int value);          // InputDeviceAxis class
    virtual void axisReleasedEvent(int setindex, int axisindex, int value);           // InputDeviceAxis class
    virtual void axisButtonDownEvent(int setindex, int axisindex, int buttonindex);   // InputDeviceAxisBtn class
    virtual void axisButtonUpEvent(int setindex, int axisindex, int buttonindex);     // InputDeviceAxisBtn class
    virtual void dpadButtonDownEvent(int setindex, int dpadindex, int buttonindex);   // InputDeviceHat class
    virtual void dpadButtonUpEvent(int setindex, int dpadindex, int buttonindex);     // InputDeviceHat class
    virtual void stickButtonDownEvent(int setindex, int stickindex, int buttonindex); // InputDeviceStick class
    virtual void stickButtonUpEvent(int setindex, int stickindex, int buttonindex);   // InputDeviceStick class

    void updateSetButtonNames(int index);                            // InputDeviceButton class
    void updateSetAxisButtonNames(int axisIndex, int buttonIndex);   // InputDeviceAxis class
    void updateSetStickButtonNames(int stickIndex, int buttonIndex); // InputDeviceStick class
    void updateSetDPadButtonNames(int dpadIndex, int buttonIndex);   // InputDeviceHat class
    void updateSetVDPadButtonNames(int vdpadIndex, int buttonIndex); // InputDeviceVDPad class

    void updateSetAxisNames(int axisIndex);   // InputDeviceAxis class
    void updateSetStickNames(int stickIndex); // InputDeviceStick class
    void updateSetDPadNames(int dpadIndex);   // InputDeviceHat class
    void updateSetVDPadNames(int vdpadIndex); // InputDeviceVDPad class

  private:
    QList<bool> &getButtonstatesLocal();
    QList<int> &getAxesstatesLocal();
    QList<int> &getDpadstatesLocal();

    SDL_Joystick *m_joyhandle;
    QHash<int, SetJoystick *> joystick_sets;
    QHash<int, JoyAxis::ThrottleTypes> cali;
    AntiMicroSettings *m_settings;
    int active_set;
    int joyNumber;
    int buttonDownCount;
    SDL_JoystickID joystickID;
    bool deviceEdited;

    bool keyRepeatEnabled;
    int keyRepeatDelay;
    int keyRepeatRate;

    QList<bool> buttonstates;
    QList<int> axesstates;
    QList<int> dpadstates;
};

Q_DECLARE_METATYPE(InputDevice *)
Q_DECLARE_METATYPE(SDL_JoystickID)

#endif // INPUTDEVICE_H
