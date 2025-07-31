#include "..\kernel.hpp"
#include "ke.hpp"
#include "hal.hpp"
#include "rtl.hpp"
#include "fsc.hpp"
#include "mm.hpp"
#include "mi.hpp"
#include "vad_tree.hpp"
#include <assert.h>

/*
 * @implemented
 */
EXPORTNUM(177) PVOID NTAPI MmMapIoSpace
(
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN SIZE_T NumberOfBytes,
    IN ULONG Protect
)
{

    PFN_NUMBER Pfn;
    PFN_COUNT PageCount;
    PMMPTE PointerPte;
    PVOID BaseAddress;
    MMPTE TempPte;
    #if 0
    PMMPFN Pfn1 = NULL;
    MI_PFN_CACHE_ATTRIBUTE CacheAttribute;
    BOOLEAN IsIoMapping;

    if (!NumberOfBytes || !MiConvertPageToSystemPtePermissions(Protect, &TempPte))
        return NULL;

    // Is it a physical address for hardware devices (flash, NV2A, etc) ?
    if (PhysicalAddress >= XBOX_WRITE_COMBINED_BASE &&
        PhysicalAddress + NumberOfBytes <= XBOX_UNCACHED_END)
    {
        // Return physical address as virtual (accesses will go through EmuException)

        RETURN(Paddr);
    }

    //
    // Must be called with a non-zero count
    //
    NT_ASSERT(NumberOfBytes != 0);

        //
        // Normalize and validate the caching attributes
        //
    CacheType &= 0xFF;
    if (CacheType >= MmMaximumCacheType) return NULL;

    //
    // Calculate page count
    //
    PageCount = ADDRESS_AND_SIZE_TO_SPAN_PAGES(PhysicalAddress.LowPart,
        NumberOfBytes);

//
// Compute the PFN and check if it's a known I/O mapping
// Also translate the cache attribute
//
    Pfn = (PFN_NUMBER)(PhysicalAddress.QuadPart >> PAGE_SHIFT);
    Pfn1 = MiGetPfnEntry(Pfn);
    IsIoMapping = (Pfn1 == NULL) ? TRUE : FALSE;
    CacheAttribute = MiPlatformCacheAttributes[IsIoMapping][CacheType];

    //
    // Now allocate system PTEs for the mapping, and get the VA
    //
    PointerPte = MiReserveSystemPtes(PageCount, SystemPteSpace);
    if (!PointerPte) return NULL;
    BaseAddress = MiPteToAddress(PointerPte);

    //
    // Check if this is uncached
    //
    if (CacheAttribute != MiCached)
    {
        //
        // Flush all caches
        //
        KeFlushEntireTb(TRUE, TRUE);
        KeInvalidateAllCaches();
    }

    //
    // Now compute the VA offset
    //
    BaseAddress = (PVOID)((ULONG_PTR)BaseAddress +
        BYTE_OFFSET(PhysicalAddress.LowPart));

//
// Get the template and configure caching
//
    TempPte = ValidKernelPte;
    switch (CacheAttribute)
    {
    case MiNonCached:

        //
        // Disable the cache
        //
        MI_PAGE_DISABLE_CACHE(&TempPte);
        MI_PAGE_WRITE_THROUGH(&TempPte);
        break;

    case MiCached:

        //
        // Leave defaults
        //
        break;

    case MiWriteCombined:

        //
        // Disable the cache and allow combined writing
        //
        MI_PAGE_DISABLE_CACHE(&TempPte);
        MI_PAGE_WRITE_COMBINED(&TempPte);
        break;

    default:

        //
        // Should never happen
        //
        ASSERT(FALSE);
        break;
    }

    //
    // Sanity check and re-flush
    //
    Pfn = (PFN_NUMBER)(PhysicalAddress.QuadPart >> PAGE_SHIFT);
    ASSERT((Pfn1 == MiGetPfnEntry(Pfn)) || (Pfn1 == NULL));
    KeFlushEntireTb(TRUE, TRUE);
    KeInvalidateAllCaches();

    //
    // Do the mapping
    //
    do
    {
        //
        // Write the PFN
        //
        TempPte.u.Hard.PageFrameNumber = Pfn++;
        MI_WRITE_VALID_PTE(PointerPte++, TempPte);
    } while (--PageCount);

    //
    // We're done!
    //
    #endif

    return BaseAddress;
}

/*
 * @implemented
 */
EXPORTNUM(183) VOID NTAPI MmUnmapIoSpace
(
    IN PVOID BaseAddress,
    IN SIZE_T NumberOfBytes
)
{
    PFN_NUMBER Pfn;
    PMMPTE PointerPte;

    //
    // Sanity check
    //
    NT_ASSERT(NumberOfBytes != 0);

    //
    // Get the page count
    //
    PFN_COUNT PageCount = ADDRESS_AND_SIZE_TO_SPAN_PAGES(BaseAddress, NumberOfBytes);

    //
    // Get the PTE and PFN
    //
    GetPteAddress(BaseAddress);
    PointerPte = MiAddressToPte(BaseAddress);
    
    Pfn = PFN_FROM_PTE(PointerPte);

    //
    // Is this an I/O mapping?
    //
    if (!MiGetPfnEntry(Pfn))
    {
        ///
        // Destroy the PTE
        //
        RtlZeroMemory(PointerPte, PageCount * sizeof(MMPTE));

        //
        // Blow the TLB
        //
        MiFlushEntireTlb();
    }

    //
    // Release the PTEs
    //
    //MiReleaseSystemPtes(PointerPte, PageCount, 0);
}
