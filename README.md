# KOBJX: SLAB + RCU + HASH TABLE + SHRINKER Kernel Object Registry

A Linux Kernel Module that implements a miniature kernel object registry using several important Linux kernel subsystems:

* SLAB Allocator
* Linked Lists
* Hash Tables
* RCU (Read-Copy-Update)
* Shrinkers
* Procfs
* Per-CPU Statistics

The project demonstrates how kernel objects can be:

* Created
* Registered
* Tracked
* Looked Up
* Deleted Safely
* Reclaimed Under Memory Pressure

Think of it as a simplified kernel object database similar to mechanisms used inside:

* inode cache
* dentry cache
* device registries
* kernel object stores
* networking lookup tables

---

## ARCHITECTURE OVERVIEW

```
                            +------------------+
                            |   kmem_cache     |
                            +---------+--------+
                                      |
                                 alloc/free
                                      |
                                      v

                +----------------------------------+
                |              kobjx               |
                +----------------------------------+
                | id                               |
                | name                             |
                | age                              |
                | refcnt                           |
                | in_use                           |
                +----------------------------------+
                     |                      |
                     v                      v

              global_list          global_hash_table
                 (list)                (lookup)

                     |
                     v

                shrinker scan

                     |
                     v

                  call_rcu()

                     |
                     v

                 actual free
```

---

## FEATURES

* Custom SLAB cache
* Hash-based object lookup
* Linked-list object tracking
* RCU-safe object deletion
* Memory reclaim via shrinker
* Procfs statistics interface
* Per-CPU counters
* Kernel lifecycle demonstration

---

## BUILD

Requirements:

* Linux kernel headers
* GCC
* Make

Build module:

```bash
make
```

Generated files:

```text
kobjx_slab_rcu_registry.ko
kobjx_slab_rcu_registry.o
kobjx_slab_rcu_registry.mod.o
```

---

## LOAD MODULE

Insert module:

```bash
sudo insmod kobjx_slab_rcu_registry.ko
```

Verify:

```bash
lsmod | grep kobjx
```

View kernel logs:

```bash
dmesg | tail -100
```

---

## REMOVE MODULE

```bash
sudo rmmod kobjx_slab_rcu_registry
```

Check logs:

```bash
dmesg | tail -100
```

---

## PROCFS INTERFACE

Display runtime statistics:

```bash
cat /proc/kobjx_stats
```

Example output:

```text
Active Objects : 0
Alloc Count    : 2
Free Count     : 2
```

---

## MODULE EXECUTION FLOW

When the module loads:

```text
1. Create SLAB cache
2. Register shrinker
3. Create /proc/kobjx_stats
4. Allocate Object A
5. Allocate Object B
6. Insert objects into linked list
7. Insert objects into hash table
8. Perform hash lookup
9. Delete objects using RCU
10. Expose statistics through procfs
```

---

## DATA STRUCTURES USED

1. SLAB CACHE

Purpose:

Fast object allocation and reuse.

Kernel API:

```c
kmem_cache_create()
kmem_cache_alloc()
kmem_cache_free()
```

Benefits:

* Reduced fragmentation
* Object reuse
* CPU cache friendly
* High performance allocation

Real Kernel Examples:

* task_struct
* inode
* dentry

---

2. LINKED LIST

---

Purpose:

Track all active objects.

Kernel API:

```c
struct list_head
list_add()
list_del()
```

Benefits:

* Simple traversal
* Easy insertion
* Easy removal

---

3. HASH TABLE

---

Purpose:

Fast object lookup by ID.

Kernel API:

```c
DEFINE_HASHTABLE()
hash_add()
hash_del()
hash_for_each_possible()
```

Benefits:

* O(1) average lookup
* Scalable object registry

Real Kernel Examples:

* inode caches
* routing tables
* process lookup tables

---

4. RCU (READ COPY UPDATE)

---

Purpose:

Allow readers to access objects without blocking.

Kernel API:

```c
rcu_read_lock()
rcu_read_unlock()
call_rcu()
```

Benefits:

* Lock-free reads
* High scalability
* Excellent SMP performance

Real Kernel Examples:

* Networking subsystem
* Routing tables
* VFS caches

---

5. SHRINKER

---

Purpose:

Reclaim memory when system experiences pressure.

Kernel API:

```c
register_shrinker()
```

Benefits:

* Automatic reclaim
* Memory pressure integration
* Cache cleanup

Real Kernel Examples:

* inode reclaim
* dentry reclaim
* slab reclaim

---

6. PROCFS

---

Purpose:

Expose internal statistics to user space.

Kernel API:

```c
proc_create()
seq_file
```

Benefits:

* Runtime visibility
* Easy debugging
* Monitoring support

Real Kernel Examples:

```text
/proc/meminfo
/proc/slabinfo
/proc/vmstat
```

---

7. PER-CPU STATISTICS

---

Purpose:

Avoid contention during counter updates.

Kernel API:

```c
DEFINE_PER_CPU()
this_cpu_inc()
```

Benefits:

* No global lock
* Better scalability
* SMP friendly

Real Kernel Examples:

* Scheduler counters
* Network packet counters
* SLUB allocator statistics

---

## OBJECT CREATION FLOW

Function:

```text
kobjx_alloc()
```

Flow:

```text
SLAB Allocate
      |
      v

Initialize Fields
      |
      v

Insert Into List
      |
      v

Insert Into Hash
      |
      v

Update Counters
```

Simulates:

* inode creation
* dentry creation
* task object initialization

---

## OBJECT LOOKUP FLOW

Function:

```text
kobjx_lookup()
```

Flow:

```text
rcu_read_lock()
        |
        v

Hash Bucket Search
        |
        v

ID Match
        |
        v

Return Object
```

Key Idea:

```text
READ FAST
READ LOCK-FREE
READ SCALABLE
```

---

## OBJECT DELETION FLOW

Function:

```text
kobjx_delete()
```

Flow:

```text
Remove From List
        |
        v

Remove From Hash
        |
        v

Mark Inactive
        |
        v

call_rcu()
        |
        v

Grace Period
        |
        v

Actual Free
```

Why RCU?

Immediate free is unsafe because another CPU may still hold a reference.

RCU guarantees memory is released only after all readers have exited.

---

## SHRINKER FLOW

Function:

```text
scan_objects()
```

Flow:

```text
Memory Pressure
       |
       v

Scan Object List
       |
       v

Check Age
       |
       v

Remove Old Objects
       |
       v

Free Memory
```

Simulates:

* inode pruning
* dentry reclaim
* VM memory pressure handling

---

## REAL KERNEL SUBSYSTEM MAPPING

This module models concepts used by:

```text
VFS inode cache
VFS dentry cache
SLAB allocator
Memory reclaim subsystem
RCU infrastructure
Procfs monitoring
Networking lookup tables
```

---

## LEARNING OBJECTIVES

This project demonstrates:

* Linux kernel memory allocation
* SLAB allocator internals
* Hash table design
* RCU synchronization
* Kernel memory reclaim
* Procfs implementation
* Per-CPU optimization
* Kernel object lifecycle management

---

## FINAL SUMMARY

KOBJX is a miniature kernel subsystem demonstrating:

* Memory Allocation (SLAB)
* Fast Lookup (HASH)
* Safe Concurrency (RCU)
* Memory Reclaim (SHRINKER)
* Runtime Monitoring (PROCFS)
* Scalability (PER-CPU Statistics)

The project serves as a compact educational model of how multiple Linux kernel subsystems cooperate to implement safe, scalable, high-performance object management.

