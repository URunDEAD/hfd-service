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

#include "leds-sysfs-single-color.h"
#include "utils.h"

#include <iostream>
#include <regex>
#include <unistd.h>

static bool has_timer_trigger(Udev::UdevDevice const& dev) {
    std::string trigger_value;

    try {
        // get_sysattr() always gives the full list of available values.
        trigger_value = dev.get_sysattr("trigger");
    } catch (std::runtime_error const&) {
        return false;
    }

    /*
     * Word "timer" maybe at the start or the end, in the middle surrounded by
     * spaces, or surrounded by '[' and ']'.
     */
    static std::regex timer_re("(^|[[ ])timer([] ]|$)", std::regex_constants::extended);
    return std::regex_search(trigger_value, timer_re);
}

namespace hfd {
bool LedsSysfsSingleColor::usable() {
    if (access("/sys/class/leds", F_OK ) == -1)
        return false;

    Udev::Udev udev;
    Udev::UdevEnumerate enumerator = udev.enumerate_new();
    enumerator.add_match_sysattr("subsystem","leds");
    enumerator.scan_devices();

    for (auto dev : enumerator.enumerate_devices()) {
        if (has_timer_trigger(dev)) {
            return true;
        }
    }

    return false;
}

LedsSysfsSingleColor::LedsSysfsSingleColor(): Leds() {
    Udev::Udev udev;

    Udev::UdevEnumerate enumerator = udev.enumerate_new();
    enumerator.add_match_sysattr("subsystem","leds");
    enumerator.scan_devices();

    bool found_led = false;
    for (auto dev : enumerator.enumerate_devices()) {
        if (has_timer_trigger(dev)) {
            m_ledDevice = dev;
            found_led = true;
            std::cout << "Using LED: " << dev.get_sysname() << std::endl;
            break;
        }
    }

    if (found_led) {
        m_ledDevice.set_sysattr("trigger", "timer");
        m_timer = true;
    }

    // Get up to date
    configure();
}

void LedsSysfsSingleColor::configure() {
    if (m_state == State::On) {
        int brightness = ((77*((m_color>>16)&0x00ff)) +
                          (150*((m_color>>8)&0x00ff)) +
                          (29*(m_color&0x00ff)));
        std::cout << "From RGB: " << std::to_string(rgba::Red(m_color)) << ", "
                                  << std::to_string(rgba::Green(m_color)) << ", "
                                  << std::to_string(rgba::Blue(m_color)) << std::endl;

        setDelay();
        setLedBrightness(brightness);
    } else {
        setLedBrightness(0);
    }
}

void LedsSysfsSingleColor::setDelay()
{
    if (m_timer) {
        m_ledDevice.set_sysattr("trigger", "timer");
        m_ledDevice.set_sysattr("delay_on", std::to_string(m_onMs));
        m_ledDevice.set_sysattr("delay_off", std::to_string(m_offMs));
    }
}

void LedsSysfsSingleColor::setLedBrightness(int value)
{
    try {
        std::string max_brightness = m_ledDevice.get_sysattr("max_brightness");
        int max_value = std::stoi(max_brightness);
        value = ((value * max_value) + 0xffff) / 0x10000;
    } catch (std::exception const&) {
        // If we can't get max_brightness, default to 255
        value = ((value * 255) + 0xffff) / 0x10000;
    }

    std::cout << "Setting brightness: " << value << std::endl;
    m_ledDevice.set_sysattr("brightness", std::to_string(value));
}
}
