//--------------------------------------------------------------------------------------------------
/**
 *  Data Channel Server's C code implementation of its southbound interfaces with the Ethernet
 *  component.
 *
 *  Copyright (C) Sierra Wireless Inc.
 *
 */
//--------------------------------------------------------------------------------------------------

#include <stdlib.h>
#include <string.h>
#include "legato.h"
#include "interfaces.h"
#include "le_print.h"
#include "le_cfg_interface.h"
#include "dcs.h"
#include "pa_dcs.h"
#include "dcsEthernet.h"
#include "pa_ethernet.h"

//--------------------------------------------------------------------------------------------------
/**
 * Max number of Ethernet connections allowed.
 */
//--------------------------------------------------------------------------------------------------
#define ETHERNET_CONNDBS_MAX 8

//--------------------------------------------------------------------------------------------------
/**
 * Pool for Ethernet connection data base.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t EthernetConnDbPool;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for Ethernet connection database objects
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t EthernetConnectionRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Flag to allow Ethernet channel query
 */
//--------------------------------------------------------------------------------------------------
static bool AllowEthernetChannelQuery = true;

//--------------------------------------------------------------------------------------------------
/**
 * Search for the given Ethernet connection reference's connDb from its reference map
 *
 * @return
 *     - The found connDb will be returned in the function's return value; otherwise, NULL
 */
//--------------------------------------------------------------------------------------------------
static ethernet_connDb_t *DcsEthernetGetDbFromRef
(
    le_dcs_EthernetConnectionRef_t ethernetConnRef
)
{
    return (ethernet_connDb_t *)le_ref_Lookup(EthernetConnectionRefMap, ethernetConnRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Utility for retrieving the ethernetConnDb of the given Ethernet interface name in the argument
 *
 * @return
 *     - The ethernetConnDb data structure of the given Ethernet interface name in the function's
 *       return value upon success; otherwise NULL
 */
//--------------------------------------------------------------------------------------------------
static ethernet_connDb_t *DcsEthernetGetDbFromNetInterface
(
    const char* netInterface
)
{
    le_ref_IterRef_t iterRef = le_ref_GetIterator(EthernetConnectionRefMap);
    ethernet_connDb_t *ethernetConnDb;

    if (NULL == netInterface)
    {
        LE_ERROR("Cannot get Ethernet connection db with NULL interface");
        return NULL;
    }

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        ethernetConnDb = (ethernet_connDb_t *)le_ref_GetValue(iterRef);
        if (0 == strcmp(ethernetConnDb->netIntf, netInterface))
        {
            return ethernetConnDb;
        }
    }
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for internally creating a connection db of the Ethernet type for the given Ethernet
 * interface name
 *
 * @return
 *     - The newly created Ethernet connection db is returned in this function's return value upon
 *       successful creation; otherwise NULL
 */
//--------------------------------------------------------------------------------------------------
static ethernet_connDb_t *DcsEthernetCreateConnDb
(
    const char* netInterface
)
{
    ethernet_connDb_t *ethernetConnDb;
    le_dcs_State_t operationState;

    if (NULL == netInterface)
    {
        LE_ERROR("Cannot create Ethernet connection db with NULL interface");
        return NULL;
    }

    ethernetConnDb = DcsEthernetGetDbFromNetInterface(netInterface);
    if (ethernetConnDb)
    {
        return ethernetConnDb;
    }

    ethernetConnDb = le_mem_ForceAlloc(EthernetConnDbPool);
    if (!ethernetConnDb)
    {
        LE_ERROR("Unable to allocate Ethernet Db for connection %s", netInterface);
        return NULL;
    }

    memset(ethernetConnDb, 0, sizeof(ethernet_connDb_t));

    le_utf8_Copy(ethernetConnDb->netIntf, netInterface, sizeof(ethernetConnDb->netIntf), NULL);

    ethernetConnDb->connRef = le_ref_CreateRef(EthernetConnectionRefMap, ethernetConnDb);

    pa_ethernet_GetInterfaceState(netInterface, &operationState);
    ethernetConnDb->opState = (operationState == LE_DCS_STATE_UP);

    LE_DEBUG("ConnRef %p created for Ethernet connection %s with opState %d",
             ethernetConnDb->connRef, netInterface, ethernetConnDb->opState);
    return (ethernetConnDb);
}

//--------------------------------------------------------------------------------------------------
/**
 * CallBack for PA Ethernet Event Indications.
 */
//--------------------------------------------------------------------------------------------------
static void PaEventIndicationHandler
(
    le_dcs_ChannelInfo_t* ethernetChannelInfoPtr,
    void *contextPtr
)
{
    ethernet_connDb_t *ethernetConnDb;
    le_dcs_ChannelRef_t channelRef;

    bool ipv4AddrAssigned = false;
    bool ipv6AddrAssigned = false;

    if (NULL != ethernetChannelInfoPtr)
    {
        LE_DEBUG("Ethernet event: interface: %s, technology: %d, state: %d",
                 ethernetChannelInfoPtr->name,
                 ethernetChannelInfoPtr->technology,
                 ethernetChannelInfoPtr->state);
    }
    else
    {
        LE_ERROR("ethernetChannelInfoPtr is NULL");
        return;
    }
    //Note at this moment the channel reference of channel info structure is empty,
    //so we can not get data connection from reference, but from interface
    ethernetConnDb = DcsEthernetGetDbFromNetInterface(ethernetChannelInfoPtr->name);
    if (!ethernetConnDb)
    {
        //Create new reported Ethernet channel
        ethernetConnDb = DcsEthernetCreateConnDb(ethernetChannelInfoPtr->name);
        if (!ethernetConnDb)
        {
            LE_ERROR("Failed to create Ethernet connection db for connection %s",
                     ethernetChannelInfoPtr->name);
            return;
        }
    }

    //Update channel status
    ethernetConnDb->opState = (ethernetChannelInfoPtr->state == LE_DCS_STATE_UP);

    channelRef = dcs_GetChannelRefFromTechRef(LE_DCS_TECH_ETHERNET, ethernetConnDb->connRef);
    if (!channelRef)
    {
        // It's a newly reported channel, create its dbs for DCS
        channelRef = dcs_CreateChannelDb(ethernetChannelInfoPtr->technology,
                                         ethernetChannelInfoPtr->name);
        if (!channelRef)
        {
            LE_ERROR("Failed to create dbs for new channel %s of technology %d",
                     ethernetChannelInfoPtr->name, ethernetChannelInfoPtr->technology);
            return;
        }
        LE_DEBUG("Create dbs for new channel %s of technology %d",
                 ethernetChannelInfoPtr->name, ethernetChannelInfoPtr->technology);
    }

    switch (ethernetChannelInfoPtr->state)
    {
        case LE_DCS_STATE_UP:
            //Check whether IP address has been assigned or not
            if (LE_OK != pa_dcs_GetInterfaceState(ethernetConnDb->netIntf,
                                                  &ipv4AddrAssigned,
                                                  &ipv6AddrAssigned))
            {
                LE_ERROR("Failed to retrieve IP address status for %s", ethernetConnDb->netIntf);
                dcs_ChannelEventNotifier(channelRef, LE_DCS_EVENT_DOWN);
                return;
            }
            //Busybox udhcpc only acquires IPV4 address,
            //if IPV4 address is not assigned, ask for IPV4 address
            if (!ipv4AddrAssigned)
            {
                //Stop any running udhcpc first
                if (LE_OK != pa_dcs_StopDhcp(ethernetConnDb->netIntf))
                {
                    LE_DEBUG("Failed to stop dhcp for %s", ethernetConnDb->netIntf);
                }
                //Ask for IPv4 address
                if (LE_OK != pa_dcs_AskForIpAddress(ethernetConnDb->netIntf))
                {
                    LE_ERROR("Failed to obtain IP address for %s", ethernetConnDb->netIntf);
                    dcs_ChannelEventNotifier(channelRef, LE_DCS_EVENT_DOWN);
                    return;
                }
            }
            dcs_ChannelEventNotifier(channelRef, LE_DCS_EVENT_UP);
            break;

        case LE_DCS_STATE_DOWN:
            //Stop any running DHCP
            if (LE_OK != pa_dcs_StopDhcp(ethernetConnDb->netIntf))
            {
                LE_DEBUG("Failed to stop dhcp for %s", ethernetConnDb->netIntf);
            }
            dcs_ChannelEventNotifier(channelRef, LE_DCS_EVENT_DOWN);
            break;

        default:
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Function to get the list of all available Ethernet ports
 *
 * @return
 *     - The function returns LE_OK upon a successful retrieval; otherwise, some other
 *       le_result_t failure cause
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dcsEthernet_GetChannelList
(
    void
)
{
    ethernet_connDb_t *ethernetConnDb;
    size_t listLen = LE_DCS_CHANNEL_LIST_QUERY_MAX;
    le_dcs_ChannelInfo_t channelList[LE_DCS_CHANNEL_LIST_QUERY_MAX];

    uint16_t i = 0;
    memset(channelList, 0, sizeof(channelList));
    le_ref_IterRef_t iterRef = le_ref_GetIterator(EthernetConnectionRefMap);

    //If Ethernet cable is connected before power cycle, there could be no channel event
    //to update Ethernet connection db, hence allow channel query if no channel available,
    //and at least once.
    if (AllowEthernetChannelQuery)
    {
        le_result_t ret = pa_ethernet_GetChannelList(channelList, &listLen);
        if (LE_OK != ret)
        {
            LE_ERROR("Failed to get Ethernet channel list; error: %d", ret);
            dcsTech_CollectChannelQueryResults(LE_DCS_TECH_ETHERNET, LE_FAULT, channelList, 0);
            return LE_FAULT;
        }
        else
        {
            for (i = 0; i < listLen; i++)
            {
                DcsEthernetCreateConnDb(channelList[i].name);
            }
        }
    }
    else
    {
        //Fill channel list from Ethernet channel db.
        i = 0;
        while ((le_ref_NextNode(iterRef) == LE_OK) && (i < LE_DCS_CHANNEL_LIST_QUERY_MAX))
        {
            ethernetConnDb = (ethernet_connDb_t *)le_ref_GetValue(iterRef);
            le_utf8_Copy(channelList[i].name, ethernetConnDb->netIntf, sizeof(channelList[i].name), NULL);
            channelList[i].technology = LE_DCS_TECH_ETHERNET;
            i++;
        }
    }

    if (i > 0)
    {
        AllowEthernetChannelQuery = false;
    }

    dcsTech_CollectChannelQueryResults(LE_DCS_TECH_ETHERNET, LE_OK, channelList, i);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for querying the network interface of the given connection specified in the 1st argument
 *
 * @return
 *     - The retrieved network interface's name will be returned in the 2nd argument which allowed
 *       buffer length is specified in the 3rd argument that is to be observed strictly
 *     - The function returns LE_OK upon a successful retrieval; otherwise, returns LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dcsEthernet_GetNetInterface
(
    void *techRef,          ///< [IN] Object reference of the Ethernet connection
    char *intfName,         ///< [INOUT] Network interface
    int nameSize            ///< [IN] Size of network interface
)
{
    if ((!techRef) || (!intfName))
    {
        LE_ERROR("Invalid parameter");
        return LE_BAD_PARAMETER;
    }

    intfName[0] = '\0';
    ethernet_connDb_t *ethernetConnDb;

    ethernetConnDb = DcsEthernetGetDbFromRef((le_dcs_EthernetConnectionRef_t)techRef);
    if (!ethernetConnDb)
    {
        LE_ERROR("Failed to find Ethernet connection db with reference %p", techRef);
        return LE_FAULT;
    }
    le_utf8_Copy(intfName, ethernetConnDb->netIntf, nameSize, NULL);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for requesting Ethernet to start the given data/connection in the 1st argument
 *
 * @return
 *     - The function returns LE_OK upon a successful start; otherwise, some other
 *       le_result_t failure cause
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dcsEthernet_Start
(
    void *techRef           ///< [IN] Object reference of the Ethernet connection
)
{
    ethernet_connDb_t *ethernetConnDb;
    le_dcs_ChannelRef_t channelRef;

    if (!techRef)
    {
        LE_ERROR("Can not start connection db with NULL tech reference");
        return LE_BAD_PARAMETER;
    }

    bool ipv4AddrAssigned = false;
    bool ipv6AddrAssigned = false;

    ethernetConnDb = DcsEthernetGetDbFromRef((le_dcs_EthernetConnectionRef_t)techRef);
    if (!ethernetConnDb)
    {
        LE_ERROR("Failed to find Ethernet connection db with reference %p", techRef);
        return LE_FAULT;
    }

    if (!ethernetConnDb->opState)
    {
        LE_DEBUG("Connection %s not immediately started due to down Ethernet state",
                 ethernetConnDb->netIntf);
        return LE_UNAVAILABLE;
    }

    if (LE_OK != pa_dcs_GetInterfaceState(ethernetConnDb->netIntf,
                                          &ipv4AddrAssigned,
                                          &ipv6AddrAssigned))
    {
        LE_ERROR("Failed to retrieve IP address status for %s", ethernetConnDb->netIntf);
        return LE_UNAVAILABLE;
    }

    if ((!ipv4AddrAssigned) && (!ipv6AddrAssigned))
    {
        LE_ERROR("IP address of %s is not assigned yet", ethernetConnDb->netIntf);
        return LE_UNAVAILABLE;
    }

    channelRef = dcs_GetChannelRefFromTechRef(LE_DCS_TECH_ETHERNET,
                                                 ethernetConnDb->connRef);
    dcs_ChannelEventNotifier(channelRef, LE_DCS_EVENT_UP);

    LE_DEBUG("Ethernet is started successfully");
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for stopping the given Ethernet connection in the argument
 *
 * @return
 *     - The function returns LE_OK upon a successful stop; otherwise, some other
 *       le_result_t failure cause
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dcsEthernet_Stop
(
    void *techRef                   ///< [IN] object reference of the Ethernet connection
)
{
    ethernet_connDb_t *ethernetConnDb;
    le_dcs_ChannelRef_t channelRef;

    if (!techRef)
    {
        LE_ERROR("Can not stop connection db with NULL tech reference");
        return LE_BAD_PARAMETER;
    }

    ethernetConnDb = DcsEthernetGetDbFromRef((le_dcs_EthernetConnectionRef_t)techRef);
    if (!ethernetConnDb)
    {
        LE_ERROR("Failed to find Ethernet connection db with reference %p", techRef);
        return LE_FAULT;
    }
    channelRef = dcs_GetChannelRefFromTechRef(LE_DCS_TECH_ETHERNET,
                                                 ethernetConnDb->connRef);
    dcs_ChannelEventNotifier(channelRef, LE_DCS_EVENT_DOWN);
    LE_DEBUG("Ethernet is stopped successfully");
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for creating an Ethernet connection db of the given connection if it's not present yet.
 * If present, it will set itself into the given connection's connDb
 *
 * @return
 *     - The object reference to the newly created Ethernet connection db is returned in this
 *       function's return value upon successful creation or found existence; otherwise NULL
 */
//--------------------------------------------------------------------------------------------------
void *le_dcsEthernet_CreateConnDb
(
    const char *netInterface
)
{
    ethernet_connDb_t *ethernetConnDb;

    if (!netInterface)
    {
        LE_ERROR("Can not create connection db for NULL interface");
        return NULL;
    }

    ethernetConnDb = DcsEthernetCreateConnDb(netInterface);
    if (!ethernetConnDb)
    {
        LE_ERROR("Failed to create Ethernet connection db for connection %s", netInterface);
        return NULL;
    }

    LE_DEBUG("Create Ethernet connection db for connection %s", netInterface);
    return ((void *)ethernetConnDb->connRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for checking if the given Ethernet connection db's operational state is up or not
 *
 * @return
 *     - In the function's return value, the bool is returned to indicate whether the given
 *       connection's techRef is up or not
 */
//--------------------------------------------------------------------------------------------------
bool le_dcsEthernet_GetOpState
(
    void *techRef
)
{
    ethernet_connDb_t *ethernetConnDb;

    if (!techRef)
    {
        LE_ERROR("Can not get operational state of connection db with NULL tech reference");
        return false;
    }

    le_dcs_EthernetConnectionRef_t ethernetConnRef = (le_dcs_EthernetConnectionRef_t)techRef;
    ethernetConnDb = DcsEthernetGetDbFromRef(ethernetConnRef);
    if (!ethernetConnDb)
    {
        return false;
    }

    return ethernetConnDb->opState;
}

//--------------------------------------------------------------------------------------------------
/**
 * Ethernet has no technology-specific restriction, thus, this function always returns LE_OK with
 * valid parameter.
 *
 * @return
 *      - LE_OK             Always with valid parameter
 *      - LE_NOT_PERMITTED  Invalid parameter
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dcsEthernet_AllowChannelStart
(
    void *techRef
)
{
    if (!techRef)
    {
        LE_ERROR("Can not allow channel start with NULL tech reference");
        return LE_NOT_PERMITTED;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Destructor function that runs when a Ethernet connection db is deallocated
 */
//--------------------------------------------------------------------------------------------------
static void DcsEthernetConnDbDestructor
(
    void *objPtr
)
{
    ethernet_connDb_t *ethernetConnDb = (ethernet_connDb_t *)objPtr;
    le_ref_DeleteRef(EthernetConnectionRefMap, ethernetConnDb->connRef);
    ethernetConnDb->connRef = NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for releasing a ethernet_connDb_t back to free memory after it's looked up from the
 * given reference in the argument
 */
//--------------------------------------------------------------------------------------------------
void le_dcsEthernet_ReleaseConnDb
(
    void *techRef
)
{
    ethernet_connDb_t *ethernetConnDb;

    if (!techRef)
    {
        LE_ERROR("Can not release connection db with NULL tech reference");
        return;
    }

    le_dcs_EthernetConnectionRef_t ethernetConnRef = (le_dcs_EthernetConnectionRef_t)techRef;
    ethernetConnDb = DcsEthernetGetDbFromRef(ethernetConnRef);
    if (ethernetConnDb)
    {
        le_mem_Release(ethernetConnDb);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 *  Ethernet handlers component initialization
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // Allocate the connection DB app event pool, and set the max number of objects
    EthernetConnDbPool = le_mem_CreatePool("EthernetConnDbPool", sizeof(ethernet_connDb_t));
    le_mem_ExpandPool(EthernetConnDbPool, ETHERNET_CONNDBS_MAX);
    le_mem_SetDestructor(EthernetConnDbPool, DcsEthernetConnDbDestructor);

    // Create a safe reference map for Ethernet connection objects
    EthernetConnectionRefMap = le_ref_CreateMap("Ethernet Connection Reference Map",
                                                ETHERNET_CONNDBS_MAX);
    // Register for events from PA Ethernet.
    le_result_t result = pa_ethernet_AddEventIndHandler(PaEventIndicationHandler, NULL);
    if (LE_OK != result)
    {
        LE_ERROR("Failed to add event handler");
    }

    LE_INFO("Data Channel Service's Ethernet component is ready");
}
