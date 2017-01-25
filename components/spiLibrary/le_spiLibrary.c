/**
 *
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "le_spiLibrary.h"
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

//--------------------------------------------------------------------------------------------------
/**
 * Configures the SPI bus for use with a specific device.
 */
//--------------------------------------------------------------------------------------------------
void le_spiLib_Configure
(
    int fd,         ///< [in] name of device file. dont pass */dev* prefix
    int mode,       ///< [in] mode options for the bus as defined in spidev.h.
    uint8_t bits,   ///< [in] bits per word, typically 8 bits per word
    uint32_t speed, ///< [in] max speed (Hz), this is slave dependant
    int msb         ///< [in] set as 0 for MSB as first bit or 1 for LSB as first bit
)
{
    LE_DEBUG("Running the configure library call");

    int ret;

    LE_FATAL_IF(
        ((ret = ioctl(fd, SPI_IOC_WR_MODE, &mode)) < 0),
        "SPI modeset failed with error %d: %d (%m)",
        ret,
        errno);
    LE_FATAL_IF(
        ((ret = ioctl(fd, SPI_IOC_RD_MODE, &mode)) < 0),
        "SPI modeget failed with error %d: %d (%m)",
        ret,
        errno);

    LE_FATAL_IF(
        ((ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits)) < 0),
        "SPI bitset failed with error %d : %d (%m)",
        ret,
        errno);
    LE_FATAL_IF(
        ((ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits)) < 0),
        "SPI bitget failed with error %d : %d (%m)",
        ret,
        errno);


    LE_FATAL_IF(
        ((ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed)) < 0),
        "SPI speedset failed with error %d : %d (%m)",
        ret,
        errno);
    LE_FATAL_IF(
        ((ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed)) < 0),
        "SPI speedget failed with error %d : %d (%m)",
        ret,
        errno);

    LE_FATAL_IF(
        ((ret = ioctl(fd, SPI_IOC_WR_LSB_FIRST, &msb)) < 0),
        "SPI MSB/LSB write failed with error %d : %d (%m)",
        ret,
        errno);
    LE_FATAL_IF(
        ((ret = ioctl(fd, SPI_IOC_RD_LSB_FIRST, &msb)) < 0),
        "SPI MSB/LSB read failed  with error %d : %d (%m)",
        ret,
        errno);


    LE_DEBUG("Mode = %d, Speed = %d, Bits = %d, MSB = %d", mode, speed, bits, msb);
}


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
le_result_t le_spiLib_WriteReadHD
(
    int fd,                   ///< [in] open file descriptor of SPI port
    const uint8_t* writeData, ///< [in] tx command/address being sent to slave
    size_t writeDataLength,   ///< [in] number of bytes in tx message
    uint8_t* readData,        ///< [out] rx response from slave
    size_t* readDataLength    ///< [in/out] number of bytes in rx message
)
{
    int transferResult;
    le_result_t result;

    struct spi_ioc_transfer tr[] =
    {
        {
            .tx_buf = (unsigned long)writeData,
            .rx_buf = (unsigned long)NULL,
            .len = writeDataLength,
            // .delay_usecs = delay_out,
            .cs_change = 0
        },
        {
            .tx_buf = (unsigned long)NULL,
            .rx_buf = (unsigned long)readData,
            .len = *readDataLength,
            // .delay_usecs = delay_in,
            .cs_change = 0
        }
    };

    LE_DEBUG("Transmitting this message...len:%zu", writeDataLength);

    for (size_t i = 0; i < writeDataLength; i++)
    {
        LE_DEBUG("%.2X ", writeData[i]);
    }

    transferResult = ioctl(fd, SPI_IOC_MESSAGE(2), tr);

    if (transferResult < 1)
    {
        LE_ERROR("Transfer failed with error %d : %d (%m)", transferResult, errno);
        LE_ERROR("can't send spi message");
        result = LE_FAULT;
    }
    else
    {
        LE_DEBUG("Successful transmission with success %d", transferResult);
        result = LE_OK;
    }

    LE_DEBUG("Received message...");
    for (size_t i = 0; i < *readDataLength; i++)
    {
        LE_DEBUG("%.2X ", readData[i]);
    }

    return result;
}


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
le_result_t le_spiLib_WriteHD
(
    int fd,                   ///< [in] open file descriptor of SPI port
    const uint8_t* writeData, ///< [in] command/address being sent to slave
    size_t writeDataLength    ///< [in] number of bytes in tx message
)
{
    int transferResult;
    le_result_t result;

    struct spi_ioc_transfer tr[] =
    {
        {
            .tx_buf = (unsigned long)writeData,
            .rx_buf = (unsigned long)NULL,
            .len = writeDataLength,
            // .delay_usecs = delay_out,
            .cs_change = 0
        }
    };


    LE_DEBUG("Transferring this message...len: %zu", writeDataLength);
    for (size_t i = 0; i < writeDataLength; i++)
    {
        LE_DEBUG("%.2X ", writeData[i]);
    }

    transferResult = ioctl(fd, SPI_IOC_MESSAGE(1), tr);
    if (transferResult < 1)
    {
        LE_ERROR("Transfer failed with error %d : %d (%m)", transferResult, errno);
        LE_ERROR("can't send spi message");
        result = LE_FAULT;
    }
    else
    {
        LE_DEBUG("%d", transferResult);
        result = LE_OK;
    }

    return result;
}


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
le_result_t le_spiLib_WriteReadFD
(
    int fd,                   ///< [in] open file descriptor of SPI port
    const uint8_t* writeData, ///< [in] tx command/address being sent to slave
    uint8_t* readData,        ///< [out] rx response from slave
    size_t dataLength         ///< [in/out] number of bytes in tx message
)
{
    int transferResult;
    le_result_t result;

    struct spi_ioc_transfer tr[] =
    {
        {
            .tx_buf = (unsigned long)writeData,
            .rx_buf = (unsigned long)readData,
            .len = dataLength,
            // .delay_usecs = delay_out,
            .cs_change = 0
        },
    };

    LE_DEBUG("Transmitting this message...len:%zu", dataLength);

    for (size_t i = 0; i < dataLength; i++)
    {
        LE_DEBUG("%.2X ", writeData[i]);
    }

    transferResult = ioctl(fd, SPI_IOC_MESSAGE(1), tr);

    if (transferResult < 1)
    {
        LE_ERROR("Transfer failed with error %d : %d (%m)", transferResult, errno);

       LE_ERROR("can't send spi message");
        result = LE_FAULT;
    }
    else
    {
        LE_DEBUG("Successful transmission with success %d", transferResult);
        result = LE_OK;
    }

    LE_DEBUG("Received message...");
    for (size_t i = 0; i < dataLength; i++)
    {
        LE_DEBUG("%.2X ", readData[i]);
    }

    return result;
}


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
le_result_t le_spiLib_ReadHD
(
    int fd,                   ///< [in] open file descriptor of SPI port
    uint8_t* readData,        ///< [out] data being sent by slave
    size_t* readDataLength    ///< [in/out] number of bytes in rx message
)
{
    int transferResult;
    le_result_t result;

    struct spi_ioc_transfer tr[] =
    {
        {
            .tx_buf = (unsigned long)NULL,
            .rx_buf = (unsigned long)readData,
            .len = *readDataLength,
            // .delay_usecs = delay_out,
            .cs_change = 0
        }
    };

    transferResult = ioctl(fd, SPI_IOC_MESSAGE(1), tr);
    if (transferResult < 1)
    {
        LE_ERROR("Transfer failed with error %d : %d (%m)", transferResult, errno);
        LE_ERROR("can't receive spi message");
        result = LE_FAULT;
    }
    else
    {
        LE_DEBUG("%d", transferResult);
        result = LE_OK;
    }
    LE_DEBUG("Received message...");
    for (size_t i = 0; i < *readDataLength; i++)
    {
        LE_DEBUG("%.2X ", readData[i]);
    }

    return result;
}


COMPONENT_INIT
{
    LE_DEBUG("spiLibrary initializing");
}
