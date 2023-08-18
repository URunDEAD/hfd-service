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

#include <functional>

#include <QDBusConnection>
#include <QMap>
#include <QObject>

#include "asinterface.h"
#include "propertiesinterface.h"

class UserSettings : public QObject
{
    Q_OBJECT
public:
    typedef std::function<void(bool allowed)> callback_t;

    UserSettings(QDBusConnection bus, QObject * parent = nullptr);
    void getSettingsForUid(uid_t uid, callback_t cb);

private:
    struct UserInfo {
        PropertiesInterface * properties;
        bool allowed;
        bool valid;

        QList<callback_t> pendingRequests;
    };

    void onListCachedUsersFinished(QDBusPendingCallWatcher * call);

    UserInfo & initUser(uid_t uid, QString const & userPath);
    void userSetObjPath(UserInfo & user, QString const & userPath);
    void userGetAllowed(UserInfo & user);
    void onGetFinished(UserInfo & user, QDBusPendingCallWatcher * call);
    void onPropertiesChanged(UserInfo & user, const QString &interface_name, const QVariantMap &changed_properties, const QStringList &invalidated_properties);

    void userAllowedChanged(UserInfo & user, bool allowed);

    void onUserDeleted(const QDBusObjectPath & userObjPath);

    QMap<uid_t, UserInfo> m_infoForUid;

    QDBusConnection m_bus;
    ASInterface * m_accountsService;
};
