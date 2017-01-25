//--------------------------------------------------------------------------------------------------
/**
 * Song Player Component.
 *
 * This component reads the lyrics from a song file one line at a time and prints it to its standard
 * out.  This component also services an API that can be used to set the song file and the playback
 * speed.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"

static FILE* SongFile = NULL;
static le_timer_Ref_t PlayTimer = NULL;
static bool ValidSongFile = false;

#define SLOW_SPEED          4000
#define NORMAL_SPEED        2000
#define FAST_SPEED          200


static void Play
(
    le_timer_Ref_t timerRef
)
{
    if (ValidSongFile)
    {
        char line[3000] = "";

        errno = 0;

        if (fgets(line, sizeof(line), SongFile) == NULL)
        {
            if (errno == 0)
            {
                // Reached the end of the file.  Go back to the beginning.
                rewind(SongFile);
            }
            else
            {
                fprintf(stderr, "Could not read song file.  %m.\n");
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            printf("--- %s\n", line);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets a song.
 */
//--------------------------------------------------------------------------------------------------
void songs_SetSong
(
    const char* songPath
        ///< [IN] Path to the song file.
)
{
    if (PlayTimer != NULL)
    {
        le_timer_Stop(PlayTimer);
    }

    // Close the current song file.
    if (SongFile != NULL)
    {
        fclose(SongFile);
    }

    ValidSongFile = false;

    if (strcmp(songPath, "") != 0)
    {
        // Open the new song file.
        SongFile = fopen(songPath, "r");

        if (SongFile == NULL)
        {
            fprintf(stderr, "Could not open song file %s.\n", songPath);
            exit(EXIT_FAILURE);
        }

        printf("Playing file %s.\n", songPath);

        ValidSongFile = true;
    }

    le_timer_Start(PlayTimer);
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets speed.  "normal", "slow", "fast".
 */
//--------------------------------------------------------------------------------------------------
void songs_SetSpeed
(
    const char* speed
        ///< [IN] Song speed.
)
{
    if (PlayTimer != NULL)
    {
        le_timer_Stop(PlayTimer);
    }

    if (strcmp(speed, "slow") == 0)
    {
        le_timer_SetMsInterval(PlayTimer, SLOW_SPEED);
    }
    else if (strcmp(speed, "fast") == 0)
    {
        le_timer_SetMsInterval(PlayTimer, FAST_SPEED);
    }
    else
    {
        le_timer_SetMsInterval(PlayTimer, NORMAL_SPEED);
    }

    le_timer_Start(PlayTimer);
}


COMPONENT_INIT
{
    printf("Starting Karaoke Player.\n");

    PlayTimer = le_timer_Create("playTimer");

    le_timer_SetHandler(PlayTimer, Play);
    le_timer_SetRepeat(PlayTimer, 0);
    le_timer_SetMsInterval(PlayTimer, NORMAL_SPEED);
}
