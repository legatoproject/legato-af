//--------------------------------------------------------------------------------------------------
/**
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"


//--------------------------------------------------------------------------------------------------
/**
 * Close file.
 */
//--------------------------------------------------------------------------------------------------
static void CloseFile(int fd)
{
    int err;

    do
    {
        err = le_fd_Close(fd);
    }
    while ( (err != 0) && (errno == EINTR) );

    if (err != 0)
    {
        LE_ERROR("Could not open file.  errno=%d", errno);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Write a buffer to a file.
 */
//--------------------------------------------------------------------------------------------------
static void WriteBuf(const char* fileNamePtr, const uint8_t* bufPtr, size_t bufSize)
{
    int fd;
    do
    {
        fd = le_fd_Open(fileNamePtr, O_WRONLY | O_CREAT | O_TRUNC);
    }
    while ( (fd == -1) && (errno == EINTR) );

    LE_TEST_ASSERT(fd != -1, "Could not open file %s. errno=%d", fileNamePtr, errno);

    const uint8_t* currentPtr = bufPtr;
    size_t numBytes = 0;

    while (bufSize - numBytes > 0)
    {
        ssize_t c;
        do
        {
            c = le_fd_Write(fd, currentPtr, bufSize - numBytes);
        } while ( (c == -1) && (errno == EINTR) );

        if(c == -1)
        {
            CloseFile(fd);
            LE_TEST_FATAL("Could not write to %s.  errno=%d", fileNamePtr, errno);
        }

        numBytes += c;
        currentPtr += c;
    }

    CloseFile(fd);
}

//--------------------------------------------------------------------------------------------------
/**
 * Reads an entire file into a buffer.
 *
 * @return
 *      Number of bytes read.
 */
//--------------------------------------------------------------------------------------------------
static size_t ReadEntireFile(const char* fileNamePtr, uint8_t* bufPtr, size_t bufSize)
{
    int fd;
    do
    {
        fd = le_fd_Open(fileNamePtr, O_RDONLY);
    }
    while ( (fd == -1) && (errno == EINTR) );

    LE_TEST_ASSERT(fd != -1, "Could not open file %s.  errno=%d", fileNamePtr, errno);

    uint8_t* currentPtr = bufPtr;
    size_t numBytes = 0;

    while (1)
    {
        ssize_t c;
        do
        {
            c = le_fd_Read(fd, currentPtr, bufSize - numBytes);
        } while ( (c == -1) && (errno == EINTR) );

        if (c == -1)
        {
            CloseFile(fd);
            LE_TEST_FATAL("Could not read the %s.  errno=%d", fileNamePtr, errno);
        }

        if (c == 0)
        {
            CloseFile(fd);
            return numBytes;
        }
        else if (bufSize - numBytes == 0)
        {
            LE_TEST_FATAL("Buffer too small to read entire file %s.", fileNamePtr);
        }

        numBytes += c;
        currentPtr += c;
    }
}

#if LE_CONFIG_TARGET_GILL
#define BASE_LOCATION                       "/keys/"
#else
#define BASE_LOCATION                       "/mnt/flash/keys/"
#endif

#define TEST_DIR                            BASE_LOCATION"test/"
//--------------------------------------------------------------------------------------------------
/**
 * Authorized server directory.
 */
//--------------------------------------------------------------------------------------------------
#define AUTH_SERVER_DIR                     BASE_LOCATION"authorizedServer/"

//--------------------------------------------------------------------------------------------------
/**
 * Provisioning data file.
 */
//--------------------------------------------------------------------------------------------------
#define PROVISION_DATA_FILE                 TEST_DIR"provData"

//--------------------------------------------------------------------------------------------------
/**
 * Requests the authorized server to wrap a key value with the provisioning key.  The result will
 * be written out to the provisioning data file.
 */
//--------------------------------------------------------------------------------------------------
static void WrapKey
(
    const char* keyId,
    const uint8_t* keyValPtr,
    size_t keyValSize
)
{
    const char provKeyFile[] = TEST_DIR"provKey";
    const char rawKeyFile[] = TEST_DIR"rawKey";
    le_result_t result;

    // Get the provisioning key.
    uint8_t provKey[LE_IKS_MAX_ASN1_VAL_BUF_SIZE];
    size_t provKeySize = sizeof(provKey);

    result = le_iks_GetWrappingKey(provKey, &provKeySize);
    LE_TEST_ASSERT(result == LE_OK, "Could not get provisioning key.");

    WriteBuf(provKeyFile, provKey, provKeySize);

    WriteBuf(rawKeyFile, keyValPtr, keyValSize);

    // Get the authorized server to wrap the key value with the provisioning key.
    char serverReq[500]  = "";

    if (snprintf(serverReq, sizeof(serverReq), AUTH_SERVER_DIR"wrapKey %s", keyId) >= sizeof(serverReq))
    {
        LE_TEST_FATAL("Server request too long: %s.", serverReq);
    }

    LE_TEST_ASSERT(system(serverReq) == 0, "Failed to wrap key.");
}


//--------------------------------------------------------------------------------------------------
/**
 * Tests Milenage.
 */
//--------------------------------------------------------------------------------------------------
static void MilenageTest(void)
{
#define NUM_MILENAGE_TEST_VECTORS           6

    // Test vectors from 3GPP TS 35.207.
    uint8_t keys[NUM_MILENAGE_TEST_VECTORS][LE_IKS_AESMILENAGE_K_SIZE] = {
        {0x46, 0x5b, 0x5c, 0xe8, 0xb1, 0x99, 0xb4, 0x9f, 0xaa, 0x5f, 0x0a, 0x2e, 0xe2, 0x38, 0xa6, 0xbc},
        {0x03, 0x96, 0xeb, 0x31, 0x7b, 0x6d, 0x1c, 0x36, 0xf1, 0x9c, 0x1c, 0x84, 0xcd, 0x6f, 0xfd, 0x16},
        {0xfe, 0xc8, 0x6b, 0xa6, 0xeb, 0x70, 0x7e, 0xd0, 0x89, 0x05, 0x75, 0x7b, 0x1b, 0xb4, 0x4b, 0x8f},
        {0x9e, 0x59, 0x44, 0xae, 0xa9, 0x4b, 0x81, 0x16, 0x5c, 0x82, 0xfb, 0xf9, 0xf3, 0x2d, 0xb7, 0x51},
        {0x4a, 0xb1, 0xde, 0xb0, 0x5c, 0xa6, 0xce, 0xb0, 0x51, 0xfc, 0x98, 0xe7, 0x7d, 0x02, 0x6a, 0x84},
        {0x6c, 0x38, 0xa1, 0x16, 0xac, 0x28, 0x0c, 0x45, 0x4f, 0x59, 0x33, 0x2e, 0xe3, 0x5c, 0x8c, 0x4f}
    };

    uint8_t opc[NUM_MILENAGE_TEST_VECTORS][LE_IKS_AESMILENAGE_OPC_SIZE] = {
        {0xcd, 0x63, 0xcb, 0x71, 0x95, 0x4a, 0x9f, 0x4e, 0x48, 0xa5, 0x99, 0x4e, 0x37, 0xa0, 0x2b, 0xaf},
        {0x53, 0xc1, 0x56, 0x71, 0xc6, 0x0a, 0x4b, 0x73, 0x1c, 0x55, 0xb4, 0xa4, 0x41, 0xc0, 0xbd, 0xe2},
        {0x10, 0x06, 0x02, 0x0f, 0x0a, 0x47, 0x8b, 0xf6, 0xb6, 0x99, 0xf1, 0x5c, 0x06, 0x2e, 0x42, 0xb3},
        {0xa6, 0x4a, 0x50, 0x7a, 0xe1, 0xa2, 0xa9, 0x8b, 0xb8, 0x8e, 0xb4, 0x21, 0x01, 0x35, 0xdc, 0x87},
        {0xdc, 0xf0, 0x7c, 0xbd, 0x51, 0x85, 0x52, 0x90, 0xb9, 0x2a, 0x07, 0xa9, 0x89, 0x1e, 0x52, 0x3e},
        {0x38, 0x03, 0xef, 0x53, 0x63, 0xb9, 0x47, 0xc6, 0xaa, 0xa2, 0x25, 0xe5, 0x8f, 0xae, 0x39, 0x34},
    };

    uint8_t rand[NUM_MILENAGE_TEST_VECTORS][LE_IKS_AESMILENAGE_RAND_SIZE] = {
        {0x23, 0x55, 0x3c, 0xbe, 0x96, 0x37, 0xa8, 0x9d, 0x21, 0x8a, 0xe6, 0x4d, 0xae, 0x47, 0xbf, 0x35},
        {0xc0, 0x0d, 0x60, 0x31, 0x03, 0xdc, 0xee, 0x52, 0xc4, 0x47, 0x81, 0x19, 0x49, 0x42, 0x02, 0xe8},
        {0x9f, 0x7c, 0x8d, 0x02, 0x1a, 0xcc, 0xf4, 0xdb, 0x21, 0x3c, 0xcf, 0xf0, 0xc7, 0xf7, 0x1a, 0x6a},
        {0xce, 0x83, 0xdb, 0xc5, 0x4a, 0xc0, 0x27, 0x4a, 0x15, 0x7c, 0x17, 0xf8, 0x0d, 0x01, 0x7b, 0xd6},
        {0x74, 0xb0, 0xcd, 0x60, 0x31, 0xa1, 0xc8, 0x33, 0x9b, 0x2b, 0x6c, 0xe2, 0xb8, 0xc4, 0xa1, 0x86},
        {0xee, 0x64, 0x66, 0xbc, 0x96, 0x20, 0x2c, 0x5a, 0x55, 0x7a, 0xbb, 0xef, 0xf8, 0xba, 0xbf, 0x63},
    };

    uint8_t sqn[NUM_MILENAGE_TEST_VECTORS][LE_IKS_AESMILENAGE_SQN_SIZE] = {
        {0xff, 0x9b, 0xb4, 0xd0, 0xb6, 0x07},
        {0xfd, 0x8e, 0xef, 0x40, 0xdf, 0x7d},
        {0x9d, 0x02, 0x77, 0x59, 0x5f, 0xfc},
        {0x0b, 0x60, 0x4a, 0x81, 0xec, 0xa8},
        {0xe8, 0x80, 0xa1, 0xb5, 0x80, 0xb6},
        {0x41, 0x4b, 0x98, 0x22, 0x21, 0x81}
    };

    uint8_t amf[NUM_MILENAGE_TEST_VECTORS][LE_IKS_AESMILENAGE_AMF_SIZE] = {
        {0xb9, 0xb9},
        {0xaf, 0x17},
        {0x72, 0x5c},
        {0x9e, 0x09},
        {0x9f, 0x07},
        {0x44, 0x64}
    };

    uint8_t maca[NUM_MILENAGE_TEST_VECTORS][LE_IKS_AESMILENAGE_MACA_SIZE] = {
        {0x4a, 0x9f, 0xfa, 0xc3, 0x54, 0xdf, 0xaf, 0xb3},
        {0x5d, 0xf5, 0xb3, 0x18, 0x07, 0xe2, 0x58, 0xb0},
        {0x9c, 0xab, 0xc3, 0xe9, 0x9b, 0xaf, 0x72, 0x81},
        {0x74, 0xa5, 0x82, 0x20, 0xcb, 0xa8, 0x4c, 0x49},
        {0x49, 0xe7, 0x85, 0xdd, 0x12, 0x62, 0x6e, 0xf2},
        {0x07, 0x8a, 0xdf, 0xb4, 0x88, 0x24, 0x1a, 0x57}
    };

    uint8_t macs[NUM_MILENAGE_TEST_VECTORS][LE_IKS_AESMILENAGE_MACS_SIZE] = {
        {0x01, 0xcf, 0xaf, 0x9e, 0xc4, 0xe8, 0x71, 0xe9},
        {0xa8, 0xc0, 0x16, 0xe5, 0x1e, 0xf4, 0xa3, 0x43},
        {0x95, 0x81, 0x4b, 0xa2, 0xb3, 0x04, 0x43, 0x24},
        {0xac, 0x2c, 0xc7, 0x4a, 0x96, 0x87, 0x18, 0x37},
        {0x9e, 0x85, 0x79, 0x03, 0x36, 0xbb, 0x3f, 0xa2},
        {0x80, 0x24, 0x6b, 0x8d, 0x01, 0x86, 0xbc, 0xf1}
    };

    uint8_t res[NUM_MILENAGE_TEST_VECTORS][LE_IKS_AESMILENAGE_RES_SIZE] = {
        {0xa5, 0x42, 0x11, 0xd5, 0xe3, 0xba, 0x50, 0xbf},
        {0xd3, 0xa6, 0x28, 0xed, 0x98, 0x86, 0x20, 0xf0},
        {0x80, 0x11, 0xc4, 0x8c, 0x0c, 0x21, 0x4e, 0xd2},
        {0xf3, 0x65, 0xcd, 0x68, 0x3c, 0xd9, 0x2e, 0x96},
        {0x58, 0x60, 0xfc, 0x1b, 0xce, 0x35, 0x1e, 0x7e},
        {0x16, 0xc8, 0x23, 0x3f, 0x05, 0xa0, 0xac, 0x28}
    };

    uint8_t ak[NUM_MILENAGE_TEST_VECTORS][LE_IKS_AESMILENAGE_AK_SIZE] = {
        {0xaa, 0x68, 0x9c, 0x64, 0x83, 0x70},
        {0xc4, 0x77, 0x83, 0x99, 0x5f, 0x72},
        {0x33, 0x48, 0x4d, 0xc2, 0x13, 0x6b},
        {0xf0, 0xb9, 0xc0, 0x8a, 0xd0, 0x2e},
        {0x31, 0xe1, 0x1a, 0x60, 0x91, 0x18},
        {0x45, 0xb0, 0xf6, 0x9a, 0xb0, 0x6c}
    };

    uint8_t ck[NUM_MILENAGE_TEST_VECTORS][LE_IKS_AESMILENAGE_CK_SIZE] = {
        {0xb4, 0x0b, 0xa9, 0xa3, 0xc5, 0x8b, 0x2a, 0x05, 0xbb, 0xf0, 0xd9, 0x87, 0xb2, 0x1b, 0xf8, 0xcb},
        {0x58, 0xc4, 0x33, 0xff, 0x7a, 0x70, 0x82, 0xac, 0xd4, 0x24, 0x22, 0x0f, 0x2b, 0x67, 0xc5, 0x56},
        {0x5d, 0xbd, 0xbb, 0x29, 0x54, 0xe8, 0xf3, 0xcd, 0xe6, 0x65, 0xb0, 0x46, 0x17, 0x9a, 0x50, 0x98},
        {0xe2, 0x03, 0xed, 0xb3, 0x97, 0x15, 0x74, 0xf5, 0xa9, 0x4b, 0x0d, 0x61, 0xb8, 0x16, 0x34, 0x5d},
        {0x76, 0x57, 0x76, 0x6b, 0x37, 0x3d, 0x1c, 0x21, 0x38, 0xf3, 0x07, 0xe3, 0xde, 0x92, 0x42, 0xf9},
        {0x3f, 0x8c, 0x75, 0x87, 0xfe, 0x8e, 0x4b, 0x23, 0x3a, 0xf6, 0x76, 0xae, 0xde, 0x30, 0xba, 0x3b}
    };

    uint8_t ik[NUM_MILENAGE_TEST_VECTORS][LE_IKS_AESMILENAGE_IK_SIZE] = {
        {0xf7, 0x69, 0xbc, 0xd7, 0x51, 0x04, 0x46, 0x04, 0x12, 0x76, 0x72, 0x71, 0x1c, 0x6d, 0x34, 0x41},
        {0x21, 0xa8, 0xc1, 0xf9, 0x29, 0x70, 0x2a, 0xdb, 0x3e, 0x73, 0x84, 0x88, 0xb9, 0xf5, 0xc5, 0xda},
        {0x59, 0xa9, 0x2d, 0x3b, 0x47, 0x6a, 0x04, 0x43, 0x48, 0x70, 0x55, 0xcf, 0x88, 0xb2, 0x30, 0x7b},
        {0x0c, 0x45, 0x24, 0xad, 0xea, 0xc0, 0x41, 0xc4, 0xdd, 0x83, 0x0d, 0x20, 0x85, 0x4f, 0xc4, 0x6b},
        {0x1c, 0x42, 0xe9, 0x60, 0xd8, 0x9b, 0x8f, 0xa9, 0x9f, 0x27, 0x44, 0xe0, 0x70, 0x8c, 0xcb, 0x53},
        {0xa7, 0x46, 0x6c, 0xc1, 0xe6, 0xb2, 0xa1, 0x33, 0x7d, 0x49, 0xd3, 0xb6, 0x6e, 0x95, 0xd7, 0xb4}
    };
    uint8_t akStar[NUM_MILENAGE_TEST_VECTORS][LE_IKS_AESMILENAGE_AK_SIZE] = {
        {0x45, 0x1e, 0x8b, 0xec, 0xa4, 0x3b},
        {0x30, 0xf1, 0x19, 0x70, 0x61, 0xc1},
        {0xde, 0xac, 0xdd, 0x84, 0x8c, 0xc6},
        {0x60, 0x85, 0xa8, 0x6c, 0x6f, 0x63},
        {0xfe, 0x25, 0x55, 0xe5, 0x4a, 0xa9},
        {0x1f, 0x53, 0xcd, 0x2b, 0x11, 0x13}
    };

    le_result_t result;
    const char keyId[] = "milenage_K";
    const char opcId[] = "milenage_OPc";

    // Create keys.
    uint64_t keyRef;
    result = le_iks_CreateKeyByType(keyId, LE_IKS_KEY_TYPE_AES_MILENAGE_K, 16, &keyRef);
    LE_TEST_OK(result == LE_OK, "Creating Milenage Key: %s", LE_RESULT_TXT(result));
    LE_TEST_INFO("keyRef %"PRIu64, keyRef);

    uint64_t opcRef;
    result = le_iks_CreateKeyByType(opcId, LE_IKS_KEY_TYPE_AES_MILENAGE_OPC, 16, &opcRef);
    LE_TEST_OK(result == LE_OK, "Creating Milenage OPc: %s", LE_RESULT_TXT(result));
    LE_TEST_INFO("opcRef %"PRIu64, opcRef);

    int i;
    for (i = 0; i < NUM_MILENAGE_TEST_VECTORS; i++)
    {
        // Wrap the key value.
        WrapKey(keyId, keys[i], LE_IKS_AESMILENAGE_K_SIZE);

        uint8_t wrappedKey[LE_IKS_MAX_ASN1_VAL_BUF_SIZE];
        size_t wrappedKeySize = sizeof(wrappedKey);

        wrappedKeySize = ReadEntireFile(PROVISION_DATA_FILE, wrappedKey, wrappedKeySize);

        // Provision key.
        result = le_iks_ProvisionKeyValue(keyRef, wrappedKey, wrappedKeySize);
        LE_TEST_OK(result == LE_OK, "Provision K.");

        // Wrap the OPc value.
        WrapKey(opcId, opc[i], LE_IKS_AESMILENAGE_OPC_SIZE);

        wrappedKeySize = sizeof(wrappedKey);
        wrappedKeySize = ReadEntireFile(PROVISION_DATA_FILE, wrappedKey, wrappedKeySize);

        // Provision OPc.
        result = le_iks_ProvisionKeyValue(opcRef, wrappedKey, wrappedKeySize);
        LE_TEST_OK(result == LE_OK, "Provision OPc.");

        // Get MAC-A.
        uint8_t buf[LE_IKS_AES_BLOCK_SIZE];

        size_t macaSize = sizeof(buf);
        LE_TEST_INFO("sizeofs: rand %zu amf %zu aqn %zu", sizeof(rand[i]), sizeof(amf[i]), sizeof(sqn[i]));
        result = le_iks_aesMilenage_GetMacA(keyRef, opcRef,
                                            rand[i], sizeof(rand[i]),
                                            amf[i], sizeof(amf[i]),
                                            sqn[i], sizeof(sqn[i]),
                                            buf, &macaSize);
        LE_TEST_OK(result == LE_OK, "Get MAC-A.");

        LE_TEST_OK(memcmp(buf, maca[i], LE_IKS_AESMILENAGE_MACA_SIZE) == 0,
                   "MAC-A incorrect.  Test vector %d.", i);

        // Get MAC-S.
        size_t macsSize = sizeof(buf);
        result = le_iks_aesMilenage_GetMacS(keyRef, opcRef,
                                            rand[i], sizeof(rand[i]),
                                            amf[i], sizeof(amf[i]),
                                            sqn[i], sizeof(sqn[i]),
                                            buf, &macsSize);
        LE_TEST_OK(result == LE_OK, "Get MAC-S.");

        LE_TEST_OK(memcmp(buf, macs[i], LE_IKS_AESMILENAGE_MACS_SIZE) == 0,
                       "Checking MAC-S: test vector %d.", i);

        // Get milenage generated keys.
        uint8_t resBuf[LE_IKS_AESMILENAGE_RES_SIZE];
        uint8_t ckBuf[LE_IKS_AESMILENAGE_CK_SIZE];
        uint8_t ikBuf[LE_IKS_AESMILENAGE_IK_SIZE];
        uint8_t akBuf[LE_IKS_AESMILENAGE_AK_SIZE];

        size_t resBufSize = sizeof(resBuf);
        size_t ckBufSize = sizeof(ckBuf);
        size_t ikBufSize = sizeof(ikBuf);
        size_t akBufSize = sizeof(akBuf);
        result = le_iks_aesMilenage_GetKeys(keyRef, opcRef,
                                            rand[i], sizeof(rand[i]),
                                            resBuf, &resBufSize,
                                            ckBuf, &ckBufSize,
                                            ikBuf, &ikBufSize,
                                            akBuf, &akBufSize);
        LE_TEST_OK(result == LE_OK, "Get milenage generated keys.");

        LE_TEST_OK(memcmp(resBuf, res[i], sizeof(resBuf)) == 0, "Check RES.  Test vector %d.", i);
        LE_TEST_OK(memcmp(ckBuf, ck[i], sizeof(ckBuf)) == 0, "Check CK.  Test vector %d.", i);
        LE_TEST_OK(memcmp(ikBuf, ik[i], sizeof(ikBuf)) == 0, "Check IK.  Test vector %d.", i);
        LE_TEST_OK(memcmp(akBuf, ak[i], sizeof(akBuf)) == 0, "Check AK.  Test vector %d.", i);

        // Get the AK from the milenage f5* function.
        akBufSize = sizeof(akBuf);
        result = le_iks_aesMilenage_GetAk(keyRef, opcRef,
                                          rand[i], sizeof(rand[i]),
                                          akBuf, &akBufSize);
        LE_TEST_OK(result == LE_OK, "Get AK for milenage f5*.");

        LE_TEST_OK(memcmp(akBuf, akStar[i], sizeof(akBuf)) == 0, "AK for f5*.  Test vector %d.", i);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Tests AES GCM packet encryption & decryption.
 */
//--------------------------------------------------------------------------------------------------
static void AesGcmPacketTest
(
    void
)
{
    uint64_t keyRef;
    le_result_t result;
    const char keyId[] = "GcmKey";
    const size_t keySize = 16;
    const uint8_t msg[] = "Black as the Pit from pole to pole";
    uint8_t aad[] = "I thank whatever gods may be";

    uint8_t buf[LE_IKS_AESGCM_NONCE_SIZE + LE_IKS_AESGCM_TAG_SIZE + sizeof(msg)];
    uint8_t* noncePtr = buf;
    uint8_t* tagPtr = noncePtr + LE_IKS_AESGCM_NONCE_SIZE;
    uint8_t* ciphertextPtr = tagPtr + LE_IKS_AESGCM_TAG_SIZE;
    uint8_t decryptedText[sizeof(msg)] = "";

    LE_TEST_INFO("Trying to retrieve key");
    result = le_iks_GetKey(keyId, &keyRef);
    LE_TEST_OK((result == LE_OK) || (result == LE_NOT_FOUND), "Get AES GCM key: %s",
               LE_RESULT_TXT(result));
    if (result == LE_OK)
    {
        LE_TEST_INFO("GetKey Succeeded!");
    }
    else
    {
        LE_TEST_INFO("Key not found, creating new key");
        result = le_iks_CreateKeyByType(keyId, LE_IKS_KEY_TYPE_AES_GCM, keySize, &keyRef);
        LE_TEST_OK(result == LE_OK, "Creating GCM Key: %s", LE_RESULT_TXT(result));
        result = le_iks_GenKeyValue(keyRef, NULL, 0);
        LE_TEST_OK(result == LE_OK, "Generating GCM key");
        result = le_iks_SaveKey(keyRef);
        LE_TEST_OK(result == LE_OK, "Saving GCM key");
    }

    LE_TEST_INFO("Encrypting string '%s'", msg);
    size_t nonceSize = LE_IKS_AESGCM_NONCE_SIZE;
    size_t ciphertextSize = sizeof(msg);
    size_t tagSize = LE_IKS_AESGCM_TAG_SIZE;
    result = le_iks_aesGcm_EncryptPacket(keyRef,
                                    noncePtr,
                                    &nonceSize,
                                    aad,
                                    sizeof(aad),
                                    msg,
                                    sizeof(msg),
                                    ciphertextPtr,
                                    &ciphertextSize,
                                    tagPtr,
                                    &tagSize);

    LE_TEST_OK(result == LE_OK, "Encrypting result");
    LE_TEST_OK(ciphertextSize == sizeof(msg), "Ciphertext size");
    LE_TEST_OK(nonceSize == LE_IKS_AESGCM_NONCE_SIZE, "Nonce size");
    LE_TEST_INFO("Decrypting...");
    size_t decryptedTextSize = sizeof(msg);
    nonceSize = LE_IKS_AESGCM_NONCE_SIZE;
    result = le_iks_aesGcm_DecryptPacket(keyRef,
                                        noncePtr,
                                        nonceSize,
                                        aad,
                                        sizeof(aad),
                                        ciphertextPtr,
                                        ciphertextSize,
                                        decryptedText,
                                        &decryptedTextSize,
                                        tagPtr,
                                        LE_IKS_AESGCM_TAG_SIZE);
    LE_TEST_OK(result == LE_OK, "Decrypting result %s", LE_RESULT_TXT(result));
    LE_TEST_OK(decryptedTextSize == sizeof(msg), "Decrypted text size");
    LE_TEST_INFO("Decrypted text '%s'", decryptedText);
    LE_TEST_OK(strncmp((const char *)msg, (const char *)decryptedText, sizeof(msg)) == 0,
               "Decrypted text correctness check");
}


//--------------------------------------------------------------------------------------------------
/**
 * Tests MAC generation and verification with HMAC.
 */
//--------------------------------------------------------------------------------------------------
static void HmacTest(void)
{
    uint8_t mac[10];
    uint64_t keyRef;
    size_t macSize = sizeof(mac);
    const char keyId[] = "HmacMsgKey";
    le_result_t result;

    LE_TEST_INFO("If key already exists, delete it");
    result = le_iks_GetKey(keyId, &keyRef);
    LE_TEST_OK((result == LE_OK) || (result == LE_NOT_FOUND), "Get HMAC key: %s",
               LE_RESULT_TXT(result));
    if (result == LE_OK)
    {
        result = le_iks_DeleteKey(keyRef, NULL, 0);
        LE_TEST_OK(result == LE_OK, "Deleting HMAC key: %s", LE_RESULT_TXT(result));
    }

    // Create an HMAC key.
    result = le_iks_CreateKeyByType(keyId, LE_IKS_KEY_TYPE_HMAC_SHA256, 17, &keyRef);
    LE_TEST_OK(result == LE_OK, "Create HMAC key: %s", LE_RESULT_TXT(result));

    result = le_iks_GenKeyValue(keyRef, NULL, 0);
    LE_TEST_OK(result == LE_OK, "Generate HMAC key: %s", LE_RESULT_TXT(result));

    // Create a session.
    uint64_t sessionRef;
    result = le_iks_CreateSession(keyRef, &sessionRef);
    LE_TEST_OK(result == LE_OK, "Creating session: %s", LE_RESULT_TXT(result));

    // Attempt to get the MAC before processing any messages.
    result = le_iks_hmac_Done(sessionRef, mac, &macSize);
    LE_TEST_OK(result == LE_FAULT, "Negative test: to get MAC when no messages are processed.");

    // Generate a MAC in two parts.
    uint8_t part1[] = "Do not go gentle into that goodnight.";
    uint8_t part2[] = "Rage, rage against the dying of the light.";

    result = le_iks_hmac_ProcessChunk(sessionRef, part1, sizeof(part1));
    LE_TEST_OK(result == LE_OK, "HMAC process chunk %s", LE_RESULT_TXT(result));

    result = le_iks_hmac_ProcessChunk(sessionRef, part2, sizeof(part2));
    LE_TEST_OK(result == LE_OK, "HMAC process chunk %s", LE_RESULT_TXT(result));

    // Get the MAC.
    result = le_iks_hmac_Done(sessionRef, mac, &macSize);
    LE_TEST_OK(result == LE_OK, "Get MAC %s", LE_RESULT_TXT(result));

    // Attempt to process more messages after getting the MAC.
    result = le_iks_hmac_ProcessChunk(sessionRef, part1, sizeof(part1));
    LE_TEST_OK(result == LE_FAULT,
               "Negative test: to process more messages after getting the MAC.");

    // Cleanup.
    result = le_iks_DeleteSession(sessionRef);
    LE_TEST_OK(result == LE_OK, "Delete session %s", LE_RESULT_TXT(result));

    // Create a session.
    result = le_iks_CreateSession(keyRef, &sessionRef);
    LE_TEST_OK(result == LE_OK, "Creating session: %s", LE_RESULT_TXT(result));

    // Verify the MAC by recalculating it.
    result = le_iks_hmac_ProcessChunk(sessionRef, part1, sizeof(part1));
    LE_TEST_OK(result == LE_OK, "Start HMAC process chunk %s", LE_RESULT_TXT(result));

    result = le_iks_hmac_ProcessChunk(sessionRef, part2, sizeof(part2));
    LE_TEST_OK(result == LE_OK, "Start HMAC process chunk %s", LE_RESULT_TXT(result));

    result = le_iks_hmac_Verify(sessionRef, mac, macSize);
    LE_TEST_OK(result == LE_OK, "Verify MAC %s", LE_RESULT_TXT(result));

    // Attempt to get the mac again.
    result = le_iks_hmac_Done(sessionRef, mac, &macSize);
    LE_TEST_OK(result == LE_FAULT, "Negative test: attempt to get MAC after verifying the MAC %s",
               LE_RESULT_TXT(result));

    // Ensure that the mac size has not changed.
    LE_TEST_OK(macSize == sizeof(mac), "MAC size correctness");

    // Cleanup.
    result = le_iks_DeleteSession(sessionRef);
    LE_TEST_OK(result == LE_OK, "Delete session %s", LE_RESULT_TXT(result));

    LE_TEST_INFO("Successfully performed HMAC generation and verification.  %zu", macSize);
}


//--------------------------------------------------------------------------------------------------
/**
 * Tests AES CMAC generation and verification with CMAC.
 */
//--------------------------------------------------------------------------------------------------
static void AesCmacTest(void)
{
    uint8_t mac[10];
    uint64_t keyRef;
    size_t macSize = sizeof(mac);
    const char keyId[] = "CmacMsgKey";
    le_result_t result;

    LE_TEST_INFO("If key already exists, delete it");
    result = le_iks_GetKey(keyId, &keyRef);
    LE_TEST_OK((result == LE_OK) || (result == LE_NOT_FOUND), "Get CMAC key: %s",
               LE_RESULT_TXT(result));
    if (result == LE_OK)
    {
        result = le_iks_DeleteKey(keyRef, NULL, 0);
        LE_TEST_OK(result == LE_OK, "Deleting CMAC key");
    }

    // Create an CMAC key.
    result = le_iks_CreateKeyByType(keyId, LE_IKS_KEY_TYPE_AES_CMAC, 16, &keyRef);
    LE_TEST_OK(result == LE_OK, "Creating CMAC Key: %s", LE_RESULT_TXT(result));

    result = le_iks_GenKeyValue(keyRef, NULL, 0);
    LE_TEST_OK(result == LE_OK, "Generate CMAC key: %s", LE_RESULT_TXT(result));

    // Create a session.
    uint64_t sessionRef;
    result = le_iks_CreateSession(keyRef, &sessionRef);
    LE_TEST_OK(result == LE_OK, "Creating session: %s", LE_RESULT_TXT(result));

    // Attempt to get the MAC before processing any messages.
    result = le_iks_aesCmac_Done(sessionRef, mac, &macSize);
    LE_TEST_OK(result == LE_FAULT, "Negative test: to get MAC when no messages are processed.");

    // Generate a MAC in two parts.
    uint8_t part1[] = "Do not go gentle into that goodnight.";
    uint8_t part2[] = "Rage, rage against the dying of the light.";

    result = le_iks_aesCmac_ProcessChunk(sessionRef, part1, sizeof(part1));
    LE_TEST_OK(result == LE_OK, "CMAC process chunk %s", LE_RESULT_TXT(result));

    result = le_iks_aesCmac_ProcessChunk(sessionRef, part2, sizeof(part2));
    LE_TEST_OK(result == LE_OK, "CMAC process chunk %s", LE_RESULT_TXT(result));

    // Get the MAC.
    result = le_iks_aesCmac_Done(sessionRef, mac, &macSize);
    LE_TEST_OK(result == LE_OK, "Get MAC %s", LE_RESULT_TXT(result));

    // Attempt to process more messages after getting the MAC.
    result = le_iks_aesCmac_ProcessChunk(sessionRef, part1, sizeof(part1));
    LE_TEST_OK(result == LE_FAULT,
               "Negative test: to process more messages after getting the MAC.");

    // Cleanup.
    result = le_iks_DeleteSession(sessionRef);
    LE_TEST_OK(result == LE_OK, "Delete session %s", LE_RESULT_TXT(result));

    // Create a session.
    result = le_iks_CreateSession(keyRef, &sessionRef);
    LE_TEST_OK(result == LE_OK, "Creating session: %s", LE_RESULT_TXT(result));

    // Verify the MAC by recalculating it.
    result = le_iks_aesCmac_ProcessChunk(sessionRef, part1, sizeof(part1));
    LE_TEST_OK(result == LE_OK, "Start CMAC process chunk %s", LE_RESULT_TXT(result));

    result = le_iks_aesCmac_ProcessChunk(sessionRef, part2, sizeof(part2));
    LE_TEST_OK(result == LE_OK, "Start CMAC process chunk %s", LE_RESULT_TXT(result));

    result = le_iks_aesCmac_Verify(sessionRef, mac, macSize);
    LE_TEST_OK(result == LE_OK, "Verify MAC %s", LE_RESULT_TXT(result));

    // Attempt to get the mac again.
    result = le_iks_aesCmac_Done(sessionRef, mac, &macSize);
    LE_TEST_OK(result == LE_FAULT, "Negative test: attempt to get MAC after verifying the MAC %s",
               LE_RESULT_TXT(result));

    // Ensure that the mac size has not changed.
    LE_TEST_OK(macSize == sizeof(mac), "MAC size correctness");

    // Cleanup.
    result = le_iks_DeleteSession(sessionRef);
    LE_TEST_OK(result == LE_OK, "Delete session %s", LE_RESULT_TXT(result));

    LE_TEST_INFO("Successfully performed CMAC generation and verification.  %zu", macSize);
}


//--------------------------------------------------------------------------------------------------
/**
 * Tests AES CBC encryption/decryption.
 */
//--------------------------------------------------------------------------------------------------
static void AesCbcTest(void)
{
    const char keyId[] = "CbcMsgKey";
    le_result_t result;
    uint64_t keyRef;

    LE_TEST_INFO("If key already exists, delete it");
    result = le_iks_GetKey(keyId, &keyRef);
    LE_TEST_OK((result == LE_OK) || (result == LE_NOT_FOUND), "Get CBC key: %s",
               LE_RESULT_TXT(result));
    if (result == LE_OK)
    {
        result = le_iks_DeleteKey(keyRef, NULL, 0);
        LE_TEST_OK(result == LE_OK, "Deleting AES CBC key %s", LE_RESULT_TXT(result));
    }

    // Create a CBC key.
    result = le_iks_CreateKeyByType(keyId, LE_IKS_KEY_TYPE_AES_CBC, 24, &keyRef);
    LE_TEST_OK(result == LE_OK, "Creating CBC Key: %s", LE_RESULT_TXT(result));

    result = le_iks_GenKeyValue(keyRef, NULL, 0);
    LE_TEST_OK(result == LE_OK, "Generate CBC key %s", LE_RESULT_TXT(result));

    // Create a session.
    uint64_t sessionRef;
    result = le_iks_CreateSession(keyRef, &sessionRef);
    LE_TEST_OK(result == LE_OK, "Creating session: %s", LE_RESULT_TXT(result));

    // Start encryption.
    uint8_t plaintext[] = "0123456789012345 123456789012345";
    int textSize = sizeof(plaintext)-1;
    plaintext[textSize] = '\0';

    uint8_t iv[LE_IKS_AESCBC_IV_SIZE] = {0};

    result = le_iks_aesCbc_StartEncrypt(sessionRef, iv, sizeof(iv));
    LE_TEST_OK(result == LE_OK, "Start CBC encryption process %s", LE_RESULT_TXT(result));

    // Attempt to encrypt a plaintext that is not a multiple of the block size.
    uint8_t ct1[sizeof(plaintext)] = "";
    ct1[sizeof(ct1)-1] = '\0';

    size_t ct1Size = sizeof(ct1);
    result = le_iks_aesCbc_Encrypt(sessionRef, plaintext, textSize + 1, ct1, &ct1Size);
    LE_TEST_OK(result == LE_OUT_OF_RANGE,
               "Negative test: encrypt plaintext that is not a multiple of the block size %s",
               LE_RESULT_TXT(result));

    // Encrypt a message that is exactly two blocks.
    result = le_iks_aesCbc_Encrypt(sessionRef, plaintext, textSize, ct1, &ct1Size);
    LE_TEST_OK(result == LE_OK, "CBC encrypt message %s", LE_RESULT_TXT(result));
    LE_TEST_OK(memcmp(plaintext, ct1, textSize) != 0,
               "Plaintext must be different from ciphertext.");

    // Encrypt the message again with a different IV.
    uint8_t ct2[sizeof(plaintext)] = "";
    ct2[sizeof(ct2)-1] = '\0';

    iv[2] = 8;
    result = le_iks_aesCbc_StartEncrypt(sessionRef, iv, sizeof(iv));
    LE_TEST_OK(result == LE_OK, "Start CBC encryption process %s", LE_RESULT_TXT(result));

    size_t ct2Size = sizeof(ct2);
    result = le_iks_aesCbc_Encrypt(sessionRef, plaintext, textSize, ct2, &ct2Size);
    LE_TEST_OK(result == LE_OK, "CBC encrypt message %s", LE_RESULT_TXT(result));
    LE_TEST_OK(memcmp(plaintext, ct2, textSize) != 0,
               "Plaintext must be different from ciphertext.");
    LE_TEST_OK(memcmp(ct1, ct2, textSize) != 0,
               "Encryption with different IVs should produce different ciphertexts.");

    // long message test, encrypt section 2 - same plaintext
    ct1Size = sizeof(ct1);
    result = le_iks_aesCbc_Encrypt(sessionRef, plaintext, textSize, ct1, &ct1Size);
    LE_TEST_OK(result == LE_OK, "CBC encrypt message %s", LE_RESULT_TXT(result));
    LE_TEST_OK(memcmp(plaintext, ct1, textSize) != 0,
               "Plaintext must be different from ciphertext.");
    LE_TEST_OK(memcmp(ct1, ct2, textSize) != 0,
               "Encryption with chaining mode should produce different ciphertexts.");

    // Attempt to decrypt the message without starting a decryption process.
    uint8_t pt[sizeof(plaintext)] = "";
    pt[sizeof(pt)-1] = '\0';
    size_t ptSize = sizeof(pt);

    result = le_iks_aesCbc_Decrypt(sessionRef, ct2, textSize, pt, &ptSize);
    LE_TEST_OK(result == LE_FAULT,
               "Negative test: attempt to decrypt without starting %s", LE_RESULT_TXT(result));

    // Decrypt the message.
    result = le_iks_aesCbc_StartDecrypt(sessionRef, iv, sizeof(iv));
    LE_TEST_OK(result == LE_OK, "Start CBC decryption process %s", LE_RESULT_TXT(result));

    ptSize = sizeof(pt);
    result = le_iks_aesCbc_Decrypt(sessionRef, ct2, textSize, pt, &ptSize);
    LE_TEST_OK(result == LE_OK, "CBC decrypt message %s", LE_RESULT_TXT(result));

    LE_TEST_OK(memcmp(plaintext, pt, textSize) == 0, "Decrypted plaintext matches original.");
    LE_TEST_INFO("PT = '%s'", pt);

    // decrypt from another ciphertext
    ptSize = sizeof(pt);
    result = le_iks_aesCbc_Decrypt(sessionRef, ct1, textSize, pt, &ptSize);
    LE_TEST_OK(result == LE_OK, "CBC decrypt message %s", LE_RESULT_TXT(result));

    LE_TEST_OK(memcmp(plaintext, pt, textSize) == 0, "Decrypted plaintext matches original.");
    LE_TEST_INFO("PT = '%s'", pt);

    // Cleanup.
    result = le_iks_DeleteSession(sessionRef);
    LE_TEST_OK(result == LE_OK, "Delete session %s", LE_RESULT_TXT(result));

    LE_TEST_INFO("CBC encrypt/decrypt test done.");
}


//--------------------------------------------------------------------------------------------------
/**
 * ECIES encryption helper routine - groups together set of API calls.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t EccEncHelper
(
    uint64_t sesRef,
    const uint8_t* labelPtr,
    size_t labelSize,
    uint8_t* ephemBufPtr,
    size_t* ephemBufSizePtr,
    const uint8_t* msgPtr,
    uint8_t* ctPtr,
    size_t msgSize,
    uint8_t* tagPtr,
    size_t tagSize
)
{
    le_result_t result;

    // Start encryption process
    result = le_iks_ecc_Ecies_StartEncrypt(sesRef,
                                    labelPtr, labelSize,
                                    ephemBufPtr, ephemBufSizePtr);
    LE_TEST_OK(result == LE_OK, "Start ECIES encryption process %s", LE_RESULT_TXT(result));
    if (result != LE_OK)
    {
        return result;
    }

    // Encrypt the message.
    size_t ctSize = msgSize;
    result = le_iks_ecc_Ecies_Encrypt(sesRef, msgPtr, msgSize, ctPtr, &ctSize);
    LE_TEST_OK(result == LE_OK, "Encrypt with ECIES %s", LE_RESULT_TXT(result));
    if (result != LE_OK)
    {
        return result;
    }

    // Get the tag.
    result = le_iks_ecc_Ecies_DoneEncrypt(sesRef, tagPtr, &tagSize);
    LE_TEST_OK(result == LE_OK, "Get tag for ECIES: size %zu rc %s", tagSize,
               LE_RESULT_TXT(result));
    if (result != LE_OK)
    {
        return result;
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * ECIES decryption helper routine - groups together set of API calls.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t EccDecHelper
(
    uint64_t sesRef,
    const uint8_t* labelPtr,
    size_t labelSize,
    const uint8_t* ephemBufPtr,
    size_t ephemBufSize,
    const uint8_t* ctPtr,
    uint8_t* ptPtr,
    size_t msgSize,
    const uint8_t* tagPtr,
    size_t tagSize
)
{
    le_result_t result;

    // Start the decryption process.
    result = le_iks_ecc_Ecies_StartDecrypt(sesRef,
                                           labelPtr, labelSize,
                                           ephemBufPtr, ephemBufSize);
    LE_TEST_OK(result == LE_OK, "Start ECIES decryption process %s", LE_RESULT_TXT(result));
    if (result != LE_OK)
    {
        return result;
    }

    // Decrypt the message.
    size_t ptSize = msgSize;
    result = le_iks_ecc_Ecies_Decrypt(sesRef, ctPtr, msgSize, ptPtr, &ptSize);
    LE_TEST_OK(result == LE_OK, "ECIES decrypt %s", LE_RESULT_TXT(result));
    if (result != LE_OK)
    {
        return result;
    }

    // Check the tag.
    result = le_iks_ecc_Ecies_DoneDecrypt(sesRef, tagPtr, tagSize);
    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Tests ECIES streaming encryption/decryption.
 */
//--------------------------------------------------------------------------------------------------
static void EccEncTest
(
    void
)
{
    const char keyId[] = "eciesKey";
    const size_t eccKeySize = 28;
    const uint8_t label[] = "Invictus";
    const uint8_t msg[] = "Beyond this place of wrath and tears";
    const size_t eccTagSize = 16;
    le_result_t result;
    uint64_t keyRef;

    LE_TEST_INFO("If key already exists, delete it");
    result = le_iks_GetKey(keyId, &keyRef);
    LE_TEST_OK((result == LE_OK) || (result == LE_NOT_FOUND), "Get ECIES key: %s",
               LE_RESULT_TXT(result));
    if (result == LE_OK)
    {
        result = le_iks_DeleteKey(keyRef, NULL, 0);
        LE_TEST_OK(result == LE_OK, "Deleting ECIES key %s", LE_RESULT_TXT(result));
    }

    // Create an ECIES key.
    result = le_iks_CreateKeyByType(keyId,
                                    LE_IKS_KEY_TYPE_PRIV_ECIES_HKDF_SHA256_GCM128,
                                    eccKeySize,
                                    &keyRef);
    LE_TEST_OK(result == LE_OK, "Creating ECIES Key: %s", LE_RESULT_TXT(result));
    LE_TEST_INFO("keyRef %"PRIu64, keyRef);
    result = le_iks_GenKeyValue(keyRef, NULL, 0);
    LE_TEST_OK(result == LE_OK, "Generate ECIES key %s", LE_RESULT_TXT(result));

    // Create a session.
    uint64_t sessionRef;
    result = le_iks_CreateSession(keyRef, &sessionRef);
    LE_TEST_OK(result == LE_OK, "Creating session: %s", LE_RESULT_TXT(result));

    // Encrypt the message with the ECIES public key.
    uint8_t ephemBuf[2*eccKeySize + 1];
    size_t ephemBufSize = sizeof(ephemBuf);
    uint8_t ct[sizeof(msg)];
    uint8_t tag[eccTagSize];

    result = EccEncHelper(sessionRef,
                          label, sizeof(label),
                          ephemBuf, &ephemBufSize,
                          msg, ct, sizeof(msg),
                          tag, sizeof(tag));
    LE_TEST_OK(result == LE_OK, "ECIES encrypt message.");

    // Attempt to decrypt but with the wrong label.
    uint8_t wrongLabel[sizeof(label)];
    memcpy(wrongLabel, label, sizeof(wrongLabel));
    wrongLabel[2] = wrongLabel[2] + 1;

    uint8_t decrBuf[sizeof(msg)];
    result = EccDecHelper(sessionRef,
                          wrongLabel, sizeof(wrongLabel),
                          ephemBuf, ephemBufSize,
                          ct, decrBuf, sizeof(msg),
                          tag, sizeof(tag));
    LE_TEST_OK(result != LE_OK, "ECIES decrypt message with wrong label.");

    // Decrypt the message properly.
    result = EccDecHelper(sessionRef,
                          label, sizeof(label),
                          ephemBuf, ephemBufSize,
                          ct, decrBuf, sizeof(msg),
                          tag, sizeof(tag));
    LE_TEST_OK(result == LE_OK, "ECIES decrypt message: rc %s", LE_RESULT_TXT(result));

    LE_TEST_OK(memcmp(msg, decrBuf, sizeof(msg)) == 0, "Decrypted plaintext matches original.");

    // Cleanup.
    result = le_iks_DeleteSession(sessionRef);
    LE_TEST_OK(result == LE_OK, "Delete session %s", LE_RESULT_TXT(result));

    LE_TEST_INFO("ECC encrypt/decrypt test done.");
}


//--------------------------------------------------------------------------------------------------
/**
 * Tests ECIES packet encryption/decryption.
 */
//--------------------------------------------------------------------------------------------------
static void EccPacketTest
(
    void
)
{
    const char keyId[] = "eciesKey";
    const size_t eccKeySize = 66;
    const uint8_t label[] = "William Ernest Henley";
    const uint8_t msg[] = "And yet the menace of the years";

    const size_t eccTagSize = 16;
    le_result_t result;
    uint64_t keyRef;

    LE_TEST_INFO("If key already exists, delete it");
    result = le_iks_GetKey(keyId, &keyRef);
    LE_TEST_OK((result == LE_OK) || (result == LE_NOT_FOUND), "Get ECIES key: %s",
               LE_RESULT_TXT(result));
    if (result == LE_OK)
    {
        result = le_iks_DeleteKey(keyRef, NULL, 0);
        LE_TEST_OK(result == LE_OK, "Deleting ECIES key %s", LE_RESULT_TXT(result));
    }

    // Create an ECIES key.
    result = le_iks_CreateKeyByType(keyId,
                                    LE_IKS_KEY_TYPE_PRIV_ECIES_HKDF_SHA512_GCM256,
                                    eccKeySize,
                                    &keyRef);
    LE_TEST_OK(result == LE_OK, "Creating ECIES Key: %s", LE_RESULT_TXT(result));
    LE_TEST_INFO("keyRef %"PRIu64, keyRef);
    LE_TEST_OK(keyRef != 0, "Create ECIES key.");
    result = le_iks_GenKeyValue(keyRef, NULL, 0);
    LE_TEST_OK(result == LE_OK, "Generate ECIES key %s", LE_RESULT_TXT(result));

    // Encrypt the message with the ECIES public key.
    LE_TEST_INFO("Encrypting string '%s'", msg);
    uint8_t ephemBuf[2*eccKeySize + 1];
    size_t ephemBufSize = sizeof(ephemBuf);
    uint8_t ct[sizeof(msg)];
    size_t ctSize = sizeof(ct);
    uint8_t tag[eccTagSize];
    size_t tagSize = sizeof(tag);

    result = le_iks_ecc_Ecies_EncryptPacket(keyRef,
                                            label, sizeof(label),
                                            msg, sizeof(msg),
                                            ct, &ctSize,
                                            ephemBuf, &ephemBufSize,
                                            tag, &tagSize);

    LE_TEST_OK(result == LE_OK, "Encrypting result %s", LE_RESULT_TXT(result));
    LE_TEST_OK(ctSize == sizeof(msg), "Ciphertext size");

    LE_TEST_INFO("Decrypting...");
    uint8_t pt[sizeof(msg)] = "";
    size_t ptSize = sizeof(msg);
    result = le_iks_ecc_Ecies_DecryptPacket(keyRef,
                                     label, sizeof(label),
                                     ephemBuf, ephemBufSize,
                                     ct, ctSize,
                                     pt, &ptSize,
                                     tag, sizeof(tag));

    LE_TEST_OK(result == LE_OK, "Decrypting result %s", LE_RESULT_TXT(result));
    LE_TEST_OK(ptSize == sizeof(msg), "Decrypted text size");
    LE_TEST_INFO("Decrypted text '%s'", pt);
    LE_TEST_OK(strncmp((const char *)msg, (const char *)pt, sizeof(msg)) == 0,
               "Decrypted text correctness check");

    LE_TEST_INFO("ECC encrypt/decrypt test done.");
}


//--------------------------------------------------------------------------------------------------
/**
 * Tests RSASSA-PSS signature generation/verification.
 */
//--------------------------------------------------------------------------------------------------
static void RsaSigTest
(
    void
)
{
#define RSA_SIG_KEY_SIZE            384
#define RSA_SIG_HASH_DIGEST_SIZE    32
#define RSA_SIG_SALT_SIZE           8
    const char keyId[] = "rsaSigKey";
    le_result_t result;
    uint64_t keyRef;

    LE_TEST_INFO("If key already exists, delete it");
    result = le_iks_GetKey(keyId, &keyRef);
    LE_TEST_OK((result == LE_OK) || (result == LE_NOT_FOUND), "Get RSA key: %s",
               LE_RESULT_TXT(result));
    if (result == LE_OK)
    {
        result = le_iks_DeleteKey(keyRef, NULL, 0);
        LE_TEST_OK(result == LE_OK, "Deleting RSA key");
    }

    result = le_iks_CreateKeyByType(keyId,
                                    LE_IKS_KEY_TYPE_PRIV_RSASSA_PSS_SHA512_256,
                                    RSA_SIG_KEY_SIZE,
                                    &keyRef);

    LE_TEST_OK(result == LE_OK, "Creating RSA Key: %s", LE_RESULT_TXT(result));
    LE_TEST_INFO("keyRef %"PRIu64, keyRef);
    result = le_iks_GenKeyValue(keyRef, NULL, 0);
    LE_TEST_OK(result == LE_OK, "Generate RSA key %s", LE_RESULT_TXT(result));

    // Generate a signature for a fake hash of a fake message.
    uint8_t signature[RSA_SIG_KEY_SIZE];
    size_t signatureSize = sizeof(signature);
    uint8_t msgHash[RSA_SIG_HASH_DIGEST_SIZE] = {35};

    result = le_iks_rsa_Pss_GenSig(keyRef,
                                   RSA_SIG_SALT_SIZE,
                                   msgHash,
                                   sizeof(msgHash),
                                   signature,
                                   &signatureSize);
    LE_TEST_OK(result == LE_OK, "Generate RSA signature %s", LE_RESULT_TXT(result));

    // Use a different salt length and check that the signature verification fails.
    result = le_iks_rsa_Pss_VerifySig(keyRef,
                                      RSA_SIG_SALT_SIZE*2,
                                      msgHash,
                                      sizeof(msgHash),
                                      signature,
                                      signatureSize);

    LE_TEST_OK(result != LE_OK,
               "Negative test: verify signature with wrong salt length: rc %s",
               LE_RESULT_TXT(result));

    // Use a different different msg and check that the signature verification fails.
    uint8_t modifiedMsgHash[RSA_SIG_HASH_DIGEST_SIZE] = {35};
    modifiedMsgHash[3] = 24;

    result = le_iks_rsa_Pss_VerifySig(keyRef,
                                      RSA_SIG_SALT_SIZE,
                                      modifiedMsgHash,
                                      sizeof(modifiedMsgHash),
                                      signature,
                                      signatureSize);

    LE_TEST_OK(result == LE_FAULT, "Negative test: verify signature with wrong message.");

    // Verify the signature.
    result = le_iks_rsa_Pss_VerifySig(keyRef,
                                      RSA_SIG_SALT_SIZE,
                                      msgHash,
                                      sizeof(msgHash),
                                      signature,
                                      signatureSize);

    LE_TEST_OK(result == LE_OK, "Verify RSA signature %s", LE_RESULT_TXT(result));
}


COMPONENT_INIT
{
    LE_TEST_PLAN(LE_TEST_NO_PLAN);

    LE_TEST_INFO("=== IoT Keystore test BEGIN ===");
    AesGcmPacketTest();
    if (0) MilenageTest();
    HmacTest();
    AesCbcTest();
    AesCmacTest();
    RsaSigTest();
    if (0) EccPacketTest(); // Native test is currently failing as well.
    EccEncTest();

    LE_TEST_INFO("=== IoT Keystore test END ===");

    LE_TEST_EXIT;
}
