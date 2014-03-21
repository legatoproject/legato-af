/*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Sierra Wireless - initial API and implementation
 *******************************************************************************/
//***************************************************************************
//
//  Filename:
//    MSCIIDs.h
//
//  Description:
//
//
//  Copyright (C) 2011 Sierra Wireless, Inc
//  All Rights Reserved.
//
//***************************************************************************
//
//  Revisions:
//
//  10/07/2003 arb Created
//
//***************************************************************************

#ifndef _airlink_MSCIIDs_h
#define _airlink_MSCIIDs_h 1


/*................................. INCLUDES ..............................*/

//#include "typedefs.h"

/*............................ MACROS AND CONSTANTS .......................*/

//typedef  uint16  MSCIID;

#define MSCIID_MODBUS_LIST_EMPTY_STR  "0=:0"

#define  MSCIID_INVALID                 0xffff
#define  MSCIID_ROOT_GROUP                   0

//#define  MSCIID_INF_GROUP                    1
//#define    MSCIID_INF_CMN_GROUP            255
//#define      MSCIID_INF_ACS_GROUP            2
#define      MSCIID_INF_VERSION              3
#define      MSCIID_INF_ALEOS_SW_VER         4
#define      MSCIID_INF_ALEOS_HW_VER         5
#define      MSCIID_INF_BOOT_VER             6
#define      MSCIID_INF_PRODUCT_STR          7
#define      MSCIID_INF_MODEM_SW_VER         8
#define      MSCIID_INF_MODEM_HW_VER         9
#define      MSCIID_INF_MODEM_ID            10
#define      MSCIID_INF_MODEM_NAME          11
#define      MSCIID_INF_AT_GROUP            16
#define      MSCIID_INF_PHONE_NUM           17
#define      MSCIID_INF_CFG_GROUP           24
#define      MSCIID_INF_DEVICE_ID           25
// DEPRECATED: MSCIID_INF_NETWORK_IP        26
#define      MSCIID_INF_RADIO_TECH          27
#define    MSCIID_INF_ENET_GROUP            64
#define      MSCIID_INF_MAC_ADDR            66


//#define  MSCIID_STS_GROUP                  256
//#define    MSCIID_STS_CMN_GROUP            257
//#define      MSCIID_STS_CMN_AT_GROUP       258
#define      MSCIID_STS_NETWORK_STATE      259
#define      MSCIID_STS_NETWORK_CHANNEL    260
#define      MSCIID_STS_NETWORK_RSSI       261
#define      MSCIID_STS_POWER_MODE         262
#define      MSCIID_STS_NETWORK_ER         263
#define      MSCIID_STS_NETWORK_SERVICE    264
#define      MSCIID_STS_HOST_COMM_STATE    265
#define      MSCIID_STS_POWERIN            266
#define      MSCIID_STS_BOARDTEMP          267
#define      MSCIID_STS_NET_STATE_RAW      268
#define      MSCIID_STS_NET_COVERAGE_RAW   269
#define      MSCIID_STS_SEC_SINCE_RESET    270
//#define      MSCIID_STS_CMN_GROUP_LAST     270
#define      MSCIID_STS_LINK_STATE         271

//#define      MSCIID_STS_CMN_ACS_GROUP      272
#define      MSCIID_STS_HOST_SENT          273
#define      MSCIID_STS_HOST_RECV          274
#define      MSCIID_STS_HOST_MODE          275
#define      MSCIID_STS_CPU_USER           276
#define      MSCIID_STS_CPU_INTERRUPT      277
#define      MSCIID_STS_CPU_TOTAL          278
#define      MSCIID_STS_HOST_IP_SENT       279
#define      MSCIID_STS_HOST_IP_RECV       280
#define      MSCIID_STS_MODEM_IP_SENT      281
#define      MSCIID_STS_MODEM_IP_RECV      282
#define      MSCIID_STS_MODEM_SENT         283
#define      MSCIID_STS_MODEM_RECV         284
//#define      MSCIID_STS_CMN_CFG_GROUP      300
#define      MSCIID_STS_NETWORK_IP         301
#define      MSCIID_STS_SYSTEM_BOOTS       302

//#define    MSCIID_STS_SEC_GROUP            383
//#define      MSCIID_STS_SEC_ACS_GROUP      384
// DEPRECATED:  MSCIID_STS_BAD_PW_CNT      385
#define      MSCIID_STS_IP_REJECT_CNT      386
// DEPRECATED:  MSCIID_STS_IP_REJECT_LIST  387
//#define      MSCIID_STS_IP_REJECT_LIST     388
#define         MSCIID_STS_IP_REJECT_01       389
// ...
#define      MSCIID_STS_IP_REJECT_10       398
//#define    MSCIID_STS_CDPD_GROUP           512
#define      MSCIID_STS_CDPD_REG_STATUS    513
#define      MSCIID_STS_CDPD_FWD_BLER      514
#define      MSCIID_STS_CDPD_CELL_NUM      515
#define      MSCIID_STS_CDPD_COLOR_CODE    516
#define      MSCIID_STS_CDPD_XMIT_PWR      517
#define      MSCIID_STS_CDPD_PWR_PROD      518
#define      MSCIID_STS_CDPD_CDPD_VER      519
//#define    MSCIID_STS_CDMA_GROUP           640
#define      MSCIID_STS_CDMA_HW_TEMP       641
#define      MSCIID_STS_CDMA_PRL_VERSION   642
#define      MSCIID_STS_CDMA_ECIO          643
#define      MSCIID_STS_CDMA_OPERATOR      644
// DEPRECATED: MSCIID_STS_CDMA_SERVICE     645
#define      MSCIID_STS_CDMA_PRL_UPD_STS   646
#define      MSCIID_STS_CDMA_NEXT_AUTO_PRL 647
#define      MSCIID_STS_CDMA_SID           648
#define      MSCIID_STS_CDMA_NID           649
#define      MSCIID_STS_CDMA_PN_OFFSET     650
#define      MSCIID_STS_CDMA_BASE_CLASS    651
#define    MSCIID_STS_CDMA_GROUP_LAST      651

//#define    MSCIID_STS_GPRS_GROUP           768
#define      MSCIID_STS_GPRS_AT_GROUP      769
#define      MSCIID_STS_GPRS_OPERATOR      770
#define      MSCIID_STS_GPRS_SIMID         771
#define      MSCIID_STS_GPRS_ECIO          772
#define        MSCIID_STS_GPRS_CELL_ID       773
#define        MSCIID_STS_GPRS_LAC           774
#define        MSCIID_STS_GPRS_BSIC          775
//#define      MSCIID_STS_GPRS_STATSAT_LAST  772

//#define      MSCIID_STS_GPRS_CFG_GROUP     784
#define      MSCIID_STS_GPRS_IMSI          785
//#define      MSCIID_STS_GPRS_ACS_GROUP     800
#define      MSCIID_STS_GPRS_CELL_INFO     801
//#define      MSCIID_STS_GPRS_LAST          801

//#define    MSCIID_STS_IO_GROUP             850
#define      MSCIID_STS_IO_DIGITAL_IN1     851
#define      MSCIID_STS_IO_DIGITAL_IN2     852
#define      MSCIID_STS_IO_DIGITAL_IN3     853
#define      MSCIID_STS_IO_DIGITAL_IN4     854
#define      MSCIID_STS_IO_ANALOG_IN1      855
#define      MSCIID_STS_IO_ANALOG_IN2      856
#define      MSCIID_STS_IO_ANALOG_IN3      857
#define      MSCIID_STS_IO_ANALOG_IN4      858
#define      MSCIID_STS_IO_DIGITAL_OUT1    859
#define      MSCIID_STS_IO_DIGITAL_OUT2    860
#define      MSCIID_STS_IO_DIGITAL_CFG1    861
#define      MSCIID_STS_IO_DIGITAL_CFG2    862
#define      MSCIID_STS_IO_DIGITAL_OUT3    863
#define      MSCIID_STS_IO_DIGITAL_OUT4    864
#define      MSCIID_STS_IO_DIGITAL_OUT5    865
#define      MSCIID_STS_IO_DIGITAL_OUT6    866
#define      MSCIID_STS_IO_DIGITAL_IN5     867
#define      MSCIID_STS_IO_DIGITAL_IN6     868
#define      MSCIID_STS_IO_DIGITAL_CFG3    869
#define      MSCIID_STS_IO_DIGITAL_CFG4    870
#define      MSCIID_STS_IO_DIGITAL_CFG5    871
#define      MSCIID_STS_IO_DIGITAL_CFG6    872
#define      MSCIID_STS_IO_LAST            872      // move down when rest are implemented


//#define    MSCIID_STS_PP_GROUP             896
#define      MSCIID_STS_PP_GPS_QUAL        900
#define      MSCIID_STS_PP_GPS_SAT_CNT     901
#define      MSCIID_STS_PP_GPS_LAT         902
#define      MSCIID_STS_PP_GPS_LONG        903
#define      MSCIID_STS_PP_GPS_HEADING     904
#define      MSCIID_STS_PP_GPS_SPEED       905
#define      MSCIID_STS_PP_ENGINE_HRS      906
//#define    MSCIID_STS_PP_GROUP_LAST        906

//#define    MSCIID_STS_IDEN_GROUP           928
//#define      MSCIID_STS_IDEN_AT_GROUP      929
#define      MSCIID_STS_IDEN_OPERATOR      930
#define      MSCIID_STS_IDEN_SIMID         931
#define      MSCIID_STS_IDEN_NEI           932


//#define  MSCIID_CFG_GROUP                 1024
//#define    MSCIID_CFG_CMN_GROUP           1025
//#define      MSCIID_CFG_CMN_CFG_GROUP     1026
#define      MSCIID_CFG_CMN_AT_VERBOSE    1027
#define      MSCIID_CFG_CMN_QUIET_MODE    1028
// DEPRECATED:#define MSCIID_CFG_CMN_CMD_ECHO 1029
#define      MSCIID_CFG_CMN_DISABLE_ATESC 1030
#define      MSCIID_CFG_CMN_NO_ATZ_RESET  1031
#define      MSCIID_CFG_CMN_IGNORE_DTR    1032
#define      MSCIID_CFG_CMN_CALLP_RSLT_MD 1033
#define      MSCIID_CFG_CMN_TELNET_PORT   1034
#define      MSCIID_CFG_CMN_TNET_TIMEOUT  1035
#define      MSCIID_CFG_CMN_DIAL_UDP_ALW  1036
#define      MSCIID_CFG_CMN_TCP_IDLE_SECS 1037
#define      MSCIID_CFG_CMN_TCP_SER_ENQ   1038
#define      MSCIID_CFG_CMN_UDP_ANS_RESP  1039
#define      MSCIID_CFG_CMN_UDP_CNT_LAST  1040
#define      MSCIID_CFG_CMN_ALLOW_ANY_IP  1041
#define      MSCIID_CFG_CMN_ALLOW_ALL_UDP 1042
#define      MSCIID_CFG_CMN_DEFAULT_MODE  1043
#define      MSCIID_CFG_CMN_DEST_ADDR     1044
#define      MSCIID_CFG_CMN_DEST_PORT     1045
#define      MSCIID_CFG_CMN_DATA_FWD_TO   1046
#define      MSCIID_CFG_CMN_DATA_FWD_CHAR 1047
#define      MSCIID_CFG_CMN_TCP_AUTO_ANS  1048
#define      MSCIID_CFG_CMN_TCP_CNT_TO    1049
#define      MSCIID_CFG_CMN_TCP_IDLE_TO   1050
#define      MSCIID_CFG_CMN_TCP_CNT_DELAY 1051
#define      MSCIID_CFG_CMN_UDP_SER_DELAY 1052
#define      MSCIID_CFG_CMN_TELNET_ECHO   1053
#define      MSCIID_CFG_CMN_UDP_AUTO_ANS  1054
#define      MSCIID_CFG_CMN_UDP_IDLE_TO   1055
#define      MSCIID_CFG_CMN_DTR_POWER     1056
#define      MSCIID_CFG_CMN_VLTG_POWER    1057
#define      MSCIID_CFG_CMN_POWER_OFF     1058
#define      MSCIID_CFG_CMN_OVER_AIR_PROG 1059
#define      MSCIID_CFG_CMN_CLIENT_PPP_AUTH 1060  // DEPRECATED:  MSCIID_CFG_CMN_DBUG_CLIENT   1060
// DEPRECATED:  MSCIID_CFG_CMN_EXP_ACE_PW    1061
#define      MSCIID_CFG_CMN_EXP_ACE_PW    1061    //should be renamed not related EXP anymore but is used in rap and updater
#define      MSCIID_CFG_CMN_FRIEND_MODE   1062
#define      MSCIID_CFG_CMN_FRIEND_IP0    1063
#define      MSCIID_CFG_CMN_FRIEND_IP1    1064
#define      MSCIID_CFG_CMN_FRIEND_IP2    1065
#define      MSCIID_CFG_CMN_FRIEND_IP3    1066
#define      MSCIID_CFG_CMN_FRIEND_IP4    1067
#define      MSCIID_CFG_CMN_FRIEND_IP5    1068
#define      MSCIID_CFG_CMN_FRIEND_IP6    1069
#define      MSCIID_CFG_CMN_FRIEND_IP7    1070
#define      MSCIID_CFG_CMN_FRIEND_IP8    1071
#define      MSCIID_CFG_CMN_FRIEND_IP9    1072
// DEPRECATED:  MSCIID_CFG_CMN_AIRLK_FRIEND0 1073
// DEPRECATED:  MSCIID_CFG_CMN_AIRLK_FRIEND1 1074
// DEPRECATED:  MSCIID_CFG_CMN_AIRLK_FRIEND2 1075
// DEPRECATED:  MSCIID_CFG_CMN_AIRLK_FRIEND3 1076
// DEPRECATED:  MSCIID_CFG_CMN_AIRLK_FRIEND4 1077
// DEPRECATED:  MSCIID_CFG_CMN_PRIV_HOST_IP  1078
#define      MSCIID_CFG_CMN_HOST_CTS_ENA  1079
#define      MSCIID_CFG_CMN_DNS_UPDATES   1080
#define      MSCIID_CFG_CMN_DEV_PORT      1081
#define      MSCIID_CFG_CMN_MDM_DNS_IP1   1082
#define      MSCIID_CFG_CMN_MDM_DNS_IP2   1083
#define      MSCIID_CFG_CMN_HOST_LOCAL_IP 1084
// DEPRECATED:    MSCIID_CFG_CMN_HOST_PEER_IP  1085
#define      MSCIID_CFG_CMN_HOST_PEER_IP  1085
#define      MSCIID_CFG_CMN_NETWORK_PW    1087
#define      MSCIID_CFG_CMN_NETWORK_UID   1088
#define      MSCIID_CFG_CMN_HOST_DSR_BEH  1089
#define      MSCIID_CFG_CMN_HOST_DCD_BEH  1090
#define      MSCIID_CFG_CMN_USER_DNS      1091
// DEPRECATED:  MSCIID_CFG_CMN_PPP_LOGGING   1092
// DEPRECATED:  MSCIID_CFG_CMN_IP_LOGGING    1093
#define      MSCIID_CFG_CMN_HOST_PAP      1094
#define      MSCIID_CFG_CMN_IPMSRVR1_KEY  1095
#define      MSCIID_CFG_CMN_IPMSRVR2_KEY  1096
#define      MSCIID_CFG_CMN_IPMSRVR1_UPD  1097
#define      MSCIID_CFG_CMN_IPMSRVR2_UPD  1098
#define      MSCIID_CFG_CMN_MDM_RESET     1099
#define      MSCIID_CFG_CMN_NUM_TO_IP     1100
#define      MSCIID_CFG_CMN_PT_INIT       1101
#define      MSCIID_CFG_CMN_PT_INIT_REF   1102
#define      MSCIID_CFG_CMN_CSX_PT_ECHO   1103
#define      MSCIID_CFG_CMN_IPPING_PERIOD 1104
#define      MSCIID_CFG_CMN_IPPING_ADDR   1105
#define      MSCIID_CFG_CMN_SMS2EMAIL     1106
#define      MSCIID_CFG_CMN_SNTP_ENABLE   1107
#define      MSCIID_CFG_CMN_SNTP_SERVER   1108
#define      MSCIID_CFG_CMN_NET_CONN_WDOG 1109
#define      MSCIID_CFG_CMN_SMTP_SERVER   1110
#define      MSCIID_CFG_CMN_SMTP_FROM     1111
#define      MSCIID_CFG_CMN_SMTP_USER     1112
#define      MSCIID_CFG_CMN_SMTP_PASSWD   1113
#define      MSCIID_CFG_CMN_SMTP_SUBJECT  1114
#define      MSCIID_CFG_CMN_SNMP_PORT     1115
#define      MSCIID_CFG_CMN_SNMP_SECLVL   1116
#define      MSCIID_CFG_CMN_IPPING_FORCE  1117
#define      MSCIID_CFG_CMN_HOST_NETMASK  1118
#define      MSCIID_CFG_CMN_ALLOW_ZERO_IP 1119
#define      MSCIID_CFG_CMN_HOST_AUTH     1120
#define      MSCIID_CFG_CMN_HOST_UID      1121
#define      MSCIID_CFG_CMN_HOST_PW       1122
#define      MSCIID_CFG_CMN_STATIC_IP     1123
//#define      MSCIID_CFG_CMN_USB_DFLT_MODE 1124
#define      MSCIID_CFG_CMN_SERIAL_ECHO   1125
#define      MSCIID_CFG_CMN_USB_ECHO      1126
#define      MSCIID_CFG_CMN_ECHO_TELNET   1127
#define      MSCIID_CFG_CMN_USB_DFLT_MODE 1128
#define      MSCIID_CFG_CMN_SNMP_COMMUNITY_STRING 1129
#define      MSCIID_CFG_CMN_USB_DEVICE    1130
#define      MSCIID_CFG_CMN_HOST_IP2      1131
#define      MSCIID_CFG_CMN_HOST_NETMASK2 1132
#define      MSCIID_CFG_CMN_HOST_IP3      1133
#define      MSCIID_CFG_CMN_HOST_NETMASK3 1134
#define      MSCIID_CFG_CMN_DHCP_NETMASK  1135
#define      MSCIID_CFG_CMN_USER_DNS2     1136
#define      MSCIID_CFG_CMN_ETH_START_IP  1137
#define      MSCIID_CFG_CMN_ETH_END_IP    1138
#define      MSCIID_CFG_CMN_HOST_IP_MODE  1139
#define      MSCIID_CFG_CMN_LOCAL_USB_IP  1140
#define      MSCIID_CFG_CMN_HOST_USB_IP   1141
#define      MSCIID_CFG_CMN_LOCAL_RS232_IP 1142
#define      MSCIID_CFG_CMN_HOST_RS232_IP 1143

#define      MSCIID_NET_WIFI_HOST_IP      1144
#define      MSCIID_NET_WIFI_START_IP     1145
#define      MSCIID_NET_WIFI_END_IP       1146
#define      MSCIID_NET_WIFI_NETMASK      1147
#define      MSCIID_NET_WIFI_NETMASK      1147
#define      MSCIID_CFG_NON_FRIEND_PF     1148
#define      MSCIID_CFG_CMN_ACEWEB_ACCESS 1149
#define      MSCIID_CFG_CMN_ACEWEB_PORT   1150
#define      MSCIID_CFG_CMN_LP_RELAY_MODE 1151
#define      MSCIID_CFG_CMN_LAST          1151

//#define      MSCIID_CFG_CMN_AT_GROUP      1152
#define      MSCIID_CFG_CMN_DATE          1153
#define      MSCIID_CFG_CMN_MDM_NAME      1154
#define      MSCIID_CFG_CMN_DOMAIN        1155
#define      MSCIID_CFG_CMN_IPMSRVR1      1156
#define      MSCIID_CFG_CMN_IPMSRVR2      1157
#define      MSCIID_CFG_CMN_HOST_COMM     1159
#define      MSCIID_CFG_CMN_HOST_FLOWCTL  1160
#define      MSCIID_CFG_CMN_MSCI_UPD_ADDR 1161
#define      MSCIID_CFG_CMN_MSCI_UPD_PER  1162
#define      MSCIID_CFG_CMN_MSCI_PASSWD   1163
#define      MSCIID_CFG_CMN_PHONE_NUM     1164
#define      MSCIID_CFG_CMN_HOST_DTR_BEH  1165
#define      MSCIID_CFG_CMN_SNMP_TRAPDEST 1166
#define      MSCIID_CFG_CMN_MODEM_HISPEED 1167
// from Aleos
#define      MSCIID_CFG_CMN_TIME_OFFSET   1168
#define      MSCIID_CFG_CMN_TIME_OFFSET_DIR   1169
#define      MSCIID_CFG_CMN_TODAYS_DATE_STR   1170    // not including time
#define      MSCIID_CFG_CMN_TODAYS_DATE_INT   1171    // As an integer, for ER.
#define      MSCIID_CFG_CMN_SSLWEB_PORT       1172      //   Used in 4.2 now
#define      MSCIID_CFG_CMN_AM_OTA_ACCESS     1173
#define      MSCIID_CFG_CMN_AM_HOST_ACCESS    1174
#define      MSCIID_CFG_CMN_AM_WIFI_ACCESS    1175
#define      MSCIID_CFG_CMN_RESET_TIMER_HARD  1176
#define      MSCIID_CFG_CMN_HOST_KA_IP        1177
#define      MSCIID_CFG_CMN_HOST_KA_TIME      1178
#define      MSCIID_CFG_CMN_HOST_NO_NAT       1179
#define      MSCIID_CFG_CMN_TOD_RESET_DAYS    1180
#define      MSCIID_CFG_CMN_TOD_RESET_OSET    1181
#define      MSCIID_CFG_CMN_TOD_RESET_HOUR    1182
#define      MSCIID_CFG_CMN_RAD_ON_OFF_ENABLE 1183
#define      MSCIID_CFG_CMN_RAD_ON_OFF_DIN    1184
#define      MSCIID_CFG_CMN_RAD_ON_OFF_DOUT   1185
#define      MSCIID_CFG_CMN_RAD_ON_OFF_FLIPS  1186
#define      MSCIID_CFG_CMN_GEOFENCE_RADIUS   1187
#define      MSCIID_CFG_CMN_AT_GROUP_LAST     1187

#define      MSCIID_CFG_CMN_ACS_GROUP     1200
#define      MSCIID_CFG_CMN_DIALCODE      1201
#define      MSCIID_CFG_CMN_PW_RESET_CHAL 1202

//#define    MSCIID_CFG_PP_GROUP            1280
//#define      MSCIID_CFG_PP_CFG_GROUP      1282
#ifndef AT91RM9200
#define    MSCIID_CFG_PP_FIRST            1283
#else
#define    MSCIID_CFG_PP_FIRST            1285
#endif
#define      MSCIID_CFG_PP_RTS_INPUT      1283
#define      MSCIID_CFG_PP_DTR_INPUT      1284
#define      MSCIID_CFG_PP_ODOMETER_ENA   1285
#define      MSCIID_CFG_PP_SNF_ENABLE     1287
#define      MSCIID_CFG_PP_SERVER_IP      1288
#define      MSCIID_CFG_PP_SERVER_PORT    1289
#define      MSCIID_CFG_PP_RPT_INT_TIME   1290
#define      MSCIID_CFG_PP_RPT_INT_DIST   1291
#define      MSCIID_CFG_PP_LEGACY_FORMAT  1293
#define      MSCIID_CFG_PP_SNF_TYPE       1294
#define      MSCIID_CFG_PP_SNF_MIN_RPTS   1295
#define      MSCIID_CFG_PP_SNF_REL_MODE   1296
#define      MSCIID_CFG_PP_VS_TIME_INT    1297
#define      MSCIID_CFG_PP_GPS_INIT_TIME  1298
#define      MSCIID_CFG_PP_LOCAL_RPT_SECS 1299
#define      MSCIID_CFG_PP_TAIP_ID        1300
#define      MSCIID_CFG_PP_ADD_GPS_UDPR   1301
#define      MSCIID_CFG_PP_DISABLE_IP_UPD 1302
#define      MSCIID_CFG_PP_INPUT_EVENTS   1303
#define      MSCIID_CFG_PP_FLUSH_ON_EVT   1304
#define      MSCIID_CFG_PP_ENABLE_COM1000 1305
#define      MSCIID_CFG_PP_LATS_EXTRA     1306
#define      MSCIID_CFG_PP_SERVER_NAME    1307
#define      MSCIID_CFG_PP_MAX_RETRIES    1308
#define      MSCIID_CFG_PP_SIMPL_RETRY_TO 1309
#define      MSCIID_CFG_PP_TCP_POLL_PORT  1310
#define      MSCIID_CFG_PP_ENABLE_INPUTS  1311
#define      MSCIID_CFG_PP_ENGINE_VOLTS   1312    // 3.2
#define      MSCIID_CFG_PP_ENGINE_SENSE   1313    // 3.2
#define      MSCIID_CFG_PP_SERVER_NAME2   1314    // 3.2
#define      MSCIID_CFG_PP_RETRY_FAILOVER 1315    // 3.2
#define      MSCIID_CFG_PP_REDUNE_SERVER1 1316    // 3.2
#define      MSCIID_CFG_PP_REDUNE_SERVER2 1317    // 3.2
#define      MSCIID_CFG_PP_PERSIST_GPS_F  1318    // 3.1
#define      MSCIID_CFG_PP_COVERAGE       1319
#define      MSCIID_CFG_PP_PERSIST_DELAY  1320
#define      MSCIID_CFG_PP_USE_LOCAL_DID  1321
#define      MSCIID_CFG_PP_REDUNE_PORT1   1322
#define      MSCIID_CFG_PP_REDUNE_PORT2   1323
#define      MSCIID_CFG_PP_MAX_SPEED      1324
#define      MSCIID_CFG_PP_STATIONARY_EVENT 1325
#define      MSCIID_CFG_PP_GARMIN_ENABLE  1326
#define      MSCIID_STS_PP_GARMIN_STATUS  1327

//#define    MSCIID_CFG_PP_GROUP_LAST       1327

//#define    MSCIID_CFG_PP_AT_GROUP       1408
#define      MSCIID_CFG_PP_USE_DEVICEID   1409
#define      MSCIID_CFG_PP_GPS_RPT_TYPE   1410
#define      MSCIID_CFG_PP_LOCAL_RPT_TYPE 1411
#define      MSCIID_CFG_PP_ODOMETER       1412
#define      MSCIID_CFG_PP_RPT_MIN_TIME   1413
#define      MSCIID_CFG_PP_PERSIST_GPS    1414
#define      MSCIID_CFG_PP_GPS_DATUM      1415
#define      MSCIID_CFG_PP_GPS_SENTENCES  1416
#define      MSCIID_CFG_PP_PERSIST_GPS_R  1417
#define      MSCIID_CFG_PP_GPS_FIXTYPE  1418
//#define    MSCIID_CFG_PP_AT_GROUP_LAST    1418

//#define    MSCIID_CFG_CDMA_GROUP         1536
#define      MSCIID_CFG_CDMA_DORMANCY_TMR 1537
#define      MSCIID_CFG_CDMA_QCMIP        1538
#define      MSCIID_CFG_CDMA_MSL          1539
#define      MSCIID_CFG_CDMA_PRL_DIAL     1540
#define      MSCIID_CFG_CDMA_AUTO_PRL_FRQ 1541
#define      MSCIID_CFG_CDMA_CK_DL        1542  // Check Data Link [now Profile1]
#define      MSCIID_CFG_CDMA_QCMIPNAI     1543
#define      MSCIID_CFG_CDMA_QCMIPPHA     1544
#define      MSCIID_CFG_CDMA_QCMIPSHA     1545
#define      MSCIID_CFG_CDMA_QCMIPMHSS    1546
#define      MSCIID_CFG_CDMA_QCMIPMASS    1547
//#define    MSCIID_CFG_CDMA_GROUP_LAST    1547

#define  MSCIID_CFG_RE_GROUP              1600
//#define      MSCIID_CFG_RE_FIRST          1642
#define    MSCIID_CFG_RE2_CFG_GROUP       1640
//#define      MSCIID_CFG_RE2_FIRST          1642
#define      MSCIID_CFG_RE2_GPS_RPT_TYPE   1642
#define      MSCIID_CFG_RE2_SERVER_NAME    1643
#define      MSCIID_CFG_RE2_SERVER_PORT    1644
#define      MSCIID_CFG_RE2_SNF_ENABLE     1645
#define      MSCIID_CFG_RE2_SNF_TYPE       1646
#define      MSCIID_CFG_RE2_SNF_MIN_RPTS   1647
#define      MSCIID_CFG_RE2_SNF_REL_MODE   1648
#define      MSCIID_CFG_RE2_MAX_RETRIES    1649
#define      MSCIID_CFG_RE2_USE_DEVICEID   1650
#define      MSCIID_CFG_RE2_RPT_INT_TIME   1651
#define      MSCIID_CFG_RE2_RPT_INT_DIST   1652
#define      MSCIID_CFG_RE2_VS_TIME_INT    1653
//#define  MSCIID_CFG_RE_GROUP_LAST          1653
//#define    MSCIID_CFG_RE2_GROUP_LAST       1653

#define    MSCIID_CFG_RE3_CFG_GROUP       1660
//#define      MSCIID_CFG_RE3_FIRST          1662
#define      MSCIID_CFG_RE3_GPS_RPT_TYPE   1662
#define      MSCIID_CFG_RE3_SERVER_NAME    1663
#define      MSCIID_CFG_RE3_SERVER_PORT    1664
#define      MSCIID_CFG_RE3_SNF_ENABLE     1665
#define      MSCIID_CFG_RE3_SNF_TYPE       1666
#define      MSCIID_CFG_RE3_SNF_MIN_RPTS   1667
#define      MSCIID_CFG_RE3_SNF_REL_MODE   1668
#define      MSCIID_CFG_RE3_MAX_RETRIES    1669
#define      MSCIID_CFG_RE3_USE_DEVICEID   1670
#define      MSCIID_CFG_RE3_RPT_INT_TIME   1671
#define      MSCIID_CFG_RE3_RPT_INT_DIST   1672
#define      MSCIID_CFG_RE3_VS_TIME_INT    1673
//#define    MSCIID_CFG_RE3_GROUP_LAST       1673

#define    MSCIID_CFG_RE4_CFG_GROUP       1680
//#define      MSCIID_CFG_RE4_FIRST          1682
#define      MSCIID_CFG_RE4_GPS_RPT_TYPE   1682
#define      MSCIID_CFG_RE4_SERVER_NAME    1683
#define      MSCIID_CFG_RE4_SERVER_PORT    1684
#define      MSCIID_CFG_RE4_SNF_ENABLE     1685
#define      MSCIID_CFG_RE4_SNF_TYPE       1686
#define      MSCIID_CFG_RE4_SNF_MIN_RPTS   1687
#define      MSCIID_CFG_RE4_SNF_REL_MODE   1688
#define      MSCIID_CFG_RE4_MAX_RETRIES    1689
#define      MSCIID_CFG_RE4_USE_DEVICEID   1690
#define      MSCIID_CFG_RE4_RPT_INT_TIME   1691
#define      MSCIID_CFG_RE4_RPT_INT_DIST   1692
#define      MSCIID_CFG_RE4_VS_TIME_INT    1693
//#define    MSCIID_CFG_RE4_GROUP_LAST       1693
//#define  MSCIID_CFG_RE_GROUP_LAST          1699
//#define  MSCIID_CFG_RE_GROUP_LAST          1693

#define    MSCIID_CFG_SB555_GROUP         1780
// DEPRECATED:  MSCIID_CFG_SB555_QCMIP       1781
// DEPRECATED:  MSCIID_CFG_SB555_STATIC_IP   1782
//#define    MSCIID_CFG_RVN_GROUP           1792
//#define      MSCIID_CFG_RVN_CFG_GROUP     1793
#define      MSCIID_CFG_RVN_RKEYING_ENA   1794
#define      MSCIID_CFG_RVN_MODBUS_VTYPE  1795
#define      MSCIID_CFG_RVN_MODBUS_VOFF   1796
#define      MSCIID_CFG_RVN_MODBUS_VLEN   1797
#define      MSCIID_CFG_RVN_MODBUS_VMSK   1798
#define      MSCIID_CFG_RVN_IP_LIST_DIAL  1799
//#define      MSCIID_CFG_RVN_MODBUS_GROUP  1856
#define      MSCIID_CFG_RVN_MODBUS_01     1857
#define      MSCIID_CFG_RVN_MODBUS_02     1858
#define      MSCIID_CFG_RVN_MODBUS_03     1859
#define      MSCIID_CFG_RVN_MODBUS_04     1860
#define      MSCIID_CFG_RVN_MODBUS_05     1861
#define      MSCIID_CFG_RVN_MODBUS_06     1862
#define      MSCIID_CFG_RVN_MODBUS_07     1863
#define      MSCIID_CFG_RVN_MODBUS_08     1864
#define      MSCIID_CFG_RVN_MODBUS_09     1865
#define      MSCIID_CFG_RVN_MODBUS_10     1866
#define      MSCIID_CFG_RVN_MODBUS_11     1867
#define      MSCIID_CFG_RVN_MODBUS_12     1868
#define      MSCIID_CFG_RVN_MODBUS_13     1869
#define      MSCIID_CFG_RVN_MODBUS_14     1870
#define      MSCIID_CFG_RVN_MODBUS_15     1871
#define      MSCIID_CFG_RVN_MODBUS_16     1872
#define      MSCIID_CFG_RVN_MODBUS_17     1873
#define      MSCIID_CFG_RVN_MODBUS_18     1874
#define      MSCIID_CFG_RVN_MODBUS_19     1875
#define      MSCIID_CFG_RVN_MODBUS_20     1876
#define      MSCIID_CFG_RVNHST_MODBUS_21  1877
#define      MSCIID_CFG_RVNHST_MODBUS_22  1878
#define      MSCIID_CFG_RVNHST_MODBUS_23  1879
#define      MSCIID_CFG_RVNHST_MODBUS_24  1880
#define      MSCIID_CFG_RVNHST_MODBUS_25  1881
#define      MSCIID_CFG_RVNHST_MODBUS_26  1882
#define      MSCIID_CFG_RVNHST_MODBUS_27  1883
#define      MSCIID_CFG_RVNHST_MODBUS_28  1884
#define      MSCIID_CFG_RVNHST_MODBUS_29  1885
#define      MSCIID_CFG_RVNHST_MODBUS_30  1886
#define      MSCIID_CFG_RVNHST_MODBUS_31  1887
#define      MSCIID_CFG_RVNHST_MODBUS_32  1888
#define      MSCIID_CFG_RVNHST_MODBUS_33  1889
#define      MSCIID_CFG_RVNHST_MODBUS_34  1890
#define      MSCIID_CFG_RVNHST_MODBUS_35  1891
#define      MSCIID_CFG_RVNHST_MODBUS_36  1892
#define      MSCIID_CFG_RVNHST_MODBUS_37  1893
#define      MSCIID_CFG_RVNHST_MODBUS_38  1894
#define      MSCIID_CFG_RVNHST_MODBUS_39  1895
#define      MSCIID_CFG_RVNHST_MODBUS_40  1896
#define      MSCIID_CFG_RVNHST_MODBUS_41  1897
#define      MSCIID_CFG_RVNHST_MODBUS_42  1898
#define      MSCIID_CFG_RVNHST_MODBUS_43  1899
#define      MSCIID_CFG_RVNHST_MODBUS_44  1900
#define      MSCIID_CFG_RVNHST_MODBUS_45  1901
#define      MSCIID_CFG_RVNHST_MODBUS_46  1902
#define      MSCIID_CFG_RVNHST_MODBUS_47  1903
#define      MSCIID_CFG_RVNHST_MODBUS_48  1904
#define      MSCIID_CFG_RVNHST_MODBUS_49  1905
#define      MSCIID_CFG_RVNHST_MODBUS_50  1906
#define      MSCIID_CFG_RVNHST_MODBUS_51  1907
#define      MSCIID_CFG_RVNHST_MODBUS_52  1908
#define      MSCIID_CFG_RVNHST_MODBUS_53  1909
#define      MSCIID_CFG_RVNHST_MODBUS_54  1910
#define      MSCIID_CFG_RVNHST_MODBUS_55  1911
#define      MSCIID_CFG_RVNHST_MODBUS_56  1912
#define      MSCIID_CFG_RVNHST_MODBUS_57  1913
#define      MSCIID_CFG_RVNHST_MODBUS_58  1914
#define      MSCIID_CFG_RVNHST_MODBUS_59  1915
#define      MSCIID_CFG_RVNHST_MODBUS_60  1916
#define      MSCIID_CFG_RVNHST_MODBUS_61  1917
#define      MSCIID_CFG_RVNHST_MODBUS_62  1918
#define      MSCIID_CFG_RVNHST_MODBUS_63  1919
#define      MSCIID_CFG_RVNHST_MODBUS_64  1920
#define      MSCIID_CFG_RVNHST_MODBUS_65  1921
#define      MSCIID_CFG_RVNHST_MODBUS_66  1922
#define      MSCIID_CFG_RVNHST_MODBUS_67  1923
#define      MSCIID_CFG_RVNHST_MODBUS_68  1924
#define      MSCIID_CFG_RVNHST_MODBUS_69  1925
#define      MSCIID_CFG_RVNHST_MODBUS_70  1926
#define      MSCIID_CFG_RVNHST_MODBUS_71  1927
#define      MSCIID_CFG_RVNHST_MODBUS_72  1928
#define      MSCIID_CFG_RVNHST_MODBUS_73  1929
#define      MSCIID_CFG_RVNHST_MODBUS_74  1930
#define      MSCIID_CFG_RVNHST_MODBUS_75  1931
#define      MSCIID_CFG_RVNHST_MODBUS_76  1932
#define      MSCIID_CFG_RVNHST_MODBUS_77  1933
#define      MSCIID_CFG_RVNHST_MODBUS_78  1934
#define      MSCIID_CFG_RVNHST_MODBUS_79  1935
#define      MSCIID_CFG_RVNHST_MODBUS_80  1936
#define      MSCIID_CFG_RVNHST_MODBUS_81  1937
#define      MSCIID_CFG_RVNHST_MODBUS_82  1938
#define      MSCIID_CFG_RVNHST_MODBUS_83  1939
#define      MSCIID_CFG_RVNHST_MODBUS_84  1940
#define      MSCIID_CFG_RVNHST_MODBUS_85  1941
#define      MSCIID_CFG_RVNHST_MODBUS_86  1942
#define      MSCIID_CFG_RVNHST_MODBUS_87  1943
#define      MSCIID_CFG_RVNHST_MODBUS_88  1944
#define      MSCIID_CFG_RVNHST_MODBUS_89  1945
#define      MSCIID_CFG_RVNHST_MODBUS_90  1946
#define      MSCIID_CFG_RVNHST_MODBUS_91  1947
#define      MSCIID_CFG_RVNHST_MODBUS_92  1948
#define      MSCIID_CFG_RVNHST_MODBUS_93  1949
#define      MSCIID_CFG_RVNHST_MODBUS_94  1950
#define      MSCIID_CFG_RVNHST_MODBUS_95  1951
#define      MSCIID_CFG_RVNHST_MODBUS_96  1952
#define      MSCIID_CFG_RVNHST_MODBUS_97  1953
#define      MSCIID_CFG_RVNHST_MODBUS_98  1954
#define      MSCIID_CFG_RVNHST_MODBUS_99  1955
#define      MSCIID_CFG_RVNHST_MODBUS_100 1956
//#define    MSCIID_CFG_GPRS_GROUP          2048
#define      MSCIID_CFG_GPRS_AT_GROUP     2049
#define      MSCIID_CFG_GPRS_CONTEXT      2050
#define      MSCIID_CFG_GPRS_COPS         2051
#define      MSCIID_CFG_GPRS_CGQREQ       2052
#define      MSCIID_CFG_GPRS_CGQMIN       2053
#define      MSCIID_CFG_GPRS_SIM_PIN      2054
#define      MSCIID_CFG_GPRS_SIM_UNBLOCK  2055
#define      MSCIID_CFG_GPRS_RADIO_BAND   2056
#define      MSCIID_CFG_GPRS_BAND         2057
#define      MSCIID_CFG_GPRS_BAND_STATUS  2058
#define      MSCIID_CFG_GPRS_LAST_ULOCK   2059
#define      MSCIID_CFG_GPRS_LAST_UBLOCK  2060
#define      MSCIID_CFG_GPRS_LAST_TRY     2061
//#define    MSCIID_CFG_GPRS_AT_GROUP_LAST  2058

#define      MSCIID_CFG_GPRS_CFG_GROUP    2150
#define      MSCIID_CFG_GPRS_APN          2151
#define      MSCIID_CFG_GPRS_DIVERSITY    2152
#define      MSCIID_CFG_GPRS_SIM_ENABLE   2153
#define      MSCIID_CFG_GPRS_USE_IMEI     2154
#define    MSCIID_CFG_GPRS_CFG_GROUP_LAST 2154

//#define    MSCIID_CFG_CDPD_GROUP          2304
#define      MSCIID_CFG_CDPD_3W_POWER     2305
#define      MSCIID_CFG_CDPD_RSSI_HYS     2306
#define      MSCIID_CFG_CDPD_SERVICE_ID   2307
#define      MSCIID_CFG_CDPD_CHANLST_MODE 2308
#define      MSCIID_CFG_CDPD_CHANLST      2309
#define      MSCIID_CFG_CDPD_SRV_ID_PREF  2310
#define      MSCIID_CFG_CDPD_SIDE_PREF    2311
#define      MSCIID_CFG_CDPD_DISABLE_SIDE 2312
#define      MSCIID_CFG_CDPD_NETWORK_IP   2313
#define    MSCIID_CFG_RIMGPRS_GROUP       2560
#define      MSCIID_CFG_RIMGPRS_STATUSCHK 2562
#define    MSCIID_CFG_WCCDMA_GROUP        2688
#define      MSCIID_CFG_WCCDMA_STATUSCHK  2690
#define      MSCIID_CFG_WCCDMA_ROAM_PREF  2691
#define    MSCIID_CFG_ENET_GROUP          2720
#define      MSCIID_CFG_DHCP_SVR_ENABLE   2722
#define      MSCIID_CFG_LINK_ETH_TO_RADIO 2723
#define      MSCIID_CFG_RADIO_LINK_DELAY  2724
#define      MSCIID_CFG_PING_RESPONSE     2725
#define      MSCIID_CFG_HOST2_DEST        2726
#define      MSCIID_CFG_HOST2_GATEWAY     2727
#define      MSCIID_CFG_HOST3_DEST        2728
#define      MSCIID_CFG_HOST3_GATEWAY     2729
#define      MSCIID_CFG_SNMP_CONTACT      2730
#define      MSCIID_CFG_SNMP_NAME         2731
#define      MSCIID_CFG_SNMP_LOCATION     2732
#define      MSCIID_CFG_SNMP_LOCATION     2732
#define      MSCIID_CFG_CMN_FRIEND_IP10   2733
#define      MSCIID_CFG_CMN_FRIEND_IP11   2734
#define      MSCIID_CFG_CMN_FRIEND_IP12   2735
#define      MSCIID_CFG_CMN_FRIEND_IP13   2736
#define      MSCIID_CFG_CMN_FRIEND_IP14   2737
#define      MSCIID_CFG_CMN_FRIEND_IP15   2738
#define      MSCIID_CFG_CMN_FRIEND_IP16   2739
#define      MSCIID_CFG_CMN_FRIEND_IP17   2740
#define      MSCIID_CFG_CMN_FRIEND_IP18   2741
#define      MSCIID_CFG_CMN_FRIEND_IP19   2742
#define      MSCIID_CFG_CMN_FRIEND_IP20   2743
#define      MSCIID_CFG_CMN_FRIEND_IP21   2744
#define      MSCIID_CFG_CMN_FRIEND_IP22   2745
#define      MSCIID_CFG_CMN_FRIEND_IP23   2746
#define      MSCIID_CFG_CMN_FRIEND_IP24   2747
#define      MSCIID_CFG_DHCP_MTU_SIZE        2748
#define      MSCIID_CFG_DHCP_LEASE_TIME      2749
#define      MSCIID_CFG_CMN_LOCAL_PING_IP    2750
#define      MSCIID_STS_LAST_PING            2751
#define      MSCIID_BTN_PING_LOCAL_IP        2752    //special command to ping a local ip address
#define      MSCIID_CFG_NEW_DNS_PORT         2753


#define      MSCIID_CFG_IDEN_GROUP        2944
#define      MSCIID_CFG_IDEN_UDP_FWD      2946
#define      MSCIID_CFG_IDEN_TCP_FWD      2947

#define      MSCIID_CFG_SEC_GROUP         3027
#define      MSCIID_CFG_TELNET_SSH        3029
#define      MSCIID_CFG_MAKE_SSH_KEYS     3030
#define      MSCIID_STS_SSH_STATUS        3031
#define      MSCIID_CFG_SSH_PORT          3032
#define      MSCIID_CFG_SSH_SESSION_TOUT  3033
#define      MSCIID_CFG_SSH_MAX_LOGIN_TRY 3034

//#define    MSCIID_CFG_EVDO_GROUP          3100
#define      MSCIID_CFG_EVDO_DIVERSITY    3102
#define      MSCIID_CFG_EVDO_DATASERV     3103
#define      MSCIID_CFG_EVDO_ROAM_PREF    3104
#define      MSCIID_NAMLCK_VAL            3105
#define      MSCIID_MDN_VAL               3106
#define      MSCIID_MIN_VAL               3107
#define      MSCIID_CMD_PROVISION_RADIO   3110
#define      MSCIID_STS_PROVISION_RADIO   3111


//#define    MSCIID_CFG_IPSEC_GROUP         3150
#define      MSCIID_CFG_TUNNEL_TYPE       3151
#define      MSCIID_CFG_IKE_TYPE          3152
#define      MSCIID_CFG_IKE_GATEWAY       3153
#define      MSCIID_CFG_IKE_GW_MASK       3154
#define      MSCIID_CFG_IKE_PSK1          3155
#define      MSCIID_CFG_IKE_PSK2          3156
#define      MSCIID_CFG_IKE_LID_TYPE      3157    //  Local ID
#define      MSCIID_CFG_IKE_LID_IP        3158
#define      MSCIID_CFG_IKE_RID_TYPE      3159    //  Local ID
#define      MSCIID_CFG_IKE_RID_IP        3160
#define      MSCIID_CFG_IKE_MODE          3161
#define      MSCIID_CFG_IKE_ENCRYPT       3162
#define      MSCIID_CFG_IKE_AUTH          3163
#define      MSCIID_CFG_IKE_DH            3164
#define      MSCIID_CFG_IKE_SALT          3165    //  SA Life Time
#define      MSCIID_CFG_IPSEC_LA_TYPE     3166    //  LA = Local Address
#define      MSCIID_CFG_IPSEC_LA_ADDR1    3167
#define      MSCIID_CFG_IPSEC_LA_ADDR2    3168
#define      MSCIID_CFG_IPSEC_RA_TYPE     3169    //  RA = Remote Address
#define      MSCIID_CFG_IPSEC_RA_ADDR1    3170
#define      MSCIID_CFG_IPSEC_RA_ADDR2    3171
#define      MSCIID_CFG_IPSEC_ENCRYPT     3172
#define      MSCIID_CFG_IPSEC_AUTH        3173
#define      MSCIID_CFG_IPSEC_DH          3174
#define      MSCIID_CFG_IPSEC_SALT        3175    //  SA Lift Time
#define      MSCIID_STS_IPSEC_STATE       3176
#define      MSCIID_CFG_IPSEC_INBOUND     3177
#define      MSCIID_CFG_IPSEC_OB_ALEOS    3178
#define      MSCIID_CFG_IPSEC_OB_HOST     3179
#define      MSCIID_CFG_TUNNEL_TYPE2      3180
#define      MSCIID_CFG_IKE2_TYPE         3181
#define      MSCIID_CFG_IKE2_GATEWAY      3182
#define      MSCIID_CFG_IKE2_GW_MASK      3183
#define      MSCIID_CFG_IKE2_PSK1         3184
#define      MSCIID_CFG_IKE2_PSK2         3185
#define      MSCIID_CFG_IKE2_LID_TYPE     3186    //  Local ID
#define      MSCIID_CFG_IKE2_LID_IP       3187
#define      MSCIID_CFG_IKE2_RID_TYPE     3188    //  Local ID
#define      MSCIID_CFG_IKE2_RID_IP       3189
#define      MSCIID_CFG_IKE2_MODE         3190
#define      MSCIID_CFG_IKE2_ENCRYPT      3191
#define      MSCIID_CFG_IKE2_AUTH         3192
#define      MSCIID_CFG_IKE2_DH           3193
#define      MSCIID_CFG_IKE2_SALT         3194    //  SA Life Time
#define      MSCIID_CFG_IPSEC2_LA_TYPE    3195    //  LA = Local Address
#define      MSCIID_CFG_IPSEC2_LA_ADDR1   3196
#define      MSCIID_CFG_IPSEC2_LA_ADDR2   3197
#define      MSCIID_CFG_IPSEC2_RA_TYPE    3198    //  RA = Remote Address
#define      MSCIID_CFG_IPSEC2_RA_ADDR1   3199
#define      MSCIID_CFG_IPSEC2_RA_ADDR2   3200
#define      MSCIID_CFG_IPSEC2_ENCRYPT    3201
#define      MSCIID_CFG_IPSEC2_AUTH       3202
#define      MSCIID_CFG_IPSEC2_DH         3203
#define      MSCIID_CFG_IPSEC2_SALT       3204    //  SA Lift Time
#define      MSCIID_STS_IPSEC2_STATE      3205

#define      MSCIID_CFG_TUNNEL_TYPE3      3206
#define      MSCIID_CFG_IKE3_TYPE         3207
#define      MSCIID_CFG_IKE3_GATEWAY      3208
#define      MSCIID_CFG_IKE3_GW_MASK      3209
#define      MSCIID_CFG_IKE3_PSK1         3210
#define      MSCIID_CFG_IKE3_PSK2         3211
#define      MSCIID_CFG_IKE3_LID_TYPE     3212    //  Local ID
#define      MSCIID_CFG_IKE3_LID_IP       3213
#define      MSCIID_CFG_IKE3_RID_TYPE     3214    //  Local ID
#define      MSCIID_CFG_IKE3_RID_IP       3215
#define      MSCIID_CFG_IKE3_MODE         3216
#define      MSCIID_CFG_IKE3_ENCRYPT      3217
#define      MSCIID_CFG_IKE3_AUTH         3218
#define      MSCIID_CFG_IKE3_DH           3219
#define      MSCIID_CFG_IKE3_SALT         3220    //  SA Life Time
#define      MSCIID_CFG_IPSEC3_LA_TYPE    3221    //  LA = Local Address
#define      MSCIID_CFG_IPSEC3_LA_ADDR1   3222
#define      MSCIID_CFG_IPSEC3_LA_ADDR2   3223
#define      MSCIID_CFG_IPSEC3_RA_TYPE    3224    //  RA = Remote Address
#define      MSCIID_CFG_IPSEC3_RA_ADDR1   3225
#define      MSCIID_CFG_IPSEC3_RA_ADDR2   3226
#define      MSCIID_CFG_IPSEC3_ENCRYPT    3227
#define      MSCIID_CFG_IPSEC3_AUTH       3228
#define      MSCIID_CFG_IPSEC3_DH         3229
#define      MSCIID_CFG_IPSEC3_SALT       3230    //  SA Lift Time
#define      MSCIID_STS_IPSEC3_STATE      3231

#define      MSCIID_CFG_TUNNEL_TYPE4      3232
#define      MSCIID_CFG_IKE4_TYPE         3233
#define      MSCIID_CFG_IKE4_GATEWAY      3234
#define      MSCIID_CFG_IKE4_GW_MASK      3235
#define      MSCIID_CFG_IKE4_PSK1         3236
#define      MSCIID_CFG_IKE4_PSK2         3237
#define      MSCIID_CFG_IKE4_LID_TYPE     3238    //  Local ID
#define      MSCIID_CFG_IKE4_LID_IP       3239
#define      MSCIID_CFG_IKE4_RID_TYPE     3240    //  Local ID
#define      MSCIID_CFG_IKE4_RID_IP       3241
#define      MSCIID_CFG_IKE4_MODE         3242
#define      MSCIID_CFG_IKE4_ENCRYPT      3243
#define      MSCIID_CFG_IKE4_AUTH         3244
#define      MSCIID_CFG_IKE4_DH           3245
#define      MSCIID_CFG_IKE4_SALT         3246    //  SA Life Time
#define      MSCIID_CFG_IPSEC4_LA_TYPE    3247    //  LA = Local Address
#define      MSCIID_CFG_IPSEC4_LA_ADDR1   3248
#define      MSCIID_CFG_IPSEC4_LA_ADDR2   3249
#define      MSCIID_CFG_IPSEC4_RA_TYPE    3250    //  RA = Remote Address
#define      MSCIID_CFG_IPSEC4_RA_ADDR1   3251
#define      MSCIID_CFG_IPSEC4_RA_ADDR2   3252
#define      MSCIID_CFG_IPSEC4_ENCRYPT    3253
#define      MSCIID_CFG_IPSEC4_AUTH       3254
#define      MSCIID_CFG_IPSEC4_DH         3255
#define      MSCIID_CFG_IPSEC4_SALT       3256    //  SA Lift Time
#define      MSCIID_STS_IPSEC4_STATE      3257

#define      MSCIID_CFG_TUNNEL_TYPE5      3258
#define      MSCIID_CFG_IKE5_TYPE         3259
#define      MSCIID_CFG_IKE5_GATEWAY      3260
#define      MSCIID_CFG_IKE5_GW_MASK      3261
#define      MSCIID_CFG_IKE5_PSK1         3262
#define      MSCIID_CFG_IKE5_PSK2         3263
#define      MSCIID_CFG_IKE5_LID_TYPE     3264    //  Local ID
#define      MSCIID_CFG_IKE5_LID_IP       3265
#define      MSCIID_CFG_IKE5_RID_TYPE     3266    //  Local ID
#define      MSCIID_CFG_IKE5_RID_IP       3267
#define      MSCIID_CFG_IKE5_MODE         3268
#define      MSCIID_CFG_IKE5_ENCRYPT      3269
#define      MSCIID_CFG_IKE5_AUTH         3270
#define      MSCIID_CFG_IKE5_DH           3271
#define      MSCIID_CFG_IKE5_SALT         3272    //  SA Life Time
#define      MSCIID_CFG_IPSEC5_LA_TYPE    3273    //  LA = Local Address
#define      MSCIID_CFG_IPSEC5_LA_ADDR1   3274
#define      MSCIID_CFG_IPSEC5_LA_ADDR2   3275
#define      MSCIID_CFG_IPSEC5_RA_TYPE    3276    //  RA = Remote Address
#define      MSCIID_CFG_IPSEC5_RA_ADDR1   3277
#define      MSCIID_CFG_IPSEC5_RA_ADDR2   3278
#define      MSCIID_CFG_IPSEC5_ENCRYPT    3279
#define      MSCIID_CFG_IPSEC5_AUTH       3280
#define      MSCIID_CFG_IPSEC5_DH         3281
#define      MSCIID_CFG_IPSEC5_SALT       3282    //  SA Lift Time
#define      MSCIID_STS_IPSEC5_STATE      3283
//#define    MSCIID_CFG_IPSEC_GROUP_LAST    3257

//VPN Perfect forward security
#define      MSCIID_VPN1_PFS      3289
#define      MSCIID_VPN2_PFS      3290
#define      MSCIID_VPN3_PFS      3291
#define      MSCIID_VPN4_PFS      3292
#define      MSCIID_VPN5_PFS      3293

//VPN FQDN
#define      MSCIID_CFG_IKE_LID_FQDN      3305
#define      MSCIID_CFG_IKE_RID_FQDN      3306
#define      MSCIID_CFG_IKE2_LID_FQDN     3307
#define      MSCIID_CFG_IKE2_RID_FQDN     3308
#define      MSCIID_CFG_IKE3_LID_FQDN     3309
#define      MSCIID_CFG_IKE3_RID_FQDN     3310
#define      MSCIID_CFG_IKE4_LID_FQDN     3311
#define      MSCIID_CFG_IKE4_RID_FQDN     3312
#define      MSCIID_CFG_IKE5_LID_FQDN     3313
#define      MSCIID_CFG_IKE5_RID_FQDN     3314


// number used
#define  PF_MAX_ENTRIES   5       // max 19
#define  FW_MAX_IP_RANGES 3       // max 9
#define  FW_MAX_PORTS     5       // max 19
#define  FW_MAX_O_IPS     5       // max 10

//#define    MSCIID_NET_UBER_GROUP          3500
//#define      MSCIID_NET_ELEM_GROUP        3501
#define      MSCIID_NET_PF_ENTRIES        3502
#define      MSCIID_NET_DEF_IF            3503
#define      MSCIID_NET_DEF_IP            3504
#define      MSCIID_FW_PORTS_WOB          3505
#define      MSCIID_FW_O_PORTS_WOB        3506
#define      MSCIID_FW_O_IP_ENABLE        3507
#define      MSCIID_NET_MAC_FILTER_WOB    3508
#define      MSCIID_NET_MAC_FILTER_ENABLE 3509
#define      MSCIID_NET_PRIMARY_GATEWAY   3510
#define      MSCIID_NET_SPI_LEVEL         3511


//#define      MSCIID_NET_ELEM_GROUP_LAST   3509

//#define      MSCIID_NET_LIST_GROUP            3600
#define        MSCIID_NET_PF_IN_PORT_LIST     3620
#define          MSCIID_NET_PF_IN_PORT_01     3621
//#define          MSCIID_NET_PF_IN_PORT_LAST   (MSCIID_NET_PF_IN_PORT_LIST + PF_MAX_ENTRIES - 1)
//#define        MSCIID_NET_PF_OUT_IF_LIST      3640
#define          MSCIID_NET_PF_OUT_IF_01      3641
#define          MSCIID_NET_PF_OUT_IF_LAST    (MSCIID_NET_PF_OUT_IF_LIST + PF_MAX_ENTRIES - 1)
#define        MSCIID_NET_PF_OUT_IP_LIST      3660
#define          MSCIID_NET_PF_OUT_IP_01      3661
//#define          MSCIID_NET_PF_OUT_IP_LAST    (MSCIID_NET_PF_OUT_IP_LIST + PF_MAX_ENTRIES - 1)
//#define        MSCIID_NET_PF_OUT_PORT_LIST    3680
#define          MSCIID_NET_PF_OUT_PORT_01    3681
//#define          MSCIID_NET_PF_OUT_PORT_LAST  (MSCIID_NET_PF_OUT_PORT_LIST + PF_MAX_ENTRIES - 1)
//#define        MSCIID_NET_FW_IP_START_LIST    3700
#define          MSCIID_NET_FW_IP_START_01    3701
//#define          MSCIID_NET_FW_IP_START_LAST  (MSCIID_NET_FW_IP_START_LIST + FW_MAX_IP_RANGES - 1)
//#define        MSCIID_NET_FW_IP_END_LIST      3710
#define          MSCIID_NET_FW_IP_END_01      3711
//#define          MSCIID_NET_FW_IP_END_LAST    (MSCIID_NET_FW_IP_END_LIST + FW_MAX_IP_RANGES - 1)
//#define        MSCIID_NET_FW_PORTS_LIST       3720
#define          MSCIID_NET_FW_PORTS_01       3721
//#define          MSCIID_NET_FW_PORTS_LAST     (MSCIID_NET_FW_PORTS_LIST + FW_MAX_PORTS - 1)
//#define        MSCIID_NET_FW_PORTS_END_LIST     3740
#define          MSCIID_NET_FW_PORTS_END_01     3741
//#define          MSCIID_NET_FW_PORTS_END_LAST (MSCIID_NET_FW_PORTS_END_LIST + FW_MAX_PORTS - 1)
//#define        MSCIID_NET_FW_O_PORTS_LIST         3760
#define          MSCIID_NET_FW_O_PORTS_01         3761
//#define          MSCIID_NET_FW_O_PORTS_LAST     (MSCIID_NET_FW_O_PORTS_LIST + FW_MAX_PORTS - 1)
//#define        MSCIID_NET_FW_O_PORTS_END_LIST     3780
#define          MSCIID_NET_FW_O_PORTS_END_01     3781
//#define          MSCIID_NET_FW_O_PORTS_END_LAST (MSCIID_NET_FW_O_PORTS_END_LIST + FW_MAX_PORTS - 1)
//#define        MSCIID_NET_PF_IN_PORT_END_LIST     3800
#define          MSCIID_NET_PF_IN_PORT_END_01     3801
//#define          MSCIID_NET_PF_IN_PORT_END_LAST (MSCIID_NET_PF_IN_PORT_END_LIST + PF_MAX_ENTRIES - 1)

//#define        MSCIID_NET_MAC_FILTER_KEY_LIST    3820
#define          MSCIID_NET_MAC_FILTER_KEY_01    3821
//#define          MSCIID_NET_MAC_FILTER_KEY_LAST  3840

//#define        MSCIID_NET_FW_O_IPS_LIST            3849
#define          MSCIID_NET_FW_O_IPS_01            3850
//#define          MSCIID_NET_FW_O_IPS_LAST       (MSCIID_NET_FW_O_IPS_01 + FW_MAX_O_IPS - 1)

//#define      MSCIID_NET_LIST_GROUP_LAST   3898
//#define    MSCIID_NET_UBER_GROUP_LAST     3899

//#define  MSCIID_CFG_GROUP_LAST            3999


//#define    MSCIID_EIO_UBER_GROUP          4000
//#define      MSCIID_PULSE_CNT_LIST        4001
#define        MSCIID_PULSE_CNT_01        4002
#define        MSCIID_PULSE_CNT_LAST      4007    // 6 in list
//#define      MSCIID_IO_A_COEF_LIST        4010
#define        MSCIID_IO_A_COEF_01        4011
#define        MSCIID_IO_A_COEF_LAST      4014    // 4 in list
//#define      MSCIID_IO_A_OFFSET_LIST      4020
#define        MSCIID_IO_A_OFFSET_01      4021
#define        MSCIID_IO_A_OFFSET_LAST    4024
//#define      MSCIID_IO_A_UNITS_LIST       4030
#define        MSCIID_IO_A_UNITS_01       4031
#define        MSCIID_IO_A_UNITS_LAST     4034    // 4 in list
//#define      MSCIID_SCALED_ANALOG_LIST    4040
#define        MSCIID_SCALED_ANALOG_01    4041
#define        MSCIID_SCALED_ANALOG_LAST  4044    // 4 in list
//#define    MSCIID_EIO_UBER_GROUP_LAST     4099


//#define  MSCIID_WIFI_UBER_GROUP               4500
//#define    MSCIID_WIFI_ELEM_GROUP             4501
#define      MSCIID_WIFI_ENABLE               4506
#define      MSCIID_WIFI_SSID                 4507
#define      MSCIID_WIFI_CHANNEL              4508
#define      MSCIID_WIFI_SECURITY_TYPE        4509
#define      MSCIID_WIFI_WEP_KEY              4510
#define      MSCIID_WIFI_SSID_HIDE            4511
#define      MSCIID_RAD_SERVER1_IP            4512
#define      MSCIID_RAD_SERVER1_PORT          4513
#define      MSCIID_RAD_SERVER1_SECRET        4514
#define      MSCIID_RAD_SERVER2_IP            4515
#define      MSCIID_RAD_SERVER2_PORT          4516
#define      MSCIID_RAD_SERVER2_SECRET        4517
#define      MSCIID_RAD_INTERFACE             4518
#define      MSCIID_WIFI_WEP_PASSPHRASE       4519
#define      MSCIID_WIFI_WEP_TYPE             4520
#define      MSCIID_WIFI_ENCRYPTION_TYPE      4521
//#define    MSCIID_WIFI_ELEM_GROUP_LAST        4518


//#define    MSCIID_WIFI_LIST_GROUP             4600
//#define    MSCIID_WIFI_NETWORK_KEY_LIST     4605
#define      MSCIID_WIFI_NETWORK_KEY_01       4606
#define      MSCIID_WIFI_NETWORK_KEY_LAST     4616



// WIFI1
#define     MSCIID_WIFI_1_ENABLE                4531
#define     MSCIID_WIFI_1_SSID                  4532
#define     MSCIID_WIFI_1_CHANNEL               4533
#define     MSCIID_WIFI_1_SECURITY_TYPE         4534
#define     MSCIID_WIFI_1_WEP_KEY               4535
#define     MSCIID_WIFI_1_SSID_HIDE             4536
#define     MSCIID_RAD_1_SERVER1_IP             4537
#define     MSCIID_RAD_1_SERVER1_PORT           4538
#define     MSCIID_RAD_1_SERVER1_SECRET         4539
#define     MSCIID_RAD_1_SERVER2_IP             4540
#define     MSCIID_RAD_1_SERVER2_PORT           4541
#define     MSCIID_RAD_1_SERVER2_SECRET         4542
#define     MSCIID_RAD_1_INTERFACE              4543
#define     MSCIID_WIFI_1_WEP_PASSPHRASE        4544
#define     MSCIID_WIFI_1_WEP_TYPE              4545
#define     MSCIID_WIFI_1_ENCRYPTION_TYPE       4546
#define     MSCIID_WIFI_1_NETWORK_KEY_01        4626

// WIFI2
#define     MSCIID_WIFI_2_ENABLE                4556
#define     MSCIID_WIFI_2_SSID                  4557
#define     MSCIID_WIFI_2_CHANNEL               4558
#define     MSCIID_WIFI_2_SECURITY_TYPE         4559
#define     MSCIID_WIFI_2_WEP_KEY               4560
#define     MSCIID_WIFI_2_SSID_HIDE             4561
#define     MSCIID_RAD_2_SERVER1_IP             4562
#define     MSCIID_RAD_2_SERVER1_PORT           4563
#define     MSCIID_RAD_2_SERVER1_SECRET         4564
#define     MSCIID_RAD_2_SERVER2_IP             4565
#define     MSCIID_RAD_2_SERVER2_PORT           4566
#define     MSCIID_RAD_2_SERVER2_SECRET         4567
#define     MSCIID_RAD_2_INTERFACE              4568
#define     MSCIID_WIFI_2_WEP_PASSPHRASE        4569
#define     MSCIID_WIFI_2_WEP_TYPE              4570
#define     MSCIID_WIFI_2_ENCRYPTION_TYPE       4571
#define     MSCIID_WIFI_2_NETWORK_KEY_01        4646

// WIFI3
#define     MSCIID_WIFI_3_ENABLE                4581
#define     MSCIID_WIFI_3_SSID                  4582
#define     MSCIID_WIFI_3_CHANNEL               4583
#define     MSCIID_WIFI_3_SECURITY_TYPE         4584
#define     MSCIID_WIFI_3_WEP_KEY               4585
#define     MSCIID_WIFI_3_SSID_HIDE             4586
#define     MSCIID_RAD_3_SERVER1_IP             4587
#define     MSCIID_RAD_3_SERVER1_PORT           4588
#define     MSCIID_RAD_3_SERVER1_SECRET         4589
#define     MSCIID_RAD_3_SERVER2_IP             4590
#define     MSCIID_RAD_3_SERVER2_PORT           4591
#define     MSCIID_RAD_3_SERVER2_SECRET         4592
#define     MSCIID_RAD_3_INTERFACE              4593
#define     MSCIID_WIFI_3_WEP_PASSPHRASE        4594
#define     MSCIID_WIFI_3_WEP_TYPE              4595
#define     MSCIID_WIFI_3_ENCRYPTION_TYPE       4596
#define     MSCIID_WIFI_3_NETWORK_KEY_01        4666

//#define    MSCIID_WIFI_LIST_GROUP_LAST        4699
//#define  MSCIID_WIFI_UBER_GROUP_LAST          4999

#define      MSCIID_VIEWER_PASSWD             5000
#define      STS_FIELD_COMMANDER              5001
#define      CELL_OVERRIDE_DEFAULTS_ENABLED   5002
#define      ADMIN_PASS                       5003
#define      BOOTUP_RESET_ENABLED             5004
#define      CONFIDENTIAL                     5005
#define      STS_CONNECTED_CLIENTS            5006
#define      DDNS_FQDN                        5007
#define      DDNS_INTERVAL                    5008
#define      DDNS_LOGIN                       5009
#define      DDNS_PASSWORD                    5010
#define      DDNS_SERVICE                     5011
#define      DDNS_SUPPORTED_SERVICES_LIST     5012
#define      DEBUG_BAUD_RATE                  5013  //NO DISPLAY
#define      DEBUG_DEV_PORT                   5014  //NO DISPLAY
#define      DEBUG_ENABLED                    5015  //NO DISPLAY
#define      DEBUG_OUTPUT                     5016  //NO DISPLAY
#define      DEBUG_TCP_PORT                   5017  //NO DISPLAY
#define      DHCP_LEASE_TIME                  5018
#define      STS_ETHERNET_PORT_1        5020
#define      STS_ETHERNET_PORT_2        5021
#define      FC_ENABLED                 5022
#define      FC_IDENTITY                5023
#define      FC_INTERVAL                5024
#define      FC_LOGIN                   5025
#define      FC_STATUS                5026
#define      FC_SERVER                  5027

#define      GDNS_ENABLED               5030
#define      GRE_TTL                    5033
#define      LAN_IFACES                 5034 //NO DISPLAY

#define      MODEL_NUMBER               5037 //deprecated use 7 instead
//#define      PB_ALLOWED_RANGES_LIST     3761     MSCIID_NET_FW_O_PORTS_01
#define      UPLINK_STATE               5040
#define      SPLASH_BYPASS_DOMAINS_LIST 5042
#define      SPLASH_ENABLED             5043
#define      SPLASH_FILENAME            5044
#define      SPLASH_TIMEOUT             5045
#define      STS_TIME_IN_USE            5046
#define      STS_TRAFFIC_OVER_WAN       5047

#define      VPN_AUTHENTICATION_TYPE    5049
#define      VPN_DPD_ACTION             5050
#define      VPN_DPD_DELAY              5051
#define      VPN_DPD_ENABLED            5052
#define      VPN_DPD_TIMEOUT            5053

#define      VPN_RSA_CA_CERT            5066
#define      VPN_RSA_HOST_CERT          5067
#define      VPN_RSA_HOST_KEY           5068
#define      VPN_RSA_HOST_KEY_PASS      5069
#define      VPN_RSA_SERVER_CERT        5070

#define      VPN_XAUTH_ENABLED          5074 //NO DISPLAY
#define      VPN_XAUTH_PASSWORD         5075 //NO DISPLAY
#define      VPN_XAUTH_USERNAME         5076 //NO DISPLAY
#define      STS_WAN_LINK               5077
#define      WIFI_IFACE                 5079
#define      WIFI_NETWORK_MODE          5080

#define      UNUSED                     5087

#define      WJ_PORT_DNS1               5092
#define      WJ_PORT_DNS2               5093
#define      WJ_PORT_ENABLE_DHCP        5094
#define      WJ_PORT_GW                 5095
#define      WJ_PORT_IFACE              5096 //NO DISPLAY
#define      WJ_PORT_IP                 5097
#define      WJ_PORT_NETMASK            5098 //NO DISPLAY
#define      WJ_PRIMARY_IFACE           5099
#define      WJ_PRIMARY_ROUTE_INTERVAL  5100
#define      WJ_PRIMARY_ROUTE_IP        5101
#define      STS_WWAN_GATEWAY           5102
#define      CELL_DIAL                  5103
#define      STS_NODES_NAME_IP_MAC_LIST 5105
#define      UPDATE_CHECK_VER           5106
#define      UPDATE_FIRMWARE            5107
#define      UPDATE_RESET_FACTORY_DEFAULT 5108
#define      STS_WAN_IP                 5110 //DEPRECATED USE 301
#define      STS_EWAN_IP                5111
#define      PF_ENABLED                 5112
#define      DMZ_ENABLED                5113
#define      WAN_ETH_PORT               5114
#define      MSCIID_CFG_LANDINGPAGE_EN  5115
#define      MSCIID_CFG_LANDINGPAGE_URL 5116

#define      MSCIID_CFG_VPN1_TTL      5201
#define      MSCIID_CFG_VPN2_TTL      5202
#define      MSCIID_CFG_VPN3_TTL      5203
#define      MSCIID_CFG_VPN4_TTL      5204
#define      MSCIID_CFG_VPN5_TTL      5205

#define      MSCIID_L2TP_PPP_UNAMCE         5210
#define      MSCIID_L2TP_PPP_PWD            5211
#define      MSCIID_L2TP_PPP_SERVER         5212
#define      MSCIID_L2TP_PPP_IP             5213
#define      MSCIID_L2TP_PPP_AUTH_PAP       5214
#define      MSCIID_L2TP_PPP_AUTH_CPAP      5215
#define      MSCIID_L2TP_PPP_AUTH_MSCHAP1   5216
#define      MSCIID_L2TP_PPP_AUTH_MSCHAP2   5217
#define      MSCIID_L2TP_PPP_NET_IP        5218
#define      MSCIID_L2TP_PPP_NET_MASK       5219

#define      MSCIID_SMS_UBER_GROUP              6000
#define      MSCIID_SMS_ELEM_GROUP            6001
#define      MSCIID_SMS_PHONE                 6002
#define      MSCIID_SMS_STARTUP_ACTION        6003    // probably no display
#define      MSCIID_SMS_EVER_ONLINE           6004    // was the modem ever online
#define      MSCIID_SMS_INCLUDE_PHONE         6005    //
#define      MSCIID_SMS_LAST_PHONE            6006
#define      MSCIID_SMS_PASSTHRU_DEST         6007
#define      MSCIID_SMS_HOST_IP               6008
#define      MSCIID_SMS_HOST_PORT             6009
#define      MSCIID_SMS_PROTOCOL              6010
#define      MSCIID_SMS_ETH2SMS_ENABLE        6011
#define      MSCIID_SMS_ETH2SMS_PORT          6012
#define      MSCIID_SMS_PROTO_START           6013
#define      MSCIID_SMS_PROTO_SEP             6014
#define      MSCIID_SMS_PROTO_END             6015
#define      MSCIID_SMS_PROTO_ACK             6016
#define      MSCIID_SMS_PROTO_BODY_FMT        6017
#define      MSCIID_SMS_LAST_MESSAGE          6018
#define      MSCIID_SMS_ELEM_GROUP_LAST       6018
#define      MSCIID_SMS_MODE                  6019
#define      MSCIID_SMS_CTRLCMD_PREFIX        6020
#define      MSCIID_SMS_SECURITY              6021

#define      MSCIID_SMS_LIST_GROUP            6028
#define        MSCIID_SMS_FRIENDS_LIST        6029
#define          MSCIID_SMS_FRIENDS_01        6030
#define          MSCIID_SMS_FRIENDS_LAST      6039
#define        MSCIID_SMS_GET_SEC_LIST        6070    //  in case they need more that 10
#define          MSCIID_SMS_GET_SEC_01        6071
#define          MSCIID_SMS_GET_SEC_LAST      6076
#define        MSCIID_SMS_SET_SEC_LIST        6080
#define          MSCIID_SMS_SET_SEC_01        6081
#define          MSCIID_SMS_SET_SEC_LAST      6086
#define      MSCIID_SMS_LIST_GROUP_LAST       6090
#define    MSCIID_SMS_UBER_GROUP_LAST         6099



#define      VLAN_ENABLED                   8001

#define      MSCIID_CFG_LAN_IF_TYPE_1         8010
#define      MSCIID_CFG_LAN_IF_TYPE_2         8011
#define      MSCIID_CFG_LAN_IF_TYPE_3         8012
#define      MSCIID_CFG_LAN_IF_TYPE_4         8013
#define      MSCIID_CFG_LAN_IF_TYPE_5         8014
#define      MSCIID_CFG_LAN_IF_TYPE_6         8015
#define      MSCIID_CFG_LAN_IF_TYPE_7         8016

#define      MSCIID_CFG_VLAN_ID_1             8020
#define      MSCIID_CFG_VLAN_ID_2             8021
#define      MSCIID_CFG_VLAN_ID_3             8022
#define      MSCIID_CFG_VLAN_ID_4             8023
#define      MSCIID_CFG_VLAN_ID_5             8024
#define      MSCIID_CFG_VLAN_ID_6             8025
#define      MSCIID_CFG_VLAN_ID_7             8026

#define      MSCIID_CFG_IF_IP_1               8030
#define      MSCIID_CFG_IF_IP_2               8031
#define      MSCIID_CFG_IF_IP_3               8032
#define      MSCIID_CFG_IF_IP_4               8033
#define      MSCIID_CFG_IF_IP_5               8034
#define      MSCIID_CFG_IF_IP_6               8035
#define      MSCIID_CFG_IF_IP_7               8036

#define      MSCIID_CFG_IF_SUBNETMASK_1       8040
#define      MSCIID_CFG_IF_SUBNETMASK_2       8041
#define      MSCIID_CFG_IF_SUBNETMASK_3       8042
#define      MSCIID_CFG_IF_SUBNETMASK_4       8043
#define      MSCIID_CFG_IF_SUBNETMASK_5       8044
#define      MSCIID_CFG_IF_SUBNETMASK_6       8045
#define      MSCIID_CFG_IF_SUBNETMASK_7       8046


#define      MSCIID_CFG_INTERNET_ACCESS_1     8050
#define      MSCIID_CFG_INTERNET_ACCESS_2     8051
#define      MSCIID_CFG_INTERNET_ACCESS_3     8052
#define      MSCIID_CFG_INTERNET_ACCESS_4     8053
#define      MSCIID_CFG_INTERNET_ACCESS_5     8054
#define      MSCIID_CFG_INTERNET_ACCESS_6     8055
#define      MSCIID_CFG_INTERNET_ACCESS_7     8056

#define      MSCIID_CFG_DHCP_ENABLED_1        8060
#define      MSCIID_CFG_DHCP_ENABLED_2        8061
#define      MSCIID_CFG_DHCP_ENABLED_3        8062
#define      MSCIID_CFG_DHCP_ENABLED_4        8063
#define      MSCIID_CFG_DHCP_ENABLED_5        8064
#define      MSCIID_CFG_DHCP_ENABLED_6        8065
#define      MSCIID_CFG_DHCP_ENABLED_7        8066

#define      MSCIID_CFG_START_IP_1            8070
#define      MSCIID_CFG_START_IP_2            8071
#define      MSCIID_CFG_START_IP_3            8072
#define      MSCIID_CFG_START_IP_4            8073
#define      MSCIID_CFG_START_IP_5            8074
#define      MSCIID_CFG_START_IP_6            8075
#define      MSCIID_CFG_START_IP_7            8076
#define      MSCIID_CFG_END_IP_1              8080
#define      MSCIID_CFG_END_IP_2              8081
#define      MSCIID_CFG_END_IP_3              8082
#define      MSCIID_CFG_END_IP_4              8083
#define      MSCIID_CFG_END_IP_5              8084
#define      MSCIID_CFG_END_IP_6              8085
#define      MSCIID_CFG_END_IP_7              8086


//VRRP Group MSCIIDs
#define      MSCIID_VRRP_ENABLED              9001
#define      MSCIID_VRRP_INTERFACE            9002
#define      MSCIID_VRRP_AUTH_TYPE            9003
#define      MSCIID_VRRP_AUTH_PASS            9004
#define      MSCIID_VRRP_STATE0               9010
#define      MSCIID_VRRP_STATE1               9011
#define      MSCIID_VRRP_STATE2               9012
#define      MSCIID_VRRP_STATE3               9013
#define      MSCIID_VRRP_ROUTER_ID0           9020
#define      MSCIID_VRRP_ROUTER_ID1           9021
#define      MSCIID_VRRP_ROUTER_ID2           9022
#define      MSCIID_VRRP_ROUTER_ID3           9023
#define      MSCIID_VRRP_PRIORITY0            9030
#define      MSCIID_VRRP_PRIORITY1            9031
#define      MSCIID_VRRP_PRIORITY2            9032
#define      MSCIID_VRRP_PRIORITY3            9033
#define      MSCIID_VRRP_AD_INTERVAL0         9040
#define      MSCIID_VRRP_AD_INTERVAL1         9041
#define      MSCIID_VRRP_AD_INTERVAL2         9042
#define      MSCIID_VRRP_AD_INTERVAL3         9043
#define      MSCIID_VRRP_IP0                  9050
#define      MSCIID_VRRP_IP1                  9051
#define      MSCIID_VRRP_IP2                  9052
#define      MSCIID_VRRP_IP3                  9053
#define      MSCIID_VRRP_ENUM0                9060
#define      MSCIID_VRRP_ENUM1                9061
#define      MSCIID_VRRP_ENUM2                9062
#define      MSCIID_VRRP_ENUM3                9063
#define      MSCIID_VRRP_ENUM4                9064
#define      MSCIID_VRRP_ENUM5                9065
#define      MSCIID_VRRP_ENUM6                9066


//SSL VPN Group MSCIIDs
//OPENVPN related
#define      MSCIID_CFG_OPENVPN_ROLE              10001 //client
#define      MSCIID_CFG_OPENVPN_MODE              10002 //routing
#define      MSCIID_CFG_OPENVPN_PROTO             10003 //udp
#define      MSCIID_CFG_OPENVPN_PORT              10004
#define      MSCIID_CFG_OPENVPN_PEER_ID           10005
#define      MSCIID_CFG_OPENVPN_CA_CERT_PATH      10006  // Used by GUI
#define      MSCIID_CFG_OPENVPN_CA_CERT           10007
#define      MSCIID_CFG_OPENVPN_CIPHER            10008
#define      MSCIID_CFG_OPENVPN_AUTH              10009
#define      MSCIID_CFG_OPENVPN_USERNAME          10010
#define      MSCIID_CFG_OPENVPN_PASSWORD          10011


// MDEX
#define      MSCIID_CFG_OPENVPN_TUN_MTU           10020 // mdex= 1500
#define      MSCIID_CFG_OPENVPN_FRAG              10021 // mdex= 1300
#define      MSCIID_CFG_OPENVPN_RENEG_SEC         10022 // mdex = 86400 24hrs
#define      MSCIID_CFG_OPENVPN_NAT               10023 // mdex: fixedip requires NAT
#define      MSCIID_CFG_OPENVPN_COMP_LZO          10024 // mdex = enable
#define      MSCIID_CFG_OPENVPN_MSS_FIX           10025 // 1500
#define      MSCIID_CFG_OPENVPN_PEER_DYN_IP       10026 // mdex= 1500
#define      MSCIID_CFG_OPENVPN_PING_INTERVAL     10027
#define      MSCIID_CFG_OPENVPN_PING_RESTART      10028

// SNMP New
#define      MSCIID_CFG_SNMP_ENABLE         10040
#define      MSCIID_CFG_SNMP_VERSION        10041
#define      MSCIID_CFG_SNMP_PORT           10042
#define      MSCIID_CFG_SNMP_TRAPPORT       10043
#define      MSCIID_CFG_SNMP_ENGINEID       10044
#define      MSCIID_CFG_SNMP_ROUSER         10045
#define      MSCIID_CFG_SNMP_ROUSER_SECLVL      10046
#define      MSCIID_CFG_SNMP_ROUSER_AUTHTYPE    10047
#define      MSCIID_CFG_SNMP_ROUSER_AUTHKEY     10048
#define      MSCIID_CFG_SNMP_ROUSER_PRIVTYPE    10049
#define      MSCIID_CFG_SNMP_ROUSER_PRIVKEY     10050
#define      MSCIID_CFG_SNMP_RWUSER             10051
#define      MSCIID_CFG_SNMP_RWUSER_SECLVL      10052
#define      MSCIID_CFG_SNMP_RWUSER_AUTHTYPE    10053
#define      MSCIID_CFG_SNMP_RWUSER_AUTHKEY     10054
#define      MSCIID_CFG_SNMP_RWUSER_PRIVTYPE    10055
#define      MSCIID_CFG_SNMP_RWUSER_PRIVKEY     10056
#define      MSCIID_CFG_SNMP_TRAP_USER          10057
#define      MSCIID_CFG_SNMP_TRAP_SECLVL        10058
#define      MSCIID_CFG_SNMP_TRAP_AUTHTYPE      10059
#define      MSCIID_CFG_SNMP_TRAP_AUTHKEY       10060
#define      MSCIID_CFG_SNMP_TRAP_PRIVTYPE      10061
#define      MSCIID_CFG_SNMP_TRAP_PRIVKEY       10062
#define      MSCIID_CFG_SNMP_ROCOMMUNITY        10063
#define      MSCIID_CFG_SNMP_RWCOMMUNITY        10064
#define      MSCIID_CFG_SNMP_TRAP_COMMUNITY     10065
#define      MSCIID_CFG_CMN_ETH_DEVICE      10066


#define     MSCIID_RADIOMODULE_ACTIVATED        10100
#define     MSCIID_AUTODMZ_IP                   10101
#define     MSCIID_HIF_READY                     10103
#define     MSCIID_INF_CORE_SW_VER              10104
#define     MSCIID_WATCHDOG                     10105

#define      MSCIID_CFG_IO_RESERVE_SERIAL0      10120
#define      MSCIID_CFG_IO_RESERVE_SERIAL1      10121
#define      MSCIID_CFG_IO_RESERVE_XDIGITAL     10122
#define      MSCIID_CFG_IO_RESERVE_XACCEL       10123
#define      MSCIID_CFG_IO_RESERVE_XANALOG      10124


#define      MSCIID_CFG_CMN_DEFAULT_MODE2       10150
#define      MSCIID_CFG_CMN_HOST_COMM2          10151
#define      MSCIID_CFG_CMN_HOST_FLOWCTL2       10152
#define      MSCIID_CFG_CMN_SERIAL_ECHO2        10153
#define      MSCIID_CFG_CMN_DATA_FWD_TO2        10154
#define      MSCIID_CFG_CMN_DISABLE_ATESC2      10155

#define      MSCIID_CFG_CMN_TCP_AUTO_ANS2       10170
#define      MSCIID_CFG_CMN_DEV_PORT2           10171
#define      MSCIID_CFG_CMN_DEST_ADDR2          10172
#define      MSCIID_CFG_CMN_DEST_PORT2          10173
#define      MSCIID_CFG_CMN_DIALCODE2           10174
#define      MSCIID_CFG_CMN_TCP_CNT_TO2         10175
#define      MSCIID_CFG_CMN_TCP_IDLE_TO2        10176
#define      MSCIID_CFG_CMN_TCP_IDLE_SECS2      10177
#define      MSCIID_CFG_CMN_TCP_CNT_DELAY2      10178
#define      MSCIID_CFG_CMN_ALLOW_ALL_UDP2      10179
#define      MSCIID_CFG_CMN_UDP_SER_DELAY2      10180
#define      MSCIID_CFG_CMN_UDP_AUTO_ANS2       10181
#define      MSCIID_CFG_CMN_UDP_IDLE_TO2        10182
#define      MSCIID_CFG_CMN_UDP_ANS_RESP2       10183
#define      MSCIID_CFG_CMN_UDP_CNT_LAST2       10184
#define      MSCIID_CFG_CMN_ALLOW_ANY_IP2       10185
#define      MSCIID_CFG_CMN_DATA_FWD_CHAR2      10186
#define      MSCIID_STS_LINK_STATE2             10187
#define      MSCIID_CFG_CMN_QUIET_MODE2         10189

// USB Bypass
#define      MSCIID_CFG_CMN_USB_BYPASS      10200
#define      MSCIID_CFG_USB_INTERNET_EN     10201
#define      MSCIID_STS_DMZ_IN_USE          10202


#define      MSCIID_GROUP_COMMON            10300
#define      MSCIID_GROUP_GSM               10301
#define      MSCIID_GROUP_CDMA              10302
#define      MSCIID_GROUP_LTE               10303
#define      MSCIID_GROUP_SERIAL            10304
#define      MSCIID_GROUP_GPS               10305
#define      MSCIID_GROUP_X_WIFI            10306
#define      MSCIID_GROUP_X_DIO             10307
#define      MSCIID_GROUP_X_ETHERNET        10308
#define      MSCIID_GROUP_X_SERIAL          10309
#define      MSCIID_GROUP_X_ACCEL           10310
#define      MSCIID_GROUP_X_AIO             10311
#define      MSCIID_GROUP_X_CANBUS          10312
#define      MSCIID_GROUP_UNUSED            10313
#define      MSCIID_GROUP_GSM_LTE           10314
#define      MSCIID_GROUP_CDMA_LTE          10315

#define      MSCIID_CFG_WIFI_MAX_CLIENT     10400
#define      MSCIID_CFG_WIFI_BRIDGE_EN      10401
#define      MSCIID_CFG_WIFI_TX_POWER       10403
#define      MSCIID_CFG_WIFI_AP_ISOLATION   10404
#define      MSCIID_STS_WIFI_TX_PKT_COUNT   10405
#define      MSCIID_STS_WIFI_RX_PKT_COUNT   10406
#define      MSCIID_STS_X_CARD_STATE        10407
#define      MSCIID_X_CARD_TYPE             10408


#define      MSCIID_CFG_GPS_RAW_NMEA        10900
#define      MSCIID_STS_GPS_SUSPENDED        10901
#define      MSCIID_STS_GPS_NMEA_1              10907
#define      MSCIID_STS_GPS_NMEA_2              10908
#define      MSCIID_STS_GPS_NMEA_3              10909
#define      MSCIID_STS_GPS_NMEA_4              10910
#define      MSCIID_STS_GPS_NMEA_5              10911
#define      MSCIID_STS_GPS_NMEA_6              10912
#define      MSCIID_STS_GPS_NMEA_7              10913
#define      MSCIID_STS_GPS_NMEA_8              10914
#define      MSCIID_STS_PP_GPS_ALTITUDE     10915
#define      MSCIID_STS_PP_GPS_TIME_HH      10916
#define      MSCIID_STS_PP_GPS_TIME_MM      10917
#define      MSCIID_STS_PP_GPS_TIME_SS       10918
#define      MSCIID_STS_PP_GPS_TIME_YR       10919
#define      MSCIID_STS_PP_GPS_TIME_MH       10920
#define      MSCIID_STS_PP_GPS_TIME_DY       10921
#define      MSCIID_STS_PP_GPS_FIX                              10922
#define      MSCIID_STS_PP_GPS_TICKS_SINCE_FIX       10923
#define      MSCIID_STS_PP_ENGINE_MIN      10924
#define      MSCIID_STS_PP_GPS_LAT7         10925
#define      MSCIID_STS_PP_GPS_LONG7        10926
#define      MSCIID_STS_PP_IGNITION_SENSE       10927
#define      MSCIID_STS_RAP_DEVICEID       10928

#define      MSCIID_CFG_CMN_LOW_PWR_MODE  11110
#define      MSCIID_CFG_CMN_LOW_PWR_WAKE_VLTG  11111
#define      MSCIID_CFG_CMN_LOW_PWR_DUR_ACTIVE  11112
#define      MSCIID_CFG_CMN_LOW_PWR_DUR_INACTIVE  11113
#define      MSCIID_CFG_CMN_LOW_PWR_WAKEUP  11114
#define      MSCIID_STS_CMN_LOW_PWR_NXT_UP_WDAY  11115
#define      MSCIID_STS_CMN_LOW_PWR_NXT_UP_HOUR  11116
#define      MSCIID_STS_CMN_LOW_PWR_NXT_UP_MIN  11117
#define      MSCIID_STS_CMN_LOW_PWR_NXT_DOWN_WDAY  11118
#define      MSCIID_STS_CMN_LOW_PWR_NXT_DOWN_HOUR  11119
#define      MSCIID_STS_CMN_LOW_PWR_NXT_DOWN_MIN  11120
#define      MSCIID_STS_CMN_LOW_PWR_SLEEP  11121

#define      MSCIID_CFG_CMN_IPMSRVR1_UPD_TYPE  11169
#define      MSCIID_CFG_CMN_IPMSRVR2_UPD_TYPE  11170

#define      MSCIID_CFG_CMN_APN_TYPE  11200
#define      MSCIID_CFG_CMN_APN_SELECTED  11201
#define      MSCIID_STS_CMN_APN_CURRENT  11202

#define      MSCIID_STS_SMS_MULTICAST  11301
#define      MSCIID_CFG_APPDEV_WD_DISABLE  11302

#define      MSCIID_CFG_PP_LATS_PORT     11326
#define      MSCIID_STS_PP_LATS_IP     11327
#define      MSCIID_CFG_PP_LATS_ODOMETER_ENA     11328
#define      MSCIID_CFG_PP_LATS_ENABLE_INPUTS     11329

#define      MSCIID_CFG_RE2_SIMPL_RETRY_TO 11641
#define      MSCIID_CFG_RE2_INPUT_EVENTS   11654
#define      MSCIID_CFG_RE2_ENABLE_INPUTS  11655
#define      MSCIID_CFG_RE2_MAX_SPEED      11656
#define      MSCIID_CFG_RE2_STATIONARY_EVENT 11657
#define      MSCIID_CFG_RE2_RPT_MIN_TIME  11658
#define      MSCIID_CFG_RE2_ODOMETER_ENA   11659
#define      MSCIID_CFG_RE3_SIMPL_RETRY_TO 11661
#define      MSCIID_CFG_RE3_INPUT_EVENTS   11674
#define      MSCIID_CFG_RE3_ENABLE_INPUTS  11675
#define      MSCIID_CFG_RE3_MAX_SPEED      11676
#define      MSCIID_CFG_RE3_STATIONARY_EVENT 11677
#define      MSCIID_CFG_RE3_RPT_MIN_TIME  11678
#define      MSCIID_CFG_RE3_ODOMETER_ENA   11679
#define      MSCIID_CFG_RE4_SIMPL_RETRY_TO 11681
#define      MSCIID_CFG_RE4_INPUT_EVENTS   11694
#define      MSCIID_CFG_RE4_ENABLE_INPUTS  11695
#define      MSCIID_CFG_RE4_MAX_SPEED      11696
#define      MSCIID_CFG_RE4_STATIONARY_EVENT 11697
#define      MSCIID_CFG_RE4_RPT_MIN_TIME  11698
#define      MSCIID_CFG_RE4_ODOMETER_ENA   11699
#define    MSCIID_CFG_RE5_CFG_GROUP       11700
//#define      MSCIID_CFG_RE5_FIRST          11701
#define      MSCIID_CFG_RE5_SIMPL_RETRY_TO 11701
//#define      MSCIID_CFG_RE5_GPS_RPT_TYPE   11702
#define      MSCIID_CFG_RE5_SERVER_NAME    11703
#define      MSCIID_CFG_RE5_SERVER_PORT    11704
#define      MSCIID_CFG_RE5_SNF_ENABLE     11705
#define      MSCIID_CFG_RE5_SNF_TYPE       11706
#define      MSCIID_CFG_RE5_SNF_MIN_RPTS   11707
#define      MSCIID_CFG_RE5_SNF_REL_MODE   11708
#define      MSCIID_CFG_RE5_MAX_RETRIES    11709
#define      MSCIID_CFG_RE5_USE_DEVICEID   11710
//#define      MSCIID_CFG_RE5_RPT_INT_TIME   11711
//#define      MSCIID_CFG_RE5_RPT_INT_DIST   11712
//#define      MSCIID_CFG_RE5_VS_TIME_INT    11713
#define      MSCIID_CFG_RE5_ENABLE_INPUTS  11714
//#define      MSCIID_CFG_RE5_INPUT_EVENTS   11715
//#define      MSCIID_CFG_RE5_MAX_SPEED      11716
//#define      MSCIID_CFG_RE5_STATIONARY_EVENT 11717
#define      MSCIID_CFG_RE5_RPT_MIN_TIME  11718
#define      MSCIID_CFG_RE5_ODOMETER_ENA   11719
//#define    MSCIID_CFG_RE5_GROUP_LAST       11719

#define    MSCIID_CFG_RE6_CFG_GROUP       11720
//#define      MSCIID_CFG_RE6_FIRST          11721
#define      MSCIID_CFG_RE6_SIMPL_RETRY_TO 11721
//#define      MSCIID_CFG_RE6_GPS_RPT_TYPE   11722
#define      MSCIID_CFG_RE6_SERVER_NAME    11723
#define      MSCIID_CFG_RE6_SERVER_PORT    11724
#define      MSCIID_CFG_RE6_SNF_ENABLE     11725
#define      MSCIID_CFG_RE6_SNF_TYPE       11726
#define      MSCIID_CFG_RE6_SNF_MIN_RPTS   11727
#define      MSCIID_CFG_RE6_SNF_REL_MODE   11728
#define      MSCIID_CFG_RE6_MAX_RETRIES    11729
#define      MSCIID_CFG_RE6_USE_DEVICEID   11730
//#define      MSCIID_CFG_RE6_RPT_INT_TIME   11731
//#define      MSCIID_CFG_RE6_RPT_INT_DIST   11732
//#define      MSCIID_CFG_RE6_VS_TIME_INT    11733
#define      MSCIID_CFG_RE6_ENABLE_INPUTS  11734
//#define      MSCIID_CFG_RE6_INPUT_EVENTS   11735
//#define      MSCIID_CFG_RE6_MAX_SPEED      11736
//#define      MSCIID_CFG_RE6_STATIONARY_EVENT 11737
#define      MSCIID_CFG_RE6_RPT_MIN_TIME  11738
#define      MSCIID_CFG_RE6_ODOMETER_ENA   11739
//#define    MSCIID_CFG_RE6_GROUP_LAST       11739

#define    MSCIID_CFG_RE7_CFG_GROUP       11740
//#define      MSCIID_CFG_RE7_FIRST          11741
#define      MSCIID_CFG_RE7_SIMPL_RETRY_TO 11741
//#define      MSCIID_CFG_RE7_GPS_RPT_TYPE   11742
#define      MSCIID_CFG_RE7_SERVER_NAME    11743
#define      MSCIID_CFG_RE7_SERVER_PORT    11744
#define      MSCIID_CFG_RE7_SNF_ENABLE     11745
#define      MSCIID_CFG_RE7_SNF_TYPE       11746
#define      MSCIID_CFG_RE7_SNF_MIN_RPTS   11747
#define      MSCIID_CFG_RE7_SNF_REL_MODE   11748
#define      MSCIID_CFG_RE7_MAX_RETRIES    11749
#define      MSCIID_CFG_RE7_USE_DEVICEID   11750
//#define      MSCIID_CFG_RE7_RPT_INT_TIME   11751
//#define      MSCIID_CFG_RE7_RPT_INT_DIST   11752
//#define      MSCIID_CFG_RE7_VS_TIME_INT    11753
#define      MSCIID_CFG_RE7_ENABLE_INPUTS  11754
//#define      MSCIID_CFG_RE7_INPUT_EVENTS   11755
//#define      MSCIID_CFG_RE7_MAX_SPEED      11756
//#define      MSCIID_CFG_RE7_STATIONARY_EVENT 11757
#define      MSCIID_CFG_RE7_RPT_MIN_TIME  11758
#define      MSCIID_CFG_RE7_ODOMETER_ENA   11759
//#define    MSCIID_CFG_RE7_GROUP_LAST       11759

#define    MSCIID_CFG_RE8_CFG_GROUP       11760
//#define      MSCIID_CFG_RE8_FIRST          11761
#define      MSCIID_CFG_RE8_SIMPL_RETRY_TO 11761
//#define      MSCIID_CFG_RE8_GPS_RPT_TYPE   11762
#define      MSCIID_CFG_RE8_SERVER_NAME    11763
#define      MSCIID_CFG_RE8_SERVER_PORT    11764
#define      MSCIID_CFG_RE8_SNF_ENABLE     11765
#define      MSCIID_CFG_RE8_SNF_TYPE       11766
#define      MSCIID_CFG_RE8_SNF_MIN_RPTS   11767
#define      MSCIID_CFG_RE8_SNF_REL_MODE   11768
#define      MSCIID_CFG_RE8_MAX_RETRIES    11769
#define      MSCIID_CFG_RE8_USE_DEVICEID   11770
//#define      MSCIID_CFG_RE8_RPT_INT_TIME   11771
//#define      MSCIID_CFG_RE8_RPT_INT_DIST   11772
//#define      MSCIID_CFG_RE8_VS_TIME_INT    11773
#define      MSCIID_CFG_RE8_ENABLE_INPUTS  11774
//#define      MSCIID_CFG_RE8_INPUT_EVENTS   11775
//#define      MSCIID_CFG_RE8_MAX_SPEED      11776
//#define      MSCIID_CFG_RE8_STATIONARY_EVENT 11777
#define      MSCIID_CFG_RE8_RPT_MIN_TIME  11778
#define      MSCIID_CFG_RE8_ODOMETER_ENA   11779
//#define    MSCIID_CFG_RE8_GROUP_LAST       11779

#define    MSCIID_CFG_RE9_CFG_GROUP       11780
//#define      MSCIID_CFG_RE9_FIRST          11781
#define      MSCIID_CFG_RE9_SIMPL_RETRY_TO 11781
//#define      MSCIID_CFG_RE9_GPS_RPT_TYPE   11782
#define      MSCIID_CFG_RE9_SERVER_NAME    11783
#define      MSCIID_CFG_RE9_SERVER_PORT    11784
#define      MSCIID_CFG_RE9_SNF_ENABLE     11785
#define      MSCIID_CFG_RE9_SNF_TYPE       11786
#define      MSCIID_CFG_RE9_SNF_MIN_RPTS   11787
#define      MSCIID_CFG_RE9_SNF_REL_MODE   11788
#define      MSCIID_CFG_RE9_MAX_RETRIES    11789
#define      MSCIID_CFG_RE9_USE_DEVICEID   11790
//#define      MSCIID_CFG_RE9_RPT_INT_TIME   11791
//#define      MSCIID_CFG_RE9_RPT_INT_DIST   11792
//#define      MSCIID_CFG_RE9_VS_TIME_INT    11793
#define      MSCIID_CFG_RE9_ENABLE_INPUTS  11794
//#define      MSCIID_CFG_RE9_INPUT_EVENTS   11795
//#define      MSCIID_CFG_RE9_MAX_SPEED      11796
//#define      MSCIID_CFG_RE9_STATIONARY_EVENT 11797
#define      MSCIID_CFG_RE9_RPT_MIN_TIME  11798
#define      MSCIID_CFG_RE9_ODOMETER_ENA   11799
//#define    MSCIID_CFG_RE9_GROUP_LAST       11799


// IPv6 Related MSCIIDs 12000 series
#define      MSCIID_STS_NETWORK_IPV6     12001
#define      MSCIID_STS_NETWORK_IPV6PREFIXLEN     12002
#define      MSCIID_CFG_NETWORK_IPV6_PREF     12003 // IPv4/IPv6 Preference- doesn't mean you will have requested connection
#define      MSCIID_STS_NETWORK_IPV6_CONN     12004 // Indicates if current connection is IPv4 or IPv6
#define      MSCIID_STS_IPV6_DNS1     12005
#define      MSCIID_STS_IPV6_DNS2     12006
#define      MSCIID_CFG_IPV6_DNS_OVERRIDE     12007
#define      MSCIID_CFG_IPV6_USER_DNS1     12008
#define      MSCIID_CFG_IPV6_USER_DNS2     12009


//#define    MSCIID_CFG_DBG_GROUP          16384
//#define      MSCIID_CFG_DBG_CFG_GROUP    16385
#define      MSCIID_CFG_PPP_LOGGING      16386
#define      MSCIID_CFG_COMM_LOGGING     16387
#define      MSCIID_CFG_INIT_STRING      16388
#define      MSCIID_CFG_ENABLE_DBUG_UDP  16389
#define      MSCIID_CFG_DBG_AT_GROUP     16416
#define      MSCIID_CFG_IP_LOGGING       16417
#define      MSCIID_CFG_SYSLOG_STR       16418   // remove later
#define      MSCIID_CFG_DBG_AT_GRP_LAST  16417
#define      MSCIIF_CFG_DBG_ENET_GROUP   16432
#define      MSCIID_CFG_DBG_ENET_LOGGING 16434
#define      MSCIID_CFG_DBG_DHCP_LOGGING 16435

//Logging Verbosity MSCIIDs
#define      MSCIID_CFG_WAN_LOGGING      17001
#define      MSCIID_CFG_LAN_LOGGING  17002
#define      MSCIID_CFG_VPN_LOGGING  17003
#define      MSCIID_CFG_SECURITY_LOGGING     17004
#define      MSCIID_CFG_SERVICES_LOGGING  17005
#define      MSCIID_CFG_EVENTS_LOGGING     17006
#define      MSCIID_CFG_SERIAL_LOGGING   17007
#define      MSCIID_CFG_APPS_LOGGING      17008
#define      MSCIID_CFG_UI_LOGGING  17009
#define      MSCIID_CFG_AMS_LOGGING  17010
#define      MSCIID_CFG_ADMIN_LOGGING  17011
#define      MSCIID_CFG_SYSTEM_LOGGING  17012
//Logging Filtering MSCIIDs
#define      MSCIID_CFG_SYSLOG_LOGFILTER  17100
#define      MSCIID_CFG_WAN_LOGFILTER 17101
#define      MSCIID_CFG_LAN_LOGFILTER 17102
#define      MSCIID_CFG_VPN_LOGFILTER 17103
#define      MSCIID_CFG_SECURITY_LOGFILTER 17104
#define      MSCIID_CFG_SERVICES_LOGFILTER 17105
#define      MSCIID_CFG_EVENTS_LOGFILTER 17106
#define      MSCIID_CFG_SERIAL_LOGFILTER 17107
#define      MSCIID_CFG_APPS_LOGFILTER 17108
#define      MSCIID_CFG_UI_LOGFILTER 17109
#define      MSCIID_CFG_AMS_LOGFILTER 17110
#define      MSCIID_CFG_ADMIN_LOGFILTER 17111
#define      MSCIID_CFG_SYSTEM_LOGFILTER 17112
//Logging Subsystem MSCIIDs
#define      MSCIID_CFG_WAN_SUBSYSTEM 17501
#define      MSCIID_CFG_LAN_SUBSYSTEM 17502
#define      MSCIID_CFG_VPN_SUBSYSTEM 17503
#define      MSCIID_CFG_SECURITY_SUBSYSTEM 17504
#define      MSCIID_CFG_SERVICES_SUBSYSTEM 17505
#define      MSCIID_CFG_EVENTS_SUBSYSTEM 17506
#define      MSCIID_CFG_SERIAL_SUBSYSTEM 17507
#define      MSCIID_CFG_APPS_SUBSYSTEM 17508
#define      MSCIID_CFG_UI_SUBSYSTEM 17509
#define      MSCIID_CFG_AMS_SUBSYSTEM 17510
#define      MSCIID_CFG_ADMIN_SUBSYSTEM 17511
#define      MSCIID_CFG_SYSTEM_SUBSYSTEM 17512

//Log Configuration Logging MSCIIDs
#define      MSCIID_CFG_LOGCFG_LOG_FREQ     17601
#define      MSCIID_CFG_LOGCFG_LOG_START    17602


// changing these changes CONFIG I"D  so stored values will be screwed
#define    EVENT_TABLE_SIZE   30
#define    REPORT_TABLE_SIZE  13
#define    REPORT_GRP_COUNT   12
#define    REPORT_GRP_SIZE    10
//#define    MSCIID_EVENTS_REPORTS_UBER_GROUP   20000
//#define      MSCIID_EVENT_UBER_GROUP          20001
//#define        MSCIID_EVENT_MSCI_LIST         20100
#define          MSCIID_EVENT_MSCI_01         20101
#define          MSCIID_EVENT_MSCI_LAST      (20100 + EVENT_TABLE_SIZE)
//#define        MSCIID_EVENT_OPR_LIST          20200
#define          MSCIID_EVENT_OPR_01          20201
#define          MSCIID_EVENT_OPR_LAST       (20200 + EVENT_TABLE_SIZE)
//#define        MSCIID_EVENT_VALUE_LIST        20300
#define          MSCIID_EVENT_VALUE_01        20301
#define          MSCIID_EVENT_VALUE_LAST     (20300 + EVENT_TABLE_SIZE)
//#define        MSCIID_EVENT_EXTRA_LIST        20400
#define          MSCIID_EVENT_EXTRA_01        20401
#define          MSCIID_EVENT_EXTRA_LAST     (20400 + EVENT_TABLE_SIZE)
//#define        MSCIID_EVENT_RPT_BF_LIST       20500       // report  bitfield
#define          MSCIID_EVENT_RPT_BF_01       20501
#define          MSCIID_EVENT_RPT_BF_LAST    (20500 + EVENT_TABLE_SIZE)
//#define        MSCIID_EVENT_RPT_STS_LIST      20600
#define          MSCIID_EVENT_RPT_STS_01      20601
#define          MSCIID_EVENT_RPT_STS_LAST   (20600 + EVENT_TABLE_SIZE)
//#define        MSCIID_EVENT_RPT_CFG_LIST      20700
#define          MSCIID_EVENT_RPT_CFG_01      20701
#define          MSCIID_EVENT_RPT_CFG_LAST   (20700 + EVENT_TABLE_SIZE)
//#define        MSCIID_EVENT_RPT_GRP_LIST      20800
#define          MSCIID_EVENT_RPT_GRP_01      20801
#define          MSCIID_EVENT_RPT_GRP_LAST   (20800 + EVENT_TABLE_SIZE)
//#define        MSCIID_EVENT_DESCRIPTION_LIST      20900
#define          MSCIID_EVENT_DESCRIPTION_01      20901
#define          MSCIID_EVENT_DESCRIPTION_LAST   (20900 + EVENT_TABLE_SIZE)
//#define      MSCIID_EVENT_UBER_GROUP_LAST     20999

//#define      MSCIID_REPORTS_UBER_GROUP        21001
//#define        MSCIID_REPORT_SHOW_LIST        21100
#define          MSCIID_REPORT_SHOW_01        21101
#define          MSCIID_REPORT_SHOW_LAST     (21100 + REPORT_TABLE_SIZE)
//#define        MSCIID_REPORT_TYPE_LIST        21200
#define          MSCIID_REPORT_TYPE_01        21201
#define          MSCIID_REPORT_TYPE_LAST     (21200 + REPORT_TABLE_SIZE)
//#define        MSCIID_REPORT_TO_LIST          21300
#define          MSCIID_REPORT_TO_01          21301
#define          MSCIID_REPORT_TO_LAST       (21300 + REPORT_TABLE_SIZE)
//#define        MSCIID_REPORT_SUB_LIST         21400
#define          MSCIID_REPORT_SUB_01         21401
#define          MSCIID_REPORT_SUB_LAST      (21400 + REPORT_TABLE_SIZE)
//#define        MSCIID_REPORT_MSG_LIST         21500
#define          MSCIID_REPORT_MSG_01         21501
#define          MSCIID_REPORT_MSG_LAST      (21500 + REPORT_TABLE_SIZE)
//#define        MSCIID_REPORT_OBJ_LIST         21600
#define          MSCIID_REPORT_OBJ_01         21601
#define          MSCIID_REPORT_OBJ_LAST      (21600 + REPORT_TABLE_SIZE * REPORT_GRP_COUNT) // 12 * 10
//#define        MSCIID_REPORT_EVENT_BF_LIST       21700
#define          MSCIID_REPORT_EVENT_BF_01       21701
#define          MSCIID_REPORT_EVENT_BF_LAST    (21700 + REPORT_TABLE_SIZE )
//#define        MSCIID_REPORT_BODY_LIST       21800
#define          MSCIID_REPORT_BODY_01       21801
#define          MSCIID_REPORT_BODY_LAST    (21800 + REPORT_TABLE_SIZE )
//#define        MSCIID_ACTION_DESCRIPTION_LIST       21900
#define          MSCIID_ACTION_DESCRIPTION_01       21901
#define          MSCIID_ACTION_DESCRIPTION_LAST    (21900 + REPORT_TABLE_SIZE )
//#define      MSCIID_REPORTS_UBER_GROUP_LAST   21999

//#define      MSCIID_DATA_GROUPS_UBER_GROUP    23001
//#define        MSCIID_DATA_GRP_SHOW_LIST      23100
#define          MSCIID_DATA_GRP_SHOW_01      23101
#define          MSCIID_DATA_GRP_SHOW_LAST   (23100 + REPORT_GRP_COUNT)
//#define        MSCIID_DATA_GRP_LEN_LIST       23200
#define          MSCIID_DATA_GRP_LEN_01       23201
#define          MSCIID_DATA_GRP_LEN_LAST    (23200 + REPORT_GRP_COUNT)
//#define        MSCIID_DATA_GRP_MSCI_LIST      23300
#define          MSCIID_DATA_GRP_MSCI_01      23301
#define          MSCIID_DATA_GRP_MSCI_LAST   (23300 + REPORT_GRP_COUNT * REPORT_GRP_SIZE)
//#define        MSCIID_DATA_GRP_ENABLE_LIST    23600
#define          MSCIID_DATA_GRP_ENABLE_01    23601
#define          MSCIID_DATA_GRP_ENABLE_LAST (23600 + REPORT_GRP_COUNT * REPORT_GRP_SIZE)
//#define      MSCIID_DATA_GROUPS_UBER_GROUP_LAST  23999

//#define      MSCIID_ER_SETUP_GROUP            24000
#define        MSCIID_ER_SETUP_PARADYMN       24001
#define        MSCIID_ER_SETUP_EVENT_MAX      24002
#define        MSCIID_ER_SHOW_EVENT_MSCIS     24003
#define        MSCIID_ER_SHOW_EVENT_GROUPS    24004
#define        MSCIID_ER_SHOW_EVENT_INDEXS    24005
#define        MSCIID_ER_SETUP_REPORTS_MAX    24006
#define        MSCIID_ER_SHOW_MULTI_RPT       24007
#define        MSCIID_ER_SETUP_RPT_GRP_MAX    24008
// #define        MSCIID_ER_SETUP_TEMPLATE       24009
//#define      MSCIID_ER_SETUP_GROUP_LAST       24008

//#define    MSCIID_EVENTS_REPORTS_UBER_GROUP_LAST   24999

#define MSCIID_STS_CURRENT_DAILY_DATA_USAGEMB   25001
#define MSCIID_STS_CURRENT_MONTHLY_DATA_USAGEMB 25002
#define MSCIID_STS_CURRENT_SERVICE_STATE    25003
#define MSCIID_CFG_STARTOF_MONTHLY_BILLING  25004
#define MSCIID_CFG_DAILY_DATA_LIMIT     25005
#define MSCIID_CFG_MONTHLY_DATA_LIMIT       25006
#define MSCIID_CFG_MONTHLY_DATA_USAGE_UNITS 25007
#define MSCIID_STS_CURRENT_DAILY_DATA_USAGEB    25008
#define MSCIID_STS_CURRENT_MONTHLY_DATA_USAGEB  25009
#define MSCIID_STS_END_OF_DAY           25010
#define MSCIID_STS_END_OF_MONTH         25011
#define MSCIID_CFG_STARTOF_MONTHLY_BILLING_BACK 25012
#define MSCIID_STS_USAGE_TXCOUNT        25013
#define MSCIID_STS_USAGE_RXCOUNT        25014
#define MSCIID_STS_PPP_STATE                25015

//#define        MSCIID_REPORT_RELAY_TYPE_LIST          25200
#define          MSCIID_REPORT_RELAY_TYPE_01          25201
#define          MSCIID_REPORT_RELAY_TYPE_LAST       (25200 + REPORT_TABLE_SIZE)

//#define        MSCIID_REPORT_SMS_PHONE_LIST          25300
#define          MSCIID_REPORT_SMS_PHONE_01          25301
#define          MSCIID_REPORT_SMS_PHONE_LAST       (25300 + REPORT_TABLE_SIZE)

//#define        MSCIID_REPORT_SMS_MSG_LIST          25500
#define          MSCIID_REPORT_SMS_MSG_01          25501
#define          MSCIID_REPORT_SMS_MSG_LAST       (25500 + REPORT_TABLE_SIZE)

//#define        MSCIID_EVENT_DU_COMP_LIST          25600
#define          MSCIID_EVENT_DU_COMP_01          25601
#define          MSCIID_EVENT_DU_COMP_LAST       (25600 + EVENT_TABLE_SIZE)

//#define        MSCIID_EVENT_NET_SERV_COMP_LIST          25700
#define          MSCIID_EVENT_NET_SERV_COMP_01          25701
#define          MSCIID_EVENT_NET_SERV_COMP_LAST       (25700 + EVENT_TABLE_SIZE)


//#define    MSCIID_INF_FID_GROUP          32768
#define      MSCIID_INF_SIM_LOCK         32769
#define      MSCIID_INF_MIN_XMIT_REST    32790
#define      MSCIID_INF_FID_MAC_ADDR     32791
#define      MSCIID_INF_INT_PUBL_KEY     32792
#define      MSCIID_INF_INT_PRIV_KEY     32793


#define    MSCIID_WEB_UBER_GROUP              33000
#define      MSCIID_WEB_IPSEC_GROUP           33001
#define        MSCIID_IPSEC_POLICIES_BTN      33002
#define        MSCIID_IPSEC_CONNECT_BTN       33003
#define        MSCIID_IPSEC_CLEAR_LOG_BTN     33004
#define        MSCIID_IPSEC_LOG_LEVEL_BTN     33005
#define      MSCIID_WEB_IPSEC_GROUP_LAST      33005
#define      MSCIID_WEB_ER_GROUP              33100
#define        MSCIID_WEB_ER_EVENT_MSCI       33101
#define        MSCIID_WEB_ER_EVENT_OPR        33102
#define        MSCIID_WEB_ER_EVENT_VALUE      33103
#define        MSCIID_WEB_ER_EVENT_EXTRA      33104
#define        MSCIID_WEB_ER_EVENT_REPORTS    33105
#define        MSCIID_WEB_ER_EVENT_GROUP      33106
#define        MSCIID_WEB_ER_EVENT_UNUSED1    33107
#define        MSCIID_WEB_ER_EVENT_UNUSED2    33108
#define        MSCIID_WEB_ER_EVENT_CREATE     33109
#define        MSCIID_WEB_ER_RPT_TYPE         33110
#define        MSCIID_WEB_ER_RPT_TO           33111
#define        MSCIID_WEB_ER_RPT_SUBJECT      33112
#define        MSCIID_WEB_ER_RPT_MESSAGE      33113
#define        MSCIID_WEB_ER_RPT_UNUSED1      33114
#define        MSCIID_WEB_ER_RPT_UNUSED2      33115
#define        MSCIID_WEB_ER_RPT_UNUSED3      33116
#define        MSCIID_WEB_ER_RPT_UNUSED4      33117
#define        MSCIID_WEB_ER_RPT_UNUSED5      33118
#define        MSCIID_WEB_ER_RPT_UNUSED6      33119
#define        MSCIID_WEB_ER_RPT_CREATE       33120
#define        MSCIID_WEB_ER_RPT_DATA1        33121
#define        MSCIID_WEB_ER_RPT_DATA2        33122
#define        MSCIID_WEB_ER_RPT_DATA3        33123
#define        MSCIID_WEB_ER_RPT_DATA4        33124
#define        MSCIID_WEB_ER_RPT_DATA5        33125
#define        MSCIID_WEB_ER_RPT_DATA6        33126
#define        MSCIID_WEB_ER_RPT_DATA7        33127
#define        MSCIID_WEB_ER_RPT_DATA8        33128
#define        MSCIID_WEB_ER_RPT_DATA9        33129
#define        MSCIID_WEB_ER_RPT_DATA10       33130
#define        MSCIID_WEB_ER_RPT_DESC1        33131
#define        MSCIID_WEB_ER_RPT_DESC2        33132
#define        MSCIID_WEB_ER_RPT_DESC3        33133
#define        MSCIID_WEB_ER_RPT_DESC4        33134
#define        MSCIID_WEB_ER_RPT_DESC5        33135
#define        MSCIID_WEB_ER_RPT_DESC6        33136
#define        MSCIID_WEB_ER_RPT_DESC7        33137
#define        MSCIID_WEB_ER_RPT_DESC8        33138
#define        MSCIID_WEB_ER_RPT_DESC9        33139
#define        MSCIID_WEB_ER_RPT_DESC10       33140
#define      MSCIID_WEB_ER_GROUP_LAST         33140

#define         MSCIID_HELIX_LOG_VERSION        33200

#define         MSCIID_SIM_SLOT_SELECTED        33212

#define     MSCIID_DDNS_UPDATE_TYPE     33213

#define         MSCIID_GSM_RESCAN_ALGO_ENABLE   33211


#define    MSCIID_WEB_UBER_GROUP_LAST         39999

#define         MSCIID_MODEM_DEBUG_PORT        50001
#define         MSCIID_MODEM_DEBUG_LEVEL       50002
#define         MSCIID_SYSTEM_DEBUG_PORT       50003
#define         MSCIID_SYSTEM_DEBUG_LEVEL      50004
#define         MSCIID_CDMA_OTASP_STATE        50005
#define         MSCIID_GPRS_COLD_TEMP_SET      50006


#define         MSCIID_GPRS_QOS_REQ_PRECEDENCE    50007
#define         MSCIID_GPRS_QOS_REQ_DELAY         50008
#define         MSCIID_GPRS_QOS_REQ_RELIABLITY    50009
#define         MSCIID_GPRS_QOS_REQ_PEAK          50010
#define         MSCIID_GPRS_QOS_REQ_MEAN          50011
#define         MSCIID_GPRS_QOS_MIN_PRECEDENCE    50012
#define         MSCIID_GPRS_QOS_MIN_DELAY         50013
#define         MSCIID_GPRS_QOS_MIN_RELIABLITY    50014
#define         MSCIID_GPRS_QOS_MIN_PEAK          50015
#define         MSCIID_GPRS_QOS_MIN_MEAN          50016
#define         MSCIID_GPRS_QOS_REQ_SET           50017
#define         MSCIID_GPRS_QOS_MIN_SET           50018

#define         MSCIID_GPRS_COPS_MODE             50019
#define         MSCIID_GPRS_COPS_FORMAT           50020
#define         MSCIID_GPRS_COPS_OPER             50021
#define         MSCIID_GPRS_SIM_STATUS            50022
#define         MSCIID_GPRS_SIM_UNBLOCK_STATUS    50023

#define         MSCIID_MODEM_PEER_IP              50024
#define         MSCIID_LAST_NETWORK_IP            50025
#define         MSCIID_IPPING_BACKOFF             50026

#define         MSCIID_PINPOINT_GPS_SIMULATE            50027
#define         MSCIID_CFG_CMN_PPP_NO_CARRIER           50028


#define         MSCIID_UPD_ATTEMPT_CNT            50029
#define         MSCIID_UPD_LAST_STATUS            50030
#define         MSCIID_UPD_ALEOS_START            50031
#define         MSCIID_UPD_ALEOS_LENGTH           50032
#define         MSCIID_UPD_ALEOS_CRC              50033
#define         MSCIID_UPD_USER_PASSWD            50034
#define         MSCIID_UPD_EID                    50035
#define         MSCIID_UPD_LDR_ENTRY_SEG          50036
#define         MSCIID_UPD_LDR_ENTRY_OFF          50037
#define         MSCIID_UPD_MDM_ENTRY_SEG          50038
#define         MSCIID_UPD_MDM_ENTRY_OFF          50039

#define         MSCIID_SYSLOG_2_EVENTLOG          50040
#define         MSCIID_MODEM_DOMAIN               50041

#define         MSCIID_SMS_SCA                 50042
#define     MSCIID_SMS_AT_CGSMS          50043



#define         MSCIID_STS_ETH_NET                55001
#define         MSCIID_STS_USB_NET                55002
#define         MSCIID_STS_SMTP_SEND              55003
#define         MSCIID_STS_CSM_READY              55004

#define         MSCIID_GSM_PLMN             55006
#define         MSCIID_GSM_SIM_STATUS             55007
#define         MSCIID_GSM_SIM_UNBLOCK_STATUS     55009
#define         MSCIID_STS_TIME_VALID             55010

#define         MSCIID_SNTP_LAST_CHECK            55042
#define         MSCIID_SNTP_FALLBACK_RETRIES      55043

#define         MSCIID_ENABLE_SMS_RESET           55044
#define         MSCIID_SMS_RESET_LAST_NONCE       55045

#define         MSCIID_SNMP_TRAPPORT              55046
#define         MSCIID_SNMP_TRAPDESTDOMAIN        55047

#define         MSCIID_EM_PERFORM_IOTA            55050

#define         MSCIID_PINPOINT_COM1000_LOGGING   55051



#define   MSCIID_COMMIT_ALL              65000     //special command for webace to commit all
#define   MSCIID_RESET_MODEM             65001     //special command for webace to reset modem
#define   MSCIID_SYS_LOG                 65002     //special command for sys_log data
#define   MSCIID_VPN_LOG                 65003     //special command for VPN_log data
#define   MSCIID_ER_EVENTS               65004     //special command for ER_EVENTS
#define   MSCIID_ER_REPORTS              65005     //special command for ER_REPORTS
#define   MSCIID_SPLASH_DATA             65006
#define   MSCIID_HELIX_LOG               65007
#define   MSCIID_RESET_RADIO             65008     //special command for webace to reset modem
#define   MSCIID_RESET_CONFIG             65009     //special command for webace to reset config
#define   MSCIID_RESET_EVENTLOG             65010     //special command for webace to reset eventlog
#define   MSCIID_RESET_MINXMIT             65011     //special command for webace to reset eventlog
#define   MSCIID_APN_LIST                 65012     //special command for apn list data
#define  MSCIID_INF_DUMMY               65501       // Created for AceManager


#define   MSCIID_LAST_MSCIID              65535

/*................................... TYPES ...............................*/

/*.................................. GLOBALS ..............................*/

/*............................ FUNCTION PROTOTYPES ........................*/


#endif

