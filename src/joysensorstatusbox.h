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

#include <QSize>
#include <QWidget>

class JoySensor;
class QPaintEvent;

class JoySensorStatusBox : public QWidget
{
    Q_OBJECT

  public:
    explicit JoySensorStatusBox(QWidget *parent = nullptr);
    explicit JoySensorStatusBox(JoySensor *sensor, QWidget *parent = nullptr);

    void setSensor(JoySensor *sensor);

    JoySensor *getSensor() const;

    virtual int heightForWidth(int width) const;
    QSize sizeHint() const;

  protected:
    virtual void paintEvent(QPaintEvent *event);
    void drawArtificialHorizon();

  private:
    JoySensor *m_sensor;
};
