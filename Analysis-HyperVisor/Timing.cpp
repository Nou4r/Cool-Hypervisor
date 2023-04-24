#include "includes.h"
#include "Undocumented.hpp"
#include "Func_defs.hpp"

namespace Timing {

	uint64_t CpuidTimeBeforeLaunch = 0;
	uint64_t CpuidTimeAfterLaunch = 0;
	uint64_t VmxInstructionBeforeLaunch = 0;
	uint64_t VmxInstructionTimeAfterLaunch = 0;

	uint64_t MeasureCpuidTscOffset() {

		_disable();

		uint64_t Start = 0;
		uint64_t End = 0;
		int Data[4] = { 0 };

		Start = __rdtsc();

		__cpuidex((int*)&Data, 0, 0);

		End = __rdtsc();

		_enable();

		uint64_t InstructionLen = End - Start;

		return InstructionLen;
	}

	uint64_t GetAverageCpuidTime() {
		uint64_t AverageTime = 0;

		for (uint64_t i = 0; i < 100; i++) {
			AverageTime += MeasureCpuidTscOffset();
			KeStallExecutionProcessor(10);
		}

		AverageTime = AverageTime / 100;

		return AverageTime;
	}

	uint64_t MeasureVmxInstructionTscOffset() {

		_disable();

		uint64_t Start = 0;
		uint64_t End = 0;

		Start = __rdtsc();

		__try 
		{
			_mm_lfence();
			__vmx_vmread(0, 0);
			_mm_lfence();

		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			_mm_lfence();
		}

		End = __rdtsc();

		_enable();

		uint64_t InstructionLen = End - Start;

		return InstructionLen;
	}

	uint64_t GetAverageVmxInstructionTime() {
		uint64_t AverageTime = 0;

		for (uint64_t i = 0; i < 100; i++) {
			AverageTime += MeasureVmxInstructionTscOffset();
			KeStallExecutionProcessor(10);
		}

		AverageTime = AverageTime / 100;

		return AverageTime;
	}

	void PopulateTscOffsetTableBeforeLaunch() {

		CpuidTimeBeforeLaunch = Timing::GetAverageCpuidTime();
		VmxInstructionBeforeLaunch = Timing::GetAverageVmxInstructionTime();
	}

	void PopulateTscOffsetTableAfterLaunch() {

		CpuidTimeAfterLaunch = Timing::GetAverageCpuidTime();
		VmxInstructionTimeAfterLaunch = Timing::GetAverageVmxInstructionTime();
	}

	void FinishTscTablePopulating() {
		//To do: Add support for ept violations etc.
		Hv.Timing.CpuidTimeDiff = CpuidTimeAfterLaunch - CpuidTimeBeforeLaunch + Hv.Timing.AverageEditTime;
		Hv.Timing.VmxInstructionTimeDiff = VmxInstructionTimeAfterLaunch - VmxInstructionBeforeLaunch + Hv.Timing.AverageEditTime;

		DBG_LOG("Cpuid timing diff: 0x%llx", Hv.Timing.CpuidTimeDiff);
		DBG_LOG("Vmx instruction timing diff: 0x%llx", Hv.Timing.VmxInstructionTimeDiff);

		Hv.Timing.UseTimeDiffs = true;
	}

}