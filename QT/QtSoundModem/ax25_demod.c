// UZ7HO Soundmodem Port

#include "UZ7HOStuff.h"



/*

unit ax25_demod;

interface

uses math,sysutils,Graphics,classes;

  procedure detector_init;
  procedure detector_free;
  procedure Mux3(snd_ch,rcvr_nr,emph: byte; src1,core: array of single; var dest,prevI,prevQ: array of single; tap,buf_size: word);
  procedure Mux3_PSK(snd_ch,rcvr_nr,emph: byte; src1,core: array of single; var destI,destQ,prevI,prevQ: array of single; tap,buf_size: word);
  procedure make_core_intr(snd_ch: byte);
  procedure make_core_LPF(snd_ch: byte; width: single);
  procedure make_core_BPF(snd_ch: byte; freq,width: single);
  procedure make_core_TXBPF(snd_ch: byte; freq,width: single);
  procedure init_BPF(freq1,freq2: single; tap: word; samplerate: single; var buf: array of single);
  procedure FIR_filter(src: array of single; buf_size,tap: word; core: array of single; var dest,prev: array of single);
  procedure Demodulator(snd_ch,rcvr_nr: byte; src_buf: array of single; last: boolean);
  function memory_ARQ(buf: TStringList; data: string): string;

type TSurvivor = record
    BitEstimates: int64;
    Pathdistance: integer;
}

type TMChannel = record
  prev_LPF1I_buf : array [0..4095] of single;
  prev_LPF1Q_buf : array [0..4095] of single;
  prev_dLPFI_buf : array [0..4095] of single;
  prev_dLPFQ_buf : array [0..4095] of single;
  prev_AFCI_buf  : array [0..4095] of single;
  prev_AFCQ_buf  : array [0..4095] of single;
  AngleCorr      : single;
  MUX_osc        : single;
  AFC_IZ1        : single;
  AFC_IZ2        : single;
  AFC_QZ1        : single;
  AFC_QZ2        : single;
  AFC_bit_buf1I  : array [0..1023] of single;
  AFC_bit_buf1Q  : array [0..1023] of single;
  AFC_bit_buf2   : array [0..1023] of single;
  AFC_IIZ1       : single;
  AFC_QQZ1       : single;
}
*/
 
 
#define sbc 175
  
single  ch_offset[4] = { -sbc * 1.5,-sbc * 0.5,sbc*0.5,sbc*1.5 };



float PI125 = 0.125f * M_PI;
float PI375 = 0.375f * M_PI;
float PI625 = 0.625f * M_PI;
float PI875 = 0.875f * M_PI;
float PI5 = 0.5f * M_PI;
float PI25 = 0.25f * M_PI;
float PI75 = 0.75f * M_PI;

byte emph_decoded[nr_emph];

unsigned char  modem_mode[5] ={0,0,0,0};

unsigned short bpf[5] = { 500, 500, 500, 500,500 };
unsigned short lpf[5] = { 150, 150, 150, 150, 150 };




float BIT_AFC = 32;
float slottime_tick[5] = { 0 };
float resptime_tick[5] = { 0 };
int dcd_threshold = 128;
float DCD_LastPkPos[5] = { 0 };
float DCD_LastPerc[5] = { 0 };
int dcd_bit_cnt[5] = { 0 };
byte DCD_status[5] = { 0 };
float DCD_persist[5] = { 0 };
int dcd_bit_sync[5] = { 0 };
byte dcd_hdr_cnt[5] = { 0 };
longword DCD_header[5] = { 0 };
int dcd_on_hdr[5] = { 0 };
 

unsigned short n_INTR[5] = { 1,1,1,1,1 };
unsigned short INTR_tap[5] = { 16, 16,16,16,16 };
unsigned short BPF_tap[5] = { 256, 256,256,256,256 };   // 256 default
unsigned short LPF_tap[5] = { 128, 128,128,128,128 };   // 128
  


short rx_freq[5] = { 1700, 1700,1700,1700,1700 };
short rx_shift[5] = { 200, 200, 200, 200, 200 };  
short rx_baudrate[5] = { 300, 300, 300, 300, 300 };
short rcvr_offset[5] = { 30, 30, 30, 30,30 };  



int pnt_change[5] = { 0 };
float src_buf[5][2048];

float INTR_core[5][2048];
float AFC_core[5][2048];
float LPF_core[5][2048];

int new_tx_port[4] = { 0,0,0,0 };
UCHAR RCVR[5] = { 0 };

// We allow two (or more!) ports to be assigned to the same soundcard channel

int soundChannel[5] = { 0 };		// 0 = Unused 1 = Left 2 = Right 3 = Mono
int modemtoSoundLR[4] = { 0 };

struct TDetector_t  DET[nr_emph + 1][16];

TStringList detect_list_l[5];
TStringList detect_list[5];
TStringList detect_list_c[5];

/*

implementation

uses sm_main,ax25,ax25_l2,ax25_mod,ax25_agw,rsunit,kiss_mode;
*/

void detector_init()
{
	int i, k, j;

	for (k = 0; k < 16; k++)
	{
		for (i = 1; i <= 4; i++)
		{
			for (j = 0; j <= nr_emph; j++)
			{
				struct TDetector_t * pDET = &DET[j][k];

				pDET->AngleCorr[i] = 0;
				pDET->last_sample[i] = 0;
				pDET->sample_cnt[i] = 0;
				pDET->last_bit[i] = 0;
				pDET->PkAmp[i] = 0;
				pDET->PkAmpMax[i] = 0;
				pDET->newpkpos[i] = 0;
				pDET->ones[i] = 0;
				pDET->zeros[i] = 0;
				pDET->MinAmp[i] = 0;
				pDET->MaxAmp[i] = 0;
				pDET->MUX3_osc[i] = 0;
				pDET->Preemphasis6[i] = 0;
				pDET->Preemphasis12[i] = 0;
				pDET->PSK_AGC[i] = 0;
				pDET->AGC[i] = 0;
				pDET->AGC1[i] = 0;
				pDET->AGC2[i] = 0;
				pDET->AGC3[i] = 0;
				pDET->AGC_max[i] = 0;
				pDET->AGC_min[i] = 0;
				pDET->AFC_IZ1[i] = 0;
				pDET->AFC_IZ2[i] = 0;
				pDET->AFC_QZ1[i] = 0;
				pDET->AFC_QZ2[i] = 0;
				pDET->AFC_dF[i] = 0;
				pDET->AFC_cnt[i] = 0;
				pDET->PSK_IZ1[i] = 0;
				pDET->PSK_QZ1[i] = 0;
				pDET->PkAmpI[i] = 0;
				pDET->PkAmpQ[i] = 0;
				pDET->last_rx_bit[i] = 0;
				pDET->bit_stream[i] = 0;
				pDET->byte_rx[i] = 0;
				pDET->bit_stuff_cnt[i] = 0;
				pDET->bit_cnt[i] = 0;
				pDET->bit_osc[i] = 0;
				pDET->frame_status[i] = 0;
				initString(&pDET->FEC_rx_data[i]);
				initString(&pDET->rx_data[i]);
				initString(&pDET->raw_bits[i]);
				initTStringList(&pDET->mem_ARQ_buf[i]);
				initTStringList(&pDET->mem_ARQ_F_buf[i]);
			}
		}
	}
	
	for (i = 1; i <= 4; i++)
	{
		initTStringList(&detect_list[i]);
		initTStringList(&detect_list_l[i]);
		initTStringList(&detect_list_c[i]);
  }
}


/*
procedure detector_free;
var
  i,k,j: word;
{
  for i = 1 to 4 do
  {
    detect_list[i].Free;
    detect_list_l[i].Free;
    detect_list_c[i].Free;
  }
  for k = 0 to 16 do
    for i = 1 to 4 do
      for j = 0 to nr_emph do
      {
        DET[j,k].mem_ARQ_buf[i].Free;
        DET[j,k].mem_ARQ_F_buf[i].Free;
      }
}
*/

void FIR_filter(float * src, unsigned short buf_size, unsigned short tap, float * core, float * dest, float * prev)
{
	float accum = 0.0f;
	float fp1;

	int eax, ebx;
	float * edi;

	fmove(&prev[buf_size], &prev[0], tap * 4);
	fmove(&src[0], &prev[tap], buf_size * 4);

	eax = 0;

	//  ; shl ecx, 2;
	//  ; shl edx, 2;

cfir_i:
	edi = prev;

	edi += eax;

	ebx = 0;
	accum = 0.0f;

cfir_k:

	// FLD pushes operand onto stack, so old value goes to fp1

	fp1 = accum;
	accum = edi[ebx];
	accum *= core[ebx];
	accum += fp1;

	ebx++;
	if (ebx != tap)
		goto cfir_k;

	dest[eax] = accum;

	eax++;

	if (eax != buf_size)
		goto cfir_i;

}
/*

  __asm
  {
    push eax;
    push ebx;
    push ecx;
    push edx;
    push edi;
    push esi;
    xor eax,eax;
    movzx ecx,tap;
    movzx edx,buf_size;
    shl ecx,2;
    shl edx,2;
    mov esi,core;
    fir_i:
      mov edi,prev;
      add edi,eax;
      xor ebx,ebx;
      fldz;
      fir_k:
        fld dword ptr [edi+ebx];
        fmul dword ptr [esi+ebx];
        fadd;
        add ebx,4;
        cmp ebx,ecx;
      jne fir_k;
      mov edi,dest;
      fstp dword ptr [edi+eax];
      wait;
      add eax,4;
      cmp eax,edx;
    jne fir_i;
    pop esi;
    pop edi;
    pop edx;
    pop ecx;
    pop ebx;
    pop eax;
  }
}
*/

/*
function sgn(p1: single): integer;
{
  result = 0;
  if p1>0 then result = 1;
  if p1<0 then result = -1;
}

function pila(x: single): single;
{
  x = frac(x);
  if x>0.5 then x = 1-x;
  result = 2*x;
}

*/
float get_persist(int snd_ch, int persist)
{
	single x, x1 ;

	x = 256 / persist;

	x1 = round(x*x) * rand() / RAND_MAX;

	return x1 * 0.5 * slottime[snd_ch];
}

void chk_dcd1(int snd_ch, int buf_size)
{
	// This seems to schedule all TX, but is only called when a frame has been processed
	//  ? does this work as Andy passes aborted frames to decoder

	byte port;
	word  i;
	single  tick;
	word  active;
	boolean  ind_dcd;
	boolean  dcd_sync;
	longint  n;

	TAX25Port * AX25Sess;

	dcd[snd_ch] = 1;

	ind_dcd = 0;

	tick = 1000 / RX_Samplerate;

	if (dcd_bit_cnt[snd_ch] > 0)
		dcd_bit_sync[snd_ch] = 0;
	else
		dcd_bit_sync[snd_ch] = 1;

	if (dcd_on_hdr[snd_ch])
		dcd_bit_sync[snd_ch] = 1;

	if (modem_mode[snd_ch] == MODE_MPSK && DET[0][0].frame_status[snd_ch] == FRAME_LOAD)
		dcd_bit_sync[snd_ch] = 1;

	if (dcd_bit_sync[snd_ch])
		updateDCD(snd_ch, 1);
	else
		updateDCD(snd_ch, 0);


	if (resptime_tick[snd_ch] < resptime[snd_ch])
		resptime_tick[snd_ch] = resptime_tick[snd_ch] + tick * buf_size;

	slottime_tick[snd_ch] = slottime_tick[snd_ch] + tick * buf_size;

	if (dcd_bit_sync[snd_ch]) // reset the slottime timer
	{
		slottime_tick[snd_ch] = 0;
		DCD_status[snd_ch] = DCD_WAIT_SLOT;
	}

	switch (DCD_status[snd_ch])
	{
	case DCD_WAIT_SLOT:

		if (slottime_tick[snd_ch] >= slottime[snd_ch])
		{
			DCD_status[snd_ch] = DCD_WAIT_PERSIST;
			DCD_persist[snd_ch] = get_persist(snd_ch, persist[snd_ch]);
		}
		break;

	case DCD_WAIT_PERSIST:

		if (slottime_tick[snd_ch] >= slottime[snd_ch] + DCD_persist[snd_ch])
		{
			dcd[snd_ch] = FALSE;
			slottime_tick[snd_ch] = 0;
			DCD_status[snd_ch] = DCD_WAIT_SLOT;
		}
		break;
	}

	active = 0;

	for (i = 0; i < port_num; i++)
	{
		if (AX25Port[snd_ch][i].status != STAT_NO_LINK)
			active++;

		if (active < 2)
			resptime_tick[snd_ch] = resptime[snd_ch];

		if (TX_rotate)
		{
			for (i = 0; i < 4; i++)
			{
				if (snd_status[i] == SND_TX)
					dcd[snd_ch] = TRUE;
			}
		}

		if (snd_ch == 1)
			snd_ch = 1;

		if (!dcd[snd_ch] && resptime_tick[snd_ch] >= resptime[snd_ch])
		{
			i = 0;

			port = new_tx_port[snd_ch];
			do
			{
				AX25Sess = &AX25Port[snd_ch][port];

				if (AX25Sess->frame_buf.Count > 0)
					Frame_Optimize(AX25Sess, &AX25Sess->frame_buf);

				if (AX25Sess->frame_buf.Count > 0)
				{
					for (n = 0; n < AX25Sess->frame_buf.Count; n++)
					{
						Add(&all_frame_buf[modemtoSoundLR[snd_ch]], duplicateString(Strings(&AX25Sess->frame_buf, n)));
					}

					Clear(&AX25Sess->frame_buf);
				}

				port++;

				if (port >= port_num)
					port = 0;

				if (all_frame_buf[snd_ch].Count > 0)
					new_tx_port[snd_ch] = port;

				i++;

			} while (all_frame_buf[snd_ch].Count == 0 && i < port_num);

			// Add KISS frames

			if (KISSServ)
			{
				// KISS monitor outgoing frames

				if (all_frame_buf[snd_ch].Count > 0)
				{
					for (n = 0; n < all_frame_buf[snd_ch].Count; n++)
					{
						KISS_on_data_out(snd_ch, Strings(&all_frame_buf[modemtoSoundLR[snd_ch]], n), 1);	// Mon TX
					}

					// AGW monitor

					if (KISS.buffer[snd_ch].Count > 0)
					{
						for (n = 0; n < KISS.buffer[snd_ch].Count; n++)
						{
							if (AGWServ)
								AGW_Raw_monitor(snd_ch, Strings(&KISS.buffer[snd_ch], n));

							Add(&all_frame_buf[modemtoSoundLR[snd_ch]], Strings(&KISS.buffer[snd_ch], n));
						}

						Clear(&KISS.buffer[snd_ch]);

					}

					//     if KISS.request[snd_ch].Count>0 then
					 //    {
					 //      for n = 0 to KISS.request[snd_ch].Count-1 do
					 //        KISS.acked[snd_ch].add(KISS.request[snd_ch].Strings[n]);
					 //      KISS.request[snd_ch].Clear;
				}
			}

			//

			if (all_frame_buf[snd_ch].Count > 0 && snd_status[snd_ch] == SND_IDLE)
			{
				resptime_tick[snd_ch] = 0;
				RX2TX(snd_ch);					// Do TX
				return;
			}
		}
	}
}


string * get_pkt_data(string * stream)
{
	byte  bitstuff_cnt;
	byte  bits_cnt;
	word  i;
	string * s = newString();

	byte  bit;
	byte  raw_bit;
	byte  sym;

	bits_cnt = 0;
	bitstuff_cnt = 0;
	sym = 0;

	if (stream->Length > 0)
	{
		for (i = 0; i < stream->Length; i++)
		{
			if (stream->Data[i] == '1')
				bit = RX_BIT1;
			else
				bit = RX_BIT0;

			if (bitstuff_cnt < 5)
			{
				sym = (sym >> 1) | bit;
				bits_cnt++;
			}

			if (bitstuff_cnt == 5 || bit == RX_BIT0)
				bitstuff_cnt = 0;

			if (bit == RX_BIT1)
				bitstuff_cnt++;

			if (bits_cnt == 8)
			{
				stringAdd(s, &sym, 1);
				sym = 0;
				bits_cnt = 0;
			}
		}
	}

	return s;
}

/*

function get_pkt_data2(var stream: string; last_nrzi_bit: byte): string;
var
  bitstuff_cnt: byte;
  bits_cnt: byte;
  i: word;
  s: string;
  bit: byte;
  raw_bit: byte;
  sym: byte;
  n: byte;
{
  // limit of AX.25 packet
  setlength(s,330);
  n = 0;
  bits_cnt = 0;
  bitstuff_cnt = 0;
  sym = 0;
  if length(stream)>0 then
    for i = 1 to length(stream) do
    {
      if stream[i]='1' then raw_bit = RX_BIT1 else raw_bit = RX_BIT0;
      if raw_bit=last_nrzi_bit then bit = RX_BIT1 else bit = RX_BIT0;
      last_nrzi_bit = raw_bit;
      if bitstuff_cnt<5 then
      {
        sym = (sym shr 1) or bit;
        inc(bits_cnt);
      }
      if (bitstuff_cnt=5) or (bit=RX_BIT0) then bitstuff_cnt = 0;
      if bit=RX_BIT1 then inc(bitstuff_cnt);
      if bits_cnt=8 then
      {
        if n<330 then
        {
          inc(n);
          s[n] = chr(sym);
        }
        sym = 0;
        bits_cnt = 0;
      }
    }
  if n>0 then setlength(s,n) else s = '';
  if bits_cnt>0 then s = '';
  result = s;
}
*/

string * get_NRZI_data(string * stream, UCHAR * last_nrzi_bit)
{
	longword len;
	word i;
	string * s = NULL;
	byte raw_bit;

	len = stream->Length;

	if (len > 65535)
		len = 65535;

	if (len > 0)
	{
		s = newString(); 
		
		setlength(s, len);

		for (i = 0; i < len; i++)
		{
			if (stream->Data[i] == '1')
				raw_bit = RX_BIT1;
			else
				raw_bit = RX_BIT0;

			if (raw_bit == *last_nrzi_bit)
				s->Data[i] = '1';
			else
				s->Data[i] = '0';

			*last_nrzi_bit = raw_bit;
		}
	}
	return s;
}
/*

function invert_NRZI_data(stream: string; last_nrzi_bit: byte): string;
var
  len: longword;
  i: word;
  s: string;
{
  s = '';
  len = length(stream);
  if len>65535 then len = 65535;
  if len>0 then
  {
    setlength(s,len);
    for i = 1 to len do
      if last_nrzi_bit=RX_BIT0 then
      {
        if stream[i]='1' then s[i] = '0' else s[i] = '1';
      end
      else s[i] = stream[i];
  }
  result = s;
}

function memory_ARQ(buf: TStringList; data: string): string;
var
  crc: string;
  s: string;
  frame: string;
  k: word;
  len: word;
  i: word;
  zeros,ones: byte;
  need_frames: TStringList;
{
  s = data;
  need_frames = TStringList.Create;
  len = length(data);
  crc = copy(data,len-17,18);
  if buf.Count>0 then
  for i = 0 to buf.Count-1 do
    if length(buf.Strings[i])=len then
      if copy(buf.Strings[i],len-17,18)=crc then
        need_frames.Add(buf.Strings[i]);
  if need_frames.Count>2 then
  for i = 1 to len-18 do
  {
    zeros = 0;
    ones = 0;
    for k = 0 to need_frames.Count-1 do
    {
      frame = need_frames.Strings[k];
      if frame[i]='1' then inc(ones) else inc(zeros);
    }
    if ones>zeros then s[i] = '1' else s[i] = '0';
  }
  need_frames.Free;
  result = s;
}
*/

void make_rx_frame(int snd_ch, int rcvr_nr, int emph, byte last_nrzi_bit, string * raw_data, string * raw_data1)
{
	int swap_i, swap_k;
	string * data;
	string * nrzi_data;
	longword raw_len;
	word len, crc1, crc2;
	int arq_mem = 0;
	string s;
	word i, k, n;

	// Decode RAW-stream

	raw_len = raw_data->Length;

	if (raw_len < 10)
		return;

	char lastbit[10] = "";

//	memcpy(lastbit, &raw_data->Data[raw_data->Length - 7], 7);

//	Debugprintf("Last 7 bits %s", lastbit);

	if (raw_len > 6)
	{
		mydelete(raw_data, raw_len - 6, 7);  // Does this remove trailing flag
		raw_len = raw_data->Length;
	}

	nrzi_data = get_NRZI_data(raw_data, &last_nrzi_bit);

	if (nrzi_data == NULL)
		return;

	data = newString();
	data = get_pkt_data(nrzi_data);

	free(nrzi_data);

	len = data->Length;

	if (len < pkt_raw_min_len)
	{
		freeString(data);
		return;
	}
		

	crc1 = get_fcs(data->Data, len - 2);

	crc2 = (data->Data[len - 1] << 8) | data->Data[len - 2];

	// MEM recovery
/*

	arq_mem = FALSE;

	if (raw_len < 2971)
	{
	DET[emph,rcvr_nr].mem_ARQ_buf[snd_ch].Add(nrzi_data);
	if DET[emph,rcvr_nr].mem_ARQ_buf[snd_ch].Count>MEMRecovery[snd_ch] then DET[emph,rcvr_nr].mem_ARQ_buf[snd_ch].Delete(0);
	if crc1<>crc2 then
	{
	  data = get_pkt_data(memory_ARQ(DET[emph,rcvr_nr].mem_ARQ_buf[snd_ch],nrzi_data));
	  crc1 = get_fcs(data,len-2);
	  arq_mem = TRUE;
	}
  }
*/

	if (crc1 == crc2)
	{
		Debugprintf("Good CRC %x Len %d chan %d rcvr %d emph %d", crc1, len, snd_ch, rcvr_nr, emph);

		if (rcvr_nr == 0)
			emph_decoded[emph] = 1; //Normal

		if (arq_mem)
		{
			stat_r_mem++;
			if (rcvr_nr == 0)
				emph_decoded[emph] = 2; //MEM
		}

		if (detect_list[snd_ch].Count > 0)
		{
			//if detect_list[snd_ch].IndexOf(data)<0 then

			if (my_indexof(&detect_list[snd_ch], data) < 0)
			{
				string * xx = newString();
				Add(&detect_list_c[snd_ch], xx);

				Add(&detect_list[snd_ch], data);

				if (arq_mem)
					stringAdd(xx, "MEM", 3);
				else
					stringAdd(xx, "", 0);
			}
			else
				Debugprintf("Discarding copy rcvr %d", rcvr_nr);
		}
		else
		{
			string * xx = newString();
			Add(&detect_list_c[snd_ch], xx);

			Add(&detect_list[snd_ch], data);

			if (arq_mem)
				stringAdd(xx, "MEM", 3);
			else
				stringAdd(xx, "", 0);
		}
	}
	else
	{
//	Debugprintf("Bad CRC %x %x Len %d", crc1, crc2, len);

	if (len == 17)
		i = 0;

		// Single bit recovery
		i = 0;
		/*

		if (Recovery[snd_ch]=1) and (raw_len<2971) then
		repeat
		  inc(i);
		  if raw_data[i]<>raw_data1[i] then
		  {
			//change bit
			if raw_data[i]='1' then raw_data[i] = '0' else raw_data[i] = '1';
			// get new data
			data = get_pkt_data2(raw_data,last_nrzi_bit);
			//restore bit
			if raw_data[i]='1' then raw_data[i] = '0' else raw_data[i] = '1';
			len = length(data);
			if not (len<pkt_raw_min_len) then
			{
			  crc1 = get_fcs(data,len-2);
			  crc2 = (ord(data[len]) shl 8) or ord(data[len-1]);
			  if crc1=crc2 then
			  {
				if detect_list[snd_ch].Count>0 then
				{
				  //if detect_list[snd_ch].IndexOf(data)<0 then
				  if my_indexof(detect_list[snd_ch],data)<0 then
				  {
					detect_list[snd_ch].Add(data);
					detect_list_c[snd_ch].Add('SINGLE');
				  }
				end
				else
				{
				  detect_list[snd_ch].Add(data);
				  detect_list_c[snd_ch].Add('SINGLE');
				}
				if (rcvr_nr=0) then emph_decoded[emph] = 3; //SINGLE
			  }
			}
		  }
		until (crc1=crc2) or (i=raw_len);
	  }
	  */
	}
}



int lastcrc = 0;


void make_rx_frame_PSK(int snd_ch, int rcvr_nr, int emph, string * data)
{
	word len, crc1, crc2;

	len = data->Length;

	if (len < pkt_raw_min_len)
	{
// ?		freeString(data);
		return;
	}

	crc1 = get_fcs(data->Data, len - 2);
	crc2 = (data->Data[len - 1] << 8) | data->Data[len - 2];

	if (crc1 == crc2)
	{
		Debugprintf("Good CRC %x Len %d chan %d rcvr %d emph %d", crc1, len, snd_ch, rcvr_nr, emph);

		if (rcvr_nr == 0)
			emph_decoded[emph] = 1; //Normal


		if (detect_list[snd_ch].Count > 0)
		{
			if (my_indexof(&detect_list[snd_ch], data) < 0)
			{
				string * xx = newString();
				Add(&detect_list_c[snd_ch], xx);
				Add(&detect_list[snd_ch], data);
			}
			else
				Debugprintf("Discarding copy rcvr %d", rcvr_nr);
		}
		else
		{
			string * xx = newString();
			Add(&detect_list_c[snd_ch], xx);

			xx = duplicateString(data);
			Add(&detect_list[snd_ch], xx);
		}
	}
	else
	{
		data->Data[len] = 0;
	//	Debugprintf("Bad CRC %x %x Len %d", crc1, crc2, len);

		if (crc2 == lastcrc  && len > 40  && len < 45)
			Debugprintf("%s", &data->Data[18]);

		lastcrc = crc2;


// ?		freeString(data);
	}
}


  /*

function memory_ARQ_FEC(buf: TStringList; data: string): string;
var
  len,i,k: integer;
  s,temp: string;
  new_blk,temp_blk: TStringList;
  n,err: byte;
  done: boolean;
{
  s = '';
  if data='' then { result = s; exit; }
  new_blk = TStringList.Create;
  temp_blk = TStringList.Create;
  temp = data;
  len = length(data);
  // Split new data;
  repeat
    n = ord(temp[1]) and $7F;
    err = ord(temp[1]) and $80;
    if err=0 then new_blk.Add(copy(temp,2,n)) else new_blk.Add('');
    delete(temp,1,n+1);
  until temp='';
  // Search blocks
  if (buf.Count>0) and (new_blk.Count>0) then
  {
    i = 0;
    repeat
      // If length is the same
      if length(buf.Strings[i])=len then
      {
        temp = buf.Strings[i];
        // If last 4 bytes is the same
        if copy(temp,len-3,4)=copy(data,len-3,4) then
        {
          temp_blk.Clear;
          repeat
            n = ord(temp[1]) and $7F;
            err = ord(temp[1]) and $80;
            if err=0 then temp_blk.Add(copy(temp,2,n)) else temp_blk.Add('');
            delete(temp,1,n+1);
          until temp='';
          // Add new parts
          if new_blk.Count=temp_blk.Count then
          {
            done = TRUE;
            for k = 0 to new_blk.Count-1 do
            {
              if (new_blk.Strings[k]='') and (temp_blk.Strings[k]<>'') then
                new_blk.Strings[k] = temp_blk.Strings[k];
              // Check if no empty data
              if new_blk.Strings[k]='' then done = FALSE;
            }
          }
        }
      }
      inc(i);
    until (i=buf.Count) or done;
    if done then for k = 0 to new_blk.Count-1 do s = s+new_blk.Strings[k];
  }
  result = s;
  new_blk.Free;
  temp_blk.Free
}

procedure add_to_ARQ_FEC(buf: TStringList; data: string);
{
  if buf.Count=50 then buf.Delete(0);
  buf.Add(data);
}

procedure make_rx_frame_FEC(snd_ch,rcvr_nr: byte; data,fec_data: string; nErr: word);
var
  len,crc1,crc2: word;
  s: string;
  i,k,n: word;
{
  len = length(data);
  if len<17 then exit;
  crc1 = get_fcs(data,len-2);
  crc2 = (ord(data[len]) shl 8) or ord(data[len-1]);
  if crc1=crc2 then
  {
    if detect_list[snd_ch].Count>0 then
      {
        //if detect_list[snd_ch].IndexOf(data)<0 then
        if my_indexof(detect_list[snd_ch],data)<0 then
        {
          detect_list[snd_ch].Add(data);
          detect_list_c[snd_ch].Add('Err: '+inttostr(nErr));
        }
      end
    else
    {
      detect_list[snd_ch].Add(data);
      detect_list_c[snd_ch].Add('Err: '+inttostr(nErr));
    }
    add_to_ARQ_FEC(DET[0,rcvr_nr].mem_ARQ_F_buf[snd_ch],fec_data);
  }
  if crc1<>crc2 then
  {
    data = memory_ARQ_FEC(DET[0,rcvr_nr].mem_ARQ_F_buf[snd_ch],fec_data);
    add_to_ARQ_FEC(DET[0,rcvr_nr].mem_ARQ_F_buf[snd_ch],fec_data);
    if data<>'' then
    {
      len = length(data);
      crc1 = get_fcs(data,len-2);
      crc2 = (ord(data[len]) shl 8) or ord(data[len-1]);
      if crc1=crc2 then
      {
        if detect_list[snd_ch].Count>0 then
        {
          //if detect_list[snd_ch].IndexOf(data)<0 then
          if my_indexof(detect_list[snd_ch],data)<0 then
          {
            detect_list[snd_ch].Add(data);
            detect_list_c[snd_ch].Add('MEM Err: '+inttostr(nErr));
          }
        end
        else
        {
          detect_list[snd_ch].Add(data);
          detect_list_c[snd_ch].Add('MEM Err: '+inttostr(nErr));
        }
      }
    }
  }
}

*/
////////////////////////////  PLL-Peak-detector  ////////////////////////////

void  Mux3(int snd_ch, int rcvr_nr, int emph, float * src1, float * core, float *dest, float * prevI, float * prevQ, int tap, int buf_size)
{
	float pi2 = 2 * pi;

	int i;
	float x;
	float acc1, acc2, acc3, mag;
	int tap4;
	int tap_cnt;
	unsigned int ii, kk;

	float Preemphasis6, Preemphasis12, MUX3_osc, AGC;
	float AFC_IZ1, AFC_QZ1, AFC_IZ2, AFC_QZ2;

	// looks like this is an LPF

	// Get local copy of this detectors variables

	struct TDetector_t * pDET = &DET[emph][rcvr_nr];

	Preemphasis6 = pDET->Preemphasis6[snd_ch];
	Preemphasis12 = pDET->Preemphasis12[snd_ch];
	MUX3_osc = pDET->MUX3_osc[snd_ch];
	AGC = pDET->AGC[snd_ch];
	AFC_IZ2 = pDET->AFC_IZ2[snd_ch];
	AFC_QZ2 = pDET->AFC_QZ2[snd_ch];
	AFC_QZ1 = pDET->AFC_QZ1[snd_ch];
	AFC_IZ1 = pDET->AFC_IZ1[snd_ch];
	//
	tap4 = tap * 4;
	x = rx_freq[snd_ch] * pi2 / RX_Samplerate;

	fmove(&prevI[buf_size], &prevI[0], tap4);
	fmove(&prevQ[buf_size], &prevQ[0], tap4);
	tap_cnt = tap;

	if (prevI[128] != prevI[128])
		prevI[128] = 0;

	for (i = 0; i < buf_size; i++)
	{
		// Pre-emphasis 6dB
		if (emph > 0)
		{
			acc1 = Preemphasis6 - src1[i];
			Preemphasis6 = src1[i];
			src1[i] = acc1;
		}
		// Pre-emphasis 12dB
		if (emph > 1)
		{
			acc1 = Preemphasis12 - src1[i];
			Preemphasis12 = src1[i];
			src1[i] = acc1;
		}
		//
		MUX3_osc = MUX3_osc + x;

		if (MUX3_osc > pi2)
			MUX3_osc = MUX3_osc - pi2;

		if (src1[i] != src1[i])
			src1[i] = 0;

		if (prevI[128] != prevI[128])
			prevI[128] = 0;


		prevI[tap_cnt] = src1[i] * sinf(MUX3_osc);
		prevQ[tap_cnt] = src1[i] * cosf(MUX3_osc);

		if (prevI[128] != prevI[128])
			prevI[tap_cnt] = src1[i] * sinf(MUX3_osc);

		if (prevI[128] != prevI[128])
			prevI[128] = 0;
		/*

			mag = sqrtf(prevI[tap_cnt] * prevI[tap_cnt] + prevQ[tap_cnt] * prevQ[tap_cnt]);
			DET[emph][rcvr_nr].AGC1[snd_ch] = 0.5*DET[emph][rcvr_nr].AGC1[snd_ch] + 0.5*mag;
			AGC = 0.5*AGC + 0.5*DET[emph][rcvr_nr].AGC1[snd_ch];
			if (AGC > 1)
			begin
				prevI[tap_cnt] = prevI[tap_cnt] / AGC;
				prevQ[tap_cnt] = prevQ[tap_cnt] / AGC;
			end
	*/


	// Fast AGC

		mag = sqrtf(prevI[tap_cnt] * prevI[tap_cnt] + prevQ[tap_cnt] * prevQ[tap_cnt]);

		AGC = 0.5 * AGC + 0.5  *mag;

		if (AGC > 1)
		{
			prevI[tap_cnt] = prevI[tap_cnt] / AGC;
			prevQ[tap_cnt] = prevQ[tap_cnt] / AGC;
		}

		ii = i << 2;
		kk = tap << 2;

		// C version of delphi asm code below
		{
			float accum = 0.0f;
			float fp1;

			int ebx;
			float * edi;

			edi = &prevI[i];
			ebx = 0;
			accum = 0.0f;

		fsk_k1:

			// FLD pushes operand onto stack, so old value goes to fp1

			fp1 = accum;
			accum = edi[ebx];
			if (accum != accum)
				accum = 0;

			accum *= core[ebx];
			if (accum != accum)
				accum = 0;
			accum += fp1;
			if (accum != accum)
				accum = 0;

			ebx++;
			if (ebx != tap)
				goto fsk_k1;

			acc1 = accum;

			if (acc1 != acc1)
				acc1 = 0;

			edi = &prevQ[i];

			ebx = 0;
			accum = 0.0f;

		fsk_k2:

			fp1 = accum;
			accum = edi[ebx];
			accum *= core[ebx];
			accum += fp1;

			ebx++;
			if (ebx != tap)
				goto fsk_k2;

			acc2 = accum;
		}

		if (acc1 != acc1)
			acc1 = 0;


		tap_cnt++;

		/// PLL-Detector ///


		dest[i] = (acc1 - AFC_IZ2)*AFC_QZ1 - (acc2 - AFC_QZ2)*AFC_IZ1;

		// Check for NAN

		if (dest[i] != dest[i])
			dest[i] = 0.0f;

		AFC_IZ2 = AFC_IZ1;
		AFC_QZ2 = AFC_QZ1;
		AFC_IZ1 = acc1;
		AFC_QZ1 = acc2;
	}

	pDET->Preemphasis6[snd_ch] = Preemphasis6;
	pDET->Preemphasis12[snd_ch] = Preemphasis12;
	pDET->MUX3_osc[snd_ch] = MUX3_osc;
	pDET->AGC[snd_ch] = AGC;
	pDET->AFC_IZ2[snd_ch] = AFC_IZ2;
	pDET->AFC_QZ2[snd_ch] = AFC_QZ2;
	pDET->AFC_QZ1[snd_ch] = AFC_QZ1;
	pDET->AFC_IZ1[snd_ch] = AFC_IZ1;
}



void Mux3_PSK(int snd_ch, int rcvr_nr, int emph, float * src1, float * core, float *destI, float *destQ, float * prevI, float * prevQ, int tap, int buf_size)
{
	float pi2 = 2 * pi;

	int i;
	float x;
	float acc1, acc2, mag;
	int tap4;
	int prev_cnt, tap_cnt;

	float Preemphasis6, Preemphasis12, MUX3_osc;

	// looks like this is an LPF

	// Get local copy of this detectors variables

	struct TDetector_t * pDET = &DET[emph][rcvr_nr];

	Preemphasis6 = pDET->Preemphasis6[snd_ch];
	Preemphasis12 = pDET->Preemphasis12[snd_ch];
	MUX3_osc = pDET->MUX3_osc[snd_ch];

	tap4 = tap * 4;

	x = rx_freq[snd_ch] * pi2 / RX_Samplerate;

	fmove(&prevI[buf_size], &prevI[0], tap4);
	fmove(&prevQ[buf_size], &prevQ[0], tap4);

	tap_cnt = tap;

	if (prevI[128] != prevI[128])
		prevI[128] = 0;

	for (i = 0; i < buf_size; i++)
	{
		// Pre-emphasis 6dB
		if (emph > 0)
		{
			acc1 = Preemphasis6 - src1[i];
			Preemphasis6 = src1[i];
			src1[i] = acc1;
		}
		// Pre-emphasis 12dB
		if (emph > 1)
		{
			acc1 = Preemphasis12 - src1[i];
			Preemphasis12 = src1[i];
			src1[i] = acc1;
		}

		MUX3_osc = MUX3_osc + x;

		if (MUX3_osc > pi2)
			MUX3_osc = MUX3_osc - pi2;

		prevI[tap_cnt] = src1[i] * sinf(MUX3_osc);
		prevQ[tap_cnt] = src1[i] * cosf(MUX3_osc);


		// C version of delphi asm code 
		{
			float accum = 0.0f;
			float fp1;

			int ebx;
			float * edi;

			edi = &prevI[i];
			ebx = 0;
			accum = 0.0f;

		fsk_k1:

			// FLD pushes operand onto stack, so old value goes to fp1

			fp1 = accum;
			accum = edi[ebx];
			accum *= core[ebx];
			accum += fp1;

			ebx++;

			if (ebx != tap)
				goto fsk_k1;

			acc1 = accum;

			edi = &prevQ[i];

			ebx = 0;
			accum = 0.0f;

		fsk_k2:

			fp1 = accum;
			accum = edi[ebx];
			accum *= core[ebx];
			accum += fp1;

			ebx++;
			if (ebx != tap)
				goto fsk_k2;

			acc2 = accum;
		}

		if (acc1 != acc1)
			acc1 = 0;

		tap_cnt++;

		destI[i] = acc1;
		destQ[i] = acc2;
	}

	pDET->Preemphasis6[snd_ch] = Preemphasis6;
	pDET->Preemphasis12[snd_ch] = Preemphasis12;
	pDET->MUX3_osc[snd_ch] = MUX3_osc;

}



void decode_stream_MPSK(int snd_ch, int rcvr_nr, float *  src, int buf_size, int  last)
{
	float  pi2 = 2 * pi;

#define NR_FEC_CH 3

	float agc_fast = 0.01f;
	float agc_fast1 = 1 - agc_fast;
	float agc_slow = agc_fast / 4;
	float agc_slow1 = 1 - agc_slow;

#define dcd_corr 0.11111

	word  dcnt, dsize;
	word  i, k, j, j1, j2, j3;
	single  x, x1;
	single  amp, acc1, acc2;
	single  sumI, sumQ, sumIQ, muxI, muxQ;
	word  tap_cnt, tap_cnt1;
	single  afc_lim;
	word  i_tap, tap;
	single  AFC, k1, k2, freq;
	single  max, div_bit_afc, baudrate;
	word  max_cnt;
	single  AmpI, AmpQ, angle, muxI1, muxQ1, muxI2, muxQ2, sumIQ1, sumIQ2;
	single  AFC_acc1, AFC_acc2;
	single  BIT_acc1, BIT_acc2;
	integer  AFC_newpkpos;
	//
	single  threshol;
	single  tr;
	byte fec_ch, bit;
	longword ii, kk;
	single * core, prevI, prevQ;
	//
	int64  bit64;
	boolean  hdr_ok;
	byte  fec_code;
	string  fec_data_blk;
	string  line, line1;
	integer nErr;
	word  crc1, crc2, size;

	byte  hdr_byte[15];

	tr = dcd_threshold * dcd_corr;

	if (last)
	{
		if (dcd_hdr_cnt[snd_ch] == 0)
			dcd_on_hdr[snd_ch] = 0;

		dcd_bit_cnt[snd_ch] = 0;
	}

	baudrate = 400;
	div_bit_afc = 1.0f / round(BIT_AFC*(RX_Samplerate / 11025));
	x1 = baudrate / RX_Samplerate;
	max_cnt = round(RX_Samplerate / baudrate);
	//

	afc_lim = rx_baudrate[snd_ch] * 0.1;
	dsize = buf_size / n_INTR[snd_ch];
	tap = LPF_tap[snd_ch];
	i_tap = INTR_tap[snd_ch];
	freq = rx_freq[snd_ch];
	for (fec_ch = 0; fec_ch <= NR_FEC_CH; fec_ch++)
	{
		struct TMChannel_t * pDET = &DET[0][rcvr_nr].MChannel[snd_ch][fec_ch];

		fmove(&pDET->prev_dLPFI_buf[buf_size], &pDET->prev_dLPFI_buf[0], i_tap * 4);
		fmove(&pDET->prev_dLPFQ_buf[buf_size], &pDET->prev_dLPFQ_buf[0], i_tap * 4);
		fmove(&pDET->prev_LPF1I_buf[dsize], &pDET->prev_LPF1I_buf[0], tap * 4);
		fmove(&pDET->prev_LPF1Q_buf[dsize], &pDET->prev_LPF1Q_buf[0], tap * 4);
		fmove(&pDET->prev_AFCI_buf[dsize], &pDET->prev_AFCI_buf[0], tap * 4);
		fmove(&pDET->prev_AFCQ_buf[dsize], &pDET->prev_AFCQ_buf[0], tap * 4);
	}
	tap_cnt = i_tap;
	tap_cnt1 = tap;
	dcnt = 0;
	k = 0;
	/*

	for i = 0 to buf_size-1 do
  {
    for fec_ch = 0 to NR_FEC_CH do with DET[0,rcvr_nr] do
    {
      x = (freq+AFC_dF[snd_ch]+ch_offset[fec_ch])*pi2/RX_SampleRate;
      with MChannel[snd_ch,fec_ch] do
      {
        MUX_osc = MUX_osc+x;
        if MUX_osc>pi2 then MUX_osc = MUX_osc-pi2;
        prev_dLPFI_buf[tap_cnt] = src[i]*sin(MUX_osc);
        prev_dLPFQ_buf[tap_cnt] = src[i]*cos(MUX_osc);
        prevI = @prev_dLPFI_buf;
        prevQ = @prev_dLPFQ_buf;
        core = @INTR_core[snd_ch];
        // Decimation filter
        ii = i shl 2;
        kk = i_tap shl 2;
        asm
          push eax;
          push ebx;
          push edi;
          push esi;
          mov edi,prevI;
          mov esi,core;
          add edi,ii;
          mov eax,kk;
          xor ebx,ebx;
          fldz;
          @k1:
            fld dword ptr [edi+ebx];
            fmul dword ptr [esi+ebx];
            fadd;
            add ebx,4;
            cmp ebx,eax;
          jne @k1;
          fstp dword ptr acc1;
          wait;
          mov edi,prevQ;
          add edi,ii;
          xor ebx,ebx;
          fldz;
          @k2:
            fld dword ptr [edi+ebx];
            fmul dword ptr [esi+ebx];
            fadd;
            add ebx,4;
            cmp ebx,eax;
          jne @k2;
          fstp dword ptr acc2;
          wait;
          pop esi;
          pop edi;
          pop ebx;
          pop eax;
        }
      }
      if fec_ch=NR_FEC_CH then inc(tap_cnt);
      // Decimation
      if dcnt=0 then
      {
        with MChannel[snd_ch,fec_ch] do
        {
          prev_LPF1I_buf[tap_cnt1] = acc1;
          prev_LPF1Q_buf[tap_cnt1] = acc2;
          prev_AFCI_buf[tap_cnt1] = acc1;
          prev_AFCQ_buf[tap_cnt1] = acc2;
          // Bit-filter
          prevI = @prev_LPF1I_buf;
          prevQ = @prev_LPF1Q_buf;
          core = @LPF_core[snd_ch];
          ii = k shl 2;
          kk = tap shl 2;
          asm
            push eax;
            push ebx;
            push edi;
            push esi;
            mov edi,prevI;
            mov esi,core;
            add edi,ii;
            mov eax,kk;
            xor ebx,ebx;
            fldz;
            @k1:
              fld dword ptr [edi+ebx];
              fmul dword ptr [esi+ebx];
              fadd;
              add ebx,4;
              cmp ebx,eax;
            jne @k1;
            fstp dword ptr BIT_acc1;
            wait;
            mov edi,prevQ;
            add edi,ii;
            xor ebx,ebx;
            fldz;
            @k2:
              fld dword ptr [edi+ebx];
              fmul dword ptr [esi+ebx];
              fadd;
              add ebx,4;
              cmp ebx,eax;
            jne @k2;
            fstp dword ptr BIT_acc2;
            wait;
            pop esi;
            pop edi;
            pop ebx;
            pop eax;
          }
          // AFC-filter
          prevI = @prev_AFCI_buf;
          prevQ = @prev_AFCQ_buf;
          core = @AFC_core[snd_ch];
          ii = k shl 2;
          kk = tap shl 2;
          asm
            push eax;
            push ebx;
            push edi;
            push esi;
            mov edi,prevI;
            mov esi,core;
            add edi,ii;
            mov eax,kk;
            xor ebx,ebx;
            fldz;
            @k1:
              fld dword ptr [edi+ebx];
              fmul dword ptr [esi+ebx];
              fadd;
              add ebx,4;
              cmp ebx,eax;
            jne @k1;
            fstp dword ptr AFC_acc1;
            wait;
            mov edi,prevQ;
            add edi,ii;
            xor ebx,ebx;
            fldz;
            @k2:
              fld dword ptr [edi+ebx];
              fmul dword ptr [esi+ebx];
              fadd;
              add ebx,4;
              cmp ebx,eax;
            jne @k2;
            fstp dword ptr AFC_acc2;
            wait;
            pop esi;
            pop edi;
            pop ebx;
            pop eax;
          }
        }
        // AGC
        amp = sqrt(BIT_acc1*BIT_acc1+BIT_acc2*BIT_acc2);
        if amp>PSK_AGC[snd_ch] then
          PSK_AGC[snd_ch] = PSK_AGC[snd_ch]*agc_fast1+amp*agc_fast
        else
          PSK_AGC[snd_ch] = PSK_AGC[snd_ch]*agc_slow1+amp*agc_slow;
        if PSK_AGC[snd_ch]>1 then
        {
          BIT_acc1 = BIT_acc1/PSK_AGC[snd_ch];
          BIT_acc2 = BIT_acc2/PSK_AGC[snd_ch];
          AFC_acc1 = AFC_acc1/PSK_AGC[snd_ch];
          AFC_acc2 = AFC_acc2/PSK_AGC[snd_ch];
          amp = amp/PSK_AGC[snd_ch];
        }
        // AFC correction
        with MChannel[snd_ch,fec_ch] do
        {
          sumIQ = (AFC_acc1-AFC_IZ2)*AFC_QZ1-(AFC_acc2-AFC_QZ2)*AFC_IZ1;
          AFC_IZ2 = AFC_IZ1;
          AFC_QZ2 = AFC_QZ1;
          AFC_IZ1 = AFC_acc1;
          AFC_QZ1 = AFC_acc2;
        }
        AFC_dF[snd_ch] = AFC_dF[snd_ch]-sumIQ*0.07; // AFC_LPF=1
        if AFC_dF[snd_ch]>AFC_lim  then AFC_dF[snd_ch] = AFC_lim;
        if AFC_dF[snd_ch]<-AFC_lim then AFC_dF[snd_ch] = -AFC_lim;
        //
        MChannel[snd_ch,fec_ch].AFC_bit_buf1I[AFC_cnt[snd_ch]] = BIT_acc1;
        MChannel[snd_ch,fec_ch].AFC_bit_buf1Q[AFC_cnt[snd_ch]] = BIT_acc2;
        MChannel[snd_ch,fec_ch].AFC_bit_buf2[AFC_cnt[snd_ch]] = amp;
        if fec_ch=NR_FEC_CH then
        {
          inc(AFC_cnt[snd_ch]);
          AFC_bit_osc[snd_ch] = AFC_bit_osc[snd_ch]+x1;
          if AFC_bit_osc[snd_ch]>=1 then
          {
            // Находим максимум в буфере синхронизации
            for j = 0 to NR_FEC_CH do
            {
              max = 0;
              for j1 = 0 to AFC_cnt[snd_ch]-1 do
              {
                amp = MChannel[snd_ch,j].AFC_bit_buf2[j1];
                AFC_bit_buf[snd_ch][j1] = AFC_bit_buf[snd_ch][j1]*0.95+amp*0.05;
                if AFC_bit_buf[snd_ch][j1]>max then
                {
                  AFC_newpkpos = j1;
                  max = AFC_bit_buf[snd_ch][j1];
                }
              }
              k1 = AFC_newpkpos/(AFC_cnt[snd_ch]-1);
              k2 = pila(k1)-1;
              //AFC = div_bit_afc*k2;
              AFC = div_bit_afc*k2*0.25; //for 4 carriers
              if k1>0.5 then AFC_bit_osc[snd_ch] = AFC_bit_osc[snd_ch]+AFC else AFC_bit_osc[snd_ch] = AFC_bit_osc[snd_ch]-AFC;
              //DCD feature
              if last then
              {
                DCD_LastPkPos[snd_ch] = DCD_LastPkPos[snd_ch]*0.96+AFC_newpkpos*0.04;
                DCD_LastPerc[snd_ch] = DCD_LastPerc[snd_ch]*0.96+abs(AFC_newpkpos-DCD_LastPkPos[snd_ch])*0.04;
                if (DCD_LastPerc[snd_ch]>=tr) or (DCD_LastPerc[snd_ch]<0.00001) then dcd_bit_cnt[snd_ch] = dcd_bit_cnt[snd_ch]+1 else dcd_bit_cnt[snd_ch] = dcd_bit_cnt[snd_ch]-1;
              }
              // Bit-detector
              with MChannel[snd_ch,j] do
              {
                AmpI = AFC_bit_buf1I[AFC_newpkpos];
                AmpQ = AFC_bit_buf1Q[AFC_newpkpos];
                muxI1 = AmpI*AFC_IIZ1;
                muxI2 = AmpQ*AFC_IIZ1;
                muxQ1 = AmpQ*AFC_QQZ1;
                muxQ2 = AmpI*AFC_QQZ1;
                sumIQ1 = muxI1+muxQ1;
                sumIQ2 = muxI2-muxQ2;
                angle = arctan2(sumIQ2,sumIQ1);
                AFC_IIZ1 = AmpI;
                AFC_QQZ1 = AmpQ;
                // Phase corrector
                if abs(angle)<PI5 then AngleCorr = AngleCorr*0.9-angle*0.1
                else
                {
                if angle>0 then
                  AngleCorr = AngleCorr*0.9+(PI-angle)*0.1
                else
                  AngleCorr = AngleCorr*0.9+(-PI-angle)*0.1;
                }
                angle = angle+AngleCorr;
              }
              if abs(angle)<PI5 then bit = RX_BIT1 else bit = RX_BIT0;
              // DCD on flag
              if last then
              {
                if dcd_hdr_cnt[snd_ch]>0 then dec(dcd_hdr_cnt[snd_ch]);
                DCD_header[snd_ch] = (DCD_header[snd_ch] shr 1) or (bit shl 24);
                if
                ((DCD_header[snd_ch] and $FFFF0000)=$7E7E0000) or
                ((DCD_header[snd_ch] and $FFFFFF00)=$7E000000) or
                ((DCD_header[snd_ch] and $FFFFFF00)=$00000000) then
                { dcd_hdr_cnt[snd_ch] = 48; dcd_on_hdr[snd_ch] = TRUE; }
              }
              // header stream
              bit64 = (bit64 or bit) shl 56;
              FEC_header1[snd_ch][1] = (FEC_header1[snd_ch][1] shr 1) or (FEC_header1[snd_ch][0] shl 63);
              FEC_header1[snd_ch][0] = (FEC_header1[snd_ch][0] shr 1) or bit64;
              // copy body
              if (frame_status[snd_ch]=FRAME_LOAD) then
              {
                bit_stream[snd_ch] = (bit_stream[snd_ch] shr 1)+bit;
                inc(bit_cnt[snd_ch]);
                if bit_cnt[snd_ch]=8 then
                {
                  bit_cnt[snd_ch] = 0;
                  inc(FEC_len_cnt[snd_ch]);
                  FEC_rx_data[snd_ch] = FEC_rx_data[snd_ch]+chr(bit_stream[snd_ch]);
                  if FEC_len_cnt[snd_ch]=FEC_len[snd_ch] then
                  {
                    // descrambler
                    FEC_rx_data[snd_ch] = scrambler(FEC_rx_data[snd_ch]);
                    // deinterleave
                    FEC_blk_int[snd_ch] = ((FEC_len[snd_ch]-1) div 16)+1;
                    line = FEC_rx_data[snd_ch];
                    j3 = 1;
                    for j1 = 1 to 16 do
                      for j2 = 0 to FEC_blk_int[snd_ch]-1 do
                        if ((j2*16+j1)<=FEC_len[snd_ch]) and (j3<=FEC_len[snd_ch]) then
                        {
                          FEC_rx_data[snd_ch][j2*16+j1] = line[j3];
                          inc(j3);
                        }
                    // RS-decode
                    line = FEC_rx_data[snd_ch];
                    FEC_rx_data[snd_ch] = '';
                    repeat
                      line1 = copy(line,1,16);
                      size = length(line1);
                      FillChar(xEncoded,SizeOf(xEncoded),0);
                      FillChar(xDecoded,SizeOf(xDecoded),0);
                      move(line1[1],xEncoded[0],size);
                      RS.InitBuffers;
                      nErr = RS.DecodeRS(xEncoded,xDecoded);
                      line1 = '';
                      for j1 = MaxErrors*2 to size-1 do line1 = line1+chr(xDecoded[j1]);
                      FEC_rx_data[snd_ch] = FEC_rx_data[snd_ch]+line1;
                      if nErr>=0 then FEC_err[snd_ch] = FEC_err[snd_ch]+nErr;
                      // For MEM-ARQ
                      fec_code = length(line1) and $7F;
                      if nErr<0 then fec_code = fec_code or $80;
                      fec_data_blk = fec_data_blk+chr(fec_code)+line1;
                      delete(line,1,16);
                    until line='';
                    //
                    make_rx_frame_FEC(snd_ch,rcvr_nr,FEC_rx_data[snd_ch],fec_data_blk,FEC_err[snd_ch]);
                    FEC_rx_data[snd_ch] = '';
                    frame_status[snd_ch] = FRAME_WAIT;
                    FEC_header1[snd_ch][0] = 0;
                    FEC_header1[snd_ch][1] = 0;
                  }
                }
              }
              hdr_ok = FALSE;
              if (frame_status[snd_ch]=FRAME_WAIT) then
              {
                j1 = FEC_header1[snd_ch][1] shr 16 xor $7E7E;
                asm
                  push ax;
                  push bx;
                  push cx;
                  mov ax,15;
                  mov bx,j1;
                  @loop:
                    mov cx,bx;
                    and cx,1;
                    cmp cx,1;
                    jne @is_zero;
                    inc ah; // count damaged bits
                    @is_zero:
                    shr bx,1;
                    dec al;
                  jnz @loop;
                  cmp ah,5; // greater than 4 bits
                  jnb @greater;
                  mov hdr_ok,TRUE;
                  @greater:
                  pop cx;
                  pop bx;
                  pop ax;
                }
              }
              //if (FEC_header1[snd_ch][1] shr 24 and $FF=$7E) and (frame_status[snd_ch]=FRAME_WAIT) then
              if hdr_ok then
              {
                hdr_ok = FALSE;
                hdr_byte[1] = FEC_header1[snd_ch][0] shr 56 and $FF;
                hdr_byte[2] = FEC_header1[snd_ch][0] shr 48 and $FF;
                hdr_byte[3] = FEC_header1[snd_ch][0] shr 40 and $FF;
                hdr_byte[4] = FEC_header1[snd_ch][0] shr 32 and $FF;
                hdr_byte[5] = FEC_header1[snd_ch][0] shr 24 and $FF;
                hdr_byte[6] = FEC_header1[snd_ch][0] shr 16 and $FF;
                hdr_byte[7] = FEC_header1[snd_ch][0] shr 8 and $FF;
                hdr_byte[8] = FEC_header1[snd_ch][0] and $FF;
                hdr_byte[9] = FEC_header1[snd_ch][1] shr 56 and $FF;
                hdr_byte[10] = FEC_header1[snd_ch][1] shr 48 and $FF;
                hdr_byte[11] = FEC_header1[snd_ch][1] shr 40 and $FF;
                hdr_byte[12] = FEC_header1[snd_ch][1] shr 32 and $FF;
                hdr_byte[13] = FEC_header1[snd_ch][1] shr 24 and $FF;
                hdr_byte[14] = FEC_header1[snd_ch][1] shr 16 and $FF;
                if (hdr_byte[13]=$7E) and (hdr_byte[14]=$7E) then
                {
                  FEC_len[snd_ch] = hdr_byte[12] shl 8 + hdr_byte[11];
                  line = #$7E#$7E+chr(hdr_byte[12])+chr(hdr_byte[11]);
                  crc1 = hdr_byte[10] shl 8 + hdr_byte[9];
                  crc2 = get_fcs(line,4);
                  if crc1=crc2 then hdr_ok = TRUE;
                }
                if not hdr_ok then
                {
                  line = '';
                  for j1 = 14 downto 1 do line = line+chr(hdr_byte[j1]);
                  FillChar(xEncoded,SizeOf(xEncoded),0);
                  FillChar(xDecoded,SizeOf(xDecoded),0);
                  line = copy(line,7,8)+copy(line,1,6);
                  move(line[1],xEncoded[0],14);
                  RS.InitBuffers;
                  nErr = RS.DecodeRS(xEncoded,xDecoded);
                  if nErr>-1 then
                  {
                    line = '';
                    for j1 = 8 to 13 do line = line+chr(xDecoded[j1]);
                    if (line[1]=#$7E) and (line[2]=#$7E) then
                    {
                      FEC_len[snd_ch] = ord(line[3]) shl 8 + ord(line[4]);
                      crc1 = ord(line[5]) shl 8 + ord(line[6]);
                      line = copy(line,1,4);
                      crc2 = get_fcs(line,4);
                      if crc1=crc2 then hdr_ok = TRUE;
                    }
                  }
                }
                if hdr_ok then
                {
                  FEC_len[snd_ch] = FEC_len[snd_ch] and 1023; //limit of length
                  if FEC_len[snd_ch]>0 then
                  {
                    frame_status[snd_ch] = FRAME_LOAD;
                    FEC_len_cnt[snd_ch] = 0;
                    bit_cnt[snd_ch] = 0;
                    FEC_err[snd_ch] = 0;
                    FEC_rx_data[snd_ch] = '';
                    fec_data_blk = '';
                  }
                }
              }
            }
            // Finalize
            if AFC_cnt[snd_ch]<=max_cnt then
              for j = AFC_cnt[snd_ch] to max_cnt+5 do
                AFC_bit_buf[snd_ch][j] = 0.95*AFC_bit_buf[snd_ch][j];
            AFC_cnt[snd_ch] = 0;
            AFC_bit_osc[snd_ch] = AFC_bit_osc[snd_ch]-1;
          }
        }
        if fec_ch=NR_FEC_CH then
        {
          inc(tap_cnt1);
          inc(k);
        }
      }
    }
    dcnt = (dcnt+1) mod n_INTR[snd_ch];
	*/
 
  
}



void decode_stream_FSK(int last, int snd_ch, int rcvr_nr, int emph, float * src_buf, float * bit_buf, int  buf_size, string * data)
{
	int i, k, j, n;
	UCHAR bit;
	UCHAR raw_bit;
	UCHAR raw_bit1;
	UCHAR raw_bit2;
	float AFC, x, amp, k1, k2;
	float baudrate;
	float div_bit_afc;
	word max_cnt;
	float threshold;
	float tr;

	byte sample_cnt;
	float PkAmp, PkAmpMax, MaxAmp, MinAmp, AverageAmp;
	int newpkpos;
	float bit_osc;
	byte last_rx_bit, bit_stream, frame_status;

	// get saved values to local variables to speed up access

	struct TDetector_t * pDET = &DET[emph][rcvr_nr];

	last_rx_bit = pDET->last_rx_bit[snd_ch];
	sample_cnt = pDET->sample_cnt[snd_ch];
	PkAmp = pDET->PkAmp[snd_ch];
	PkAmpMax = pDET->PkAmpMax[snd_ch];
	newpkpos = pDET->newpkpos[snd_ch];
	bit_osc = pDET->bit_osc[snd_ch];
	MaxAmp = pDET->MaxAmp[snd_ch];
	MinAmp = pDET->MinAmp[snd_ch];
	AverageAmp = pDET->AverageAmp[snd_ch];
	bit_stream = pDET->bit_stream[snd_ch];
	frame_status = pDET->frame_status[snd_ch];
	//
	tr = dcd_threshold * dcd_corr;

	if (last)
	{
		// Update DCD status

		if (dcd_hdr_cnt[snd_ch] == 0)
			dcd_on_hdr[snd_ch] = 0;
	
		dcd_bit_cnt[snd_ch] = 0;
	}

	// src_buf is input samples processed in some way.
	// not sure what bit_buf is, but guess bits extracted from samples
	// but then why floats ??

	baudrate = 300;

	div_bit_afc = 1.0f / roundf(BIT_AFC*(RX_Samplerate / 11025.0f));

	x = baudrate / RX_Samplerate;

	// I guess max_cnt is samples per bit

	//was - why + 1 then round??
	max_cnt = roundf(RX_Samplerate / baudrate) + 1;

	max_cnt = (RX_Samplerate / baudrate);

	for (i = 0; i < buf_size; i++)
	{
		// Seems to be accumulating squares of all samples in the input for one bit period
	
		bit_buf[sample_cnt] = 0.95*bit_buf[sample_cnt] + 0.05*src_buf[i] * src_buf[i];
	
		// Check for NAN

		if (bit_buf[sample_cnt] != bit_buf[sample_cnt])
			bit_buf[sample_cnt] = 0.0f;

		// Находим максимум в буфере синхронизации
		// Find the maximum in the synchronization buffer

		if (bit_buf[sample_cnt] > PkAmpMax)
		{
			PkAmp = src_buf[i];
			PkAmpMax = bit_buf[sample_cnt];
			newpkpos = sample_cnt;
		}
		sample_cnt++;

		bit_osc = bit_osc + x;

		// This seems to be how it does samples to bits


		if (bit_osc >= 0.99f)					// Allow for rounding errors
		{
			if (sample_cnt <= max_cnt)
				for (k = sample_cnt; k <= max_cnt; k++)
					bit_buf[k] = 0.95*bit_buf[k];

			k1 = (1.0f * newpkpos) / (sample_cnt - 1);
			k2 = pila(k1) - 1;
			AFC = div_bit_afc * k2;

			if (k1 > 0.5)
				bit_osc = bit_osc + AFC;
			else
				bit_osc = bit_osc - AFC;

			PkAmpMax = 0;
			sample_cnt = 0;

			// Not sure about this, but if bit_buf gets to NaN it stays there

			bit_osc = bit_osc - 1;
			//DCD feature
			if (last)
			{
				DCD_LastPkPos[snd_ch] = DCD_LastPkPos[snd_ch] * 0.96 + newpkpos * 0.04;
				DCD_LastPerc[snd_ch] = DCD_LastPerc[snd_ch] * 0.96 + fabsf(newpkpos - DCD_LastPkPos[snd_ch])*0.04;

				if (DCD_LastPerc[snd_ch] >= tr || DCD_LastPerc[snd_ch] < 0.00001f)
					dcd_bit_cnt[snd_ch]++;
				else
					dcd_bit_cnt[snd_ch]--;
			}

 			amp = PkAmp;

			if (amp > 0)
				raw_bit1 = RX_BIT1;
			else
				raw_bit1 = RX_BIT0;
			//
			if (amp > 0)
				MaxAmp = MaxAmp * 0.9f + amp*0.1f; //0.9  
			else
				MinAmp = MinAmp * 0.9f + amp*0.1f;

			amp = amp - (MaxAmp + MinAmp)*0.5f;

			//  Bit-detector

			AverageAmp = AverageAmp * 0.5f + amp*0.5f;
			threshold = 0.5f * AverageAmp;

			if (amp > threshold)
				raw_bit = RX_BIT1;
			else
				raw_bit = RX_BIT0;

			// 0.75

			if (amp > 0.75*AverageAmp)
				raw_bit2 = RX_BIT1;
			else
				raw_bit2 = RX_BIT0;

			if (raw_bit != raw_bit2)
				raw_bit1 = raw_bit2;

			//NRZI

			if (raw_bit == last_rx_bit)
				bit = RX_BIT1;
			else
				bit = RX_BIT0;

			last_rx_bit = raw_bit;
			//
			bit_stream = (bit_stream >> 1) | bit;
	
			// DCD on flag

			if (last)
			{
				if (dcd_hdr_cnt[snd_ch] > 0)
					dcd_hdr_cnt[snd_ch]--;

				DCD_header[snd_ch] = (DCD_header[snd_ch] >> 1) | (bit << 24);

				if (((DCD_header[snd_ch] & 0xFFFF0000) == 0x7E7E0000) ||
					((DCD_header[snd_ch] & 0xFFFFFF00) == 0x7E000000) ||
					((DCD_header[snd_ch] & 0xFFFFFF00) == 0x00000000))
				{
					dcd_hdr_cnt[snd_ch] = 48;
					dcd_on_hdr[snd_ch] = 1;
				}
			}

/*
		// I think Andy looks for both flag and abort here. I think it would be
			// clearer to detect abort separately

			// This may not be optimun but should work

			if (bit_stream == 0xFF || bit_stream == 0x7F || bit_stream == 0xFE)
			{
				// All have 7 or more 1 bits

				if (frame_status == FRAME_LOAD)
				{
					// Have started receiving frame 

//					Debugprintf("Frame Abort len= %d bits", pDET->raw_bits[snd_ch].Length);
					
					frame_status = FRAME_WAIT;

					// Raw stream init

					pDET->raw_bits[snd_ch].Length = 0;
					pDET->raw_bits1[snd_ch].Length = 0;
					pDET->last_nrzi_bit[snd_ch] = raw_bit;
				}
				continue;	
			}





//			if (((bit_stream & FRAME_FLAG) == FRAME_FLAG) && (frame_status == FRAME_LOAD))
			if (bit_stream == FRAME_FLAG && frame_status == FRAME_LOAD)
			{
				frame_status = FRAME_WAIT;
				Debugprintf("Got Frame len = %d", pDET->raw_bits[snd_ch].Length);
				make_rx_frame(snd_ch, rcvr_nr, emph, pDET->last_nrzi_bit[snd_ch], &pDET->raw_bits[snd_ch], &pDET->raw_bits1[snd_ch]);
			}

			if (frame_status == FRAME_LOAD)
			{
				//Raw stream

				if (pDET->raw_bits[snd_ch].Length < 36873)
				{
					if (raw_bit == RX_BIT1)
						stringAdd(&pDET->raw_bits[snd_ch], "1", 1);
					 else
						stringAdd(&pDET->raw_bits[snd_ch], "0", 1);
				}
				
				if (pDET->raw_bits1[snd_ch].Length < 36873)
				{
					if (raw_bit1 == RX_BIT1)
						stringAdd(&pDET->raw_bits1[snd_ch], "1", 1);
					else
						stringAdd(&pDET->raw_bits1[snd_ch], "0", 1);
				}
				//
			}
	
//			if ((bit_stream & FRAME_FLAG == FRAME_FLAG) && (frame_status == FRAME_WAIT))
			if (bit_stream == FRAME_FLAG && frame_status == FRAME_WAIT)
			{
				frame_status = FRAME_LOAD;
				
				// Raw stream init

				pDET->raw_bits[snd_ch].Length = 0;
				pDET->raw_bits1[snd_ch].Length = 0;
				pDET->last_nrzi_bit[snd_ch] = raw_bit;

//				Debugprintf("New Frame");
			}

			*/

	
			if (bit_stream == 0xFF || bit_stream == 0x7F || bit_stream == 0xFE)
			{
				// All have 7 or more 1 bits

				if (frame_status == FRAME_LOAD)
				{
					// Have started receiving frame 

//					Debugprintf("Frame Abort len= %d bits", pDET->raw_bits[snd_ch].Length);

					frame_status = FRAME_WAIT;

					// Raw stream init

					pDET->raw_bits[snd_ch].Length = 0;
					pDET->raw_bits1[snd_ch].Length = 0;
					pDET->last_nrzi_bit[snd_ch] = raw_bit;

					if (last)
						chk_dcd1(snd_ch, buf_size);
				}
				continue;
			}

			if (((bit_stream & FRAME_FLAG) == FRAME_FLAG) && (frame_status == FRAME_LOAD))
			{
				frame_status = FRAME_WAIT;

				if (pDET->raw_bits[snd_ch].Length == 7)		// Another flag
				{
					// Raw stream init

					pDET->raw_bits[snd_ch].Length = 0;
					pDET->raw_bits1[snd_ch].Length = 0;
					pDET->last_nrzi_bit[snd_ch] = raw_bit;
				}
				
				if (pDET->raw_bits[snd_ch].Length > 7)
				{
//					Debugprintf("Got Frame len = %d Lastbit %d", pDET->raw_bits[snd_ch].Length, raw_bit == 128);
					make_rx_frame(snd_ch, rcvr_nr, emph, pDET->last_nrzi_bit[snd_ch], &pDET->raw_bits[snd_ch], &pDET->raw_bits1[snd_ch]);
				}
			}


			if (frame_status == FRAME_LOAD)
			{
				//Raw stream

				if (pDET->raw_bits[snd_ch].Length < 36873)
				{
					if (raw_bit == RX_BIT1)
						stringAdd(&pDET->raw_bits[snd_ch], "1", 1);
					else
						stringAdd(&pDET->raw_bits[snd_ch], "0", 1);
				}

				if (pDET->raw_bits1[snd_ch].Length < 36873)
				{
					if (raw_bit1 == RX_BIT1)
						stringAdd(&pDET->raw_bits1[snd_ch], "1", 1);
					else
						stringAdd(&pDET->raw_bits1[snd_ch], "0", 1);
				}
				//
			}

			if (((bit_stream & FRAME_FLAG) == FRAME_FLAG) && (frame_status == FRAME_WAIT))
			{
				frame_status = FRAME_LOAD;

				// Raw stream init

				pDET->raw_bits[snd_ch].Length = 0;
				pDET->raw_bits1[snd_ch].Length = 0;
				pDET->last_nrzi_bit[snd_ch] = raw_bit;

				//				Debugprintf("New Frame");
			}

		}
	}
	pDET->sample_cnt[snd_ch] = sample_cnt;
	pDET->PkAmp[snd_ch] = PkAmp;
	pDET->PkAmpMax[snd_ch] = PkAmpMax;
	pDET->newpkpos[snd_ch] = newpkpos;
	pDET->bit_osc[snd_ch] = bit_osc;
	pDET->MaxAmp[snd_ch] = MaxAmp;
	pDET->MinAmp[snd_ch] = MinAmp;
	pDET->AverageAmp[snd_ch] = AverageAmp;
	pDET->bit_stream[snd_ch] = bit_stream;
	pDET->frame_status[snd_ch] = frame_status;
	pDET->last_rx_bit[snd_ch] = last_rx_bit;
}

int stats[2] = { 0 };

void decode_stream_BPSK(int last, int snd_ch, int rcvr_nr, int emph, float * srcI, float * srcQ, float * bit_buf, int  buf_size, string * data)
{
	float agc_fast = 0.01f;
	float agc_fast1 = 1 - agc_fast;
	float agc_slow = agc_fast / 4;
	float agc_slow1 = 1 - agc_slow;

	int i, k, j, n;
	byte dibit, bit;
	single afc, x, amp, k1, k2;
	single baudrate;
	single div_bit_afc;
	word max_cnt;
	single threshold;
	single tr;
	single KCorr, AngleCorr, angle, muxI1, muxQ1, muxI2, muxQ2, sumIQ1, sumIQ2;
	byte newpkpos, sample_cnt;
	single PkAmpI, PkAmpQ, PkAmpMax, PSK_AGC;
	single PSK_IZ1, PSK_QZ1;
	single bit_osc;
	byte bit_stuff_cnt, last_rx_bit, frame_status, bit_cnt, bit_stream, byte_rx;

	// get saved values to local variables to speed up access

	struct TDetector_t * pDET = &DET[emph][rcvr_nr];

	// global -> local

	AngleCorr = pDET->AngleCorr[snd_ch];
	bit_stuff_cnt = pDET->bit_stuff_cnt[snd_ch];
	sample_cnt = pDET->sample_cnt[snd_ch];
	PSK_AGC = pDET->PSK_AGC[snd_ch];
	PkAmpI = pDET->PkAmpI[snd_ch];
	PkAmpQ = pDET->PkAmpQ[snd_ch];
	PkAmpMax = pDET->PkAmpMax[snd_ch];
	newpkpos = pDET->newpkpos[snd_ch];
	PSK_IZ1 = pDET->PSK_IZ1[snd_ch];
	PSK_QZ1 = pDET->PSK_QZ1[snd_ch];
	bit_osc = pDET->bit_osc[snd_ch];
	frame_status = pDET->frame_status[snd_ch];
	bit_cnt = pDET->bit_cnt[snd_ch];
	bit_stream = pDET->bit_stream[snd_ch];
	byte_rx = pDET->byte_rx[snd_ch];

	//
	tr = dcd_threshold * dcd_corr;

	if (last)
	{
		// Update DCD status

		if (dcd_hdr_cnt[snd_ch] == 0)
			dcd_on_hdr[snd_ch] = 0;

		dcd_bit_cnt[snd_ch] = 0;
	}

	baudrate = 300;
	div_bit_afc = 1.0f / round(BIT_AFC*(RX_Samplerate / 11025));
	x = baudrate / RX_Samplerate;

//	max_cnt = round(RX_Samplerate / baudrate) + 1;
	max_cnt = round(RX_Samplerate / baudrate) + 1;

	for (i = 0; i < buf_size - 1; i++)
	{
		// AGC
		amp = sqrt(srcI[i] * srcI[i] + srcQ[i] * srcQ[i]);

		if (amp > PSK_AGC)

			PSK_AGC = PSK_AGC * agc_fast1 + amp*agc_fast;
		else
			PSK_AGC = PSK_AGC * agc_slow1 + amp*agc_slow;

		if (PSK_AGC > 1)

		{
			srcI[i] = srcI[i] / PSK_AGC;
			srcQ[i] = srcQ[i] / PSK_AGC;
			amp = amp / PSK_AGC; // Вместо SQRT
		}
		//
		bit_buf[sample_cnt] = 0.95*bit_buf[sample_cnt] + 0.05*amp;
		// Находим максимум в буфере синхронизации
		if (bit_buf[sample_cnt] > PkAmpMax)
		{
			PkAmpI = srcI[i];
			PkAmpQ = srcQ[i];
			PkAmpMax = bit_buf[sample_cnt];
			newpkpos = sample_cnt;
		}
	
		sample_cnt++;

		bit_osc = bit_osc + x;

		if (bit_osc >= 1)
		{
			if (sample_cnt <= max_cnt)
				for (k = sample_cnt; k <= max_cnt; k++)
					bit_buf[k] = 0.95*bit_buf[k];

			k1 = (1.0f * newpkpos) / (sample_cnt - 1);
			k2 = pila(k1) - 1;

			afc = div_bit_afc * k2;

			if (k1 > 0.5)
				bit_osc = bit_osc + afc;
			else
				bit_osc = bit_osc - afc;

			PkAmpMax = 0;
			sample_cnt = 0;
			bit_osc = bit_osc - 1;

			//DCD feature
			if (last)
			{
				DCD_LastPkPos[snd_ch] = DCD_LastPkPos[snd_ch] * 0.96 + newpkpos * 0.04;
				DCD_LastPerc[snd_ch] = DCD_LastPerc[snd_ch] * 0.96 + fabsf(newpkpos - DCD_LastPkPos[snd_ch])*0.04;

				if (DCD_LastPerc[snd_ch] >= tr || DCD_LastPerc[snd_ch] < 0.00001)
					dcd_bit_cnt[snd_ch] = dcd_bit_cnt[snd_ch] + 1;
				else
					dcd_bit_cnt[snd_ch] = dcd_bit_cnt[snd_ch] - 1;
			}

			// Bit-detector

			muxI1 = PkAmpI * PSK_IZ1;
			muxI2 = PkAmpQ * PSK_IZ1;
			muxQ1 = PkAmpQ * PSK_QZ1;
			muxQ2 = PkAmpI * PSK_QZ1;
			sumIQ1 = muxI1 + muxQ1;
			sumIQ2 = muxI2 - muxQ2;
			angle = atan2f(sumIQ2, sumIQ1);
			PSK_IZ1 = PkAmpI;
			PSK_QZ1 = PkAmpQ;
			// Phase corrector

			if (fabsf(angle) < PI5)
				AngleCorr = AngleCorr * 0.95 - angle * 0.05;
			else
			{
				if (angle > 0)
					AngleCorr = AngleCorr * 0.95 + (pi - angle)*0.05;
				else
					AngleCorr = AngleCorr * 0.95 - (pi + angle)*0.05;
			}

			angle = angle + AngleCorr;
			//

			if (fabsf(angle) < PI5)
				bit = RX_BIT1;
			else
				bit = RX_BIT0;
			//

			if (bit)
				stats[1]++;
			else
				stats[0]++;

			bit_stream = (bit_stream >> 1) | bit;

			// DCD on flag

			if (last)
			{
				if (dcd_hdr_cnt[snd_ch] > 0)
					dcd_hdr_cnt[snd_ch]--;

				DCD_header[snd_ch] = (DCD_header[snd_ch] >> 1) | (bit << 24);

				if (((DCD_header[snd_ch] & 0xFFFF0000) == 0x7E7E0000) ||
					((DCD_header[snd_ch] & 0xFFFFFF00) == 0x7E000000) ||
					((DCD_header[snd_ch] & 0xFFFFFF00) == 0x00000000))
				{
					dcd_hdr_cnt[snd_ch] = 48;
					dcd_on_hdr[snd_ch] = 1;
				}
			}


			// I think Andy looks for both flag and abort here. I think it would be
			// clearer to detect abort separately

			// This may not be optimun but should work

			if (bit_stream == 0xFF || bit_stream == 0x7F || bit_stream == 0xFE)
			{
				// All have 7 or more 1 bits

				if (frame_status == FRAME_LOAD)
				{
					// Have started receiving frame

//	  				Debugprintf("Frame Abort len= %d bytes", data->Length);

					frame_status = FRAME_WAIT;

					// Raw stream init

					bit_cnt = 0;
					bit_stuff_cnt = 0;
					data->Length = 0;
				}
				continue;
			}


			if ((bit_stream & FRAME_FLAG) == FRAME_FLAG && frame_status == FRAME_LOAD)
			{
				frame_status = FRAME_WAIT;
				//				if (bit_cnt == 6)
				make_rx_frame_PSK(snd_ch, rcvr_nr, emph, data);
			}

			if (frame_status == FRAME_LOAD)
			{
				if (bit_stuff_cnt == 5)
					bit_stuff_cnt = 0;
				else
				{
					if (bit == RX_BIT1)
						bit_stuff_cnt++;
					else
						bit_stuff_cnt = 0;

					byte_rx = (byte_rx >> 1) + bit;
					bit_cnt++;
				}

				if (bit_cnt == 8)
				{
					if (data->Length < 4097)
						stringAdd(data, &byte_rx, 1);
					bit_cnt = 0;
				}
			}
			if ((bit_stream & FRAME_FLAG) == FRAME_FLAG && frame_status == FRAME_WAIT)
			{
				frame_status = FRAME_LOAD;
				bit_cnt = 0;
				bit_stuff_cnt = 0;
				data->Length = 0;
			}
		}
	}

	pDET->sample_cnt[snd_ch] = sample_cnt;
	pDET->PSK_AGC[snd_ch] = PSK_AGC;
	pDET->PkAmpI[snd_ch] = PkAmpI;
	pDET->PkAmpQ[snd_ch] = PkAmpQ;
	pDET->PkAmpMax[snd_ch] = PkAmpMax;
	pDET->newpkpos[snd_ch] = newpkpos;
	pDET->PSK_IZ1[snd_ch] = PSK_IZ1;
	pDET->PSK_QZ1[snd_ch] = PSK_QZ1;
	pDET->bit_osc[snd_ch] = bit_osc;
	pDET->frame_status[snd_ch] = frame_status;
	pDET->bit_cnt[snd_ch] = bit_cnt;
	pDET->bit_stream[snd_ch] = bit_stream;
	pDET->byte_rx[snd_ch] = byte_rx;
	pDET->bit_stuff_cnt[snd_ch] = bit_stuff_cnt;
	pDET->AngleCorr[snd_ch] = AngleCorr;
}


void decode_stream_QPSK(int last, int snd_ch, int rcvr_nr, int emph, float * srcI, float * srcQ, float * bit_buf, int  buf_size, string * data)
{
	float agc_fast = 0.01f;
	float agc_fast1 = 1 - agc_fast;
	float agc_slow = agc_fast / 4;
	float agc_slow1 = 1 - agc_slow;

	int i, k, j, n;
	byte dibit, bit;
	single afc, x, amp, k1, k2;
	single baudrate;
	single div_bit_afc;
	word max_cnt;
	single threshold;
	single tr;
	single KCorr, AngleCorr, angle, muxI1, muxQ1, muxI2, muxQ2, sumIQ1, sumIQ2;
	byte newpkpos, sample_cnt;
	single PkAmpI, PkAmpQ, PkAmpMax, PSK_AGC;
	single PSK_IZ1, PSK_QZ1;
	single bit_osc;
	byte bit_stuff_cnt, last_rx_bit, frame_status, bit_cnt, bit_stream, byte_rx;

	// get saved values to local variables to speed up access

	struct TDetector_t * pDET = &DET[emph][rcvr_nr];


	bit_stuff_cnt = pDET->bit_stuff_cnt[snd_ch];
	last_rx_bit = pDET->last_rx_bit[snd_ch];
	sample_cnt = pDET->sample_cnt[snd_ch];
	PSK_AGC = pDET->PSK_AGC[snd_ch];
	PkAmpI = pDET->PkAmpI[snd_ch];
	PkAmpQ = pDET->PkAmpQ[snd_ch];
	PkAmpMax = pDET->PkAmpMax[snd_ch];
	newpkpos = pDET->newpkpos[snd_ch];
	PSK_IZ1 = pDET->PSK_IZ1[snd_ch];
	PSK_QZ1 = pDET->PSK_QZ1[snd_ch];
	bit_osc = pDET->bit_osc[snd_ch];
	frame_status = pDET->frame_status[snd_ch];
	bit_cnt = pDET->bit_cnt[snd_ch];
	bit_stream = pDET->bit_stream[snd_ch];
	byte_rx = pDET->byte_rx[snd_ch];
	AngleCorr = pDET->AngleCorr[snd_ch];

	tr = dcd_threshold * dcd_corr;

	if (last)
	{
		// Update DCD status

		if (dcd_hdr_cnt[snd_ch] == 0)
			dcd_on_hdr[snd_ch] = 0;

		dcd_bit_cnt[snd_ch] = 0;
	}

	// I think this works because of upsampling - 1200 = 4x 300 and we upsampled by 4

	baudrate = 300;

	div_bit_afc = 1.0f / roundf(BIT_AFC*(RX_Samplerate / 11025.0f));

	x = baudrate / RX_Samplerate;

	max_cnt = roundf(RX_Samplerate / baudrate) + 1;

	for (i = 0; i < buf_size; i++)
	{
		// AGC
		amp = sqrt(srcI[i] * srcI[i] + srcQ[i] * srcQ[i]);

		if (amp > PSK_AGC)
			PSK_AGC = PSK_AGC * agc_fast1 + amp * agc_fast;
		else
			PSK_AGC = PSK_AGC * agc_slow1 + amp * agc_slow;

		if (PSK_AGC > 1)
		{
			srcI[i] = srcI[i] / PSK_AGC;
			srcQ[i] = srcQ[i] / PSK_AGC;

			amp = amp / PSK_AGC; // Вместо SQRT
		}

		bit_buf[sample_cnt] = 0.95 *  bit_buf[sample_cnt] + 0.05 * amp;

		// Check for NAN

		if (bit_buf[sample_cnt] != bit_buf[sample_cnt])
			bit_buf[sample_cnt] = 0.0f;

		// Find the maximum in the synchronization buffer

		if (bit_buf[sample_cnt] > PkAmpMax)
		{
			PkAmpI = srcI[i];
			PkAmpQ = srcQ[i];
			PkAmpMax = bit_buf[sample_cnt];
			newpkpos = sample_cnt;
		}

		sample_cnt++;

		bit_osc = bit_osc + x;

		// This seems to be how it does samples to bits

		if (bit_osc >= 1)
		{
			if (sample_cnt <= max_cnt)
				for (k = sample_cnt; k <= max_cnt; k++)
					bit_buf[k] = 0.95*bit_buf[k];

			k1 = (1.0f * newpkpos) / (sample_cnt - 1);
			k2 = pila(k1) - 1;
			afc = div_bit_afc * k2;

			if (k1 > 0.5)
				bit_osc = bit_osc + afc;
			else
				bit_osc = bit_osc - afc;

			PkAmpMax = 0;
			sample_cnt = 0;
			bit_osc = bit_osc - 1;
			//DCD feature

			if (last)
			{
				DCD_LastPkPos[snd_ch] = DCD_LastPkPos[snd_ch] * 0.96 + newpkpos * 0.04;
				DCD_LastPerc[snd_ch] = DCD_LastPerc[snd_ch] * 0.96 + fabsf(newpkpos - DCD_LastPkPos[snd_ch])*0.04;

				if (DCD_LastPerc[snd_ch] >= tr || DCD_LastPerc[snd_ch] < 0.00001f)
					dcd_bit_cnt[snd_ch] = dcd_bit_cnt[snd_ch] + 1;
				else
					dcd_bit_cnt[snd_ch] = dcd_bit_cnt[snd_ch] - 1;
			}

			// Bit-detector

			muxI1 = PkAmpI * PSK_IZ1;
			muxI2 = PkAmpQ * PSK_IZ1;
			muxQ1 = PkAmpQ * PSK_QZ1;
			muxQ2 = PkAmpI * PSK_QZ1;
			sumIQ1 = muxI1 + muxQ1;
			sumIQ2 = muxI2 - muxQ2;
			angle = atan2f(sumIQ2, sumIQ1);
			PSK_IZ1 = PkAmpI;
			PSK_QZ1 = PkAmpQ;

			if (angle > pi || angle < -pi)
				angle = angle;

			if (modem_mode[snd_ch] == MODE_PI4QPSK)
			{
				// Phase corrector

				// I'm pretty sure we send 4 phases starting 45 degrees from 0 so .25, .75 - .25 - .75

				if (angle >= 0 && angle <= PI5)
					KCorr = angle - PI25;

				if (angle > PI5)
					KCorr = angle - PI75;

				if (angle < -PI5)
					KCorr = angle + PI75;

				if (angle < 0 && angle >= -PI5)
					KCorr = angle + PI25;

				AngleCorr = AngleCorr * 0.95f - KCorr * 0.05f;
				angle = angle + AngleCorr;

				if (angle >= 0 && angle <= PI5)
				{
					dibit = qpsk_set[snd_ch].rx[0];		// 00 - 0
					qpsk_set[snd_ch].count[0]++;
				}
				else if (angle > PI5)
				{
					dibit = qpsk_set[snd_ch].rx[1];		// 01 - PI/2
					qpsk_set[snd_ch].count[1]++;
				}
				else if (angle < -PI5)
				{
					dibit = qpsk_set[snd_ch].rx[2];		// 10 - PI
					qpsk_set[snd_ch].count[2]++;
				}
				else if (angle < 0 && angle >= -PI5)
				{
					dibit = qpsk_set[snd_ch].rx[3];		// 11 - -PI/2
					qpsk_set[snd_ch].count[3]++;
				}
			}
			else
			{
				// Phase corrector

				// I think this sends 0 90 180 270

				if (fabsf(angle) < PI25)
					KCorr = angle;

				if (angle >= PI25 && angle <= PI75)
					KCorr = angle - PI5;

				if (angle <= -PI25 && angle >= -PI75)
					KCorr = angle + PI5;

				if (fabsf(angle) > PI75)
				{
					if (angle > 0)
						KCorr = -M_PI + angle;
					else
						KCorr = M_PI + angle;
				}

				AngleCorr = AngleCorr * 0.95 - KCorr * 0.05;
				angle = angle + AngleCorr;


				if (fabsf(angle) < PI25)
					dibit = qpsk_set[snd_ch].rx[0];							// 00 - 0

				if (angle >= PI25 && angle <= PI75)
					dibit = qpsk_set[snd_ch].rx[1];				// 01 - PI/2

				if (fabsf(angle) > PI75)
					dibit = qpsk_set[snd_ch].rx[2];					// 10 - PI

				if (angle <= -PI25 && angle >= -PI75)
					dibit = qpsk_set[snd_ch].rx[3];					// 11 - -PI/2

			}

			for (j = 0; j < 2; j++)
			{
				dibit = dibit << 1;

				// NRZI

				if (last_rx_bit == (dibit & RX_BIT1))
					bit = RX_BIT1;
				else
					bit = RX_BIT0;

				last_rx_bit = dibit & RX_BIT1;
			
				bit_stream = (bit_stream >> 1) | bit;

				// DCD on flag

				if (last)
				{
					if (dcd_hdr_cnt[snd_ch] > 0)
						dcd_hdr_cnt[snd_ch]--;

					DCD_header[snd_ch] = (DCD_header[snd_ch] >> 1) | (bit << 24);

					if (((DCD_header[snd_ch] & 0xFFFF0000) == 0x7E7E0000) ||
						((DCD_header[snd_ch] & 0xFFFFFF00) == 0x7E000000) ||
						((DCD_header[snd_ch] & 0xFFFFFF00) == 0x00000000))
					{
						dcd_hdr_cnt[snd_ch] = 48;
						dcd_on_hdr[snd_ch] = 1;
					}
				}


				// I think Andy looks for both flag and abort here. I think it would be
				// clearer to detect abort separately

				// This may not be optimun but should work

				if (bit_stream == 0xFF || bit_stream == 0x7F || bit_stream == 0xFE)
				{
					// All have 7 or more 1 bits

					if (frame_status == FRAME_LOAD)
					{
						// Have started receiving frame

	//					Debugprintf("Frame Abort len= %d bytes", data->Length);

						frame_status = FRAME_WAIT;

						// Raw stream init

						bit_cnt = 0;
						bit_stuff_cnt = 0;
						data->Length = 0;
					}
					continue;
				}

				if ((bit_stream & FRAME_FLAG) == FRAME_FLAG && frame_status == FRAME_LOAD)
				{
					frame_status = FRAME_WAIT;
					//				if (bit_cnt == 6)
					make_rx_frame_PSK(snd_ch, rcvr_nr, emph, data);
				}

				if (frame_status == FRAME_LOAD)
				{
					if (bit_stuff_cnt == 5)
						bit_stuff_cnt = 0;
					else
					{
						if (bit == RX_BIT1)
							bit_stuff_cnt++;
						else
							bit_stuff_cnt = 0;

						byte_rx = (byte_rx >> 1) + bit;
						bit_cnt++;

					}

					if (bit_cnt == 8)
					{
						if (data->Length < 4097)
							stringAdd(data, &byte_rx, 1);
						bit_cnt = 0;
					}
				}
				if ((bit_stream & FRAME_FLAG) == FRAME_FLAG && frame_status == FRAME_WAIT)
				{
					frame_status = FRAME_LOAD;
					bit_cnt = 0;
					bit_stuff_cnt = 0;
					data->Length = 0;
				}
			}
		}
	}

	pDET->sample_cnt[snd_ch] = sample_cnt;
	pDET->PSK_AGC[snd_ch] = PSK_AGC;
	pDET->PkAmpI[snd_ch] = PkAmpI;
	pDET->PkAmpQ[snd_ch] = PkAmpQ;
	pDET->PkAmpMax[snd_ch] = PkAmpMax;
	pDET->newpkpos[snd_ch] = newpkpos;
	pDET->PSK_IZ1[snd_ch] = PSK_IZ1;
	pDET->PSK_QZ1[snd_ch] = PSK_QZ1;
	pDET->bit_osc[snd_ch] = bit_osc;
	pDET->frame_status[snd_ch] = frame_status;
	pDET->bit_cnt[snd_ch] = bit_cnt;
	pDET->bit_stream[snd_ch] = bit_stream;
	pDET->byte_rx[snd_ch] = byte_rx;
	pDET->last_rx_bit[snd_ch] = last_rx_bit;
	pDET->bit_stuff_cnt[snd_ch] = bit_stuff_cnt;
	pDET->AngleCorr[snd_ch] = AngleCorr;
}

/*

procedure decode_stream_8PSK(last: boolean; snd_ch,rcvr_nr,emph: byte; var srcI,srcQ,bit_buf: array of single; buf_size: word; var data: string);
const
  dcd_corr=0.11111;
  agc_fast=0.01;
  agc_fast1=1-agc_fast;
  agc_slow=agc_fast/4;
  agc_slow1=1-agc_slow;
var
  i,k,j,n: word;
  tribit,bit: byte;
  afc,x,amp,k1,k2: single;
  baudrate: single;
  div_bit_afc: single;
  max_cnt: word;
  threshold: single;
  tr: single;
  KCorr,AngleCorr,angle,muxI1,muxQ1,muxI2,muxQ2,sumIQ1,sumIQ2: single;
  newpkpos,sample_cnt: byte;
  PkAmpI,PkAmpQ,PkAmpMax,PSK_AGC: single;
  PSK_IZ1,PSK_QZ1: single;
  bit_osc: single;
  bit_stuff_cnt,last_rx_bit,frame_status,bit_cnt,bit_stream,byte_rx: byte;
{
  // global -> local
  bit_stuff_cnt = DET[emph,rcvr_nr].bit_stuff_cnt[snd_ch];
  last_rx_bit = DET[emph,rcvr_nr].last_rx_bit[snd_ch];
  sample_cnt = DET[emph,rcvr_nr].sample_cnt[snd_ch];
  PSK_AGC = DET[emph,rcvr_nr].PSK_AGC[snd_ch];
  PkAmpI = DET[emph,rcvr_nr].PkAmpI[snd_ch];
  PkAmpQ = DET[emph,rcvr_nr].PkAmpQ[snd_ch];
  PkAmpMax = DET[emph,rcvr_nr].PkAmpMax[snd_ch];
  newpkpos = DET[emph,rcvr_nr].newpkpos[snd_ch];
  PSK_IZ1 = DET[emph,rcvr_nr].PSK_IZ1[snd_ch];
  PSK_QZ1 = DET[emph,rcvr_nr].PSK_QZ1[snd_ch];
  bit_osc = DET[emph,rcvr_nr].bit_osc[snd_ch];
  frame_status = DET[emph,rcvr_nr].frame_status[snd_ch];
  bit_cnt = DET[emph,rcvr_nr].bit_cnt[snd_ch];
  bit_stream = DET[emph,rcvr_nr].bit_stream[snd_ch];
  byte_rx = DET[emph,rcvr_nr].byte_rx[snd_ch];
  AngleCorr = DET[emph,rcvr_nr].AngleCorr[snd_ch];
  //
  tr = dcd_threshold*dcd_corr;
  if last then
  {
    if (dcd_hdr_cnt[snd_ch]=0) then dcd_on_hdr[snd_ch] = FALSE;
    dcd_bit_cnt[snd_ch] = 0;
  }
  baudrate = 1600/6;
  div_bit_afc = 1/round(BIT_AFC*(RX_Samplerate/11025));
  x = baudrate/RX_Samplerate;
  max_cnt = round(RX_Samplerate/baudrate)+1;
  for i = 0 to buf_size-1 do
  {
    // AGC
    amp = sqrt(srcI[i]*srcI[i]+srcQ[i]*srcQ[i]);
    if amp>PSK_AGC then
      PSK_AGC = PSK_AGC*agc_fast1+amp*agc_fast
    else
      PSK_AGC = PSK_AGC*agc_slow1+amp*agc_slow;
    if PSK_AGC>1 then
    {
      srcI[i] = srcI[i]/PSK_AGC;
      srcQ[i] = srcQ[i]/PSK_AGC;
      amp = amp/PSK_AGC; // Вместо SQRT
    }
    //
    bit_buf[sample_cnt] = 0.95*bit_buf[sample_cnt]+0.05*amp;
    // Находим максимум в буфере синхронизации
    if bit_buf[sample_cnt]>PkAmpMax then
    {
      PkAmpI = srcI[i];
      PkAmpQ = srcQ[i];
      PkAmpMax = bit_buf[sample_cnt];
      newpkpos = sample_cnt;
    }
    //
    inc(sample_cnt);
    bit_osc = bit_osc+x;
    if bit_osc>=1 then
    {
      if sample_cnt<=max_cnt then
        for k = sample_cnt to max_cnt do
           bit_buf[k] = 0.95*bit_buf[k];
      k1 = newpkpos/(sample_cnt-1);
      k2 = pila(k1)-1;
      AFC = div_bit_afc*k2;
      if k1>0.5 then bit_osc = bit_osc+AFC else bit_osc = bit_osc-AFC;
      PkAmpMax = 0;
      sample_cnt = 0;
      bit_osc = bit_osc-1;
      //DCD feature
      if last then
      {
        DCD_LastPkPos[snd_ch] = DCD_LastPkPos[snd_ch]*0.96+newpkpos*0.04;
        DCD_LastPerc[snd_ch] = DCD_LastPerc[snd_ch]*0.96+abs(newpkpos-DCD_LastPkPos[snd_ch])*0.04;
        if (DCD_LastPerc[snd_ch]>=tr) or (DCD_LastPerc[snd_ch]<0.00001) then
          dcd_bit_cnt[snd_ch] = dcd_bit_cnt[snd_ch]+1
        else
          dcd_bit_cnt[snd_ch] = dcd_bit_cnt[snd_ch]-1;
      }
      // Bit-detector
      muxI1 = PkAmpI*PSK_IZ1;
      muxI2 = PkAmpQ*PSK_IZ1;
      muxQ1 = PkAmpQ*PSK_QZ1;
      muxQ2 = PkAmpI*PSK_QZ1;
      sumIQ1 = muxI1+muxQ1;
      sumIQ2 = muxI2-muxQ2;
      angle = arctan2(sumIQ2,sumIQ1);
      PSK_IZ1 = PkAmpI;
      PSK_QZ1 = PkAmpQ;
      // Phase corrector
      if abs(angle)<PI125 then KCorr = angle;
      if (angle>=PI125) and (angle<=PI375) then KCorr = angle-PI25;
      if (angle>=PI375) and (angle<PI625) then KCorr = angle-PI5;
      if (angle>=PI625) and (angle<=PI875) then KCorr = angle-PI75;
      if (angle<=-PI125) and (angle>-PI375) then KCorr = angle+PI25;
      if (angle<=-PI375) and (angle>-PI625) then KCorr = angle+PI5;
      if (angle<=-PI625) and (angle>=-PI875) then KCorr = angle+PI75;
      if abs(angle)>PI875 then
      {
        if angle>0 then KCorr = angle-PI else KCorr = angle+PI;
      }
      AngleCorr = AngleCorr*0.95-KCorr*0.05;
      angle = angle+AngleCorr;
      //
      if abs(angle)<PI125 then tribit = 1;
      if (angle>=PI125) and (angle<PI375) then tribit = 0;
      if (angle>=PI375) and (angle<PI625) then tribit = 2;
      if (angle>=PI625) and (angle<=PI875) then tribit = 3;
      if abs(angle)>PI875 then tribit = 7;
      if (angle<=-PI625) and (angle>=-PI875) then tribit = 6;
      if (angle<=-PI375) and (angle>-PI625) then tribit = 4;
      if (angle<=-PI125) and (angle>-PI375) then tribit = 5;
      //if snd_ch=2 then if length(form1.Memo1.text)<20000 then form1.Memo1.SelText = inttostr(tribit);
      tribit = tribit shl 4;
      for j = 0 to 2 do
      {
        tribit = tribit shl 1;
        //NRZI
        if last_rx_bit=tribit and RX_BIT1 then bit = RX_BIT1 else bit = RX_BIT0;
        last_rx_bit = tribit and RX_BIT1;
        //
        bit_stream = (bit_stream shr 1) or bit;
        // DCD on flag
        if last then
        {
          if dcd_hdr_cnt[snd_ch]>0 then dec(dcd_hdr_cnt[snd_ch]);
          DCD_header[snd_ch] = (DCD_header[snd_ch] shr 1) or (bit shl 24);
          if
            ((DCD_header[snd_ch] and $FFFF0000)=$7E7E0000) or
            ((DCD_header[snd_ch] and $FFFFFF00)=$7E000000) or
            ((DCD_header[snd_ch] and $FFFFFF00)=$00000000) then
          { dcd_hdr_cnt[snd_ch] = 48; dcd_on_hdr[snd_ch] = TRUE; }
        }
        //
        if (bit_stream and FRAME_FLAG=FRAME_FLAG) and (frame_status=FRAME_LOAD) then
        {
          frame_status = FRAME_WAIT;
          if bit_cnt=6 then make_rx_frame_PSK(snd_ch,rcvr_nr,emph,data);
        }
        if (frame_status=FRAME_LOAD) then
        {
          if bit_stuff_cnt=5 then bit_stuff_cnt = 0
          else
          {
            if bit=RX_BIT1 then inc(bit_stuff_cnt) else bit_stuff_cnt = 0;
            byte_rx = (byte_rx shr 1)+bit;
            inc(bit_cnt);
          }
          if bit_cnt=8 then
          {
            if length(data)<4097 then data = data+chr(byte_rx);
            bit_cnt = 0;
          }
        }
        if (bit_stream and FRAME_FLAG=FRAME_FLAG) and (frame_status=FRAME_WAIT) then
        {
          frame_status = FRAME_LOAD;
          bit_cnt = 0;
          bit_stuff_cnt = 0;
          data = '';
        }
      }
    }
  }
  DET[emph,rcvr_nr].sample_cnt[snd_ch] = sample_cnt;
  DET[emph,rcvr_nr].PSK_AGC[snd_ch] = PSK_AGC;
  DET[emph,rcvr_nr].PkAmpI[snd_ch] = PkAmpI;
  DET[emph,rcvr_nr].PkAmpQ[snd_ch] = PkAmpQ;
  DET[emph,rcvr_nr].PkAmpMax[snd_ch] = PkAmpMax;
  DET[emph,rcvr_nr].newpkpos[snd_ch] = newpkpos;
  DET[emph,rcvr_nr].PSK_IZ1[snd_ch] = PSK_IZ1;
  DET[emph,rcvr_nr].PSK_QZ1[snd_ch] = PSK_QZ1;
  DET[emph,rcvr_nr].bit_osc[snd_ch] = bit_osc;
  DET[emph,rcvr_nr].frame_status[snd_ch] = frame_status;
  DET[emph,rcvr_nr].bit_cnt[snd_ch] = bit_cnt;
  DET[emph,rcvr_nr].bit_stream[snd_ch] = bit_stream;
  DET[emph,rcvr_nr].byte_rx[snd_ch] = byte_rx;
  DET[emph,rcvr_nr].last_rx_bit[snd_ch] = last_rx_bit;
  DET[emph,rcvr_nr].bit_stuff_cnt[snd_ch] = bit_stuff_cnt;
  DET[emph,rcvr_nr].AngleCorr[snd_ch] = AngleCorr;
}

////////////////////////////////////////////////////////

function blackman(i,tap: word): single;
var
  a0,a1,a2,a: single;
{
  a = 0.16;
  a0 = (1-a)/2;
  a1 = 1/2;
  a2 = a/2;
  result = a0-a1*cos(2*pi*i/(tap-1))+a2*cos(4*pi*i/(tap-1));
}

function nuttal(i,tap: word): single;
var
  a0,a1,a2,a3: single;
{
  a0 = 0.355768;
  a1 = 0.487396;
  a2 = 0.144232;
  a3 = 0.012604;
  result = a0-a1*cos(2*pi*i/(tap-1))+a2*cos(4*pi*i/(tap-1))-a3*cos(6*pi*i/(tap-1));
}

function flattop(i,tap: word): single;
var
  a0,a1,a2,a3,a4: single;
{
  a0 = 1;
  a1 = 1.93;
  a2 = 1.29;
  a3 = 0.388;
  a4 = 0.032;
  result = a0-a1*cos(2*pi*i/(tap-1))+a2*cos(4*pi*i/(tap-1))-a3*cos(6*pi*i/(tap-1))+a4*cos(8*pi*i/(tap-1));
}
*/


void init_BPF(float freq1, float freq2, unsigned short tap, float samplerate, float * buf)
{
	unsigned short tap1, i;
	float tap12, ham, acc1, acc2;
	float bpf_l[2048];
	float bpf_h[2048];
	float itap12, pi2, x1, x2;

	acc1 = 0;
	acc2 = 0;
	tap1 = tap - 1;
	tap12 = tap1 / 2;
	pi2 = 2 * pi;
	x1 = pi2 * freq1 / samplerate;
	x2 = pi2 * freq2 / samplerate;
	for (i = 0; i <= tap1; i++)
	{
//		float x = (pi2 * i) / tap1;
//		x = cosf(x);
//		ham = 0.5 - 0.5 * x;

		ham = 0.5 - 0.5 * cosf((pi2 * i) / tap1); //old

		if (ham != ham)			// check for NaN
			ham = 0.0f;

		itap12 = i - tap12;

		if (itap12 == 0)
		{
			bpf_l[i] = x1;
			bpf_h[i] = x2;
		}
		else
		{
			bpf_l[i] = sinf(x1*itap12) / itap12;
			bpf_h[i] = sinf(x2*itap12) / itap12;
		}

		bpf_l[i] = bpf_l[i] * ham;
		bpf_h[i] = bpf_h[i] * ham;
		acc1 = acc1 + bpf_l[i];
		acc2 = acc2 + bpf_h[i];
	}

	for (i = 0; i <= tap1; i++)
	{
		bpf_l[i] = bpf_l[i] / acc1;
		bpf_h[i] = -(bpf_h[i] / acc2);
	};

	bpf_h[tap / 2] = bpf_h[tap / 2] + 1;

	for (i = 0; i <= tap; i++)
	{
		buf[i] = -(bpf_l[i] + bpf_h[i]);
	}
	buf[tap / 2] = buf[tap / 2] + 1;
}



void  init_LPF(float width, unsigned short tap, float samplerate, float * buf)
{
	float acc1, ham;
	unsigned short tap1, i;
	float itap12, tap12, x1, pi2;

	acc1 = 0;
	tap1 = tap - 1;
	tap12 = tap1 / 2;
	pi2 = 2 * pi;
	x1 = pi2 * width / samplerate;

	for (i = 0; i <= tap1; i++)
	{
		ham = 0.53836f - 0.46164f * cosf(pi2 * i / tap1); //old

		if (ham != ham)			// check for NaN
			ham = 0.0f;

	   //ham = 0.5-0.5*cos(pi2*i/tap1);
		//ham = 0.5*(1-cos(pi2*i/tap1)); //hann

		//ham = blackman(i,tap); //blackman
		//ham = nuttal(i,tap);

		itap12 = i - tap12;

		if (itap12 == 0)
			buf[i] = x1;
		else
			buf[i] = sinf(x1*itap12) / itap12;

		buf[i] = buf[i] * ham;
		acc1 = acc1 + buf[i];
	}
	for (i = 0; i <= tap1; i++)
		buf[i] = buf[i] / acc1;
}

void make_core_INTR(UCHAR snd_ch)
{
	float width;

	width = roundf(RX_Samplerate / 2);

	n_INTR[snd_ch] = 1;

	switch (speed[snd_ch])
	{
	case SPEED_300:

		width = roundf(RX_Samplerate / 2);
		n_INTR[snd_ch] = 1;
		break;

	case SPEED_P300:

		width = roundf(RX_Samplerate / 2);
		n_INTR[snd_ch] = 1;
		break;

	case SPEED_600:

		width = roundf(RX_Samplerate / 4);
		n_INTR[snd_ch] = 2;
		break;

	case SPEED_P600:

		width = roundf(RX_Samplerate / 4);
		n_INTR[snd_ch] = 2;
		break;

	case SPEED_1200:
		width = roundf(RX_Samplerate / 8);
		n_INTR[snd_ch] = 4;
		break;

	case SPEED_P1200:
		width = roundf(RX_Samplerate / 8);
		n_INTR[snd_ch] = 4;
		break;

	case SPEED_Q2400:
		width = 300;
		n_INTR[snd_ch] = 4;
		break; //8

	case SPEED_DW2400:

		width = 300;
		n_INTR[snd_ch] = 4;
		break;

	case SPEED_AE2400:

		width = 300;
		n_INTR[snd_ch] = 4;
		break;

	case SPEED_MP400:

		width = round(RX_Samplerate / 8);
		n_INTR[snd_ch] = 4;
		break;

	case SPEED_Q3600:
		width = 300;
		n_INTR[snd_ch] = 6;//12
		break;

	case SPEED_8P4800:
		width = 100;
		n_INTR[snd_ch] = 6;
		break;

	case SPEED_2400:

		width = round(RX_Samplerate / 16);
		n_INTR[snd_ch] = 8;
		break;

	case SPEED_P2400:
		width = round(RX_Samplerate / 16);
		n_INTR[snd_ch] = 8;
		break;

	case SPEED_Q4800:
		width = 300;
		n_INTR[snd_ch] = 8;//16
		break;
	}


	init_LPF(width, INTR_tap[snd_ch], RX_Samplerate, INTR_core[snd_ch]);
}

void  make_core_LPF(UCHAR snd_ch, short width)
{
	if (modem_mode[snd_ch] == MODE_MPSK)
	{
		init_LPF(width, LPF_tap[snd_ch], RX_Samplerate / n_INTR[snd_ch], LPF_core[snd_ch]);
		init_LPF(rx_baudrate[snd_ch], LPF_tap[snd_ch], RX_Samplerate / n_INTR[snd_ch], AFC_core[snd_ch]);
	}
	else
		init_LPF(width, LPF_tap[snd_ch], RX_Samplerate, LPF_core[snd_ch]);
}


void  make_core_BPF(UCHAR snd_ch, short freq, short width)
{
	float old_freq, width2, rx_samplerate2, freq1, freq2;

	UCHAR i;

	rx_samplerate2 = 0.5*RX_Samplerate;
	width2 = 0.5*width;
	old_freq = freq;
	for (i = 0; i <= RCVR[snd_ch] << 1; i++)
	{
		if (i > 0)
		{
			if (i > RCVR[snd_ch])
				freq = old_freq - rcvr_offset[snd_ch] * (i - RCVR[snd_ch]);
			else
				freq = old_freq + rcvr_offset[snd_ch] * i;
		}

		freq1 = freq - width2;
		freq2 = freq + width2;
		if (freq1 < 1)
			freq1 = 1;

		if (freq2 < 1)
			freq2 = 1;

		if (freq1 > rx_samplerate2)
			freq1 = rx_samplerate2;

		if (freq2 > rx_samplerate2)
			freq2 = rx_samplerate2;

		init_BPF(freq1, freq2, BPF_tap[snd_ch], RX_Samplerate, &DET[0][i].BPF_core[snd_ch][0]);
	}
}



void make_core_TXBPF(UCHAR snd_ch, float freq, float width)
{
	float freq1, freq2;

	freq1 = freq - width / 2;
	freq2 = freq + width / 2;

	if (freq1 < 1)
		freq1 = 1;

	if (freq2 < 1)
		freq2 = 1;

	if (freq1 > TX_Samplerate / 2)
		freq1 = TX_Samplerate / 2;

	if (freq2 > TX_Samplerate / 2)
		freq2 = TX_Samplerate / 2;

	init_BPF(freq1, freq2, tx_BPF_tap[snd_ch], TX_Samplerate, tx_BPF_core[snd_ch]);
}




void interpolation(int snd_ch, int rcvr_nr, int emph, float * dest_buf, float * src_buf, int buf_size)
{
	int n_intr1, buf_size1, k, i, j;
	float buf[8192];

	buf_size1 = buf_size - 1;
	n_intr1 = n_INTR[snd_ch] - 1;
	k = 0;

	for (i = 0; i <= buf_size1; i++)
	{
		for (j = 0; j <= n_intr1; j++)
		{
			buf[k] = src_buf[i];
			k++;
	   }
	}
	FIR_filter(buf, buf_size *n_INTR[snd_ch], INTR_tap[snd_ch], INTR_core[snd_ch], dest_buf, DET[emph][rcvr_nr].prev_INTR_buf[snd_ch]);
}

void interpolation_PSK(int snd_ch, int rcvr_nr, int emph, float * destI, float * destQ, float * srcI, float * srcQ, int buf_size)
{
	word n_intr1, buf_size1, k, i, j;
	single bufI[8192], bufQ[8192];

	buf_size1 = buf_size - 1;
	n_intr1 = n_INTR[snd_ch] - 1;

	k = 0;

	for (i = 0; i <= buf_size1; i++)
	{
		for (j = 0; j <= n_intr1; j++)
		{
			bufI[k] = srcI[i];
			bufQ[k] = srcQ[i];
			k++;
		}
	}

	FIR_filter(bufI, buf_size*n_INTR[snd_ch], INTR_tap[snd_ch], INTR_core[snd_ch], destI, DET[emph][rcvr_nr].prev_INTRI_buf[snd_ch]);
	FIR_filter(bufQ, buf_size*n_INTR[snd_ch], INTR_tap[snd_ch], INTR_core[snd_ch], destQ, DET[emph][rcvr_nr].prev_INTRQ_buf[snd_ch]);
}


void FSK_Demodulator(int snd_ch, int rcvr_nr, int emph, int last)
{
	// filtered samples in src_BPF_buf, output in src_Loop_buf

	Mux3(snd_ch,rcvr_nr,emph, &DET[0][rcvr_nr].src_BPF_buf[snd_ch][0], &LPF_core[snd_ch][0], &DET[emph][rcvr_nr].src_Loop_buf[snd_ch][0],
		&DET[emph][rcvr_nr].prev_LPF1I_buf[snd_ch][0], &DET[emph][rcvr_nr].prev_LPF1Q_buf[snd_ch][0], LPF_tap[snd_ch], rx_bufsize);
  
  if (n_INTR[snd_ch] > 1)
  {
	  interpolation(snd_ch, rcvr_nr, emph, DET[emph][rcvr_nr].src_INTR_buf[snd_ch], DET[emph][rcvr_nr].src_Loop_buf[snd_ch], rx_bufsize);
	  decode_stream_FSK(last, snd_ch, rcvr_nr, emph, &DET[emph][rcvr_nr].src_INTR_buf[snd_ch][0], &DET[emph][rcvr_nr].bit_buf[snd_ch][0], rx_bufsize*n_INTR[snd_ch], &DET[emph][rcvr_nr].rx_data[snd_ch]);
  }
  else 
	  decode_stream_FSK(last,snd_ch,rcvr_nr,emph,DET[emph][rcvr_nr].src_Loop_buf[snd_ch], &DET[emph][rcvr_nr].bit_buf[snd_ch][0], rx_bufsize, &DET[emph][rcvr_nr].rx_data[snd_ch]);
}

void  BPSK_Demodulator(int snd_ch, int rcvr_nr, int emph, int last)
{
	Mux3_PSK(snd_ch, rcvr_nr, emph,
		DET[0][rcvr_nr].src_BPF_buf[snd_ch],
		LPF_core[snd_ch],
		DET[emph][rcvr_nr].src_LPF1I_buf[snd_ch],
		DET[emph][rcvr_nr].src_LPF1Q_buf[snd_ch],
		DET[emph][rcvr_nr].prev_LPF1I_buf[snd_ch],
		DET[emph][rcvr_nr].prev_LPF1Q_buf[snd_ch],
		LPF_tap[snd_ch], rx_bufsize);

	if (n_INTR[snd_ch] > 1)
	{
		interpolation_PSK(snd_ch, rcvr_nr, emph,
			DET[emph][rcvr_nr].src_INTRI_buf[snd_ch],
			DET[emph][rcvr_nr].src_INTRQ_buf[snd_ch],
			DET[emph][rcvr_nr].src_LPF1I_buf[snd_ch],
			DET[emph][rcvr_nr].src_LPF1Q_buf[snd_ch], rx_bufsize);

		decode_stream_BPSK(last, snd_ch, rcvr_nr, emph,
			DET[emph][rcvr_nr].src_INTRI_buf[snd_ch],
			DET[emph][rcvr_nr].src_INTRQ_buf[snd_ch],
			DET[emph][rcvr_nr].bit_buf[snd_ch],
			rx_bufsize*n_INTR[snd_ch],
			&DET[emph][rcvr_nr].rx_data[snd_ch]);

	}
	else
		decode_stream_BPSK(last, snd_ch, rcvr_nr, emph,
			DET[emph][rcvr_nr].src_LPF1I_buf[snd_ch],
			DET[emph][rcvr_nr].src_LPF1Q_buf[snd_ch],
			DET[emph][rcvr_nr].bit_buf[snd_ch],
			rx_bufsize,
			&DET[emph][rcvr_nr].rx_data[snd_ch]);
}

void  QPSK_Demodulator(int snd_ch, int rcvr_nr, int emph, int last)
{
	Mux3_PSK(snd_ch, rcvr_nr, emph,
		DET[0][rcvr_nr].src_BPF_buf[snd_ch],
		LPF_core[snd_ch],
		DET[emph][rcvr_nr].src_LPF1I_buf[snd_ch],
		DET[emph][rcvr_nr].src_LPF1Q_buf[snd_ch],
		DET[emph][rcvr_nr].prev_LPF1I_buf[snd_ch],
		DET[emph][rcvr_nr].prev_LPF1Q_buf[snd_ch],
		LPF_tap[snd_ch], rx_bufsize);

	if (n_INTR[snd_ch] > 1)
	{
		interpolation_PSK(snd_ch, rcvr_nr, emph,
			DET[emph][rcvr_nr].src_INTRI_buf[snd_ch],
			DET[emph][rcvr_nr].src_INTRQ_buf[snd_ch],
			DET[emph][rcvr_nr].src_LPF1I_buf[snd_ch],
			DET[emph][rcvr_nr].src_LPF1Q_buf[snd_ch], rx_bufsize);

		decode_stream_QPSK(last, snd_ch, rcvr_nr, emph,
			DET[emph][rcvr_nr].src_INTRI_buf[snd_ch],
			DET[emph][rcvr_nr].src_INTRQ_buf[snd_ch],
			DET[emph][rcvr_nr].bit_buf[snd_ch],
			rx_bufsize*n_INTR[snd_ch],
			&DET[emph][rcvr_nr].rx_data[snd_ch]);

	}
	else decode_stream_QPSK(last, snd_ch, rcvr_nr, emph, 
		DET[emph][rcvr_nr].src_LPF1I_buf[snd_ch],
		DET[emph][rcvr_nr].src_LPF1Q_buf[snd_ch],
		DET[emph][rcvr_nr].bit_buf[snd_ch],
		rx_bufsize,
		&DET[emph][rcvr_nr].rx_data[snd_ch]);

}


/*

procedure PSK8_Demodulator(snd_ch,rcvr_nr,emph: byte; last: boolean);
{
  MUX3_PSK(snd_ch,rcvr_nr,emph,DET[0,rcvr_nr].src_BPF_buf[snd_ch],LPF_core[snd_ch],DET[emph,rcvr_nr].src_LPF1I_buf[snd_ch],DET[emph,rcvr_nr].src_LPF1Q_buf[snd_ch],DET[emph,rcvr_nr].prev_LPF1I_buf[snd_ch],DET[emph,rcvr_nr].prev_LPF1Q_buf[snd_ch],LPF_tap[snd_ch],rx_bufsize);
  if n_INTR[snd_ch]>1 then
  {
    interpolation_PSK(snd_ch,rcvr_nr,emph,DET[emph,rcvr_nr].src_intrI_buf[snd_ch],DET[emph,rcvr_nr].src_intrQ_buf[snd_ch],DET[emph,rcvr_nr].src_LPF1I_buf[snd_ch],DET[emph,rcvr_nr].src_LPF1Q_buf[snd_ch],rx_bufsize);
    decode_stream_8PSK(last,snd_ch,rcvr_nr,emph,DET[emph,rcvr_nr].src_intrI_buf[snd_ch],DET[emph,rcvr_nr].src_intrQ_buf[snd_ch],DET[emph,rcvr_nr].bit_buf[snd_ch],rx_bufsize*n_INTR[snd_ch],DET[emph,rcvr_nr].rx_data[snd_ch]);
  end
  else decode_stream_8PSK(last,snd_ch,rcvr_nr,emph,DET[emph,rcvr_nr].src_LPF1I_buf[snd_ch],DET[emph,rcvr_nr].src_LPF1Q_buf[snd_ch],DET[emph,rcvr_nr].bit_buf[snd_ch],rx_bufsize,DET[emph,rcvr_nr].rx_data[snd_ch]);
}
*/

void Demodulator(int snd_ch, int rcvr_nr, float * src_buf, int last)
{
	// called once per decoder (current one in rcvr_nr)

	int i, k;
	string rec_code;
	UCHAR emph;
	int found;
	string * s_emph;
	struct TDetector_t * pDET = &DET[0][rcvr_nr];

	// looks like tihs filters to src_BPF_buf

	FIR_filter(src_buf, rx_bufsize, BPF_tap[snd_ch], pDET->BPF_core[snd_ch], pDET->src_BPF_buf[snd_ch], pDET->prev_BPF_buf[snd_ch]);

	// AFSK demodulator

	if (modem_mode[snd_ch] == MODE_FSK)
	{
		if (emph_all[snd_ch])
		{
			for (emph = 1; emph <= nr_emph; emph++)
				FSK_Demodulator(snd_ch, rcvr_nr, emph, FALSE);

			FSK_Demodulator(snd_ch, rcvr_nr, 0, last);
		}
		else
			FSK_Demodulator(snd_ch, rcvr_nr, emph_db[snd_ch], last);
	}

	// BPSK demodulator
	if (modem_mode[snd_ch] == MODE_BPSK)
	{
		if (emph_all[snd_ch])
		{
			for (emph = 1; emph <= nr_emph; emph++)
				BPSK_Demodulator(snd_ch, rcvr_nr, emph, FALSE);

			BPSK_Demodulator(snd_ch, rcvr_nr, 0, last);
		}
		else
			BPSK_Demodulator(snd_ch, rcvr_nr, emph_db[snd_ch], last);

	}

	// QPSK demodulator
	if (modem_mode[snd_ch] == MODE_QPSK || modem_mode[snd_ch] == MODE_PI4QPSK)
	{
		if (emph_all[snd_ch])
		{
			for (emph = 1; emph <= nr_emph; emph++)
				QPSK_Demodulator(snd_ch, rcvr_nr, emph, FALSE);

			QPSK_Demodulator(snd_ch, rcvr_nr, 0, last);
		}
		else
			QPSK_Demodulator(snd_ch, rcvr_nr, emph_db[snd_ch], last);
	}
	/*

	// QPSK demodulator
	if MODEM_mode[snd_ch]=MODE_8PSK then
	{
	  if emph_all[snd_ch] then
	  {
		for emph = 1 to nr_emph do PSK8_Demodulator(snd_ch,rcvr_nr,emph,FALSE);
		PSK8_Demodulator(snd_ch,rcvr_nr,0,last);
	  end
	  else PSK8_Demodulator(snd_ch,rcvr_nr,emph_db[snd_ch],last);
	}
	*/

	// MPSK demodulator
	if (modem_mode[snd_ch] == MODE_MPSK)
	{
		decode_stream_MPSK(snd_ch, rcvr_nr, DET[0][rcvr_nr].src_BPF_buf[snd_ch], rx_bufsize, last);
	}


// I think this handles multiple decoders and passes packet on to next level

// Packet manager

	if (last)
	{
		s_emph = newString();

		if (emph_all[snd_ch])
		{
			for (i = 0; i < nr_emph; i++)
			{
				switch (emph_decoded[i])
				{
				case 1:
					stringAdd(s_emph, "+", 1); //Norma
					break;
				case 2:
					stringAdd(s_emph, "#", 1); //MEM
					break;
				case 3:
					stringAdd(s_emph, "$", 1);//Single
					break;
				default:
					stringAdd(s_emph, "-", 1);//None 
				}

				emph_decoded[i] = 0; //None
			}
		}

		if (detect_list[snd_ch].Count > 0)
		{
			for (i = 0; i < detect_list[snd_ch].Count; i++)
			{
				found = 0;

				if (detect_list_l[snd_ch].Count > 0)
					if (my_indexof(&detect_list_l[snd_ch], detect_list[snd_ch].Items[i]) > -1)
						found = 1;

				if (found == 0)
				{
					if (modem_mode[snd_ch] == MODE_MPSK)
					{
						//					analiz_frame(snd_ch, detect_list[snd_ch].Items[i]->Data, [snd_ch].Items[i]->Data + ' dF: ' + FloatToStrF(DET[0, 0].AFC_dF[snd_ch], ffFixed, 0, 1));
					}
					else
					{
						if (emph_all[snd_ch])
							analiz_frame(snd_ch, detect_list[snd_ch].Items[i], s_emph);
						else
							analiz_frame(snd_ch, detect_list[snd_ch].Items[i], detect_list_c[snd_ch].Items[i]);
					}
				}
			}
		}

		freeString(s_emph);

		Assign(&detect_list_l[snd_ch], &detect_list[snd_ch]);	// Duplicate detect_list to detect_list_l

		Clear(&detect_list[snd_ch]);
		Clear(&detect_list_c[snd_ch]);

		chk_dcd1(snd_ch, rx_bufsize);
		
	}
}
	
