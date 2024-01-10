#include "hd.h"
#include "global.h"

int get_fs_dev(int drive, int fs_type)
{
	int i;
	for (i = 1; i < NR_PRIM_PER_DRIVE; i++) // 跳过第1个主分区，因为第1个分区是启动分区 comment added by ran
	{
		if (hd_info[drive].part[i].fs_type == fs_type)
			return ((drive << MAJOR_SHIFT) | i);
	}

	// added by mingxuan 2020-10-29
	for (i = NR_PRIM_PER_DRIVE; i < NR_PRIM_PER_DRIVE + NR_SUB_PER_PART; i++)
	{
		if (hd_info[drive].part[i].fs_type == fs_type)
			return ((drive << MAJOR_SHIFT) | i);
	}
}