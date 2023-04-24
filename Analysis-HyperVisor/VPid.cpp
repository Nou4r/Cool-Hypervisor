#include "includes.h"
#include "Common.hpp"
#include "ia32.hpp"
#include "Func_defs.hpp"
#include "Undocumented.hpp"

namespace Vpid {

	void Invvpid(invvpid_type Type, invvpid_descriptor* Descriptor)
	{
		if (!Descriptor)
		{
			static invvpid_descriptor ZeroDescriptor = { 0 };
			Descriptor = &ZeroDescriptor;
		}

		__invvpid(Type, Descriptor);
	}

	void InvvpidIndividualAddress(uint16_t Vpid, uint64_t LinearAddress)
	{
		invvpid_descriptor Descriptor = { 0 };
		Descriptor.vpid = Vpid;
		Descriptor.linear_address = LinearAddress;
		return Invvpid(invvpid_individual_address, &Descriptor);
	}

	void InvvpidSingleContext(uint16_t Vpid)
	{
		invvpid_descriptor Descriptor = { 0 };
		Descriptor.vpid = Vpid;
		return Invvpid(invvpid_single_context, &Descriptor);
	}

	void InvvpidAllContexts()
	{
		return Invvpid(invvpid_all_context, NULL);
	}

	void InvvpidSingleContextRetainingGlobals(uint16_t Vpid)
	{
		invvpid_descriptor Descriptor = { Vpid, 0, 0 };
		return Invvpid(invvpid_single_context_retaining_globals, &Descriptor);
	}
}