/*
 * kobjx_slab_rcu_registry.c
 *
 * Unified Advanced SLAB + RCU + Shrinker + Registry System
 * Combines:
 *  - SLAB allocator cache
 *  - Shrinker-based reclaim
 *  - Procfs stats
 *  - Per-CPU counters
 *  - Hash table registry
 *  - RCU safe lookup + deletion
 *  - Aging-based cleanup
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/shrinker.h>
#include <linux/refcount.h>
#include <linux/jiffies.h>
#include <linux/percpu.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/hashtable.h>
#include <linux/rcupdate.h>
#include <linux/atomic.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Unified Kernel Lab");
MODULE_DESCRIPTION("KOBJX SLAB + RCU + Shrinker Registry System");

/* ================= CONFIG ================= */

#define NAME_LEN 64
#define MAX_AGE_MS 10000

/* ================= OBJECT ================= */

struct kobjx {
    int id;
    char name[NAME_LEN];

    unsigned long created_jiffies;

    refcount_t refcnt;
    atomic_t in_use;

    struct list_head list;
    struct hlist_node hnode;
    struct rcu_head rcu;
};

/* ================= GLOBALS ================= */

static struct kmem_cache *cache;

static LIST_HEAD(global_list);
DEFINE_HASHTABLE(global_table, 8);

static DEFINE_SPINLOCK(lock);

static atomic_long_t active = ATOMIC_LONG_INIT(0);

/* ================= PER CPU STATS ================= */

struct stats {
    unsigned long alloc;
    unsigned long free;
};

static DEFINE_PER_CPU(struct stats, cpu_stats);

/* ================= CONSTRUCTOR ================= */

static void ctor(void *obj)
{
    struct kobjx *k = obj;

    k->id = -1;
    strscpy(k->name, "init", NAME_LEN);
    refcount_set(&k->refcnt, 1);
    atomic_set(&k->in_use, 1);
    k->created_jiffies = jiffies;
    INIT_LIST_HEAD(&k->list);
}

/* ================= ALLOC ================= */

static struct kobjx *kobjx_alloc(int id, const char *name)
{
    struct kobjx *k = kmem_cache_alloc(cache, GFP_KERNEL);
    if (!k) return NULL;

    k->id = id;
    strscpy(k->name, name, NAME_LEN);

    spin_lock(&lock);

    list_add_tail(&k->list, &global_list);
    hash_add(global_table, &k->hnode, id);
    atomic_long_inc(&active);

    spin_unlock(&lock);

    this_cpu_inc(cpu_stats.alloc);

    pr_info("[KOBJX] alloc id=%d name=%s\n", id, name);
    return k;
}

/* ================= RCU LOOKUP ================= */

static struct kobjx *kobjx_lookup(int id)
{
    struct kobjx *k;

    rcu_read_lock();

    hash_for_each_possible_rcu(global_table, k, hnode, id) {
        if (k->id == id) {
            rcu_read_unlock();
            return k;
        }
    }

    rcu_read_unlock();
    return NULL;
}

/* ================= RCU FREE ================= */

static void rcu_free(struct rcu_head *r)
{
    struct kobjx *k = container_of(r, struct kobjx, rcu);
    kmem_cache_free(cache, k);
    this_cpu_inc(cpu_stats.free);
    pr_info("[KOBJX] freed via RCU\n");
}

/* ================= DELETE ================= */

static void kobjx_delete(struct kobjx *k)
{
    if (!k) return;

    spin_lock(&lock);

    list_del(&k->list);
    hash_del(&k->hnode);

    atomic_long_dec(&active);

    spin_unlock(&lock);

    atomic_set(&k->in_use, 0);

    call_rcu(&k->rcu, rcu_free);
}

/* ================= SHRINKER ================= */

static unsigned long count_objects(struct shrinker *s,
                                    struct shrink_control *sc)
{
    return atomic_long_read(&active);
}

static unsigned long scan_objects(struct shrinker *s,
                                  struct shrink_control *sc)
{
    struct kobjx *k, *tmp;
    unsigned long freed = 0;
    unsigned long age = msecs_to_jiffies(MAX_AGE_MS);

    spin_lock(&lock);

    list_for_each_entry_safe(k, tmp, &global_list, list) {

        if (time_before(jiffies, k->created_jiffies + age))
            continue;

        list_del(&k->list);
        hash_del(&k->hnode);
        atomic_long_dec(&active);

        spin_unlock(&lock);

        kmem_cache_free(cache, k);
        freed++;

        if (freed >= sc->nr_to_scan)
            return freed;

        spin_lock(&lock);
    }

    spin_unlock(&lock);
    return freed;
}

static struct shrinker shr = {
    .count_objects = count_objects,
    .scan_objects  = scan_objects,
    .seeks = DEFAULT_SEEKS,
};

/* ================= PROCFS ================= */

static int show(struct seq_file *m, void *v)
{
    int cpu;
    unsigned long a = 0, f = 0;
    struct stats *s;

    seq_printf(m, "KOBJX SLAB RCU SYSTEM\n");
    seq_printf(m, "Active: %ld\n", atomic_long_read(&active));

    for_each_possible_cpu(cpu) {
        s = &per_cpu(cpu_stats, cpu);
        a += s->alloc;
        f += s->free;
    }

    seq_printf(m, "Alloc: %lu Free: %lu\n", a, f);
    return 0;
}

static int open(struct inode *i, struct file *f)
{
    return single_open(f, show, NULL);
}

static const struct proc_ops proc_fops = {
    .proc_open = open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

/* ================= INIT ================= */

static int __init kobjx_init(void)
{
    struct kobjx *a, *b;

    pr_info("[KOBJX] loading...\n");

    cache = kmem_cache_create("kobjx_cache",
                              sizeof(struct kobjx),
                              0,
                              SLAB_POISON | SLAB_RED_ZONE,
                              ctor);

    if (!cache)
        return -ENOMEM;

    register_shrinker(&shr);

    proc_create("kobjx_stats", 0444, NULL, &proc_fops);

    a = kobjx_alloc(1, "A");
    b = kobjx_alloc(2, "B");

    pr_info("[KOBJX] lookup %s\n", kobjx_lookup(1)->name);

    kobjx_delete(a);
    kobjx_delete(b);

    return 0;
}

/* ================= EXIT ================= */

static void __exit kobjx_exit(void)
{
    rcu_barrier();
    unregister_shrinker(&shr);
    remove_proc_entry("kobjx_stats", NULL);
    kmem_cache_destroy(cache);
    pr_info("[KOBJX] exit\n");
}

module_init(kobjx_init);
module_exit(kobjx_exit);
