/**
 * @file le_exterr.h
 *
 * The legato extended error codes are defined to reveal all legato components
 * error and be returned directly to end applications, such as ATIP.
 *
 * The error codes are hexadecimal format which are positive values, the upper column
 * is defined as group/component code. (ex: 0x0100 as SEC, 0x0200 as TLS, 0x0300 as SOCKET, etc)
 *
 * There can be legato extended error codes whose values are from 3rd-party library, such as
 * mbedTLS with negative values (ex: -2700, -7280) or
 * openSSL with positive values (0x01 ~ 0x80) and defined in those library header files.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LE_EXTERR_H_INCLUDE_GUARD
#define LE_EXTERR_H_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------
/**
 * Legato Extended Error Code
 */
//--------------------------------------------------------------------------------------------
typedef enum
{
    EXT_NO_ERR = 0x0000,                            //!< No error
    // SECSTORE
    EXT_SEC_GET_ROOT_CA_PATH_ERR = 0x0100,          //!< Failed to get secsotre root CA
                                                    //!< certificate path
    EXT_SEC_READ_ROOT_CA_ERR,                       //!< Failed to read secsotre root CA
                                                    //!< certificate
    EXT_SEC_GET_CLI_CERT_PRIVKEY_PATH_ERR,          //!< Failed to get secsotre client
                                                    //!< certificate and private key path
    EXT_SEC_READ_CLI_CERT_ERR,                      //!< Failed to read secsotre client
                                                    //!< certificate
    EXT_SEC_READ_CLI_PRIVKEY_ERR,                   //!< Failed to read secsotre client
                                                    //!< private key
    // TLS
    EXT_TLS_SET_AUTH_ERR = 0x0200,                  //!< Failed to set tls authentication type
    EXT_TLS_SET_CIPHER_SUITE_ERR,                   //!< Failed to set tls cipher suite
    EXT_TLS_SET_ALPN_ERR,                           //!< Failed to set tls ALPN protocol list
    // SOCKET
    EXT_SOC_GET_DNS_QUERY_ERR = 0x0300,             //!< Failed to query IP address of server
    EXT_SOC_SET_CNX_TIMEOUT_ERR,                    //!< Failed to set connection timeout
    EXT_SOC_SET_READ_TIMEOUT_ERR,                   //!< Failed to set read timeout
    EXT_SOC_SET_WRITE_TIMEOUT_ERR,                  //!< Failed to set write timeout
    EXT_SOC_SET_EVENT_HANDLER_ERR,                  //!< Failed to add socket event handler
    EXT_SOC_SET_ASYNC_ERR,                          ///< Failed to enable async mode
    EXT_SOC_SET_CLI_SOCKET_ERR,                     //!< Failed to create client socket
    EXT_SOC_CNX_REMOTE_SERVER_ERR,                  //!< Failed to connect remote server
    EXT_SOC_SET_CNX_TIMER_ERR,                      //!< Failed to create connection timer
    // HTTP
    EXT_HTTP_SET_CREDENTIALS_ERR = 0x0400,          ///< Failed to set http username/password
    // MQTT
    EXT_MQTT_SEND_CNX_PACKAGE_ERR = 0x0500,         ///< Failed to send mqtt connection package
    EXT_MQTT_SET_CLI_SESSION_ERR                    ///< Failed to create mqtt client session
}
le_exterr_result_t;

#endif // LE_EXTERR_H_INCLUDE_GUARD
