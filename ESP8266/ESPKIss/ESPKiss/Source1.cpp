void ICACHE_RAM_ATTR timer0_rx_handler()
{
	// This interrupt occurs at bit rate (1200 HZ), and clocks bits
	// into the RX shift register

	// reprime timer
	
	nexttonetick += BIT_DELAY;
	timer0_write(nexttonetick);

 	if (dcd)								// If we are actively monitoring a signal
	{
		dcd--;								// Decrement the dcd timer
		busy = TRUE;

		if (rxtoggled)						// See if a tone toggle was recognized
		{
			if(ones_count != 5)				// Only process if NOT a bit stuff toggle
			{
				bit_count++;				// Increment bit counter
				last8bits >>= 1;			// Shift in a zero from the left
			}

			rxtoggled = FALSE;			// Clear toggle flag
			ones_count = 0;				// Clear number of sequential o12 clicks
		}
		else
		{
			ones_count++;				// Increment ones counter since no toggle
			bit_count++;				// Increment bit counter
			last8bits >>= 1;			// Shift the bits to the right
			last8bits |= 0x80;			// shift in a one from the left
		}	// end else for 'if (rxtoggled)'

		if (last8bits == 0x7E)			// If the last 8 bits match the ax25 flag
		{
			bit_count = 0;				// Sync bit_count for an 8-bit boundary
			flagreceived = TRUE;		// Have one char time to decide if start or end flag
		}
		else
		{
			if (bit_count == 8)			// Just grabbed 8'th bit for a full byte
			{
				bit_count = 0;				// Reset bit counter
				if (!(msg_end))
					rxbytes[bytes_recd++] = last8bits;	// And stuff the byte

			
			}	// end 'if (bit_count == 8)'
		}		// end else for 'if (last8bits == 0x7E)'

	}		// end 'if (dcd)'
	else
	{
		busy = FALSE;
		//	PORTB &= 0x3E;						// Turn off the DCD LED
	}		// end else for 'if (dcd)'	
}



/*
unsigned char last8bits;		// Last 8 bits received
unsigned char bit_count;		// Bits of the next incoming byte
unsigned char ones_count;		// Sequential ones (detect a stuff)
unsigned char start_temp;		// Msg starts at end of header
unsigned char bytes_recd;		// Incoming byte index
bool flagreceived = FALSE;
*/

