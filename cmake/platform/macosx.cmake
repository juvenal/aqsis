SET(BINDIR "bin" 
	CACHE STRING "Install location for binary files. (relative to CMAKE_INSTALL_PREFIX)")
SET(LIBDIR "lib" 
	CACHE STRING "Install location for shared libraries (relative to CMAKE_INSTALL_PREFIX)")
SET(DISPLAYSDIR "${LIBDIR}/aqsis" 
	CACHE STRING "Install location for display libraries (relative to CMAKE_INSTALL_PREFIX)")
SET(SHADERDIR "share/aqsis/shaders" 
	CACHE STRING "Install location for shaders (relative to CMAKE_INSTALL_PREFIX)")
SET(SYSCONFDIR "etc/aqsis" 
	CACHE STRING "Install location for system configuration files (relative to CMAKE_INSTALL_PREFIX)")
SET(INCLUDEDIR "include/aqsis" 
	CACHE STRING "Install location for aqsis header files (relative to CMAKE_INSTALL_PREFIX)")
SET(CONTENTDIR_NAME "content"
	CACHE STRING "Name of content directory")
SET(CONTENTDIR "share/aqsis/${CONTENTDIR_NAME}" 
	CACHE STRING "Install location for content (relative to CMAKE_INSTALL_PREFIX)")
SET(SCRIPTSDIR_NAME "scripts"
	CACHE STRING "Name of scripts directory")
SET(SCRIPTSDIR "share/aqsis/${SCRIPTSDIR_NAME}" 
	CACHE STRING "Install location for scripts (relative to CMAKE_INSTALL_PREFIX)")

IF(AQSIS_USE_POINTCLOUD)
	SET(SHADERPATH "${SHADERPATH}${CMAKE_INSTALL_PREFIX}/${LIBDIR}:")
ENDIF(AQSIS_USE_POINTCLOUD)
IF(AQSIS_ENABLE_MASSIVE)
	SET(PROCEDURALPATH "%MASSIVE_HOME%/bin:")
ENDIF(AQSIS_ENABLE_MASSIVE)

# Ensure that the Carbons libraries are found and used appropriately
INCLUDE_DIRECTORIES ( /Developer/Headers/FlatCarbon )
FIND_LIBRARY(CARBON_LIBRARY Carbon)
#FIND_LIBRARY(QUICKTIME_LIBRARY QuickTime )
FIND_LIBRARY(APP_SERVICES_LIBRARY ApplicationServices )
MARK_AS_ADVANCED (CARBON_LIBRARY)
#MARK_AS_ADVANCED (QUICKTIME_LIBRARY)
MARK_AS_ADVANCED (APP_SERVICES_LIBRARY)

FILE(RELATIVE_PATH LIB_REL_TO_BIN ${CMAKE_INSTALL_PREFIX}/${BINDIR} ${CMAKE_INSTALL_PREFIX}/${LIBDIR})
SET(CMAKE_INSTALL_NAME_DIR "@executable_path/${LIB_REL_TO_BIN}")

