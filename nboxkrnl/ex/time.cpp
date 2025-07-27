#include "ex.hpp"
#include "ke.hpp"
#include "nt.hpp"

/*
 * FUNCTION: Sets the system time.
 * PARAMETERS:
 *        NewTime - Points to a variable that specified the new time
 *        of day in the standard time format.
 *        OldTime - Optionally points to a variable that receives the
 *        old time of day in the standard time format.
 * RETURNS: Status
 */
EXPORTNUM(228) NTSTATUS NTAPI NtSetSystemTime
(
    IN PLARGE_INTEGER SystemTime,
    OUT PLARGE_INTEGER PreviousTime OPTIONAL
)
{
    LARGE_INTEGER OldSystemTime;
    LARGE_INTEGER NewSystemTime;
    LARGE_INTEGER LocalTime;
    TIME_FIELDS TimeFields;
    ULONG TimeZoneIdSave;

    PAGED_CODE();

    if (!SystemTime)
    {
        return STATUS_ACCESS_VIOLATION;
    }

    /* Reuse the pointer */
    NewSystemTime = *SystemTime;

    // TODO: set the RTC here

    /* Convert the time and set it in HAL */
   // ExSystemTimeToLocalTime(&NewSystemTime, &LocalTime);
    //RtlTimeToTimeFields(&LocalTime, &TimeFields);
    //HalSetRealTimeClock(&TimeFields);

    /* Now set the system time and notify the system */
    KeSetSystemTime(&NewSystemTime, &OldSystemTime);
    //PoNotifySystemTimeSet();

    /* Check if caller wanted previous time */
    if (PreviousTime)
    {
        *PreviousTime = OldSystemTime;
    }

    /* Return status */
    return STATUS_SUCCESS;
}