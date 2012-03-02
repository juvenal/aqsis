PROJECT(OSXBUNDLE)

set(CMAKE_PROJECT_NAME "${AQSIS_PROJECT_NAME_SHORT}"
	CACHE STRING "Package name for OS X"
	)
set(AQSIS_PACKAGE_NAME "${CMAKE_PROJECT_NAME}-${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_BUILD}-${CMAKE_SYSTEM_NAME}.dmg"
	CACHE STRING "Package filename for OS X"
	)
	
set(BUNDLE_RESOURCES "%AQSISHOME%/Contents/Resources")
set(BUNDLE_SHADERDIR "${BUNDLE_RESOURCES}/share/aqsis/shaders")

set(display_DISPLAYLIB "file_dspy.so")
set(d_exr_DISPLAYLIB "exr_dspy.so")
set(d_bmp_DISPLAYLIB "bmp_dspy.so")
set(d_xpm_DISPLAYLIB "xpm_dspy.so")
set(piqsl_DISPLAYLIB "piqsl_dspy.so")

set(DEFAULT_SHADERPATH "${BUNDLE_SHADERDIR}/displacement:${BUNDLE_SHADERDIR}/surface:${BUNDLE_SHADERDIR}/light:${BUNDLE_SHADERDIR}/imager:${BUNDLE_SHADERDIR}/volume")
set(DEFAULT_ARCHIVEPATH "${BUNDLE_RESOURCES}")
set(DEFAULT_TEXTUREPATH "${BUNDLE_RESOURCES}")
set(DEFAULT_DISPLAYPATH "${BUNDLE_RESOURCES}/lib")
set(DEFAULT_PROCEDURALPATH "${BUNDLE_RESOURCES}/lib")
set(DEFAULT_RESOURCEPATH "${BUNDLE_RESOURCES}")
set(SHADERPATH "${BUNDLE_SHADERDIR}/displacement:${BUNDLE_SHADERDIR}/surface:${BUNDLE_SHADERDIR}/light:${BUNDLE_SHADERDIR}/imager:${BUNDLE_SHADERDIR}/volume" CACHE STRING "Aqsis shader searchpath")
set(ARCHIVEPATH "${BUNDLE_RESOURCES}" CACHE STRING "Aqsis archive searchpath")
set(TEXTUREPATH "${DEFAULT_TEXTUREPATH}" CACHE STRING "Aqsis texture searchpath")
set(DISPLAYPATH "${BUNDLE_RESOURCES}/lib" CACHE STRING "Aqsis display searchpath")
set(PROCEDURALPATH "${BUNDLE_RESOURCES}/lib" CACHE STRING "Aqsis procedural searchpath")
set(RESOURCEPATH "${DEFAULT_RESOURCEPATH}" CACHE STRING "Aqsis resource searchpath")

configure_file(${CMAKE_SOURCE_DIR}/aqsisrc.in.cmake ${BUNDLEDIR}/aqsisrc)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in.cmake ${BUNDLEDIR}/Info.plist)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../application.icns ${BUNDLEDIR}/${AQSIS_PROJECT_NAME_SHORT}.icns COPYONLY)
file(WRITE "${BUNDLEDIR}/PkgInfo" "APPL????")
install(FILES "${BUNDLEDIR}/bundle_config.app/Contents/MacOS/applet" 
		DESTINATION "../MacOS"
		PERMISSIONS OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
install(FILES "${BUNDLEDIR}/bundle_config.app/Contents/Resources/applet.rsrc"
		DESTINATION ".")
install(FILES "${BUNDLEDIR}/bundle_config.app/Contents/Resources/Scripts/main.scpt"
		DESTINATION "Scripts")
install(FILES "${BUNDLEDIR}/aqsisrc"
		DESTINATION "bin")
install(FILES "${BUNDLEDIR}/PkgInfo"
		DESTINATION "../")
		
ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/bundle_config.app
	COMMAND ${AQSIS_OSACOMPILE_EXECUTABLE} -o ${BUNDLEDIR}/bundle_config.app ${CMAKE_CURRENT_SOURCE_DIR}/bundle_config.applescript
	)
ADD_CUSTOM_TARGET(bundleconfig ALL echo
	DEPENDS  ${CMAKE_CURRENT_BINARY_DIR}/bundle_config.app
	)
