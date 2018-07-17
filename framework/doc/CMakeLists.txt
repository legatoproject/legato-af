#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

# Generate API documentation with Doxygen
find_package(Doxygen)

if(DOXYGEN_FOUND)
    # End-user framework documentation

    # Check if an external project needs to be handled
    if(EXISTS $ENV{LE_CONFIG_EXTERNAL_DOCS_ROOT})
        set(CMAKE_EXTERNAL_DOXYGEN_ROOT  $ENV{LE_CONFIG_EXTERNAL_DOCS_ROOT})

        # CMake preprocesses the doxygen ".in" file and replaces variable with value
        set(EXTERNAL_DOXYGEN_DEFINE  "EXTERNAL_DOXYGEN_DEFINE")
        message(STATUS "Msg:External Project detected, adding to documents: " ${CMAKE_EXTERNAL_DOXYGEN_ROOT})

        # Provide hook to do other build steps within the external project
        if(EXISTS "$ENV{LE_CONFIG_EXTERNAL_DOCS_ROOT}/CMakeLists.txt")
            message(STATUS "Msg:External project detected, including : $ENV{LE_CONFIG_EXTERNAL_DOCS_ROOT}/CMakeLists.txt")
            include("$ENV{LE_CONFIG_EXTERNAL_DOCS_ROOT}/CMakeLists.txt")
        endif()
    endif()

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/doxygen.user.cfg.in
                   ${CMAKE_CURRENT_BINARY_DIR}/doxygen.user.cfg
                   @ONLY
    )

    # Internal framework implementation documentation.
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/doxygen.implementation.cfg.in
                   ${CMAKE_CURRENT_BINARY_DIR}/doxygen.implementation.cfg
                   @ONLY
    )

    # About Legato to include the current version
    if(LEGATO_VERSION MATCHES "(.*)-([0-9]+)-g([a-z0-9]+)")
        string(REGEX REPLACE "(.*)-([0-9]+)-g([a-z0-9]+)" "\\1" LEGATO_TAG "${LEGATO_VERSION}")
        string(REGEX REPLACE "(.*)-([0-9]+)-g([a-z0-9]+)" "\\3" LEGATO_REV "${LEGATO_VERSION}")

        set(LEGATO_VERSION_DOC "${LEGATO_TAG} <em>(${LEGATO_REV})</em>")
    else()
        set(LEGATO_VERSION_DOC "${LEGATO_VERSION}")
    endif()

    message(STATUS "Doc Version: ${LEGATO_VERSION_DOC}")

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/aboutLegato.dox.in
                   ${CMAKE_CURRENT_BINARY_DIR}/generated/aboutLegato.dox
                   @ONLY
    )

    add_custom_target(mkdoc_gen
        COMMAND ${LEGATO_TOOL_MKDOC} ${CMAKE_CURRENT_BINARY_DIR}/generated)

    add_custom_target(kconfig_gen
        COMMAND ${LEGATO_TOOL_K2DOX}
            --title "Legato KConfig Options"
            --page kconfigOptions
            --kconfig ${LEGATO_ROOT}/KConfig
            --dox ${CMAKE_CURRENT_BINARY_DIR}/generated/kconfigOptions.dox)

    function(generate_doc DOC_TARGET DOC_TITLE DOXYGEN_OUTPUT DOXYGEN_CFG)

        # HTML
        add_custom_target("${DOC_TARGET}_docs"
            COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/static/legato.css ${CMAKE_CURRENT_BINARY_DIR}
            COMMAND mkdir -p ${DOXYGEN_OUTPUT} && ${DOXYGEN_EXECUTABLE} ${DOXYGEN_CFG}
            COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/static/navtree.css                            ${DOXYGEN_OUTPUT}/html
            COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/static/tabs.css                               ${DOXYGEN_OUTPUT}/html
            COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/static/fonts.css                              ${DOXYGEN_OUTPUT}/html
            COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/static/opensans.ttf                           ${DOXYGEN_OUTPUT}/html
            COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/static/opensans-light.ttf                     ${DOXYGEN_OUTPUT}/html
            COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/static/opensans-bold.ttf                      ${DOXYGEN_OUTPUT}/html
            COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/static/favicon.ico                            ${DOXYGEN_OUTPUT}/html
            COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/static/swi-ico-medium.png                     ${DOXYGEN_OUTPUT}/html
            COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/static/ftv2mnode.png                          ${DOXYGEN_OUTPUT}/html
            COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/static/ftv2mlastnode.png                      ${DOXYGEN_OUTPUT}/html
            COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/static/ftv2pnode.png                          ${DOXYGEN_OUTPUT}/html
            COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/static/ftv2plastnode.png                      ${DOXYGEN_OUTPUT}/html
            COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/static/ftv2doc.png                            ${DOXYGEN_OUTPUT}/html
            COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/static/ftv2folderopen.png                     ${DOXYGEN_OUTPUT}/html
            COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/static/resize.js                              ${DOXYGEN_OUTPUT}/html
            COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/static/search.css                             ${DOXYGEN_OUTPUT}/html/search
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating HTML for ${DOC_TITLE}" VERBATIM
            DEPENDS api_headers mkdoc_gen kconfig_gen
        )

        # PDF
        add_custom_target("${DOC_TARGET}_pdf"
            COMMAND make > ${CMAKE_CURRENT_BINARY_DIR}/doxygen-${DOC_TARGET}.pdf.log && cp refman.pdf ${DOXYGEN_OUTPUT}/legato-${DOC_TARGET}.pdf
            WORKING_DIRECTORY ${DOXYGEN_OUTPUT}/latex
            DEPENDS "${DOC_TARGET}_docs"
            COMMENT "Generating PDF for ${DOC_TITLE} (log @ ${CMAKE_CURRENT_BINARY_DIR}/doxygen-${DOC_TARGET}.pdf.log)"
        )

    endfunction()

    # Doc Generation
    generate_doc(user
        "API documentation"
        ${DOCUMENTATION_OUTPUT_PATH}/user
        ${CMAKE_CURRENT_BINARY_DIR}/doxygen.user.cfg
    )

    generate_doc(implementation
        "Framework implementation documentation"
        ${DOCUMENTATION_OUTPUT_PATH}/implementation
        ${CMAKE_CURRENT_BINARY_DIR}/doxygen.implementation.cfg
    )


else()
    message(WARNING "Documentation: Doxygen not found, no documentation will be produced.")
endif()
