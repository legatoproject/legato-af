#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
* Maximum number of letters allowed in POI search
*
*/
//--------------------------------------------------------------------------------------------------
#define MAX_NAME_LETTERS 26

//--------------------------------------------------------------------------------------------------
/**
* Reference to handler function for updating search results
*
*/
//--------------------------------------------------------------------------------------------------
static ctrlGPS_UpdatedValueHandlerRef_t UpdatedValueHandler;

//--------------------------------------------------------------------------------------------------
/**
* Number of words contained in POI search
*
*/
//--------------------------------------------------------------------------------------------------
static int CommandWordCount;

//--------------------------------------------------------------------------------------------------
/**
* Program Name
*
*/
//--------------------------------------------------------------------------------------------------
static const char* ProgramName;

//--------------------------------------------------------------------------------------------------
/**
* Input options
*
*/
//--------------------------------------------------------------------------------------------------
static int KM;
static int Accuracy;

//--------------------------------------------------------------------------------------------------
/**
* Prints help to stdout
*
*/
//--------------------------------------------------------------------------------------------------
static void PrintHelp
(
    void
)
{
    puts(
        //! [help]
        "NAME\n"
        "        map - perform GNSS/positioning operations such as finding nearby places using forward geocoding.\n"
        "\n"
        "Prerequisites\n"
        "        A MapBox Access Token is needed for all operations.\n"
        "        A GNSS receiver as well as data connection are required as well.\n"
        "\n"
        "SYNOPSIS\n"
        "        map [OPTION]... COMMAND [Place Name]\n"
        "        map -h\n"
        "        map --help\n"
        "\n"
        "COMMANDS\n"
        "       find\n"
        "               Searches for <Place Name> in the proximity of the current coordinates provided by the GNSS\n"
        "                service and fed into the mapbox api.\n"
        "\n"
        "       locate\n"
        "               Get the current coordinates using the GNSS service, and feed them into mapbox's reverse-\n"
        "                geocoding api to turn the coordinates into an address.\n"
        "\n"
        "OPTIONS\n"
        "       -w N\n"
        "       --within=N\n"
        "               Confine the search to within an imaginary square with sides 2*N KM and your current\n"
        "                coordinates are at the center of the square. The provided N KMs will be converted\n"
        "                to coordinates and fed into the mapbox api.\n"
        "               If not specified, the current coordinates will be used and results will be found,\n"
        "                within the proximity of the current location.\n"
        "       -a N\n"
        "       --accuracy=N\n"
        "               This is the accuracy in meters to which the device will be located. In other words,\n"
        "                if the accuracy reaches 20m the algorithm will not try to further the accuracy and use\n"
        "                the available coordinates.\n"
        "               If not specified, it will be set to 20 meters by default.\n"
        //! [help]
        );

    exit(EXIT_SUCCESS);
}

//--------------------------------------------------------------------------------------------------
/**
* Set point of interest name specified on the command line and send it to Find function with the options
*
*/
//--------------------------------------------------------------------------------------------------
static void SetPOI
(
    const char* argPtr
)
{
    static char FullName[MAX_NAME_LETTERS];
    static int  Words;

    le_arg_AllowLessPositionalArgsThanCallbacks();

    strcat(FullName, argPtr);

    if (++Words == CommandWordCount)
    {
        ctrlGPS_FindPoi(FullName, (double)KM, (double)Accuracy);
    }
    else
    {
        strcat(FullName, "+");
        if (strlen(FullName) >= MAX_NAME_LETTERS)
        {
          fprintf(stderr, "Name is too long. Make sure there are at most 26 letters, including spaces, in your search name.\n");
          ctrlGPS_CleanUp(false);
      }
      le_arg_AddPositionalCallback(SetPOI);
  }
}

//--------------------------------------------------------------------------------------------------
/**
* Sets the command handler for the option selected using the command line. Also sets the number of
*  words contained in the search.
*
*/
//--------------------------------------------------------------------------------------------------
static void CommandHandler
(
    const char* argPtr                  ///< [IN] Command-line argument.
)
{
    // To get the number of words in the search, we first deduct the command from the argument list count
    CommandWordCount = le_arg_NumArgs() - 1;

    // If accuracy option was specified it counts as 2 arguments, so we deduct 2 from argument list count
    if (Accuracy)
    {
        CommandWordCount += -2;
    }
    // If bbox option was specified it counts as 2 arguments, so we deduct 2 from argument list count
    if (KM)
    {
        CommandWordCount += -2;
    }
    // Now we are left with the total number of words in front of the command

    if (strcmp(argPtr, "find") == 0)
    {
        le_arg_AddPositionalCallback(SetPOI);
    }
    // If CommandWordCount larger than 0, it is too many arguments
    else if (strcmp(argPtr, "locate") == 0 && CommandWordCount == 0)
    {
        ctrlGPS_LocateMe((double)Accuracy);
    }
    else
    {
        fprintf(stderr, "Unknown command.\n");
        fprintf(stderr, "Try '%s --help'.\n", ProgramName);
        exit(EXIT_FAILURE);
    }
}

//--------------------------------------------------------------------------------------------------
/**
* Handler function for updated search results
*
*/
//--------------------------------------------------------------------------------------------------
void UpdatedValueHandlerFunc
(
    const ctrlGPS_ResultInfo_t* results,        ///< temperature in F
    void* contextPtr                         ///< Context pointer
)
{
    static int resultCounter;

    // If error is set, there is an error
    if (results->error)
    {
        fprintf(stderr, "An error has occured! Please refer to the logs for more information.\n");
        ctrlGPS_CleanUp(false);
    }
    // If search is done but result counter is 0, it means that the locate me option was selected. So we only display
    //  the location
    if (resultCounter == 0 && results->searchDone)
    {
        printf("You are located at: %s\n", results->result);
    }
    // If search is done, cleanup and exit
    if (results->searchDone)
    {
        printf("\nSearch Complete!\n");
        ctrlGPS_RemoveUpdatedValueHandler(UpdatedValueHandler);
        ctrlGPS_CleanUp(results->searchDone);
        exit(EXIT_SUCCESS);
    }
    // Print the address
    printf("Result %d (%-5.1f KM): %s\n", ++resultCounter, results->distance, results->result);
}

//--------------------------------------------------------------------------------------------------
/**
* Set program name if not aleady set
* Search command line arguments for varius available options
* Add positional callback functions
* Scan argument list
*
*/
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    UpdatedValueHandler = ctrlGPS_AddUpdatedValueHandler (UpdatedValueHandlerFunc, NULL);

    // Read out the program name so that we can better format our error and help messages.
    ProgramName = le_arg_GetProgramName();
    if (ProgramName == NULL)
    {
        ProgramName = "map";
    }

    le_arg_SetFlagCallback(PrintHelp, "h", "help");
    le_arg_SetIntVar(&KM, "w", "within");
    le_arg_SetIntVar(&Accuracy, "a", "accuracy");
    le_arg_AddPositionalCallback(CommandHandler);
    le_arg_Scan();
}
