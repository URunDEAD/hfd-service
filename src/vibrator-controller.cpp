/*
 * Copyright 2023 UBports foundation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Ratchanan Srirattanamet <ratchanan@ubports.com>
 */

#include <vibrator-controller.h>

#include <vibrator.h>

namespace hfd {

VibratorController::VibratorController(const std::shared_ptr<Vibrator> &vibrator, QObject * parent)
    : m_vibrator(vibrator)
{
    m_timer.setSingleShot(true);
    // Not the best for power consumption, but to make sure the pattern doesn't skew.
    m_timer.setTimerType(Qt::PreciseTimer);

    QObject::connect(&m_timer, &QTimer::timeout, this, &VibratorController::continuePattern);
}

void VibratorController::vibratePattern(const QVector<quint32> &pattern, quint32 repeat)
{
    if (m_timer.isActive()) {
        // Cancel ongoing pattern in place of this one.
        // TODO: will this enable denial of service?
        m_timer.stop();
    }

    m_currentPattern = pattern;
    m_currentPatternOffset = 0;
    m_remainingRepeat = repeat;

    continuePattern();
}

void VibratorController::continuePattern()
{
    if (m_currentPatternOffset == m_currentPattern.size() &&
        m_remainingRepeat != 0)
    {
        m_remainingRepeat -= 1;
        m_currentPatternOffset = 0;
    }

    if (m_remainingRepeat == 0) {
        m_currentPattern = QVector<quint32>();
        m_currentPatternOffset = 0;

        return;
    }

    uint onMs = m_currentPattern[m_currentPatternOffset];
    m_currentPatternOffset++;

    uint sleepMs = onMs;
    if (m_currentPatternOffset != m_currentPattern.size()) {
        uint offMs = m_currentPattern[m_currentPatternOffset];
        sleepMs += offMs;
        m_currentPatternOffset++;
    }

    m_timer.setInterval(sleepMs);
    m_timer.start();

    // Call vibrator after starting timer. Some vibrator implementation blocks
    // for the vibration duration.
    m_vibrator->vibrate(onMs);
}

} // namespace hfd
