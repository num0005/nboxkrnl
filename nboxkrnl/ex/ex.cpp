/*
 * ergo720                Copyright (c) 2022
 * PatrickvL              Copyright (c) 2017
 */

#include "ex.hpp"
#include "ke.hpp"
#include "ob.hpp"
#include "rtl.hpp"
#include <string.h>
#include <hal.hpp>

#define XC_END_MARKER (XC_VALUE_INDEX)-1

#define EEPROM_INFO_ENTRY(XC, Member, REG_Type) \
	{ XC, offsetof(XBOX_EEPROM, Member), REG_Type, sizeof(((XBOX_EEPROM *)0)->Member) }


struct EepromInfo {
	XC_VALUE_INDEX Index;
	INT ValueOffset;
	INT ValueType;
	INT ValueLength;
};

// Source: Cxbx-Reloaded
static const EepromInfo EepromInfos[] = {
	EEPROM_INFO_ENTRY(XC_TIMEZONE_BIAS,         UserSettings.TimeZoneBias,                REG_DWORD),
	EEPROM_INFO_ENTRY(XC_TZ_STD_NAME,           UserSettings.TimeZoneStdName,             REG_BINARY),
	EEPROM_INFO_ENTRY(XC_TZ_STD_DATE,           UserSettings.TimeZoneStdDate,             REG_DWORD),
	EEPROM_INFO_ENTRY(XC_TZ_STD_BIAS,           UserSettings.TimeZoneStdBias,             REG_DWORD),
	EEPROM_INFO_ENTRY(XC_TZ_DLT_NAME,           UserSettings.TimeZoneDltName,             REG_BINARY),
	EEPROM_INFO_ENTRY(XC_TZ_DLT_DATE,           UserSettings.TimeZoneDltDate,             REG_DWORD),
	EEPROM_INFO_ENTRY(XC_TZ_DLT_BIAS,           UserSettings.TimeZoneDltBias,             REG_DWORD),
	EEPROM_INFO_ENTRY(XC_LANGUAGE,              UserSettings.Language,                    REG_DWORD),
	EEPROM_INFO_ENTRY(XC_VIDEO,                 UserSettings.VideoFlags,                  REG_DWORD),
	EEPROM_INFO_ENTRY(XC_AUDIO,                 UserSettings.AudioFlags,                  REG_DWORD),
	EEPROM_INFO_ENTRY(XC_P_CONTROL_GAMES,       UserSettings.ParentalControlGames,        REG_DWORD),
	EEPROM_INFO_ENTRY(XC_P_CONTROL_PASSWORD,    UserSettings.ParentalControlPassword,     REG_DWORD),
	EEPROM_INFO_ENTRY(XC_P_CONTROL_MOVIES,      UserSettings.ParentalControlMovies,       REG_DWORD),
	EEPROM_INFO_ENTRY(XC_ONLINE_IP_ADDRESS,     UserSettings.OnlineIpAddress,             REG_DWORD),
	EEPROM_INFO_ENTRY(XC_ONLINE_DNS_ADDRESS,    UserSettings.OnlineDnsAddress,            REG_DWORD),
	EEPROM_INFO_ENTRY(XC_ONLINE_DEFAULT_GATEWAY_ADDRESS, UserSettings.OnlineDefaultGatewayAddress, REG_DWORD),
	EEPROM_INFO_ENTRY(XC_ONLINE_SUBNET_ADDRESS, UserSettings.OnlineSubnetMask,            REG_DWORD),
	EEPROM_INFO_ENTRY(XC_MISC,                  UserSettings.MiscFlags,                   REG_DWORD),
	EEPROM_INFO_ENTRY(XC_DVD_REGION,            UserSettings.DvdRegion,                   REG_DWORD),
	// XC_MAX_OS is called to return a complete XBOX_USER_SETTINGS structure
	EEPROM_INFO_ENTRY(XC_MAX_OS,                UserSettings,                             REG_BINARY),
	EEPROM_INFO_ENTRY(XC_FACTORY_SERIAL_NUMBER, FactorySettings.SerialNumber,             REG_BINARY),
	EEPROM_INFO_ENTRY(XC_FACTORY_ETHERNET_ADDR, FactorySettings.EthernetAddr,             REG_BINARY),
	EEPROM_INFO_ENTRY(XC_FACTORY_ONLINE_KEY,    FactorySettings.OnlineKey,                REG_BINARY),
	EEPROM_INFO_ENTRY(XC_FACTORY_AV_REGION,     FactorySettings.AVRegion,                 REG_DWORD),
	// Note : XC_FACTORY_GAME_REGION is linked to a separate ULONG XboxFactoryGameRegion (of type REG_DWORD)
	EEPROM_INFO_ENTRY(XC_FACTORY_GAME_REGION,   EncryptedSettings.GameRegion,             REG_DWORD),
	EEPROM_INFO_ENTRY(XC_ENCRYPTED_SECTION,     EncryptedSettings,                        REG_BINARY),
	{ XC_MAX_ALL, 0, REG_BINARY, sizeof(XBOX_EEPROM) },
	{ XC_END_MARKER }
};

static INITIALIZE_GLOBAL_CRITICAL_SECTION(ExpEepromLock);

// Source: Cxbx-Reloaded
static const EepromInfo *ExpFindEepromInfo(XC_VALUE_INDEX Index)
{
	for (int i = 0; EepromInfos[i].Index != XC_END_MARKER; ++i) {
		if (EepromInfos[i].Index == Index) {
			return &EepromInfos[i];
		}
	}

	return nullptr;
}

EXPORTNUM(22) OBJECT_TYPE ExMutantObjectType = {
	ExAllocatePoolWithTag,
	ExFreePool,
	nullptr,
	ExpDeleteMutant,
	nullptr,
	(PVOID)offsetof(KMUTANT, Header),
	'atuM'
};

static void EepromCRC(ULONG* Checsum, long dataLen)
{
	// clear CRC
	RtlZeroMemory(Checsum, sizeof(*Checsum));

	unsigned char* crc =  reinterpret_cast<unsigned char*>(Checsum);
	unsigned char* data = &crc[sizeof(*Checsum)];

	for (int pos=0; pos < 4; ++pos)
	{
		unsigned short CRCPosVal = 0xFFFF;
		for (unsigned long l=pos; l < dataLen; l+=4)
		{
			// remap to source position
			unsigned long source_index;
			if (l == 0)
				source_index = dataLen - 1;
			else
				source_index = l + 1;

			// read low and high part of short (0 out of bounds)

			unsigned short LowValue = 0;
			if (source_index < dataLen)
				LowValue = data[source_index];

			source_index++;

			unsigned short HighValue = 0;
			if (source_index < dataLen)
				HighValue = data[source_index];

			unsigned short Value = (HighValue << 8) | LowValue;

			CRCPosVal -= Value;
		}
		CRCPosVal &= 0xFF00;
		crc[pos] = (unsigned char)(CRCPosVal >> 8);
	}
}

void ExpMakeEEPROMValid(XBOX_EEPROM &eeprom)
{
	CachedEeprom;
	constexpr long ChecksumLength = sizeof(ULONG);

	static_assert(sizeof(eeprom.FactorySettings.Checksum) == ChecksumLength);
	static_assert(offsetof(XBOX_FACTORY_SETTINGS, Checksum) == 0);
	static_assert(sizeof(eeprom.UserSettings.Checksum) == ChecksumLength);
	static_assert(offsetof(XBOX_USER_SETTINGS, Checksum) == 0);

	constexpr long FactoryDataSize = sizeof(eeprom.FactorySettings) - ChecksumLength;
	constexpr long UserDataSize = sizeof(eeprom.UserSettings) - ChecksumLength;

	EepromCRC(
		&eeprom.FactorySettings.Checksum,
		FactoryDataSize
	);
	EepromCRC(
		&eeprom.UserSettings.Checksum,
		UserDataSize
	);
}

// Source: Cxbx-Reloaded
EXPORTNUM(24) NTSTATUS XBOXAPI ExQueryNonVolatileSetting
(
	DWORD ValueIndex,
	DWORD *Type,
	PVOID Value,
	SIZE_T ValueLength,
	PSIZE_T ResultLength
)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PVOID ValueAddr = nullptr;
	INT ValueType;
	SIZE_T ActualLength;

	if (ValueIndex == XC_FACTORY_GAME_REGION) {
		ValueAddr = &XboxFactoryGameRegion;
		ValueType = REG_DWORD;
		ActualLength = sizeof(ULONG);
	}
	else {
		const EepromInfo *Info = ExpFindEepromInfo((XC_VALUE_INDEX)ValueIndex);
		if (Info) {
			ValueAddr = (PVOID)((PUCHAR)&CachedEeprom + Info->ValueOffset);
			ValueType = Info->ValueType;
			ActualLength = Info->ValueLength;
		}
	}

	if (ValueAddr) {
		if (ResultLength) {
			*ResultLength = ActualLength;
		}

		if (ValueLength >= ActualLength) {
			RtlEnterCriticalSectionAndRegion(&ExpEepromLock);

			*Type = ValueType;
			memset(Value, 0, ValueLength);
			memcpy(Value, ValueAddr, ActualLength);

			RtlLeaveCriticalSectionAndRegion(&ExpEepromLock);
		}
		else {
			Status = STATUS_BUFFER_TOO_SMALL;
		}
	}
	else {
		Status = STATUS_OBJECT_NAME_NOT_FOUND;
	}

	return Status;
}

// ******************************************************************
// * 0x001D - ExSaveNonVolatileSetting()
// ******************************************************************
EXPORTNUM(29) NTSTATUS NTAPI ExSaveNonVolatileSetting
(
	IN  DWORD			   ValueIndex,
	IN  DWORD			   Type,
	IN  PVOID			   Value,
	IN  SIZE_T			   ValueLength
)
{
	PVOID ValueAddr = nullptr;
	INT ValueType;
	SIZE_T ActualLength;

	// Don't allow writing to the eeprom encrypted area
	if (ValueIndex == XC_ENCRYPTED_SECTION)
		return STATUS_OBJECT_NAME_NOT_FOUND;

	// if it's not a DEVKIT we check if the value is within the permitted range
	if (HalQueryConsoleType() != ConsoleType::CONSOLE_DEVKIT &&
		!(ValueIndex <= XC_MAX_OS || ValueIndex > XC_MAX_FACTORY))
	{
		return STATUS_OBJECT_NAME_NOT_FOUND;
	}

	const EepromInfo* Info = ExpFindEepromInfo((XC_VALUE_INDEX)ValueIndex);
	if (Info)
	{
		ValueAddr = (PVOID)((PUCHAR)&CachedEeprom + Info->ValueOffset);
		ValueType = Info->ValueType;
		ActualLength = Info->ValueLength;
	}

	// look-up failed for whatever reason
	if (!ValueAddr)
		return STATUS_OBJECT_NAME_NOT_FOUND;

	// data is too long
	if (ValueLength > ActualLength)
		return STATUS_INVALID_PARAMETER;

	// not done by retail kernel
#if DBG
	if (ValueType != Type)
		return STATUS_INVALID_PARAMETER;
#endif

	RtlEnterCriticalSectionAndRegion(&ExpEepromLock);

	memset(ValueAddr, 0, ValueLength);
	memcpy(ValueAddr, Value, ValueLength);
	ExpMakeEEPROMValid(CachedEeprom);
	// TODO: write out eeprom

	RtlLeaveCriticalSectionAndRegion(&ExpEepromLock);

	return STATUS_SUCCESS;
}
