//--------------------------------------------------------------------------------------------------
/**
 * Song Selector Component.
 *
 * This component is used to select the song and speed for the song player.  The playback speed is
 * specified as an command line argument and the song selection is done interactively on the command
 * line.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"


COMPONENT_INIT
{
    songs_SetSpeed(le_arg_GetArg(0));

    while (1)
    {
        puts(
            "Select a song:\n"
            "\n"
            "    0 = None\n"
            "    1 = Danny Boy\n"
            "    2 = Jingle Bells\n"
            "    3 = Deck The Halls\n"
            );

        char selection[10];

        if (fgets(selection, sizeof(selection), stdin) != NULL)
        {
            switch (selection[0])
            {
                case '0':
                    songs_SetSong("");
                    printf("Disabling playback.\n");
                    break;

                case '1':
                    songs_SetSong("dannyBoy");
                    printf("'Danny Boy' selected.  Playing now.\n");
                    break;

                case '2':
                    songs_SetSong("jingleBells");
                    printf("'Jingle Bells' selected.  Playing now.\n");
                    break;

                case '3':
                    songs_SetSong("deckTheHalls");
                    printf("'Deck The Halls' selected.  Playing now.\n");
                    break;

                default:
                    fprintf(stderr, "Invalid selection.\n");
            }
        }
        else
        {
            fprintf(stderr, "Failed to read selection.  Setting default song.\n");

            songs_SetSong("dannyBoy");

            exit(EXIT_SUCCESS);
        }
    }
}
