/*
 * Copyright (C) 2015 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: James Henstridge <james.henstridge@canonical.com>
 */

#include "credentialscache.h"

#include <QDBusPendingCallWatcher>

#include <assert.h>
#include <vector>

/*
 * Note: this file originates from lomiri-thumbnailer, with a few modifications
 * to remove its namespace and AppArmor support (not needed by us).
 */

using namespace std;

namespace {

char const DBUS_BUS_NAME[] = "org.freedesktop.DBus";
char const DBUS_BUS_PATH[] = "/org/freedesktop/DBus";

char const UNIX_USER_ID[] = "UnixUserID";

int const MAX_CACHE_SIZE = 50;

}

struct CredentialsCache::Request
{
    QDBusPendingCallWatcher watcher;
    std::vector<CredentialsCache::Callback> callbacks;

    Request(QDBusPendingReply<QVariantMap> const& call) : watcher(call) {}
};

CredentialsCache::CredentialsCache(QDBusConnection const& bus)
    : bus_daemon_(DBUS_BUS_NAME, DBUS_BUS_PATH, bus)
{
}

CredentialsCache::~CredentialsCache() = default;

void CredentialsCache::get(QString const& peer, Callback const& callback)
{
    // Return the credentials directly if they are cached
    try
    {
        Credentials const& credentials = cache_.at(peer);
        callback(credentials);
        return;
    }
    catch (std::out_of_range const &)
    {
        // ignore
    }

    // If the credentials exist in the previous generation of the
    // cache, move them to the current generation.
    try
    {
        Credentials& credentials = old_cache_.at(peer);
        // No real way to get coverage here because we'd
        // need more than 50 peers with different credentials.
        // LCOV_EXCL_START
        cache_.emplace(peer, std::move(credentials));
        old_cache_.erase(peer);
        callback(cache_.at(peer));
        return;
        // LCOV_EXCL_STOP
    }
    catch (std::out_of_range const &)
    {
        // ignore
    }

    // If the credentials are already being requested, add ourselves
    // to the callback list.
    try
    {
        unique_ptr<Request>& request = pending_.at(peer);
        request->callbacks.push_back(callback);
        return;
    }
    catch (std::out_of_range const &)
    {
        // ignore
    }

    // Ask the bus daemon for the peer's credentials
    unique_ptr<Request> request(
        new Request(bus_daemon_.GetConnectionCredentials(peer)));
    QObject::connect(&request->watcher, &QDBusPendingCallWatcher::finished,
                     [this, peer](QDBusPendingCallWatcher *watcher)
                     {
                         this->received_credentials(peer, *watcher);
                     });
    request->callbacks.push_back(callback);
    pending_.emplace(peer, std::move(request));
}

void CredentialsCache::received_credentials(QString const& peer, QDBusPendingReply<QVariantMap> const& reply)
{
    Credentials credentials;
    if (reply.isError())
    {
        // LCOV_EXCL_START
        qWarning() << "CredentialsCache::received_credentials(): "
            "error retrieving credentials for" << peer <<
            ":" << reply.error().message();
        // LCOV_EXCL_STOP
    }
    else
    {
        credentials.valid = true;
        // The contents of this map are described in the specification here:
        // http://dbus.freedesktop.org/doc/dbus-specification.html#bus-messages-get-connection-credentials
        credentials.user = reply.value().value(UNIX_USER_ID).value<uint32_t>();
    }

    // If we've hit our maximum cache size, start a new generation.
    if (cache_.size() >= MAX_CACHE_SIZE)
    {
        // LCOV_EXCL_START
        old_cache_ = std::move(cache_);
        cache_.clear();
        // LCOV_EXCL_STOP
    }
    cache_.emplace(peer, credentials);

    // Notify anyone waiting on the request and remove it from the map:
    for (auto& callback : pending_.at(peer)->callbacks)
    {
        callback(credentials);
    }
    pending_.erase(peer);
}
