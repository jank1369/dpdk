/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2020 Mellanox Technologies, Ltd
 */

#include <rte_string_fns.h>

#include <eal_export.h>
#include "eal_private.h"
#include "eal_memcfg.h"

/* early configuration structure, when memory config is not mmapped 
DPDK 必须先有一个能用的全局配置区和锁，否则连多核、内存、队列都启动不了。
	锁类型,          底层实现,                            适用场景,                        是否会睡眠
	rte_spinlock_t, 自旋（死循环 + pause）,               极短临界区（< 1µs）,              不会
	rte_rwlock_t,   自旋 + 原子操作（x86 cmpxchg16b）,    读多写少、临界区稍长（< 10µs）,    不会
*/
static struct rte_mem_config early_mem_config = {
	.mlock = RTE_RWLOCK_INITIALIZER, //保护 主内存区（rte_malloc、memzone、ring 等） 的读写锁
	.qlock = RTE_RWLOCK_INITIALIZER, //保护 全局队列（rte_ring） 的创建/释放锁
	.mplock = RTE_RWLOCK_INITIALIZER, //保护 mempool（内存池） 的全局操作锁
	.tlock = RTE_SPINLOCK_INITIALIZER, //保护 尾队列（tailq） —— 所有 DPDK 链表（dev、mempool、ring 等）都靠它实现
	.ethdev_lock = RTE_SPINLOCK_INITIALIZER, //保护 网卡设备（rte_eth_dev） 的全局链表增删锁
	.memory_hotplug_lock = RTE_RWLOCK_INITIALIZER, //保护 内存热插拔 相关的操作（后期动态增减 hugepage
};

/* Address of global and public configuration */
static struct rte_config  rte_config = {
	.mem_config = &early_mem_config,
};

/* platform-specific runtime dir */
static char runtime_dir[PATH_MAX];

/* internal configuration */
static struct internal_config internal_config;

RTE_EXPORT_SYMBOL(rte_eal_get_runtime_dir)
const char *
rte_eal_get_runtime_dir(void)
{
	return runtime_dir;
}

int
eal_set_runtime_dir(const char *run_dir)
{
	if (strlcpy(runtime_dir, run_dir, PATH_MAX) >= PATH_MAX) {
		EAL_LOG(ERR, "Runtime directory string too long");
		return -1;
	}

	return 0;
}

/* Return a pointer to the configuration structure */
struct rte_config *
rte_eal_get_configuration(void)
{
	return &rte_config;
}

/* Return a pointer to the internal configuration structure */
struct internal_config *
eal_get_internal_configuration(void)
{
	return &internal_config;
}

RTE_EXPORT_SYMBOL(rte_eal_iova_mode)
enum rte_iova_mode
rte_eal_iova_mode(void)
{
	return rte_eal_get_configuration()->iova_mode;
}

/* Get the EAL base address */
RTE_EXPORT_INTERNAL_SYMBOL(rte_eal_get_baseaddr)
uint64_t
rte_eal_get_baseaddr(void)
{
	return (internal_config.base_virtaddr != 0) ?
		       (uint64_t) internal_config.base_virtaddr :
		       eal_get_baseaddr();
}

RTE_EXPORT_SYMBOL(rte_eal_process_type)
enum rte_proc_type_t
rte_eal_process_type(void)
{
	return rte_config.process_type;
}

/* Return user provided mbuf pool ops name */
RTE_EXPORT_SYMBOL(rte_eal_mbuf_user_pool_ops)
const char *
rte_eal_mbuf_user_pool_ops(void)
{
	return internal_config.user_mbuf_pool_ops_name;
}

/* return non-zero if hugepages are enabled. */
RTE_EXPORT_SYMBOL(rte_eal_has_hugepages)
int
rte_eal_has_hugepages(void)
{
	return !internal_config.no_hugetlbfs;
}

RTE_EXPORT_SYMBOL(rte_eal_has_pci)
int
rte_eal_has_pci(void)
{
	return !internal_config.no_pci;
}
