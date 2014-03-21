/*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Laurent Barthelemy for Sierra Wireless - initial API and implementation
 *******************************************************************************/

#include <lua.h>
#include <lauxlib.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "ntp.h"

//#include "swi_log.h"

/*
   rfc 4330 is the base used to develop this file
   Simple Network Time Protocol (SNTP) Version 4 for IPv4, IPv6 and OSI

   some excerpts:

   Timestamp Name          ID   When Generated
   ------------------------------------------------------------
   Originate Timestamp     T1   time request sent by client
   Receive Timestamp       T2   time request received by server
   Transmit Timestamp      T3   time reply sent by server
   Destination Timestamp   T4   time reply received by client

   The roundtrip delay *d* and system clock offset *t* are defined as:

   d = (T4 - T1) - (T3 - T2)
   t = ((T2 - T1) + (T3 - T4)) / 2

   Read more: http://www.faqs.org/rfcs/rfc4330.html
   */

/*
 * Macros to convert from/to big endian (network order) without knowing endianess of current hardware
 * as there is no standard way ANSI to get this info.
 * Those macro do not use htonl, be64toh, ... functions as they are not available on Open AT devices.
 * */

#define UINT64_FROM_BIG_ENDIAN(BUFF)  (\
        (((uint64_t)((uint8_t)*(BUFF)))   << 56) \
        |(((uint64_t)((uint8_t)*(BUFF+1))) << 48) \
        |(((uint64_t)((uint8_t)*(BUFF+2))) << 40) \
        |(((uint64_t)((uint8_t)*(BUFF+3))) << 32) \
        |(((uint64_t)((uint8_t)*(BUFF+4))) << 24) \
        |(((uint64_t)((uint8_t)*(BUFF+5))) << 16) \
        |(((uint64_t)((uint8_t)*(BUFF+6))) << 8) \
        |(((uint64_t)((uint8_t)*(BUFF+7))) ) \
        )

#define UINT64_TO_BIG_ENDIAN_BUFF(INT, BUFF)do{ \
    *((uint8_t*)BUFF)    = (uint8_t) ((((INT) & 0xff00000000000000ULL)) >> 56); \
    *(((uint8_t*)BUFF)+1)= (uint8_t) ((((INT) & 0x00ff000000000000ULL)) >> 48); \
    *(((uint8_t*)BUFF)+2)= (uint8_t) ((((INT) & 0x0000ff0000000000ULL)) >> 40); \
    *(((uint8_t*)BUFF)+3)= (uint8_t) ((((INT) & 0x000000ff00000000ULL)) >> 32); \
    *(((uint8_t*)BUFF)+4)= (uint8_t) ((((INT) & 0x00000000ff000000ULL)) >> 24); \
    *(((uint8_t*)BUFF)+5)= (uint8_t) ((((INT) & 0x0000000000ff0000ULL)) >> 16); \
    *(((uint8_t*)BUFF)+6)= (uint8_t) ((((INT) & 0x000000000000ff00ULL)) >> 8 ); \
    *(((uint8_t*)BUFF)+7)= (uint8_t) ((((INT) & 0x00000000000000ffULL))  ); \
} \
while(0)

//Debug functions: to be activated whe packet printing is needed
//void print_pkg(const char* pktname, const uint8_t* pkt) {
//    char str[512];
//    memset(str, 0, 512);
//    int i;
//    for (i = 0; i < 48; i++) {
//        sprintf(str + (3 * i), "%02d ", i);
//    }
//
//    *(str + (3 * 48)) = '\n';
//    char* str2 = (str + (3 * 48) + 1);
//    for (i = 0; i < 48; i++) {
//        sprintf(str2 + (3 * i), "%02x ", *(pkt + i));
//    }
//
//    *(str2 + (3 * 48)) = '\n';
//    SWI_LOG("NTP", DEBUG, "%s:\n%s\n", pktname, str);
//}
//
//void print_uint64_hexa(const char* name, uint64_t val) {
//    char str[25];
//    memset(str, 0, 25);
//    int i;
//    for (i = 7; i >= 0; i--) {
//        sprintf(str + (3 * (7 - i)), "%02x ", (uint8_t) ((val
//                & ((uint64_t) 0xff << (8 * (i)))) >> (8 * i)));
//    }
//    SWI_LOG("NTP", DEBUG,"%s:%s\n", name, str);
//}


//Some Lua macros for Error/Warning return "pattern"

#define LUA_PUSH_ERR(ERR)  do { \
    lua_pushnil(L);lua_pushliteral(L, ERR); return 2; \
} \
while(0)

#define SANITY_WARN(ERR)  do { \
    lua_pushliteral(L, "2"); lua_pushliteral(L, ERR); return 2; \
} \
while(0)


/* buildNTPpacket: build an NTP packet, either a brand new one, or a response to received  one
 *
 * inputs:
 * org and rec are either both NULL (then first NTP packet is created) or both non-NULL to create response packet
 * Usually org and rec are returned by processpackets function.
 * org is originate timestamp of packet to respond to, given as a (string) buffer of size 8
 * rec is reception timestamp of packet to respond to, given as a (string) buffer of size 8.
 *
 * outputs:
 * on success, NTP packet as output in a (string) buffer of size 48.
 * on error, nil+error string
 */
static int l_buildNTPpacket(lua_State *L) {
    uint64_t oriTS = 0, rcvTs = 0, trTs = 0;
    size_t len1 = 0, len2 = 0;
    const char *org = luaL_optlstring(L, 1, NULL, &len1);
    const char *rec = luaL_optlstring(L, 2, NULL, &len2);

    if (org != NULL || rec != NULL) {
        //both param must be given together and have size of 8
        if (len1 != 8 || len2 != 8) {
            LUA_PUSH_ERR("Internal error: bad Timestamp (Originate or Received) to build NTP packet");
        }
        oriTS = UINT64_FROM_BIG_ENDIAN(org);
        rcvTs = UINT64_FROM_BIG_ENDIAN(rec);
    }

    uint8_t ntppkg[48];
    memset(ntppkg, 0, 48);
    //static value for ntp "header" : 0 for everything except version = 4 mode = 3,
    ntppkg[0] = 35;

    UINT64_TO_BIG_ENDIAN_BUFF( oriTS, ntppkg + 24);
    UINT64_TO_BIG_ENDIAN_BUFF( rcvTs, ntppkg +32);

    //create Transmit timestamp the later possible
    if (internal_getntptime(&trTs)) {
        LUA_PUSH_ERR("Cannot get time to build ntp packet");
    }

    UINT64_TO_BIG_ENDIAN_BUFF( trTs, ntppkg + 40 );

    lua_pushlstring(L, (char*) ntppkg, 48);
    return 1;
}

//34 years limit in seconds
static int64_t maxTsDiff = 60 * 60 * 24 * (365.25) * 34;
// check if timestamp are not too much away from each other
static int TsDiff(uint64_t t1, uint64_t t2) {
    //only use sec part of NTP timestamps
    int64_t tmp = (t1 >> 32) - (t2 >> 32);
    if (tmp < 0)
        tmp = -tmp;
    return tmp >= maxTsDiff;
}

/* processpackets: check packets sanity, and compute offset and delay
 *
 * inputs:
 * pkt1: NTP packet sent
 * Usually pkt1 is the one returned by buildNTPpacket.
 *
 * pkt2: NTP packet received
 * Usually pkt2 is one received from the server.
 *
 * outputs:
 * - on success: status = 0, offset, delay, originate timestamp, reception timestamp
 *         (the 4 last outputs are (string) buffers of size 8)
 * - when time as be set to server time (34 years limit was run over): 1, maxTsDiff (fixed offset in seconds)
 * - sanity checks failed: status=2
 * - on error: nil+error string
 */
static int l_processpackets(lua_State *L) {
    size_t len1 = 0, len2 = 0;
    //get time first!

    //T4: Destination Timestamp: time reply received by client
    uint64_t T4;
    if (internal_getntptime(&T4)) {
        LUA_PUSH_ERR("Cannot get time to set time of reception");
    }
    //get params:
    //s1 is the first packet that should have been created using l_buildNTPpacket
    const char *pkt1 = luaL_checklstring(L, 1, &len1);
    //s2 is the NTP packet received.
    const char *pkt2 = luaL_checklstring(L, 2, &len2);

    //check NTP packets size
    if (len1 != 48 || len2 != 48) {
      LUA_PUSH_ERR("Invalid NTP packets: size is not 48");
    }

    //retrieve T1,T2,T3,T4 from NTP packets
    //T1: Originate Timestamp: time request sent by client
    const uint64_t T1 = UINT64_FROM_BIG_ENDIAN(pkt1+40);

    //T2: Receive Timestamp: time request received by server
    const uint64_t T2 = UINT64_FROM_BIG_ENDIAN(pkt2+32);
    //T3: Transmit Timestamp: time reply sent by server
    const uint64_t T3 = UINT64_FROM_BIG_ENDIAN(pkt2+40);

    // Let's do some packet sanity checks
    // rfc 4330,  5.  SNTP Client Operations -, .3 originate_ts in server packet should be equal to T1
    const uint64_t originate_ts = UINT64_FROM_BIG_ENDIAN(pkt2+24);
    if (originate_ts != T1) {
        SANITY_WARN("Originate timestamp in server NTP packet is not equal to T1, packet sanity check failed");
    }
    // rfc 4330,  5.  SNTP Client Operations -, .4 T3 diff from 0
    if (T3 == 0) {
        SANITY_WARN("T3 == 0 in server NTP packet, packet sanity check failed");
    }
    const uint8_t header = pkt2[0];
    // rfc 4330,  5.  SNTP Client Operations -, mode must 4 (as we send request with 3)
    if ((header & 0x07) != 0x04) {
        SANITY_WARN("mode!=4 in server NTP packet, packet sanity check failed");
    }
    // rfc 4330,  5.  SNTP Client Operations -, version must 4 (as we send request with 4)
    if ((header & 0x38) != 0x20) {
        SANITY_WARN("version!=4 in server NTP packet, packet sanity check failed");
    }
    // rfc 4330,  5.  SNTP Client Operations -, kiss o death message : stratum = 0
    if (pkt2[1] == 0) {
        SANITY_WARN("stratum == 0 in server NTP packet (kiss o death message), packet sanity check failed");
    }

    //check 4330, 3.  NTP Timestamp Format: "The arithmetic calculations used by NTP to determine the clock
    //offset and roundtrip delay require the client time to be within 34
    //years of the server time before the client is launched."
    if (TsDiff(T1, T2) || TsDiff(T3, T4)) {
        // if the 34 year limits is reach, settime!
        if (internal_setntptime(T3)) {
            LUA_PUSH_ERR("Cannot set system time! settimentp was trying to set device time directly to NTP server time (T3) due to 34 years interval");
        } else {
            lua_pushliteral(L, "1");
            lua_pushnumber(L, T3 > T4 ? maxTsDiff : -maxTsDiff);
            return 2;
        }
    }

    //now we can calculate the round trip delay(d)) and clock offset(t)

    //d = (T4 - T1) - (T3 - T2)
    int64_t d = (T4 - T1) - (T3 - T2);

    //t = ((T2 - T1) + (T3 - T4)) / 2.
    int64_t t = ((T2 - T1) + (T3 - T4));
    t >>= 1;

    //format data to return to Lua code.
    char offset[8];
    char delay[8];
    char org[8];
    char rec[8];
    memset(org, 0, 8);
    memset(rec, 0, 8);
    memset(delay, 0, 8);
    memset(offset, 0, 8);
    UINT64_TO_BIG_ENDIAN_BUFF( T3 , org);
    UINT64_TO_BIG_ENDIAN_BUFF( T4 , rec);
    UINT64_TO_BIG_ENDIAN_BUFF( t, offset);
    UINT64_TO_BIG_ENDIAN_BUFF( d, delay );

    lua_pushliteral(L, "0");
    lua_pushlstring(L, offset, 8);
    lua_pushlstring(L, delay, 8);
    lua_pushlstring(L, org, 8);
    lua_pushlstring(L, rec, 8);
    return 5;
}

/* getbestdelay: compare 2 delays and return the best one (i.e. the minimum in absolute value)
 *
 * inputs:
 * strdelay1: delay as a (string) buffer of size 8
 *
 * strdelay2: delay as a (string) buffer of size 8
 *
 * outputs:
 * - on success: best delay as a (string) buffer of size 8
 * - on error: nil+error string
 */
int l_getbestdelay(lua_State *L) {
    size_t len1 = 0, len2 = 0;
    const char *strdelay1 = luaL_checklstring(L, 1, &len1);
    const char *strdelay2 = luaL_checklstring(L, 2, &len2);
    if(len1 != 8 || len2 != 8){
        LUA_PUSH_ERR("Internal error: Bad params for l_getbestdelay");
    }

    int64_t delay1 = UINT64_FROM_BIG_ENDIAN(strdelay1);
    int64_t delay2 = UINT64_FROM_BIG_ENDIAN(strdelay2);
    if (delay1 < 0)
        delay1 = -delay1;
    if (delay2 < 0)
        delay2 = -delay2;

    if (delay1 < delay2)
        lua_pushlstring(L, strdelay1, 8);
    else
        lua_pushlstring(L, strdelay2, 8);

    return 1;
}

/* settime: set system time using an offset relative to current time
 *
 * inputs:
 * off: time offset as a (string) buffer of size 8
 * Time offset is usually returned by getbestdelay/processpackets functions
 *
 *
 * outputs:
 * - on success:"ok"
 * - on error: nil+error string
 */

int l_settime(lua_State *L) {
    size_t len1 = 0;
    const char *off = luaL_checklstring(L, 1, &len1);
    if(len1 != 8){
        LUA_PUSH_ERR("Internal error: Bad params for l_settime");
    }
    int64_t offset = UINT64_FROM_BIG_ENDIAN(off);
    uint64_t currenttime = 0;
    internal_getntptime(&currenttime);
    int64_t newtime = (int64_t) currenttime + offset;
    if (internal_setntptime((uint64_t) newtime)) {
        LUA_PUSH_ERR("Cannot set system time");
    } else {
        lua_pushliteral(L, "ok");
        lua_Number res = ((lua_Number) (offset >> 32));
        lua_Number div = ((lua_Number) (((int64_t) 1) << 32) );
        if(div)
            res+= ((lua_Number) (offset & 0xFFFFFFFF))    / div;
        lua_pushnumber(L, res);
        return 2;
    }
}

static const luaL_reg R[] = {
    { "buildntppacket", l_buildNTPpacket },
    { "processpackets", l_processpackets },
    { "settime", l_settime },
    { "getbestdelay", l_getbestdelay},
    { NULL,    NULL } };

int luaopen_ntp_core(lua_State *L) {
    luaL_register(L, "ntp.core", R);
    return 1;
}
