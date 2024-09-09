/*
 * Copyright 2020 UBports foundation
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
 * Author: Marius Gripsgard <marius@ubports.com>
 */

#include "vibrator.h"
#include "vibrator-ff.h"
#include "vibrator-sysfs.h"
#include "vibrator-legacy.h"
#ifdef HAVE_LIBGBINDER
#include "vibrator-binder-aidl.h"
#include "vibrator-binder-hidl.h"
#endif

#include <iostream>

namespace hfd {
class VibratorDummy : public Vibrator {
public:
    VibratorDummy() = default;
protected:
    void configure(State state, int durationMs) override {
        if (state == State::On) {
            std::cout << "VibratorDummy on, duration: " << durationMs << std::endl;
        } else {
            std::cout << "VibratorDummy off" << std::endl;
        }
    };
};


std::shared_ptr<Vibrator> Vibrator::create()
{
#ifdef HAVE_LIBGBINDER
    if (VibratorBinderAidl::usable()) {
        std::cout << "Using binder AIDL vibrator" << std::endl;
        return std::make_shared<VibratorBinderAidl>();
    }
    else if (VibratorBinderHidl::usable()) {
        std::cout << "Using binder HIDL vibrator" << std::endl;
        return std::make_shared<VibratorBinderHidl>();
    }
    else
#endif
    if (VibratorSysfs::usable()) {
        std::cout << "Using sysfs vibrator" << std::endl;
        return std::make_shared<VibratorSysfs>();
    }
    else if (VibratorLegacy::usable()) {
        std::cout << "Using legacy vibrator" << std::endl;
        return std::make_shared<VibratorLegacy>();
    }
    else if (VibratorFF::usable()) {
        std::cout << "Using force feedback vibrator" << std::endl;
        return std::make_shared<VibratorFF>();
    }

    std::cout << "Using dummy vibrator" << std::endl;
    return std::make_shared<VibratorDummy>();
}

std::shared_ptr<Vibrator> Vibrator::create(std::string type)
{
    if (type == "ff") {
        std::cout << "Using force feedback vibrator" << std::endl;
        return std::make_shared<VibratorFF>();
    }
#ifdef HAVE_LIBGBINDER
    else if (type == "binder-aidl") {
        std::cout << "Using binder AIDL vibrator" << std::endl;
        return std::make_shared<VibratorBinderAidl>();
    } else if (type == "binder-hidl" || type == "binder") {
        std::cout << "Using binder HIDL vibrator" << std::endl;
        return std::make_shared<VibratorBinderHidl>();
    }
#endif
    else if (type == "sysfs") {
        std::cout << "Using sysfs vibrator" << std::endl;
        return std::make_shared<VibratorSysfs>();
    } else if (type == "legacy") {
        std::cout << "Using legacy vibrator" << std::endl;
        return std::make_shared<VibratorLegacy>();
    }

    std::cout << "Using dummy vibrator" << std::endl;
    return std::make_shared<VibratorDummy>();
}


Vibrator::Vibrator()
{
}

void Vibrator::vibrate(int durationMs)
{
    configure(State::On, durationMs);
}
}
