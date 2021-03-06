/*********************************************************************************************************************************************************************************************************************************************************************************************
#   Intel® Single Event API
#
#   This file is provided under the BSD 3-Clause license.
#   Copyright (c) 2015, Intel Corporation
#   All rights reserved.
#
#   Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
#       Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
#       Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
#       Neither the name of the Intel Corporation nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
#
#   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
#   IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
#   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
**********************************************************************************************************************************************************************************************************************************************************************************************/

#pragma once
#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <dlfcn.h>
#endif
#include <string>
#include "IttNotifyStdSrc.h"
#include "ittnotify.h"

inline size_t GetMemPageSize()
{
#ifdef _WIN32
    SYSTEM_INFO si = {};
    GetSystemInfo(&si);
    return si.dwAllocationGranularity;
#else
    return sysconf(_SC_PAGE_SIZE);
#endif
}

class CMemMap
{
    CMemMap(const CMemMap&) = delete;
    CMemMap& operator = (const CMemMap&) = delete;
public:

    CMemMap(const std::string& path, size_t size, size_t offset = 0);

    void* Remap(size_t size, size_t offset = 0);

    void* GetPtr()
    {
        return m_pView;
    }
    size_t GetSize(){return m_size;}

    void Unmap();

    void Resize(size_t size)
    {
        Unmap();
#ifdef _WIN32
        //resize
        LARGE_INTEGER liSize = {};
        liSize.QuadPart = size;
        SetFilePointerEx(m_hFile, liSize, nullptr, FILE_BEGIN);
        ::SetEndOfFile(m_hFile);
#else
        ftruncate(m_fdin, size);
#endif
    }

    ~CMemMap()
    {
        Unmap();
#ifdef _WIN32
        if (m_hMapping)
        {
            CloseHandle(m_hMapping);
        }
        if (m_hFile)
        {
            CloseHandle(m_hFile);
        }
#else
        if (m_fdin)
        {
            close(m_fdin);
        }
#endif
    }
protected:
#ifdef _WIN32
    HANDLE m_hFile = nullptr;
    HANDLE m_hMapping = nullptr;
#else
    int m_fdin = 0;
#endif
    size_t m_size = 0;
    void* m_pView = nullptr;
};

class CRecorder
{
    CRecorder(const CRecorder&) = delete;
    CRecorder& operator = (const CRecorder&) = delete;
public:
    CRecorder();
    void Init(const std::string& path, uint64_t time, void* pCut);
    size_t CheckCapacity(size_t size);
    void* Allocate(size_t size);
    uint64_t GetCount(){return m_counter;}
    uint64_t GetCreationTime(){return m_time;}
    void Close();
    inline bool SameCut(void* pCut){return pCut == m_pCut;}
    ~CRecorder();
protected:
    std::unique_ptr<CMemMap> m_memmap;
    size_t m_nWroteTotal = 0;
    void* volatile m_pCurPos;
    uint64_t m_time = 0;
    uint64_t m_counter = 0;
    void* m_pCut = nullptr;
};


enum class ERecordType: uint8_t
{
    BeginTask,
    EndTask,
    BeginOverlappedTask,
    EndOverlappedTask,
    Metadata,
    Marker,
    Counter,
    BeginFrame,
    EndFrame,
    ObjectNew,
    ObjectSnapshot,
    ObjectDelete,
    Relation
};

struct SRecord
{
    const CTraceEventFormat::SRegularFields& rf;
    const __itt_domain& domain;
    const __itt_id& taskid;
    const __itt_id& parentid;
    const __itt_string_handle *pName;
    double* pDelta;
    const char *pData;
    size_t length;
    void* function;
};
void WriteRecord(ERecordType type, const SRecord& record);

namespace sea {
    struct IHandler;
    void WriteThreadName(uint64_t tid, const char* name);
    void ReportString(__itt_string_handle* pStr);
    void ReportModule(void* fn);
}
sea::IHandler& GetSEARecorder();




