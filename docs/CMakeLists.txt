
message(STATUS "📚 Configuration du système de documentation...")

# === Doxygen ===
find_package(Doxygen REQUIRED)

set(DOXYGEN_IN ${CMAKE_SOURCE_DIR}/docs/Doxyfile)
set(DOXYGEN_OUT_DIR ${CMAKE_BINARY_DIR}/docs)
set(DOXYGEN_CONFIG ${DOXYGEN_OUT_DIR}/Doxyfile)

file(MAKE_DIRECTORY ${DOXYGEN_OUT_DIR} ${DOXYGEN_OUT_DIR}/doxygen)

if(EXISTS ${DOXYGEN_IN})
    configure_file(${DOXYGEN_IN} ${DOXYGEN_CONFIG} @ONLY)
else()
    message(FATAL_ERROR "Doxygen input file not found: ${DOXYGEN_IN}")
endif()

add_custom_target(doc
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_CONFIG}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "📚 Génération de la documentation avec Doxygen..."
    VERBATIM
)
