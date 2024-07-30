#pragma once

#include <string>

    namespace Kernel
    {

    // InstructionSetInfo provides a simplified/consistent way to find out what instructions are provided
    // by the processor.  This allows one to have different code depending on the processor being used.
    class InstructionSetInfo
    {
    public:
        InstructionSetInfo();

        bool SupportsAVX(void);
        bool SupportsAVX2(void);

        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! If you add a method to check if the processor supports a particular feature,
        // !!! please add the support infomration to the GetSupportString() method.
        // !!! This will help in debugging different behaviors.
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 
        std::string GetSupportString();

    private:
        bool m_Supports_AVX;
        bool m_Supports_AVX2;

#if 0
        // getters
        std::string Vendor(void);
        std::string Brand(void);

        bool SSE3(void);
        bool PCLMULQDQ(void);
        bool MONITOR(void);
        bool SSSE3(void);
        bool FMA(void);
        bool CMPXCHG16B(void);
        bool SSE41(void);
        bool SSE42(void);
        bool MOVBE(void);
        bool POPCNT(void);
        bool AES(void);
        bool XSAVE(void);
        bool OSXSAVE(void);
        bool F16C(void);
        bool RDRAND(void);

        bool MSR(void);
        bool CX8(void);
        bool SEP(void);
        bool CMOV(void);
        bool CLFSH(void);
        bool MMX(void);
        bool FXSR(void);
        bool SSE(void);
        bool SSE2(void);

        bool FSGSBASE(void);
        bool BMI1(void);
        bool HLE(void);
        bool BMI2(void);
        bool ERMS(void);
        bool INVPCID(void);
        bool RTM(void);
        bool AVX512F(void);
        bool RDSEED(void);
        bool ADX(void);
        bool AVX512PF(void);
        bool AVX512ER(void);
        bool AVX512CD(void);
        bool SHA(void);

        bool PREFETCHWT1(void);

        bool LAHF(void);
        bool LZCNT(void);
        bool ABM(void);
        bool SSE4a(void);
        bool XOP(void);
        bool TBM(void);

        bool SYSCALL(void);
        bool MMXEXT(void);
        bool RDTSCP(void);
        bool _3DNOWEXT(void);
        bool _3DNOW(void);
#endif
    };
}