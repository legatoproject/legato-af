#include "legato.h"
#include "pa.h"
#include "pa_sim.h"


static const char PIN1[] = "1234";
static const char PUK1[] = "11111111";

static const char NEW_PIN1[] = "54321";

static le_event_Id_t SimInsertEvent;


static void TestIccid(void)
{
    pa_sim_CardId_t iccid;
    LE_ASSERT(pa_sim_GetCardIdentification(iccid) == LE_OK);
    LE_INFO("ICCID is: %s", iccid);
}


static void TestImsi(void)
{
    pa_sim_Imsi_t imsi;
    LE_ASSERT(pa_sim_GetIMSI(imsi) == LE_OK);
    LE_INFO("IMSI is: %s", imsi);
}


static void TestState(le_sim_States_t expectedState)
{
    le_sim_States_t state;
    
    LE_ASSERT(pa_sim_GetState(&state) == LE_OK);
    
    LE_FATAL_IF(state != expectedState, 
                "Card state is %d but should be %d", state, expectedState);
                
    LE_INFO("Card state is: %d which is correct.", state);
}


static void TestEnterPin(void)
{
    LE_ASSERT(pa_sim_EnterPIN(PA_SIM_PIN, PIN1) == LE_OK);    
    LE_INFO("Pin verified");
}


static void TestPinStatus(void)
{
    uint32_t pinRetriesLeft;
    uint32_t pukRetriesLeft;
    uint32_t retriesLeft;

    LE_ASSERT(pa_sim_GetPINRemainingAttempts(PA_SIM_PIN, &pinRetriesLeft) == LE_OK);
    LE_INFO("PIN1 retries left: %d", pinRetriesLeft);

    LE_ASSERT(pa_sim_GetPUKRemainingAttempts(PA_SIM_PUK, &pukRetriesLeft) == LE_OK);
    LE_INFO("PUK1 retries left: %d", pukRetriesLeft);

    // block the pin by repeated entering the wrong pin.
    int i;
    for (i = 0; i <= pinRetriesLeft; i++)
    {
        LE_ASSERT(pa_sim_EnterPIN(PA_SIM_PIN, "12345") != LE_OK);

        LE_ASSERT(pa_sim_GetPINRemainingAttempts(PA_SIM_PIN, &retriesLeft) == LE_OK);
        LE_INFO("PIN1 retries left: %d", retriesLeft);
    }
    
    TestState(LE_SIM_BLOCKED);

    // attempt unblock with wrong puk.
    LE_ASSERT(pa_sim_EnterPUK(PA_SIM_PUK,  "21111111", PIN1) != LE_OK);
    LE_ASSERT(pa_sim_GetPUKRemainingAttempts(PA_SIM_PUK, &retriesLeft) == LE_OK);
    LE_INFO("PUK1 retries left: %d", retriesLeft);
    LE_ASSERT(retriesLeft == pukRetriesLeft-1);

    // unblock the pin with the right puk.
    LE_ASSERT(pa_sim_EnterPUK(PA_SIM_PUK,  PUK1, PIN1) == LE_OK);
    LE_INFO("Pin unblocked");
    
    TestState(LE_SIM_READY);

    LE_ASSERT(pa_sim_GetPINRemainingAttempts(PA_SIM_PIN, &retriesLeft) == LE_OK);
    LE_INFO("PIN1 retries left: %d", retriesLeft);
    LE_ASSERT(pa_sim_GetPUKRemainingAttempts(PA_SIM_PUK, &retriesLeft) == LE_OK);
    LE_INFO("PUK1 retries left: %d", retriesLeft);
}


static void TestChangePin(void)
{
    uint32_t retriesLeft;

    LE_ASSERT(pa_sim_ChangePIN(PA_SIM_PIN, PIN1, NEW_PIN1) == LE_OK);
    LE_INFO("Changed Pin");

    LE_ASSERT(pa_sim_GetPINRemainingAttempts(PA_SIM_PIN, &retriesLeft) == LE_OK);
    LE_INFO("PIN1 retries left: %d", retriesLeft);

    // Change pin back.
    LE_ASSERT(pa_sim_ChangePIN(PA_SIM_PIN, NEW_PIN1, PIN1) == LE_OK);
    LE_INFO("Changed Pin back");

    LE_ASSERT(pa_sim_GetPINRemainingAttempts(PA_SIM_PIN, &retriesLeft) == LE_OK);
    LE_INFO("PIN1 retries left: %d", retriesLeft);
}

static void TestEnablePin(void)
{
    LE_ASSERT(pa_sim_EnablePIN(PA_SIM_PIN, PIN1) == LE_OK);
    LE_INFO("PIN1 enabled");
}


static void TestDisablePin(void)
{
    LE_ASSERT(pa_sim_DisablePIN(PA_SIM_PIN, PIN1) == LE_OK);
    LE_INFO("PIN1 disabled");
}


static void SimStateHandler(pa_sim_Event_t* simStatePtr)
{
    static bool testStarted = false;
    
    LE_INFO("SIM state handler called.  Card number: %d, State: %d", simStatePtr->num, simStatePtr->state);

    if ( (!testStarted) && (simStatePtr->state == LE_SIM_INSERTED) )
    {
        testStarted = true;
        
        // Report the event so that the test can get started.
        le_event_Report(SimInsertEvent, NULL, 0);
    }

    le_mem_Release(simStatePtr);
}

static void SimReadyHandler(void* reportPtr)
{
    // Pin is assumed to be enabled on startup and set to PIN1.
    TestState(LE_SIM_INSERTED);
    
    TestEnterPin();
    TestState(LE_SIM_READY);
    
    TestIccid();
    TestImsi();
    
    TestPinStatus();
    TestChangePin();
    
    TestDisablePin();
    TestEnablePin();

    LE_INFO("======== Completed SIM Platform Adapter's QMI implementation Test (PASSED)  ========");
    exit(EXIT_SUCCESS);
}


COMPONENT_INIT
{
    LE_INFO("======== Begin SIM Platform Adapter's QMI implementation Test  ========");

    // Create this event and handler to perform the actual test.  The event report is not
    // generated until the card is inserted.
    SimInsertEvent = le_event_CreateId("SimTest", 0);
    le_event_AddHandler("SimInsert", SimInsertEvent, SimReadyHandler);

    pa_Init();

    pa_sim_AddNewStateHandler(SimStateHandler);

    LE_INFO("The testing does not start until the state has changed to ready.")
    LE_INFO("Insert (re-insert) the test SIM to start the testing.");
}
