/*
 * Memory DLL loading code
 * Version 0.0.4
 *
 * 版权信息：2004 - 2015年，由Joachim Bauch创作，邮箱为mail@joachim-bauch.de
 * 网站：http://www.joachim-bauch.de
 * 本文件内容遵循Mozilla Public License Version 2.0许可协议
 *
 * The Original Code is MemoryModule.c
 * 原始代码是MemoryModule.c
 * The Initial Developer of the Original Code is Joachim Bauch.
 * 原始代码的初始开发者是Joachim Bauch。
 * Portions created by Joachim Bauch are Copyright (C) 2004 - 2015
 * Joachim Bauch. All Rights Reserved.
 *
 * THeller: 在MemoryGetProcAddress函数中添加了二分查找（#define USE_BINARY_SEARCH可启用）。
 * 对于导出大量函数的库，这能显著提升速度。这些部分版权归Thomas Heller (C) 2013所有。
 */

#include <windows.h>
#include <winnt.h>
#include <stddef.h>
#include <tchar.h>
#ifdef DEBUG_OUTPUT
#include <stdio.h>
#endif

#if _MSC_VER
// 禁用数据到函数指针转换的警告
#pragma warning(disable:4055)
// C4244: 从 'uintptr_t' 到 'DWORD' 的转换，可能丢失数据
#pragma warning(error: 4244)
// C4267: 从 'size_t' 到 'int' 的转换，可能丢失数据
#pragma warning(error: 4267)

#define inline __inline
#endif

#ifndef IMAGE_SIZEOF_BASE_RELOCATION
// Vista SDKs不再定义IMAGE_SIZEOF_BASE_RELOCATION，手动定义
#define IMAGE_SIZEOF_BASE_RELOCATION (sizeof(IMAGE_BASE_RELOCATION))
#endif

#ifdef _WIN64
#define HOST_MACHINE IMAGE_FILE_MACHINE_AMD64
#else
#define HOST_MACHINE IMAGE_FILE_MACHINE_I386
#endif

#include "MemoryModule.h"

// 用于存储导出函数名及其对应索引的结构体
struct ExportNameEntry {
    LPCSTR name;
    WORD idx;
};

// DLL入口点函数指针类型
typedef BOOL (WINAPI *DllEntryProc)(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved);
// EXE入口点函数指针类型
typedef int (WINAPI *ExeEntryProc)(void);

#ifdef _WIN64
// 用于管理64位系统中内存块链表的结构体
typedef struct POINTER_LIST {
    struct POINTER_LIST *next;
    void *address;
} POINTER_LIST;
#endif

// 内存模块结构体，用于管理加载到内存中的模块相关信息
typedef struct {
    PIMAGE_NT_HEADERS headers;
    unsigned char *codeBase;
    HCUSTOMMODULE *modules;
    int numModules;
    BOOL initialized;
    BOOL isDLL;
    BOOL isRelocated;
    CustomAllocFunc alloc;
    CustomFreeFunc free;
    CustomLoadLibraryFunc loadLibrary;
    CustomGetProcAddressFunc getProcAddress;
    CustomFreeLibraryFunc freeLibrary;
    struct ExportNameEntry *nameExportsTable;
    void *userdata;
    ExeEntryProc exeEntry;
    DWORD pageSize;
#ifdef _WIN64
    POINTER_LIST *blockedMemory;
#endif
} MEMORYMODULE, *PMEMORYMODULE;

// 用于存储节（section）最终处理数据的结构体
typedef struct {
    LPVOID address;
    LPVOID alignedAddress;
    SIZE_T size;
    DWORD characteristics;
    BOOL last;
} SECTIONFINALIZEDATA, *PSECTIONFINALIZEDATA;

// 获取模块头数据目录的宏
#define GET_HEADER_DICTIONARY(module, idx)  &(module)->headers->OptionalHeader.DataDirectory[idx]

// 将值按指定对齐值向下对齐
static inline uintptr_t
AlignValueDown(uintptr_t value, uintptr_t alignment) {
    return value & ~(alignment - 1);
}

// 将地址按指定对齐值向下对齐
static inline LPVOID
AlignAddressDown(LPVOID address, uintptr_t alignment) {
    return (LPVOID) AlignValueDown((uintptr_t) address, alignment);
}

// 将值按指定对齐值向上对齐
static inline size_t
AlignValueUp(size_t value, size_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

// 根据偏移量获取指针
static inline void*
OffsetPointer(void* data, ptrdiff_t offset) {
    return (void*) ((uintptr_t) data + offset);
}

// 输出最后一个错误信息
static inline void
OutputLastError(const char *msg)
{
#ifndef DEBUG_OUTPUT
    UNREFERENCED_PARAMETER(msg);
#else
    LPVOID tmp;
    char *tmpmsg;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&tmp, 0, NULL);
    tmpmsg = (char *)LocalAlloc(LPTR, strlen(msg) + strlen(tmp) + 3);
    sprintf(tmpmsg, "%s: %s", msg, tmp);
    OutputDebugString(tmpmsg);
    LocalFree(tmpmsg);
    LocalFree(tmp);
#endif
}

#ifdef _WIN64
// 释放64位系统中的指针链表
static void
FreePointerList(POINTER_LIST *head, CustomFreeFunc freeMemory, void *userdata)
{
    POINTER_LIST *node = head;
    while (node) {
        POINTER_LIST *next;
        freeMemory(node->address, 0, MEM_RELEASE, userdata);
        next = node->next;
        free(node);
        node = next;
    }
}
#endif

// 检查数据大小是否满足预期
static BOOL
CheckSize(size_t size, size_t expected) {
    if (size < expected) {
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }

    return TRUE;
}

// 复制节数据到新内存位置
static BOOL
CopySections(const unsigned char *data, size_t size, PIMAGE_NT_HEADERS old_headers, PMEMORYMODULE module)
{
    int i, section_size;
    unsigned char *codeBase = module->codeBase;
    unsigned char *dest;
    PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(module->headers);
    for (i = 0; i < module->headers->FileHeader.NumberOfSections; i++, section++) {
        if (section->SizeOfRawData == 0) {
            // 该节在DLL中不包含数据，但可能定义了未初始化数据
            section_size = old_headers->OptionalHeader.SectionAlignment;
            if (section_size > 0) {
                dest = (unsigned char *)module->alloc(codeBase + section->VirtualAddress,
                    section_size,
                    MEM_COMMIT,
                    PAGE_READWRITE,
                    module->userdata);
                if (dest == NULL) {
                    return FALSE;
                }

                // 始终使用文件中的位置以支持小于页面大小的对齐（上面的分配会对齐到页面大小）
                dest = codeBase + section->VirtualAddress;
                // 注意：在64位系统中，这里截断为32位，之后使用"PhysicalAddress"时会再次扩展
                section->Misc.PhysicalAddress = (DWORD) ((uintptr_t) dest & 0xffffffff);
                memset(dest, 0, section_size);
            }

            // 节为空，继续处理下一个节
            continue;
        }

        if (!CheckSize(size, section->PointerToRawData + section->SizeOfRawData)) {
            return FALSE;
        }

        // 提交内存块并从DLL复制数据
        dest = (unsigned char *)module->alloc(codeBase + section->VirtualAddress,
                            section->SizeOfRawData,
                            MEM_COMMIT,
                            PAGE_READWRITE,
                            module->userdata);
        if (dest == NULL) {
            return FALSE;
        }

        // 始终使用文件中的位置以支持小于页面大小的对齐（上面的分配会对齐到页面大小）
        dest = codeBase + section->VirtualAddress;
        memcpy(dest, data + section->PointerToRawData, section->SizeOfRawData);
        // 注意：在64位系统中，这里截断为32位，之后使用"PhysicalAddress"时会再次扩展
        section->Misc.PhysicalAddress = (DWORD) ((uintptr_t) dest & 0xffffffff);
    }

    return TRUE;
}

// 内存页面保护标志数组，对应可执行、可读、可写属性组合
static int ProtectionFlags[2][2][2] = {
    {
        // 不可执行
        {PAGE_NOACCESS, PAGE_WRITECOPY},
        {PAGE_READONLY, PAGE_READWRITE},
    }, {
        // 可执行
        {PAGE_EXECUTE, PAGE_EXECUTE_WRITECOPY},
        {PAGE_EXECUTE_READ, PAGE_EXECUTE_READWRITE},
    },
};

// 获取节的实际大小
static SIZE_T
GetRealSectionSize(PMEMORYMODULE module, PIMAGE_SECTION_HEADER section) {
    DWORD size = section->SizeOfRawData;
    if (size == 0) {
        if (section->Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA) {
            size = module->headers->OptionalHeader.SizeOfInitializedData;
        } else if (section->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA) {
            size = module->headers->OptionalHeader.SizeOfUninitializedData;
        }
    }
    return (SIZE_T) size;
}

// 完成节的最终处理，设置内存保护属性等
static BOOL
FinalizeSection(PMEMORYMODULE module, PSECTIONFINALIZEDATA sectionData) {
    DWORD protect, oldProtect;
    BOOL executable;
    BOOL readable;
    BOOL writeable;

    if (sectionData->size == 0) {
        return TRUE;
    }

    if (sectionData->characteristics & IMAGE_SCN_MEM_DISCARDABLE) {
        // 该节不再需要，可以安全释放
        if (sectionData->address == sectionData->alignedAddress &&
            (sectionData->last ||
             module->headers->OptionalHeader.SectionAlignment == module->pageSize ||
             (sectionData->size % module->pageSize) == 0)
           ) {
            // 只允许释放整个页面
            module->free(sectionData->address, sectionData->size, MEM_DECOMMIT, module->userdata);
        }
        return TRUE;
    }

    // 根据节特性确定保护标志
    executable = (sectionData->characteristics & IMAGE_SCN_MEM_EXECUTE)!= 0;
    readable =   (sectionData->characteristics & IMAGE_SCN_MEM_READ)!= 0;
    writeable =  (sectionData->characteristics & IMAGE_SCN_MEM_WRITE)!= 0;
    protect = ProtectionFlags[executable][readable][writeable];
    if (sectionData->characteristics & IMAGE_SCN_MEM_NOT_CACHED) {
        protect |= PAGE_NOCACHE;
    }

    // 更改内存访问标志
    if (VirtualProtect(sectionData->address, sectionData->size, protect, &oldProtect) == 0) {
        OutputLastError("Error protecting memory page");
        return FALSE;
    }

    return TRUE;
}

// 完成所有节的最终处理
static BOOL
FinalizeSections(PMEMORYMODULE module)
{
    int i;
    PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(module->headers);
#ifdef _WIN64
    // "PhysicalAddress" 可能在上面被截断为32位，在此扩展为64位
    uintptr_t imageOffset = ((uintptr_t) module->headers->OptionalHeader.ImageBase & 0xffffffff00000000);
#else
    static const uintptr_t imageOffset = 0;
#endif
    SECTIONFINALIZEDATA sectionData;
    sectionData.address = (LPVOID)((uintptr_t)section->Misc.PhysicalAddress | imageOffset);
    sectionData.alignedAddress = AlignAddressDown(sectionData.address, module->pageSize);
    sectionData.size = GetRealSectionSize(module, section);
    sectionData.characteristics = section->Characteristics;
    sectionData.last = FALSE;
    section++;

    // 遍历所有节并更改访问标志
    for (i = 1; i < module->headers->FileHeader.NumberOfSections; i++, section++) {
        LPVOID sectionAddress = (LPVOID)((uintptr_t)section->Misc.PhysicalAddress | imageOffset);
        LPVOID alignedAddress = AlignAddressDown(sectionAddress, module->pageSize);
        SIZE_T sectionSize = GetRealSectionSize(module, section);
        // 合并共享页面的所有节的访问标志
        // TODO(fancycode): 当前我们将尾随大节的标志与第一个小节的页面共享。这应该优化。
        if (sectionData.alignedAddress == alignedAddress || (uintptr_t) sectionData.address + sectionData.size > (uintptr_t) alignedAddress) {
            // 该节与前一个节共享页面
            if ((section->Characteristics & IMAGE_SCN_MEM_DISCARDABLE) == 0 || (sectionData.characteristics & IMAGE_SCN_MEM_DISCARDABLE) == 0) {
                sectionData.characteristics = (sectionData.characteristics | section->Characteristics) & ~IMAGE_SCN_MEM_DISCARDABLE;
            } else {
                sectionData.characteristics |= section->Characteristics;
            }
            sectionData.size = (((uintptr_t)sectionAddress) + ((uintptr_t) sectionSize)) - (uintptr_t) sectionData.address;
            continue;
        }

        if (!FinalizeSection(module, &sectionData)) {
            return FALSE;
        }
        sectionData.address = sectionAddress;
        sectionData.alignedAddress = alignedAddress;
        sectionData.size = sectionSize;
        sectionData.characteristics = section->Characteristics;
    }
    sectionData.last = TRUE;
    if (!FinalizeSection(module, &sectionData)) {
        return FALSE;
    }
    return TRUE;
}

// 执行线程本地存储（TLS）回调
static BOOL
ExecuteTLS(PMEMORYMODULE module)
{
    unsigned char *codeBase = module->codeBase;
    PIMAGE_TLS_DIRECTORY tls;
    PIMAGE_TLS_CALLBACK* callback;

    PIMAGE_DATA_DIRECTORY directory = GET_HEADER_DICTIONARY(module, IMAGE_DIRECTORY_ENTRY_TLS);
    if (directory->VirtualAddress == 0) {
        return TRUE;
    }

    tls = (PIMAGE_TLS_DIRECTORY) (codeBase + directory->VirtualAddress);
    callback = (PIMAGE_TLS_CALLBACK *) tls->AddressOfCallBacks;
    if (callback) {
        while (*callback) {
            (*callback)((LPVOID) codeBase, DLL_PROCESS_ATTACH, NULL);
            callback++;
        }
    }
    return TRUE;
}

// 执行基址重定位
static BOOL
PerformBaseRelocation(PMEMORYMODULE module, ptrdiff_t delta)
{
    unsigned char *codeBase = module->codeBase;
    PIMAGE_BASE_RELOCATION relocation;

    PIMAGE_DATA_DIRECTORY directory = GET_HEADER_DICTIONARY(module, IMAGE_DIRECTORY_ENTRY_BASERELOC);
    if (directory->Size == 0) {
        return (delta == 0);
    }

    relocation = (PIMAGE_BASE_RELOCATION) (codeBase + directory->VirtualAddress);
    for (; relocation->VirtualAddress > 0; ) {
        DWORD i;
        unsigned char *dest = codeBase + relocation->VirtualAddress;
        unsigned short *relInfo = (unsigned short*) OffsetPointer(relocation, IMAGE_SIZEOF_BASE_RELOCATION);
        for (i = 0; i < ((relocation->SizeOfBlock - IMAGE_SIZEOF_BASE_RELOCATION) / 2); i++, relInfo++) {
            // 高4位定义重定位类型
            int type = *relInfo >> 12;
            // 低12位定义偏移量
            int offset = *relInfo & 0xfff;

            switch (type)
            {
            case IMAGE_REL_BASED_ABSOLUTE:
                // 跳过此重定位
                break;

            case IMAGE_REL_BASED_HIGHLOW:
                // 更改完整的32位地址
                {
                    DWORD *patchAddrHL = (DWORD *) (dest + offset);
                    *patchAddrHL += (DWORD) delta;
                }
                break;

#ifdef _WIN64
            case IMAGE_REL_BASED_DIR64:
                {
                    ULONGLONG *patchAddr64 = (ULONGLONG *) (dest + offset);
                    *patchAddr64 += (ULONGLONG) delta;
                }
                break;
#endif

            default:
                //printf("Unknown relocation: %d\n", type);
                break;
            }
        }

        // 前进到下一个重定位块
        relocation = (PIMAGE_BASE_RELOCATION) OffsetPointer(relocation, relocation->SizeOfBlock);
    }
    return TRUE;
}

//
// 构建导入表函数
static BOOL
BuildImportTable(PMEMORYMODULE module)
{
    unsigned char *codeBase = module->codeBase;
    PIMAGE_IMPORT_DESCRIPTOR importDesc;
    BOOL result = TRUE;

    // 获取导入表数据目录
    PIMAGE_DATA_DIRECTORY directory = GET_HEADER_DICTIONARY(module, IMAGE_DIRECTORY_ENTRY_IMPORT);
    if (directory->Size == 0) {
        // 若导入表为空，说明没有外部依赖，直接返回成功
        return TRUE;
    }

    // 获取导入描述符起始位置
    importDesc = (PIMAGE_IMPORT_DESCRIPTOR) (codeBase + directory->VirtualAddress);
    // 遍历每个导入描述符，直到遇到无效指针或导入描述符的名称为空
    for (;!IsBadReadPtr(importDesc, sizeof(IMAGE_IMPORT_DESCRIPTOR)) && importDesc->Name; importDesc++) {
        uintptr_t *thunkRef;
        FARPROC *funcRef;
        HCUSTOMMODULE *tmp;
        // 使用自定义加载库函数加载依赖库
        HCUSTOMMODULE handle = module->loadLibrary((LPCSTR) (codeBase + importDesc->Name), module->userdata);
        if (handle == NULL) {
            // 加载失败，设置错误码，标记失败并跳出循环
            SetLastError(ERROR_MOD_NOT_FOUND);
            result = FALSE;
            break;
        }

        // 为存储加载的模块句柄的数组重新分配内存
        tmp = (HCUSTOMMODULE *) realloc(module->modules, (module->numModules + 1)*(sizeof(HCUSTOMMODULE)));
        if (tmp == NULL) {
            // 内存分配失败，释放已加载的库，设置错误码，标记失败并跳出循环
            module->freeLibrary(handle, module->userdata);
            SetLastError(ERROR_OUTOFMEMORY);
            result = FALSE;
            break;
        }
        module->modules = tmp;

        // 存储新加载的模块句柄
        module->modules[module->numModules++] = handle;
        if (importDesc->OriginalFirstThunk) {
            // 使用OriginalFirstThunk获取导入函数信息
            thunkRef = (uintptr_t *) (codeBase + importDesc->OriginalFirstThunk);
            funcRef = (FARPROC *) (codeBase + importDesc->FirstThunk);
        } else {
            // 没有OriginalFirstThunk，直接使用FirstThunk
            thunkRef = (uintptr_t *) (codeBase + importDesc->FirstThunk);
            funcRef = (FARPROC *) (codeBase + importDesc->FirstThunk);
        }
        for (; *thunkRef; thunkRef++, funcRef++) {
            if (IMAGE_SNAP_BY_ORDINAL(*thunkRef)) {
                // 通过序号获取函数地址
                *funcRef = module->getProcAddress(handle, (LPCSTR)IMAGE_ORDINAL(*thunkRef), module->userdata);
            } else {
                // 通过函数名获取函数地址
                PIMAGE_IMPORT_BY_NAME thunkData = (PIMAGE_IMPORT_BY_NAME) (codeBase + (*thunkRef));
                *funcRef = module->getProcAddress(handle, (LPCSTR)&thunkData->Name, module->userdata);
            }
            if (*funcRef == 0) {
                // 获取函数地址失败，标记失败并跳出循环
                result = FALSE;
                break;
            }
        }

        if (!result) {
            // 释放已加载的库，设置错误码，跳出循环
            module->freeLibrary(handle, module->userdata);
            SetLastError(ERROR_PROC_NOT_FOUND);
            break;
        }
    }

    return result;
}

// 默认内存分配函数
LPVOID MemoryDefaultAlloc(LPVOID address, SIZE_T size, DWORD allocationType, DWORD protect, void* userdata)
{
    // 未使用userdata参数
    UNREFERENCED_PARAMETER(userdata);
    // 使用VirtualAlloc进行内存分配
    return VirtualAlloc(address, size, allocationType, protect);
}

// 默认内存释放函数
BOOL MemoryDefaultFree(LPVOID lpAddress, SIZE_T dwSize, DWORD dwFreeType, void* userdata)
{
    // 未使用userdata参数
    UNREFERENCED_PARAMETER(userdata);
    // 使用VirtualFree释放内存
    return VirtualFree(lpAddress, dwSize, dwFreeType);
}

// 默认加载库函数
HCUSTOMMODULE MemoryDefaultLoadLibrary(LPCSTR filename, void *userdata)
{
    HMODULE result;
    // 未使用userdata参数
    UNREFERENCED_PARAMETER(userdata);
    // 使用LoadLibraryA加载库
    result = LoadLibraryA(filename);
    if (result == NULL) {
        return NULL;
    }

    return (HCUSTOMMODULE) result;
}

// 默认获取函数地址函数
FARPROC MemoryDefaultGetProcAddress(HCUSTOMMODULE module, LPCSTR name, void *userdata)
{
    // 未使用userdata参数
    UNREFERENCED_PARAMETER(userdata);
    // 使用GetProcAddress获取函数地址
    return (FARPROC) GetProcAddress((HMODULE) module, name);
}

// 默认释放库函数
void MemoryDefaultFreeLibrary(HCUSTOMMODULE module, void *userdata)
{
    // 未使用userdata参数
    UNREFERENCED_PARAMETER(userdata);
    // 使用FreeLibrary释放库
    FreeLibrary((HMODULE) module);
}

/**
 * 加载库函数，使用默认的分配、释放等函数
 * @brief MemoryLoadLibrary
 * @param data
 * @param size
 * @return
 */
HMEMORYMODULE MemoryLoadLibrary(const void *data, size_t size)
{
    return MemoryLoadLibraryEx(data, size, MemoryDefaultAlloc, MemoryDefaultFree, MemoryDefaultLoadLibrary, MemoryDefaultGetProcAddress, MemoryDefaultFreeLibrary, NULL);
}

/**
 * 扩展的加载库函数
 * @brief MemoryLoadLibraryEx
 * @param data
 * @param size
 * @param allocMemory
 * @param freeMemory
 * @param loadLibrary
 * @param getProcAddress
 * @param freeLibrary
 * @param userdata
 * @return
 */
HMEMORYMODULE MemoryLoadLibraryEx(const void *data, size_t size,
    CustomAllocFunc allocMemory,
    CustomFreeFunc freeMemory,
    CustomLoadLibraryFunc loadLibrary,
    CustomGetProcAddressFunc getProcAddress,
    CustomFreeLibraryFunc freeLibrary,
    void *userdata)
{
    PMEMORYMODULE result = NULL;
    PIMAGE_DOS_HEADER dos_header;
    PIMAGE_NT_HEADERS old_header;
    unsigned char *code, *headers;
    ptrdiff_t locationDelta;
    SYSTEM_INFO sysInfo;
    PIMAGE_SECTION_HEADER section;
    DWORD i;
    size_t optionalSectionSize;
    size_t lastSectionEnd = 0;
    size_t alignedImageSize;
#ifdef _WIN64
    POINTER_LIST *blockedMemory = NULL;
#endif

    // 检查数据大小是否至少包含IMAGE_DOS_HEADER
    if (!CheckSize(size, sizeof(IMAGE_DOS_HEADER))) {
        return NULL;
    }
    dos_header = (PIMAGE_DOS_HEADER)data;
    // 检查是否是有效的DOS签名
    if (dos_header->e_magic!= IMAGE_DOS_SIGNATURE) {
        SetLastError(ERROR_BAD_EXE_FORMAT);
        return NULL;
    }

    // 检查数据大小是否包含PE头
    if (!CheckSize(size, dos_header->e_lfanew + sizeof(IMAGE_NT_HEADERS))) {
        return NULL;
    }
    old_header = (PIMAGE_NT_HEADERS)&((const unsigned char *)(data))[dos_header->e_lfanew];
    // 检查是否是有效的PE签名
    if (old_header->Signature!= IMAGE_NT_SIGNATURE) {
        SetLastError(ERROR_BAD_EXE_FORMAT);
        return NULL;
    }

    // 检查机器类型是否匹配主机
    if (old_header->FileHeader.Machine!= HOST_MACHINE) {
        SetLastError(ERROR_BAD_EXE_FORMAT);
        return NULL;
    }

    // 检查节对齐是否是2的倍数
    if (old_header->OptionalHeader.SectionAlignment & 1) {
        SetLastError(ERROR_BAD_EXE_FORMAT);
        return NULL;
    }

    section = IMAGE_FIRST_SECTION(old_header);
    optionalSectionSize = old_header->OptionalHeader.SectionAlignment;
    for (i = 0; i < old_header->FileHeader.NumberOfSections; i++, section++) {
        size_t endOfSection;
        if (section->SizeOfRawData == 0) {
            // 该节没有原始数据，计算虚拟地址加上对齐大小作为结束位置
            endOfSection = section->VirtualAddress + optionalSectionSize;
        } else {
            // 该节有原始数据，计算虚拟地址加上数据大小作为结束位置
            endOfSection = section->VirtualAddress + section->SizeOfRawData;
        }

        if (endOfSection > lastSectionEnd) {
            lastSectionEnd = endOfSection;
        }
    }

    // 获取系统信息，获取页面大小
    GetNativeSystemInfo(&sysInfo);
    // 按页面大小向上对齐镜像大小
    alignedImageSize = AlignValueUp(old_header->OptionalHeader.SizeOfImage, sysInfo.dwPageSize);
    // 检查计算的对齐大小是否与最后一节结束位置的对齐大小一致
    if (alignedImageSize!= AlignValueUp(lastSectionEnd, sysInfo.dwPageSize)) {
        SetLastError(ERROR_BAD_EXE_FORMAT);
        return NULL;
    }

    // 尝试在指定的镜像基址分配内存
    code = (unsigned char *)allocMemory((LPVOID)(old_header->OptionalHeader.ImageBase),
        alignedImageSize,
        MEM_RESERVE | MEM_COMMIT,
        PAGE_READWRITE,
        userdata);

    if (code == NULL) {
        // 若指定基址分配失败，尝试在任意位置分配内存
        code = (unsigned char *)allocMemory(NULL,
            alignedImageSize,
            MEM_RESERVE | MEM_COMMIT,
            PAGE_READWRITE,
            userdata);
        if (code == NULL) {
            SetLastError(ERROR_OUTOFMEMORY);
            return NULL;
        }
    }

#ifdef _WIN64
    // 在64位系统下，确保内存块不跨越4GB边界
    while ((((uintptr_t) code) >> 32) < (((uintptr_t) (code + alignedImageSize)) >> 32)) {
        POINTER_LIST *node = (POINTER_LIST*) malloc(sizeof(POINTER_LIST));
        if (!node) {
            freeMemory(code, 0, MEM_RELEASE, userdata);
            FreePointerList(blockedMemory, freeMemory, userdata);
            SetLastError(ERROR_OUTOFMEMORY);
            return NULL;
        }

        node->next = blockedMemory;
        node->address = code;
        blockedMemory = node;

        code = (unsigned char *)allocMemory(NULL,
            alignedImageSize,
            MEM_RESERVE | MEM_COMMIT,
            PAGE_READWRITE,
            userdata);
        if (code == NULL) {
            FreePointerList(blockedMemory, freeMemory, userdata);
            SetLastError(ERROR_OUTOFMEMORY);
            return NULL;
        }
    }
#endif

    // 分配内存模块结构体
    result = (PMEMORYMODULE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MEMORYMODULE));
    if (result == NULL) {
        freeMemory(code, 0, MEM_RELEASE, userdata);
#ifdef _WIN64
        FreePointerList(blockedMemory, freeMemory, userdata);
#endif
        SetLastError(ERROR_OUTOFMEMORY);
        return NULL;
    }

    result->codeBase = code;
    result->isDLL = (old_header->FileHeader.Characteristics & IMAGE_FILE_DLL)!= 0;
    result->alloc = allocMemory;
    result->free = freeMemory;
    result->loadLibrary = loadLibrary;
    result->getProcAddress = getProcAddress;
    result->freeLibrary = freeLibrary;
    result->userdata = userdata;
    result->pageSize = sysInfo.dwPageSize;
#ifdef _WIN64
    result->blockedMemory = blockedMemory;
#endif

    // 检查数据大小是否包含头信息
    if (!CheckSize(size, old_header->OptionalHeader.SizeOfHeaders)) {
        goto error;
    }

    // 提交内存用于存放头信息
    headers = (unsigned char *)allocMemory(code,
        old_header->OptionalHeader.SizeOfHeaders,
        MEM_COMMIT,
        PAGE_READWRITE,
        userdata);

    // 复制DOS头到新内存位置
    memcpy(headers, dos_header, old_header->OptionalHeader.SizeOfHeaders);
    result->headers = (PIMAGE_NT_HEADERS)&((const unsigned char *)(headers))[dos_header->e_lfanew];

    // 更新头信息中的镜像基址
    result->headers->OptionalHeader.ImageBase = (uintptr_t)code;

    // 复制节数据到新内存位置
    if (!CopySections((const unsigned char *) data, size, old_header, result)) {
        goto error;
    }

    // 计算基址偏移量
    locationDelta = (ptrdiff_t)(result->headers->OptionalHeader.ImageBase - old_header->OptionalHeader.ImageBase);
    if (locationDelta!= 0) {
        result->isRelocated = PerformBaseRelocation(result, locationDelta);
    } else {
        result->isRelocated = TRUE;
    }

    // 构建导入表
    if (!BuildImportTable(result)) {
        goto error;
    }

    // 根据节头信息标记内存页，并释放可丢弃的节
    if (!FinalizeSections(result)) {
        goto error;
    }

    // 执行TLS回调
    if (!ExecuteTLS(result)) {
        goto error;
    }

    // 获取加载库的入口点
    if (result->headers->OptionalHeader.AddressOfEntryPoint!= 0) {
        if (result->isDLL) {
            DllEntryProc DllEntry = (DllEntryProc)(LPVOID)(code + result->headers->OptionalHeader.AddressOfEntryPoint);
            // 通知DLL附加到进程
            BOOL successfull = (*DllEntry)((HINSTANCE)code, DLL_PROCESS_ATTACH, 0);
            if (!successfull) {
                SetLastError(ERROR_DLL_INIT_FAILED);
                goto error;
            }
            result->initialized = TRUE;
        } else {
            result->exeEntry = (ExeEntryProc)(LPVOID)(code + result->headers->OptionalHeader.AddressOfEntryPoint);
        }
    } else {
        result->exeEntry = NULL;
    }

    return (HMEMORYMODULE)result;

error:
    // 清理资源
    MemoryFreeLibrary(result);
    return NULL;
}

// 用于比较导出函数名的函数
static int _compare(const void *a, const void *b)
{
    const struct ExportNameEntry *p1 = (const struct ExportNameEntry*) a;
    const struct ExportNameEntry *p2 = (const struct ExportNameEntry*) b;
    return strcmp(p1->name, p2->name);
}

// 用于查找导出函数名的函数
static int _find(const void *a, const void *b)
{
    LPCSTR *name = (LPCSTR *) a;
    const struct ExportNameEntry *p = (const struct ExportNameEntry*) b;
    return strcmp(*name, p->name);
}

/**
 *  获取函数地址函数
 * @brief MemoryGetProcAddress
 * @param mod
 * @param name
 * @return
 */
FARPROC MemoryGetProcAddress(HMEMORYMODULE mod, LPCSTR name) {
    PMEMORYMODULE module = (PMEMORYMODULE)mod;
    unsigned char *codeBase = module->codeBase;
    DWORD idx = 0;
    PIMAGE_EXPORT_DIRECTORY exports;
    PIMAGE_DATA_DIRECTORY directory = GET_HEADER_DICTIONARY(module, IMAGE_DIRECTORY_ENTRY_EXPORT);
    // 检查导出表数据目录大小，如果为0，说明没有导出表
    if (directory->Size == 0) {
        // 没有找到导出表，设置相应错误码
        SetLastError(ERROR_PROC_NOT_FOUND);
        return NULL;
    }

    // 获取导出目录的起始位置
    exports = (PIMAGE_EXPORT_DIRECTORY)(codeBase + directory->VirtualAddress);
    // 检查导出函数数量和名称数量，如果都为0，说明DLL没有导出任何函数
    if (exports->NumberOfNames == 0 || exports->NumberOfFunctions == 0) {
        SetLastError(ERROR_PROC_NOT_FOUND);
        return NULL;
    }

    // 判断传入的函数名是否是按序号指定的（高16位为0）
    if (HIWORD(name) == 0) {
        // 通过序号加载函数
        // 检查序号是否小于导出表的起始序号
        if (LOWORD(name) < exports->Base) {
            SetLastError(ERROR_PROC_NOT_FOUND);
            return NULL;
        }
        // 计算实际的索引值，序号减去起始序号
        idx = LOWORD(name) - exports->Base;
    } else if (!exports->NumberOfNames) {
        // 如果传入的不是按序号指定且没有导出函数名，设置错误码
        SetLastError(ERROR_PROC_NOT_FOUND);
        return NULL;
    } else {
        const struct ExportNameEntry *found;
        // 如果还没有构建导出函数名表
        if (!module->nameExportsTable) {
            DWORD i;
            // 获取函数名数组的指针
            DWORD *nameRef = (DWORD *)(codeBase + exports->AddressOfNames);
            // 获取序号数组的指针
            WORD *ordinal = (WORD *)(codeBase + exports->AddressOfNameOrdinals);
            // 分配内存用于存储导出函数名和对应的序号
            struct ExportNameEntry *entry = (struct ExportNameEntry*)malloc(exports->NumberOfNames * sizeof(struct ExportNameEntry));
            module->nameExportsTable = entry;
            if (!entry) {
                // 内存分配失败，设置错误码
                SetLastError(ERROR_OUTOFMEMORY);
                return NULL;
            }
            // 遍历每个导出函数，填充函数名和序号
            for (i = 0; i < exports->NumberOfNames; i++, nameRef++, ordinal++, entry++) {
                entry->name = (const char *)(codeBase + (*nameRef));
                entry->idx = *ordinal;
            }
            // 使用qsort对导出函数名表按名称排序，方便后续二分查找
            qsort(module->nameExportsTable,
                    exports->NumberOfNames,
                    sizeof(struct ExportNameEntry), _compare);
        }

        // 使用二分查找在导出函数名表中查找指定函数名
        found = (const struct ExportNameEntry*)bsearch(&name,
                module->nameExportsTable,
                exports->NumberOfNames,
                sizeof(struct ExportNameEntry), _find);
        if (!found) {
            // 没找到导出的符号，设置错误码
            SetLastError(ERROR_PROC_NOT_FOUND);
            return NULL;
        }
        // 获取找到的函数对应的索引
        idx = found->idx;
    }

    // 检查索引是否超过导出函数数量，若超过则不匹配
    if (idx > exports->NumberOfFunctions) {
        SetLastError(ERROR_PROC_NOT_FOUND);
        return NULL;
    }

    // AddressOfFunctions数组存储着实际函数的相对虚拟地址，计算并返回函数地址
    return (FARPROC)(LPVOID)(codeBase + (*(DWORD *)(codeBase + exports->AddressOfFunctions + (idx * 4))));
}

/**
 * 释放库函数
 * @brief MemoryFreeLibrary
 * @param mod
 */
void MemoryFreeLibrary(HMEMORYMODULE mod) {
    PMEMORYMODULE module = (PMEMORYMODULE)mod;

    if (module == NULL) {
        return;
    }
    // 如果库已经初始化，调用DllEntry函数通知库从进程中分离
    if (module->initialized) {
        DllEntryProc DllEntry = (DllEntryProc)(LPVOID)(module->codeBase + module->headers->OptionalHeader.AddressOfEntryPoint);
        (*DllEntry)((HINSTANCE)module->codeBase, DLL_PROCESS_DETACH, 0);
    }

    // 释放导出函数名表的内存
    free(module->nameExportsTable);
    if (module->modules!= NULL) {
        // 释放之前打开的所有依赖库
        int i;
        for (i = 0; i < module->numModules; i++) {
            if (module->modules[i]!= NULL) {
                // 调用自定义的释放库函数
                module->freeLibrary(module->modules[i], module->userdata);
            }
        }
        // 释放存储依赖库句柄的数组内存
        free(module->modules);
    }

    if (module->codeBase!= NULL) {
        // 释放库占用的内存
        module->free(module->codeBase, 0, MEM_RELEASE, module->userdata);
    }

#ifdef _WIN64
    // 在64位系统下，释放被阻塞的内存块链表
    FreePointerList(module->blockedMemory, module->free, module->userdata);
#endif
    // 释放内存模块结构体本身占用的堆内存
    HeapFree(GetProcessHeap(), 0, module);
}

// 调用入口点函数
int MemoryCallEntryPoint(HMEMORYMODULE mod) {
    PMEMORYMODULE module = (PMEMORYMODULE)mod;
    // 检查模块是否为空、是否是DLL、是否有入口点函数以及是否已经重定位，只要有一个条件不满足就返回 -1
    if (module == NULL || module->isDLL || module->exeEntry == NULL ||!module->isRelocated) {
        return -1;
    }

    // 调用入口点函数并返回结果
    return module->exeEntry();
}

#define DEFAULT_LANGUAGE        MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL)

// 查找资源函数，使用默认语言
HMEMORYRSRC MemoryFindResource(HMEMORYMODULE module, LPCTSTR name, LPCTSTR type) {
    return MemoryFindResourceEx(module, name, type, DEFAULT_LANGUAGE);
}

// 内部函数：在资源目录中搜索资源项
static PIMAGE_RESOURCE_DIRECTORY_ENTRY _MemorySearchResourceEntry(
    void *root,
    PIMAGE_RESOURCE_DIRECTORY resources,
    LPCTSTR key) {
    PIMAGE_RESOURCE_DIRECTORY_ENTRY entries = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(resources + 1);
    PIMAGE_RESOURCE_DIRECTORY_ENTRY result = NULL;
    DWORD start;
    DWORD end;
    DWORD middle;

    // 如果传入的不是资源ID，且是以'#'开头的字符串形式的资源ID
    if (!IS_INTRESOURCE(key) && key[0] == TEXT('#')) {
        TCHAR *endpos = NULL;
        // 将字符串形式的资源ID转为整数
        long int tmpkey = (WORD)_tcstol((TCHAR *)&key[1], &endpos, 10);
        // 检查转换是否成功且没有多余字符
        if (tmpkey <= 0xffff && lstrlen(endpos) == 0) {
            key = MAKEINTRESOURCE(tmpkey);
        }
    }

    // 资源项按序存储，先有命名项，再有ID项，可以用二分查找加速搜索
    if (IS_INTRESOURCE(key)) {
        WORD check = (WORD)(uintptr_t)key;
        start = resources->NumberOfNamedEntries;
        end = start + resources->NumberOfIdEntries;

        while (end > start) {
            WORD entryName;
            middle = (start + end) >> 1;
            entryName = (WORD)entries[middle].Name;
            if (check < entryName) {
                end = (end!= middle? middle : middle - 1);
            } else if (check > entryName) {
                start = (start!= middle? middle : middle + 1);
            } else {
                result = &entries[middle];
                break;
            }
        }
    } else {
        LPCWSTR searchKey;
        size_t searchKeyLen = _tcslen(key);
#if defined(UNICODE)
        searchKey = key;
#else
        // 资源名总是用16位字符存储，非Unicode环境下需转换字符串
#define MAX_LOCAL_KEY_LENGTH 2048
        wchar_t _searchKeySpace[MAX_LOCAL_KEY_LENGTH + 1];
        LPWSTR _searchKey;
        if (searchKeyLen > MAX_LOCAL_KEY_LENGTH) {
            size_t _searchKeySize = (searchKeyLen + 1) * sizeof(wchar_t);
            _searchKey = (LPWSTR)malloc(_searchKeySize);
            if (_searchKey == NULL) {
                SetLastError(ERROR_OUTOFMEMORY);
                return NULL;
            }
        } else {
            _searchKey = &_searchKeySpace[0];
        }

        mbstowcs(_searchKey, key, searchKeyLen);
        _searchKey[searchKeyLen] = 0;
        searchKey = _searchKey;
#endif
        start = 0;
        end = resources->NumberOfNamedEntries;
        while (end > start) {
            int cmp;
            PIMAGE_RESOURCE_DIR_STRING_U resourceString;
            middle = (start + end) >> 1;
            resourceString = (PIMAGE_RESOURCE_DIR_STRING_U)OffsetPointer(root, entries[middle].Name & 0x7FFFFFFF);
            cmp = _wcsnicmp(searchKey, resourceString->NameString, resourceString->Length);
            if (cmp == 0) {
                // 处理部分匹配情况
                if (searchKeyLen > resourceString->Length) {
                    cmp = 1;
                } else if (searchKeyLen < resourceString->Length) {
                    cmp = -1;
                }
            }
            if (cmp < 0) {
                end = (middle!= end? middle : middle - 1);
            } else if (cmp > 0) {
                start = (middle!= start? middle : middle + 1);
            } else {
                result = &entries[middle];
                break;
            }
        }
#if!defined(UNICODE)
        if (searchKeyLen > MAX_LOCAL_KEY_LENGTH) {
            free(_searchKey);
        }
#undef MAX_LOCAL_KEY_LENGTH
#endif
    }

    return result;
}

// 查找资源扩展函数，可指定语言
HMEMORYRSRC MemoryFindResourceEx(HMEMORYMODULE module, LPCTSTR name, LPCTSTR type, WORD language) {
    unsigned char *codeBase = ((PMEMORYMODULE) module)->codeBase;
    PIMAGE_DATA_DIRECTORY directory = GET_HEADER_DICTIONARY((PMEMORYMODULE) module, IMAGE_DIRECTORY_ENTRY_RESOURCE);
    PIMAGE_RESOURCE_DIRECTORY rootResources;
    PIMAGE_RESOURCE_DIRECTORY nameResources;
    PIMAGE_RESOURCE_DIRECTORY typeResources;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY foundType;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY foundName;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY foundLanguage;
    // 检查资源表数据目录大小，若为0，说明没有资源表
    if (directory->Size == 0) {
        SetLastError(ERROR_RESOURCE_DATA_NOT_FOUND);
        return NULL;
    }

    if (language == DEFAULT_LANGUAGE) {
        // 使用当前线程的语言
        language = LANGIDFROMLCID(GetThreadLocale());
    }

    // 资源按三级树结构存储：类型、名称、语言
    rootResources = (PIMAGE_RESOURCE_DIRECTORY)(codeBase + directory->VirtualAddress);
    // 在根资源目录中查找指定类型的资源
    foundType = _MemorySearchResourceEntry(rootResources, rootResources, type);
    if (foundType == NULL) {
        SetLastError(ERROR_RESOURCE_TYPE_NOT_FOUND);
        return NULL;
    }

    // 获取指定类型资源的目录
    typeResources = (PIMAGE_RESOURCE_DIRECTORY)(codeBase + directory->VirtualAddress + (foundType->OffsetToData & 0x7fffffff));
    // 在类型资源目录中查找指定名称的资源
    foundName = _MemorySearchResourceEntry(rootResources, typeResources, name);
    if (foundName == NULL) {
        SetLastError(ERROR_RESOURCE_NAME_NOT_FOUND);
        return NULL;
    }

    // 获取指定名称资源的目录
    nameResources = (PIMAGE_RESOURCE_DIRECTORY)(codeBase + directory->VirtualAddress + (foundName->OffsetToData & 0x7fffffff));
    // 在名称资源目录中查找指定语言的资源
    foundLanguage = _MemorySearchResourceEntry(rootResources, nameResources, (LPCTSTR)(uintptr_t) language);
    if (foundLanguage == NULL) {
        // 如果请求的语言未找到，使用第一个可用的语言
        if (nameResources->NumberOfIdEntries == 0) {
            SetLastError(ERROR_RESOURCE_LANG_NOT_FOUND);
            return NULL;
        }
        foundLanguage = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(nameResources + 1);
    }

    // 返回找到的资源的起始位置
    return (codeBase + directory->VirtualAddress + (foundLanguage->OffsetToData & 0x7fffffff));
}

// 获取资源大小函数
DWORD MemorySizeofResource(HMEMORYMODULE module, HMEMORYRSRC resource) {
    PIMAGE_RESOURCE_DATA_ENTRY entry;
    // 未使用模块参数
    UNREFERENCED_PARAMETER(module);
    entry = (PIMAGE_RESOURCE_DATA_ENTRY) resource;
    if (entry == NULL) {
        return 0;
    }

    return entry->Size;
}

// 加载资源函数
LPVOID MemoryLoadResource(HMEMORYMODULE module, HMEMORYRSRC resource) {
    unsigned char *codeBase = ((PMEMORYMODULE) module)->codeBase;
    PIMAGE_RESOURCE_DATA_ENTRY entry = (PIMAGE_RESOURCE_DATA_ENTRY) resource;
    if (entry == NULL) {
        return NULL;
    }

    return codeBase + entry->OffsetToData;
}

// 加载字符串函数，使用默认语言
int
MemoryLoadString(HMEMORYMODULE module, UINT id, LPTSTR buffer, int maxsize) {
    return MemoryLoadStringEx(module, id, buffer, maxsize, DEFAULT_LANGUAGE);
}

// 加载字符串扩展函数，可指定语言
int
MemoryLoadStringEx(HMEMORYMODULE module, UINT id, LPTSTR buffer, int maxsize, WORD language) {
    HMEMORYRSRC resource;
    PIMAGE_RESOURCE_DIR_STRING_U data;
    DWORD size;

    // 检查目标缓冲区的最大容量是否为0，如果是，直接返回0，因为没有空间可供写入字符串
    if (maxsize == 0) {
        return 0;
    }

    // 查找字符串资源
    // 根据传入的模块、资源ID和语言，调用MemoryFindResourceEx函数查找对应的字符串资源
    // MAKEINTRESOURCE((id >> 4) + 1)是将传入的id经过位移运算后转换为资源ID的形式
    // RT_STRING表示要查找的资源类型是字符串
    resource = MemoryFindResourceEx(module, MAKEINTRESOURCE((id >> 4) + 1), RT_STRING, language);
    // 如果没找到对应的资源，将缓冲区首字符设为0（即空字符串），然后返回0
    if (resource == NULL) {
        buffer[0] = 0;
        return 0;
    }

    // 加载字符串资源数据
    // 找到资源后，调用MemoryLoadResource函数将资源数据加载到内存中
    // 并将加载后的资源数据起始位置赋值给data指针
    data = (PIMAGE_RESOURCE_DIR_STRING_U) MemoryLoadResource(module, resource);

    // 根据id的低4位，在资源数据里定位到目标字符串
    id = id & 0x0f;
    while (id--) {
        // 通过偏移指针，跳过前面的字符串数据，定位到目标字符串
        data = (PIMAGE_RESOURCE_DIR_STRING_U) OffsetPointer(data, (data->Length + 1) * sizeof(WCHAR));
    }

    // 检查目标字符串的长度是否为0，如果是，设置错误码，并将缓冲区首字符设为0，返回0
    if (data->Length == 0) {
        SetLastError(ERROR_RESOURCE_NAME_NOT_FOUND);
        buffer[0] = 0;
        return 0;
    }

    // 获取目标字符串的长度
    size = data->Length;
    // 如果字符串长度大于等于缓冲区最大容量，将实际写入长度设为缓冲区最大容量
    if (size >= (DWORD) maxsize) {
        size = maxsize;
    } else {
        // 否则，在缓冲区末尾添加字符串结束符0
        buffer[size] = 0;
    }

    // 根据是否定义了UNICODE宏，选择合适的字符串复制函数
    // 如果定义了UNICODE，使用宽字符复制函数wcsncpy
    // 否则，使用多字节字符转宽字符函数wcstombs进行复制
#if defined(UNICODE)
    wcsncpy(buffer, data->NameString, size);
#else
    wcstombs(buffer, data->NameString, size);
#endif

    // 返回实际复制到缓冲区的字符数量
    return size;
}

#ifdef TESTSUITE
#include <stdio.h>

#ifndef PRIxPTR
#ifdef _WIN64
#define PRIxPTR "I64x"
#else
#define PRIxPTR "x"
#endif
#endif

// 定义AlignValueDownTests数组，用于测试AlignValueDown函数
// 数组中的每个元素都是一个包含三个uintptr_t类型值的数组
// 分别表示测试输入值、对齐基数、预期输出值
static const uintptr_t AlignValueDownTests[][3] = {
    // 测试值16按16对齐，预期输出16
    {16, 16, 16},
    // 测试值17按16对齐，预期输出16
    {17, 16, 16},
    // 测试值32按16对齐，预期输出32
    {32, 16, 32},
    // 测试值33按16对齐，预期输出32
    {33, 16, 32},
#ifdef _WIN64
    // 64位系统下的测试值对齐测试
    {0x12345678abcd1000, 0x1000, 0x12345678abcd1000},
    {0x12345678abcd101f, 0x1000, 0x12345678abcd1000},
#endif
    // 测试值0按0对齐，预期输出0
    {0, 0, 0},
};

// 定义AlignValueUpTests数组，用于测试AlignValueUp函数
// 数组中的每个元素都是一个包含三个uintptr_t类型值的数组
// 分别表示测试输入值、对齐基数、预期输出值
static const uintptr_t AlignValueUpTests[][3] = {
    // 测试值16按16对齐，预期输出16
    {16, 16, 16},
    // 测试值17按16对齐，预期输出32
    {17, 16, 32},
    // 测试值32按16对齐，预期输出32
    {32, 16, 32},
    // 测试值33按16对齐，预期输出48
    {33, 16, 48},
#ifdef _WIN64
    // 64位系统下的测试值对齐测试
    {0x12345678abcd1000, 0x1000, 0x12345678abcd1000},
    {0x12345678abcd101f, 0x1000, 0x12345678abcd2000},
#endif
    // 测试值0按0对齐，预期输出0
    {0, 0, 0},
};

// MemoryModuleTestsuite函数用于测试AlignValueDown和AlignValueUp函数
BOOL MemoryModuleTestsuite() {
    BOOL success = TRUE;
    size_t idx;

    // 遍历AlignValueDownTests数组，测试AlignValueDown函数
    for (idx = 0; AlignValueDownTests[idx][0]; ++idx) {
        const uintptr_t* tests = AlignValueDownTests[idx];
        // 调用AlignValueDown函数，获取实际输出值
        uintptr_t value = AlignValueDown(tests[0], tests[1]);
        // 如果实际输出值与预期输出值不符，打印错误信息，并将success设为FALSE
        if (value!= tests[2]) {
            printf("AlignValueDown failed for 0x%" PRIxPTR "/0x%" PRIxPTR ": expected 0x%" PRIxPTR ", got 0x%" PRIxPTR "\n",
                tests[0], tests[1], tests[2], value);
            success = FALSE;
        }
    }

    // 遍历AlignValueUpTests数组，测试AlignValueUp函数
    for (idx = 0; AlignValueDownTests[idx][0]; ++idx) {
        const uintptr_t* tests = AlignValueUpTests[idx];
        // 调用AlignValueUp函数，获取实际输出值
        uintptr_t value = AlignValueUp(tests[0], tests[1]);
        // 如果实际输出值与预期输出值不符，打印错误信息，并将success设为FALSE
        if (value!= tests[2]) {
            printf("AlignValueUp failed for 0x%" PRIxPTR "/0x%" PRIxPTR ": expected 0x%" PRIxPTR ", got 0x%" PRIxPTR "\n",
                tests[0], tests[1], tests[2], value);
            success = FALSE;
        }
    }

    // 如果所有测试都通过，打印"OK"
    if (success) {
        printf("OK\n");
    }

    // 返回测试结果
    return success;
}
#endif
