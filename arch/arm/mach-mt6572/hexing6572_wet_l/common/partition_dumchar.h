#include <linux/module.h>
#include "partition_define.h"
struct excel_info PartInfo[PART_MAX_COUNT];
EXPORT_SYMBOL(PartInfo);

#if defined(MTK_EMMC_SUPPORT) || defined(CONFIG_MTK_EMMC_SUPPORT)
struct MBR_EBR_struct MBR_EBR_px[MBR_COUNT]={
	{"mbr", {1, 2, 3, 4, }},
	{"ebr1", {5, 6, 7, }},
	{"ebr2", {8, 9, }},
};

EXPORT_SYMBOL(MBR_EBR_px);
#endif

