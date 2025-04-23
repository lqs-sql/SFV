/*
 * Memory DLL loading code
 * Version 0.0.4
 *
 * 这段头文件代码实现了从内存中加载EXE/DLL相关的功能，由Joachim Bauch编写，包含了相应的版权信息。
 *
 * Copyright (c) 2004-2015 by Joachim Bauch / mail@joachim-bauch.de
 * http://www.joachim-bauch.de
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 2.0 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is MemoryModule.h
 *
 * The Initial Developer of the Original Code is Joachim Bauch.
 *
 * Portions created by Joachim Bauch are Copyright (C) 2004-2015
 * Joachim Bauch. All Rights Reserved.
 */

#ifndef __MEMORY_MODULE_HEADER
#define __MEMORY_MODULE_HEADER

#include <windows.h>

// 定义指向内存模块的句柄类型，用于后续操作内存中加载的模块
typedef void *HMEMORYMODULE;
// 定义指向内存资源的句柄类型，用于处理内存模块中的资源相关操作
typedef void *HMEMORYRSRC;
// 定义指向自定义模块的句柄类型，与自定义加载库相关的操作会用到
typedef void *HCUSTOMMODULE;

#ifdef __cplusplus
// 如果是C++代码环境，使用extern "C" 包裹后续函数声明，确保C++代码能正确调用这些函数，
// 因为这些函数采用的是C语言风格的调用约定
extern "C" {
#endif

// 自定义内存分配函数指针类型，用于分配加载库所需内存
// 参数依次为：指向内存分配起始位置的指针、分配大小、内存分配标志、保护属性、用户自定义数据指针
typedef LPVOID (*CustomAllocFunc)(LPVOID, SIZE_T, DWORD, DWORD, void*);
// 自定义内存释放函数指针类型，用于释放加载库占用的内存
// 参数依次为：指向要释放内存起始位置的指针、释放大小、内存释放标志、用户自定义数据指针
typedef BOOL (*CustomFreeFunc)(LPVOID, SIZE_T, DWORD, void*);
// 自定义加载库函数指针类型，用于加载额外的库
// 参数依次为：要加载库的名称字符串指针、用户自定义数据指针
typedef HCUSTOMMODULE (*CustomLoadLibraryFunc)(LPCSTR, void *);
// 自定义获取函数地址函数指针类型，用于获取导出函数的地址
// 参数依次为：自定义模块句柄、要获取地址的函数名或序号字符串指针、用户自定义数据指针
typedef FARPROC (*CustomGetProcAddressFunc)(HCUSTOMMODULE, LPCSTR, void *);
// 自定义释放库函数指针类型，用于释放已经加载的额外库
// 参数依次为：自定义模块句柄、用户自定义数据指针
typedef void (*CustomFreeLibraryFunc)(HCUSTOMMODULE, void *);

/**
 * Load EXE/DLL from memory location with the given size.
 * 从给定大小的内存位置加载EXE/DLL。
 * 此函数使用Windows API的默认LoadLibrary/GetProcAddress调用来解析所有依赖项。
 */
HMEMORYMODULE MemoryLoadLibrary(const void *, size_t);

/**
 * Load EXE/DLL from memory location with the given size using custom dependency
 * resolvers.
 * 使用自定义依赖解析器从给定大小的内存位置加载EXE/DLL。
 * 依赖项将使用传递的回调方法进行解析。
 */
HMEMORYMODULE MemoryLoadLibraryEx(const void *, size_t,
    CustomAllocFunc,
    CustomFreeFunc,
    CustomLoadLibraryFunc,
    CustomGetProcAddressFunc,
    CustomFreeLibraryFunc,
    void *);

/**
 * Get address of exported method. Supports loading both by name and by
 * ordinal value.
 * 获取导出方法的地址。支持通过名称和序号值两种方式加载。
 */
FARPROC MemoryGetProcAddress(HMEMORYMODULE, LPCSTR);

/**
 * Free previously loaded EXE/DLL.
 * 释放先前加载的EXE/DLL。
 */
void MemoryFreeLibrary(HMEMORYMODULE);

/**
 * Execute entry point (EXE only). The entry point can only be executed
 * if the EXE has been loaded to the correct base address or it could
 * be relocated (i.e. relocation information have not been stripped by
 * the linker).
 * 执行入口点（仅适用于EXE）。只有当EXE已加载到正确的基地址，或者它可以重定位（即链接器未剥离重定位信息）时，
 * 入口点才能被执行。
 * Important: calling this function will not return, i.e. once the loaded
 * EXE finished running, the process will terminate.
 * 重要提示：调用此函数将不会返回，也就是说，一旦加载的EXE运行完毕，进程将终止。
 * Returns a negative value if the entry point could not be executed.
 * 如果无法执行入口点，则返回负值。
 */
int MemoryCallEntryPoint(HMEMORYMODULE);

/**
 * Find the location of a resource with the specified type and name.
 * 查找具有指定类型和名称的资源位置。
 */
HMEMORYRSRC MemoryFindResource(HMEMORYMODULE, LPCTSTR, LPCTSTR);

/**
 * Find the location of a resource with the specified type, name and language.
 * 查找具有指定类型、名称和语言的资源位置。
 */
HMEMORYRSRC MemoryFindResourceEx(HMEMORYMODULE, LPCTSTR, LPCTSTR, WORD);

/**
 * Get the size of the resource in bytes.
 * 获取资源的字节大小。
 */
DWORD MemorySizeofResource(HMEMORYMODULE, HMEMORYRSRC);

/**
 * Get a pointer to the contents of the resource.
 * 获取指向资源内容的指针。
 */
LPVOID MemoryLoadResource(HMEMORYMODULE, HMEMORYRSRC);

/**
 * Load a string resource.
 * 加载字符串资源。
 */
int MemoryLoadString(HMEMORYMODULE, UINT, LPTSTR, int);

/**
 * Load a string resource with a given language.
 * 加载具有给定语言的字符串资源。
 */
int MemoryLoadStringEx(HMEMORYMODULE, UINT, LPTSTR, int, WORD);

/**
* Default implementation of CustomAllocFunc that calls VirtualAlloc
* internally to allocate memory for a library
* 这是CustomAllocFunc的默认实现，内部调用VirtualAlloc为库分配内存。
* This is the default as used by MemoryLoadLibrary.
* 这是MemoryLoadLibrary使用的默认实现。
*/
LPVOID MemoryDefaultAlloc(LPVOID, SIZE_T, DWORD, DWORD, void *);

/**
* Default implementation of CustomFreeFunc that calls VirtualFree
* internally to free the memory used by a library
* 这是CustomFreeFunc的默认实现，内部调用VirtualFree释放库使用的内存。
* This is the default as used by MemoryLoadLibrary.
* 这是MemoryLoadLibrary使用的默认实现。
*/
BOOL MemoryDefaultFree(LPVOID, SIZE_T, DWORD, void *);

/**
 * Default implementation of CustomLoadLibraryFunc that calls LoadLibraryA
 * internally to load an additional libary.
 * 这是CustomLoadLibraryFunc的默认实现，内部调用LoadLibraryA加载额外的库。
 * This is the default as used by MemoryLoadLibrary.
 * 这是MemoryLoadLibrary使用的默认实现。
 */
HCUSTOMMODULE MemoryDefaultLoadLibrary(LPCSTR, void *);

/**
 * Default implementation of CustomGetProcAddressFunc that calls GetProcAddress
 * internally to get the address of an exported function.
 * 这是CustomGetProcAddressFunc的默认实现，内部调用GetProcAddress获取导出函数的地址。
 * This is the default as used by MemoryLoadLibrary.
 * 这是MemoryLoadLibrary使用的默认实现。
 */
FARPROC MemoryDefaultGetProcAddress(HCUSTOMMODULE, LPCSTR, void *);

/**
 * Default implementation of CustomFreeLibraryFunc that calls FreeLibrary
 * internally to release an additional libary.
 * 这是CustomFreeLibraryFunc的默认实现，内部调用FreeLibrary释放额外的库。
 * This是MemoryLoadLibrary使用的默认实现。
 */
void MemoryDefaultFreeLibrary(HCUSTOMMODULE, void *);

#ifdef __cplusplus
}
#endif

#endif  // __MEMORY_MODULE_HEADER