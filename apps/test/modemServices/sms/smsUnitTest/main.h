/*
 * main.h

 */

#ifndef APPS_TEST_MODEMSERVICES_SMS_SMSUNITTEST_MAIN_H_
#define APPS_TEST_MODEMSERVICES_SMS_SMSUNITTEST_MAIN_H_


//--------------------------------------------------------------------------------------------------
/**
 * Test sequence Structure list
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*TestFunc)(void);

typedef struct
{
        char * name;
        TestFunc ptrfunc;
} my_struct;


//--------------------------------------------------------------------------------------------------
/*
 * SMS PDU encoding and decoding test
 *
 */
//--------------------------------------------------------------------------------------------------
void testle_sms_SmsPduTest
(
    void
);


//--------------------------------------------------------------------------------------------------
/*
 * CDMA SMS PDU encoding and decoding test
 *
 */
//--------------------------------------------------------------------------------------------------
void testle_sms_CdmaPduTest
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Dump the PDU
 */
//--------------------------------------------------------------------------------------------------
void DumpPdu
(
    const char      *labelStr,     ///< [IN] label
    const uint8_t   *bufferPtr,    ///< [IN] buffer
    size_t           bufferSize    ///< [IN] buffer size
);

//--------------------------------------------------------------------------------------------------
/**
 * SMS API Unitary Test
 */
//--------------------------------------------------------------------------------------------------
void testle_sms_SmsApiUnitTest
(
    void
);


#endif /* APPS_TEST_MODEMSERVICES_SMS_SMSUNITTEST_MAIN_H_ */
