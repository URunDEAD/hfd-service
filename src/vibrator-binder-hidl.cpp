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
 * Author: Erfan Abdi <erfangplus@gmail.com>
 */

#include "vibrator-binder-hidl.h"

#include <iostream>
#include <unistd.h>

#define BINDER_HIDL_VIBRATOR_SERVICE_DEVICE "/dev/hwbinder"
#define BINDER_HIDL_VIBRATOR_SERVICE_IFACE "android.hardware.vibrator@1.0::IVibrator"
#define BINDER_HIDL_VIBRATOR_SERVICE_FQNAME BINDER_HIDL_VIBRATOR_SERVICE_IFACE "/default"
#define BINDER_HIDL_VIBRATOR_ON (1)
#define BINDER_HIDL_VIBRATOR_OFF (2)

namespace hfd {

bool VibratorBinderHidl::usable() {
    GBinderServiceManager *sm;
    GBinderRemoteObject *remote;

    sm = gbinder_servicemanager_new(BINDER_HIDL_VIBRATOR_SERVICE_DEVICE);
    if (!sm)
        return false;

    /* "autoreleased" i.e. unowned */
    remote = gbinder_servicemanager_get_service_sync(sm, BINDER_HIDL_VIBRATOR_SERVICE_FQNAME, NULL);
    if (!remote) {
        gbinder_servicemanager_unref(sm);
        return false;
    }

    gbinder_servicemanager_unref(sm);

    return true;
}

VibratorBinderHidl::VibratorBinderHidl(): Vibrator() {
    GBinderRemoteObject *remote;

    mSm = gbinder_servicemanager_new(BINDER_HIDL_VIBRATOR_SERVICE_DEVICE);
    if (!mSm)
        return;

    /* "autoreleased" i.e. unowned */
    remote = gbinder_servicemanager_get_service_sync(mSm, BINDER_HIDL_VIBRATOR_SERVICE_FQNAME, NULL);

    if (!remote) {
        gbinder_servicemanager_unref(mSm);
        return;
    }

    mClient = gbinder_client_new(remote, BINDER_HIDL_VIBRATOR_SERVICE_IFACE);
    if (!mClient) {
        gbinder_servicemanager_unref(mSm);
        return;
    }

    // Make sure we are off on init
    configure(State::Off, 0);
}

VibratorBinderHidl::~VibratorBinderHidl()
{
    gbinder_servicemanager_unref(mSm);

    if (m_thread) {
        m_thread->join();
    }
}

void VibratorBinderHidl::configure(State state, int durationMs) {
    int status;
    GBinderRemoteReply *reply;
    GBinderLocalRequest *req = gbinder_client_new_request(mClient);

    if (state == State::On) {
        gbinder_local_request_append_int32(req, durationMs);

        reply = gbinder_client_transact_sync_reply(mClient,
                                                   BINDER_HIDL_VIBRATOR_ON /* on */, req, &status);
    } else {
        reply = gbinder_client_transact_sync_reply(mClient,
                                                   BINDER_HIDL_VIBRATOR_OFF /* off */, req, &status);
    }
    gbinder_local_request_unref(req);

    if (status == GBINDER_STATUS_OK) {
        GBinderReader reader;
        guint value;

        gbinder_remote_reply_init_reader(reply, &reader);
        status = gbinder_reader_read_uint32(&reader, &value);
    }

    gbinder_remote_reply_unref(reply);
}
}
