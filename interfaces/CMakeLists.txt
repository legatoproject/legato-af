#
# Rules to generate C header files containing Doxygen markup for documentation purposes only.
#
# Copyright 2014, Sierra Wireless Inc.  Use of this work is subject to license.
#


function(generate_header API_FILE)

    get_filename_component(API_NAME ${API_FILE} NAME_WE)

    set(API_PATH    "${CMAKE_CURRENT_SOURCE_DIR}/${API_FILE}")
    set(HEADER_PATH "${CMAKE_CURRENT_BINARY_DIR}/${API_NAME}_interface.h")

    add_custom_command( OUTPUT ${HEADER_PATH}
                        COMMAND ${LEGATO_TOOL_IFGEN} --gen-interface ${API_PATH}
                        --import-dir ${CMAKE_CURRENT_SOURCE_DIR}/audio
                        ${ARGN}
                        COMMENT "ifgen '${API_FILE}': ${HEADER_PATH}"
                        DEPENDS ${API_FILE} legato
                        )

    add_custom_target(  ${API_NAME}_if
                        DEPENDS ${HEADER_PATH}
                        )

    add_dependencies( api_headers ${API_NAME}_if)

endfunction()

add_custom_target(api_headers)

generate_header(cellNetService/le_cellnet.api)
generate_header(le_cfg.api)
generate_header(supervisor/le_sup_state.api)
generate_header(supervisor/le_sup_ctrl.api)
generate_header(supervisor/le_sup_wdog.api)
generate_header(le_wdog.api)
generate_header(modemServices/le_ecall.api)
generate_header(modemServices/le_mdmDefs.api)
generate_header(modemServices/le_sms.api)
generate_header(modemServices/le_mdc.api)
generate_header(modemServices/le_mcc_profile.api)
generate_header(modemServices/le_mrc.api)
generate_header(modemServices/le_fwupdate.api)
generate_header(modemServices/le_sim.api)
generate_header(modemServices/le_info.api)
generate_header(modemServices/le_mcc_call.api)
generate_header(audio/le_audio.api)
generate_header(le_cfgAdmin.api)
generate_header(positioning/le_pos.api)
generate_header(positioning/le_posCtrl.api)
generate_header(dataConnectionService/le_data.api)
generate_header(voiceCallService/le_voicecall.api)
