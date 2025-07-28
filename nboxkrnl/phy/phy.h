#pragma once

#include "ke.hpp"

// from cxbx-reloaded  

// Flags returned by PhyGetLinkState
#define XNET_ETHERNET_LINK_ACTIVE           0x01
#define XNET_ETHERNET_LINK_100MBPS          0x02
#define XNET_ETHERNET_LINK_10MBPS           0x04
#define XNET_ETHERNET_LINK_FULL_DUPLEX      0x08
#define XNET_ETHERNET_LINK_HALF_DUPLEX      0x10

#define PhyLock() InterlockedCompareExchange(&PhyLockFlag, 1, 0)
#define PhyUnlock() (PhyLockFlag = 0)
#define NETERR_OK           STATUS_SUCCESS
#define NETERR_BUSY         STATUS_DEVICE_BUSY
#define NETERR_HARDWARE     0x801f0001  // hardware not responding

#define BIT(n)              (1u << (n))
#define MDIOADR_LOCK        BIT(15)
#define MDIOADR_WRITE       BIT(10)
#define PHY_ADDR			1
#define MDIOADR_PHYSHIFT    5
#define MDIOADR_REGSHIFT    0
#define MIITM_INTERVAL  5
#define PHYRW_TIMEOUT   ((64*2*2*400*MIITM_INTERVAL/1000)*16)
#define MIIREG_CONTROL 0
#define MIIREG_STATUS  1
#define MIIREG_ANAR    4
#define MIIREG_LPANAR  5
#define MIICONTROL_RESET                        BIT(15)
#define MIICONTROL_SPEED_SELECTION_BIT1         BIT(13)
#define MIICONTROL_RESTART_AUTO_NEGOTIATION     BIT(9)
#define MIICONTROL_SPEED_SELECTION_BIT0         BIT(6)
#define MIISTATUS_100MBS_T4_CAPABLE             BIT(15)
#define MIISTATUS_100MBS_X_HALF_DUPLEX_CAPABLE  BIT(13)
#define MIISTATUS_10MBS_HALF_DUPLEX_CAPABLE     BIT(11)
#define MIISTATUS_100MBS_T2_HALF_DUPLEX_CAPABLE BIT(9)
#define MIISTATUS_AUTO_NEGOTIATION_COMPLETE     BIT(5)
#define MIISTATUS_AUTO_NEGOTIATION_CAPABLE      BIT(3)
#define MII4_100BASE_T4                         BIT(9)
#define MII4_100BASE_T_FULL_DUPLEX              BIT(8)
#define MII4_100BASE_T_HALF_DUPLEX              BIT(7)
#define MII4_10BASE_T_FULL_DUPLEX               BIT(6)
#define MII4_10BASE_T_HALF_DUPLEX               BIT(5)
#define MIISTATUS_LINK_IS_UP                    BIT(2)

// NVNET Register Definitions
// Taken from XQEMU through cxbx-reloaded
enum
{
	NvRegIrqStatus = 0x000,
	#       define NVREG_IRQSTAT_BIT1     0x002
	#       define NVREG_IRQSTAT_BIT4     0x010
	#       define NVREG_IRQSTAT_MIIEVENT 0x040
	#       define NVREG_IRQSTAT_MASK     0x1ff
	NvRegIrqMask = 0x004,
	#       define NVREG_IRQ_RX           0x0002
	#       define NVREG_IRQ_RX_NOBUF     0x0004
	#       define NVREG_IRQ_TX_ERR       0x0008
	#       define NVREG_IRQ_TX2          0x0010
	#       define NVREG_IRQ_TIMER        0x0020
	#       define NVREG_IRQ_LINK         0x0040
	#       define NVREG_IRQ_TX1          0x0100
	#       define NVREG_IRQMASK_WANTED_1 0x005f
	#       define NVREG_IRQMASK_WANTED_2 0x0147
	#       define NVREG_IRQ_UNKNOWN      (~(NVREG_IRQ_RX|NVREG_IRQ_RX_NOBUF|\
    NVREG_IRQ_TX_ERR|NVREG_IRQ_TX2|NVREG_IRQ_TIMER|NVREG_IRQ_LINK|\
    NVREG_IRQ_TX1))
	NvRegUnknownSetupReg6 = 0x008,
	#       define NVREG_UNKSETUP6_VAL 3
		/*
		* NVREG_POLL_DEFAULT is the interval length of the timer source on the nic
		* NVREG_POLL_DEFAULT=97 would result in an interval length of 1 ms
		*/
		NvRegPollingInterval = 0x00c,
		#       define NVREG_POLL_DEFAULT 970
		NvRegMisc1 = 0x080,
		#       define NVREG_MISC1_HD    0x02
		#       define NVREG_MISC1_FORCE 0x3b0f3c
		NvRegTransmitterControl = 0x084,
		#       define NVREG_XMITCTL_START 0x01
		NvRegTransmitterStatus = 0x088,
		#       define NVREG_XMITSTAT_BUSY 0x01
		NvRegPacketFilterFlags = 0x8c,
		#       define NVREG_PFF_ALWAYS  0x7F0008
		#       define NVREG_PFF_PROMISC 0x80
		#       define NVREG_PFF_MYADDR  0x20
		NvRegOffloadConfig = 0x90,
		#       define NVREG_OFFLOAD_HOMEPHY 0x601
		#       define NVREG_OFFLOAD_NORMAL  0x5ee
		NvRegReceiverControl = 0x094,
		#       define NVREG_RCVCTL_START 0x01
		NvRegReceiverStatus = 0x98,
		#       define NVREG_RCVSTAT_BUSY  0x01
		NvRegRandomSeed = 0x9c,
		#       define NVREG_RNDSEED_MASK  0x00ff
		#       define NVREG_RNDSEED_FORCE 0x7f00
		NvRegUnknownSetupReg1 = 0xA0,
		#       define NVREG_UNKSETUP1_VAL 0x16070f
		NvRegUnknownSetupReg2 = 0xA4,
		#       define NVREG_UNKSETUP2_VAL 0x16
		NvRegMacAddrA = 0xA8,
		NvRegMacAddrB = 0xAC,
		NvRegMulticastAddrA = 0xB0,
		#       define NVREG_MCASTADDRA_FORCE  0x01
		NvRegMulticastAddrB = 0xB4,
		NvRegMulticastMaskA = 0xB8,
		NvRegMulticastMaskB = 0xBC,
		NvRegTxRingPhysAddr = 0x100,
		NvRegRxRingPhysAddr = 0x104,
		NvRegRingSizes = 0x108,
		#       define NVREG_RINGSZ_TXSHIFT 0
		#       define NVREG_RINGSZ_RXSHIFT 16
		NvRegUnknownTransmitterReg = 0x10c,
		NvRegLinkSpeed = 0x110,
		#       define NVREG_LINKSPEED_FORCE 0x10000
		#       define NVREG_LINKSPEED_10    10
		#       define NVREG_LINKSPEED_100   100
		#       define NVREG_LINKSPEED_1000  1000
		NvRegUnknownSetupReg5 = 0x130,
		#       define NVREG_UNKSETUP5_BIT31 (1<<31)
		NvRegUnknownSetupReg3 = 0x134,
		#       define NVREG_UNKSETUP3_VAL1 0x200010
		NvRegUnknownSetupReg8 = 0x13C,
		#       define NVREG_UNKSETUP8_VAL1 0x300010
		NvRegUnknownSetupReg7 = 0x140,
		#       define NVREG_UNKSETUP7_VAL 0x300010
		NvRegTxRxControl = 0x144,
		#       define NVREG_TXRXCTL_KICK  0x0001
		#       define NVREG_TXRXCTL_BIT1  0x0002
		#       define NVREG_TXRXCTL_BIT2  0x0004
		#       define NVREG_TXRXCTL_IDLE  0x0008
		#       define NVREG_TXRXCTL_RESET 0x0010
		NvRegMIIStatus = 0x180,
		#       define NVREG_MIISTAT_ERROR      0x0001
		#       define NVREG_MIISTAT_LINKCHANGE 0x0008
		#       define NVREG_MIISTAT_MASK       0x000f
		#       define NVREG_MIISTAT_MASK2      0x000f
		NvRegUnknownSetupReg4 = 0x184,
		#       define NVREG_UNKSETUP4_VAL 8
		NvRegAdapterControl = 0x188,
		#       define NVREG_ADAPTCTL_START    0x02
		#       define NVREG_ADAPTCTL_LINKUP   0x04
		#       define NVREG_ADAPTCTL_PHYVALID 0x4000
		#       define NVREG_ADAPTCTL_RUNNING  0x100000
		#       define NVREG_ADAPTCTL_PHYSHIFT 24
		NvRegMIISpeed = 0x18c,
		#       define NVREG_MIISPEED_BIT8 (1<<8)
		#       define NVREG_MIIDELAY  5
		NvRegMIIControl = 0x190,
		#       define NVREG_MIICTL_INUSE 0x10000
		#       define NVREG_MIICTL_WRITE 0x08000
		#       define NVREG_MIICTL_ADDRSHIFT  5
		NvRegMIIData = 0x194,
		NvRegWakeUpFlags = 0x200,
		#       define NVREG_WAKEUPFLAGS_VAL               0x7770
		#       define NVREG_WAKEUPFLAGS_BUSYSHIFT         24
		#       define NVREG_WAKEUPFLAGS_ENABLESHIFT       16
		#       define NVREG_WAKEUPFLAGS_D3SHIFT           12
		#       define NVREG_WAKEUPFLAGS_D2SHIFT           8
		#       define NVREG_WAKEUPFLAGS_D1SHIFT           4
		#       define NVREG_WAKEUPFLAGS_D0SHIFT           0
		#       define NVREG_WAKEUPFLAGS_ACCEPT_MAGPAT     0x01
		#       define NVREG_WAKEUPFLAGS_ACCEPT_WAKEUPPAT  0x02
		#       define NVREG_WAKEUPFLAGS_ACCEPT_LINKCHANGE 0x04
		NvRegPatternCRC = 0x204,
		NvRegPatternMask = 0x208,
		NvRegPowerCap = 0x268,
		#       define NVREG_POWERCAP_D3SUPP (1<<30)
		#       define NVREG_POWERCAP_D2SUPP (1<<26)
		#       define NVREG_POWERCAP_D1SUPP (1<<25)
		NvRegPowerState = 0x26c,
		#       define NVREG_POWERSTATE_POWEREDUP 0x8000
		#       define NVREG_POWERSTATE_VALID     0x0100
		#       define NVREG_POWERSTATE_MASK      0x0003
		#       define NVREG_POWERSTATE_D0        0x0000
		#       define NVREG_POWERSTATE_D1        0x0001
		#       define NVREG_POWERSTATE_D2        0x0002
		#       define NVREG_POWERSTATE_D3        0x0003
};

extern  "C"
{
	EXPORTNUM(252) DWORD NTAPI PhyGetLinkState
	(
		IN ULONG Mode
	);

	EXPORTNUM(253) NTSTATUS NTAPI PhyInitialize
	(
		IN ULONG	forceReset,
		IN PVOID	Parameter2
	);
}