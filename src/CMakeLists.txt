add_subdirectory(udev)

if (HAVE_LIBHYBRIS)
    include_directories(
        ${ANDROID_HEADERS_INCLUDE_DIRS}
    )

    set(LIBHYBRIS_SRC
        leds-hybris.cpp
    )

    set(LIBHYBRIS_LIBRARIES
        ${ANDROID_HARDWARE_LIBRARIES}
    )

    add_definitions(-DHAVE_LIBHYBRIS)
endif()

if (HAVE_LIBGBINDER)
    include_directories(
        ${GLIB_UTIL_INCLUDE_DIRS}
        ${GBINDER_INCLUDE_DIRS}
    )

    set(LIBGBINDER_SRC
        leds-binder.cpp
        vibrator-binder.cpp
    )

    set(LIBGBINDER_LIBRARIES
        ${GLIB_UTIL_LDFLAGS} ${GLIB_UTIL_LIBRARIES}
        ${GBINDER_LDFLAGS} ${GBINDER_LIBRARIES}
    )

    add_definitions(-DHAVE_LIBGBINDER)
endif()

add_library(hfd-core STATIC
    leds.cpp
    leds-sysfs.cpp

    vibrator.cpp
    vibrator-ff.cpp
    vibrator-sysfs.cpp
    vibrator-legacy.cpp

    ${LIBHYBRIS_SRC}
    ${LIBGBINDER_SRC}
)

target_link_libraries(hfd-core
    udev-cpp
    ${LIBHYBRIS_LIBRARIES}
    ${LIBGBINDER_LIBRARIES}
)

include_directories(
    ${DEVICEINFO_INCLUDE_DIRS}
)

set(SERVICE_SRC
    credentialscache.cpp
    credentialscache.h
    service.cpp
    dbusAdaptor.h
    usersettings.cpp
    usersettings.h
    vibrator-controller.cpp
    vibrator-controller.h
)

set_source_files_properties(bus.xml PROPERTIES
  CLASSNAME BusInterface
  NO_NAMESPACE TRUE)
qt5_add_dbus_interface(interface_files bus.xml businterface)

set_source_files_properties(properties.xml PROPERTIES
  CLASSNAME PropertiesInterface
  NO_NAMESPACE TRUE)
qt5_add_dbus_interface(interface_files properties.xml propertiesinterface)

set(ACCOUNTS_INTERFACE_FILE "${DBUS_INTERFACES_DIR}/org.freedesktop.Accounts.xml")
set_source_files_properties(${ACCOUNTS_INTERFACE_FILE} PROPERTIES
  CLASSNAME ASInterface
  NO_NAMESPACE TRUE)
qt5_add_dbus_interface(interface_files ${ACCOUNTS_INTERFACE_FILE} asinterface)

qt5_add_dbus_adaptor(SERVICE_SRC
    ${CMAKE_SOURCE_DIR}/data/com.lomiri.hfd.xml ${CMAKE_CURRENT_SOURCE_DIR}/dbusAdaptor.h DbusAdaptor
)

add_executable(hfd-service
    ${SERVICE_SRC}
    ${interface_files}
)

target_link_libraries(hfd-service
    hfd-core

    Qt5::Core
    Qt5::DBus
    ${DEVICEINFO_LDFLAGS}
)

install(TARGETS hfd-service RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR})
