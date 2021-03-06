/*
=====================
DiabloHorn - https://diablohorn.com
Heavily minimzed version of the original control program for imdisk
Original copyright retained below, thank you Olof Lagerkvist for this great software.

Thanks as well to Alexander Rymdeko-Harvey(@Killswitch-GUI) since I borrowed
the driver loading code from his very well structured code:
https://github.com/killswitch-GUI/HotLoad-LoadDriver/
=====================

Control program for the ImDisk Virtual Disk Driver for Windows NT/2000/XP.

Copyright (C) 2004-2015 Olof Lagerkvist.

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM , DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/

#include "stdafx.h"

NTSTATUS(NTAPI *NtLoadDriver)(IN PUNICODE_STRING DriverServiceName);
VOID(NTAPI *RtlInitUnicodeString)(PUNICODE_STRING DestinationString, PCWSTR SourceString);
NTSTATUS(NTAPI *NtUnloadDriver)(IN PUNICODE_STRING DriverServiceName);
bool EnablePrivilege(LPCWSTR);

// https://github.com/iceboy-sjtu/ntdrvldr/blob/master/main.c
bool EnablePrivilege(LPCWSTR lpPrivilegeName){
	TOKEN_PRIVILEGES Privilege;
	HANDLE hToken;
	DWORD dwErrorCode;

	Privilege.PrivilegeCount = 1;
	Privilege.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	if (!LookupPrivilegeValueW(NULL, lpPrivilegeName,
		&Privilege.Privileges[0].Luid))
		return GetLastError();

	if (!OpenProcessToken(GetCurrentProcess(),
		TOKEN_ADJUST_PRIVILEGES, &hToken))
		return GetLastError();

	if (!AdjustTokenPrivileges(hToken, FALSE, &Privilege, sizeof(Privilege),
		NULL, NULL)) {
		dwErrorCode = GetLastError();
		CloseHandle(hToken);
		return dwErrorCode;
	}

	CloseHandle(hToken);
	return TRUE;
}

HANDLE openRamDisk(void) {
	HANDLE device = NULL;
	CHAR ramdisk_drive_name[] = "\\\\.\\ImDiskCtl";
	DWORD VersionCheck = 0;
	DWORD BytesReturned = 0;

	device = CreateFile(ramdisk_drive_name, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);
	if (device == INVALID_HANDLE_VALUE) {
		printf("%s %d\n", "createfile failed", GetLastError());
		return NULL;
	}

	if (!DeviceIoControl(device, IOCTL_IMDISK_QUERY_VERSION, NULL, 0, &VersionCheck, sizeof(VersionCheck), &BytesReturned, NULL)) {
		CloseHandle(device);
		printf("%s %d\n", "failed deviceiocontrol queryversion", GetLastError());
		return NULL;
	}

	printf("Driver version %u.%u\n", HIBYTE(VersionCheck), LOBYTE(VersionCheck));
	return device;
}

int createRamDiskDrive(int diskbytesize, char *targetmountpoint, char devicenumber) {
	HANDLE device = NULL;
	int iCreateDisk = -1;
	DWORD flags = 0;
	DISK_GEOMETRY disk_geometry = { 0 };
	LARGE_INTEGER image_offset = { 0 };
	LARGE_INTEGER disk_size = { 0 };
	BOOL definedosdevice = 0;
	char mountpoint[3] = { 0 };
	char ntdevicename[MAX_PATH] = { 0 };
	DWORD BytesReturned = 0;
	PIMDISK_CREATE_DATA create_data = NULL;
	char basedevicepath[15] = "\\Device\\ImDisk";
	char *devicepath = NULL;
	long devicenumber_i = 0;

	devicepath = (char *)malloc(MAX_PATH);
	if (devicepath == NULL) {
		printf("%s\n", "malloc devicepath failed\n");
		return 1;
	}
	SecureZeroMemory(devicepath, MAX_PATH);
	strcat_s(devicepath, MAX_PATH, basedevicepath);
	strcat_s(devicepath, MAX_PATH, &devicenumber);

	mountpoint[0] = targetmountpoint[0];
	mountpoint[1] = targetmountpoint[1];

	device = openRamDisk();
	if (device == NULL) {
		printf("%s\n", "openramdisk during create ramdisk failed");
		exit(0);
	}

	flags |= IMDISK_TYPE_VM;
	flags |= IMDISK_DEVICE_TYPE_HD;
	disk_size.QuadPart = diskbytesize;
	disk_geometry.Cylinders = disk_size;

	create_data = (PIMDISK_CREATE_DATA) malloc(sizeof(IMDISK_CREATE_DATA));
	if (create_data == NULL) {
		return 1;
	}

	ZeroMemory(create_data, sizeof(IMDISK_CREATE_DATA));

	devicenumber_i = strtol(&devicenumber, NULL, 10);
	create_data->DeviceNumber = devicenumber_i; 
	create_data->DiskGeometry = disk_geometry;
	create_data->ImageOffset = image_offset;
	create_data->Flags = flags;
	create_data->DriveLetter = targetmountpoint[0];
	create_data->FileNameLength = 0;
	if (!DeviceIoControl(device, IOCTL_IMDISK_CREATE_DEVICE, create_data, sizeof(IMDISK_CREATE_DATA), create_data, sizeof(IMDISK_CREATE_DATA), &BytesReturned, NULL)) {
		printf("%s\n", "create device failed");
		return 1;
	}
	
	printf("%s\n", "Device has been created");

	definedosdevice = DefineDosDevice(DDD_RAW_TARGET_PATH, mountpoint, devicepath);
	if (!definedosdevice) {
		printf("%s\n", "define dos device NOT OK");
		exit(0);
	}
	printf("%s", "define dos device OK ");
	QueryDosDevice(mountpoint, ntdevicename, MAX_PATH);
	printf("%s\n", ntdevicename);

	CloseHandle(device);
	return 0;
}

int deleteRamDisk(char devnum) {
	HANDLE device = NULL;
	DWORD create_data_size = 0;
	PIMDISK_CREATE_DATA create_data = 0;
	DWORD dw = 0;
	DWORD devicenumber = 0;

	devicenumber = strtol(&devnum, NULL, 10);

	device = openRamDisk();
	if (device == NULL) {
		printf("%s\n", "delete disk, could not open driver");
		return 1;
	}


	if (!DeviceIoControl(device, IOCTL_IMDISK_REMOVE_DEVICE, &devicenumber,	sizeof(devicenumber), NULL, 0, &dw, NULL)){
		printf("%d\n", GetLastError());
		CloseHandle(device);
		return 1;
	}

	CloseHandle(device);
	printf("%s\n", "device removed");
	return 0;
}

int listRamDisk(void) {
	HANDLE hRamDisk = NULL;
	ULONG current_size = 5;
	ULONG dw = 0;
	DWORD counter = 0;
	PULONG device_list = NULL;

	device_list = (PULONG)HeapAlloc(GetProcessHeap(), 0, sizeof(ULONG) * current_size);
	if (device_list == NULL) {
		printf("%s\n", "mem alloc device list failed");
		return 1;
	}

	hRamDisk = openRamDisk();
	if (hRamDisk == NULL) {
		printf("%s\n", "list driver, could not open ramdisk");
		HeapFree(GetProcessHeap(), 0, device_list);
		return 1;
	}

	if (!DeviceIoControl(hRamDisk, IOCTL_IMDISK_QUERY_DRIVER, NULL, 0, device_list, current_size << 2, &dw, NULL)) {
		CloseHandle(hRamDisk);
		HeapFree(GetProcessHeap(), 0, device_list);
		printf("%s\n", "ioctl get list failed");
		return 1;
	}

	CloseHandle(hRamDisk);

	for (counter = 1; counter <= *device_list; counter++) {
		printf("%s%u\n", "\\Device\\ImDisk", device_list[counter]);
	}

	HeapFree(GetProcessHeap(), 0, device_list);
	return 0;
}

int installDriver(BOOL doinstall) {
	HMODULE hNtdll = NULL;
	NTSTATUS st1 = 0;
	UNICODE_STRING pPathReg;
	PCWSTR regpath = L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\ImDisk";
	char ntdllname[10] = "ntdll.dll";

	if (!EnablePrivilege(L"SeLoadDriverPrivilege")) {
		printf("%s\n", "enabling seloaddriverpriv failed");
		return 1;
	}

	hNtdll = GetModuleHandle(ntdllname);
	if (hNtdll == NULL) {
		printf("%s\n", "ntdll load failed");
		return 1;
	}

	*(FARPROC *)&RtlInitUnicodeString = GetProcAddress(hNtdll, "RtlInitUnicodeString");
	*(FARPROC *)&NtLoadDriver = GetProcAddress(hNtdll, "NtLoadDriver");
	*(FARPROC *)&NtUnloadDriver = GetProcAddress(hNtdll, "NtUnloadDriver");

	RtlInitUnicodeString(&pPathReg, regpath);

	if (doinstall) {
		st1 = NtLoadDriver(&pPathReg);
		if (st1 != STATUS_SUCCESS) {
			printf("%s\n", "load driver failed");
			return 1;
		}
		printf("%s\n", "driver has been loaded");
	}
	else {
		st1 = NtUnloadDriver(&pPathReg);
		if (st1 != STATUS_SUCCESS) {
			printf("%s\n", "unload driver failed");
			return 1;
		}
		printf("%s\n", "driver has been unloaded");
	}
	return 0;
}

void printUsage(void) {
	printf("DiabloHorn - https://diablohorn.com\n");
	printf("   Install   - cliramdisk.exe i\n");
	printf("   Uninstall - cliramdisk.exe u\n");
	printf("   Create    - cliramdisk.exe c <size in bytes> <drive|folder to mount> <device number>\n");
	printf("   Delete    - cliramdisk.exe d <devicenumber>\n");
	printf("   List      - cliramdisk.exe l\n");
	exit(0);
}

/*
	cliramdisk.exe i 
	cliramdisk.exe u 
	cliramdisk.exe c <size in bytes> <folder to mount> <device number>
	cliramdisk.exe d <devicenumber>
	cliramdisk.exe l
*/
int main(int argc, char*argv[])
{
	long disksize_l = 0;
	int disksize = 0;
	
	if (argc < 2 || argc > 5) {
		printUsage();
	}

	if (*argv[1] == 'i') {
		if (argc != 2) {
			printUsage();
		}
		installDriver(TRUE);
	}
	
	if (*argv[1] == 'u') {
		if (argc != 2) {
			printUsage();
		}
		installDriver(FALSE);
	}

	if (*argv[1] == 'c') {
		if (argc != 5) {
			printUsage();
		}
		disksize_l = strtol(argv[2], NULL, 10);
		disksize = disksize_l;
		createRamDiskDrive(disksize, argv[3], argv[4][0]);
	}

	if (*argv[1] == 'd') {
		if (argc != 3) {
			printUsage();
		}
		deleteRamDisk(argv[2][0]);
	}

	if (*argv[1] == 'l') {
		if (argc != 2) {
			printUsage();
		}
		listRamDisk();
	}
    return 0;
}

