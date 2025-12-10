/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright (C) IBM Corporation 2024
 */

#include <stdio.h>
#include <string.h>

#include "eal_private.h"

/*
DPDK 在 PowerPC64（ppc64/powerpc64le）架构下，用于检测当前系统是否使用了 Radix MMU（也叫 Radix Tree 页表），而不是传统的 Hash MMU。
	从 Linux 的 /proc/cpuinfo 中读取信息。
	查找包含 MMU 的行，判断其后是否出现 : Radix。
	如果是 Radix → 返回 true（支持）
	如果是 Hash 或根本找不到 → 返回 false（不支持）

为什么 DPDK 强制要求 Radix MMU？
	因为从 DPDK 19.11 开始，在 POWER9 及以上处理器上，只有 Radix MMU 模式才支持 128 TiB 以上的巨大虚拟地址空间，并且支持 52-bit 虚拟地址和 5-level 页表（2M/1G hugepage 更高效）。
Hash MMU 最大只支持 41～44-bit 虚拟地址，无法满足现代 DPDK 程序对大内存（几百 GB ~ TB 级 hugepage）的需求，容易出现 VA 空间耗尽、映射失败等问题。
*/
bool
eal_mmu_supported(void)
{
#ifdef RTE_EXEC_ENV_LINUX
	static const char proc_cpuinfo[] = "/proc/cpuinfo";
	static const char str_mmu[] = "MMU";
	static const char str_radix[] = "Radix";
	char buf[512];
	char *ret = NULL;
	FILE *f = fopen(proc_cpuinfo, "r");

	if (f == NULL) {
		EAL_LOG(ERR, "Cannot open %s", proc_cpuinfo);
		return false;
	}

	/*
	 * Example "MMU" in /proc/cpuinfo:
	 * ...
	 * model	: 8335-GTW
	 * machine	: PowerNV 8335-GTW
	 * firmware	: OPAL
	 * MMU		: Radix
	 * ... or ...
	 * model        : IBM,9009-22A
	 * machine      : CHRP IBM,9009-22A
	 * MMU          : Hash
	 */
	while (fgets(buf, sizeof(buf), f) != NULL) {
		ret = strstr(buf, str_mmu);
		if (ret == NULL)
			continue;
		ret += sizeof(str_mmu) - 1;
		ret = strchr(ret, ':');
		if (ret == NULL)
			continue;
		ret = strstr(ret, str_radix);
		break;
	}
	fclose(f);

	if (ret == NULL)
		EAL_LOG(ERR, "DPDK on PPC64 requires radix-mmu");

	return (ret != NULL);
#elif RTE_EXEC_ENV_FREEBSD
	/*
	 * Method to detect MMU type in FreeBSD not known at the moment.
	 * Return true for now to emulate previous behavior and
	 * avoid unnecessary failures.
	 */
	return true;
#else
	/* Force false for other execution environments. */
	return false;
#endif
}
