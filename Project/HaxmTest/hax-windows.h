/*
** Copyright (c) 2011, Intel Corporation
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

#ifndef __HAX_WINDOWS_H
#define __HAX_WINDOWS_H

#include <windows.h>
#include <windef.h>
#include <stdio.h>

#define HAX_INVALID_FD INVALID_HANDLE_VALUE

static void hax_mod_close(struct hax_state *hax)
{
	CloseHandle(hax->fd);
}

static void hax_close_fd(hax_fd fd)
{
	CloseHandle(fd);
}

static int hax_invalid_fd(hax_fd fd)
{
	return (fd == INVALID_HANDLE_VALUE) || (fd == NULL);
}

#define HAX_DEVICE_TYPE 0x4000

/* See comments for the ioctl in hax-darwin.h */
#define HAX_IOCTL_VERSION       CTL_CODE(HAX_DEVICE_TYPE, 0x900, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define HAX_IOCTL_CREATE_VM     CTL_CODE(HAX_DEVICE_TYPE, 0x901, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define HAX_IOCTL_CAPABILITY    CTL_CODE(HAX_DEVICE_TYPE, 0x910, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define HAX_VM_IOCTL_VCPU_CREATE   CTL_CODE(HAX_DEVICE_TYPE, 0x902, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define HAX_VM_IOCTL_ALLOC_RAM     CTL_CODE(HAX_DEVICE_TYPE, 0x903, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define HAX_VM_IOCTL_SET_RAM       CTL_CODE(HAX_DEVICE_TYPE, 0x904, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define HAX_VCPU_IOCTL_RUN      CTL_CODE(HAX_DEVICE_TYPE, 0x906, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define HAX_VCPU_IOCTL_SET_MSRS CTL_CODE(HAX_DEVICE_TYPE, 0x907, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define HAX_VCPU_IOCTL_GET_MSRS CTL_CODE(HAX_DEVICE_TYPE, 0x908, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define HAX_VCPU_IOCTL_SET_FPU  CTL_CODE(HAX_DEVICE_TYPE, 0x909, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define HAX_VCPU_IOCTL_GET_FPU  CTL_CODE(HAX_DEVICE_TYPE, 0x90a, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define HAX_VCPU_IOCTL_SETUP_TUNNEL      CTL_CODE(HAX_DEVICE_TYPE, 0x90b, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define HAX_VCPU_IOCTL_INTERRUPT        CTL_CODE(HAX_DEVICE_TYPE, 0x90c, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define HAX_VCPU_SET_REGS               CTL_CODE(HAX_DEVICE_TYPE, 0x90d, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define HAX_VCPU_GET_REGS               CTL_CODE(HAX_DEVICE_TYPE, 0x90e, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define HAX_VM_IOCTL_NOTIFY_QEMU_VERSION CTL_CODE(HAX_DEVICE_TYPE, 0x910, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define HAX_IOCTL_VCPU_DEBUG \
        CTL_CODE(HAX_DEVICE_TYPE, 0x916, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define HAX_VCPU_IOCTL_VA2GPA CTL_CODE(HAX_DEVICE_TYPE, 0x920, METHOD_BUFFERED, FILE_ANY_ACCESS)

#endif