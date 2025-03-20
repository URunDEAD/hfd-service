/*
 * Copyright 2025 Furi Labs
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
 * Author: Bardia Moshiri <bardia@furilabs.com>
 */

#pragma once

#include "leds.h"
#include "udev/udev-cpp.h"

namespace hfd {
class LedsSysfsSingleColor : public Leds {

public:
    LedsSysfsSingleColor();

    static bool usable();
protected:
    void configure() override;

private:
    void setDelay();
    void setLedBrightness(int value);

    Udev::UdevDevice m_ledDevice;
};
}
