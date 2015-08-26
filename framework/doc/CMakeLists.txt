#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

# Generate API documentation with Doxygen
find_package(Doxygen)

if(DOXYGEN_FOUND)

    set(GENERATE_TREEVIEW "YES")
    set(USER_PATH "user")

    # End-user framework documentation.
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/doxygen.user.cfg.in
                   ${CMAKE_CURRENT_BINARY_DIR}/doxygen.user.cfg
                   @ONLY
    )

    set(GENERATE_TREEVIEW "NO")
    set(USER_PATH "user_embed")

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/doxygen.user.cfg.in
                   ${CMAKE_CURRENT_BINARY_DIR}/doxygen.user.embed.cfg
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
            DEPENDS api_headers mkdoc_gen
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
        "${DOCUMENTATION_OUTPUT_PATH}/user"
        "${CMAKE_CURRENT_BINARY_DIR}/doxygen.user.cfg"
    )

    generate_doc(user_embed
        "Embedded API documentation"
        "${DOCUMENTATION_OUTPUT_PATH}/user_embed"
        "${CMAKE_CURRENT_BINARY_DIR}/doxygen.user.embed.cfg"
    )

    generate_doc(implementation
        "framework implementation documentation"
        "${DOCUMENTATION_OUTPUT_PATH}/implementation"
        "${CMAKE_CURRENT_BINARY_DIR}/doxygen.implementation.cfg"
    )


else()
    message(WARNING "Documentation: Doxygen not found, no documentation will be produced.")
endif()
