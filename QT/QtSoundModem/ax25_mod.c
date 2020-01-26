// UZ7HO Soundmodem Port

#include "UZ7HOStuff.h"

// I assume this modulates (and sends?} frames

int RSEncode(UCHAR * bytToRS, UCHAR * RSBytes, int DataLen, int RSLen);

//unit ax25_mod;

//interface

//uses sysutils,classes,math

extern int SampleNo;

extern BOOL KISSServ;

extern TStringList KISS_acked[];
extern TStringList KISS_iacked[];

extern UCHAR modem_mode[];

#define sbc 175

extern single  ch_offset[4];

#define COS45 0.70710676908493f

#define TX_SILENCE 0
#define TX_DELAY 1
#define TX_TAIL 2
#define TX_NO_DATA 3
#define TX_FRAME 4
#define TX_WAIT_BPF 5


#define TX_BIT0 0
#define TX_BIT1 1
#define FRAME_EMPTY 0
#define FRAME_FULL 1
#define FRAME_NO_FRAME 2
#define FRAME_NEW_FRAME 3
#define BYTE_EMPTY 0
#define BYTE_FULL 1


UCHAR gray_8PSK[8] = {7,0,6,5,2,1,3,4};		// ?? was 1::8
UCHAR gray_PI4QPSK[4] = {3,1,5,7};


float audio_buf[5][32768];  // [1..4,0..32767]
float tx_src_BPF_buf[5][32768];
float tx_BPF_buf[5][32768];
float tx_prev_BPF_buf[5][32768];
float tx_BPF_core[5][32768];

long tx_delay_cnt[5] = {0};		//			 : array[1..4] of longword=(0,0,0,0};
long tx_tail_cnt[5] = {0};

int tx_hitoneraisedb[5] = {0};		//   : array[1..4] of integer=(0,0,0,0};
float tx_hitoneraise[5] = {0};		//     : array[1..4] of single=(0,0,0,0};
float tx_freq[5] = { 1000, 1000, 1000, 1000, 1000};			//			     : array[1..4] of single=(1000,1000,1000,1000};
float tx_shift[5] = { 200, 200, 200, 200, 200};				  //   : array[1..4] of single=(200,200,200,200};
float tx_bit_mod[5] = {1, 1, 1, 1, 1};		//			   : array[1..4] of single=(1,1,1,1};
float tx_osc[5] = {0};		//						 : array[1..4] of single=(0,0,0,0};
float tx_bit_osc[5] = {0};		//			   : array[1..4] of single=(0,0,0,0};
unsigned short txbpf[5] = { 400, 400, 400, 400, 400};		//						  : array[1..4] of word=(400,400,400,400};
unsigned short  tx_BPF_tap[5] = { 256, 256, 256, 256, 256};		//			   : array[1..4] of word=(256,256,256,256};
unsigned short  tx_baudrate[5] = { 300, 300, 300, 300, 300};		//			  : array[1..4] of word=(300,300,300,300};
unsigned short  tx_BPF_timer[5] = {0};		//			 : array[1..4] of word=(0,0,0,0};
UCHAR tx_pol[5] = {0};		//						 : array[1..4] of byte=(0,0,0,0};
UCHAR tx_last_pol[5] = {0};		//			  : array[1..4] of byte=(0,0,0,0};
UCHAR tx_last_diddle[5] = {0};		//     : array[1..4] of byte=(0,0,0,0};
UCHAR tx_flag_cnt[5] = {0};		//			  : array[1..4] of byte=(0,0,0,0};
UCHAR tx_frame_status[5] = {0};		//    : array[1..4] of byte=(0,0,0,0};
UCHAR tx_byte_status[5] = {0};		//     : array[1..4] of byte=(0,0,0,0};
UCHAR tx_status[5] = {0};		//			     : array[1..4] of byte=(0,0,0,0};
UCHAR tx_bit_stuff_cnt[5] = {0};		//    : array[1..4] of byte=(0,0,0,0};
UCHAR tx_bit_cnt[5] = {0};		//			    : array[1..4] of byte=(0,0,0,0};
UCHAR tx_last_bit[5] = {0};		//			   : array[1..4] of byte=(0,0,0,0};
UCHAR tx_bit_stream[5] = {0};		//			 : array[1..4] of byte=(0,0,0,0};

UCHAR tx_8PSK[5] = {0};		//						 : array[1..4] of byte=(0,0,0,0};
UCHAR tx_QPSK[5] = {0};		//						 : array[1..4] of byte=(0,0,0,0};

float tx_I_mod[5] = {1, 1, 1, 1, 1};		//						: array[1..4] of single=(1,1,1,1};
float tx_Q_mod[5] = {1, 1, 1, 1, 1};		//						: array[1..4] of single=(1,1,1,1};
float tx_QPSK_avg_I[5] = {0};		//			 : array[1..4] of single=(0,0,0,0};
float tx_QPSK_avg_Q[5] = {0};		//			 : array[1..4] of single=(0,0,0,0};
float tx_QPSK_df_I[5] = {0};		//			  : array[1..4] of single=(0,0,0,0};
float tx_QPSK_df_Q [5] = {0};		//			 : array[1..4] of single=(0,0,0,0};
float tx_QPSK_I[5] = {0};		//			     : array[1..4] of single=(0,0,0,0};
float tx_QPSK_Q[5] = {0};		//			     : array[1..4] of single=(0,0,0,0};
float tx_QPSK_old_I[5] = {0};		//			 : array[1..4] of single=(0,0,0,0};
float tx_QPSK_old_Q[5] = {0};		//			 : array[1..4] of single=(0,0,0,0};
float tx_8PSK_avg_I[5] = {0};		//			 : array[1..4] of single=(0,0,0,0};
float tx_8PSK_avg_Q[5] = {0};		//			 : array[1..4] of single=(0,0,0,0};
float tx_8PSK_df_I[5] = {0};		//			  : array[1..4] of single=(0,0,0,0};
float tx_8PSK_df_Q[5] = {0};		//			  : array[1..4] of single=(0,0,0,0};
float tx_8PSK_I[5] = {0};		//			     : array[1..4] of single=(0,0,0,0};
float tx_8PSK_Q[5] = {0};		//			     : array[1..4] of single=(0,0,0,0};
float tx_8PSK_old_I[5] = {0};		//			 : array[1..4] of single=(0,0,0,0};
float tx_8PSK_old_Q[5] = {0};		//			 : array[1..4] of single=(0,0,0,0};

float tx_osc1[5] = {0};		//						 : array[1..4] of single=(0,0,0,0};
float tx_osc2[5] = {0};		//						 : array[1..4] of single=(0,0,0,0};
float tx_osc3[5] = {0};		//						 : array[1..4] of single=(0,0,0,0};
float tx_osc4[5] = {0};		//						 : array[1..4] of single=(0,0,0,0};
short tx_inv1[5] = {1, 1, 1, 1, 1};		//						: array[1..4] of shortint=(1,1,1,1};
short tx_inv2[5] = {1, 1, 1, 1, 1};		//						: array[1..4] of shortint=(1,1,1,1};
short tx_inv3[5] = {1, 1, 1, 1, 1};		//						: array[1..4] of shortint=(1,1,1,1};
short tx_inv4[5] = {1, 1, 1, 1, 1};		//						: array[1..4] of shortint=(1,1,1,1};
short tx_old_inv1[5] = {1, 1, 1, 1, 1};		//			  : array[1..4] of shortint=(1,1,1,1};
short tx_old_inv2[5] = {1, 1, 1, 1, 1};		//			  : array[1..4] of shortint=(1,1,1,1};
short tx_old_inv3[5] = {1, 1, 1, 1, 1};		//			  : array[1..4] of shortint=(1,1,1,1};
short tx_old_inv4[5] = {1, 1, 1, 1, 1};		//			  : array[1..4] of shortint=(1,1,1,1};
float tx_bit1_mod[5] = {1, 1, 1, 1, 1};		//			  : array[1..4] of single=(1,1,1,1};
float tx_bit2_mod[5] = {1, 1, 1, 1, 1};		//			  : array[1..4] of single=(1,1,1,1};
float tx_bit3_mod[5] = {1, 1, 1, 1, 1};		//			  : array[1..4] of single=(1,1,1,1};
float tx_bit4_mod[5] = {1, 1, 1, 1, 1};		//			  : array[1..4] of single=(1,1,1,1};
UINT tx_viterbi[5] = {0};		//			     : array[1..4] of word=(0,0,0,0};
UCHAR tx_intv_tbl[5][4];		//			  : array[1..4,0..3] of byte;

short tx_inv[5] = {1, 1, 1, 1, 1};		//						    : array[1..4] of shortint=(1,1,1,1};
BOOL tx_change_phase[5] = {0};		//    : array[1..4] of boolean=(FALSE,FALSE,FALSE,FALSE};
BOOL tx_bs_bit[5] = {0};		//			    : array[1..4] of boolean=(FALSE,FALSE,FALSE,FALSE};

string * tx_data[5] = {0};		//						: array[1..4] of string=('','','',''};
int tx_data_len[5] = {0};


//  uses sm_main,ax25,ax25_agw,ax25_demod,rsunit;

UCHAR tx_nrzi(UCHAR snd_ch, UCHAR bit)
{
	if (bit == TX_BIT0)
	{
		// Zero so switch bit

		tx_last_bit[snd_ch] ^= 1;
	}
	return tx_last_bit[snd_ch];	
}

BOOL tx_bit_stuffing(UCHAR snd_ch, UCHAR bit)
{					 
 // result = FALSE;
 // if bit=TX_BIT1 then inc(tx_bit_stuff_cnt[snd_ch]};
 // if bit=TX_BIT0 then tx_bit_stuff_cnt[snd_ch] = 0;
 // if tx_bit_stuff_cnt[snd_ch]=5 then begin tx_bit_stuff_cnt[snd_ch] = 0; result = TRUE; end;
//end;

	if (bit == TX_BIT1)
		tx_bit_stuff_cnt[snd_ch]++;

	if (bit == TX_BIT0)
		tx_bit_stuff_cnt[snd_ch] = 0;

	if (tx_bit_stuff_cnt[snd_ch] == 5)
	{
		tx_bit_stuff_cnt[snd_ch] = 0;
		Debugprintf("Stuffing 0");
		return TRUE;
	}

	return FALSE;
}




void interleave(char *s, int len)
{
//	var
 // data: string;
 // i,k,len: word;
 // nr_blocks: word;
//begin{
//  data = '';
 // len = length(s};
 // if len>0 then nr_blocks = ((len-1} div 16}+1 else nr_blocks = 1;
 // for i = 1 to 16 do
 //   for k = 0 to nr_blocks-1 do
  //   if (i+k*16}<=len then data = data+s[i+k*16];
 // result = data;
//end;

	char data[1024];

	UINT i,k;
	UINT nr_blocks;
	int n = 0;

	if (len > 0)
		nr_blocks = ((len - 1) / 16) + 1;
	else
		nr_blocks = 1;

	for (i = 0; i < 16; i++)
	{
		for (k = 0; k < nr_blocks - 1; k++)
		{
			if ((i + k * 16) <= len)
				data[n++] = s[i + k * 16];
		}
	}

	memcpy(s, data, len);
}

//procedure get_new_frame(snd_ch: byte; var frame_stream: TStringList};
//var
//  header,line,temp: string;
//  len,i,size: word;
 // crc: word;
//begin

void get_new_frame(UCHAR snd_ch, TStringList * frame_stream)
{
	UCHAR header[256];
	UCHAR line[1024];

	int LineLen;

	string ** Items;

	string * myTemp;

	UCHAR temp[1024];
	
	UINT len,i,size;
	UINT crc;

	tx_bs_bit[snd_ch] = FALSE;
	tx_bit_cnt[snd_ch] = 0;
	tx_flag_cnt[snd_ch] = 0;
	tx_bit_stuff_cnt[snd_ch] = 0;
	tx_bit_stream[snd_ch] = FRAME_FLAG;
	tx_frame_status[snd_ch] = FRAME_NEW_FRAME;
	tx_byte_status[snd_ch] = BYTE_EMPTY;

	if (frame_stream->Count == 0)
	{
		tx_frame_status[snd_ch] = FRAME_NO_FRAME;
		return;
	}

	// We now pass control byte and ack bytes on front and pointer to socket on end if ackmode

	myTemp = Strings(frame_stream, 0);			// get message

	if ((myTemp->Data[0] & 0x0f) == 12)			// ACKMODE
	{
		// Save copy then copy data up 3 bytes

		Add(&KISS_acked[snd_ch], duplicateString(myTemp));

		mydelete(myTemp, 0, 3);
		myTemp->Length -= sizeof(void *);
	}
	else
	{
		// Just remove control 

		mydelete(myTemp, 0, 1);
	}

	tx_data[snd_ch] = duplicateString(myTemp);		// so can free original below

	Delete(frame_stream, 0);			// This will invalidate temp

/*
	// Do KISS ACK (is this the right place) -  DON'T THINK SO!

    if (KISSServ)
	{
		if (Count(&KISS_acked[snd_ch]) > 0)
		{
			if (tx_data[snd_ch] == Strings(&KISS_acked[snd_ch], 0));
			{
				KISS_send_ack(snd_ch, Strings(&KISS_iacked[snd_ch], 0));
				Delete(&KISS_acked[snd_ch], 0);
				Delete(&KISS_iacked[snd_ch], 0);
			 //   except
			  //    KISS.acked[snd_ch].Clear;
			   //   KISS.iacked[snd_ch].Clear;
	
			}

			if (Count(frame_stream) == 0)
			{
				Clear(&KISS_acked[snd_ch]);
				Clear(&KISS_iacked[snd_ch]);
			}
		}
	}
	*/

	//AGW_AX25_frame_analiz(snd_ch,FALSE,tx_data[snd_ch]};

    put_frame(snd_ch, tx_data[snd_ch], "" , TRUE, FALSE);
    
	if (tx_data[snd_ch]->Length == 0 || modem_mode[snd_ch] != MODE_MPSK)
		return;

	// Reformat MPSK Data

	//	line->Data =  _strdup("");
		LineLen = 0;

		do 
		{
			// function copy ( Source : string; StartChar, Count : Integer ) : string;

			size = tx_data[snd_ch]->Length;

			if (size > 8)
				size = 8;
 
			memcpy(temp, tx_data[snd_ch]->Data, size);
			
			// Delete the chars from tx_data
			  
			mydelete(tx_data[snd_ch], 0, 8);

			if (size > 0)
			{
				memset(xData, 0, sizeof(xData));
				memset(xEncoded, 0, sizeof(xEncoded));

				// procedure move ( const SourcePointer; var DestinationPointer; CopyCount : Integer ) ;
				// Description
				// The move procedure is a badly named method of copying a section of memory from one place to another.
 
				// CopyCount bytes are copied from storage referenced by SourcePointer and written to DestinationPointer
 
				memcpy(xData, temp, size);

				// Can probably use my RS code - try anyway

				RSEncode(xData, xEncoded, size + (MaxErrors * 2), MaxErrors * 2);


				//RS.InitBuffers;
				//RS.EncodeRS(xData,xEncoded);

				memcpy(&line[LineLen], xData, size);
				memcpy(&line[LineLen + size], xEncoded, MaxErrors * 2);


				LineLen += size + (MaxErrors * 2);
			}
		}

		while(tx_data[snd_ch]->Length > 0);
		
		len = LineLen;
	
		interleave(line, LineLen);
		scrambler(line, LineLen);

		header[0] = 0x7e;
		header[1] = 0x7e;
		header[2] = len >> 8;
		header[3] = len;

		crc = get_fcs(header, 4);

		header[4] = crc >> 8;
		header[5] = crc;
	
		memset(xData, 0, sizeof(xData));
		memset(xEncoded, 0, sizeof(xEncoded));
		memmove(xData, header, 6);


		RSEncode(xData, xEncoded, 6 + (MaxErrors * 2), MaxErrors * 2);

	
		// RS.InitBuffers;
		// RS.EncodeRS(xData,xEncoded};

		// We should now have RS Encoded Header in xEncoded;

		// I think we send encoded header then line

		tx_data[snd_ch]->Length = 0;

		stringAdd(tx_data[snd_ch], xData, 6);
		stringAdd(tx_data[snd_ch], xEncoded,  MaxErrors * 2);
		stringAdd(tx_data[snd_ch], line, LineLen);

	}
  


	int get_new_bit(byte snd_ch, byte bit)
	{
		unsigned short len;
		string * s;

		if (tx_frame_status[snd_ch] == FRAME_FULL)
		{
			if (tx_byte_status[snd_ch] == BYTE_EMPTY)
			{
				len = tx_data[snd_ch]->Length;

				if (len > 0)
				{
					s = tx_data[snd_ch];
					tx_bit_stream[snd_ch] = (s->Data[0]);
					tx_frame_status[snd_ch] = FRAME_FULL;
					tx_byte_status[snd_ch] = BYTE_FULL;
					tx_bit_cnt[snd_ch] = 0;
					mydelete(tx_data[snd_ch], 0, 1);
				}

				else tx_frame_status[snd_ch] = FRAME_EMPTY;
			}

			if (tx_byte_status[snd_ch] == BYTE_FULL)
				bit = tx_bit_stream[snd_ch] & TX_BIT1;

			if (modem_mode[snd_ch] == MODE_MPSK)
			{
				tx_bit_cnt[snd_ch]++;
				tx_bit_stream[snd_ch] = tx_bit_stream[snd_ch] >> 1;
				if (tx_bit_cnt[snd_ch] >= 8)
					tx_byte_status[snd_ch] = BYTE_EMPTY;

			}
			else
			{
				if (tx_bs_bit[snd_ch])
					bit = TX_BIT0;

				tx_bs_bit[snd_ch] = tx_bit_stuffing(snd_ch, bit);

				if (!tx_bs_bit[snd_ch])
				{
					tx_bit_cnt[snd_ch]++;
					tx_bit_stream[snd_ch] >>= 1;
					if (tx_bit_cnt[snd_ch] >= 8 && !tx_bs_bit[snd_ch])
						tx_byte_status[snd_ch] = BYTE_EMPTY;
				}
			}
		}

		if (tx_frame_status[snd_ch] == FRAME_EMPTY)
			get_new_frame(snd_ch, &all_frame_buf[snd_ch]);

		if ((tx_frame_status[snd_ch] == FRAME_NEW_FRAME) || (tx_frame_status[snd_ch] == FRAME_NO_FRAME))
		{
			bit = tx_bit_stream[snd_ch] & TX_BIT1;
			tx_flag_cnt[snd_ch]++;
			tx_bit_stream[snd_ch] >>= 1;

			if (tx_flag_cnt[snd_ch] == 8)
			{
				switch (tx_frame_status[snd_ch])
				{
				case FRAME_NEW_FRAME:
					tx_frame_status[snd_ch] = FRAME_FULL;
					break;

				case FRAME_NO_FRAME:
					tx_tail_cnt[snd_ch] = 0;
					tx_frame_status[snd_ch] = FRAME_EMPTY;
					tx_status[snd_ch] = TX_TAIL;
					Debugprintf("Start TXTAIL %d", SampleNo);


					break;
				}
			}
		}
		return bit;
	}



int get_new_bit_tail(UCHAR snd_ch, UCHAR bit)
{
	ULONG _txtail = 0;
	UCHAR _diddles;

	if (modem_mode[snd_ch] == MODE_FSK)
		_diddles = diddles;
	else
		_diddles = 0;

	if (modem_mode[snd_ch] == MODE_FSK)
		_txtail = txtail[snd_ch];

	if (modem_mode[snd_ch] == MODE_BPSK)
		_txtail = txtail[snd_ch];

	if (modem_mode[snd_ch] == MODE_8PSK)
		_txtail = txtail[snd_ch] * 3;

	if (modem_mode[snd_ch] == MODE_QPSK || modem_mode[snd_ch] == MODE_PI4QPSK)
	  _txtail = txtail[snd_ch] << 1;
  
  
	if (modem_mode[snd_ch] == MODE_MPSK)
		_txtail = txtail[snd_ch] << 2;

	_txtail = _txtail * tx_baudrate[snd_ch] / 1000;

	if (qpsk_set[snd_ch].mode == QPSK_V26 || modem_mode[snd_ch] == MODE_8PSK)
		_diddles = 2;

	switch (_diddles)
	{
	case 0:
    
		if (tx_tail_cnt[snd_ch] < _txtail)
		{
			bit = TX_BIT0;
			tx_tail_cnt[snd_ch]++;
		}
		else
		{
			Debugprintf("End TXTAIL %d", SampleNo);
			tx_status[snd_ch] = TX_WAIT_BPF;
		}

		break;

	case 1:

		if (tx_tail_cnt[snd_ch] < _txtail)
		{
			if (tx_last_diddle[snd_ch] == TX_BIT0)
				bit = TX_BIT1;
			else
				bit = TX_BIT0;

			tx_tail_cnt[snd_ch]++;
			tx_last_diddle[snd_ch] = bit;
		}
		else
		{
			Debugprintf("End TXTAIL %d", SampleNo);
			tx_status[snd_ch] = TX_WAIT_BPF;
		}
		
		break;
	
	case 2:
    
		if (tx_tail_cnt[snd_ch] < _txtail)
		{
			bit = FRAME_FLAG >> (tx_tail_cnt[snd_ch] % 8) & 1;
			tx_tail_cnt[snd_ch]++;
		}
		else
		{
			Debugprintf("End TXTAIL %d", SampleNo);
			tx_status[snd_ch] = TX_WAIT_BPF;
		}
		break;
	}
	return bit;
}

int get_new_bit_delay(UCHAR snd_ch, UCHAR bit)
{
	ULONG _txdelay = 0;
	UCHAR _diddles;

	_diddles = 0;

	switch (modem_mode[snd_ch])
	{
	case MODE_FSK:
	
		_diddles = diddles;
		break;

	case MODE_PI4QPSK:
	case MODE_8PSK:

		_diddles = 2;
		break;

    case MODE_QPSK:
		
		if (qpsk_set[snd_ch].mode == QPSK_V26)
			_diddles = 2;
		break;
	}

	if (modem_mode[snd_ch] == MODE_FSK)
		_txdelay = txdelay[snd_ch];

	else if (modem_mode[snd_ch] == MODE_BPSK)
		_txdelay = txdelay[snd_ch];

	else if (modem_mode[snd_ch] == MODE_8PSK)
		_txdelay = txdelay[snd_ch] * 3;

	else if (modem_mode[snd_ch] == MODE_QPSK || modem_mode[snd_ch] == MODE_PI4QPSK)
		_txdelay = txdelay[snd_ch] << 1;

	else if (modem_mode[snd_ch] == MODE_MPSK)
	{
		if (txdelay[snd_ch] < 400)
			_txdelay = 400 << 2;		 //AFC delay
		else
			_txdelay = txdelay[snd_ch] << 2;
	}
	
	_txdelay = _txdelay * tx_baudrate[snd_ch] / 1000;

	switch (_diddles)
	{
	case 0:
   
		if (tx_delay_cnt[snd_ch] < _txdelay)
		{
			bit = TX_BIT0;
			tx_delay_cnt[snd_ch]++;
		}
		else
		{
			tx_status[snd_ch] = TX_FRAME;
			Debugprintf("End TXD %d", SampleNo);
		}

		break;	
	
	case 1:
    
		if (tx_delay_cnt[snd_ch] < _txdelay)
		{
			if (tx_last_diddle[snd_ch] == TX_BIT0)
				bit = TX_BIT1;
			else
				bit = TX_BIT0;
		
			tx_delay_cnt[snd_ch]++;
			tx_last_diddle[snd_ch] = bit;
		}
		else
		{
			tx_status[snd_ch] = TX_FRAME;
			Debugprintf("End TXD %d", SampleNo);
		}
		break;

	case 2:

		if (tx_delay_cnt[snd_ch] < _txdelay)
		{
			bit = FRAME_FLAG >> ((8 - (_txdelay % 8) + tx_delay_cnt[snd_ch]) % 8) & 1;
			tx_delay_cnt[snd_ch]++;
		}
		else
		{
			tx_status[snd_ch] = TX_FRAME;
			Debugprintf("End TXD %d", SampleNo);
		}
		break;
	}
	return bit;
}

// is this waiting for the filter to fill?
// No, flushing BPF

void get_wait_bpf(UCHAR snd_ch)
{
	tx_BPF_timer[snd_ch]++;

	if (tx_BPF_timer[snd_ch] == tx_BPF_tap[snd_ch] )
	{
		tx_status[snd_ch] = TX_NO_DATA;
		tx_BPF_timer[snd_ch] = 0;
	}
}


//procedure modulator(snd_ch: byte; var buf: array of single; buf_size: word};
//{
/*
function filter(x,k: single}: single;
begin
  result = k*cos(x};
  if result>1 then result = 1;
  if result<-1 then result = -1;
end;
}
*/

single filter(single x)
{
	if (x <= PI25)
		return 1.0f;

	if (x >= PI75)
		return  -1.0f;

     return cosf(2.0f * x -PI5);
}


// make_samples return one sample of the waveform

// But seems to be called only once per bit ??

// No, but needs to preserve bit between calls 

float make_samples(unsigned char  snd_ch, unsigned char * bitptr)
{
	float pi2, x1, x;
	byte i,qbit,tribit,dibit;
	float z1,z2,z3,z4;
	unsigned short b, msb, lsb;
	unsigned char bit = *bitptr;

	float amp = 0;

	pi2 = 2 * pi / TX_Samplerate;
	x1 = pi * tx_baudrate[snd_ch] / TX_Samplerate;

	if (modem_mode[snd_ch] == MODE_FSK)
	{
		if (bit == TX_BIT0)
			x = pi2*(tx_freq[snd_ch] + 0.5f * tx_shift[snd_ch]);
		else
			x = pi2*(tx_freq[snd_ch] - 0.5f * tx_shift[snd_ch]);

	   amp = 1.0f;

	    if (tx_baudrate[snd_ch] > 600)
		{
			if (tx_hitoneraisedb[snd_ch] < 0 && bit == TX_BIT0)
				amp = tx_hitoneraise[snd_ch];

			if (tx_hitoneraisedb[snd_ch] > 0 && bit == TX_BIT1)
				amp = tx_hitoneraise[snd_ch];
		}

		tx_osc[snd_ch] = tx_osc[snd_ch] + x;

		if (tx_osc[snd_ch] > 2*pi)
			tx_osc[snd_ch] = tx_osc[snd_ch] - 2*pi;
	}

	if (modem_mode[snd_ch] == MODE_BPSK)
	{
		if (tx_change_phase[snd_ch])
			tx_bit_mod[snd_ch] = tx_inv[snd_ch] * cos(tx_bit_osc[snd_ch]);

		x = pi2 * (tx_freq[snd_ch]);

		tx_osc[snd_ch] = tx_osc[snd_ch] + x;

		if (tx_osc[snd_ch] > 2 * pi)
			tx_osc[snd_ch] = tx_osc[snd_ch] - 2 * pi;
	}

/*
if modem_mode[snd_ch]=MODE_QPSK then
  begin
    if tx_QPSK_old_I[snd_ch]<>tx_QPSK_I[snd_ch] then
			tx_I_mod[snd_ch] = tx_QPSK_avg_I[snd_ch]+tx_QPSK_df_I[snd_ch]*filter(tx_bit_osc[snd_ch])
    else tx_I_mod[snd_ch] = tx_QPSK_I[snd_ch];
    if tx_QPSK_old_Q[snd_ch]<>tx_QPSK_Q[snd_ch] then
			tx_Q_mod[snd_ch] = tx_QPSK_avg_Q[snd_ch]+tx_QPSK_df_Q[snd_ch]*filter(tx_bit_osc[snd_ch])
    else tx_Q_mod[snd_ch] = tx_QPSK_Q[snd_ch];
    x = pi2*(tx_freq[snd_ch]);
    tx_osc[snd_ch] = tx_osc[snd_ch]+x;
    if tx_osc[snd_ch]>2*pi then tx_osc[snd_ch] = tx_osc[snd_ch]-2*pi;
  end;
  */

	if (modem_mode[snd_ch] == MODE_8PSK || modem_mode[snd_ch] == MODE_PI4QPSK)
	{
		if (tx_8PSK_old_I[snd_ch] != tx_8PSK_I[snd_ch])
			tx_I_mod[snd_ch] = tx_8PSK_avg_I[snd_ch] + tx_8PSK_df_I[snd_ch] * filter(tx_bit_osc[snd_ch]);
		else
			tx_I_mod[snd_ch] = tx_8PSK_I[snd_ch];

		if (tx_8PSK_old_Q[snd_ch] != tx_8PSK_Q[snd_ch])
			tx_Q_mod[snd_ch] = tx_8PSK_avg_Q[snd_ch] + tx_8PSK_df_Q[snd_ch] * filter(tx_bit_osc[snd_ch]);
		else
			tx_Q_mod[snd_ch] = tx_8PSK_Q[snd_ch];

		x = pi2 * (tx_freq[snd_ch]);
		tx_osc[snd_ch] = tx_osc[snd_ch] + x;

		if (tx_osc[snd_ch] > 2 * pi)
			tx_osc[snd_ch] = tx_osc[snd_ch] - 2 * pi;

	}

	if (modem_mode[snd_ch] == MODE_MPSK)
	{
		z1 = pi2 * (tx_freq[snd_ch] + ch_offset[0]);
		z2 = pi2 * (tx_freq[snd_ch] + ch_offset[1]);
		z3 = pi2 * (tx_freq[snd_ch] + ch_offset[2]);
		z4 = pi2 * (tx_freq[snd_ch] + ch_offset[3]);

		tx_osc1[snd_ch] = tx_osc1[snd_ch] + z1;
		tx_osc2[snd_ch] = tx_osc2[snd_ch] + z2;
		tx_osc3[snd_ch] = tx_osc3[snd_ch] + z3;
		tx_osc4[snd_ch] = tx_osc4[snd_ch] + z4;

		if (tx_osc1[snd_ch] > 2 * pi)
			tx_osc1[snd_ch] = tx_osc1[snd_ch] - 2 * pi;

		if (tx_osc2[snd_ch] > 2 * pi)
			tx_osc2[snd_ch] = tx_osc2[snd_ch] - 2 * pi;

		if (tx_osc3[snd_ch] > 2 * pi)
			tx_osc3[snd_ch] = tx_osc3[snd_ch] - 2 * pi;

		if (tx_osc4[snd_ch] > 2 * pi)
			tx_osc4[snd_ch] = tx_osc4[snd_ch] - 2 * pi;

		if (tx_old_inv1[snd_ch] != tx_inv1[snd_ch])
			tx_bit1_mod[snd_ch] = tx_inv1[snd_ch] * cos(tx_bit_osc[snd_ch]);
		else
			tx_bit1_mod[snd_ch] = -tx_inv1[snd_ch];

		if (tx_old_inv2[snd_ch] != tx_inv2[snd_ch])
			tx_bit2_mod[snd_ch] = tx_inv2[snd_ch] * cos(tx_bit_osc[snd_ch]);
		else 
			tx_bit2_mod[snd_ch] = -tx_inv2[snd_ch];

		if (tx_old_inv3[snd_ch] != tx_inv3[snd_ch])
			tx_bit3_mod[snd_ch] = tx_inv3[snd_ch] * cos(tx_bit_osc[snd_ch]);
		else 
			tx_bit3_mod[snd_ch] = -tx_inv3[snd_ch];

		if (tx_old_inv4[snd_ch] != tx_inv4[snd_ch]) 
			tx_bit4_mod[snd_ch] = tx_inv4[snd_ch] * cos(tx_bit_osc[snd_ch]);
		else
			tx_bit4_mod[snd_ch] = -tx_inv4[snd_ch];
	}

	tx_bit_osc[snd_ch] = tx_bit_osc[snd_ch] + x1;

	if (tx_bit_osc[snd_ch] > pi)
	{
		// This seems to get the next bit,
		// but why??

		tx_bit_osc[snd_ch] = tx_bit_osc[snd_ch] - pi;

		// FSK Mode
		if (modem_mode[snd_ch] == MODE_FSK)
		{
			bit = 0;

			if (tx_status[snd_ch] == TX_SILENCE)
			{
				tx_delay_cnt[snd_ch] = 0;
				Debugprintf("Start TXD");
				tx_status[snd_ch] = TX_DELAY;
			}

			if (tx_status[snd_ch] == TX_DELAY)
				bit = get_new_bit_delay(snd_ch, bit);

			if (tx_status[snd_ch] == TX_TAIL)
				bit = get_new_bit_tail(snd_ch, bit);

			if (tx_status[snd_ch] == TX_FRAME)
				bit = get_new_bit(snd_ch, bit);

			*bitptr = tx_nrzi(snd_ch, bit);
			//			Debugprintf("Bit %d NRZI %d", bit, *bitptr);
		}

		// BPSK Mode
		if (modem_mode[snd_ch] == MODE_BPSK)
		{
			bit = 0;

			if (tx_status[snd_ch] == TX_SILENCE)
			{
				tx_delay_cnt[snd_ch] = 0;
				Debugprintf("Start TXD");
				tx_status[snd_ch] = TX_DELAY;
			}

			if (tx_status[snd_ch] == TX_DELAY)
				bit = get_new_bit_delay(snd_ch, bit);

			if (tx_status[snd_ch] == TX_TAIL)
				bit = get_new_bit_tail(snd_ch, bit);

			if (tx_status[snd_ch] == TX_FRAME)
				bit = get_new_bit(snd_ch, bit);

			// ??			*bitptr = tx_nrzi(snd_ch, bit);

			if (bit == 0)
			{
				tx_inv[snd_ch] = -tx_inv[snd_ch];
				tx_change_phase[snd_ch] = TRUE;
			}
			else
				tx_change_phase[snd_ch] = FALSE;
		}
		/*

					// QPSK Mode
					if (modem_mode[snd_ch] == MODE_QPSK)
					{
							dibit = 0;
							for i = 0 to 1 do
							begin
							  bit = 0;
							  if tx_status[snd_ch]=TX_SILENCE then begin tx_delay_cnt[snd_ch] = 0; tx_status[snd_ch] = TX_DELAY; end;
							  if tx_status[snd_ch]=TX_DELAY then get_new_bit_delay(snd_ch,bit);
							  if tx_status[snd_ch]=TX_TAIL then get_new_bit_tail(snd_ch,bit);
							  if tx_status[snd_ch]=TX_FRAME then get_new_bit(snd_ch,bit);
							  dibit = (dibit shl 1) or tx_nrzi(snd_ch,bit);
							end;
							dibit = QPSK_set[snd_ch].tx[dibit and 3];
							tx_QPSK[snd_ch] = (tx_QPSK[snd_ch]+dibit) mod 4;
							tx_QPSK_old_I[snd_ch] = tx_QPSK_I[snd_ch];
							tx_QPSK_old_Q[snd_ch] = tx_QPSK_Q[snd_ch];
							case tx_QPSK[snd_ch] of
							  0 : begin tx_QPSK_I[snd_ch] =  COS45; tx_QPSK_Q[snd_ch] =  COS45; end;
							  1 : begin tx_QPSK_I[snd_ch] = -COS45; tx_QPSK_Q[snd_ch] =  COS45; end;
							  2 : begin tx_QPSK_I[snd_ch] = -COS45; tx_QPSK_Q[snd_ch] = -COS45; end;
							  3 : begin tx_QPSK_I[snd_ch] =  COS45; tx_QPSK_Q[snd_ch] = -COS45; end;
							end;
							tx_QPSK_avg_I[snd_ch] = 0.5*(tx_QPSK_old_I[snd_ch]+tx_QPSK_I[snd_ch]);
							tx_QPSK_df_I[snd_ch] = 0.5*(tx_QPSK_old_I[snd_ch]-tx_QPSK_I[snd_ch]);
							tx_QPSK_avg_Q[snd_ch] = 0.5*(tx_QPSK_old_Q[snd_ch]+tx_QPSK_Q[snd_ch]);
							tx_QPSK_df_Q[snd_ch] = 0.5*(tx_QPSK_old_Q[snd_ch]-tx_QPSK_Q[snd_ch]);
					end;
					*/

					// PI/4 QPSK Mode

		if (modem_mode[snd_ch] == MODE_PI4QPSK)
		{
			dibit = 0;

			for (i = 0; i < 2; i++)
			{
				bit = 0;
				if (tx_status[snd_ch] == TX_SILENCE)
				{
					tx_delay_cnt[snd_ch] = 0;
					Debugprintf("Start TXD");
					tx_status[snd_ch] = TX_DELAY;
				}

				if (tx_status[snd_ch] == TX_DELAY)
					bit = get_new_bit_delay(snd_ch, bit);

				if (tx_status[snd_ch] == TX_TAIL)
					bit = get_new_bit_tail(snd_ch, bit);

				if (tx_status[snd_ch] == TX_FRAME)
					bit = get_new_bit(snd_ch, bit);

				*bitptr = tx_nrzi(snd_ch, bit);

				dibit = (dibit << 1) | *bitptr;

			}

			// This returns 3,1,5 or 7 so we use the odd enties in the 8PSK table

			dibit = gray_PI4QPSK[dibit & 3];

			tx_8PSK[snd_ch] = tx_8PSK[snd_ch] + (dibit & 7);
			tx_8PSK_old_I[snd_ch] = tx_8PSK_I[snd_ch];
			tx_8PSK_old_Q[snd_ch] = tx_8PSK_Q[snd_ch];

			switch (tx_8PSK[snd_ch])
			{
			case 0:
				tx_8PSK_I[snd_ch] = 0;
				tx_8PSK_Q[snd_ch] = 1;
				break;

			case 1:
				tx_8PSK_I[snd_ch] = COS45;
				tx_8PSK_Q[snd_ch] = COS45;
				break;

			case 2:
				tx_8PSK_I[snd_ch] = 1;
				tx_8PSK_Q[snd_ch] = 0;
				break;

			case 3:
				tx_8PSK_I[snd_ch] = COS45;
				tx_8PSK_Q[snd_ch] = -COS45;
				break;

			case 4:
				tx_8PSK_I[snd_ch] = 0;
				tx_8PSK_Q[snd_ch] = -1;
				break;

			case 5:
				tx_8PSK_I[snd_ch] = -COS45;
				tx_8PSK_Q[snd_ch] = -COS45;
				break;

			case 6:
				tx_8PSK_I[snd_ch] = -1;
				tx_8PSK_Q[snd_ch] = 0;
				break;

			case 7:
				tx_8PSK_I[snd_ch] = -COS45;
				tx_8PSK_Q[snd_ch] = COS45;
				break;

			}

			tx_8PSK_avg_I[snd_ch] = 0.5*(tx_8PSK_old_I[snd_ch] + tx_8PSK_I[snd_ch]);
			tx_8PSK_df_I[snd_ch] = 0.5*(tx_8PSK_old_I[snd_ch] - tx_8PSK_I[snd_ch]);
			tx_8PSK_avg_Q[snd_ch] = 0.5*(tx_8PSK_old_Q[snd_ch] + tx_8PSK_Q[snd_ch]);
			tx_8PSK_df_Q[snd_ch] = 0.5*(tx_8PSK_old_Q[snd_ch] - tx_8PSK_Q[snd_ch]);

		}
		/*

					// 8PSK Mode
					if modem_mode[snd_ch]=MODE_8PSK then
					begin
							tribit = 0;
							for i = 0 to 2 do
							begin
							  bit = 0;
							  if tx_status[snd_ch]=TX_SILENCE then begin tx_delay_cnt[snd_ch] = 0; tx_status[snd_ch] = TX_DELAY; end;
							  if tx_status[snd_ch]=TX_DELAY then get_new_bit_delay(snd_ch,bit);
							  if tx_status[snd_ch]=TX_TAIL then get_new_bit_tail(snd_ch,bit);
							  if tx_status[snd_ch]=TX_FRAME then get_new_bit(snd_ch,bit);
							  tribit = (tribit shl 1) or tx_nrzi(snd_ch,bit);
							end;
							tribit = gray_8PSK[tribit and 7];
							tx_8PSK[snd_ch] = (tx_8PSK[snd_ch]+tribit) mod 8;
							tx_8PSK_old_I[snd_ch] = tx_8PSK_I[snd_ch];
							tx_8PSK_old_Q[snd_ch] = tx_8PSK_Q[snd_ch];
							case tx_8PSK[snd_ch] of
							  0 : begin tx_8PSK_I[snd_ch] =  0;     tx_8PSK_Q[snd_ch] =  1;     end;
							  1 : begin tx_8PSK_I[snd_ch] =  COS45; tx_8PSK_Q[snd_ch] =  COS45; end;
							  2 : begin tx_8PSK_I[snd_ch] =  1;     tx_8PSK_Q[snd_ch] =  0;     end;
							  3 : begin tx_8PSK_I[snd_ch] =  COS45; tx_8PSK_Q[snd_ch] = -COS45; end;
							  4 : begin tx_8PSK_I[snd_ch] =  0;     tx_8PSK_Q[snd_ch] = -1;     end;
							  5 : begin tx_8PSK_I[snd_ch] = -COS45; tx_8PSK_Q[snd_ch] = -COS45  end;
							  6 : begin tx_8PSK_I[snd_ch] = -1;     tx_8PSK_Q[snd_ch] =  0;     end;
							  7 : begin tx_8PSK_I[snd_ch] = -COS45; tx_8PSK_Q[snd_ch] =  COS45; end;
							end;
							tx_8PSK_avg_I[snd_ch] = 0.5*(tx_8PSK_old_I[snd_ch]+tx_8PSK_I[snd_ch]);
							tx_8PSK_df_I[snd_ch] = 0.5*(tx_8PSK_old_I[snd_ch]-tx_8PSK_I[snd_ch]);
							tx_8PSK_avg_Q[snd_ch] = 0.5*(tx_8PSK_old_Q[snd_ch]+tx_8PSK_Q[snd_ch]);
							tx_8PSK_df_Q[snd_ch] = 0.5*(tx_8PSK_old_Q[snd_ch]-tx_8PSK_Q[snd_ch]);
					end;
					*/

		if (modem_mode[snd_ch] == MODE_MPSK)
		{
			qbit = 0;

			// get the bits for each of 4 carriers

			for (i = 1; i <= 4; i++)
			{
				bit = 0;

				if (tx_status[snd_ch] == TX_SILENCE)
				{
					tx_delay_cnt[snd_ch] = 0;
					Debugprintf("Start TXD");
					tx_status[snd_ch] = TX_DELAY;
				}

				if (tx_status[snd_ch] == TX_DELAY)
					bit = get_new_bit_delay(snd_ch, bit);

				if (tx_status[snd_ch] == TX_TAIL)
					bit = get_new_bit_tail(snd_ch, bit);

				if (tx_status[snd_ch] == TX_FRAME)
					bit = get_new_bit(snd_ch, bit);

				qbit = (qbit << 1) | bit;
			}

			tx_old_inv1[snd_ch] = tx_inv1[snd_ch];
			tx_old_inv2[snd_ch] = tx_inv2[snd_ch];
			tx_old_inv3[snd_ch] = tx_inv3[snd_ch];
			tx_old_inv4[snd_ch] = tx_inv4[snd_ch];

			if ((qbit & 8) == 0)
				tx_inv1[snd_ch] = -tx_inv1[snd_ch];
			if ((qbit & 4) == 0)
				tx_inv2[snd_ch] = -tx_inv2[snd_ch];
			if ((qbit & 2) == 0)
				tx_inv3[snd_ch] = -tx_inv3[snd_ch];
			if ((qbit & 1) == 0)
				tx_inv4[snd_ch] = -tx_inv4[snd_ch];

		}
	}



	if (tx_status[snd_ch] == TX_WAIT_BPF)
		get_wait_bpf(snd_ch);

	if (modem_mode[snd_ch] == MODE_FSK)
		return amp * sinf(tx_osc[snd_ch]);

	if (modem_mode[snd_ch] == MODE_BPSK)
		return sinf(tx_osc[snd_ch]) * tx_bit_mod[snd_ch];

	if (modem_mode[snd_ch] == MODE_QPSK || modem_mode[snd_ch] == MODE_8PSK || modem_mode[snd_ch] == MODE_PI4QPSK)
		return sin(tx_osc[snd_ch]) * tx_I_mod[snd_ch] + cos(tx_osc[snd_ch]) * tx_Q_mod[snd_ch];
 
	if (modem_mode[snd_ch] == MODE_MPSK)
		 return 0.35*(sin(tx_osc1[snd_ch])*tx_bit1_mod[snd_ch]+sin(tx_osc2[snd_ch])*tx_bit2_mod[snd_ch]+sin(tx_osc3[snd_ch])*tx_bit3_mod[snd_ch]+sin(tx_osc4[snd_ch])*tx_bit4_mod[snd_ch]);

	return 0.0f;
}

float make_samples_calib(UCHAR snd_ch, UCHAR tones)
{
	float amp, pi2, x, x1;

	x1 = pi * tx_baudrate[snd_ch] / TX_Samplerate;
	pi2 = 2 * pi / TX_Samplerate;

	switch(tones)
	{
	case 1:
				
		tx_last_bit[snd_ch] = 1;
		break;
	
	case 2:

		tx_last_bit[snd_ch] = 0;
		break;

	case 3:
			
		tx_bit_osc[snd_ch] = tx_bit_osc[snd_ch]+x1;
			
		if (tx_bit_osc[snd_ch] > pi)
		{
			  tx_bit_osc[snd_ch] = tx_bit_osc[snd_ch]-pi;
			  tx_last_bit[snd_ch] = tx_last_bit[snd_ch] ^ 1;
		}
		break;
	}

	amp = 1;

	if (tx_baudrate[snd_ch] > 600)
	{
		if (tx_hitoneraisedb[snd_ch] < 0 && tx_last_bit[snd_ch] == 0)
			amp = tx_hitoneraise[snd_ch];

		if (tx_hitoneraisedb[snd_ch] > 0 && tx_last_bit[snd_ch] == 1)
			amp = tx_hitoneraise[snd_ch];
	}
	
	if (tx_last_bit[snd_ch] == 0)
		x = pi2*(tx_freq[snd_ch] + 0.5f * tx_shift[snd_ch]);
	else
		x = pi2*(tx_freq[snd_ch] - 0.5f * tx_shift[snd_ch]);

	tx_osc[snd_ch] = tx_osc[snd_ch] + x;
	
	if (tx_osc[snd_ch] > 2*pi)
		tx_osc[snd_ch] = tx_osc[snd_ch] - 2 * pi;

	return amp * sinf(tx_osc[snd_ch]);
}

int amplitude = 22000;

void modulator(UCHAR snd_ch, int buf_size)
{
	// We feed samples to samplesink instead of buffering them

	// I think this is the top of the TX hierarchy

	int i;
	short Sample;

	if (calib_mode[snd_ch] > 0)
	{
		if (tx_status[snd_ch] == TX_SILENCE)
		{
			SoundIsPlaying = TRUE;
			Debugprintf("Start Calib Chan %d", snd_ch);
			RadioPTT(snd_ch, 1);

			tx_bit_osc[snd_ch] = 0;
			tx_last_bit[snd_ch] = 0;
	
			// fill filter 

			for (i = 0; i < tx_BPF_tap[snd_ch]; i++)
				tx_prev_BPF_buf[snd_ch][buf_size + i] = make_samples_calib(snd_ch,calib_mode[snd_ch]);
		}
		tx_status[snd_ch] = TX_WAIT_BPF;
	
		for (i = 0; i < buf_size; i++)
			tx_src_BPF_buf[snd_ch][i] = make_samples_calib(snd_ch, calib_mode[snd_ch]);

		FIR_filter(tx_src_BPF_buf[snd_ch],buf_size,tx_BPF_tap[snd_ch],tx_BPF_core[snd_ch],tx_BPF_buf[snd_ch],tx_prev_BPF_buf[snd_ch]);
    
		for (i = 0; i < buf_size; i++)
		{
			Sample = tx_BPF_buf[snd_ch][i] * amplitude;
			SampleSink(snd_ch, Sample);
		}
	}
	else
	{
		if (tx_status[snd_ch] == TX_SILENCE)
		{
			Debugprintf("New Packet %d", SampleNo);
			tx_bit_osc[snd_ch] = 0;
			tx_8PSK[snd_ch] = 0;
			tx_QPSK[snd_ch] = 0;
			tx_last_bit[snd_ch] = 0;
			tx_inv1[snd_ch] = 1;
			tx_inv2[snd_ch] = 1;
			tx_inv3[snd_ch] = 1;
			tx_inv4[snd_ch] = 1;
			tx_8PSK_I[snd_ch] =  0;
			tx_8PSK_Q[snd_ch] =  1;
			tx_8PSK_old_I[snd_ch] =  0;
			tx_8PSK_old_Q[snd_ch] =  1;
			tx_QPSK_I[snd_ch] =  COS45;
			tx_QPSK_Q[snd_ch] =  COS45;
			tx_QPSK_old_I[snd_ch] =  COS45;
			tx_QPSK_old_Q[snd_ch] =  COS45;

			for (i = 0; i < tx_BPF_tap[snd_ch]; i++)
				tx_prev_BPF_buf[snd_ch][buf_size+i] = make_samples(snd_ch, &tx_pol[snd_ch]);
		}
		
		for (i = 0; i < buf_size; i++)
			tx_src_BPF_buf[snd_ch][i] = make_samples(snd_ch, &tx_pol[snd_ch]);
		
		FIR_filter(tx_src_BPF_buf[snd_ch], buf_size, tx_BPF_tap[snd_ch], tx_BPF_core[snd_ch], tx_BPF_buf[snd_ch], tx_prev_BPF_buf[snd_ch]);

		for (i = 0; i < buf_size; i++)
		{
			Sample = tx_BPF_buf[snd_ch][i] * 20000.0f;
			SampleSink(modemtoSoundLR[snd_ch], Sample);
		}
	}
}


