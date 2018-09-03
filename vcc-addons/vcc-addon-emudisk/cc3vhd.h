#ifndef _CC3VHD_H_
#define _CC3VHD_H_

/***************************************************************************/

#include "cpu.h"

/***************************************************************************/

#define EMUHDD_PORT 0x80

// TODO: what about cluster sizes > 1
#define DRIVESIZE	130	//in Mb
#define MAX_SECTOR	DRIVESIZE*1024*1024
#define SECTORSIZE	256

#define HD_OK		0
#define HD_PWRUP	-1
#define HD_INVLD	-2
#define HD_NODSK	4
#define HD_WP		5

#define SECTOR_READ		0
#define SECTOR_WRITE	1
#define DISK_FLUSH		2

/***************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

	int				MountHD(char *, int iDiskNum);
	void			UnmountHD(int iDiskNum);
	void			DiskStatus(char *);

	u8_t			VHDRead(mmu_t * pMMU, u8_t Port);
	void			VHDWrite(mmu_t * pMMU, u8_t Data, u8_t Port);

#ifdef __cplusplus
}
#endif

/***************************************************************************/

#endif // _CC3VHD_H_
