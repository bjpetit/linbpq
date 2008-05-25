/* ax.25.c

 * Routines for AX.25 (Amateur Radio X25 Level 2) Protocol dissection
 * Copyright 2005,John Wiseman G8BPQ
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@ethereal.com>
 * Copyright 1998 Gerald Combs
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
 * Decodes AX.25 frames encasulated in BPQEther, AX/IP (ip proto 93) or AX/UDP (UDP Proto 10093)
 * 
 * 
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gmodule.h>
#include <epan/packet.h>
#include <epan/strutil.h>

/* Define version if we are not building ethereal statically */

#include "moduleinfo.h"

int DecodeDigiString(const guint8 * pkt,char * digiString);
char * DecodeControlByte(guint8 ctrl,char * CtrlString,guint8 DestFlag,guint8 SrcFlag);
char * DecodeNetrom(const guint8 * pkt,char * nrString);

extern guint8 INFO_FLAG;		// Information Packet ? 0 No, 1 Yes
extern guint8 NR_INFO_FLAG;		// Netrom Information Packet ? 0 No, 1 Yes


#ifndef ENABLE_STATIC
 G_MODULE_EXPORT const gchar version[] = VERSION;
#endif





/* Initialize the protocol and registered fields */

static int proto_axip = -1;
static int proto_axudp = -1;

static int proto_bpq = -1;
static int proto_ax25 = -1;
static int proto_netrom = -1;

/* Initialize the subtree pointers */

static gint ett_ax25 = -1;
static gint ett_ax25_flags = -1;

static gint ett_bpqeth = -1;
static gint ett_bpqeth_flags = -1;

static gint ett_axip = -1;
static gint ett_axip_flags = -1;

static gint ett_axudp = -1;
static gint ett_axudp_flags = -1;

static gint ett_netrom = -1;
static gint ett_netrom_flags = -1;

static dissector_handle_t ax25_handle;
static dissector_handle_t netrom_handle;
static dissector_handle_t data_handle;

static int hf_axudp_plen = -1;
static int hf_axip_plen = -1;

static int hf_bpq_plen = -1;
static int hf_bpq_fcall = -1;
static int hf_bpq_tcall = -1;
static int hf_bpq_ctrl = -1;
static int hf_bpq_pid = -1;

static int hf_nr_circuit = -1;
static int hf_nr_nr = -1;
static int hf_nr_ns = -1;
static int hf_nr_ttl = -1;
static int hf_nr_opcode = -1;


#define L4BUSY 0x80 //		; BNA - DONT SEND ANY MORE
#define L4NAK  0x40 //		; NEGATIVE RESPONSE FLAG
#define L4MORE 0x20 //		; MORE DATA FOLLOWS - FRAGMENTATION FLAG

#define L4CREQ	1		//; CONNECT REQUEST
#define L4CACK	2		//; CONNECT ACK
#define L4DREQ	3		//; DISCONNECT REQUEST
#define L4DACK	4		//; DISCONNECT ACK
#define L4INFO	5		//; INFORMATION
#define L4IACK	6		//; INFORMATION ACK


static const value_string nrOpcodes[] = {
	{ 1, "Connect Request" },
	{ 2, "Connect ACK" },
	{ 3, "Disconnect Request" },
	{ 4, "Disconnect ACK" },
	{ 5, "Info" },
	{ 6, "Info ACK" },
	{ 0,       NULL },
};
	


static int hf_nr_BUSY = -1;
static int hf_nr_NAK = -1;
static int hf_nr_MORE = -1;
static int hf_nr_OP = -1;



static const gchar * convfromax25(const guint8 *ad,char * outcall)
{
	int in,out=0;
	unsigned char chr;

//
//	CONVERT AX25 FORMAT TO NORMAL FORMAT
//
	memset(outcall,0x0,10);
	for (in=0;in<6;in++)
	{
		chr=ad[in];
		if (chr == 0x40)
			break;
		chr >>= 1;
		outcall[out++]=chr;
	}

	chr=ad[6];				// ssid
	chr >>= 1;
	chr	&= 15;

	if (chr > 0)
	{
		outcall[out++]='-';
		if (chr > 9)
		{
			chr-=10;
			outcall[out++]='1';
		}
		chr+=48;
		outcall[out++]=chr;
	}

	return outcall;
}

// Decode BPQ Ether packets. Hand off content to ax25 decoder

//	There are two formats - BPQ or RLI

static void dissect_bpqeth(tvbuff_t *tvb, packet_info *pinfo _U_ , proto_tree *tree)
{
	tvbuff_t * next_tvb = NULL;
	proto_tree *bpq_tree = NULL;
	proto_item *ti = NULL;

	guint16 bpqelen;				// Length of ax.25 frame within packet
	guint8 lenoffset;				// Offset of length field - 0 for BPQ, 3 for RLI
	BOOL rli;						// TRUE if RLI mode


	bpqelen = tvb_get_letohs(tvb, 3);	// Len if  RLI, two chars from call if bpq

	if (bpqelen < 500)
	{
		lenoffset=3;
		rli=TRUE;
		bpqelen-=5;
	}
	else
	{
		lenoffset=0;
		rli=FALSE;
		bpqelen = tvb_get_letohs(tvb, 0) - 5;
	}


	if (check_col(pinfo->cinfo, COL_PROTOCOL)) 
		col_set_str(pinfo->cinfo, COL_PROTOCOL, "BPQE");
	

	if (check_col(pinfo->cinfo, COL_INFO))
		
		if (rli)
			col_set_str(pinfo->cinfo, COL_INFO, "RLI ");
		else
			col_set_str(pinfo->cinfo, COL_INFO, "BPQ ");
	

	if (tree) {
		ti = proto_tree_add_item(tree, proto_bpq, tvb, 0, bpqelen+lenoffset+2, FALSE);
		bpq_tree = proto_item_add_subtree(ti, ett_bpqeth);

		proto_tree_add_text(bpq_tree, tvb, lenoffset, 2, "Data Length: %d",bpqelen);

		if (rli)
			proto_tree_add_text(bpq_tree, tvb, 0, 3, "RLI Mode");
		else
			proto_tree_add_text(bpq_tree, tvb, 0, 0, "BPQ Mode");

	}

	next_tvb = tvb_new_subset(tvb, lenoffset+2, bpqelen, -1);

	if (tvb_length(next_tvb) && ax25_handle)
		call_dissector(ax25_handle, next_tvb, pinfo, tree);

}

// Decode AX/IP/UDP packets. Hand off content to ax25 decoder


static void dissect_axudp(tvbuff_t *tvb, packet_info *pinfo _U_ , proto_tree *tree)
{
	tvbuff_t * next_tvb = NULL;
	proto_tree *axudp_tree = NULL;
	proto_item *ti = NULL;

	guint16 axudplen;				// Length of ax.25 frame within packet

	axudplen=tvb_length(tvb);		// Remove checksum

	if (check_col(pinfo->cinfo, COL_PROTOCOL)) 
		col_set_str(pinfo->cinfo, COL_PROTOCOL, "AXUDP");


	if (check_col(pinfo->cinfo, COL_INFO))
	{
		if (axudplen < 15)

			// Probably Keepalive - Just display

			col_add_fstr(pinfo->cinfo, COL_INFO, "%s",tvb_get_ptr(tvb,0 ,axudplen), axudplen);
		else
			col_set_str(pinfo->cinfo, COL_INFO, "BPQ ");
	
	}

	if (axudplen > 14)
	{
		axudplen-=2;	// Remove checksum

		next_tvb = tvb_new_subset(tvb, 0, axudplen, -1);
		call_dissector(ax25_handle, next_tvb, pinfo, tree);
	}


}


static void dissect_axip(tvbuff_t *tvb, packet_info *pinfo _U_ , proto_tree *tree)
{
	tvbuff_t * next_tvb = NULL;
	proto_tree *axip_tree = NULL;
	proto_item *ti = NULL;

	guint16 axiplen;				// Length of ax.25 frame within packet

	axiplen=tvb_length(tvb);

	if (check_col(pinfo->cinfo, COL_PROTOCOL)) 
		col_set_str(pinfo->cinfo, COL_PROTOCOL, "AXIP");


	if (check_col(pinfo->cinfo, COL_INFO))

		if (axiplen < 15)

			// Probably Keepalive - Just display

			col_add_fstr(pinfo->cinfo, COL_INFO, "%s",tvb_get_ptr(tvb,0 ,axiplen), axiplen);
		else
			col_set_str(pinfo->cinfo, COL_INFO, "BPQ ");
	
		
	if (axiplen > 14)		
	{
		axiplen-=2;	// Remove checksum

		next_tvb = tvb_new_subset(tvb, 0, axiplen, -1);
		call_dissector(ax25_handle, next_tvb, pinfo, tree);
	}


}


//Code to decode ax.25 packet

static void dissect_ax25(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	proto_item *ti;
	proto_tree *ax25_tree;
	tvbuff_t *next_tvb = NULL;

	int fcall_offset, tcall_offset;
	const guint8      *fcall_val, *tcall_val;
	guint8 ctrl;
	guint8 PID;
	guint x25length,payloadlen;
	int NumberofDigis;
	guint ctrlOffset;
	char * ptr;

	char fcall_str[10], tcall_str[10];

	char digiString[100];
	char ctrlString[100];
	char textString[1000];

	guint8 DestFlag, SrcFlag;

	x25length=tvb_length(tvb);


	tcall_offset = 0;
	tcall_val = tvb_get_ptr(tvb, tcall_offset, 7);
	convfromax25(tcall_val,tcall_str);

	fcall_offset = 7;
	fcall_val = tvb_get_ptr(tvb, fcall_offset, 7);
	convfromax25(fcall_val,fcall_str);

	if (check_col(pinfo->cinfo, COL_PROTOCOL)) 
		col_set_str(pinfo->cinfo, COL_PROTOCOL, "ax.25");

	
	DestFlag = tvb_get_guint8(tvb, 6);
	SrcFlag = tvb_get_guint8(tvb, 13);

	// See if any digis


	if ((SrcFlag & 1) == 0)

		NumberofDigis=DecodeDigiString(tvb_get_ptr(tvb, 14, x25length-14), digiString);
	else
		NumberofDigis=0;


	ctrlOffset=(NumberofDigis*7)+14;

	payloadlen=x25length-ctrlOffset;


	ctrl = tvb_get_guint8(tvb, ctrlOffset);

	ptr = DecodeControlByte(ctrl,&ctrlString[0],DestFlag,SrcFlag);

	*(ptr++)=0;

	if (INFO_FLAG == 1)
	{
		PID = tvb_get_guint8(tvb, ctrlOffset+1);
		sprintf(textString," Pid %x %s",PID,format_text(tvb_get_ptr(tvb,ctrlOffset+2 ,payloadlen-2), payloadlen-2));
	}
	else
		textString[0]=0;




	if (check_col(pinfo->cinfo, COL_INFO)) 
		col_append_fstr(pinfo->cinfo, COL_INFO, "%s>%s %s %s",fcall_str,tcall_str,ctrlString,textString);


	if (tree) {
		ti = proto_tree_add_item(tree, proto_ax25, tvb, 0, x25length, FALSE);
		ax25_tree = proto_item_add_subtree(ti, ett_ax25);


		proto_tree_add_text(ax25_tree, tvb, 0, 7,"  To Callsign: %s",tcall_str);

		proto_tree_add_text(ax25_tree, tvb, 7, 7, "From Callsign: %s",fcall_str);

		if (NumberofDigis > 0)
			proto_tree_add_text(ax25_tree, tvb, 14, NumberofDigis*7, "Digi Callsign: %s",&digiString[1]);

		proto_tree_add_text(ax25_tree, tvb, ctrlOffset, 1, "Control %X=%s",ctrl,ctrlString);
		
		if (INFO_FLAG == 1)
			proto_tree_add_item(ax25_tree, hf_bpq_pid, tvb, ctrlOffset+1, 1, FALSE);


	}

	if ((tvb_length(tvb) > ctrlOffset+2) && (INFO_FLAG == 1))
	{		
		next_tvb = tvb_new_subset(tvb, ctrlOffset+2, -1, -1);
	
		if (PID == 0xcf)
		{
			// Netrom
			if (tvb_length(next_tvb) && data_handle)
				call_dissector(netrom_handle, next_tvb, pinfo, tree);
		}
		else
		{
			if (tvb_length(next_tvb) && data_handle)
				call_dissector(data_handle, next_tvb, pinfo, tree);
		}
	}

}


//Code to decode netrom packet

static void dissect_netrom(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	proto_item *ti;
	proto_tree *netrom_tree,*flags_tree;
	tvbuff_t *next_tvb = NULL;

	guint16 nrlength;

	char * ptr;

	char nrString[1000];

	nrlength=tvb_length(tvb);

	if (check_col(pinfo->cinfo, COL_PROTOCOL)) 
		col_set_str(pinfo->cinfo, COL_PROTOCOL, "NetRom");


	ptr = DecodeNetrom(tvb_get_ptr(tvb, 0, nrlength),&nrString[0]);

	if (check_col(pinfo->cinfo, COL_INFO)) 
		col_add_fstr(pinfo->cinfo, COL_INFO, "%s",nrString);


	if (tree) {
		ti = proto_tree_add_item(tree, proto_netrom, tvb, 0, nrlength, FALSE);
		netrom_tree = proto_item_add_subtree(ti, ett_netrom);

//		ti = proto_tree_add_item(bpq_tree, hf_bpqeth_flags, tvb, 0, 1, FALSE);
//		flags_tree = proto_item_add_subtree(ti, ett_bpqeth_flags);

//		for (i = 0; i < 8; i++)
//			proto_tree_add_item(flags_tree, flags[i], tvb, 0, 1, FALSE);


		proto_tree_add_item(netrom_tree, hf_nr_ttl, tvb, 14, 1, FALSE);
		proto_tree_add_item(netrom_tree, hf_nr_circuit, tvb, 15, 2, FALSE);
		proto_tree_add_item(netrom_tree, hf_nr_ns, tvb, 17, 1, FALSE);
		proto_tree_add_item(netrom_tree, hf_nr_nr, tvb, 18, 1, FALSE);

		
		ti = proto_tree_add_item(netrom_tree, hf_nr_opcode, tvb, 19, 1, FALSE);
		flags_tree = proto_item_add_subtree(ti, ett_netrom_flags);

		proto_tree_add_item(flags_tree, hf_nr_BUSY, tvb, 19, 1, FALSE);
		proto_tree_add_item(flags_tree, hf_nr_NAK, tvb, 19, 1, FALSE);
		proto_tree_add_item(flags_tree, hf_nr_MORE, tvb, 19, 1, FALSE);
		proto_tree_add_item(flags_tree, hf_nr_OP, tvb, 19, 1, FALSE);

	}

	if (tvb_length(tvb) > 20 && NR_INFO_FLAG == 1)
	{
		next_tvb = tvb_new_subset(tvb, 20, -1, -1);

		if (tvb_length(next_tvb) && data_handle)
			call_dissector(data_handle, next_tvb, pinfo, tree);
	}



}



void proto_register_ax25(void)
{                 
	static hf_register_info hf[] = {

		{ &hf_bpq_pid,
			{ "          Pid","bpq.pid",FT_UINT8, BASE_HEX, NULL, 0x0,"", HFILL }
		},

		{ &hf_bpq_ctrl,
			{ "Control Field","bpq.ctrl",FT_UINT8, BASE_HEX, NULL, 0x0,"", HFILL }
		},

		
	};


/* Setup protocol subtree array */
	static gint *ett[] = {
		&ett_ax25,
		&ett_ax25_flags,
	};


/* Register the protocol name and description */
	proto_ax25 = proto_register_protocol("Amateur Radio ax.25",
	    "AX.25", "ax.25");

	register_dissector("ax.25", dissect_ax25, proto_ax25);

/* Required function calls to register the header fields and subtrees used */
	proto_register_field_array(proto_ax25, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));
}


void proto_register_bpq(void)
{

	static hf_register_info hf[] = {

		{ &hf_bpq_plen,
			{ "Packet Len",           "bpq.plen",
			FT_UINT16, BASE_DEC, NULL, 0x0,
			"", HFILL }
		},

		
	};

/* Setup protocol subtree array */
	static gint *axett[] = {
		&ett_bpqeth,
		&ett_bpqeth_flags,
	};


	if (proto_bpq == -1) {
	    proto_bpq = proto_register_protocol (
		"BPQ_ETHER",		/* name */
		"BPQE",		/* short name */
		"BPQ"		/* abbrev */
		);
	}

	proto_register_field_array(proto_bpq, hf, array_length(hf));
	proto_register_subtree_array(axett, array_length(axett));

}
void proto_register_axudp(void)
{

	static hf_register_info hf[] = {

		{ &hf_axudp_plen,
			{ "Packet Len",           "axudp.plen",
			FT_UINT16, BASE_DEC, NULL, 0x0,
			"", HFILL }
		},

		
	};

/* Setup protocol subtree array */
	static gint *axudpett[] = {
		&ett_axudp,
		&ett_axudp_flags,
	};


	if (proto_axudp == -1) {
	    proto_axudp = proto_register_protocol (
		"AX/UDP",		/* name */
		"AXUDP",		/* short name */
		"axudp"		/* abbrev */
		);
	}

	proto_register_field_array(proto_axudp, hf, array_length(hf));
	proto_register_subtree_array(axudpett, array_length(axudpett));

}

void proto_register_axip(void)
{

	static hf_register_info hf[] = {

		{ &hf_axip_plen,
			{ "Packet Len",           "axip.plen",
			FT_UINT16, BASE_DEC, NULL, 0x0,
			"", HFILL }
		},

		
	};

/* Setup protocol subtree array */
	static gint *axipett[] = {
		&ett_axip,
		&ett_axip_flags,
	};


	if (proto_axip == -1) {
	    proto_axip = proto_register_protocol (
		"AX/IP",		/* name */
		"AXIP",		/* short name */
		"axip"		/* abbrev */
		);
	}

	proto_register_field_array(proto_axip, hf, array_length(hf));
	proto_register_subtree_array(axipett, array_length(axipett));

}


void proto_register_netrom(void)
{                 
	static hf_register_info hf[] = {
		{ &hf_nr_ttl,
			{ "TTL",           "nr.ttl",
			FT_UINT8, BASE_DEC, NULL, 0x0,
			"", HFILL }
		},

		{ &hf_nr_circuit,
			{ "Circuit",           "nr.circuit",
			FT_UINT16, BASE_HEX, NULL, 0x0,
			"", HFILL }
		},

		{ &hf_nr_opcode,
			{ "Opcode",           "nr.opcode",
			FT_UINT8, BASE_HEX, NULL, 0x0,
			"", HFILL }
		},

		{ &hf_nr_nr,
			{ "N/R",           "nr.nr",
			FT_UINT8, BASE_DEC, NULL, 0x0,
			"", HFILL }
		},

	
		{ &hf_nr_ns,
			{ "N/S",           "nr.ns",
			FT_UINT8, BASE_DEC, NULL, 0x0,
			"", HFILL }
		},

	
		{ &hf_nr_BUSY,
			{ "Netrom Busy flag",           "nr.flags.busy",
			FT_BOOLEAN, 8, NULL, L4BUSY,
			"", HFILL }
		},

		{ &hf_nr_NAK,
			{ "Netrom NAK flag",           "nr.flags.nak",
			FT_BOOLEAN, 8, NULL, L4NAK,
			"", HFILL }
		},
		{ &hf_nr_MORE,
			{ "Netrom More flag",           "nr.flags.more",
			FT_BOOLEAN, 8, NULL, L4MORE,
			"", HFILL }
		},

		{ &hf_nr_OP,
			{ "Netrom OPCODE flag",           "nr.flags.opcode",
			FT_UINT8, BASE_HEX, VALS(nrOpcodes), 0xf,
			"", HFILL }
		},

	};


/* Setup protocol subtree array */
	static gint *nrett[] = {
		&ett_netrom,
		&ett_netrom_flags,
	};


/* Register the protocol name and description */
	proto_netrom = proto_register_protocol("Amateur Radio NET/ROM",
	    "NETROM", "netrom");

	register_dissector("netrom", dissect_netrom, proto_netrom);

/* Required function calls to register the header fields and subtrees used */

	proto_register_field_array(proto_netrom, hf, array_length(hf));
	proto_register_subtree_array(nrett, array_length(nrett));
}



void plugin_reg_handoff_ax25(void)
{
	return;
}

void plugin_reg_handoff_netrom(void)
{
	return;
}


void proto_reg_handoff_bpq(void) 
{
	static dissector_handle_t bpq_handle = NULL;

	if (!bpq_handle) {
		bpq_handle = create_dissector_handle(dissect_bpqeth, proto_bpq);
	}

	dissector_add("ethertype", 0x8ff, bpq_handle);
}

void proto_reg_handoff_axudp(void)
{
	static dissector_handle_t axudp_handle = NULL;

	if (!axudp_handle) {
		axudp_handle = create_dissector_handle(dissect_axudp, proto_axudp);
	}

	dissector_add("udp.port", 10093, axudp_handle);
}

void proto_reg_handoff_axip(void)
{
	static dissector_handle_t axip_handle = NULL;

	if (!axip_handle) {
		axip_handle = create_dissector_handle(dissect_axip, proto_axip);
	}
	dissector_add("ip.proto", 93, axip_handle);
}

#ifndef ENABLE_STATIC

G_MODULE_EXPORT void plugin_register(void)
{
	/* register the new protocol, protocol fields, and subtrees */

	/* execute protocol initialization only once */
	
	if (proto_bpq == -1) 
		proto_register_bpq();
	
	if (proto_axudp == -1) 
		proto_register_axip();

	if (proto_axudp == -1) 
		proto_register_axudp();
	
	if (proto_ax25 == -1)  
		proto_register_ax25();
	
	if (proto_netrom == -1)  
		proto_register_netrom();
	


}

G_MODULE_EXPORT void
plugin_reg_handoff(void)
{
	proto_reg_handoff_bpq();
	proto_reg_handoff_axudp();
	proto_reg_handoff_axip();
	plugin_reg_handoff_ax25();
	plugin_reg_handoff_netrom();

	data_handle = find_dissector("data");
	ax25_handle = find_dissector("ax.25");
	netrom_handle = find_dissector("netrom");

}


#endif


