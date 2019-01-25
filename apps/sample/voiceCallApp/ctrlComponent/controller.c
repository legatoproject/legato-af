#include "legato.h"
#include "interfaces.h"

/// Name used to launch this program.
static const char* ProgramName;

/// Set audio to default flag
bool setAudioToDefaultFlag = false;

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
        "    voice - Used to perform voice call operations.\n"
        "\n"
        "PREREQUISITES:\n"
        "    SIM is inserted, registered on the network, and is in ready state. Type cm sim in order to see state.\n"
        "\n"
        "    voiceCallService is running. voiceCallService can be started using app start voiceCallService.\n"
        "\n"
        "DESCRIPTION:\n"
        "    voice call <Destination Number>\n"
        "       Initiates a voice call to <Destination Number>.  <Destination Number> is assumed to be valid and\n"
        "       registered on the network.\n"
        "\n"
        "    voice answer\n"
        "       Answers an incoming voice call. LE_VOICECALL_EVENT_INCOMING indicates that there is an incoming call.\n"
        "\n"
        "    voice redial\n"
        "       Initiates a voice call to the last dialed number.\n"
        "\n"
        "    voice hangup\n"
        "       Ends an active voice call. If there is an incoming call, it rejects the call. If a number is being\n"
        "       dialed, it ends the outgoing call.\n"
        "\n"
        "    voice hold\n"
        "       Places an active call on hold and plays music for the other side of the call whilst disconnecting all\n"
        "       audio input and output from the mic the speaker respectively. By default, an included piano.wav file\n"
        "       will be played. This can be changed by specifying a new .wav file. Please refer to voice wav <path>.\n"
        "\n"
        "    voice unhold\n"
        "       Unholds a call which has been placed on hold. This will reconnect all audio input and output back to\n"
        "       to the mic and the speaker respectively and stop the music.\n"
        "\n"
        "    voice wav <path>\n"
        "       Changes the audio file played while placing a call on hold to the new file specified by <path>. <path>\n"
        "       is assumed to be an absolute path. The format of the file specified must be WAV.\n"
        "\n"
        "    voice wav default\n"
        "       Changes the audio file played while placing a call on hold to the default piano.wav included with the app.\n"
        "\n"
        //! [help]
        );

    exit(EXIT_SUCCESS);
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
    ctrlVC_MakeCall(argPtr);
}

//--------------------------------------------------------------------------------------------------
/**
* Sets the path to a .wav file specified on the command-line.
*/
//--------------------------------------------------------------------------------------------------
static void SetAudioFilePath
(
    const char* argPtr
)
{
    if (strcmp(argPtr, "default") == 0)
    {
        setAudioToDefaultFlag = true;
        ctrlVC_SetWav(argPtr, setAudioToDefaultFlag);
    }
    else
    {
        setAudioToDefaultFlag = false;
        ctrlVC_SetWav(argPtr, setAudioToDefaultFlag);
    }
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
    if (strcmp(argPtr, "call") == 0 && le_arg_NumArgs() == 2)
    {
        le_arg_AddPositionalCallback(SetNumber);
    }
    else if (strcmp(argPtr, "hangup") == 0 && le_arg_NumArgs() == 1)
    {
        ctrlVC_HangupCall();
    }
    else if (strcmp(argPtr, "redial") == 0 && le_arg_NumArgs() == 1)
    {
        ctrlVC_Redial();
    }
    else if (strcmp(argPtr, "hold") == 0 && le_arg_NumArgs() == 1)
    {
        ctrlVC_HoldCall();
    }
    else if (strcmp(argPtr, "unhold") == 0 && le_arg_NumArgs() == 1)
    {
        ctrlVC_UnholdCall();
    }
    else if (strcmp(argPtr, "answer") == 0 && le_arg_NumArgs() == 1)
    {
        ctrlVC_AnswerCall();
    }
    else if (strcmp(argPtr, "wav") == 0)
    {
        le_arg_AddPositionalCallback(SetAudioFilePath);
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
// Read out the program name
    ProgramName = le_arg_GetProgramName();
    if (ProgramName == NULL)
    {
        ProgramName = "voice";
    }

    le_arg_SetFlagCallback(PrintHelp, "h", "help");

// The first positional argument is the command that the caller wants to execute.
    le_arg_AddPositionalCallback(CommandHandler);

// Scan the argument list.
    le_arg_Scan();

    exit(EXIT_SUCCESS);
}
