#include "legato.h"
#include "interfaces.h"

static const char*  ProgramName;
const char*         numPtr;
bool                async = false;
bool                AT = false;

//--------------------------------------------------------------------------------------------------
/**
* Prints help to stdout.
*/
//--------------------------------------------------------------------------------------------------
static void PrintHelp
(
    void
)
{
    puts(
        "\n"
        //! [help]
        "NAME:\n"
        "    sms - Used to perform SMS operations\n"
        "\n"
        "PREREQUISITES:\n"
        "    SIM is inserted, registered on the network, and is in ready state. Type cm sim in order to see state.\n"
        "    This is only necessary if you wish to send and receive txt mesasges. Inbox operations don't require this.\n"
        "\n"
        "DESCRIPTION:\n"
        "    sms send <Destination Number> <Text Message to Send>\n"
        "       Sends <Text Message to Send> to <Destination Number> synchronously. <Destination Number> is assumed\n"
        "       to be valid and registered on the network.\n"
        "       If a text message is longer than 160 characters, it automatically gets broken down into smaller texts\n"
        "       each containing at most 160 characters and gets sent through multiple messages. However, it is important\n"
        "       to note that longer than 511 character messages are not supported due to legato argument parser and not\n"
        "       the app itself.\n"
        "\n"
        "    sms sendAsync <Destination Number> <Text Message to Send>\n"
        "       Sends <Text Message to Send> to <Destination Number> asynchronously. <Destination Number> is assumed\n"
        "       to be valid and registered on the network.\n"
        "\n"
        "    sms sendAT <Destination Number> <Text Message to Send>\n"
        "       Sends <Text Message to Send> to <Destination Number> using AT commands API. <Destination Number> is assumed\n"
        "       to be valid and registered on the network.\n"
        "\n"
        "    sms inbox\n"
        "       Loads the messages stored on the device and displays the first one in the list.\n"
        "\n"
        "    sms next\n"
        "       Goes to the next message in the inbox and displays its contents. Messages need to already be loaded\n"
        "       in advance by typig: sms inbox. If end of messages is reached, it will go back to the beginning.\n"
        "\n"
        "    sms delete\n"
        "       Deletes from storage the last message which was displayed using either Inbox or Next.\n"
        "\n"
        "    sms status\n"
        "       Displays the status of the last message which was displayed using either Inbox or Next.\n"
        "\n"
        "    sms unread\n"
        "       Marks as unread, the last message which was displayed using either Inbox or Next.\n"
        "\n"
        "    sms count\n"
        "       Counts the total number of received messages since the last reset count.\n"
        "\n"
        "    sms reset\n"
        "       Resets the received message counter.\n"
        "\n"
        "DEVICE INFORMATION REQUESTS:\n"
        "    Send the following messages to the device via SMS and get the associated device information as a reply.\n"
        "\n"
        "    info reset\n"
        "       Get various reset information\n"
        "\n"
        "    info psn\n"
        "       Get Platform Serial Number (PSN).\n"
        "\n"
        "    info sku\n"
        "       Get product stock keeping unit number (SKU).\n"
        "\n"
        "    info pri\n"
        "       Get Product Requirement Information Identifier (PRI ID) Part Number and the Revision Number.\n"
        "\n"
        "    info manufacturer\n"
        "       Get Manufacturer name.\n"
        "\n"
        "    info nai\n"
        "       get CDMA Network Access Identifier (NAI).\n"
        "\n"
        "    info prl preference\n"
        "       Get CDMA Preferred Roaming List (PRL) only preferences status.\n"
        "\n"
        "    info prl version\n"
        "       Get CDMA version of Preferred Roaming List (PRL).\n"
        "\n"
        "    info min\n"
        "       Get CDMA Mobile Identification Number (MIN).\n"
        "\n"
        "    info mdn\n"
        "       Get Mobile Directory Number (MDN) of the device.\n"
        "\n"
        "    info esn\n"
        "       Get Electronic Serial Number (ESN) of the device.\n"
        "\n"
        "    info meid\n"
        "       Get CDMA device Mobile Equipment Identifier (MEID).\n"
        "\n"
        "    info imeisv\n"
        "       Get International Mobile Equipment Identity software version number (IMEISV).\n"
        "\n"
        "    info imei\n"
        "       Get International Mobile Equipment Identity (IMEI).\n"
        "\n"
        "    info device model\n"
        "       Get target hardware platform identity.\n"
        "\n"
        "    info bootloader\n"
        "       Get bootloader version.\n"
        "\n"
        "    info firmware\n"
        "       Get firmware version.\n"
        //! [help]
        );

    exit(EXIT_SUCCESS);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets the text message specified on the command-line.
 */
//--------------------------------------------------------------------------------------------------

static void SetTxt
(
    const char* argPtr
)
{
    ctrlSMS_SendMessage(numPtr, argPtr, async, AT);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets the phone number specified on the command-line.
 */
//--------------------------------------------------------------------------------------------------

static void SetNumber
(
    const char* argPtr
)
{
    numPtr = argPtr;
    le_arg_AddPositionalCallback(SetTxt);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets the command handler to call depending on which command was specified on the command-line.
 */
//--------------------------------------------------------------------------------------------------
static void CommandHandler
(
    const char* argPtr                  ///< [IN] Command-line argument.
)
{
    if (strcmp(argPtr, "send") == 0)
    {
        async = false;
        le_arg_AddPositionalCallback(SetNumber);
    }
    else if (strcmp(argPtr, "sendAsync") == 0)
    {
        async = true;
        le_arg_AddPositionalCallback(SetNumber);
    }
    else if (strcmp(argPtr, "sendAT") == 0)
    {
        AT = true;
        le_arg_AddPositionalCallback(SetNumber);
    }
    else if (strcmp(argPtr, "inbox") == 0)
    {
        ctrlSMS_GetInbox();
    }
    else if (strcmp(argPtr, "next") == 0)
    {
        ctrlSMS_GetNext();
    }
    else if (strcmp(argPtr, "delete") == 0)
    {
        ctrlSMS_DeleteMessage();
    }
    else if (strcmp(argPtr, "status") == 0)
    {
        ctrlSMS_GetStatus();
    }
    else if (strcmp(argPtr, "unread") == 0)
    {
        ctrlSMS_MarkUnread();
    }
    else if (strcmp(argPtr, "count") == 0)
    {
        ctrlSMS_GetCount();
    }
    else if (strcmp(argPtr, "reset") == 0)
    {
        ctrlSMS_ResetCount();
    }
    else
    {
        fprintf(stderr, "Unknown command.\n");
        fprintf(stderr, "Try '%s --help'.\n", ProgramName);
        exit(EXIT_FAILURE);
    }
}

COMPONENT_INIT
{
    // Read out the program name so that we can better format our error and help messages.
    ProgramName = le_arg_GetProgramName();
    if (ProgramName == NULL)
    {
        ProgramName = "sms";
    }

    le_arg_SetFlagCallback(PrintHelp, "h", "help");

    // The first positional argument is the command that the caller wants us to execute.
    le_arg_AddPositionalCallback(CommandHandler);

    // Scan the argument list.  This will set the CommandHandler and its parameters.
    le_arg_Scan();

    exit(EXIT_SUCCESS);
}
