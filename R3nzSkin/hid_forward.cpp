#include <Windows.h>
#include "hid_forward.h"

// Defined in hid_thunks.asm (native COFF); declared here so the LTCG object
// references the asm-defined table instead of defining its own.
extern "C" { extern void* hid_ptrs[44]; }

static const char* const hid_names[44] = {
    "HidD_FlushQueue",
    "HidD_FreePreparsedData",
    "HidD_GetAttributes",
    "HidD_GetConfiguration",
    "HidD_GetFeature",
    "HidD_GetHidGuid",
    "HidD_GetIndexedString",
    "HidD_GetInputReport",
    "HidD_GetManufacturerString",
    "HidD_GetMsGenreDescriptor",
    "HidD_GetNumInputBuffers",
    "HidD_GetPhysicalDescriptor",
    "HidD_GetPreparsedData",
    "HidD_GetProductString",
    "HidD_GetSerialNumberString",
    "HidD_Hello",
    "HidD_SetConfiguration",
    "HidD_SetFeature",
    "HidD_SetNumInputBuffers",
    "HidD_SetOutputReport",
    "HidP_GetButtonCaps",
    "HidP_GetCaps",
    "HidP_GetData",
    "HidP_GetExtendedAttributes",
    "HidP_GetLinkCollectionNodes",
    "HidP_GetScaledUsageValue",
    "HidP_GetSpecificButtonCaps",
    "HidP_GetSpecificValueCaps",
    "HidP_GetUsageValue",
    "HidP_GetUsageValueArray",
    "HidP_GetUsages",
    "HidP_GetUsagesEx",
    "HidP_GetValueCaps",
    "HidP_InitializeReportForID",
    "HidP_MaxDataListLength",
    "HidP_MaxUsageListLength",
    "HidP_SetData",
    "HidP_SetScaledUsageValue",
    "HidP_SetUsageValue",
    "HidP_SetUsageValueArray",
    "HidP_SetUsages",
    "HidP_TranslateUsagesToI8042ScanCodes",
    "HidP_UnsetUsages",
    "HidP_UsageListDifference",
};

void hid_forward_init() noexcept
{
    const HMODULE real{ ::LoadLibraryW(L"C:\\Windows\\System32\\hid.dll") };
    if (!real)
        return;

    for (int i{ 0 }; i < 44; ++i)
        hid_ptrs[i] = reinterpret_cast<void*>(::GetProcAddress(real, hid_names[i]));
}

#pragma comment(linker, "/export:HidD_FlushQueue=hid_thunk_0")
#pragma comment(linker, "/export:HidD_FreePreparsedData=hid_thunk_1")
#pragma comment(linker, "/export:HidD_GetAttributes=hid_thunk_2")
#pragma comment(linker, "/export:HidD_GetConfiguration=hid_thunk_3")
#pragma comment(linker, "/export:HidD_GetFeature=hid_thunk_4")
#pragma comment(linker, "/export:HidD_GetHidGuid=hid_thunk_5")
#pragma comment(linker, "/export:HidD_GetIndexedString=hid_thunk_6")
#pragma comment(linker, "/export:HidD_GetInputReport=hid_thunk_7")
#pragma comment(linker, "/export:HidD_GetManufacturerString=hid_thunk_8")
#pragma comment(linker, "/export:HidD_GetMsGenreDescriptor=hid_thunk_9")
#pragma comment(linker, "/export:HidD_GetNumInputBuffers=hid_thunk_10")
#pragma comment(linker, "/export:HidD_GetPhysicalDescriptor=hid_thunk_11")
#pragma comment(linker, "/export:HidD_GetPreparsedData=hid_thunk_12")
#pragma comment(linker, "/export:HidD_GetProductString=hid_thunk_13")
#pragma comment(linker, "/export:HidD_GetSerialNumberString=hid_thunk_14")
#pragma comment(linker, "/export:HidD_Hello=hid_thunk_15")
#pragma comment(linker, "/export:HidD_SetConfiguration=hid_thunk_16")
#pragma comment(linker, "/export:HidD_SetFeature=hid_thunk_17")
#pragma comment(linker, "/export:HidD_SetNumInputBuffers=hid_thunk_18")
#pragma comment(linker, "/export:HidD_SetOutputReport=hid_thunk_19")
#pragma comment(linker, "/export:HidP_GetButtonCaps=hid_thunk_20")
#pragma comment(linker, "/export:HidP_GetCaps=hid_thunk_21")
#pragma comment(linker, "/export:HidP_GetData=hid_thunk_22")
#pragma comment(linker, "/export:HidP_GetExtendedAttributes=hid_thunk_23")
#pragma comment(linker, "/export:HidP_GetLinkCollectionNodes=hid_thunk_24")
#pragma comment(linker, "/export:HidP_GetScaledUsageValue=hid_thunk_25")
#pragma comment(linker, "/export:HidP_GetSpecificButtonCaps=hid_thunk_26")
#pragma comment(linker, "/export:HidP_GetSpecificValueCaps=hid_thunk_27")
#pragma comment(linker, "/export:HidP_GetUsageValue=hid_thunk_28")
#pragma comment(linker, "/export:HidP_GetUsageValueArray=hid_thunk_29")
#pragma comment(linker, "/export:HidP_GetUsages=hid_thunk_30")
#pragma comment(linker, "/export:HidP_GetUsagesEx=hid_thunk_31")
#pragma comment(linker, "/export:HidP_GetValueCaps=hid_thunk_32")
#pragma comment(linker, "/export:HidP_InitializeReportForID=hid_thunk_33")
#pragma comment(linker, "/export:HidP_MaxDataListLength=hid_thunk_34")
#pragma comment(linker, "/export:HidP_MaxUsageListLength=hid_thunk_35")
#pragma comment(linker, "/export:HidP_SetData=hid_thunk_36")
#pragma comment(linker, "/export:HidP_SetScaledUsageValue=hid_thunk_37")
#pragma comment(linker, "/export:HidP_SetUsageValue=hid_thunk_38")
#pragma comment(linker, "/export:HidP_SetUsageValueArray=hid_thunk_39")
#pragma comment(linker, "/export:HidP_SetUsages=hid_thunk_40")
#pragma comment(linker, "/export:HidP_TranslateUsagesToI8042ScanCodes=hid_thunk_41")
#pragma comment(linker, "/export:HidP_UnsetUsages=hid_thunk_42")
#pragma comment(linker, "/export:HidP_UsageListDifference=hid_thunk_43")
