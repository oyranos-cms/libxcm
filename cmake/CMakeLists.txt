#
# Target installation folders
#

SET (TARGET_INCLUDE_FOLDER
    X11/Xcm
		CACHE STRING
		"Optional folder below system include folder to install include files."
    )

IF(NOT CMAKE_MODULES_INSTALL_DIR)
  SET (CMAKE_MODULES_INSTALL_DIR
		"${CMAKE_INSTALL_LIBDIR}/cmake/${PACKAGE_NAME}"
		CACHE STRING
		"The folder where to install cmake files."
    )
ENDIF(NOT CMAKE_MODULES_INSTALL_DIR)

SET (TARGET_PLUGIN_FOLDER "xcm"
		CACHE STRING
		"Optional folder below system library folder where to install openicc plugins."
    )

SET (TARGET_PKGCONFIG_FOLDER
		"pkgconfig"
		CACHE STRING
		"The folder below system library folder where to install pkgconfig files."
    )

SET (TARGET_DOCUMENTATION_FOLDER
		"share/doc/xcm-api"
		CACHE STRING
		"The folder below system prefix where to install api documentation files."
    )


#
# Misc.
#

#cmake settings for use by find_package
INSTALL ( FILES
          "${CMAKE_CURRENT_SOURCE_DIR}/XcmConfig.cmake"
          DESTINATION ${CMAKE_MODULES_INSTALL_DIR}
          COMPONENT dev
        )

