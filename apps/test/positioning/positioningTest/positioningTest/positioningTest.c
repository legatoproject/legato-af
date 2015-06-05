 /**
  * This module implements the le_posCtrl's tests.
  *
  * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
  *
  */

#include "legato.h"
#include <stdio.h>

#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Test: Get position Fix info.
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_pos_GetInfo()
{
    int32_t     latitude;
    int32_t     longitude;
    int32_t     altitude;
    int32_t     hAccuracy;
    int32_t     vAccuracy;
    uint32_t    hSpeed;
    int32_t     vSpeed;
    int32_t     hSpeedAccuracy;
    int32_t     vSpeedAccuracy;
    int32_t     heading;
    int32_t     headingAccuracy=0;
    int32_t     direction;
    int32_t     directionAccuracy=0;
    le_result_t res;

    res= le_pos_Get2DLocation(&latitude, &longitude, &hAccuracy);
    LE_INFO("le_pos_Get2DLocation %s"
            , (res==LE_OK)?"OK":(res==LE_OUT_OF_RANGE)?"parameter(s) out of range":"ERROR");
    LE_ASSERT((res ==LE_OK)||(res == LE_OUT_OF_RANGE));
    LE_INFO("Check le_pos_Get2DLocation latitude.%d, longitude.%d, hAccuracy.%d"
            , latitude, longitude, hAccuracy);

    res= le_pos_Get3DLocation(&latitude, &longitude, &hAccuracy, &altitude, &vAccuracy);
    LE_INFO("le_pos_Get3DLocation %s"
            , (res==LE_OK)?"OK":(res==LE_OUT_OF_RANGE)?"parameter(s) out of range":"ERROR");
    LE_ASSERT((res ==LE_OK)||(res == LE_OUT_OF_RANGE));
    LE_INFO("Check le_pos_Get3DLocation latitude.%d, longitude.%d, hAccuracy.%d, altitude.%d"
            ", vAccuracy.%d"
            , latitude, longitude, hAccuracy, altitude, vAccuracy);

    res= le_pos_GetMotion(&hSpeed, &hSpeedAccuracy, &vSpeed, &vSpeedAccuracy);
    LE_INFO("le_pos_GetMotion %s"
            , (res==LE_OK)?"OK":(res==LE_OUT_OF_RANGE)?"parameter(s) out of range":"ERROR");
    LE_ASSERT((res ==LE_OK)||(res == LE_OUT_OF_RANGE));
    LE_INFO("Check le_pos_GetMotion hSpeed.%d, hSpeedAccuracy.%d, vSpeed.%d, vSpeedAccuracy.%d"
            , hSpeed, hSpeedAccuracy, vSpeed, vSpeedAccuracy);

    res= le_pos_GetHeading(&heading, &headingAccuracy);
    LE_INFO("le_pos_GetHeading %s"
            , (res==LE_OK)?"OK":(res==LE_OUT_OF_RANGE)?"parameter(s) out of range":"ERROR");
    LE_ASSERT((res ==LE_OK)||(res == LE_OUT_OF_RANGE));
    LE_INFO("Check le_pos_GetHeading heading.%d, headingAccuracy.%d"
            , heading, headingAccuracy);

    res= le_pos_GetDirection(&direction, &directionAccuracy);
    LE_INFO("le_pos_GetDirection %s"
            , (res==LE_OK)?"OK":(res==LE_OUT_OF_RANGE)?"parameter(s) out of range":"ERROR");
    LE_ASSERT((res ==LE_OK)||(res == LE_OUT_OF_RANGE));
    LE_INFO("Check le_pos_GetDirection direction.%d, directionAccuracy.%d"
            , direction, directionAccuracy);

}

//--------------------------------------------------------------------------------------------------
/**
 * App init.
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    le_posCtrl_ActivationRef_t      activationRef;

    LE_INFO("======== Positioning Test started  ========");
    LE_INFO("Get initial position");
    Testle_pos_GetInfo();
    LE_INFO("Request activation of the positioning service");
    LE_ASSERT((activationRef = le_posCtrl_Request()) != NULL);
    LE_INFO("Wait 120 seconds for a 3D fix");
    sleep(120);
    Testle_pos_GetInfo();
    sleep(1);
    LE_INFO("Release the positioning service");
    le_posCtrl_Release(activationRef);
    LE_INFO("======== Positioning Test finished ========");

    exit(EXIT_SUCCESS);
}
