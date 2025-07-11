/*
 * ergo720                Copyright (c) 2018
 */

#include "xc.hpp"
#include <rtl_assert.hpp>


 // The following are the default implementations of the crypto functions

VOID XBOXAPI JumpedSHAInit
(
	PUCHAR pbSHAContext
)
{
	// The sha1 context supplied to this function has an extra 24 bytes at the beginning which are unused by our implementation,
	// so we skip them. The same is true for XcSHAUpdate and XcSHAFinal

	A_SHAInit((SHA_CTX *)(pbSHAContext + 24));
}

VOID XBOXAPI JumpedSHAUpdate
(
	PUCHAR pbSHAContext,
	PUCHAR pbInput,
	ULONG dwInputLength
)
{
	A_SHAUpdate((SHA_CTX *)(pbSHAContext + 24), pbInput, dwInputLength);
}

VOID XBOXAPI JumpedSHAFinal
(
	PUCHAR pbSHAContext,
	PUCHAR pbDigest
)
{
	A_SHAFinal((SHA_CTX *)(pbSHAContext + 24), (PULONG)pbDigest);
}


 /* This struct contains the original crypto functions exposed by the kernel */
const CRYPTO_VECTOR DefaultCryptoStruct = {
	JumpedSHAInit,
	JumpedSHAUpdate,
	JumpedSHAFinal,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr
};

/* This struct contains the updated crypto functions which can be changed by the title with XcUpdateCrypto */
CRYPTO_VECTOR UpdatedCryptoStruct = DefaultCryptoStruct;

EXPORTNUM(335) VOID XBOXAPI XcSHAInit
(
	PUCHAR pbSHAContext
)
{
	NT_ASSERT(UpdatedCryptoStruct.pXcSHAInit);

	UpdatedCryptoStruct.pXcSHAInit(pbSHAContext);
}

EXPORTNUM(336) VOID XBOXAPI XcSHAUpdate
(
	PUCHAR pbSHAContext,
	PUCHAR pbInput,
	ULONG dwInputLength
)
{
	NT_ASSERT(UpdatedCryptoStruct.pXcSHAUpdate);

	UpdatedCryptoStruct.pXcSHAUpdate(pbSHAContext, pbInput, dwInputLength);
}

EXPORTNUM(337) VOID XBOXAPI XcSHAFinal
(
	PUCHAR pbSHAContext,
	PUCHAR pbDigest
)
{
	NT_ASSERT(UpdatedCryptoStruct.pXcSHAFinal);

	UpdatedCryptoStruct.pXcSHAFinal(pbSHAContext, pbDigest);
}

// ******************************************************************
// * 0x0152 - XcRC4Key()
// ******************************************************************
EXPORTNUM(338) void NTAPI XcRC4Key
(
	IN PUCHAR pbKeyStruct,
	IN ULONG  dwKeyLength,
	IN PUCHAR pbKey
)
{
	NT_ASSERT(UpdatedCryptoStruct.pXcRC4Key);

	UpdatedCryptoStruct.pXcRC4Key(pbKeyStruct, dwKeyLength, pbKey);
}

// ******************************************************************
// * 0x0153 - XcRC4Crypt
// ******************************************************************
EXPORTNUM(339) void NTAPI XcRC4Crypt
(
	IN PUCHAR pbKeyStruct,
	IN ULONG  dwInputLength,
	IN PUCHAR pbInput
)
{
	NT_ASSERT(UpdatedCryptoStruct.pXcRC4Crypt);

	UpdatedCryptoStruct.pXcRC4Crypt(pbKeyStruct, dwInputLength, pbInput);
}

// ******************************************************************
// * 0x0154 - XcHMAC()
// ******************************************************************
EXPORTNUM(340) void NTAPI XcHMAC
(
	IN PBYTE  pbKeyMaterial,
	IN ULONG  cbKeyMaterial,
	IN PBYTE  pbData,
	IN ULONG  cbData,
	IN PBYTE  pbData2,
	IN ULONG  cbData2,
	OUT PBYTE HmacData
)
{
	NT_ASSERT(UpdatedCryptoStruct.pXcHMAC);

	UpdatedCryptoStruct.pXcHMAC(pbKeyMaterial, cbKeyMaterial, pbData, cbData, pbData2, cbData2, HmacData);
}

// ******************************************************************
// * 0x0155 - XcPKEncPublic()
// ******************************************************************
EXPORTNUM(341) ULONG NTAPI XcPKEncPublic
(
	IN PUCHAR pbPubKey,
	IN PUCHAR pbInput,
	OUT PUCHAR pbOutput
)
{
	NT_ASSERT(UpdatedCryptoStruct.pXcPKEncPublic);

	ULONG Result = UpdatedCryptoStruct.pXcPKEncPublic(pbPubKey, pbInput, pbOutput);

	return Result;
}

// ******************************************************************
// * 0x0156 - XcPKDecPrivate()
// ******************************************************************
EXPORTNUM(342) ULONG NTAPI XcPKDecPrivate
(
	IN PUCHAR pbPrvKey,
	IN PUCHAR pbInput,
	OUT PUCHAR pbOutput
)
{
	NT_ASSERT(UpdatedCryptoStruct.pXcPKDecPrivate);

	ULONG Result = UpdatedCryptoStruct.pXcPKDecPrivate(pbPrvKey, pbInput, pbOutput);

	return Result;
}

// ******************************************************************
// * 0x0157 - XcPKGetKeyLen()
// ******************************************************************
EXPORTNUM(343) ULONG NTAPI XcPKGetKeyLen
(
	OUT PUCHAR pbPubKey
)
{
	NT_ASSERT(UpdatedCryptoStruct.pXcPKGetKeyLen);

	ULONG Result = UpdatedCryptoStruct.pXcPKGetKeyLen(pbPubKey);

	return Result;
}

// ******************************************************************
// * 0x0158 - XcVerifyPKCS1Signature()
// ******************************************************************
EXPORTNUM(344) BOOLEAN NTAPI XcVerifyPKCS1Signature
(
	IN PUCHAR pbSig,
	IN PUCHAR pbPubKey,
	IN PUCHAR pbDigest
)
{
	NT_ASSERT(UpdatedCryptoStruct.pXcVerifyPKCS1Signature);

	BOOLEAN Result = UpdatedCryptoStruct.pXcVerifyPKCS1Signature(pbSig, pbPubKey, pbDigest);

	return Result;
}

// ******************************************************************
// * 0x0159 - XcModExp()
// ******************************************************************
EXPORTNUM(345) ULONG NTAPI XcModExp
(
	IN LPDWORD pA,
	IN LPDWORD pB,
	IN LPDWORD pC,
	IN LPDWORD pD,
	IN ULONG   dwN
)
{
	NT_ASSERT(UpdatedCryptoStruct.pXcModExp);

	ULONG ret = UpdatedCryptoStruct.pXcModExp(pA, pB, pC, pD, dwN);

	return ret;
}

// ******************************************************************
// * 0x015A - XcDESKeyParity()
// ******************************************************************
EXPORTNUM(346) VOID NTAPI XcDESKeyParity
(
	IN PUCHAR pbKey,
	IN ULONG  dwKeyLength
)
{
	NT_ASSERT(UpdatedCryptoStruct.pXcDESKeyParity);

	UpdatedCryptoStruct.pXcDESKeyParity(pbKey, dwKeyLength);
}

// ******************************************************************
// * 0x015B - XcKeyTable()
// ******************************************************************
EXPORTNUM(347) void NTAPI XcKeyTable
(
	IN ULONG   dwCipher,
	OUT PUCHAR pbKeyTable,
	IN PUCHAR  pbKey
)
{
	NT_ASSERT(UpdatedCryptoStruct.pXcKeyTable);

	UpdatedCryptoStruct.pXcKeyTable(dwCipher, pbKeyTable, pbKey);
}

// ******************************************************************
// * 0x015C - XcBlockCrypt()
// ******************************************************************
EXPORTNUM(348) void NTAPI XcBlockCrypt
(
	IN ULONG   dwCipher,
	OUT PUCHAR pbOutput,
	IN PUCHAR  pbInput,
	IN PUCHAR  pbKeyTable,
	IN ULONG   dwOp
)
{
	NT_ASSERT(UpdatedCryptoStruct.pXcBlockCrypt);

	UpdatedCryptoStruct.pXcBlockCrypt(dwCipher, pbOutput, pbInput, pbKeyTable, dwOp);
}

// ******************************************************************
// * 0x015D - XcBlockCryptCBC()
// ******************************************************************
EXPORTNUM(349) void NTAPI XcBlockCryptCBC
(
	IN ULONG   dwCipher,
	IN ULONG   dwInputLength,
	OUT PUCHAR pbOutput,
	IN PUCHAR  pbInput,
	IN PUCHAR  pbKeyTable,
	IN ULONG   dwOp,
	IN PUCHAR  pbFeedback
)
{
	NT_ASSERT(UpdatedCryptoStruct.pXcBlockCryptCBC);

	UpdatedCryptoStruct.pXcBlockCryptCBC(dwCipher, dwInputLength, pbOutput, pbInput, pbKeyTable, dwOp, pbFeedback);
}

// ******************************************************************
// * 0x015E - XcCryptService()
// ******************************************************************
EXPORTNUM(350) ULONG NTAPI XcCryptService
(
	IN ULONG dwOp,
	IN PVOID pArgs
)
{
	NT_ASSERT(UpdatedCryptoStruct.pXcCryptService);

	ULONG Result = UpdatedCryptoStruct.pXcCryptService(dwOp, pArgs);

	return Result;
}

// Source: Cxbx-Reloaded
EXPORTNUM(351) VOID XBOXAPI XcUpdateCrypto
(
	PCRYPTO_VECTOR pNewVector,
	PCRYPTO_VECTOR pROMVector
)
{
	NT_ASSERT(pNewVector);
	// This function changes the default crypto function implementations with those supplied by the title (if not NULL)

	if (pNewVector->pXcSHAInit)
	{
		UpdatedCryptoStruct.pXcSHAInit = pNewVector->pXcSHAInit;
	}
	if (pNewVector->pXcSHAUpdate)
	{
		UpdatedCryptoStruct.pXcSHAUpdate = pNewVector->pXcSHAUpdate;
	}
	if (pNewVector->pXcSHAFinal)
	{
		UpdatedCryptoStruct.pXcSHAFinal = pNewVector->pXcSHAFinal;
	}
	if (pNewVector->pXcRC4Key)
	{
		UpdatedCryptoStruct.pXcRC4Key = pNewVector->pXcRC4Key;
	}
	if (pNewVector->pXcRC4Crypt)
	{
		UpdatedCryptoStruct.pXcRC4Crypt = pNewVector->pXcRC4Crypt;
	}
	if (pNewVector->pXcHMAC)
	{
		UpdatedCryptoStruct.pXcHMAC = pNewVector->pXcHMAC;
	}
	if (pNewVector->pXcPKEncPublic)
	{
		UpdatedCryptoStruct.pXcPKEncPublic = pNewVector->pXcPKEncPublic;
	}
	if (pNewVector->pXcPKDecPrivate)
	{
		UpdatedCryptoStruct.pXcPKDecPrivate = pNewVector->pXcPKDecPrivate;
	}
	if (pNewVector->pXcPKGetKeyLen)
	{
		UpdatedCryptoStruct.pXcPKGetKeyLen = pNewVector->pXcPKGetKeyLen;
	}
	if (pNewVector->pXcVerifyPKCS1Signature)
	{
		UpdatedCryptoStruct.pXcVerifyPKCS1Signature = pNewVector->pXcVerifyPKCS1Signature;
	}
	if (pNewVector->pXcModExp)
	{
		UpdatedCryptoStruct.pXcModExp = pNewVector->pXcModExp;
	}
	if (pNewVector->pXcDESKeyParity)
	{
		UpdatedCryptoStruct.pXcDESKeyParity = pNewVector->pXcDESKeyParity;
	}
	if (pNewVector->pXcKeyTable)
	{
		UpdatedCryptoStruct.pXcKeyTable = pNewVector->pXcKeyTable;
	}
	if (pNewVector->pXcBlockCrypt)
	{
		UpdatedCryptoStruct.pXcBlockCrypt = pNewVector->pXcBlockCrypt;
	}
	if (pNewVector->pXcBlockCryptCBC)
	{
		UpdatedCryptoStruct.pXcBlockCryptCBC = pNewVector->pXcBlockCryptCBC;
	}
	if (pNewVector->pXcCryptService)
	{
		UpdatedCryptoStruct.pXcCryptService = pNewVector->pXcCryptService;
	}

	// Return to the title the original implementations if it supplied an out buffer

	if (pROMVector)
	{
		*pROMVector = DefaultCryptoStruct;
	}
}
