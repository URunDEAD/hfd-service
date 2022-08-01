##
##  Systemd Unit File
##

pkg_check_modules(SYSTEMD systemd)
if (${SYSTEMD_FOUND})

    pkg_get_variable(SYSTEMD_SYSTEM_DIR systemd systemdsystemunitdir)
    message (STATUS "${SYSTEMD_SYSTEM_DIR} is the systemd system unit file install dir")

    set (SYSTEMD_SYSTEM_NAME "${CMAKE_PROJECT_NAME}.service")
    set (SYSTEMD_SYSTEM_FILE "${CMAKE_CURRENT_BINARY_DIR}/${SYSTEMD_SYSTEM_NAME}")
    set (SYSTEMD_SYSTEM_FILE_IN "${CMAKE_CURRENT_SOURCE_DIR}/${SYSTEMD_SYSTEM_NAME}.in")

    # build it
    configure_file ("${SYSTEMD_SYSTEM_FILE_IN}" "${SYSTEMD_SYSTEM_FILE}")

    # install it
    install (FILES "${SYSTEMD_SYSTEM_FILE}"
             DESTINATION "${SYSTEMD_SYSTEM_DIR}")

endif()

