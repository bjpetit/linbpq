//	Teensy Soundcard emulator. Presents ADC and DAC to host as a SoundCard.

//	I don't need the overhead of the Audio Library, so I'll copy the interupt routines from
//	usb_audio.cpp and add my own wrapper
//	I see two uses for this. One combined with ARDOP (or possibly Packet) so host can run a 
//  Soundcard mode (eg WINMOR) in parallel with ARDOP/

//	Or a simple passthough (a "Signalink emulator").

//	Sample rates may be a problem. WINMOR uses 48000, but ARDOP 12000. Will PC tell us what
//  it wants???

//	What happens if we try to run ARDOP at 48000???

/* Teensyduino Core Library
   http://www.pjrc.com/teensy/
   Copyright (c) 2016 PJRC.COM, LLC.

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   1. The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   2. If the Software is incorporated into a build system that allows
   selection among a list of target devices, then similar target
   devices manufactured by PJRC.COM must be included in the list of
   target devices and selectable in the same manner.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/

#include "SoundCard.h"
#include "usb_dev.h"
#include <string.h> // for memcpy()


#ifdef AUDIO_INTERFACE // defined by usb_dev.h -> usb_desc.h
#else
#error ("USB Audio Not Enabled") 
#endif // AUDIO_INTERFACE

int SampleRate = 48000;

// Called from the USB interrupt when an isochronous packet arrives
// we must completely remove it from the receive buffer before returning
//
void usb_audio_receive_callback(unsigned int len)
{
  unsigned int count, avail;
  my_audio_block_t *left, *right;
  const uint32_t *data;

  inInts++;
  rxtot += len;

  return;
 /*
  receive_flag = 1;
  len >>= 2; // 1 sample = 4 bytes: 2 left, 2 right
  data = (const uint32_t *)usb_audio_receive_buffer;

  count = incoming_count;
  left = incoming_left;
  right = incoming_right;
  if (left == NULL) {
    left = allocate();
    if (left == NULL) return;
    incoming_left = left;
  }
  if (right == NULL) {
    right = allocate();
    if (right == NULL) return;
    incoming_right = right;
  }
  while (len > 0) {
    avail = AUDIO_BLOCK_SAMPLES - count;
    if (len < avail) {
      copy_to_buffers(data, left->data + count, right->data + count, len);
      incoming_count = count + len;
      return;
    } else if (avail > 0) {
      copy_to_buffers(data, left->data + count, right->data + count, avail);
      data += avail;
      len -= avail;
      if (ready_left || ready_right) {
        // buffer overrun, PC sending too fast
        incoming_count = count + avail;
        //if (len > 0) {
        //serial_print("!");
        //serial_phex(len);
        //}
        return;
      }
send:
      ready_left = left;
      ready_right = right;
      //if (AudioInputUSB::update_responsibility) AudioStream::update_all();
      left = allocate();
      if (left == NULL) {
        incoming_left = NULL;
        incoming_right = NULL;
        incoming_count = 0;
        return;
      }
      right = allocate();
      if (right == NULL) {
        release(left);
        incoming_left = NULL;
        incoming_right = NULL;
        incoming_count = 0;
        return;
      }
      incoming_left = left;
      incoming_right = right;
      count = 0;
    } else {
      if (ready_left || ready_right) return;
      goto send; // recover from buffer overrun
    }
  }
  incoming_count = count;
*/
}


// Called from the USB interrupt when ready to transmit another
// isochronous packet.  If we place data into the transmit buffer,
// the return is the number of bytes.  Otherwise, return 0 means
// no data to transmit

// Called every mS

unsigned int usb_audio_transmit_callback(void)
{
  static uint32_t count = 5;
  uint32_t avail, num, target, offset, len = 0;
  my_audio_block_t *left, *right;

  outInts++;

  /*
  // This is for 44.1. Send 44 9 times out of `10, then 45
  
  if (++count < 9) {   // TODO: dynamic adjust to match USB rate
    target = 44;
  } else {
    count = 0;
    target = 45;
  }
  */
  
	target = SampleRate / 1000;
  
	memset(usb_audio_transmit_buffer + len, 0xaa, target * 2);
   
  /*
   while (len < target) {
    num = target - len;
    left = left_1st;
    if (left == NULL) {
      // buffer underrun - PC is consuming too quickly
      memset(usb_audio_transmit_buffer + len, 0xaa, num * 2);
      //serial_print("%");
      break;
    }
    right = right_1st;
    offset = offset_1st;

    avail = AUDIO_BLOCK_SAMPLES - offset;
    if (num > avail) num = avail;

    copy_from_buffers((uint32_t *)usb_audio_transmit_buffer + len,
                      left->data + offset, right->data + offset, num);
    len += num;
    offset += num;
    if (offset >= AUDIO_BLOCK_SAMPLES) {
      release(left);
      release(right);
      left_1st = left_2nd;
      left_2nd = NULL;
      right_1st = right_2nd;
      right_2nd = NULL;
      offset_1st = 0;
    } else {
      offset_1st = offset;
    }
  }
 */
	return target * 4;
}
