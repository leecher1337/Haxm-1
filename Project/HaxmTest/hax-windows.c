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

#include "hax-i386.h"

int valid_hax_tunnel_size(uint32_t size)
{
	return size >= sizeof(struct hax_tunnel);
}

/*
 * return 0 upon success, -1 when the driver is not loaded,
 * other negative value for other failures
 */
int hax_open_device(hax_fd *fd)
{
    uint32_t errNum = 0;
    HANDLE hDevice;

    if (!fd)
        return -2;

    hDevice = CreateFile( "\\\\.\\HAX",
      GENERIC_READ | GENERIC_WRITE,
      0,
      NULL,
      CREATE_ALWAYS,
      FILE_ATTRIBUTE_NORMAL,
      NULL);

    if (hDevice == INVALID_HANDLE_VALUE)
    {
        printf("Failed to open the HAX device!\n");
        errNum = GetLastError();
        if (errNum == ERROR_FILE_NOT_FOUND)
            return -1;
        return -2;
    }
    *fd = hDevice;
    printf("device fd:%d\n", *fd);
    return 0;
}


hax_fd hax_mod_open(void)
{
    int ret;
    hax_fd fd;

    ret = hax_open_device(&fd);
    if (ret != 0)
        printf("Open HAX device failed\n");

    return fd;
}

int hax_populate_ram(struct hax_vm *vm, uint64_t va, uint32_t size)
{
    int ret;
    struct hax_alloc_ram_info info;
    HANDLE hDeviceVM;
    DWORD dSize = 0;

    info.size = size;
    info.va = va;

    hDeviceVM = vm->fd;

    ret = DeviceIoControl(hDeviceVM,
      HAX_VM_IOCTL_ALLOC_RAM,
      &info, sizeof(info),
      NULL, 0,
      &dSize,
      (LPOVERLAPPED) NULL);

    if (!ret) {
        printf("Failed to allocate %x memory\n", size);
        return ret;
    }

    return 0;
}


int hax_set_phys_mem(struct hax_state *hax, target_phys_addr_t start_addr, ram_addr_t size, ram_addr_t phys_offset)
{
    struct hax_set_ram_info info, *pinfo = &info;
    HANDLE hDeviceVM;
    DWORD dSize = 0;
    int ret = 0;

    info.pa_start = start_addr;
    info.size = size;
    info.va = phys_offset;
    info.flags = 0;

    ret = DeviceIoControl(hax->vm->fd,
      HAX_VM_IOCTL_SET_RAM,
      pinfo, sizeof(*pinfo),
      NULL, 0,
      &dSize,
      (LPOVERLAPPED) NULL);

    if (!ret)
        return -EFAULT;

	return 0;
}

int hax_capability(struct hax_state *hax, struct hax_capabilityinfo *cap)
{
    int ret;
    HANDLE hDevice = hax->fd;   //handle to hax module
    DWORD dSize = 0;
    DWORD err = 0;

    if (hax_invalid_fd(hDevice)) {
        printf("Invalid fd for hax device!\n");
        return -ENODEV;
    }

    ret = DeviceIoControl(hDevice,
      HAX_IOCTL_CAPABILITY,
      NULL, 0,
      cap, sizeof(*cap),
      &dSize,
      (LPOVERLAPPED) NULL);

    if (!ret) {
        err = GetLastError();
        if (err == ERROR_INSUFFICIENT_BUFFER ||
            err == ERROR_MORE_DATA)
            printf("hax capability is too long to hold.\n");
        printf("Failed to get Hax capability:%d\n", err);
        return -EFAULT;
    } else
        return 0;
}

int hax_mod_version(struct hax_state *hax, struct hax_module_version *version)
{
    int ret;
    HANDLE hDevice = hax->fd;   //handle to hax module
    DWORD dSize = 0;
    DWORD err = 0;

    if (hax_invalid_fd(hDevice)) {
        printf("Invalid fd for hax device!\n");
        return -ENODEV;
    }

    ret = DeviceIoControl(hDevice,
      HAX_IOCTL_VERSION,
      NULL, 0,
      version, sizeof(*version),
      &dSize,
      (LPOVERLAPPED) NULL);

    if (!ret) {
        err = GetLastError();
        if (err == ERROR_INSUFFICIENT_BUFFER ||
            err == ERROR_MORE_DATA)
            printf("HAX module is too large.\n");
        printf("Failed to get Hax module version:%d\n", err);
        return -EFAULT;
    } else
        return 0;
}

char *hax_vm_devfs_string(int vm_id)
{
    char *name;

    if (vm_id > MAX_VM_ID)
    {
        printf("Too big VM id\n");
        return NULL;
    }

    name = _strdup("\\\\.\\hax_vmxx");
    if (!name)
        return NULL;
    sprintf(name, "\\\\.\\hax_vm%02d", vm_id);

    return name;
}

char *hax_vcpu_devfs_string(int vm_id, int vcpu_id)
{
    char *name;

    if (vm_id > MAX_VM_ID || vcpu_id > MAX_VCPU_ID)
    {
        printf("Too big vm id %x or vcpu id %x\n", vm_id, vcpu_id);
        return NULL;
    }
    name = _strdup("\\\\.\\hax_vmxx_vcpuxx");
    if (!name)
        return NULL;
    sprintf(name, "\\\\.\\hax_vm%02d_vcpu%02d", vm_id, vcpu_id);

    return name;
}

int hax_host_create_vm(struct hax_state *hax, int *vmid)
{
    int ret;
    int vm_id = 0;
    DWORD dSize = 0;

    if (hax_invalid_fd(hax->fd))
        return -EINVAL;

    if (hax->vm)
        return 0;

    ret = DeviceIoControl(hax->fd,
      HAX_IOCTL_CREATE_VM,
      NULL, 0,
      &vm_id, sizeof(vm_id),
      &dSize,
      (LPOVERLAPPED) NULL);
    if (!ret) {
        printf("error code:%d", GetLastError());
        return -1;
    }
    *vmid = vm_id;
    return 0;
}

hax_fd hax_host_open_vm(struct hax_state *hax, int vm_id)
{
    char *vm_name = NULL;
    hax_fd hDeviceVM;

    vm_name = hax_vm_devfs_string(vm_id);
    if (!vm_name) {
        printf("Incorrect name\n");
        return INVALID_HANDLE_VALUE;
    }

    hDeviceVM = CreateFile(vm_name,
      GENERIC_READ | GENERIC_WRITE,
      0,
      NULL,
      CREATE_ALWAYS,
      FILE_ATTRIBUTE_NORMAL,
      NULL);
    if (hDeviceVM == INVALID_HANDLE_VALUE)
        printf("Open the vm devcie error:%s, ec:%d\n", vm_name, GetLastError());

    free(vm_name);
    return hDeviceVM;
}

int hax_notify_qemu_version(hax_fd vm_fd, struct hax_qemu_version *qversion)
{
    int ret;
    DWORD dSize = 0;

    if (hax_invalid_fd(vm_fd))
        return -EINVAL;

    ret = DeviceIoControl(vm_fd,
      HAX_VM_IOCTL_NOTIFY_QEMU_VERSION,
      qversion, sizeof(struct hax_qemu_version),
      NULL, 0,
      &dSize,
      (LPOVERLAPPED) NULL);
    if (!ret)
    {
        printf("Failed to notify qemu API version\n");
        return -1;
    }

    return 0;
}

int hax_host_create_vcpu(hax_fd vm_fd, int vcpuid)
{
    int ret;
    DWORD dSize = 0;

    ret = DeviceIoControl(vm_fd,
      HAX_VM_IOCTL_VCPU_CREATE,
      &vcpuid, sizeof(vcpuid),
      NULL, 0,
      &dSize,
      (LPOVERLAPPED) NULL);
    if (!ret)
    {
        printf("Failed to create vcpu %x\n", vcpuid);
        return -1;
    }

    return 0;
}

hax_fd hax_host_open_vcpu(int vmid, int vcpuid)
{
    char *devfs_path = NULL;
    hax_fd hDeviceVCPU;

    devfs_path = hax_vcpu_devfs_string(vmid, vcpuid);
    if (!devfs_path)
    {
        printf("Failed to get the devfs\n");
        return INVALID_HANDLE_VALUE;
    }

    hDeviceVCPU = CreateFile( devfs_path,
      GENERIC_READ | GENERIC_WRITE,
      0,
      NULL,
      CREATE_ALWAYS,
      FILE_ATTRIBUTE_NORMAL,
      NULL);

    if (hDeviceVCPU == INVALID_HANDLE_VALUE)
        printf("Failed to open the vcpu devfs\n");
    free(devfs_path);
    return hDeviceVCPU;
}

int hax_host_setup_vcpu_channel(struct hax_vcpu_state *vcpu)
{
    hax_fd hDeviceVCPU = vcpu->fd;
    int ret;
    struct hax_tunnel_info info;
    DWORD dSize = 0;

    ret = DeviceIoControl(hDeviceVCPU,
      HAX_VCPU_IOCTL_SETUP_TUNNEL,
      NULL, 0,
      &info, sizeof(info),
      &dSize,
      (LPOVERLAPPED) NULL);
    if (!ret)
    {
        printf("Failed to setup the hax tunnel\n");
        return -1;
    }

    if (!valid_hax_tunnel_size(info.size))
    {
        printf("Invalid hax tunnel size %x\n", info.size);
        ret = -EINVAL;
        return ret;
    }
    vcpu->tunnel = (struct hax_tunnel *)(info.va);
    vcpu->iobuf = (unsigned char *)(info.io_va);
    return 0;
}

int hax_vcpu_run(struct hax_vcpu_state* vcpu)
{
    int ret;
    HANDLE hDeviceVCPU = vcpu->fd;
    DWORD dSize = 0;

    ret = DeviceIoControl(hDeviceVCPU,
      HAX_VCPU_IOCTL_RUN,
      NULL, 0,
      NULL, 0,
      &dSize,
      (LPOVERLAPPED) NULL);
    if (!ret)
        return -EFAULT;
    else
        return 0;
}

int hax_sync_fpu(struct hax_vcpu_state *env, struct fx_layout *fl, int set)
{
    int ret;
    hax_fd fd;
    HANDLE hDeviceVCPU;
    DWORD dSize = 0;

    fd = env->fd;
    if (hax_invalid_fd(fd))
        return -1;

    hDeviceVCPU = fd;

    if (set)
        ret = DeviceIoControl(hDeviceVCPU,
          HAX_VCPU_IOCTL_SET_FPU,
          fl, sizeof(*fl),
          NULL, 0,
          &dSize,
          (LPOVERLAPPED) NULL);
    else
        ret = DeviceIoControl(hDeviceVCPU,
          HAX_VCPU_IOCTL_GET_FPU,
          NULL, 0,
          fl, sizeof(*fl),
          &dSize,
          (LPOVERLAPPED) NULL);
    if (!ret)
        return -EFAULT;
    else
        return 0;
}

int hax_sync_msr(struct hax_vcpu_state *env, struct hax_msr_data *msrs, int set)
{
    int ret;
    hax_fd fd;
    HANDLE hDeviceVCPU;
    DWORD dSize = 0;

	fd = env->fd;
	if (hax_invalid_fd(fd))
		return -1;

    hDeviceVCPU = fd;

    if (set)
        ret = DeviceIoControl(hDeviceVCPU,
          HAX_VCPU_IOCTL_SET_MSRS,
          msrs, sizeof(*msrs),
          msrs, sizeof(*msrs),
          &dSize,
          (LPOVERLAPPED) NULL);
    else
        ret = DeviceIoControl(hDeviceVCPU,
          HAX_VCPU_IOCTL_GET_MSRS,
          msrs, sizeof(*msrs),
          msrs, sizeof(*msrs),
          &dSize,
          (LPOVERLAPPED) NULL);
    if (!ret)
        return -EFAULT;
    else
        return 0;
}

int hax_sync_vcpu_state(struct hax_vcpu_state *env, struct vcpu_state_t *state, int set)
{
    int ret;
    hax_fd fd;
    HANDLE hDeviceVCPU;
    DWORD dSize;

	fd = env->fd;
	if (hax_invalid_fd(fd))
		return -1;

    hDeviceVCPU = fd;

    if (set)
        ret = DeviceIoControl(hDeviceVCPU,
          HAX_VCPU_SET_REGS,
          state, sizeof(*state),
          NULL, 0,
          &dSize,
          (LPOVERLAPPED) NULL);
    else
        ret = DeviceIoControl(hDeviceVCPU,
          HAX_VCPU_GET_REGS,
          NULL, 0,
          state, sizeof(*state),
          &dSize,
          (LPOVERLAPPED) NULL);
    if (!ret)
        return -EFAULT;
    else
        return 0;
}

int hax_inject_interrupt(struct hax_vcpu_state *env, int vector)
{
    int ret;
    hax_fd fd;
    HANDLE hDeviceVCPU;
    DWORD dSize;

	fd = env->fd;
	if (hax_invalid_fd(fd))
		return -1;

    hDeviceVCPU = fd;

    ret = DeviceIoControl(hDeviceVCPU,
      HAX_VCPU_IOCTL_INTERRUPT,
      &vector, sizeof(vector),
      NULL, 0,
      &dSize,
      (LPOVERLAPPED) NULL);
    if (!ret)
        return -EFAULT;
    else
        return 0;
}

uint64_t hax_get_pa(struct hax_vcpu_state *env, uint64_t va, uint64_t *pa)
{
	int ret;
	hax_fd fd;
	HANDLE hDeviceVCPU;
	DWORD dSize;

	fd = env->fd;
	if (hax_invalid_fd(fd))
		return -1;

	hDeviceVCPU = fd;

	ret = DeviceIoControl(hDeviceVCPU,
		HAX_VCPU_IOCTL_VA2GPA,
		&va, sizeof(va),
		pa, sizeof(uint64_t),
		&dSize,
		(LPOVERLAPPED)NULL);
	if (!ret)
		return -EFAULT;
	else
		return 0;
}

int hax_set_debug(struct hax_vcpu_state *env, uint32_t control)
{
	struct hax_debug_t dbg = { 0 };
	int ret;
	hax_fd fd;
	HANDLE hDeviceVCPU;
	DWORD dSize;

	fd = env->fd;
	if (hax_invalid_fd(fd))
		return -1;

	hDeviceVCPU = fd;

	dbg.control = control;

	ret = DeviceIoControl(hDeviceVCPU,
		HAX_IOCTL_VCPU_DEBUG,
		&dbg, sizeof(dbg),
		NULL, 0,
		&dSize,
		(LPOVERLAPPED)NULL);
	if (!ret)
		return -EFAULT;
	else
		return 0;
}
