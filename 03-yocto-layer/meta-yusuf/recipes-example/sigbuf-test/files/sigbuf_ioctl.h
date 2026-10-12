/* SPDX-License-Identifier: GPL-2.0 */
#ifndef SIGBUF_IOCTL_H
#define SIGBUF_IOCTL_H

#include <linux/ioctl.h>

#define SIGBUF_IOC_MAGIC 'S'

#define SIGBUF_RESET        _IO(SIGBUF_IOC_MAGIC, 0)
#define SIGBUF_GET_STORED   _IOR(SIGBUF_IOC_MAGIC, 1, unsigned int)
#define SIGBUF_GET_CAPACITY _IOR(SIGBUF_IOC_MAGIC, 2, unsigned int)

#define SIGBUF_IOC_MAXNR 2

#endif /* SIGBUF_IOCTL_H */
