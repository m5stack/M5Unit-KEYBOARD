# ESP-IDF component registration for M5Unit-KEYBOARD.
file(GLOB_RECURSE SRCS "src/*.cpp")
idf_component_register(
    SRCS ${SRCS}
    INCLUDE_DIRS "src"
    REQUIRES M5UnitUnified            # included by public headers
    PRIV_REQUIRES M5Utility driver    # used only in .cpp (M5Utility.hpp / driver/gpio.h)
)
