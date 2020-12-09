#include "legato.h"
#include "interfaces.h"
#include "atProxyUnsolicitedRsp.h"
#include "atProxySerialUart.h"

// Max length for one unsolicitied response
#define AT_PROXY_UNSOLICITED_RESPONSE_MAX_BYTES LE_ATDEFS_COMMAND_MAX_BYTES

// Typical length of a response string
#define AT_PROXY_UNSOLICITED_RESPONSE_TYPICAL_BYTES 50

// Buffer for caching one unsolicited response
static char UnsolicitedRsp[AT_PROXY_UNSOLICITED_RESPONSE_MAX_BYTES];

// Index of next position in buffer for storing a byte of unsolicited response
static int UnsolRspInd = 0;

// Flag indicates whether an unsolicited response is started
typedef enum
{
    PARSER_SEARCH_START_CR,   // starting '\r'
    PARSER_SEARCH_START_NL,   // starting '\n'
    PARSER_SEARCH_END_CR,     // ending '\r'
    PARSER_SEARCH_END_NL      // ending '\n'
} le_atProxy_UnsolicitedRspState_t;

// Paser state for unsolicited response processor
static le_atProxy_UnsolicitedRspState_t UnsolRspState = PARSER_SEARCH_START_CR;

// Pool size for large-size response
#define DEFAULT_UNSOLICITED_RSP_COUNT_LARGE       2

// Pool size for unsolicited response items
#define DEFAULT_UNSOLICITED_RSP_COUNT             8

// Count of typical-size unsolicited response in pool
#define TYPICAL_UNSOLICITED_RESPONSE_POOL_SIZE                                      \
    (((LE_MEM_BLOCKS(UnsoliRspPoolRef, DEFAULT_UNSOLICITED_RSP_COUNT_LARGE) / 2)    \
        * AT_PROXY_UNSOLICITED_RESPONSE_MAX_BYTES)                                  \
            / AT_PROXY_UNSOLICITED_RESPONSE_TYPICAL_BYTES)

// Static pool for response data
LE_MEM_DEFINE_STATIC_POOL(UnsoliRspDataPool,
                          DEFAULT_UNSOLICITED_RSP_COUNT_LARGE,
                          AT_PROXY_UNSOLICITED_RESPONSE_MAX_BYTES);

// Pool for response string content
static le_mem_PoolRef_t  UnsoliRspDataPoolRef;

// Node of list for unsolicited message
typedef struct le_atProxy_UnSolRspString
{
    le_dls_Link_t   link;               ///< link for list
    char* resp;                         ///< string pointer
} le_atProxy_UnSolRspString_t;

// Unsolicited list to be sent
static le_dls_List_t UnsolicitedList;

// Static pool for response pointer
LE_MEM_DEFINE_STATIC_POOL(UnsoliRspPool,
                          DEFAULT_UNSOLICITED_RSP_COUNT,
                          sizeof(le_atProxy_UnSolRspString_t));

// Pool for response string pointer
static le_mem_PoolRef_t  UnsoliRspPoolRef;


//--------------------------------------------------------------------------------------------------
/**
 * Create an unsolicited response from buffer and queue it
 *
 * @return none
 */
//--------------------------------------------------------------------------------------------------
static void CreateResponse
(
    const char* resp        ///< [IN] A NULL-terminated unsolicited response
)
{
    // Allocate a node for list
    le_atProxy_UnSolRspString_t* rsp = le_mem_Alloc(UnsoliRspPoolRef);
    LE_FATAL_IF(NULL == rsp, "Could not allocate an unsolicited response instance");

    // Allocate space for response content
    int size = strnlen(resp, AT_PROXY_UNSOLICITED_RESPONSE_MAX_BYTES - 1) + 1;
    rsp->resp = le_mem_VarAlloc(UnsoliRspDataPoolRef, size);
    LE_FATAL_IF(NULL == rsp->resp, "Could not allocate space for string of size %d", size);

    memset(rsp->resp, 0, size);
    memcpy(rsp->resp, resp, size);

    // Add the node to the list
    le_dls_Queue(&UnsolicitedList, &(rsp->link));
}

//--------------------------------------------------------------------------------------------------
/**
 * Output unsolicited responses to AT port
 *
 * @return none
 */
//--------------------------------------------------------------------------------------------------
void atProxyUnsolicitedRsp_output
(
    void
)
{
    le_dls_Link_t* linkPtr;

    while ( (linkPtr = le_dls_Pop(&UnsolicitedList)) != NULL )
    {
        le_atProxy_UnSolRspString_t* rspStringPtr = CONTAINER_OF(linkPtr,
                                                                 le_atProxy_UnSolRspString_t,
                                                                 link);

        atProxySerialUart_write(
                            rspStringPtr->resp,
                            strnlen(rspStringPtr->resp, AT_PROXY_UNSOLICITED_RESPONSE_MAX_BYTES));

        le_mem_Release(rspStringPtr->resp);

        le_mem_Release(rspStringPtr);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Process an incoming unsolicited character
 *
 * @return none
 */
//--------------------------------------------------------------------------------------------------
void atProxyUnsolicitedRsp_parse
(
    char ch             ///< [IN] Incoming character of an unsolicited response
)
{
    switch (UnsolRspState)
    {
        case PARSER_SEARCH_START_CR:
            if (ch == '\r')
            {
                UnsolRspState = PARSER_SEARCH_START_NL;
            }
            else
            {
                return;
            }
            break;
        case PARSER_SEARCH_START_NL:
            if (ch == '\n')
            {
                UnsolRspState = PARSER_SEARCH_END_CR;
            }
            else
            {
                // Something not expected. Reposition index and start again
                UnsolRspInd = 0;
                UnsolRspState = PARSER_SEARCH_START_CR;
                return;
            }
            break;
        case PARSER_SEARCH_END_CR:
            if (ch == '\r')
            {
                UnsolRspState = PARSER_SEARCH_END_NL;
            }
            break;
        case PARSER_SEARCH_END_NL:
            if (ch == '\n')
            {
                if (UnsolRspInd < AT_PROXY_UNSOLICITED_RESPONSE_MAX_BYTES - 1)
                {
                    UnsolicitedRsp[UnsolRspInd++] = ch;
                    UnsolicitedRsp[UnsolRspInd] = '\0';
                    // Queue the found response
                    CreateResponse(UnsolicitedRsp);
                }
                else
                {
                    LE_ERROR("Unsolicited message is too long!");
                }

                // Reset state and index for next parsing
                UnsolRspState = PARSER_SEARCH_START_CR;
                UnsolRspInd = 0;
                return;
            }
            else
            {
                UnsolRspState = PARSER_SEARCH_END_CR;
            }
            break;
        default:
            LE_ERROR("Bad state!");
            UnsolRspState = PARSER_SEARCH_START_CR;
            UnsolRspInd = 0;
            return;
    }

    if (UnsolRspInd < AT_PROXY_UNSOLICITED_RESPONSE_MAX_BYTES)
    {
        UnsolicitedRsp[UnsolRspInd++] = ch;
    }
    else
    {
        LE_ERROR("Unsolicited message is too long!");
    }
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize unsolicited response processor
 *
 * @return none
 */
//--------------------------------------------------------------------------------------------------
void atProxyUnsolicitedRsp_init
(
    void
)
{
    // Initialize memory pools
    UnsoliRspDataPoolRef = le_mem_InitStaticPool(
                                    UnsoliRspDataPool,
                                    DEFAULT_UNSOLICITED_RSP_COUNT_LARGE,
                                    AT_PROXY_UNSOLICITED_RESPONSE_MAX_BYTES);

    UnsoliRspDataPoolRef = le_mem_CreateReducedPool(
                                    UnsoliRspDataPoolRef,
                                    "TypicalUnsolRspPool",
                                    TYPICAL_UNSOLICITED_RESPONSE_POOL_SIZE,
                                    AT_PROXY_UNSOLICITED_RESPONSE_TYPICAL_BYTES);

    UnsoliRspPoolRef = le_mem_InitStaticPool(
                                    UnsoliRspPool,
                                    DEFAULT_UNSOLICITED_RSP_COUNT,
                                    sizeof(le_atProxy_UnSolRspString_t));
}
