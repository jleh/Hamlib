/*
 * hamlib - (C) Frank Singleton 2000 (vk3fcs@ix.netcom.com)
 *
 * ic706.c - Copyright (C) 2000 Stephane Fillod
 * This shared library provides an API for communicating
 * via serial interface to an IC-706,IC-706MKII,IC706-MKIIG
 * using the "CI-V" interface.
 *
 *
 * $Id: ic706.c,v 1.7 2000-10-23 19:48:12 f4cfe Exp $  
 *
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 */

#include <stdlib.h>
#include <stdio.h>   /* Standard input/output definitions */
#include <string.h>  /* String function definitions */
#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */
#include <sys/ioctl.h>

#include <hamlib/rig.h>
#include <hamlib/riglist.h>
#include "icom.h"


#define IC706_ALL_RX_MODES (RIG_MODE_AM|RIG_MODE_CW|RIG_MODE_USB|RIG_MODE_LSB|RIG_MODE_RTTY|RIG_MODE_FM)
#define IC706_1MHZ_TS_MODES (RIG_MODE_AM|RIG_MODE_FM)
#define IC706_1HZ_TS_MODES (RIG_MODE_CW|RIG_MODE_USB|RIG_MODE_LSB|RIG_MODE_RTTY)

/* tx doesn't have WFM.
 * 100W in all modes but AM (40W)
 */ 
#define IC706_OTHER_TX_MODES (RIG_MODE_AM|RIG_MODE_CW|RIG_MODE_USB|RIG_MODE_LSB|RIG_MODE_RTTY|RIG_MODE_FM)
#define IC706_AM_TX_MODES (RIG_MODE_AM)

#define IC706_FUNC_ALL (RIG_FUNC_FAGC|RIG_FUNC_NB|RIG_FUNC_COMP|RIG_FUNC_VOX|RIG_FUNC_TONE|RIG_FUNC_TSQL|RIG_FUNC_SBKIN|RIG_FUNC_FBKIN)

#define IC706_LEVEL_ALL (RIG_LEVEL_PREAMP|RIG_LEVEL_ATT|RIG_LEVEL_AGC|RIG_LEVEL_SQL|RIG_LEVEL_SQLSTAT|RIG_LEVEL_STRENGTH)

/*
 * ic706 rigs capabilities.
 * Notice that some rigs share the same functions.
 * Also this struct is READONLY!
 */
const struct rig_caps ic706_caps = {
  RIG_MODEL_IC706, "IC-706", "Icom", "0.2", RIG_STATUS_ALPHA,
  RIG_TYPE_MOBILE, RIG_PTT_NONE, 300, 19200, 8, 1, RIG_PARITY_NONE, 
  RIG_HANDSHAKE_NONE, 0, 0, 2000, 3, IC706_FUNC_ALL, IC706_LEVEL_ALL,
  IC706_LEVEL_ALL, 101, RIG_TRN_ON,
  { {KHz(30),199999999,IC706_ALL_RX_MODES,-1,-1},{0,0,0,0,0}, }, /* rx range */
  { {KHz(1800),1999999,IC706_OTHER_TX_MODES,5000,100000},	/* 100W class */
    {KHz(1800),1999999,IC706_AM_TX_MODES,2000,40000},	/* 40W class */
    {KHz(3500),3999999,IC706_OTHER_TX_MODES,5000,100000},
    {KHz(3500),3999999,IC706_AM_TX_MODES,2000,40000},
    {MHz(7),KHz(7300),IC706_OTHER_TX_MODES,5000,100000},
    {MHz(7),KHz(7300),IC706_AM_TX_MODES,2000,40000},
    {MHz(10),KHz(10150),IC706_OTHER_TX_MODES,5000,100000},
    {MHz(10),KHz(10150),IC706_AM_TX_MODES,2000,40000},
    {MHz(14),KHz(14350),IC706_OTHER_TX_MODES,5000,100000},
    {MHz(14),KHz(14350),IC706_AM_TX_MODES,2000,40000},
    {KHz(18068),KHz(18168),IC706_OTHER_TX_MODES,5000,100000},
    {KHz(18068),KHz(18168),IC706_AM_TX_MODES,2000,40000},
    {MHz(21),KHz(21450),IC706_OTHER_TX_MODES,5000,100000},
    {MHz(21),KHz(21450),IC706_AM_TX_MODES,2000,40000},
    {KHz(24890),KHz(24990),IC706_OTHER_TX_MODES,5000,100000},
    {KHz(24890),KHz(24990),IC706_AM_TX_MODES,2000,40000},
    {MHz(28),KHz(29700),IC706_OTHER_TX_MODES,5000,100000},
    {MHz(28),KHz(29700),IC706_AM_TX_MODES,2000,40000},
    {MHz(50),MHz(54),IC706_OTHER_TX_MODES,5000,100000},
    {MHz(50),MHz(54),IC706_AM_TX_MODES,2000,40000},
    {MHz(144),MHz(148),IC706_OTHER_TX_MODES,5000,20000}, /* not sure.. */
    {MHz(144),MHz(148),IC706_AM_TX_MODES,2000,8000}, /* anyone? */
	{0,0,0,0,0} },
	{{IC706_1HZ_TS_MODES,1},
	 {IC706_ALL_RX_MODES,10},
	 {IC706_ALL_RX_MODES,100},
	 {IC706_ALL_RX_MODES,KHz(1)},
	 {IC706_ALL_RX_MODES,KHz(5)},
	 {IC706_ALL_RX_MODES,KHz(9)},
	 {IC706_ALL_RX_MODES,KHz(10)},
	 {IC706_ALL_RX_MODES,12500},
	 {IC706_ALL_RX_MODES,KHz(20)},
	 {IC706_ALL_RX_MODES,KHz(25)},
	 {IC706_ALL_RX_MODES,KHz(100)},
	 {IC706_1MHZ_TS_MODES,MHz(1)},
	 {0,0}
	},
  icom_init, icom_cleanup, NULL, NULL, NULL /* probe not supported yet */,
  icom_set_freq, icom_get_freq, icom_set_mode, icom_get_mode, icom_set_vfo, NULL,
};

const struct rig_caps ic706mkii_caps = {
  RIG_MODEL_IC706MKII, "IC-706MKII", "Icom", "0.2", RIG_STATUS_ALPHA,
  RIG_TYPE_MOBILE, RIG_PTT_NONE, 300, 19200, 8, 1, RIG_PARITY_NONE, 
  RIG_HANDSHAKE_NONE, 0, 0, 2000, 3, IC706_FUNC_ALL, IC706_LEVEL_ALL,
  IC706_LEVEL_ALL, 101, RIG_TRN_ON,
  { {30000,199999999,IC706_ALL_RX_MODES,-1,-1}, {0,0,0,0,0}, }, /* rx range */
  { {1800000,1999999,IC706_OTHER_TX_MODES,5000,100000},	/* 100W class */
    {1800000,1999999,IC706_AM_TX_MODES,2000,40000},	/* 40W class */
    {3500000,3999999,IC706_OTHER_TX_MODES,5000,100000},
    {3500000,3999999,IC706_AM_TX_MODES,2000,40000},
    {7000000,7300000,IC706_OTHER_TX_MODES,5000,100000},
    {7000000,7300000,IC706_AM_TX_MODES,2000,40000},
    {10000000,10150000,IC706_OTHER_TX_MODES,5000,100000},
    {10000000,10150000,IC706_AM_TX_MODES,2000,40000},
    {14000000,14350000,IC706_OTHER_TX_MODES,5000,100000},
    {14000000,14350000,IC706_AM_TX_MODES,2000,40000},
    {18068000,18168000,IC706_OTHER_TX_MODES,5000,100000},
    {18068000,18168000,IC706_AM_TX_MODES,2000,40000},
    {21000000,21450000,IC706_OTHER_TX_MODES,5000,100000},
    {21000000,21450000,IC706_AM_TX_MODES,2000,40000},
    {24890000,24990000,IC706_OTHER_TX_MODES,5000,100000},
    {24890000,24990000,IC706_AM_TX_MODES,2000,40000},
    {28000000,29700000,IC706_OTHER_TX_MODES,5000,100000},
    {28000000,29700000,IC706_AM_TX_MODES,2000,40000},
    {50000000,54000000,IC706_OTHER_TX_MODES,5000,100000},
    {50000000,54000000,IC706_AM_TX_MODES,2000,40000},
    {144000000,148000000,IC706_OTHER_TX_MODES,5000,20000}, /* not sure.. */
    {144000000,148000000,IC706_AM_TX_MODES,2000,8000}, /* anyone? */
	{0,0,0,0,0} },
	{{IC706_1HZ_TS_MODES,1},
	 {IC706_ALL_RX_MODES,10},
	 {IC706_ALL_RX_MODES,100},
	 {IC706_ALL_RX_MODES,KHz(1)},
	 {IC706_ALL_RX_MODES,KHz(5)},
	 {IC706_ALL_RX_MODES,KHz(9)},
	 {IC706_ALL_RX_MODES,KHz(10)},
	 {IC706_ALL_RX_MODES,12500},
	 {IC706_ALL_RX_MODES,KHz(20)},
	 {IC706_ALL_RX_MODES,KHz(25)},
	 {IC706_ALL_RX_MODES,KHz(100)},
	 {IC706_1MHZ_TS_MODES,MHz(1)},
	 {0,0}
	},
  icom_init, icom_cleanup, NULL, NULL, NULL /* probe not supported yet */,
  icom_set_freq, icom_get_freq, icom_set_mode, icom_get_mode, icom_set_vfo, NULL,
};


/*
 * Basically, the IC706MKIIG is an IC706MKII plus UHF, a DSP
 * and 50W VHF
 */
const struct rig_caps ic706mkiig_caps = {
  RIG_MODEL_IC706MKIIG, "IC-706MKIIG", "Icom", "0.2", RIG_STATUS_ALPHA,
  RIG_TYPE_MOBILE, RIG_PTT_NONE, 300, 19200, 8, 1, RIG_PARITY_NONE, 
  RIG_HANDSHAKE_NONE, 0, 0, 2000, 3, IC706_FUNC_ALL|RIG_FUNC_NR|RIG_FUNC_ANF,
  IC706_LEVEL_ALL, IC706_LEVEL_ALL, 101, RIG_TRN_ON,
  { {30000,199999999,IC706_ALL_RX_MODES,-1,-1},	/* this trx also has UHF */
 	{400000000,470000000,IC706_ALL_RX_MODES,-1,-1}, {0,0,0,0,0}, },
  { {1800000,1999999,IC706_OTHER_TX_MODES,5000,100000},	/* 100W class */
    {1800000,1999999,IC706_AM_TX_MODES,2000,40000},	/* 40W class */
    {3500000,3999999,IC706_OTHER_TX_MODES,5000,100000},
    {3500000,3999999,IC706_AM_TX_MODES,2000,40000},
	{7000000,7300000,IC706_OTHER_TX_MODES,5000,100000},
    {7000000,7300000,IC706_AM_TX_MODES,2000,40000},
    {10000000,10150000,IC706_OTHER_TX_MODES,5000,100000},
    {10000000,10150000,IC706_AM_TX_MODES,2000,40000},
    {14000000,14350000,IC706_OTHER_TX_MODES,5000,100000},
    {14000000,14350000,IC706_AM_TX_MODES,2000,40000},
    {18068000,18168000,IC706_OTHER_TX_MODES,5000,100000},
    {18068000,18168000,IC706_AM_TX_MODES,2000,40000},
    {21000000,21450000,IC706_OTHER_TX_MODES,5000,100000},
    {21000000,21450000,IC706_AM_TX_MODES,2000,40000},
    {24890000,24990000,IC706_OTHER_TX_MODES,5000,100000},
    {24890000,24990000,IC706_AM_TX_MODES,2000,40000},
    {28000000,29700000,IC706_OTHER_TX_MODES,5000,100000},
    {28000000,29700000,IC706_AM_TX_MODES,2000,40000},
    {50000000,54000000,IC706_OTHER_TX_MODES,5000,100000},
    {50000000,54000000,IC706_AM_TX_MODES,2000,40000},
    {144000000,148000000,IC706_OTHER_TX_MODES,5000,50000}, /* 50W */
    {144000000,148000000,IC706_AM_TX_MODES,2000,20000}, /* AM VHF is 20W */
    {430000000,450000000,IC706_OTHER_TX_MODES,5000,20000},
    {430000000,450000000,IC706_AM_TX_MODES,2000,8000},
	{0,0,0,0,0}, },
	{{IC706_1HZ_TS_MODES,1},
	 {IC706_ALL_RX_MODES,10},
	 {IC706_ALL_RX_MODES,100},
	 {IC706_ALL_RX_MODES,KHz(1)},
	 {IC706_ALL_RX_MODES,KHz(5)},
	 {IC706_ALL_RX_MODES,KHz(9)},
	 {IC706_ALL_RX_MODES,KHz(10)},
	 {IC706_ALL_RX_MODES,12500},
	 {IC706_ALL_RX_MODES,KHz(20)},
	 {IC706_ALL_RX_MODES,KHz(25)},
	 {IC706_ALL_RX_MODES,KHz(100)},
	 {IC706_1MHZ_TS_MODES,MHz(1)},
	 {0,0}
	},
  icom_init, icom_cleanup, NULL, NULL, NULL /* probe not supported yet */,
  icom_set_freq, icom_get_freq, icom_set_mode, icom_get_mode, icom_set_vfo,
  NULL, 
decode_event: icom_decode_event,
set_level: icom_set_level,
get_level: icom_get_level,
set_channel: icom_set_channel,
get_channel: icom_get_channel,
set_mem: icom_set_mem,
mv_ctl: icom_mv_ctl,
set_ptt: icom_set_ptt,
get_ptt: icom_get_ptt,
set_ts: icom_set_ts,
get_ts: icom_get_ts,
set_rptr_shift: icom_set_rptr_shift,
get_rptr_shift: icom_get_rptr_shift,
};


/*
 * Function definitions below
 */


