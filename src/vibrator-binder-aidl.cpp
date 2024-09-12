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
 * Author: Muhammad <muhammad23012009@hotmail.com>
 */

#include "vibrator-binder-aidl.h"

#include <iostream>
#include <unistd.h>

#define BINDER_AIDL_VIBRATOR_SERVICE_DEVICE "/dev/binder"
#define BINDER_AIDL_VIBRATOR_SERVICE_IFACE "android.hardware.vibrator.IVibrator"
#define BINDER_AIDL_VIBRATOR_SERVICE_FQNAME BINDER_AIDL_VIBRATOR_SERVICE_IFACE "/default"
#define BINDER_AIDL_VIBRATOR_ON (3)
#define BINDER_AIDL_VIBRATOR_OFF (2)

namespace hfd {

bool VibratorBinderAidl::usable() {
    GBinderServiceManager *sm;
    GBinderRemoteObject *remote;

    sm = gbinder_servicemanager_new(BINDER_AIDL_VIBRATOR_SERVICE_DEVICE);
    if (!sm)
        return false;

    remote = gbinder_servicemanager_get_service_sync(sm, BINDER_AIDL_VIBRATOR_SERVICE_FQNAME, NULL);
    if (!remote) {
        gbinder_servicemanager_unref(sm);
        return false;
    }

    gbinder_remote_object_unref(remote);
    gbinder_servicemanager_unref(sm);

    return true;
}

VibratorBinderAidl::VibratorBinderAidl(): Vibrator() {
    mSm = gbinder_servicemanager_new(BINDER_AIDL_VIBRATOR_SERVICE_DEVICE);
    if (!mSm)
        return;

    mRemote = gbinder_servicemanager_get_service_sync(mSm, BINDER_AIDL_VIBRATOR_SERVICE_FQNAME, NULL);

    if (!mRemote) {
        gbinder_servicemanager_unref(mSm);
        return;
    }

    mClient = gbinder_client_new(mRemote, BINDER_AIDL_VIBRATOR_SERVICE_IFACE);
    if (!mClient) {
        gbinder_remote_object_unref(mRemote);
        gbinder_servicemanager_unref(mSm);
        return;
    }

    // Make sure we are off on init
    configure(State::Off, 0);
}

VibratorBinderAidl::~VibratorBinderAidl()
{
    gbinder_remote_object_unref(mRemote);
    gbinder_servicemanager_unref(mSm);

    if (m_thread) {
        m_thread->join();
    }
}

void VibratorBinderAidl::configure(State state, int durationMs) {
    int status;
    GBinderRemoteReply *reply;
    GBinderLocalRequest *req = gbinder_client_new_request(mClient);

    if (state == State::On) {
        gbinder_local_request_append_int32(req, durationMs);
        gbinder_local_request_append_local_object(req, nullptr);

        reply = gbinder_client_transact_sync_reply(mClient,
                                                   BINDER_AIDL_VIBRATOR_ON /* on */, req, &status);
    } else {
        reply = gbinder_client_transact_sync_reply(mClient,
                                                   BINDER_AIDL_VIBRATOR_OFF /* off */, req, &status);
    }
    gbinder_local_request_unref(req);

    if (status == GBINDER_STATUS_OK) {
        GBinderReader reader;
        guint value;

        gbinder_remote_reply_init_reader(reply, &reader);
        status = gbinder_reader_read_uint32(&reader, &value);
    }
}
}
