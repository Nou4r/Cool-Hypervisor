#include "includesUM.h"
#include "Calling_StructsUM.h"
#include "Func_defsUM.h"
#include "Driver.h"

LONG WINAPI UdExceptionVEH(struct _EXCEPTION_POINTERS* ExceptionInfo)
{
    if (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_ILLEGAL_INSTRUCTION)
    {
        Sleep(1000);

        auto rip = (uint8_t*)ExceptionInfo->ContextRecord->Rip;

        /*  force return from the patched messagebox */

        ExceptionInfo->ContextRecord->Rip = *(uintptr_t*)ExceptionInfo->ContextRecord->Rsp;

        ExceptionInfo->ContextRecord->Rsp += 8;

        return EXCEPTION_CONTINUE_EXECUTION;
    }

    // IT's not a break intruction. Continue searching for an exception handler.
    return EXCEPTION_CONTINUE_SEARCH;
}

uint64_t MeasureTscOffset() {

	uint64_t Start = 0;
	uint64_t End = 0;
	int Data[4] = { 0 };

	Start = __rdtsc();

	__cpuidex((int*)&Data, 0, 0);

	End = __rdtsc();

	uint64_t InstructionLen = End - Start;

	return InstructionLen;
}

bool GetAverageCpuidTime() {
	uint64_t AverageCpuidTime = 0;

	for (uint64_t i = 0; i < 10; i++) {
		AverageCpuidTime += MeasureTscOffset();
	}

	AverageCpuidTime = AverageCpuidTime / 10;

	printf("%d\n", AverageCpuidTime);

	return (AverageCpuidTime < 1000 && AverageCpuidTime > 0) ? FALSE : TRUE;
}


int main() {

	EptHookType Type = { 0 };
	Type.Execute = true;

	//Driver::HookUsermodePage(MessageBoxA, (uint8_t*)"\x0F\x0B", 2); was experimented with with success but I didn't bother to fully make it 

	if (GetAverageCpuidTime())
		printf("Traced");

	MessageBoxA(0, "If you press timing will be repeated", "Seriously", MB_OK);


	if (GetAverageCpuidTime())
		printf("Traced");

	uint64_t WriteTargetAddr = 0;
	uint64_t WriteBuffer = 0xdeadbeef;
	bool Result = false;

	Result = Driver::Write(&WriteBuffer, &WriteTargetAddr, sizeof(uint64_t));

	printf("Result: %d\n", Result);
	printf("Value in write buffer: 0x%llx\n", WriteBuffer); //This is written to targetaddr
	printf("Written Value: 0x%llx\n\n", WriteTargetAddr);

	uint64_t ReadTargetAddr = 0xdeadbeef;
	uint64_t ReadBuffer = 0;
	bool ReadResult = false;

	ReadResult = Driver::Read(&ReadBuffer, &ReadTargetAddr, sizeof(uint64_t));

	printf("Result: %d\n", ReadResult);
	printf("Value at targetaddr: 0x%llx\n", ReadTargetAddr);
	printf("Value in read buffer: 0x%llx\n\n", ReadBuffer); //This is getting read from targetaddr

	uint64_t ModBase = Driver::GetModuleBase(L"notepad.exe", L"notepad.exe");
	uint64_t Buff = 0;

	printf("Module base: 0x%llx\n", ModBase);

	ReadResult = Driver::Read(&Buff, (void*)&ModBase, sizeof(uint64_t));

	printf("Result: %d\n", ReadResult);
	printf("Value at Module base: 0x%llx\n", Buff);

	MessageBoxA(0, "If you press timing will be repeated", "Seriously", MB_OK);

	

	return 0;
}