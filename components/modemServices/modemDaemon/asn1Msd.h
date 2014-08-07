/** @file le_ms.h
 *
 * This file contains the declarations of the ASN1 MSD builder.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. Use of this work is subject to license.
 */

#ifndef LEGATO_ASN1_MSD_INCLUDE_GUARD
#define LEGATO_ASN1_MSD_INCLUDE_GUARD

#include "legato.h"


//--------------------------------------------------------------------------------------------------
// Symbols and enums.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 *  Enumeration used to specify the type of vehicle.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    MSD_VEHICLE_PASSENGER_M1=1,  ///< Passenger vehicle (Class M1)
    MSD_VEHICLE_BUS_M2,          ///< Buses and coaches (Class M2)
    MSD_VEHICLE_BUS_M3,          ///< Buses and coaches (Class M3)
    MSD_VEHICLE_COMMERCIAL_N1,   ///< Light commercial vehicles (Class N1)
    MSD_VEHICLE_HEAVY_N2,        ///< Heavy duty vehicles (Class N2)
    MSD_VEHICLE_HEAVY_N3,        ///< Heavy duty vehicles (Class N3)
    MSD_VEHICLE_MOTORCYCLE_L1E,  ///< Motorcycles (Class L1e)
    MSD_VEHICLE_MOTORCYCLE_L2E,  ///< Motorcycles (Class L2e)
    MSD_VEHICLE_MOTORCYCLE_L3E,  ///< Motorcycles (Class L3e)
    MSD_VEHICLE_MOTORCYCLE_L4E,  ///< Motorcycles (Class L4e)
    MSD_VEHICLE_MOTORCYCLE_L5E,  ///< Motorcycles (Class L5e)
    MSD_VEHICLE_MOTORCYCLE_L6E,  ///< Motorcycles (Class L6e)
    MSD_VEHICLE_MOTORCYCLE_L7E,  ///< Motorcycles (Class L7e)
}
msd_VehicleType_t;

//--------------------------------------------------------------------------------------------------
// Data structures.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Data structure describing the VIN (Vehicle Identification Number).
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
   char    isowmi[3];
   char    isovds[6];
   char    isovisModelyear[1];
   char    isovisSeqPlant[7];
} msd_Vin_t;


/* ControlType */
//--------------------------------------------------------------------------------------------------
/**
 * Data structure describing the main control elements of the MSD.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
   bool                 automaticActivation;
   bool                 testCall;
   bool                 positionCanBeTrusted;
   msd_VehicleType_t    vehType;
} msd_Control_t;

//--------------------------------------------------------------------------------------------------
/**
 * Data structure describing the vehicle propulsion storage type.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
   bool gasolineTankPresent;
   bool dieselTankPresent;
   bool compressedNaturalGas;
   bool liquidPropaneGas;
   bool electricEnergyStorage;
   bool hydrogenStorage;
} msd_VehiclePropulsionStorageType_t;

//--------------------------------------------------------------------------------------------------
/**
 * Data structure describing the last known vehicle location.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
   int32_t    latitude;
   int32_t    longitude;
} msd_VehicleLocation_t;

//--------------------------------------------------------------------------------------------------
/**
 * Data structure describing the location of the vehicle some time before the generation of the data
 * for the MSD message..
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
   int32_t    latitudeDelta;
   int32_t    longitudeDelta;
} msd_VehicleLocationDelta_t;

//--------------------------------------------------------------------------------------------------
/**
 * Data structure to gather optional data.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
   uint8_t  oidlen;
   uint8_t  dataLen;
   uint8_t* oid;
   uint8_t* data;
}msd_optionalData_t;

//--------------------------------------------------------------------------------------------------
/**
 * Data structure describing the MSD without the additional optional data.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
   uint8_t                             messageIdentifier;
   msd_Control_t                       control;
   msd_Vin_t                           vehIdentificationNumber;
   msd_VehiclePropulsionStorageType_t  vehPropulsionStorageType;
   uint32_t                            timestamp;
   msd_VehicleLocation_t               vehLocation;
   uint8_t                             vehDirection;
   /* Optional */
   bool                                recentVehLocationN1Pres;
   msd_VehicleLocationDelta_t          recentVehLocationN1;
   /* Optional */
   bool                                recentVehLocationN2Pres;
   msd_VehicleLocationDelta_t          recentVehLocationN2;
   /* Optional */
   bool                                numberOfPassengersPres;
   uint8_t                             numberOfPassengers;
} msd_Structure_t;

//--------------------------------------------------------------------------------------------------
/**
 * Data structure describing the MSD with the additional optional data.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
   msd_Structure_t    msdStruct;
   /* Optional */
   bool               optionalDataPres;
   msd_optionalData_t optionalData;
}msd_Message_t;

//--------------------------------------------------------------------------------------------------
/**
 * Data structure describing the MSD message.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
    uint8_t       version;
    msd_Message_t msdMsg;
} msd_t;

/* ERA Glonass specific types for the msd_optionalData_t parts */
//--------------------------------------------------------------------------------------------------
/**
 * Data structure to gather the ERA Glonass specific data.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
   /*12-A1 ID  integer 1 Version of format of additional data of MSD is set to "1".*/
   uint8_t ID;

   /** 12-A2 -BOOLEAN
    * TRUE – significant probability of danger to life and health of people in the vehicle cab;
    * FALSE – no significant probability of danger to life and health of people in the vehicle cab.*/
   bool SevereCrashEstimation;

   bool testResultsDefPres;
   struct
   {
      bool micConnectionFailure;
      bool micFailure;
      bool rightSpeakerFailure;
      bool leftSpeakerFailure;
      bool speakersFailure;
      bool ignitionLineFailure;
      bool uimFailure;
      bool statusIndicatorFailure;
      bool batteryFailure;
      bool batteryVoltageLow;
      bool crashSensorFailure;
      bool swImageCorruption;
      bool commModuleInterfaceFailure;
      bool gnssReceiverFailure;
      bool raimProblem;
      bool gnssAntennaFailure;
      bool commModuleFailure;
      bool eventsMemoryOverflow;
      bool crashProfileMemoryOverflow;
      bool otherCriticalFailires;
      bool otherNotCriticalFailures;
   }testResultsDef;

   bool crashTypePres;
   struct
   {
      bool crashFront;
      bool crashSide;
      bool crashFrontOrSide;
      bool crashRear;
      bool crashRollover;
      bool crashAnotherType;
   }crashType;
}msd_EraGlonassData_t;


//--------------------------------------------------------------------------------------------------
/**
 * This function encodes a data buffer from the elements of the ERA Glonass additional data
 * structure.
 *
 * @return the data buffer length in bits
 */
//--------------------------------------------------------------------------------------------------
int32_t msd_EncodeOptionalDataForEraGlonass
(
    msd_EraGlonassData_t* eraGlonassDataPtr,  ///< [IN] ERA Glonass additional data
    uint8_t*              outDataPtr          ///< [OUT] encoded data buffer (allocated by the
                                              ///  calling function must be minimum 140 Bytes)
);

//--------------------------------------------------------------------------------------------------
/**
 * This function encodes the MSD message from the elements of the MSD data structure
 *
 * @return the MSD message length in bytes on success
 * @return LE_FAULT on failure
 *
 * @note Only MSD version 1 is supported.
 */
//--------------------------------------------------------------------------------------------------
int32_t msd_EncodeMsdMessage
(
    msd_t*      msdDataPtr, ///< [IN] MSD data
    uint8_t*    outDataPtr  ///< [OUT] encoded MSD message
);

#endif // LEGATO_ASN1_MSD_INCLUDE_GUARD
