cmake_minimum_required(VERSION 2.6 FATAL_ERROR)
project(dhewm3)

# TODO

# osx: place game .dylib's in the bundle (next to the binary)
# osx: -weak_framework ?

# maybe add these as options:
# TARGET_MONO
# SETUP
# SDK				-D_D3SDK

# don't add these as options, but document them?
# IDNET_HOST		-DIDNET_HOST=\\"%s\\"' % IDNET_HOST
# DEBUG_MEMORY		-DID_DEBUG_MEMORY', '-DID_REDIRECT_NEWDELETE
# LIBC_MALLOC		-DUSE_LIBC_MALLOC=0
# ID_NOLANADDRESS	-DID_NOLANADDRESS

# fallback for cmake versions without add_compile_options
if(NOT COMMAND add_compile_options)
	function(add_compile_options)
		foreach(arg ${ARGN})
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${arg}" PARENT_SCOPE)
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${arg}" PARENT_SCOPE)
		endforeach()
	endfunction()
endif()

option(LINUX_RELEASE_BINS "Set RPATH to \$ORIGIN/libs/ for Linux binary releases" OFF)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/sys/cmake")

if(LINUX_RELEASE_BINS)
	message(STATUS "Setting RPATH to \$ORIGIN/libs/ so you can put dependencies in there")
	set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)
	set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
	set(CMAKE_INSTALL_RPATH "$ORIGIN/libs")
	set(ldflags "${ldflags} -Wl,-z,origin") # not sure this is even needed, but shouldn't hurt
else()
	set(CMAKE_SKIP_RPATH ON CACHE BOOL "Skip RPATH" FORCE)
endif()

set(DHEWM3BINARY "dhewm3")

include(CheckCXXCompilerFlag)
include(GNUInstallDirs OPTIONAL RESULT_VARIABLE GNUINSTALLDIRS)

option(DEDICATED	"Build the master server" ON)
option(ONATIVE		"Optimize for the host CPU" OFF)
option(SDL2			"Use SDL2 instead of SDL1.2" ON)

if(NOT CMAKE_SYSTEM_PROCESSOR)
	message(FATAL_ERROR "No target CPU architecture set")
endif()

if(NOT CMAKE_SYSTEM_NAME)
	message(FATAL_ERROR "No target OS set")
endif()

# target cpu
set(cpu ${CMAKE_SYSTEM_PROCESSOR})
if(cpu STREQUAL "powerpc")
	set(cpu "ppc")
elseif(cpu MATCHES "i.86")
	set(cpu "x86")
endif()

if(MSVC AND CMAKE_CL_64)
	set(cpu "amd64")
endif()

# target os
if(APPLE)
	set(os "macosx")
else()
	string(TOLOWER "${CMAKE_SYSTEM_NAME}" os)
endif()

# build type
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "RelWithDebInfo")
endif()

# precompiled libraries from the dhewm3-libs repo
if(DHEWM3LIBS)
	if(CMAKE_CROSSCOMPILING)
		set(CMAKE_FIND_ROOT_PATH ${DHEWM3LIBS})
	else()
		set(ENV{CMAKE_PREFIX_PATH} ${DHEWM3LIBS})
	endif()

	# these are too stupid, give them a hint
	set(ENV{OPENALDIR} ${DHEWM3LIBS})
	set(ENV{SDLDIR} ${DHEWM3LIBS})
	set(ENV{SDL2DIR} ${DHEWM3LIBS})
endif()

# libs
find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIRS})

find_package(JPEG REQUIRED)
include_directories(${JPEG_INCLUDE_DIR})

set(CMAKE_REQUIRED_INCLUDES ${JPEG_INCLUDE_DIR})
set(CMAKE_REQUIRED_LIBRARIES ${JPEG_LIBRARY})

find_package(OGG REQUIRED)
include_directories(${OGG_INCLUDE_DIR})

find_package(Vorbis REQUIRED)
include_directories(${VORBIS_INCLUDE_DIR})

find_package(VorbisFile REQUIRED)
include_directories(${VORBISFILE_INCLUDE_DIR})

find_package(OpenAL REQUIRED)
include_directories(${OPENAL_INCLUDE_DIR})

if(NOT AROS)
	find_package(X11 REQUIRED)
	include_directories(${X11_INCLUDE_DIR})
endif()

if (SDL2)
	# skip SDL2main
	if(APPLE OR WIN32)
		set(SDL2_BUILDING_LIBRARY TRUE)
	endif()
	find_package(SDL2 REQUIRED)
	include_directories(${SDL2_INCLUDE_DIR})
	set(SDLx_LIBRARY ${SDL2_LIBRARY})
else()
	# skip SDLmain
	if(APPLE OR WIN32)
		set(SDL_BUILDING_LIBRARY TRUE)
	endif()
	find_package(SDL REQUIRED)
	include_directories(${SDL_INCLUDE_DIR})
	set(SDLx_LIBRARY ${SDL_LIBRARY})
endif()

find_package(CURL QUIET)
if(CURL_FOUND)
	set(ID_ENABLE_CURL ON)
	include_directories(${CURL_INCLUDE_DIR})
else()
	message(STATUS "libcurl not found, server downloads won't be available")
	set(ID_ENABLE_CURL OFF)
	set(CURL_LIBRARY "")
endif()

# compiler specific flags
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
	add_compile_options(-pipe)
	add_compile_options(-Wall)

	if(NOT CMAKE_CROSSCOMPILING AND ONATIVE)
		add_compile_options(-march=native)
	elseif(NOT APPLE AND cpu STREQUAL "x86")
		add_compile_options(-march=pentium3)
	endif()

	set(CMAKE_C_FLAGS_DEBUG "-g -ggdb -D_DEBUG -O0")
	set(CMAKE_C_FLAGS_DEBUGALL "-g -ggdb -D_DEBUG")
	set(CMAKE_C_FLAGS_PROFILE "-g -ggdb -D_DEBUG -O1 -fno-omit-frame-pointer")
	set(CMAKE_C_FLAGS_RELEASE "-O2 -fno-unsafe-math-optimizations -fno-math-errno -fno-trapping-math -fomit-frame-pointer")
	set(CMAKE_C_FLAGS_RELWITHDEBINFO "-g -ggdb -O2 -fno-unsafe-math-optimizations -fno-math-errno -fno-trapping-math -fno-omit-frame-pointer")
	set(CMAKE_C_FLAGS_MINSIZEREL "-Os -fno-unsafe-math-optimizations -fno-math-errno -fno-trapping-math -fomit-frame-pointer")

	set(CMAKE_CXX_FLAGS_DEBUGALL ${CMAKE_C_FLAGS_DEBUGALL})
	set(CMAKE_CXX_FLAGS_PROFILE ${CMAKE_C_FLAGS_PROFILE})

	add_compile_options(-fno-strict-aliasing)

	if(NOT AROS)
		CHECK_CXX_COMPILER_FLAG("-fvisibility=hidden" cxx_has_fvisibility)
		if(NOT cxx_has_fvisibility)
			message(FATAL_ERROR "Compiler does not support -fvisibility")
		endif()
		add_compile_options(-fvisibility=hidden)
	endif()

	# TODO fix these warnings
	add_compile_options(-Wno-sign-compare)
	add_compile_options(-Wno-switch)
	add_compile_options(-Wno-format-security)

	CHECK_CXX_COMPILER_FLAG("-Woverloaded-virtual" cxx_has_Woverload_virtual)
	if(cxx_has_Woverload_virtual)
		add_compile_options(-Woverloaded-virtual)
	endif()

	if(AROS)
		set(CMAKE_SHARED_LIBRARY_SUFFIX ".aros-${cpu}")
		add_definitions(-DIOAPI_NO_64)
	elseif(APPLE)
		add_definitions(-DMACOS_X=1)

		if(cpu STREQUAL "x86_64")
			add_compile_options(-arch x86_64 -mmacosx-version-min=10.6)
			set(ldflags "${ldflags} -arch x86_64 -mmacosx-version-min=10.6")
		elseif(cpu STREQUAL "x86")
			CHECK_CXX_COMPILER_FLAG("-arch i386" cxx_has_arch_i386)
			if(cxx_has_arch_i386)
				add_compile_options(-arch i386)
				set(ldflags "${ldflags} -arch i386")
			endif()

			add_compile_options(-mmacosx-version-min=10.4)
			set(ldflags "${ldflags} -mmacosx-version-min=10.4")
		elseif(cpu STREQUAL "ppc")
			CHECK_CXX_COMPILER_FLAG("-arch ppc" cxx_has_arch_ppc)
			if(cxx_has_arch_ppc)
				add_compile_options(-arch ppc)
				set(ldflags "${ldflags} -arch ppc")
			endif()

			add_compile_options(-mmacosx-version-min=10.4)
			set(ldflags "${ldflags} -mmacosx-version-min=10.4")
		else()
			message(FATAL_ERROR "Unsupported CPU architecture for OSX")
		endif()

		set(sys_libs ${sys_libs} "-framework Carbon -framework Cocoa -framework IOKit")
	elseif(WIN32)
		set(ldflags "${ldflags} -static-libgcc -static-libstdc++")
	elseif(os STREQUAL "linux")
		set(sys_libs ${sys_libs} dl)
	endif()
elseif(MSVC)
	add_compile_options(/W4)
	add_compile_options(/wd4100) # unreferenced formal parameter
	add_compile_options(/wd4127) # conditional expression is constant
	add_compile_options(/wd4244) # possible loss of data
	add_compile_options(/wd4245) # signed/unsigned mismatch
	add_compile_options(/wd4267) # possible loss of data
	add_compile_options(/wd4714) # 'function' marked as __forceinline not inlined
	add_compile_options(/wd4996) # 'function': was declared deprecated
	add_compile_options(/wd4068) # unknown pragma
	add_compile_options(/wd4458) # declaration of 'variable' hides class member
	add_definitions(-D_ALLOW_KEYWORD_MACROS) # because of the "#define private public" and "#define protected public" in TypeInfo.cpp
	set(CMAKE_C_FLAGS_DEBUG "-D_DEBUG /Od /Zi /MDd")
	set(CMAKE_C_FLAGS_RELEASE "/Ox /Oy /MD")
	set(CMAKE_C_FLAGS_RELWITHDEBINFO "/Ox /Oy /Zi /MD")
	set(CMAKE_C_FLAGS_MINSIZEREL "/Ox /Oy /Os /MD")
else()
	message(FATAL_ERROR "Unsupported compiler")
endif()

set(CMAKE_CXX_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_C_FLAGS_RELWITHDEBINFO})
# disable assertions in release builds
set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG ${CMAKE_C_FLAGS_RELEASE}")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-DNDEBUG ${CMAKE_C_FLAGS_MINSIZEREL}")

# mingw and msvc
if(WIN32)
	add_definitions(-DWINVER=0x0501)
	add_definitions(-D_WIN32_WINNT=0x0501)

	set(sys_libs ${sys_libs}
		winmm
		iphlpapi
		wsock32
		ole32
	)
endif()

# fallback for cmake versions without GNUInstallDirs
if(GNUINSTALLDIRS MATCHES "NOTFOUND")
	set(CMAKE_INSTALL_BINDIR		"bin"
		CACHE PATH "user executables (bin)")
	set(CMAKE_INSTALL_LIBDIR		"lib${LIB_SUFFIX}"
		CACHE PATH "object code libraries (lib${LIB_SUFFIX})")
	set(CMAKE_INSTALL_DATAROOTDIR	"share"
		CACHE PATH "read-only architecture-independent data root (share)")
	set(CMAKE_INSTALL_DATADIR		"${CMAKE_INSTALL_DATAROOTDIR}"
		CACHE PATH "read-only architecture-independent data (DATAROOTDIR)")

	mark_as_advanced(CMAKE_INSTALL_BINDIR CMAKE_INSTALL_LIBDIR CMAKE_INSTALL_DATAROOTDIR CMAKE_INSTALL_DATADIR)
	foreach(dir BINDIR LIBDIR DATAROOTDIR DATADIR)
		if(NOT IS_ABSOLUTE ${CMAKE_INSTALL_${dir}})
			set(CMAKE_INSTALL_FULL_${dir} "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_${dir}}")
		else()
			set(CMAKE_INSTALL_FULL_${dir} "${CMAKE_INSTALL_${dir}}")
		endif()
	endforeach()
endif()

set(bindir		"${CMAKE_INSTALL_FULL_BINDIR}")
set(libdir		"${CMAKE_INSTALL_FULL_LIBDIR}/dhewm3")
set(datadir		"${CMAKE_INSTALL_FULL_DATADIR}/dhewm3")

configure_file(
	"${CMAKE_SOURCE_DIR}/config.h.in"
	"${CMAKE_BINARY_DIR}/config.h"
)

message(STATUS "Building ${CMAKE_BUILD_TYPE} for ${os}-${cpu}")

if(NOT APPLE AND NOT WIN32)
	message(STATUS "The install target will use the following directories:")
	message(STATUS "  Binary directory:  ${bindir}")
	message(STATUS "  Library directory: ${libdir}")
	message(STATUS "  Data directory:    ${datadir}")
endif()


set(src_framework
	framework/CVarSystem.cpp
	framework/CmdSystem.cpp
	framework/Common.cpp
	framework/Console.cpp
	framework/Compressor.cpp
	framework/DemoFile.cpp
	framework/EditField.cpp
	framework/EventLoop.cpp
	framework/File.cpp
	framework/FileSystem.cpp
	framework/KeyInput.cpp
	framework/UsercmdGen.cpp
	framework/async/AsyncNetwork.cpp
	framework/async/AsyncServer.cpp
	framework/async/MsgChannel.cpp
	framework/async/NetworkSystem.cpp
	framework/minizip/ioapi.c
	framework/minizip/unzip.cpp
)


set(src_idlib
	idlib/hashing/CRC32.cpp
	idlib/hashing/MD4.cpp
	idlib/hashing/MD5.cpp
	idlib/math/Angles.cpp
	idlib/math/Lcp.cpp
	idlib/math/Math.cpp
	idlib/math/Matrix.cpp
	idlib/math/Ode.cpp
	idlib/math/Plane.cpp
	idlib/math/Pluecker.cpp
	idlib/math/Polynomial.cpp
	idlib/math/Quat.cpp
	idlib/math/Rotation.cpp
	idlib/math/Simd.cpp
	idlib/math/Simd_Generic.cpp
	idlib/math/Simd_AltiVec.cpp
	idlib/math/Simd_MMX.cpp
	idlib/math/Simd_3DNow.cpp
	idlib/math/Simd_SSE.cpp
	idlib/math/Simd_SSE2.cpp
	idlib/math/Simd_SSE3.cpp
	idlib/math/Vector.cpp
	idlib/BitMsg.cpp
	idlib/LangDict.cpp
	idlib/Lexer.cpp
	idlib/Lib.cpp
	idlib/containers/HashIndex.cpp
	idlib/Dict.cpp
	idlib/Str.cpp
	idlib/Parser.cpp
	idlib/CmdArgs.cpp
	idlib/Token.cpp
	idlib/Base64.cpp
	idlib/Timer.cpp
	idlib/Heap.cpp
)

set(src_core
	${src_framework}
)

if(AROS)
	set(DHEWM3BINARY "ADoom3")
	set(sys_libs ${sys_libs} dll)

	set(src_arosdll
		sys/aros/dll/dllstartup.c
		sys/aros/dll/dll.c
		sys/aros/dll/dllimport.c
	)

	set(src_sys_base
		sys/cpu.cpp
		sys/threads.cpp
		sys/events.cpp
		sys/sys_local.cpp
		sys/aros/aros_net.cpp
		sys/aros/aros_signal.cpp
		sys/aros/aros_main.cpp
		sys/aros/aros_dos.cpp
	)

elseif(APPLE)
	set(OSX_RESOURCE_FILES
		"${CMAKE_SOURCE_DIR}/sys/osx/Doom3.icns"
		"${CMAKE_SOURCE_DIR}/sys/osx/Doom 3.rsrc"
	)

	set_source_files_properties(${OSX_RESOURCE_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

	set(src_sys_base
		sys/cpu.cpp
		sys/threads.cpp
		sys/events.cpp
		sys/sys_local.cpp
		sys/posix/posix_net.cpp
		sys/posix/posix_main.cpp
	)

elseif(WIN32)

	set(src_sys_base
		sys/cpu.cpp
		sys/threads.cpp
		sys/events.cpp
		sys/sys_local.cpp
		sys/win32/win_input.cpp
		sys/win32/win_main.cpp
		sys/win32/win_net.cpp
		sys/win32/win_shared.cpp
		sys/win32/win_syscon.cpp
		sys/win32/SDL_win32_main.c
	)
else()
	set(src_sys_base
		sys/cpu.cpp
		sys/threads.cpp
		sys/events.cpp
		sys/sys_local.cpp
		sys/posix/posix_net.cpp
		sys/posix/posix_main.cpp
		sys/linux/main.cpp
	)
endif()

include_directories(${CMAKE_BINARY_DIR})
include_directories(${CMAKE_SOURCE_DIR})

add_library(idlib STATIC ${src_idlib})
if (AROS)
	add_library(dll STATIC ${src_arosdll})
        if(CMAKE_SYSTEM_PROCESSOR STREQUAL "i386")
		set(AROS_ARCH "x86")
        else()
		set(AROS_ARCH ${CMAKE_SYSTEM_PROCESSOR})
	endif()
else()
	if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID STREQUAL "Clang" AND NOT MINGW)
		set_target_properties(idlib PROPERTIES COMPILE_FLAGS "-fPIC")
	endif()
endif()

if(DEDICATED)
	add_executable(${DHEWM3BINARY}masterserver WIN32 MACOSX_BUNDLE
		${src_core}
		${src_sys_base}
	)

	set_target_properties(${DHEWM3BINARY}masterserver PROPERTIES COMPILE_DEFINITIONS "ID_DEDICATED;__DOOM_DLL__")
	set_target_properties(${DHEWM3BINARY}masterserver PROPERTIES LINK_FLAGS "${ldflags}")
	target_link_libraries(${DHEWM3BINARY}masterserver
		idlib
		${VORBISFILE_LIBRARIES}
		${VORBIS_LIBRARIES}
		${OGG_LIBRARIES}
		${CURL_LIBRARY}
		${JPEG_LIBRARY}
		${ZLIB_LIBRARY}
		${SDLx_LIBRARY}
		${sys_libs}
	)

	if(NOT APPLE AND NOT WIN32)
		install(TARGETS ${DHEWM3BINARY}masterserver
				RUNTIME DESTINATION "${bindir}"
				LIBRARY DESTINATION "${libdir}"
				ARCHIVE DESTINATION "${libdir}"
		)
	endif()
endif()