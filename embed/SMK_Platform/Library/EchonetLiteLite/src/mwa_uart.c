/*****************************************************************************/
/* ECHONET Lite Lite                                                         */
/*                                                                           */
/* (c) Copyright 2014, SMK Corporation. All rights reserved.                 */
/*****************************************************************************/

#include "ell_adapter.h"

//=============================================================================
void MWA_SetupUART(MWA_UARTHandle_t *uart, void *device)
{
    /*** Platform Dependent ***/
#ifndef __FREESCALE_MQX__
    uart->fd = -1;
    uart->dev_file = device;
#else
    uart->fd = NULL;
    uart->dev_file = device;
#endif
}

//=============================================================================
bool_t MWA_OpenUART(MWA_UARTHandle_t *uart, int baudrate)
{
#ifndef __FREESCALE_MQX__
    char *path;
    struct termios newtio;

    if (uart == NULL) return (FALSE);

    // ------------------------------------------- Try to Close Before Open ---
    MWA_CloseUART(uart);

    uart->baudrate = 0;

    // ---------------------------------------------------------- Open UART ---
    path = (char *)uart->dev_file;
    uart->fd = open(path, O_RDWR | O_NONBLOCK | O_NOCTTY);
    if (uart->fd < 0) {
        return (FALSE);
    }

    // ------------------------------------------ Set Baudrate, Parity Even ---
#if defined(__LINUX__)
    ioctl(uart->fd, TCGETS, &uart->oldtio);
#elif defined(__CYGWIN__)
    ioctl(uart->fd, TCGETA, &uart->oldtio);
#elif defined(__APPLE__)
    ioctl(uart->fd, TIOCGETA, &uart->oldtio);
#else
#error Unsupported OS
#endif

    newtio = uart->oldtio;

#if defined(__LINUX__)
    newtio.c_cflag = baudrate | CS8 | CREAD | PARENB;
#elif defined(__CYGWIN__)
    newtio.c_cflag = CS8 | CREAD | PARENB;
    cfsetispeed(&newtio, baudrate);
    cfsetospeed(&newtio, baudrate);
#elif defined(__APPLE__)
    newtio.c_cflag = CS8 | CREAD | PARENB;
    cfsetspeed(&newtio, baudrate);
#else
#error Unsupported OS
#endif

    // newtio.c_cflag |= CRTSCTS;

    newtio.c_lflag = 0;
    newtio.c_iflag = 0; // IGNBRK; // IGNPAR;
    newtio.c_oflag = 0;
    // newtio.c_cc[VTIME] = 0;
    // newtio.c_cc[VMIN] = 1;

#if defined(__LINUX__)
    ioctl(uart->fd, TCSETS, &newtio);
#elif defined(__CYGWIN__)
    ioctl(uart->fd, TCSETA, &newtio);
#elif defined(__APPLE__)
    ioctl(uart->fd, TIOCSETA, &newtio);
#else
#error Unsupported OS
#endif

    uart->baudrate = baudrate;

    ELL_Printf("UART : %dbps\n", baudrate);

    return (TRUE);
#else // __FREESCALE_MQX__
    uint32_t err;
    uint32_t param;

    if (uart == NULL) return (FALSE);

    // ------------------------------------------- Try to Close Before Open ---
    MWA_CloseUART(uart);

    // ---------------------------------------------------------- Open UART ---
    uart->fd = fopen(uart->dev_file, (void *)IO_SERIAL_NON_BLOCKING);
    if (uart->fd == NULL) {
        return (FALSE);
    }

    // ------------------------------------------------------- Set Baudrate ---
    param = (uint32_t)baudrate;
    err = ioctl(uart->fd, IO_IOCTL_SERIAL_SET_BAUD, &param);
    if (err != MQX_OK) {
        fclose(uart->fd);
        uart->fd = NULL;
        return (FALSE);
    }

    // ---------------------------------------------------- Set Parity Even ---
    param = IO_SERIAL_PARITY_EVEN;
    err = ioctl(uart->fd, IO_IOCTL_SERIAL_SET_PARITY, &param);
    if (err != MQX_OK) {
        fclose(uart->fd);
        uart->fd = NULL;
        return (FALSE);
    }

    uart->baudrate = baudrate;

    ELL_Printf("UART : %dbps\n", baudrate);

    return (TRUE);
#endif
}

//=============================================================================
bool_t MWA_CloseUART(MWA_UARTHandle_t *uart)
{
#ifndef __FREESCALE_MQX__
    if (uart == NULL) return (FALSE);

    if (uart->fd >= 0) {
#if defined(__LINUX__)
        ioctl(uart->fd, TCSETS, &uart->oldtio);
#elif defined(__CYGWIN__)
        ioctl(uart->fd, TCSETA, &uart->oldtio);
#elif defined(__APPLE__)
        ioctl(uart->fd, TIOCSETA, &uart->oldtio);
#else
#error Unsupported OS
#endif

        close(uart->fd);
    }

    return (TRUE);
#else // __FREESCALE_MQX__
    if (uart == NULL) return (FALSE);

    if (uart->fd != NULL) {
        fclose(uart->fd);
        uart->fd = NULL;
    }
#endif

    return (TRUE);
}

//=============================================================================
int MWA_WriteUART(MWA_UARTHandle_t *uart, const uint8_t *buf, int len)
{
    int num_bytes;

    if (buf == NULL || len == 0) return (0);

#ifndef __FREESCALE_MQX__
    if (uart->fd < 0) return (0);

    num_bytes = write(uart->fd, buf, len);
#else
    if (uart->fd == NULL) return (0);

    num_bytes = write(uart->fd, (void *)buf, len);
#endif

    return (num_bytes);
}

//=============================================================================
int MWA_ReadUART(MWA_UARTHandle_t *uart, uint8_t *buf, int max)
{
    int num_bytes;

    if (buf == NULL || max == 0) return (0);

#ifndef __FREESCALE_MQX__
    if (uart->fd < 0) return (0);
#else
    if (uart->fd == NULL) return (0);
#endif

    num_bytes = read(uart->fd, buf, max);

    return (num_bytes);
}

/******************************** END-OF-FILE ********************************/
