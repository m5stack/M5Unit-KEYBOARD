# Dependencies via main/idf_component.yml. PlotToSerial selects the unit via Kconfig
# (main/Kconfig.projbuild -> common/Kconfig.variant); variant.cmake maps it to USING_* macro(s).
idf_component_register(
    SRCS "PlotToSerial.cpp"
    INCLUDE_DIRS ".")

# Apply the selected variant macro after the register call (it needs ${COMPONENT_LIB}).
include("${CMAKE_CURRENT_LIST_DIR}/../../common/variant.cmake")
