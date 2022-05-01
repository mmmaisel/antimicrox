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

#include <cstddef>

/**
 * @brief Calculates mean and variance of a data stream using
 *   Welford's algorithm as well as 3 sigma interval width.
 */
class StatisticsProcessor
{
    public:
        StatisticsProcessor();

        void process(double x);
        void reset();

        inline size_t getCount() const { return m_count; }
        inline double getMean() const { return m_mean; }
        inline double getVariance() const { return m_var / (m_count - 1); }
        // Quadratic relative error (+/- x%)^2 in a 3 sigma interval.
        inline double getRelativeErrorSq() const { return 9 * getVariance() / (m_count * m_mean * m_mean); }

    private:
        double m_mean;
        double m_var;
        size_t m_count;
};
