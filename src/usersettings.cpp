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

#include "usersettings.h"

#include <functional>

#include <QDBusPendingCallWatcher>

const auto AS_SERVICE = QStringLiteral("org.freedesktop.Accounts");
const auto AS_PATH = QStringLiteral("/org/freedesktop/Accounts");

const auto ASSETTINGS_INTERFACE = QStringLiteral("com.lomiri.hfd.AccountsService.Settings");
const auto ASSETTINGS_ALLOWGENERALVIBRATION = QStringLiteral("AllowGeneralVibration");

UserSettings::UserSettings(QDBusConnection bus, QObject *parent)
    : QObject(parent)
    , m_bus(bus)
    , m_accountsService(new ASInterface(
        AS_SERVICE, AS_PATH, bus, /* parent */ this))
{
    // Allow testing asychronous initial value case.
    if (qEnvironmentVariableIsSet("HFD_USERSETTINGS_SKIP_CACHED_USERS"))
        return;

    auto listCachedUsersAsync = m_accountsService->ListCachedUsers();
    auto watcher = new QDBusPendingCallWatcher(listCachedUsersAsync, this);
    QObject::connect(watcher, &QDBusPendingCallWatcher::finished, this,
                     &UserSettings::onListCachedUsersFinished);

    QObject::connect(m_accountsService, &ASInterface::UserDeleted,
                     this, &UserSettings::onUserDeleted);
}

void UserSettings::onListCachedUsersFinished(QDBusPendingCallWatcher * call)
{
    QDBusPendingReply<QList<QDBusObjectPath>> reply = *call;

    if (reply.isError()) {
        qWarning() << "AccountsService fails to list cached users."
                   << reply.error();
        return;
    }

    for (auto const & userObjPath : reply.argumentAt<0>()) {
        auto userPath = userObjPath.path();

        // The path looks like: `/org/freedesktop/Accounts/User1000`
        bool ok;
        uid_t uid = userPath.midRef(userPath.lastIndexOf("User") + 4).toInt(&ok, /* base */ 10);
        if (!ok) {
            // Normally we would have to retrieve property org.freedesktop.Accounts.User.Uid,
            // but I think it would be rare...
            qWarning() << "Unable to parse the object path" << userPath;
            continue;
        }

        // In case a request arrives before we're called.
        if (m_infoForUid.contains(uid)) {
            return;
        }

        initUser(uid, userPath);
    }

    call->deleteLater();
}

UserSettings::UserInfo & UserSettings::initUser(uid_t uid, QString const & userObjPath)
{
    using namespace std::placeholders;

    auto & user = m_infoForUid[uid] = UserInfo {
        .properties = nullptr,
        .allowed = false,
        .valid = false,
        .pendingRequests = {},
    };

    if (!userObjPath.isNull()) {
        userSetObjPath(user, userObjPath);
    } else {
        // The purpose of FindUserById isn't to just get the path (since the
        // format is static), but to ensure that path exists and is exported
        // on the bus.
        auto findUserByIdAsync = m_accountsService->FindUserById(uid);
        auto watcher = new QDBusPendingCallWatcher(findUserByIdAsync, this);
        QObject::connect(
            watcher, &QDBusPendingCallWatcher::finished,
            this, [this, uid, &user] (QDBusPendingCallWatcher * call) {
                QDBusPendingReply<QDBusObjectPath> reply = *call;

                if (reply.isError()) {
                    qWarning() << "AccountsService fails to create object for "
                                  "user " << uid << ", defaults to deny.";
                    userAllowedChanged(user, false);
                } else {
                    userSetObjPath(user, reply.argumentAt<0>().path());
                }

                call->deleteLater();
            });
    }

    return user;
}

void UserSettings::userSetObjPath(UserSettings::UserInfo & user, QString const & userPath) {
    using namespace std::placeholders;

    user.properties = new PropertiesInterface(AS_SERVICE, userPath, m_bus, this);

    // Connect to change signal first, to avoid race condition.
    QObject::connect(
        user.properties, &PropertiesInterface::PropertiesChanged,
        this, std::bind(&UserSettings::onPropertiesChanged,
            this, std::ref(user), _1, _2, _3));

    userGetAllowed(user);
}

void UserSettings::userGetAllowed(UserSettings::UserInfo & user)
{
    using namespace std::placeholders;

    auto getAsync = user.properties->Get(
        ASSETTINGS_INTERFACE, ASSETTINGS_ALLOWGENERALVIBRATION);
    auto watcher = new QDBusPendingCallWatcher(getAsync, this);
    QObject::connect(
        watcher, &QDBusPendingCallWatcher::finished,
        this, std::bind(&UserSettings::onGetFinished, this, std::ref(user), _1)
    );
}

void UserSettings::onGetFinished(UserInfo & user, QDBusPendingCallWatcher * call)
{
    QDBusPendingReply<QDBusVariant> reply = *call;
    bool allowed;

    if (reply.isError()) {
        qWarning() << "AccountsService fails to provide settings for "
                   << user.properties->path()
                   << ", defaults to not allowed."
                   << reply.error();
        allowed = false;
    } else {
        auto variant = reply.argumentAt<0>().variant();
        if (variant.type() != QVariant::Bool) {
            qWarning() << "AccountsService provides invalid value for "
                   << user.properties->path()
                   << ", defaults to not allowed."
                   << reply.error();
            allowed = false;
        } else {
            allowed = variant.value<bool>();
        }
    }

    userAllowedChanged(user, allowed);

    call->deleteLater();
}

void UserSettings::onPropertiesChanged(
    UserInfo & user,
    const QString &interface_name,
    const QVariantMap &changed_properties,
    const QStringList &invalidated_properties)
{
    if (interface_name != ASSETTINGS_INTERFACE) {
        // We don't care about this.
        return;
    }

    if (invalidated_properties.contains(ASSETTINGS_ALLOWGENERALVIBRATION)) {
        // Unfortunately we have to retrieve value asynchronously from AS again.
        userGetAllowed(user);
    } else if (changed_properties.contains(ASSETTINGS_ALLOWGENERALVIBRATION)) {
        auto variant = changed_properties[ASSETTINGS_ALLOWGENERALVIBRATION];

        if (variant.type() != QVariant::Bool) {
            qWarning() << "AccountsService provides invalid value for "
                    << user.properties->path()
                    << " during update, ignore.";
            return;
        }

        userAllowedChanged(user, variant.value<bool>());
    }
}

void UserSettings::userAllowedChanged(UserSettings::UserInfo & user, bool allowed)
{
    user.allowed = allowed;
    if (!user.valid) {
        user.valid = true;
        for (auto const & callback : qAsConst(user.pendingRequests))
            callback(user.allowed);
        user.pendingRequests = {};
    }
}

void UserSettings::onUserDeleted(const QDBusObjectPath & userObjPath)
{
    auto userPath = userObjPath.path();

    // The path looks like: `/org/freedesktop/Accounts/User1000`
    bool ok;
    uid_t uid = userPath.midRef(userPath.lastIndexOf("User") + 4).toInt(&ok, /* base */ 10);
    if (!ok) {
        qWarning() << "Unable to parse the object path" << userPath;
        return;
    }

    if (m_infoForUid.contains(uid)) {
        auto & user = m_infoForUid[uid];

        // In case any pending requests exists at all.
        for (auto const & callback : qAsConst(user.pendingRequests))
            callback(false);

        if (user.properties)
            delete user.properties;

        m_infoForUid.remove(uid);
    }
}

void UserSettings::getSettingsForUid(uid_t uid, callback_t callback)
{
    UserInfo & user = m_infoForUid.contains(uid)
        ? m_infoForUid[uid]
        : initUser(uid, /* path */ QString());

    if (user.valid) {
        callback(user.allowed);
    } else {
        user.pendingRequests.append(callback);
    }
}
