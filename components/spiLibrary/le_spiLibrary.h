/**
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LE_SPI_LIBRARY_H
#define LE_SPI_LIBRARY_H

//--------------------------------------------------------------------------------------------------
/**
 * Configures the SPI bus for use with a specific device.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void le_spiLib_Configure
(
    int fd,         ///< [in] name of device file. dont pass */dev* prefix
    int mode,       ///< [in] mode options for the bus as defined in spidev.h.
    uint8_t bits,   ///< [in] bits per word, typically 8 bits per word
    uint32_t speed, ///< [in] max speed (Hz), this is slave dependant
    int msb         ///< [in] set as 0 for MSB as first bit or 1 for LSB as first bit
);

/**-----------------------------------------------------------------------------------------------
 * Performs SPI WriteRead Half Duplex. You can send send Read command/ address of data to read.
 *
 * @return
 *      - LE_OK
 *      - LE_FAULT
 *
 * @note
 *      Some devices do not support this mode. Check the data sheet of the device.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_spiLib_WriteReadHD
(
    int fd,                   ///< [in] open file descriptor of SPI port
    const uint8_t* writeData, ///< [in] tx command/address being sent to slave
    size_t writeDataLength,   ///< [in] number of bytes in tx message
    uint8_t* readData,        ///< [out] rx response from slave
    size_t* readDataLength    ///< [in/out] number of bytes in rx message
);

//--------------------------------------------------------------------------------------------------
/**
 * Performs SPI Write Half Duplex.  You can send send Write command/ address of data to write/data
 * to write.
 *
 * @return
 *      - LE_OK
 *      - LE_FAULT
 *
 * @note
 *      Some devices do not support this mode. Check the data sheet of the device.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_spiLib_WriteHD
(
    int fd,                   ///< [in] open file descriptor of SPI port
    const uint8_t* writeData, ///< [in] command/address being sent to slave
    size_t writeDataLength    ///< [in] number of bytes in tx message
);

/**-----------------------------------------------------------------------------------------------
 * Performs SPI WriteRead Full Duplex. You can send send Read command/ address of data to read.
 *
 * @return
 *      - LE_OK
 *      - LE_FAULT
 *
 * @note
 *      Some devices do not support this mode. Check the data sheet of the device.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_spiLib_WriteReadFD
(
    int fd,                   ///< [in]  open file descriptor of SPI port
    const uint8_t* writeData, ///< [in] tx command/address being sent to slave
    uint8_t* readData,        ///< [in] rx response from slave
    size_t dataLength         ///< [in/out] number of bytes in tx message
);

//--------------------------------------------------------------------------------------------------
/**
 * Performs SPI Read.  You can send send Write command/ address of data to write/data to write.
 *
 * @return
 *      - LE_OK
 *      - LE_FAULT
 *
 * @note
 *      Some devices do not support this mode. Check the data sheet of the device.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_spiLib_ReadHD
(
    int fd,                   ///< [in] open file descriptor of SPI port
    uint8_t* readData,        ///< [out] data being sent by slave
    size_t* readDataLength    ///< [in/out] number of bytes in rx message
);

#endif  // LE_SPI_LIBRARY_H
