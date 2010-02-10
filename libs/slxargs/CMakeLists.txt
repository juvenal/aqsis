project(slxargs)

set(slxargs_srcs
	slo.cpp
	slx.cpp
)

include_directories(
	${AQSIS_OPENEXR_INCLUDE_DIR})

aqsis_add_library(aqsis_slxargs ${slxargs_srcs} ${slxargs_hdrs}
	COMPILE_DEFINITIONS AQSIS_SLXARGS_EXPORTS
	LINK_LIBRARIES aqsis_shadervm
)

aqsis_install_targets(aqsis_slxargs)

