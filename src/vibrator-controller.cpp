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

#include <deviceinfo.h>

#include <vibrator.h>

namespace hfd {

VibratorController::VibratorController(const std::shared_ptr<Vibrator> &vibrator, QObject * parent)
    : m_vibrator(vibrator)
{
    DeviceInfo devInfo;

    m_vibrateDurationExtraMs = std::stoul(devInfo.get("VibrateDurationExtraMs", "0"));

    m_timer.setSingleShot(true);
    // Not the best for power consumption, but to make sure the pattern doesn't skew.
    m_timer.setTimerType(Qt::PreciseTimer);

    QObject::connect(&m_timer, &QTimer::timeout, this, &VibratorController::continuePattern);
}

void VibratorController::vibratePattern(const QVector<quint32> &pattern, quint32 repeat)
{
    m_currentPattern = pattern;
    m_currentPatternOffset = 0;
    m_remainingRepeat = repeat;

    // Use timer, instead of calling continuePattern() directly, to avoid
    // blocking DBus call in case vibrator implementation blocks.
    // This will also cancel an ongoing pattern, if any, in favor of this one.
    m_firstRun = true;
    m_timer.start(0 /* ms */);
}

/*
 * Here, we're assuming that VibrateDurationExtraMs is the time it takes for the
 * vibrator to spin up and spin down. So, the idea is we split the 'off' part of
 * the pattern into the spin down, rest and the spin up (rest can be 0 if not
 * enough 'off' time is left in the pattern).
 *
 * We sleep through the first and second part, then wake up and command
 * vibration for spin up + (next) pattern 'on' time. This results in perceptible
 * vibration for a short pattern without making it actually longer.
 */
void VibratorController::continuePattern()
{
    /* Get previous off ms before advancing repeat. */
    uint prevPatOffMs;
    if (m_firstRun) {
        prevPatOffMs = UINT_MAX;
        m_firstRun = false;
    } else if (m_currentPatternOffset % 2 != 0) {
        // Can happens for odd-numbered pattern length.
        prevPatOffMs = 0;
    } else {
        prevPatOffMs = m_currentPattern[m_currentPatternOffset - 1];
    }

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

    uint patOnMs = m_currentPattern[m_currentPatternOffset];
    m_currentPatternOffset++;

    uint patOffMs = 0;
    if (m_currentPatternOffset != m_currentPattern.size()) {
        patOffMs = m_currentPattern[m_currentPatternOffset];
        m_currentPatternOffset++;
    }

    uint spinUpMs = prevPatOffMs / 2 > m_vibrateDurationExtraMs
        ? m_vibrateDurationExtraMs
        : prevPatOffMs / 2;
    uint spinDownAndRestMs = patOffMs / 2 > m_vibrateDurationExtraMs
        ? patOffMs - m_vibrateDurationExtraMs
        : (patOffMs + 1) / 2;
        // +1 makes the equation rounds the number up instead of down.

    m_timer.start(spinUpMs + patOnMs + spinDownAndRestMs);

    // Call vibrator after starting timer. Some vibrator implementation blocks
    // for the vibration duration.
    m_vibrator->vibrate(spinUpMs + patOnMs);
}

} // namespace hfd
