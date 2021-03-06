# Copyright 2012 Johannes Zarl <isilmendil@gmx.net>
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
# This file is under the public domain and can be reused without restrictions.

if ( NOT DEFINED BASE_DIR )
	message ( FATAL_ERROR "UpdateVersion.cmake: BASE_DIR not set. Please supply base working directory!" )
endif()

# git or tarball?
if ( EXISTS "${BASE_DIR}/.git" )
	# -> git:
	include ( "${CMAKE_CURRENT_LIST_DIR}/GitDescription.cmake" )

	git_get_description ( KPA_VERSION GIT_ARGS --dirty )
	if ( NOT KPA_VERSION )
		set ( KPA_VERSION "unknown" )
	endif()

	message ( STATUS "Updating version information to ${KPA_VERSION}..." )
	# write version info to a temporary file
	configure_file ( "${BASE_DIR}/version.h.in" "${BASE_DIR}/version.h~" )
	# update info iff changed
	execute_process ( COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${BASE_DIR}/version.h~" "${BASE_DIR}/version.h" )
	# make sure info doesn't get stale
	file ( REMOVE "${BASE_DIR}/version.h~" )
else()
	# -> tarball
	if ( NOT EXISTS "${BASE_DIR}/version.h" )
		message ( SEND_ERROR "The generated file 'version.h' does not exist!" )
		message ( AUTHOR_WARNING "When creating a release tarball, please make sure to run cmake -P ${CMAKE_CURRENT_LIST_FILE}" )
	endif()
endif()
