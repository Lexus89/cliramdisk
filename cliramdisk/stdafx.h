// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>



// TODO: reference additional headers your program requires here
#include <Windows.h>

typedef struct _UNICODE_STRING {
	USHORT Length;
	USHORT MaximumLength;
	PWSTR  Buffer;
} UNICODE_STRING;

typedef UNICODE_STRING *PUNICODE_STRING;
#define  STATUS_SUCCESS                    ((NTSTATUS)0x00000000L)


///////////////////// constants
#define FILE_DEVICE_IMDISK				    0x8372
#define IOCTL_IMDISK_QUERY_VERSION		    ((ULONG) CTL_CODE(FILE_DEVICE_IMDISK, 0x800, METHOD_BUFFERED, 0))
#define IOCTL_IMDISK_CREATE_DEVICE		    ((ULONG) CTL_CODE(FILE_DEVICE_IMDISK, 0x801, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS))
#define IOCTL_IMDISK_QUERY_DRIVER           ((ULONG) CTL_CODE(FILE_DEVICE_IMDISK, 0x803, METHOD_BUFFERED, 0))
#define IOCTL_IMDISK_REMOVE_DEVICE          ((ULONG) CTL_CODE(FILE_DEVICE_IMDISK, 0x806, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS))
/// Virtual disk is backed by virtual memory
#define IMDISK_TYPE_VM                  0x00000200
/// Device type is virtual harddisk partition
#define IMDISK_DEVICE_TYPE_HD           0x00000010

// structs
/**
Structure used by the IOCTL_IMDISK_CREATE_DEVICE and
IOCTL_IMDISK_QUERY_DEVICE calls and by the ImDiskQueryDevice function.
*/
typedef struct _IMDISK_CREATE_DATA
{
	/// On create this can be set to IMDISK_AUTO_DEVICE_NUMBER
	ULONG           DeviceNumber;
	/// Total size in bytes (in the Cylinders field) and virtual geometry.
	DISK_GEOMETRY   DiskGeometry;
	/// The byte offset in the image file where the virtual disk begins.
	LARGE_INTEGER   ImageOffset;
	/// Creation flags. Type of device and type of connection.
	ULONG           Flags;
	/// Drive letter (if used, otherwise zero).
	WCHAR           DriveLetter;
	/// Length in bytes of the FileName member.
	USHORT          FileNameLength;
	/// Dynamically-sized member that specifies the image file name.
	WCHAR           FileName[1];
} IMDISK_CREATE_DATA, *PIMDISK_CREATE_DATA;