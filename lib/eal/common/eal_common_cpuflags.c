/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2014 Intel Corporation
 */

#include <stdio.h>

#include <eal_export.h>
#include <rte_common.h>
#include <rte_cpuflags.h>

RTE_EXPORT_SYMBOL(rte_cpu_is_supported)
int
//启动时用来检测当前运行的 CPU 是否满足编译时指定的最低 CPU 特性要求的核心检查函数
/*
	核心逻辑是：
		在编译阶段，构建系统会根据用户选择的 -march / RTE_MACHINE（如 native、nehalem、haswell 等）生成一个编译期常量数组compile_time_flags[]，里面存放了当前二进制文件所必需的 CPU 指令集特性（例如 AVX2、AVX512、AES、SSE4.2 等）。
		运行时，rte_cpu_is_supported() 会遍历这个数组，对每一项必需的 CPU flag 调用 rte_cpu_get_flag_enabled(flag)（底层通常是 __builtin_cpu_supports() 或 cpuid 指令）来检查当前 CPU 是否真的支持。
		如果任何一项必需特性缺失或查询失败，就打印错误信息并返回 0；全部支持才返回 1。
	简单来说：“我编译的时候说要 AVX512，你现在这颗 CPU 真的有吗？没有就直接报错退出。”
*/
rte_cpu_is_supported(void)
{
	/* This is generated at compile-time by the build system */
	static const enum rte_cpu_flag_t compile_time_flags[] = {
			RTE_COMPILE_TIME_CPUFLAGS
	};
	unsigned count = RTE_DIM(compile_time_flags), i;
	int ret;

	for (i = 0; i < count; i++) {
		ret = rte_cpu_get_flag_enabled(compile_time_flags[i]);

		if (ret < 0) {
			fprintf(stderr,
				"ERROR: CPU feature flag lookup failed with error %d\n",
				ret);
			return 0;
		}
		if (!ret) {
			fprintf(stderr,
			        "ERROR: This system does not support \"%s\".\n"
			        "Please check that RTE_MACHINE is set correctly.\n",
			        rte_cpu_get_flag_name(compile_time_flags[i]));
			return 0;
		}
	}

	return 1;
}
