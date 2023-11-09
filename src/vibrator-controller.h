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

#include <memory>

#include <QtGlobal>
#include <QObject>
#include <QTimer>
#include <QVector>

namespace hfd {

class Vibrator;

class VibratorController : public QObject {
    Q_OBJECT
public:
    VibratorController(const std::shared_ptr<Vibrator> &vibrator, QObject * parent = nullptr);

    void vibratePattern(const QVector<quint32> &pattern, quint32 repeat);
    inline void vibrate(uint durationMs) {
        vibratePattern({ durationMs }, /* repeat */ 1);
    }
    inline void rumble(uint durationMs, uint repeat) {
        vibratePattern({ durationMs, durationMs }, repeat);
    }
private:
    void continuePattern();

    std::shared_ptr<Vibrator> m_vibrator;

    QTimer m_timer;
    /* In the format of [onMs, offMs, ...] */
    QVector<quint32> m_currentPattern;
    uint m_currentPatternOffset = 0;
    /* Includes the current iteration. */
    uint m_remainingRepeat = 0;
};

} // namespace hfd
