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

#include "pt1.h"
#include <Qt>

const double PT1::FALLBACK_RATE = 200;

PT1::PT1(double tau, double rate)
{
    m_period = qFuzzyIsNull(rate) ? 1/FALLBACK_RATE : 1/rate;
    m_tau = tau * m_period;
    reset();
}

double PT1::process(double value)
{
    m_value = m_value + m_period / m_tau * (value - m_value);
    return m_value;
};

void PT1::reset()
{
    m_value = 0;
}
