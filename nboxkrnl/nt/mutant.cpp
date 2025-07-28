/*
 * ergo720                Copyright (c) 2023
 */

#include "nt.hpp"
#include "ex.hpp"


EXPORTNUM(192) NTSTATUS XBOXAPI NtCreateMutant
(
	PHANDLE MutantHandle,
	POBJECT_ATTRIBUTES ObjectAttributes,
	BOOLEAN InitialOwner
)
{
	PVOID MutantObject;
	NTSTATUS Status = ObCreateObject(&ExMutantObjectType, ObjectAttributes, sizeof(KMUTANT), &MutantObject);

	if (NT_SUCCESS(Status)) {
		KeInitializeMutant((PKMUTANT)MutantObject, InitialOwner);
		Status = ObInsertObject(MutantObject, ObjectAttributes, 0, MutantHandle);
	}

	return Status;
}

/*
 * @implemented
 */
EXPORTNUM(213) NTSTATUS NTAPI NtQueryMutant
(
	IN HANDLE MutantHandle,
	OUT PMUTANT_BASIC_INFORMATION MutantInformation
)
{
	PKMUTANT Mutant;
	NTSTATUS Status;
	PAGED_CODE();

	if (!MutantInformation)
		return STATUS_INVALID_PARAMETER_2;

	/* Open the Object */
	Status = ObReferenceObjectByHandle(MutantHandle, &ExMutantObjectType, (PVOID*)&Mutant);
	/* Check for Status */
	if (NT_SUCCESS(Status))
	{
		/* Fill out the Basic Information Requested */
		DPRINT("Returning Mutant Information\n");
		MutantInformation->CurrentCount = KeReadStateMutant(Mutant);
		MutantInformation->OwnedByCaller = (Mutant->OwnerThread == KeGetCurrentThread());
		MutantInformation->AbandonedState = Mutant->Abandoned;

		/* Release the Object */
		ObDereferenceObject(Mutant);
	}

	/* Return Status */
	return Status;
}

EXPORTNUM(221) NTSTATUS XBOXAPI NtReleaseMutant
(
	HANDLE MutantHandle,
	PLONG PreviousCount
)
{
	PVOID MutantObject;
	NTSTATUS Status = ObReferenceObjectByHandle(MutantHandle, &ExMutantObjectType, &MutantObject);

	if (NT_SUCCESS(Status)) {
		// KeReleaseMutant will raise an exception if this thread doesn't own the mutex
		__try {
			LONG Count = KeReleaseMutant((PKMUTANT)MutantObject, PRIORITY_BOOST_MUTANT, FALSE, FALSE);
			if (PreviousCount) {
				*PreviousCount = Count;
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			Status = GetExceptionCode();
		}

		ObfDereferenceObject(MutantObject);
	}

	return Status;
}
