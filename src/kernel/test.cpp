
// This file is a part of Simple-XX/SimpleKernel
// (https://github.com/Simple-XX/SimpleKernel).
//
// test.cpp for Simple-XX/SimpleKernel.

#include "common.h"
#include "stdio.h"
#include "iostream"
#include "assert.h"
#include "pmm.h"
#include "vmm.h"
#include "kernel.h"

int32_t test_pmm(void) {
    // 保存现有 pmm 空闲页数量
    size_t free_pages = PMM::get_free_pages_count();
    // 计算内核实际占用页数
    auto kernel_pages =
        ((uint8_t *)COMMON::ALIGN(COMMON::KERNEL_END_ADDR, COMMON::PAGE_SIZE) -
         (uint8_t *)COMMON::ALIGN(COMMON::KERNEL_START_ADDR,
                                  COMMON::PAGE_SIZE)) /
        COMMON::PAGE_SIZE;
    // 空闲页数应该等于物理内存大小-内核使用
    assert(free_pages == (COMMON::PMM_SIZE / COMMON::PAGE_SIZE) - kernel_pages);
    // 获取已使用页数
    size_t used_pages = PMM::get_used_pages_count();
    // 已使用页数应该等于内核使用页数
    assert(used_pages == kernel_pages);
    // 分配
    auto addr1 = PMM::alloc_pages(2);
    // 已使用应该会更新
    assert(PMM::get_used_pages_count() == 2 + kernel_pages);
    // 同上
    auto addr2 = PMM::alloc_pages(3);
    assert(PMM::get_used_pages_count() == 5 + kernel_pages);
    // 同上
    auto addr3 = PMM::alloc_pages(100);
    assert(PMM::get_used_pages_count() == 105 + kernel_pages);
    // 同上
    auto addr4 = PMM::alloc_pages(100);
    assert(PMM::get_used_pages_count() == 205 + kernel_pages);
    // 分配超过限度的内存，应该返回 nullptr
    auto addr5 = PMM::alloc_pages(0xFFFFFFFF);
    assert(addr5 == nullptr);
    // 全部释放
    PMM::free_pages(addr1, 2);
    PMM::free_pages(addr2, 3);
    PMM::free_pages(addr3, 100);
    PMM::free_pages(addr4, 100);
    // 现在内存使用情况应该与此函数开始时相同
    assert(PMM::get_free_pages_count() == free_pages);
    printf("pmm test done.\n");
    return 0;
}

int32_t test_vmm(void) {
    uintptr_t addr = (uintptr_t) nullptr;
    // 首先确认内核空间被映射了
    assert(VMM::get_pgd() != nullptr);
    assert(
        VMM::get_mmap(VMM::get_pgd(),
                      (void *)((uintptr_t)COMMON::KERNEL_START_ADDR + 0x1000),
                      &addr) == 1);
    assert(addr == (uintptr_t)COMMON::KERNEL_START_ADDR + 0x1000);
    addr = (uintptr_t) nullptr;
    assert(VMM::get_mmap(VMM::get_pgd(),
                         (void *)((uintptr_t)COMMON::KERNEL_START_ADDR +
                                  VMM_KERNEL_SPACE_SIZE - 1),
                         &addr) == 1);
    assert(addr == (uintptr_t)(((uintptr_t)COMMON::KERNEL_START_ADDR +
                                VMM_KERNEL_SPACE_SIZE - 1) &
                               COMMON::PAGE_MASK));
    addr = (uintptr_t) nullptr;
    assert(
        VMM::get_mmap(VMM::get_pgd(),
                      (void *)((uintptr_t)COMMON::ALIGN(
                                   COMMON::KERNEL_START_ADDR, 4 * COMMON::KB) +
                               VMM_KERNEL_SPACE_SIZE),
                      &addr) == 0);
    assert(addr == (uintptr_t) nullptr);
    addr = (uintptr_t) nullptr;
    assert(
        VMM::get_mmap(VMM::get_pgd(),
                      (void *)((uintptr_t)COMMON::ALIGN(
                                   COMMON::KERNEL_START_ADDR, 4 * COMMON::KB) +
                               VMM_KERNEL_SPACE_SIZE + 0x1024),
                      nullptr) == 0);
    // 测试映射与取消映射
    addr = (uintptr_t) nullptr;
    // 准备映射的虚拟地址 3GB 处
    uintptr_t va = 0xC0000000;
    // 准备映射的物理地址 0.75GB 处
    uintptr_t pa = 0x30000000;
    // 确定一块未映射的内存
    assert(VMM::get_mmap(VMM::get_pgd(), (void *)va, nullptr) == 0);
    // 映射
    VMM::mmap(VMM::get_pgd(), (void *)va, (void *)pa,
              VMM_PAGE_READABLE | VMM_PAGE_WRITABLE);
    assert(VMM::get_mmap(VMM::get_pgd(), (void *)va, &addr) == 1);
    assert(addr == pa);
    // 写测试
    *(uintptr_t *)va = 0xCD;
    //取消映射
    VMM::unmmap(VMM::get_pgd(), (void *)va);
    assert(VMM::get_mmap(VMM::get_pgd(), (void *)va, &addr) == 0);
    assert(addr == (uintptr_t) nullptr);
    printf("vmm test done.\n");
    return 0;
}
