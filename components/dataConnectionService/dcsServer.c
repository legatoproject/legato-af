//--------------------------------------------------------------------------------------------------
/**
 *  Data Connection Server
 *
 *  Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 *
 * The Data Connection Service (DCS) only supports in this version the 'Mobile' technology, so the
 * data connection is based on the Modem Data Control service (MDC).
 *
 * When a REQUEST command is received, the DCS first sends a REQUEST command to the Cellular Network
 * Service in order to ensure that there is a valid SIM and the modem is registered on the network.
 * The Data session is actually started when the Cellular Network Service State is 'ROAMING' or
 * 'HOME'.
 *
 *
 * todo:
 *  - assumes that DHCP client will always succeed; this is not always the case
 *  - only handles the default data connection on mobile network
 *  - has a very simple recovery mechanism after data connection is lost; this needs improvement.
 */
//--------------------------------------------------------------------------------------------------

#include <arpa/inet.h>
#include <stdlib.h>

#include "legato.h"

#include "interfaces.h"
#include "le_cfg_interface.h"
#include "mdmCfgEntries.h"

#include "le_print.h"

#include "jansson.h"


//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * The config tree path and node definitions.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_PREF_TECH      "prefTech"
#define CFG_DCS_PATH            "/dataConnectionService"

//--------------------------------------------------------------------------------------------------
/**
 * The preferred technology string max length.
 */
//--------------------------------------------------------------------------------------------------
#define DCS_TECH_LEN      16
#define DCS_TECH_BYTES    (DCS_TECH_LEN+1)

//--------------------------------------------------------------------------------------------------
/**
 * The file to read for the APN.
 */
//--------------------------------------------------------------------------------------------------
#define APN_FILE "/opt/legato/apps/dataConnectionService/usr/local/share/apns.json"
// @TODO change the APN file when dataConnectionService become a sandboxed app.
//#define APN_FILE "/usr/local/share/apns.json"

//--------------------------------------------------------------------------------------------------
/**
 * The linux system file to read for default gateway.
 */
//--------------------------------------------------------------------------------------------------
#define ROUTE_FILE "/proc/net/route"

//--------------------------------------------------------------------------------------------------
/**
 * Definitions for sending request/release commands to data thread
 */
//--------------------------------------------------------------------------------------------------
#define REQUEST_COMMAND 1
#define RELEASE_COMMAND 2


//--------------------------------------------------------------------------------------------------
// Data structures.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Data associated with the above ConnStateEvent.
 *
 * interfaceName is only valid if isConnected is true.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    bool isConnected;
    char interfaceName[100+1];
}
ConnStateData_t;

//--------------------------------------------------------------------------------------------------
/**
 * Data associated to retreive the state before the DCS started managing the default connection.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char defaultGateway[LE_MDC_IPV6_ADDR_MAX_LEN];
    char defaultInterface[20];
    char newDnsIPv4[2][LE_MDC_IPV4_ADDR_MAX_LEN];
    char newDnsIPv6[2][LE_MDC_IPV6_ADDR_MAX_LEN];
}
InterfaceDataBackup_t;

//--------------------------------------------------------------------------------------------------
/**
 *  Event callback for data session state changes.
 */
//--------------------------------------------------------------------------------------------------
static void DataSessionStateHandler
(
    bool   isConnected,
    void*  contextPtr
);

//--------------------------------------------------------------------------------------------------
// Static declarations.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Timer references
 */
//--------------------------------------------------------------------------------------------------
static le_timer_Ref_t StartDcsTimer = NULL;
static le_timer_Ref_t StopDcsTimer = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Event for sending command to Process command handler
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t CommandEvent;

//--------------------------------------------------------------------------------------------------
/**
 * Event for sending connection to applications
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t ConnStateEvent;

//--------------------------------------------------------------------------------------------------
/**
 * Used profile when Mobile technology is selected
 */
//--------------------------------------------------------------------------------------------------
static le_mdc_ProfileRef_t MobileProfileRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Store the mobile session state handler ref
 */
//--------------------------------------------------------------------------------------------------
static le_mdc_SessionStateHandlerRef_t MobileSessionStateHandlerRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Is the data session connected
 */
//--------------------------------------------------------------------------------------------------
static bool IsConnected = false;

//--------------------------------------------------------------------------------------------------
/**
 * Count the number of requests
 */
//--------------------------------------------------------------------------------------------------
static uint32_t RequestCount = 0;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for the request reference
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t RequestRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Structure used to store data allowing to restore a functioning state upon disconnection.
 */
//--------------------------------------------------------------------------------------------------
static InterfaceDataBackup_t InterfaceDataBackup;

//--------------------------------------------------------------------------------------------------
/**
 * Buffer to store resolv.conf cache.
 */
//--------------------------------------------------------------------------------------------------
static char ResolvConfBuffer[256];

// -------------------------------------------------------------------------------------------------
/**
 *  This function will know if the APN name for profileName is empty.
 *
 *  @return true, false.
 */
// -------------------------------------------------------------------------------------------------
static bool IsApnEmpty
(
    le_mdc_ProfileRef_t profileRef
)
{
    char apnName[LIMIT_MAX_PATH_BYTES] = {0};

    if ( le_mdc_GetAPN(profileRef,apnName,sizeof(apnName)) != LE_OK)
    {
        LE_WARN("APN was truncated");
        return true;
    }

    return (strcmp(apnName,"")==0);
}

// -------------------------------------------------------------------------------------------------
/**
 *  This function will attempt to read apn definition for mcc/mnc in file apnFilePtr
 *
 * @return LE_OK        Function was able to find an APN
 * @return LE_NOT_FOUND Function was not able to find an APN for this (MCC,MNC)
 * @return LE_FAULT     There was an issue with the APN source
 */
// -------------------------------------------------------------------------------------------------
static le_result_t FindApnFromFile
(
    const char* apnFilePtr, ///< [IN] apn file
    const char* mccPtr,     ///< [IN] mcc
    const char* mncPtr,     ///< [IN] mnc
    char * mccMncApnPtr,    ///< [OUT] apn for mcc/mnc
    size_t mccMncApnSize    ///< [IN] size of mccMncApn buffer
)
{
    le_result_t result = LE_FAULT;
    json_t *root, *apns, *apnArray;
    json_error_t error;
    int i;

    root = json_load_file(apnFilePtr, 0, &error);
    if (root == NULL ) {
        LE_WARN("Document not parsed successfully. \n");
        return result;
    }

    apns = json_object_get( root, "apns" );
    if ( !json_is_object(apns) )
    {
        LE_WARN("apns is not an object\n");
        json_decref(root);
        return result;
    }

    apnArray = json_object_get( apns, "apn" );
    if ( !json_is_array(apnArray) )
    {
        LE_WARN("apns is not an array\n");
        json_decref(root);
        return result;
    }

    result = LE_NOT_FOUND;
    for( i = 0; i < json_array_size(apnArray); i++ )
    {
        json_t *data, *mcc, *mnc, *apn;
        const char* mccRead;
        const char* mncRead;
        const char* apnRead;
        data = json_array_get( apnArray, i );
        if ( !json_is_object( data ))
        {
            LE_WARN("data %d is not an object",i);
            result = LE_FAULT;
            break;
        }

        mcc = json_object_get( data, "@mcc" );
        mccRead = json_string_value(mcc);

        mnc = json_object_get( data, "@mnc" );
        mncRead = json_string_value(mnc);

        if ( !(strcmp(mccRead,mccPtr))
            &&
             !(strcmp(mncRead,mncPtr))
           )
        {
            apn = json_object_get( data, "@apn" );
            apnRead = json_string_value(apn);

            if ( le_utf8_Copy(mccMncApnPtr,apnRead,mccMncApnSize,NULL) != LE_OK)
            {
                LE_WARN("Apn buffer is too small");
                break;
            }
            LE_INFO("[%s:%s] Got APN '%s'", mccPtr, mncPtr, mccMncApnPtr);
            // @note Stop on the first json entry for mcc/mnc, need to be improved?
            result = LE_OK;
            break;
        }
    }

    json_decref(root);
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the APN for the profile by reading the APN file.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SetApnFromFile
(
    le_mdc_ProfileRef_t profileRef
)
{
    le_sim_ObjRef_t sim1Ref;
    char mccString[LE_MRC_MCC_BYTES]={0};
    char mncString[LE_MRC_MNC_BYTES]={0};
    char mccMncApn[100+1] = {0};

    // Hard coded, supposed we use the sim-1, need to be improved ?
    sim1Ref = le_sim_Create(1);
    if ( sim1Ref == NULL)
    {
        LE_WARN("Could not create the sim-1");
        return LE_FAULT;
    }

    // Get MCC/MNC
    if (le_sim_GetHomeNetworkMccMnc(mccString,
                                    LE_MRC_MCC_BYTES,
                                    mncString,
                                    LE_MRC_MNC_BYTES) != LE_OK)
    {
        LE_WARN("Could not find MCC/MNC");
        return LE_FAULT;
    }
    LE_DEBUG("Search of [%s:%s] into file %s",mccString,mncString,APN_FILE);

    // Find APN value for [MCC/MNC]
    if ( FindApnFromFile(APN_FILE,mccString,mncString,mccMncApn,sizeof(mccMncApn)) != LE_OK )
    {
        LE_WARN("Could not find %s/%s in %s",mccString,mncString,APN_FILE);
        return LE_FAULT;
    }

    // Save the APN value into the modem
    return le_mdc_SetAPN(profileRef,mccMncApn);
}

//--------------------------------------------------------------------------------------------------
/**
 * Load the profile of the selected technology (retrieved from the config tree)
 */
//--------------------------------------------------------------------------------------------------
static void LoadSelectedTechProfile
(
    const char* techPtr
)
{
    // TODO: only Cellular technology is supported and we try to load the 1st profile stored in MDC
    // database
    if (!strncmp(techPtr, "cellular", DCS_TECH_BYTES))
    {
        le_mdc_ProfileRef_t profileRef;

        LE_DEBUG("Use the default profile");
        profileRef = le_mdc_GetProfile(LE_MDC_DEFAULT_PROFILE);

        if (profileRef == NULL)
        {
            LE_FATAL("Default profile not available");
        }

        // Updating profileRef
        if (profileRef != MobileProfileRef)
        {
            if (MobileSessionStateHandlerRef != NULL)
            {
                le_mdc_RemoveSessionStateHandler(MobileSessionStateHandlerRef);
                MobileSessionStateHandlerRef = NULL;
            }

            MobileProfileRef = profileRef;

            uint32_t profileIndex;
            profileIndex = le_mdc_GetProfileIndex(MobileProfileRef);
            LE_DEBUG("Working with profile %p at index %d", MobileProfileRef, profileIndex);
        }

        // MobileProfileRef is now referencing the default profile to use for data connection
        if ( IsApnEmpty(MobileProfileRef) )
        {
            LE_INFO("Search for APN in the database");
            if ( SetApnFromFile(MobileProfileRef) != LE_OK )
            {
                LE_WARN("Could not set APN from file");
            }
        }
    }
    else
    {
        LE_FATAL("Only 'cellular' technology is supported");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Load preferences from the configuration tree
 */
//--------------------------------------------------------------------------------------------------
static void LoadPreferencesFromConfigDb
(
    void
)
{
    char configPath[LIMIT_MAX_PATH_BYTES];
    char techStr[DCS_TECH_BYTES] = {0};

    snprintf(configPath, sizeof(configPath), "%s/%s", CFG_DCS_PATH, CFG_NODE_PREF_TECH);

    LE_DEBUG("Start reading DCS information in ConfigDB");

    le_cfg_IteratorRef_t cfg = le_cfg_CreateReadTxn(configPath);

    if ( le_cfg_GetString(cfg, CFG_NODE_PREF_TECH, techStr, sizeof(techStr), "cellular") != LE_OK )
    {
        LE_WARN("String value for '%s' too large.", CFG_NODE_PREF_TECH);
    }

    le_cfg_CancelTxn(cfg);

    if (techStr[0] == '\0')
    {
        LE_WARN("No node value set for '%s'", CFG_NODE_PREF_TECH);
        LE_ASSERT( LE_OK != le_utf8_Copy(techStr, "cellular", sizeof(techStr), NULL) );
    }

    LE_DEBUG("'%s' is the preferred technology for data connection.", techStr);

    LoadSelectedTechProfile(techStr);

    LE_ASSERT( MobileProfileRef != NULL );
}


//--------------------------------------------------------------------------------------------------
/**
 * Check if a default gateway is set
 *
 * return
 *      True or False
 */
//--------------------------------------------------------------------------------------------------
static bool IsDefaultGatewayPresent
(
    void
)
{
    bool result = false;
    le_result_t openResult;
    FILE *routeFile;
    char line[100] , *ifacePtr , *destPtr, *gwPtr, *saveptr;

    routeFile = le_flock_OpenStream(ROUTE_FILE , LE_FLOCK_READ, &openResult);

    if (routeFile == NULL)
    {
        LE_WARN("le_flock_OpenStream failed with error %d", openResult);
        return result;
    }

    while(fgets(line , sizeof(line) , routeFile))
    {
        ifacePtr = strtok_r(line , " \t", &saveptr);
        destPtr  = strtok_r(NULL , " \t", &saveptr);
        gwPtr    = strtok_r(NULL , " \t", &saveptr);

        if(ifacePtr!=NULL && destPtr!=NULL)
        {
            if(strcmp(destPtr , "00000000") == 0)
            {
                if (gwPtr)
                {
                    result = true;
                    break;
                }
            }
        }
    }

    le_flock_CloseStream(routeFile);
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Save the default route.
 *
 * return
 *      LE_OK           Function succeed
 *      LE_OVERFLOW     buffer provided are too small
 *      LE_NOT_FOUND    No default gateway is set.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SaveDefaultGateway
(
    char    *interfacePtr,
    size_t   interfaceSize,
    char    *gatewayPtr,
    size_t   gatewaySize
)
{
    le_result_t result;
    FILE *routeFile;
    char line[100] , *ifacePtr , *destPtr, *gwPtr, *saveptr;

    routeFile = le_flock_OpenStream(ROUTE_FILE , LE_FLOCK_READ, &result);

    if (routeFile == NULL)
    {
        return result;
    }

    // Initialize default value
    interfacePtr[0] = '\0';
    gatewayPtr[0] = '\0';

    result = LE_NOT_FOUND;
    while(fgets(line , sizeof(line) , routeFile))
    {
        ifacePtr = strtok_r(line , " \t", &saveptr);
        destPtr  = strtok_r(NULL , " \t", &saveptr);
        gwPtr    = strtok_r(NULL , " \t", &saveptr);

        if(ifacePtr!=NULL && destPtr!=NULL)
        {
            if(strcmp(destPtr , "00000000") == 0)
            {
                if (gwPtr)
                {
                    char *pEnd;
                    uint32_t ng=strtoul(gwPtr,&pEnd,16);
                    struct in_addr addr;
                    addr.s_addr=ng;

                    result = le_utf8_Copy(interfacePtr,ifacePtr,interfaceSize,NULL);
                    if (result != LE_OK)
                    {
                        LE_WARN("inferface buffer is too small");
                        break;
                    }

                    result = le_utf8_Copy(gatewayPtr,inet_ntoa(addr),gatewaySize,NULL);
                    if (result != LE_OK)
                    {
                        LE_WARN("gateway buffer is too small");
                        break;
                    }
                }
                break;
            }
        }
    }

    le_flock_CloseStream(routeFile);
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the default gateway in the system.
 *
 * return
 *      LE_OK           Function succeed
 *      LE_FAULT        Function failed
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SetDefaultGateway
(
    const char *interfacePtr,
    const char *gatewayPtr,
    bool isIpv6
)
{
    const char *optionPtr = "";
    char systemCmd[200] = {0};

    LE_DEBUG("Try set the gateway %s on %s", gatewayPtr, interfacePtr);

    if ( (strcmp(gatewayPtr,"")==0) || (strcmp(interfacePtr,"")==0) )
    {
        LE_WARN("Gateway or Interface are empty");
        return LE_FAULT;
    }

    if (IsDefaultGatewayPresent())
    {
        // Remove the last default GW.
        LE_DEBUG("Execute '/sbin/route del default'");
        if ( system("/sbin/route del default") == -1 )
        {
            LE_WARN("system '%s' failed", systemCmd);
            return LE_FAULT;
        }
    }

    if ( isIpv6 )
    {
        optionPtr = "-A inet6";
    }

    // @TODO use of ioctl instead, should be done when rework the DCS.
    snprintf(systemCmd, sizeof(systemCmd),
             "/sbin/route %s add default gw %s %s", optionPtr, gatewayPtr, interfacePtr);

    LE_DEBUG("Execute '%s", systemCmd);
    if ( system(systemCmd) == -1 )
    {
        LE_WARN("system '%s' failed", systemCmd);
        return LE_FAULT;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the default route for a profile
 *
 * return
 *      LE_FAULT        Function failed
 *      LE_OK           Function succeed
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SetRouteConfiguration
(
    le_mdc_ProfileRef_t profileRef
)
{
    bool isIpv6 = false;
    char gatewayAddr[100] = {0};
    char interface[100] = {0};
    le_result_t (*getGatewayFunction)(le_mdc_ProfileRef_t,char*,size_t) = NULL;

    if ( le_mdc_IsIPv6(profileRef) ) {
        isIpv6 = true;
        getGatewayFunction = le_mdc_GetIPv6GatewayAddress;
    }
    else if ( le_mdc_IsIPv4(profileRef) )
    {
        isIpv6 = false;
        getGatewayFunction = le_mdc_GetIPv4GatewayAddress;
    }
    else
    {
        LE_WARN("Profile is not using IPv4 nor IPv6");
        return LE_FAULT;
    }

    if ( getGatewayFunction &&
         (*getGatewayFunction)(profileRef, gatewayAddr, sizeof(gatewayAddr)) != LE_OK )
    {
        LE_INFO("le_mdc_GetGatewayAddress failed");
        return LE_FAULT;
    }

    if ( le_mdc_GetInterfaceName(profileRef, interface, sizeof(interface)) != LE_OK )
    {
        LE_WARN("le_mdc_GetInterfaceName failed");
        return LE_FAULT;
    }

    // Set the default gateway retrieved from modem.
    return SetDefaultGateway(interface,gatewayAddr, isIpv6);
}

//--------------------------------------------------------------------------------------------------
/**
 * Read DNS configuration from /etc/resolv.conf
 *
 * @return File content in a statically allocated string (shouldn't be freed)
 */
//--------------------------------------------------------------------------------------------------
static char * ReadResolvConf
(
    void
)
{
    int fd;
    char * fileContent;
    size_t fileSz;

    fd = open("/etc/resolv.conf", O_RDONLY);
    if (fd < 0)
    {
        LE_WARN("fopen on /etc/resolv.conf failed");
        return NULL;
    }

    fileSz = lseek(fd, 0, SEEK_END);
    LE_FATAL_IF( (fileSz < 0), "Unable to get resolv.conf size" );

    if (fileSz == 0)
    {
        return NULL;
    }

    LE_DEBUG("Caching resolv.conf: size[%zu]", fileSz);

    lseek(fd, 0, SEEK_SET);

    if (fileSz > (sizeof(ResolvConfBuffer)-1))
    {
        LE_ERROR("Buffer is too small (%zu), file will be truncated from %zu",
                sizeof(ResolvConfBuffer), fileSz);
        fileSz = sizeof(ResolvConfBuffer) - 1;
    }

    fileContent = ResolvConfBuffer;

    if ( 0 > read(fd, fileContent, fileSz) )
    {
        LE_ERROR("Caching resolv.conf failed");
        fileContent[0] = '\0';
        fileSz = 0;
    }
    else
    {
        fileContent[fileSz] = '\0';
    }

    if ( close(fd) != 0 )
    {
        LE_FATAL("close failed");
    }

    LE_FATAL_IF( strlen(fileContent) > fileSz,
                    "Content size (%zu) and File size (%zu) differ",
                    strlen(fileContent), fileSz );

    return fileContent;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write the DNS configuration into /etc/resolv.conf
 *
 * @return
 *      LE_FAULT        Function failed
 *      LE_OK           Function succeed
 */
//--------------------------------------------------------------------------------------------------
static le_result_t AddNameserversToResolvConf
(
    const char *dns1Ptr,
    const char *dns2Ptr
)
{
    bool addDns1 = true;
    bool addDns2 = true;

    LE_INFO("Set DNS '%s' '%s'", dns1Ptr, dns2Ptr);

    addDns1 = ('\0' != dns1Ptr[0]);
    addDns2 = ('\0' != dns2Ptr[0]);

    // Look for entries to add in the existing file
    char* resolvConfSourcePtr = ReadResolvConf();

    if (resolvConfSourcePtr != NULL)
    {
        char* currentLinePtr = resolvConfSourcePtr;
        int currentLinePos = 0;

        // For each line in source file
        while (true)
        {
            if ( ('\0' == currentLinePtr[currentLinePos]) ||
                 ('\n' == currentLinePtr[currentLinePos]) )
            {
                char sourceLineEnd = currentLinePtr[currentLinePos];
                currentLinePtr[currentLinePos] = '\0';

                if (NULL != strstr(currentLinePtr, dns1Ptr))
                {
                    LE_DEBUG("DNS 1 '%s' found in file", dns1Ptr);
                    addDns1 = false;
                }
                else if (NULL != strstr(currentLinePtr, dns2Ptr))
                {
                    LE_DEBUG("DNS 2 '%s' found in file", dns2Ptr);
                    addDns2 = false;
                }

                if (sourceLineEnd == '\0')
                {
                    break;
                }
                else
                {
                    currentLinePtr[currentLinePos] = sourceLineEnd;
                    currentLinePtr += (currentLinePos+1); // Next line
                    currentLinePos = 0;
                }
            }
            else
            {
                currentLinePos++;
            }
        }
    }

    if ( !addDns1 && !addDns2 )
    {
        // No need to change the file
        return LE_OK;
    }

    FILE* resolvConfPtr;

    resolvConfPtr = fopen("/etc/resolv.conf", "w");
    if (resolvConfPtr == NULL)
    {
        LE_WARN("fopen on /etc/resolv.conf failed");
        return LE_FAULT;
    }

    // Set DNS 1 if needed
    if ( addDns1 && (fprintf(resolvConfPtr, "nameserver %s\n", dns1Ptr) < 0) )
    {
        LE_WARN("fprintf failed");
        if ( fclose(resolvConfPtr) != 0 )
        {
            LE_WARN("fclose failed");
        }
        return LE_FAULT;
    }

    // Set DNS 2 if needed
    if ( addDns2 && (fprintf(resolvConfPtr, "nameserver %s\n", dns2Ptr) < 0) )
    {
        LE_WARN("fprintf failed");
        if ( fclose(resolvConfPtr) != 0 )
        {
            LE_WARN("fclose failed");
        }
        return LE_FAULT;
    }

    // Append rest of the file
    if (resolvConfSourcePtr != NULL)
    {
        if ( 0 > fwrite(resolvConfSourcePtr, sizeof(char), strlen(resolvConfSourcePtr), resolvConfPtr) )
        {
            LE_FATAL("Writing resolv.conf failed");
        }
    }

    if ( fclose(resolvConfPtr) != 0 )
    {
        LE_WARN("fclose failed");
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write the DNS configuration into /etc/resolv.conf
 *
 * @return
 *      LE_FAULT        Function failed
 *      LE_OK           Function succeed
 */
//--------------------------------------------------------------------------------------------------
static le_result_t RemoveNameserversFromResolvConf
(
    const char *dns1Ptr,
    const char *dns2Ptr
)
{
    char* resolvConfSourcePtr = ReadResolvConf();
    char* currentLinePtr = resolvConfSourcePtr;
    int currentLinePos = 0;

    FILE* resolvConfPtr;

    if (resolvConfSourcePtr == NULL)
    {
        // Nothing to remove
        return LE_OK;
    }

    resolvConfPtr = fopen("/etc/resolv.conf", "w");
    if (resolvConfPtr == NULL)
    {
        LE_WARN("fopen on /etc/resolv.conf failed");
        return LE_FAULT;
    }

    // For each line in source file
    while (true)
    {
        if ( ('\0' == currentLinePtr[currentLinePos]) ||
             ('\n' == currentLinePtr[currentLinePos]) )
        {
            char sourceLineEnd = currentLinePtr[currentLinePos];
            currentLinePtr[currentLinePos] = '\0';

            // Got to the end of the source file
            if ( (sourceLineEnd == '\0') && (currentLinePos == 0) )
            {
                break;
            }

            // If line doesn't contains an entry to remove,
            // copy line to new content
            if ( (NULL == strstr(currentLinePtr, dns1Ptr)) &&
                 (NULL == strstr(currentLinePtr, dns2Ptr)) )
            {
                // The original file contents may not have the final line terminated by
                // a new-line; always terminate with a new-line, since this is what is
                // usually expected on linux.
                currentLinePtr[currentLinePos] = '\n';
                fwrite(currentLinePtr, sizeof(char), (currentLinePos+1), resolvConfPtr);
            }

            if (sourceLineEnd == '\0')
            {
                // This should only occur if the last line was not terminated by a new-line.
                break;
            }
            else
            {
                currentLinePtr += (currentLinePos+1); // Next line
                currentLinePos = 0;
            }
        }
        else
        {
            currentLinePos++;
        }
    }

    if ( fclose(resolvConfPtr) != 0 )
    {
        LE_WARN("fclose failed");
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the DNS configuration for a profile
 *
 * return
 *      LE_FAULT        Function failed
 *      LE_OK           Function succeed
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SetDnsConfiguration
(
    le_mdc_ProfileRef_t profileRef
)
{
    char dns1Addr[LE_MDC_IPV6_ADDR_MAX_LEN] = {0};
    char dns2Addr[LE_MDC_IPV6_ADDR_MAX_LEN] = {0};

    if ( le_mdc_IsIPv4(profileRef) )
    {
        if ( le_mdc_GetIPv4DNSAddresses(profileRef,
                                        dns1Addr, sizeof(dns1Addr),
                                        dns2Addr, sizeof(dns2Addr)) != LE_OK )
        {
            LE_INFO("IPv4: le_mdc_GetDNSAddresses failed");
            return LE_FAULT;
        }

        if ( AddNameserversToResolvConf(dns1Addr, dns2Addr) != LE_OK )
        {
            LE_INFO("IPv4: Could not write in resolv file");
            return LE_FAULT;
        }

        strcpy(InterfaceDataBackup.newDnsIPv4[0], dns1Addr);
        strcpy(InterfaceDataBackup.newDnsIPv4[1], dns2Addr);
    }
    else
    {
        InterfaceDataBackup.newDnsIPv4[0][0] = '\0';
        InterfaceDataBackup.newDnsIPv4[1][0] = '\0';
    }

    if ( le_mdc_IsIPv6(profileRef) )
    {
        if ( le_mdc_GetIPv6DNSAddresses(profileRef,
                                        dns1Addr, sizeof(dns1Addr),
                                        dns2Addr, sizeof(dns2Addr)) != LE_OK )
        {
            LE_INFO("IPv6: le_mdc_GetDNSAddresses failed");
            return LE_FAULT;
        }

        if ( AddNameserversToResolvConf(dns1Addr, dns2Addr) != LE_OK )
        {
            LE_INFO("IPv6: Could not write in resolv file");
            return LE_FAULT;
        }

        strcpy(InterfaceDataBackup.newDnsIPv6[0], dns1Addr);
        strcpy(InterfaceDataBackup.newDnsIPv6[1], dns2Addr);
    }
    else
    {
        InterfaceDataBackup.newDnsIPv6[0][0] = '\0';
        InterfaceDataBackup.newDnsIPv6[1][0] = '\0';
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Try to start the data session
 */
//--------------------------------------------------------------------------------------------------
static void TryStartDataSession
(
    le_timer_Ref_t timerRef
)
{
    le_result_t          result;

    // Reload MobileProfileRef
    LoadPreferencesFromConfigDb();

    // Register for data session state changes
    // if there was an update of profileRef
    if (MobileSessionStateHandlerRef == NULL)
    {
        MobileSessionStateHandlerRef = le_mdc_AddSessionStateHandler(MobileProfileRef, DataSessionStateHandler, &MobileProfileRef);
    }

    result = le_mdc_StartSession(MobileProfileRef);
    if (result != LE_OK)
    {
        if (!le_timer_IsRunning(timerRef))
        {
            if (le_timer_Start(timerRef) != LE_OK)
            {
                LE_ERROR("Could not start the StartDcs timer!");
                return;
            }
        }
    }
    else
    {
        // First wait a few seconds for the default DHCP client.
        sleep(3);

        // Save gateway configuration.
        if ( SaveDefaultGateway(InterfaceDataBackup.defaultInterface, sizeof(InterfaceDataBackup.defaultInterface),
                                InterfaceDataBackup.defaultGateway, sizeof(InterfaceDataBackup.defaultGateway)) != LE_OK)
        {
            LE_WARN("Could not save the default gateway");
        }
        LE_DEBUG("default gw is: '%s' on '%s'",InterfaceDataBackup.defaultGateway,InterfaceDataBackup.defaultInterface);

        if (SetRouteConfiguration(MobileProfileRef) != LE_OK)
        {
            LE_ERROR("Failed to get configuration route.");
            if (!le_timer_IsRunning(timerRef))
            {
                if (le_timer_Start(timerRef) != LE_OK)
                {
                    LE_ERROR("Could not start the StartDcs timer!");
                    return;
                }
            }
        }

        if (SetDnsConfiguration(MobileProfileRef) != LE_OK)
        {
            LE_ERROR("Failed to get configuration DNS.");
            if (!le_timer_IsRunning(timerRef))
            {
                if (le_timer_Start(timerRef) != LE_OK)
                {
                    LE_ERROR("Could not start the StartDcs timer!");
                    return;
                }
            }
        }

        // Wait a few seconds to prevent rapid toggling of data connection
        sleep(5);

        IsConnected = true;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Start Data Connection Service Timer Handler.
 * When the timer expires, verify if the session is connected, if NOT retry to connect it and rearm
 * the timer.
 */
//--------------------------------------------------------------------------------------------------
static void StartDcsTimerHandler
(
    le_timer_Ref_t timerRef
)
{
    bool        sessionState;
    le_result_t result;

    if(RequestCount == 0)
    {
        // Release has been requested in the meantime, I must cancel the Request command process
        le_timer_Stop(timerRef);
    }
    else
    {
        result = le_mdc_GetSessionState(MobileProfileRef, &sessionState);
        if ((result == LE_OK) && (sessionState))
        {
            if (SetRouteConfiguration(MobileProfileRef) != LE_OK)
            {
                LE_ERROR("Failed to get configuration route.");
                TryStartDataSession(timerRef);
                return;
            }

            if (SetDnsConfiguration(MobileProfileRef) != LE_OK)
            {
                LE_ERROR("Failed to get configuration DNS.");
                TryStartDataSession(timerRef);
                return;
            }

            // The radio is ON, stop and delete the Timer.
            le_timer_Stop(timerRef);

            // Wait a few seconds to prevent rapid toggling of data connection
            sleep(5);

            IsConnected = true;
        }
        else
        {
            TryStartDataSession(timerRef);
            // TODO: find a solution to get off of this infinite loop
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Used the data backup upon connection to remove DNS entries locally added.
 */
//--------------------------------------------------------------------------------------------------
static void RestoreInitialNameservers
(
    void
)
{
    if ( ('\0' != InterfaceDataBackup.newDnsIPv4[0][0]) ||
         ('\0' != InterfaceDataBackup.newDnsIPv4[1][0]) )
    {
        RemoveNameserversFromResolvConf(InterfaceDataBackup.newDnsIPv4[0],
                                        InterfaceDataBackup.newDnsIPv4[1]);
    }

    if ( ('\0' != InterfaceDataBackup.newDnsIPv6[0][0]) ||
         ('\0' != InterfaceDataBackup.newDnsIPv6[1][0]) )
    {
        RemoveNameserversFromResolvConf(InterfaceDataBackup.newDnsIPv6[0],
                                        InterfaceDataBackup.newDnsIPv6[1]);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Try to stop the data session
 */
//--------------------------------------------------------------------------------------------------
static void TryStopDataSession
(
    le_timer_Ref_t timerRef
)
{
    bool        sessionState;
    le_result_t result;

    result = le_mdc_GetSessionState(MobileProfileRef, &sessionState);
    if ((result == LE_OK) && (!sessionState))
    {
        IsConnected = false;
        RestoreInitialNameservers();
        return;
    }
    else
    {
        // Try to shutdown the connection anyway
        result = le_mdc_StopSession(MobileProfileRef);
        if (result != LE_OK)
        {
            if (!le_timer_IsRunning(timerRef))
            {
                if (le_timer_Start(timerRef) != LE_OK)
                {
                    LE_ERROR("Could not start the StopDcs timer!");
                }
            }
        }
        else
        {
            IsConnected = false;

            SetDefaultGateway(InterfaceDataBackup.defaultInterface, InterfaceDataBackup.defaultGateway, false);
            RestoreInitialNameservers();
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Stop Data Connection Service Timer Handler.
 * When the timer expires, verify if the session is disconnected, if NOT retry to disconnect it and
 * rearm the timer.
 */
//--------------------------------------------------------------------------------------------------
static void StopDcsTimerHandler
(
    le_timer_Ref_t timerRef
)
{
    bool        sessionState;
    le_result_t result;

    if(RequestCount != 0)
    {
        // Request has been requested in the meantime, I must cancel the Release command process
        le_timer_Stop(timerRef);
    }
    else
    {
        result = le_mdc_GetSessionState(MobileProfileRef, &sessionState);
        if ((result == LE_OK) && (!sessionState))
        {
            IsConnected = false;
            // The data session is disconnected, stop and delete the Timer.
            le_timer_Stop(timerRef);
        }
        else
        {
            TryStopDataSession(timerRef);
            // TODO: find a solution to get off of this infinite loop
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Send connection state event
 */
//--------------------------------------------------------------------------------------------------
static void SendConnStateEvent
(
    bool isConnected
)
{
    // Init the event data
    ConnStateData_t eventData;
    eventData.isConnected = isConnected;
    if (isConnected)
    {
        le_mdc_GetInterfaceName(MobileProfileRef,
                                eventData.interfaceName,
                                sizeof(eventData.interfaceName));
    }
    else
    {
        // init to empty string
        eventData.interfaceName[0] = '\0';
    }

    LE_DEBUG("Reporting '%s' state[%i]",
        eventData.interfaceName,
        eventData.isConnected);

    // Send the event to interested applications
    le_event_Report(ConnStateEvent, &eventData, sizeof(eventData));
}


//--------------------------------------------------------------------------------------------------
/**
 * Handler to process a command
 */
//--------------------------------------------------------------------------------------------------
static void ProcessCommand
(
    void* reportPtr
)
{
    uint32_t command = *(uint32_t*)reportPtr;

    LE_PRINT_VALUE("%i", command);

    if (command == REQUEST_COMMAND)
    {
        RequestCount++;
        if (!IsConnected)
        {
            // Ensure that cellular network service is available
            le_cellnet_Request();
        }
        else
        {
            // There is already a data session, so send a fake event so that the new application
            // that just sent the command knows about the current state.  This will also cause
            // redundant info to be sent to the other registered apps, but that's okay.
            SendConnStateEvent(true);
        }
    }
    else if (command == RELEASE_COMMAND)
    {
        // Don't decrement below zero, as it will wrap-around.
        if (RequestCount > 0)
        {
            RequestCount--;
        }

        if ((RequestCount == 0) && IsConnected)
        {
            TryStopDataSession(StopDcsTimer);
        }
    }
    else
    {
        LE_ERROR("Command %i is not valid", command);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 *  Event callback for data session state changes.
 */
//--------------------------------------------------------------------------------------------------
static void DataSessionStateHandler
(
    bool   isConnected,
    void*  contextPtr
)
{
    le_mdc_ProfileRef_t profileRef = (*(le_mdc_ProfileRef_t*)contextPtr);

    uint32_t profileIndex = le_mdc_GetProfileIndex(profileRef);

    LE_PRINT_VALUE("%d", profileIndex);
    LE_PRINT_VALUE("%i", isConnected);

    // Update global state variable
    IsConnected = isConnected;

    // Send the state event to applications
    SendConnStateEvent(isConnected);

    // Restart data connection, if it has gone down, and there are still valid requests
    // todo: this mechanism needs to be much better
    if ( ( RequestCount>0 ) && ( !isConnected ) )
    {
        // Give the modem some time to recover from whatever caused the loss of the data
        // connection, before trying to recover.
        sleep(30);

        // Try to restart
        TryStartDataSession(StartDcsTimer);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 *  Event callback for Cellular Network Service state changes.
 */
//--------------------------------------------------------------------------------------------------
static void CellNetStateHandler
(
    le_cellnet_State_t state,
    void*              contextPtr
)
{
    LE_DEBUG("Cellular Network Service is in state.%d", state);
    switch (state)
    {
        case LE_CELLNET_RADIO_OFF:
        case LE_CELLNET_REG_EMERGENCY:
        case LE_CELLNET_REG_UNKNOWN:
            break;

        case LE_CELLNET_REG_HOME:
        case LE_CELLNET_REG_ROAMING:
            if ((RequestCount > 0) && (!IsConnected))
            {
                TryStartDataSession(StartDcsTimer);
            }
            break;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * This thread does the actual work of starting/stopping a data connection
 *
 * @NOTE: For now, only one MobileProfile is manage, need to improve it for all profiles.
 */
//--------------------------------------------------------------------------------------------------
static void* DataThread
(
    void* contextPtr
)
{
    // Connect to the services required by this thread
    le_cellnet_ConnectService();
    le_cfg_ConnectService();
    le_mdc_ConnectService();
    le_mrc_ConnectService();
    le_sim_ConnectService();

    LE_INFO("Data Thread Started");

    // Register for command events
    le_event_AddHandler("ProcessCommand",
                        CommandEvent,
                        ProcessCommand);

    // Register for Cellular Network Service state changes
    le_cellnet_AddStateEventHandler(CellNetStateHandler, NULL);

    // Register for data session state changes
    MobileSessionStateHandlerRef = le_mdc_AddSessionStateHandler(MobileProfileRef, DataSessionStateHandler, &MobileProfileRef);

    // Run the event loop
    le_event_RunLoop();
    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * The first-layer Connection State Handler
 *
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerConnectionStateHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    ConnStateData_t* eventDataPtr = reportPtr;
    le_data_ConnectionStateHandlerFunc_t clientHandlerFunc = secondLayerHandlerFunc;

    clientHandlerFunc(eventDataPtr->interfaceName,
                      eventDataPtr->isConnected,
                      le_event_GetContextPtr());
}

//--------------------------------------------------------------------------------------------------
/**
 * handler function to the close session service
 *
 */
//--------------------------------------------------------------------------------------------------
static void CloseSessionEventHandler
(
    le_msg_SessionRef_t sessionRef,
    void*               contextPtr
)
{
    LE_INFO("Client %p killed, remove allocated ressources", sessionRef);

    if ( !sessionRef )
    {
        LE_ERROR("ERROR sessionRef is NULL");
        return;
    }

    // Search the data reference used by the killed client.
    le_ref_IterRef_t iterRef = le_ref_GetIterator(RequestRefMap);
    le_result_t result = le_ref_NextNode(iterRef);

    while ( result == LE_OK )
    {
        le_msg_SessionRef_t session = (le_msg_SessionRef_t) le_ref_GetValue(iterRef);

        // Check if the session reference saved matchs with the current session reference.
        if (session == sessionRef)
        {
            // Release the data connexion
            le_data_Release( (le_data_RequestObjRef_t) le_ref_GetSafeRef(iterRef) );
        }

        // Get the next value in the reference mpa
        result = le_ref_NextNode(iterRef);
    }
}

//--------------------------------------------------------------------------------------------------
// APIs.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * This function adds a handler ...
 */
//--------------------------------------------------------------------------------------------------
le_data_ConnectionStateHandlerRef_t le_data_AddConnectionStateHandler
(
    le_data_ConnectionStateHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    LE_PRINT_VALUE("%p", handlerPtr);
    LE_PRINT_VALUE("%p", contextPtr);

    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler(
                                                    "DataConnState",
                                                    ConnStateEvent,
                                                    FirstLayerConnectionStateHandler,
                                                    (le_event_HandlerFunc_t)handlerPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (le_data_ConnectionStateHandlerRef_t)(handlerRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * This function removes a handler ...
 */
//--------------------------------------------------------------------------------------------------
void le_data_RemoveConnectionStateHandler
(
    le_data_ConnectionStateHandlerRef_t addHandlerRef
)
{
    LE_PRINT_VALUE("%p", addHandlerRef);

    le_event_RemoveHandler((le_event_HandlerRef_t)addHandlerRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Request the default data connection
 *
 * @return
 *      - A reference to the data connection (to be used later for releasing the connection)
 *      - NULL if the data connection requested could not be processed
 */
//--------------------------------------------------------------------------------------------------
le_data_RequestObjRef_t le_data_Request
(
    void
)
{
    uint32_t command = REQUEST_COMMAND;
    le_event_Report(CommandEvent, &command, sizeof(command));

    // Need to return a unique reference that will be used by Release.  Don't actually have
    // any data for now, but have to use some value other than NULL for the data pointer.
    le_data_RequestObjRef_t reqRef = le_ref_CreateRef(RequestRefMap,
                                                     (void*)le_data_GetClientSessionRef());

    return reqRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Release a previously requested data connection
 */
//--------------------------------------------------------------------------------------------------
void le_data_Release
(
    le_data_RequestObjRef_t requestRef
    ///< Reference to a previously requested data connection
)
{
    // Look up the reference.  If it is NULL, then the reference is not valid.
    // Otherwise, delete the reference and send the release command to the data thread.
    void* dataPtr = le_ref_Lookup(RequestRefMap, requestRef);
    if ( dataPtr == NULL )
    {
        LE_ERROR("Invalid data request reference %p", requestRef);
    }
    else
    {
        LE_PRINT_VALUE("%p", requestRef);
        le_ref_DeleteRef(RequestRefMap, requestRef);

        uint32_t command = RELEASE_COMMAND;
        le_event_Report(CommandEvent, &command, sizeof(command));
    }
}


//--------------------------------------------------------------------------------------------------
/**
 *  Server Init
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // Init the various events
    CommandEvent = le_event_CreateId("Data Command", sizeof(uint32_t));
    ConnStateEvent = le_event_CreateId("Conn State", sizeof(ConnStateData_t));

    // Create safe reference map for request references. The size of the map should be based on
    // the expected number of simultaneous data requests, so take a reasonable guess.
    RequestRefMap = le_ref_CreateMap("Requests", 5);

    // Load the preferences from the configuration tree
    LoadPreferencesFromConfigDb();

    // Set a timer to retry the start data session.
    StartDcsTimer = le_timer_Create("StartDcsTimer");
    le_clk_Time_t interval = {15, 0}; // 15 seconds

    if ( (le_timer_SetHandler(StartDcsTimer, StartDcsTimerHandler) != LE_OK) ||
         (le_timer_SetRepeat(StartDcsTimer, 0) != LE_OK) ||
         (le_timer_SetInterval(StartDcsTimer, interval) != LE_OK) )
    {
        LE_ERROR("Could not start the StartDcs timer!");
    }

    // Set a timer to retry the stop data session.
    StopDcsTimer = le_timer_Create("StopDcsTimer");
    interval.sec = 5; // 5 seconds

    if ( (le_timer_SetHandler(StopDcsTimer, StopDcsTimerHandler) != LE_OK) ||
         (le_timer_SetRepeat(StopDcsTimer, 0) != LE_OK) ||
         (le_timer_SetInterval(StopDcsTimer, interval) != LE_OK) )
    {
        LE_ERROR("Could not start the StopDcs timer!");
    }

    // Add a handler to the close session service
    le_msg_AddServiceCloseHandler( le_data_GetServiceRef(),
                                   CloseSessionEventHandler,
                                   NULL );

    // Start the data thread
    le_thread_Start( le_thread_Create("Data Thread", DataThread, NULL) );

    LE_INFO("Data Connection Server is ready");
}
