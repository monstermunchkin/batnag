#cmake_minimum_required(VERSION 3.0)
cmake_minimum_required(VERSION 2.8.7)

include(CheckIncludeFile)
include(FindPkgConfig)

# project(batnag VERSION 0.6 LANGUAGES C)
project(batnag C)
set(PROJECT_VERSION 0.6)

check_include_file(errno.h HAVE_ERRNO_H)
check_include_file(getopt.h HAVE_GETOPT_H)
check_include_file(signal.h HAVE_SIGNAL_H)
check_include_file(stdbool.h HAVE_STDBOOL_H)
check_include_file(stdint.h HAVE_STDINT_H)
check_include_file(stdio.h HAVE_STDIO_H)
check_include_file(stdlib.h HAVE_STDLIB_H)
check_include_file(string.h HAVE_STRING_H)
check_include_file(strings.h HAVE_STRING_H)
check_include_file(sys/wait.h HAVE_SYS_WAIT_H)
check_include_file(unistd.h HAVE_UNISTD_H)

#add_compile_options(-D_POSIX_C_SOURCE=200809L -std=c99 -Wall -Wextra -Wpedantic -pedantic-errors -O2 -g)
set(CMAKE_C_FLAGS "-D_POSIX_C_SOURCE=200809L -std=c99 -Wall -Wextra -Werror -pedantic-errors -O2 -g")

# config.h
set(BATSTAT /sys/class/power_supply/BAT0/status)
set(BATCAP /sys/class/power_supply/BAT0/capacity)

# set main source files
set(SRCS src/main.c src/nagbar.c)

# options
option(USE_NAGBAR "enable i3-nagbar" ON)
option(USE_LIBNOTIFY "enable libnotify" ON)

# i3-nagbar
if(USE_NAGBAR)
	find_program(NAGBAR i3-nagbar)
	if(NAGBAR)
		set(SRCS ${SRCS} src/mod-nagbar.c)
	else(NAGBAR)
		set(USE_NAGBAR OFF)
	endif(NAGBAR)
endif(USE_NAGBAR)

# libnotify
if(USE_LIBNOTIFY)
	pkg_check_modules(LIBNOTIFY libnotify)
	if(LIBNOTIFY_FOUND)
		set(SRCS ${SRCS} src/mod-libnotify.c)
		include_directories(${LIBNOTIFY_INCLUDE_DIRS})
		set(EXTRA_LIBS ${LIBNOTIFY_LIBRARIES})
	else(LIBNOTIFY_FOUND)
		set(USE_LIBNOTIFY OFF)
	endif(LIBNOTIFY_FOUND)
endif(USE_LIBNOTIFY)

configure_file("config.h.in" src/config.h)
add_executable(batnag ${SRCS})
target_link_libraries(batnag ${EXTRA_LIBS})

install(TARGETS batnag DESTINATION /usr/local/bin)
set(CMAKE_VERBOSE_MAKEFILE ON)
