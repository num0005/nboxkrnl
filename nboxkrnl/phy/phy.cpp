// 2024 num0005

// ******************************************************************
// *
// *  This file is part of the Cxbx project.
// *
// *  Cxbx and Cxbe are free software; you can redistribute them
// *  and/or modify them under the terms of the GNU General Public
// *  License as published by the Free Software Foundation; either
// *  version 2 of the license, or (at your option) any later version.
// *
// *  This program is distributed in the hope that it will be useful,
// *  but WITHOUT ANY WARRANTY; without even the implied warranty of
// *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// *  GNU General Public License for more details.
// *
// *  You should have recieved a copy of the GNU General Public License
// *  along with this program; see the file COPYING.
// *  If not, write to the Free Software Foundation, Inc.,
// *  59 Temple Place - Suite 330, Bostom, MA 02111-1307, USA.
// *
// *  (c) 2018 Luke Usher
// *
// *  All rights reserved
// *
// ******************************************************************


#include "phy.h"
#include "ex.hpp"
#include "ke.hpp"
#include "ob.hpp"
#include "rtl.hpp"
#include <string.h>
#include <hal.hpp>
#include <nt.hpp>
#include <dbg.hpp>

DWORD PhyInitFlag = 0;
DWORD PhyLinkState = 0;
LONG PhyLockFlag;

#define NVNET_BUS 0
#define NVNET_SLOT 4

#define NVNET_BASE                              0xFEF00000
#define NVNET_SIZE                              0x400

template<typename T, ULONG Register>
inline static T NvNetPCIRead()
{
	static_assert(Register < NVNET_SIZE);

	T Result;
	volatile T* MMIO_Address = reinterpret_cast<T*>(NVNET_BASE + Register);
	Result = *MMIO_Address;
	return Result;
}

template<typename T, ULONG Register>
inline static void NvNetPCIWrite(T Value)
{
	static_assert(Register < NVNET_SIZE);

	volatile T* MMIO_Address = reinterpret_cast<T*>(NVNET_BASE + Register);
	*MMIO_Address = Value;
}

void PhyClearMDIOLOCK()
{
	NVNET_BASE;
	NvNetPCIWrite<DWORD, NvRegMIIControl>(MDIOADR_LOCK);
	int timeout = PHYRW_TIMEOUT;

	do
	{
		KeStallExecutionProcessor(50);
		timeout -= 50;
	} while (timeout > 0 && (NvNetPCIRead<DWORD, NvRegMIIControl>() & MDIOADR_LOCK));
}

// NON-Exported Phy Kernel functions
BOOL PhyReadReg(DWORD phyreg, DWORD* val)
{
	DWORD mdioadr;
	INT timeout;

	if (NvNetPCIRead<DWORD, NvRegMIIControl>() & MDIOADR_LOCK)
	{
		PhyClearMDIOLOCK();
	}

	mdioadr = (PHY_ADDR << MDIOADR_PHYSHIFT) | (phyreg << MDIOADR_REGSHIFT);
	NvNetPCIWrite<DWORD, NvRegMIIControl>(mdioadr);
	mdioadr |= MDIOADR_LOCK;

	for (timeout = PHYRW_TIMEOUT; timeout > 0 && (mdioadr & MDIOADR_LOCK); timeout -= 50)
	{
		KeStallExecutionProcessor(50);
		mdioadr = NvNetPCIRead<DWORD, NvRegMIIControl>();
	}

	*val = NvNetPCIRead<DWORD, NvRegMIIData>();

	if (mdioadr & MDIOADR_LOCK)
	{
		DbgPrint("PHY read failed: reg %d.\n", phyreg);
		return FALSE;
	}

	return TRUE;
}

BOOL PhyWriteReg(DWORD phyreg, DWORD val)
{

	DWORD mdioadr;
	INT timeout;

	if (NvNetPCIRead<DWORD, NvRegMIIControl>() & MDIOADR_LOCK)
	{
		PhyClearMDIOLOCK();
	}

	NvNetPCIWrite<DWORD, NvRegMIIData>(val);

	mdioadr = (PHY_ADDR << MDIOADR_PHYSHIFT) | (phyreg << MDIOADR_REGSHIFT) | MDIOADR_WRITE;
	NvNetPCIWrite<DWORD, NvRegMIIControl>(mdioadr);
	mdioadr |= MDIOADR_LOCK;

	for (timeout = PHYRW_TIMEOUT; timeout > 0 && (mdioadr & MDIOADR_LOCK); timeout -= 50)
	{
		KeStallExecutionProcessor(50);
		mdioadr = NvNetPCIRead<DWORD, NvRegMIIControl>();
	}

	if (mdioadr & MDIOADR_LOCK)
	{
		DbgPrint("PHY write failed: reg %d.\n", phyreg);
		return FALSE;
	}

	return TRUE;
}

DWORD PhyWaitForLinkUp()
{
	DWORD miiStatus = 0;
	INT timeout = 1000;
	while (timeout-- && !(miiStatus & MIISTATUS_LINK_IS_UP))
	{
		KeStallExecutionProcessor(500);
		if (!PhyReadReg(MIIREG_STATUS, &miiStatus))
		{
			break;
		}
	}

	return miiStatus;
}

BOOL PhyUpdateLinkState()
{
	DWORD anar, lpanar, miiStatus, state = 0;

	if (!PhyReadReg(MIIREG_ANAR, &anar) || !PhyReadReg(MIIREG_LPANAR, &lpanar) || !PhyReadReg(MIIREG_STATUS, &miiStatus))
	{
		return FALSE;
	}

	anar &= lpanar;
	if (anar & (MII4_100BASE_T_FULL_DUPLEX | MII4_100BASE_T_HALF_DUPLEX))
	{
		state |= XNET_ETHERNET_LINK_100MBPS;
	}
	else if (anar & (MII4_10BASE_T_FULL_DUPLEX | MII4_10BASE_T_HALF_DUPLEX))
	{
		state |= XNET_ETHERNET_LINK_10MBPS;
	}

	if (anar & (MII4_10BASE_T_FULL_DUPLEX | MII4_100BASE_T_FULL_DUPLEX))
	{
		state |= XNET_ETHERNET_LINK_FULL_DUPLEX;
	}
	else if (anar & (MII4_10BASE_T_HALF_DUPLEX | MII4_100BASE_T_HALF_DUPLEX))
	{
		state |= XNET_ETHERNET_LINK_HALF_DUPLEX;
	}

	if (miiStatus & MIISTATUS_LINK_IS_UP)
	{
		state |= XNET_ETHERNET_LINK_ACTIVE;
	}

	PhyLinkState = state;

	return TRUE;
}

// ******************************************************************
// * 0x00FC - PhyGetLinkState()
// ******************************************************************
EXPORTNUM(252) DWORD NTAPI PhyGetLinkState
(
	IN ULONG Mode
)
{
	if ((!PhyLinkState || Mode) && PhyLock() == 0)
	{
		PhyUpdateLinkState();
		PhyUnlock();
	}

	return PhyLinkState;
}

// ******************************************************************
// * 0x00FD - PhyInitialize()
// ******************************************************************
EXPORTNUM(253) NTSTATUS NTAPI PhyInitialize
(
	IN ULONG	forceReset,
	IN PVOID	Parameter2
)
{
	DWORD miiControl, miiStatus;
	INT timeout;
	NTSTATUS status = NETERR_HARDWARE;

	if (PhyLock() != 0)
	{
		return NETERR_BUSY;
	}

	if (forceReset)
	{
		PhyInitFlag = 0;
		PhyLinkState = 0;

		miiControl = MIICONTROL_RESET;
		if (!PhyWriteReg(MIIREG_CONTROL, miiControl))
		{
			goto err;
		}

		timeout = 1000;
		while (timeout-- && (miiControl & MIICONTROL_RESET))
		{
			KeStallExecutionProcessor(500);
			if (!PhyReadReg(MIIREG_CONTROL, &miiControl))
			{
				goto err;
			}
		}

		if (miiControl & MIICONTROL_RESET)
		{
			goto err;
		}
	}
	else if (PhyInitFlag)
	{
		PhyUpdateLinkState();
		status = NETERR_OK;
		goto exit;
	}

	timeout = 6000;
	miiStatus = 0;
	while (timeout-- && !(miiStatus & MIISTATUS_AUTO_NEGOTIATION_COMPLETE))
	{
		KeStallExecutionProcessor(500);
		if (!PhyReadReg(MIIREG_STATUS, &miiStatus)) goto err;
	}

	if (!PhyReadReg(MIIREG_CONTROL, &miiControl))
	{
		goto err;
	}

	if (miiControl & MIICONTROL_RESTART_AUTO_NEGOTIATION)
	{
		if (miiStatus & (MIISTATUS_100MBS_T4_CAPABLE | MIISTATUS_100MBS_X_HALF_DUPLEX_CAPABLE | MIISTATUS_100MBS_T2_HALF_DUPLEX_CAPABLE))
		{
			miiControl |= MIICONTROL_SPEED_SELECTION_BIT1;
			miiControl &= ~MIICONTROL_SPEED_SELECTION_BIT0;
			PhyLinkState |= XNET_ETHERNET_LINK_100MBPS;
		}
		else if (miiStatus & MIISTATUS_10MBS_HALF_DUPLEX_CAPABLE)
		{
			miiControl &= ~MIICONTROL_SPEED_SELECTION_BIT1;
			miiControl |= MIICONTROL_SPEED_SELECTION_BIT0;
			PhyLinkState |= XNET_ETHERNET_LINK_10MBPS;
		}
		else
		{
			goto err;
		}

		PhyLinkState |= XNET_ETHERNET_LINK_HALF_DUPLEX;
		PhyWriteReg(MIIREG_CONTROL, miiControl);
		miiStatus = PhyWaitForLinkUp();

		if (miiStatus & MIISTATUS_LINK_IS_UP)
		{
			PhyLinkState |= XNET_ETHERNET_LINK_ACTIVE;
		}
	}
	else
	{
		PhyWaitForLinkUp();
		if (!PhyUpdateLinkState())
		{
			goto err;
		}
	}

	PhyInitFlag = 1;
	status = NETERR_OK;

exit:
	PhyUnlock();
	return status;
err:
	DbgPrint("Ethernet PHY initialization failed.\n");
	goto exit;
}