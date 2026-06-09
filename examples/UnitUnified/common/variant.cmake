# Map the Kconfig "Target unit / board" choice (see common/Kconfig.variant) to the source-level
# USING_* macro(s) that both the Arduino and ESP-IDF builds use. Include this from an example's
# main/CMakeLists.txt *after* idf_component_register() (it needs ${COMPONENT_LIB}).
# Default (nothing selected): UnitCardKB (I2C).
if(CONFIG_EXAMPLE_USING_UNIT_CARDKB2_I2C)
    set(M5UNIT_VARIANT USING_UNIT_CARDKB2 USING_I2C_FOR_CARDKB2)
elseif(CONFIG_EXAMPLE_USING_UNIT_CARDKB2_UART)
    set(M5UNIT_VARIANT USING_UNIT_CARDKB2 USING_UART_FOR_CARDKB2)
elseif(CONFIG_EXAMPLE_USING_UNIT_FACES_QWERTY)
    set(M5UNIT_VARIANT USING_UNIT_FACES_QWERTY)
elseif(CONFIG_EXAMPLE_USING_UNIT_TAB5_KEYBOARD)
    set(M5UNIT_VARIANT USING_UNIT_TAB5_KEYBOARD)
else()
    set(M5UNIT_VARIANT USING_UNIT_CARDKB)
endif()
target_compile_definitions(${COMPONENT_LIB} PRIVATE ${M5UNIT_VARIANT})
