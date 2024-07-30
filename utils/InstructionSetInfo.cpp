// InstructionSetInfo.cpp
// Compile by using: cl /EHsc /W4 InstructionSetInfo.cpp
// processor: x86, x64
// Uses the __cpuid intrinsic to get information about
// CPU extended instruction set support.

#include "stdafx.h"

#include <sstream>
#include <vector>
#include <bitset>
#include <array>

#ifdef WIN32
#include <intrin.h>
#endif

#include "InstructionSetInfo.h"

namespace Kernel
{
#ifdef WIN32

    // The code to get the processor support information came from the following webpage:
    // https://learn.microsoft.com/en-us/cpp/intrinsics/cpuid-cpuidex?redirectedfrom=MSDN&view=msvc-170
    //
    class InstructionSetInfo_Internal
    {
    public:
        InstructionSetInfo_Internal()
            : nIds_{ 0 }
            , nExIds_{ 0 }
            , isIntel_{ false }
            , isAMD_{ false }
            , f_1_ECX_{ 0 }
            , f_1_EDX_{ 0 }
            , f_7_EBX_{ 0 }
            , f_7_ECX_{ 0 }
            , f_81_ECX_{ 0 }
            , f_81_EDX_{ 0 }
            , data_{}
            , extdata_{}
        {
            //int cpuInfo[4] = {-1};
            std::array<int, 4> cpui;

            // Calling __cpuid with 0x0 as the function_id argument
            // gets the number of the highest valid function ID.
            __cpuid(cpui.data(), 0);
            nIds_ = cpui[0];

            for (int i = 0; i <= nIds_; ++i)
            {
                __cpuidex(cpui.data(), i, 0);
                data_.push_back(cpui);
            }

            // Capture vendor string
            char vendor[0x20];
            memset(vendor, 0, sizeof(vendor));
            *reinterpret_cast<int*>(vendor) = data_[0][1];
            *reinterpret_cast<int*>(vendor + 4) = data_[0][3];
            *reinterpret_cast<int*>(vendor + 8) = data_[0][2];
            vendor_ = vendor;
            if (vendor_ == "GenuineIntel")
            {
                isIntel_ = true;
            }
            else if (vendor_ == "AuthenticAMD")
            {
                isAMD_ = true;
            }

            // load bitset with flags for function 0x00000001
            if (nIds_ >= 1)
            {
                f_1_ECX_ = data_[1][2];
                f_1_EDX_ = data_[1][3];
            }

            // load bitset with flags for function 0x00000007
            if (nIds_ >= 7)
            {
                f_7_EBX_ = data_[7][1];
                f_7_ECX_ = data_[7][2];
            }

            // Calling __cpuid with 0x80000000 as the function_id argument
            // gets the number of the highest valid extended ID.
            __cpuid(cpui.data(), 0x80000000);
            nExIds_ = cpui[0];

            char brand[0x40];
            memset(brand, 0, sizeof(brand));

            for (int i = 0x80000000; i <= nExIds_; ++i)
            {
                __cpuidex(cpui.data(), i, 0);
                extdata_.push_back(cpui);
            }

            // load bitset with flags for function 0x80000001
            if (nExIds_ >= 0x80000001)
            {
                f_81_ECX_ = extdata_[1][2];
                f_81_EDX_ = extdata_[1][3];
            }

            // Interpret CPU brand string if reported
            if (nExIds_ >= 0x80000004)
            {
                memcpy(brand, extdata_[2].data(), sizeof(cpui));
                memcpy(brand + 16, extdata_[3].data(), sizeof(cpui));
                memcpy(brand + 32, extdata_[4].data(), sizeof(cpui));
                brand_ = brand;
            }
        }


        std::string Vendor(void) { return vendor_; }
        std::string Brand(void)  { return brand_; }

        bool SSE3(void)       { return f_1_ECX_[0]; }
        bool PCLMULQDQ(void)  { return f_1_ECX_[1]; }
        bool MONITOR(void)    { return f_1_ECX_[3]; }
        bool SSSE3(void)      { return f_1_ECX_[9]; }
        bool FMA(void)        { return f_1_ECX_[12]; }
        bool CMPXCHG16B(void) { return f_1_ECX_[13]; }
        bool SSE41(void)      { return f_1_ECX_[19]; }
        bool SSE42(void)      { return f_1_ECX_[20]; }
        bool MOVBE(void)      { return f_1_ECX_[22]; }
        bool POPCNT(void)     { return f_1_ECX_[23]; }
        bool AES(void)        { return f_1_ECX_[25]; }
        bool XSAVE(void)      { return f_1_ECX_[26]; }
        bool OSXSAVE(void)    { return f_1_ECX_[27]; }
        bool AVX(void)        { return f_1_ECX_[28]; }
        bool F16C(void)       { return f_1_ECX_[29]; }
        bool RDRAND(void)     { return f_1_ECX_[30]; }

        bool MSR(void)   { return f_1_EDX_[5]; }
        bool CX8(void)   { return f_1_EDX_[8]; }
        bool SEP(void)   { return f_1_EDX_[11]; }
        bool CMOV(void)  { return f_1_EDX_[15]; }
        bool CLFSH(void) { return f_1_EDX_[19]; }
        bool MMX(void)   { return f_1_EDX_[23]; }
        bool FXSR(void)  { return f_1_EDX_[24]; }
        bool SSE(void)   { return f_1_EDX_[25]; }
        bool SSE2(void)  { return f_1_EDX_[26]; }

        bool FSGSBASE(void) { return f_7_EBX_[0]; }
        bool BMI1(void)     { return f_7_EBX_[3]; }
        bool HLE(void)      { return isIntel_ && f_7_EBX_[4]; }
        bool AVX2(void)     { return f_7_EBX_[5]; }
        bool BMI2(void)     { return f_7_EBX_[8]; }
        bool ERMS(void)     { return f_7_EBX_[9]; }
        bool INVPCID(void)  { return f_7_EBX_[10]; }
        bool RTM(void)      { return isIntel_ && f_7_EBX_[11]; }
        bool AVX512F(void)  { return f_7_EBX_[16]; }
        bool RDSEED(void)   { return f_7_EBX_[18]; }
        bool ADX(void)      { return f_7_EBX_[19]; }
        bool AVX512PF(void) { return f_7_EBX_[26]; }
        bool AVX512ER(void) { return f_7_EBX_[27]; }
        bool AVX512CD(void) { return f_7_EBX_[28]; }
        bool SHA(void)      { return f_7_EBX_[29]; }

        bool PREFETCHWT1(void) { return f_7_ECX_[0]; }

        bool LAHF(void)  { return f_81_ECX_[0]; }
        bool LZCNT(void) { return isIntel_ && f_81_ECX_[5]; }
        bool ABM(void)   { return isAMD_   && f_81_ECX_[5]; }
        bool SSE4a(void) { return isAMD_   && f_81_ECX_[6]; }
        bool XOP(void)   { return isAMD_   && f_81_ECX_[11]; }
        bool TBM(void)   { return isAMD_   && f_81_ECX_[21]; }

        bool SYSCALL(void)   { return isIntel_ && f_81_EDX_[11]; }
        bool MMXEXT(void)    { return isAMD_   && f_81_EDX_[22]; }
        bool RDTSCP(void)    { return isIntel_ && f_81_EDX_[27]; }
        bool _3DNOWEXT(void) { return isAMD_   && f_81_EDX_[30]; }
        bool _3DNOW(void)    { return isAMD_   && f_81_EDX_[31]; }

        int nIds_;
        int nExIds_;
        std::string vendor_;
        std::string brand_;
        bool isIntel_;
        bool isAMD_;
        std::bitset<32> f_1_ECX_;
        std::bitset<32> f_1_EDX_;
        std::bitset<32> f_7_EBX_;
        std::bitset<32> f_7_ECX_;
        std::bitset<32> f_81_ECX_;
        std::bitset<32> f_81_EDX_;
        std::vector<std::array<int, 4>> data_;
        std::vector<std::array<int, 4>> extdata_;
    };
#endif //WIN32



    InstructionSetInfo::InstructionSetInfo()
        : m_Supports_AVX( false )
        , m_Supports_AVX2( false )
    {
    #ifdef WIN32
        InstructionSetInfo_Internal windows_info;
        m_Supports_AVX  = windows_info.AVX();
        m_Supports_AVX2 = windows_info.AVX2();
    #else
        // https://gcc.gnu.org/onlinedocs/gcc/x86-Built-in-Functions.html
        m_Supports_AVX  =  __builtin_cpu_supports( "avx" );
        m_Supports_AVX2 =  __builtin_cpu_supports( "avx2" );
    #endif
    }

    bool InstructionSetInfo::SupportsAVX(void)  { return m_Supports_AVX;  }
    bool InstructionSetInfo::SupportsAVX2(void) { return m_Supports_AVX2; }
    
    // If checking for new processor features is added, then please update the following
    // message so that the intformation can be provided in the version information.
    std::string InstructionSetInfo::GetSupportString()
    {
        std::stringstream ss;
        ss << "For your processor: ";
        ss << "AVX "  << (m_Supports_AVX  ? "is supported" : "is NOT supported") << ", ";
        ss << "AVX2 " << (m_Supports_AVX2 ? "is supported" : "is NOT supported") << ", ";

        return ss.str();
    }

#if 0
    void InstructionSetInfo::PrintSupport()
    {
        auto& outstream = std::cout;

        auto support_message = [&outstream](std::string isa_feature, bool is_supported) {
            outstream << isa_feature << (is_supported ? " supported" : " not supported") << std::endl;
        };

        std::cout << InstructionSetInfo::Vendor() << std::endl;
        std::cout << InstructionSetInfo::Brand() << std::endl;

        support_message("3DNOW",       InstructionSetInfo::_3DNOW());
        support_message("3DNOWEXT",    InstructionSetInfo::_3DNOWEXT());
        support_message("ABM",         InstructionSetInfo::ABM());
        support_message("ADX",         InstructionSetInfo::ADX());
        support_message("AES",         InstructionSetInfo::AES());
        support_message("AVX",         InstructionSetInfo::AVX() );
        support_message("AVX2",        InstructionSetInfo::AVX2()) ;
        support_message("AVX512CD",    InstructionSetInfo::AVX512CD());
        support_message("AVX512ER",    InstructionSetInfo::AVX512ER());
        support_message("AVX512F",     InstructionSetInfo::AVX512F());
        support_message("AVX512PF",    InstructionSetInfo::AVX512PF());
        support_message("BMI1",        InstructionSetInfo::BMI1());
        support_message("BMI2",        InstructionSetInfo::BMI2());
        support_message("CLFSH",       InstructionSetInfo::CLFSH());
        support_message("CMPXCHG16B",  InstructionSetInfo::CMPXCHG16B());
        support_message("CX8",         InstructionSetInfo::CX8());
        support_message("ERMS",        InstructionSetInfo::ERMS());
        support_message("F16C",        InstructionSetInfo::F16C());
        support_message("FMA",         InstructionSetInfo::FMA());
        support_message("FSGSBASE",    InstructionSetInfo::FSGSBASE());
        support_message("FXSR",        InstructionSetInfo::FXSR());
        support_message("HLE",         InstructionSetInfo::HLE());
        support_message("INVPCID",     InstructionSetInfo::INVPCID());
        support_message("LAHF",        InstructionSetInfo::LAHF());
        support_message("LZCNT",       InstructionSetInfo::LZCNT());
        support_message("MMX",         InstructionSetInfo::MMX());
        support_message("MMXEXT",      InstructionSetInfo::MMXEXT());
        support_message("MONITOR",     InstructionSetInfo::MONITOR());
        support_message("MOVBE",       InstructionSetInfo::MOVBE());
        support_message("MSR",         InstructionSetInfo::MSR());
        support_message("OSXSAVE",     InstructionSetInfo::OSXSAVE());
        support_message("PCLMULQDQ",   InstructionSetInfo::PCLMULQDQ());
        support_message("POPCNT",      InstructionSetInfo::POPCNT());
        support_message("PREFETCHWT1", InstructionSetInfo::PREFETCHWT1());
        support_message("RDRAND",      InstructionSetInfo::RDRAND());
        support_message("RDSEED",      InstructionSetInfo::RDSEED());
        support_message("RDTSCP",      InstructionSetInfo::RDTSCP());
        support_message("RTM",         InstructionSetInfo::RTM());
        support_message("SEP",         InstructionSetInfo::SEP());
        support_message("SHA",         InstructionSetInfo::SHA());
        support_message("SSE",         InstructionSetInfo::SSE());
        support_message("SSE2",        InstructionSetInfo::SSE2());
        support_message("SSE3",        InstructionSetInfo::SSE3());
        support_message("SSE4.1",      InstructionSetInfo::SSE41());
        support_message("SSE4.2",      InstructionSetInfo::SSE42());
        support_message("SSE4a",       InstructionSetInfo::SSE4a());
        support_message("SSSE3",       InstructionSetInfo::SSSE3());
        support_message("SYSCALL",     InstructionSetInfo::SYSCALL());
        support_message("TBM",         InstructionSetInfo::TBM());
        support_message("XOP",         InstructionSetInfo::XOP());
        support_message("XSAVE",       InstructionSetInfo::XSAVE());
    }
#endif
}
