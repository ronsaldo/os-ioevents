set(OSIOEvents_SRC
	internal.h
    os-ioevents-unix.c
	os-ioevents-windows.c
	threads.h
)

add_definitions(-DBUILD_OS_IOEVENTS_CORE)
add_library(os-ioevents SHARED ${OSIOEvents_SRC})
