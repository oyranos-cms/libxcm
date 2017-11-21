#
# Target installation folders
#

SET (TARGET_INCLUDE_FOLDER
    X11/Xcm
		CACHE STRING
		"Optional folder below system include folder to install include files."
    )

SET (TARGET_CMAKE_FOLDER
		"cmake/Xcm"
		CACHE STRING
		"The folder below system prefix where to install cmake files."
    )

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
          DESTINATION ${CMAKE_INSTALL_LIBDIR}/${TARGET_CMAKE_FOLDER}
          COMPONENT dev
          RENAME FindXcm.cmake
        )

