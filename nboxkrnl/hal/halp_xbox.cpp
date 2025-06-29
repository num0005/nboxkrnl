#include  "hal.hpp"

#if 0
void HalpWriteToDebugOutput(char* buffer, size_t max_length = 0)
{
	for (size_t i = 0; i < max_length; i++)
	{
		char charecter = buffer[i];
		if (!charecter)
			break;

	}
	outl(DBG_OUTPUT_STR_PORT, reinterpret_cast<ULONG>(buffer));
}
#endif
