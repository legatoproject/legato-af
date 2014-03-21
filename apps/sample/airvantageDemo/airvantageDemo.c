#include "legato.h"
#include "swi_airvantage.h"

#define ASSET_ID "house"
#define POLICY "now"

static void AddBedroomData(swi_av_Asset_t *asset, swi_av_Table_t *table, int temperature)
{
    rc_ReturnCode_t res;

    LE_INFO("Add bedroom data\n");

    swi_av_table_PushInteger(table, temperature);
    swi_av_table_PushInteger(table, 19);
    swi_av_table_PushInteger(table, SWI_AV_TSTAMP_AUTO);

    res = swi_av_table_PushRow(table);
    if (res != RC_OK)
    {
        fprintf(stderr, "Failed to push row to agent\n");
        exit(1);
    }

    if (temperature < 13)
    {
        swi_av_asset_PushInteger(asset, "bedroom.event.temptoolow.temperature", POLICY, SWI_AV_TSTAMP_AUTO, temperature);
        swi_av_asset_PushInteger(asset, "bedroom.event.temptoolow.alarmtemperature", POLICY, SWI_AV_TSTAMP_AUTO, 13);
    }
}

int sampleMain()
{
    swi_av_Asset_t *asset = NULL;
    swi_av_Table_t *table = NULL;
    rc_ReturnCode_t res;

    const char *columns[] = {
          "col1",
          "col2",
          "col3"
        };

    res = swi_av_Init();
    if (res != RC_OK)
    {
        fprintf(stderr, "Failed to initialize airvantage module\n");
        return 1;
    }

    LE_INFO("Initializing asset\n");
    res = swi_av_asset_Create(&asset, ASSET_ID);
    if (res != RC_OK)
    {
        fprintf(stderr, "Failed to create asset\n");
        return 1;
    }

    LE_INFO("Registering asset\n");
    swi_av_asset_Start(asset);
    if (res != RC_OK)
    {
        fprintf(stderr, "Failed to register asset module\n");
        return 1;
    }

    res = swi_av_asset_PushString(asset, "event.status", "now", SWI_AV_TSTAMP_AUTO, "booting");
    if (res != RC_OK)
    {
        fprintf(stderr, "Failed to push data\n");
        return 1;
    }

    res = swi_av_table_Create(asset, &table, "col.data", 3, columns, POLICY, STORAGE_RAM, 0);
    if (res != RC_OK)
    {
        fprintf(stderr, "Failed to create table bedroom.data\n");
        return 1;
    }

    AddBedroomData(asset, table, 10);
    sleep(2);

    swi_av_asset_Destroy(asset);

    swi_av_Destroy();
    return 0;
}

COMPONENT_INIT
{
    LE_INFO("Sample Airvantage Starting\n");
    sampleMain();
}
