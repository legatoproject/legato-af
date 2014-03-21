/*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless
 * All rights reserved. Use of this work is subject to license. 
 *******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "le_uart.h"

//#define MY_PORT		"/dev/ttyUSB0"

#define MODULE_NAME "SERIAL_PORT"
#define READ_BUFFER_SIZE 512 // define how much it can read data at a time

#define MY_BAUDRATE	19200
#define MY_FLOWCONTROL	"none"
#define MY_DATA		8
#define MY_PARITY	"odd"
#define MY_TIMEOUT	2

static const char* PAR_NONE  = "none";
static const char* PAR_ODD   = "odd";
static const char* PAR_EVEN  = "even";

/*static int le_uart_flush (uint32_t Handle)
{
    LE_FATAL_IF(Handle==-1,"flush Handle error\n");

    return  tcflush(Handle, TCIOFLUSH);
}*/

static int setParity(const char* parity, struct termios* term)
{
  if (strcmp(parity, PAR_NONE) == 0) {
    term->c_cflag &= ~PARENB;
  }
  else if (strcmp(parity, PAR_ODD) == 0) {
    term->c_cflag |= (PARENB | PARODD);
  }
  else if (strcmp(parity, PAR_EVEN) == 0) {
    term->c_cflag &= ~PARODD;
    term->c_cflag |= PARENB;
  }
  else {
    return -1;
  }

  return 0;
}

/*static const char* getParity(struct termios* term)
{
  if (term->c_cflag & PARENB)
  {
    if (term->c_cflag & PARODD)
      return PAR_ODD;
    else
      return PAR_EVEN;
  }
  else
    return PAR_NONE;
}*/

static const char* FC_NONE    = "none";
static const char* FC_RTSCTS  = "rtscts";
static const char* FC_XONXOFF = "xon/xoff";

static int setFlowControl(const char* fc, struct termios* term)
{
  if (strcmp(fc, FC_RTSCTS) == 0) {
    term->c_cflag |= CRTSCTS;
    term->c_iflag &= ~(IXON | IXOFF);
  }
  else if (strcmp(fc, FC_XONXOFF) == 0) {
    term->c_cflag &= ~CRTSCTS;
    term->c_iflag |= (IXON | IXOFF);
  }
  else if (strcmp(fc, FC_NONE) == 0) {
    term->c_cflag &= ~CRTSCTS;
    term->c_iflag &= ~(IXON | IXOFF);
  }
  else {
    return -1;
  }

  return 0;
}

/*static const char* getFlowControl(struct termios* term)
{
  if (term->c_cflag & CRTSCTS)
  {
    if (term->c_iflag & IXON || term->c_iflag & IXOFF)
      printf("SERIAL: IXON/IXOFF flags are set in conjunction of CRTSCTS flag. Not harmfull but may be a bug. Check serial config !\n");
    return FC_RTSCTS;
  }
  else if (term->c_iflag & IXON || term->c_iflag & IXOFF)
    return FC_XONXOFF;
  else
    return FC_NONE;
}*/

static int setData(unsigned int data, struct termios* term)
{
  term->c_cflag &= ~CSIZE;

  switch (data)
  {
    case 5:
      term->c_cflag |= CS5;
      break;
    case 6:
      term->c_cflag |= CS6;
      break;
    case 7:
      term->c_cflag |= CS7;
      break;
    case 8:
      term->c_cflag |= CS8;
      break;
    default:
      return -1;
  }
  return 0;
}

/*static int getData(struct termios* term)
{
  int data;
  switch (term->c_cflag & CSIZE)
  {
    case CS5:
      data = 5;
      break;
    case CS6:
      data = 6;
      break;
    case CS7:
      data = 7;
      break;
    case CS8:
      data = 8;
      break;
    default:
      data = 0;
      break;
  }
  return data;
}*/


static int setStopBit(unsigned int stop_bit, struct termios* term)
{
  if (stop_bit == 1) {
    term->c_cflag &= ~CSTOPB;
  }
  else if (stop_bit == 2) {
    term->c_cflag |= CSTOPB;
  }
  else {
    return -1;
  }

  return 0;
}

/*static int getStopBit(struct termios* term)
{
  if (term->c_cflag & CSTOPB)
    return 2;
  else
    return 1;
}*/


static int setBaudrate(unsigned int baudrate, struct termios* term)
{
  switch (baudrate)
  {
    case 1200:
    {
      cfsetspeed(term, B1200);
      break;
    }
    case 9600:
    {
      cfsetspeed(term, B9600);
      break;
    }
    case 19200:
    {
      cfsetspeed(term, B19200);
      break;
    }
    case 38400:
    {
      cfsetspeed(term, B38400);
      break;
    }
    case 57600:
    {
      cfsetspeed(term, B57600);
      break;
    }
    case 115200:
    {
      cfsetspeed(term, B115200);
      break;
    }
    default:
      printf("SERIAL: unsupported baudrate: %d\n", baudrate);
      return -1;
  }
  return 0;
}

/*static int getBaudrate(struct termios* term)
{
  speed_t s = cfgetispeed(term);
  if (s != cfgetospeed(term))
    printf("SERIAL: Input and Output speed are different. Check the serial configuration !\n");

  switch(s)
  {
    case B1200:
      s = 1200;
      break;
    case B9600:
      s = 9600;
      break;
    case B19200:
      s = 19200;
      break;
    case B38400:
      s = 38400;
      break;
    case B57600:
      s = 57600;
      break;
    case B115200:
      s = 115200;
      break;
    default:
      printf("SERIAL: Unknown speed setting: %d!\n", s);
      s = 0;
      break;
  }
  return s;
}*/

/*
 * TODO add flags and mode maybe
 */
uint32_t le_uart_open(const char * port)
{
  //const char* port = MY_PORT;

  int fd = open(port, O_RDWR | O_NOCTTY | O_NONBLOCK);
  LE_FATAL_IF(fd==-1,"open -%s- error %s\n",port,strerror(errno));

  le_uart_defaultConfig(fd);

  return fd;
}

void le_uart_defaultConfig(int fd)
{
    struct termios term;
    bzero(&term, sizeof(term));
    cfmakeraw(&term);
    term.c_cflag |= CREAD;

    // Default config
    tcgetattr(fd, &term);
    setParity("none", &term);
    setFlowControl("none", &term);
    setData(8, &term);
    setStopBit(1, &term);
    setBaudrate(115200, &term);

    term.c_iflag &= ~ICRNL;
    term.c_iflag &= ~INLCR;
    term.c_iflag |= IGNBRK;

    term.c_oflag &= ~OCRNL;
    term.c_oflag &= ~ONLCR;
    term.c_oflag &= ~OPOST;

    term.c_lflag &= ~ICANON;
    term.c_lflag &= ~ISIG;
    term.c_lflag &= ~IEXTEN;
    term.c_lflag &= ~(ECHO|ECHOE|ECHOK|ECHONL|ECHOCTL|ECHOPRT|ECHOKE);

    tcsetattr(fd, TCSANOW, &term);


    tcflush(fd, TCIOFLUSH);
}

#define LE_UART_WRITE_MAX_SZ          64

int le_uart_write (uint32_t Handle, const char* buf, uint32_t buf_len)
{
    size_t size;
    size_t size_to_wr;
    ssize_t size_written;

    LE_FATAL_IF(Handle==-1,"Write Handle error\n");

    for(size = 0; size < buf_len;) {
        size_to_wr = buf_len - size;
        if( size_to_wr > LE_UART_WRITE_MAX_SZ)
            size_to_wr = LE_UART_WRITE_MAX_SZ;

        size_written = write(Handle, &buf[size], size_to_wr);

        if (size_written==-1) {
            LE_WARN("Cannot write on uart: %s",strerror(errno));
            return 0;
        }

        LE_DEBUG("Uart Write: %zd", size_written);
        size += size_written;

        usleep(5000); // TODO: Fix ICC performance on telecom side problem

        if(size_written != size_to_wr)
            break;
    }

    return size;
}

int le_uart_read (uint32_t Handle, char* buf, uint32_t buf_len)
{
    LE_FATAL_IF(Handle==-1,"Read Handle error\n");

    return read(Handle, buf, buf_len);
}

int le_uart_ioctl   (uint32_t Handle, uint32_t Cmd, void* pParam )
{
    LE_FATAL_IF(Handle==-1,"ioctl Handle error\n");

    return ioctl(Handle, Cmd, pParam);
}

int le_uart_close (uint32_t Handle)
{
    LE_FATAL_IF(Handle==-1,"close Handle error\n");

    return close(Handle);
}
