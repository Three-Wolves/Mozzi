/*
 * Sample.h
 *
 * Copyright 2012 unbackwards@gmail.com.
 *
 * This file is part of Cuttlefish.
 *
 * Cuttlefish is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Cuttlefish is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Cuttlefish.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef SAMPLE_H_
#define SAMPLE_H_

#include "Arduino.h"
#include "CuttlefishGuts.h"
#include <util/atomic.h>

// fractional bits for samplelator index precision
#define SAMPLE_F_BITS 16
#define SAMPLE_F_BITS_AS_MULTIPLIER 65536

// phmod_proportion is an 1n15 fixed-point number only using
// the fractional part and the sign bit
#define SAMPLE_PHMOD_BITS 16


/** Sample is like Oscil, it plays a wavetable, but defaults to playing once through, from start to finish.
*/
template <unsigned int NUM_TABLE_CELLS, unsigned int UPDATE_RATE>
class Sample
{

public:

	/** Sample is a template class, declared in a sketch as follows, with
	NUM_TABLE_CELLS and UPDATE_RATE replaced by your own values: Sample
	<NUM_TABLE_CELLS, UPDATE_RATE> mysample(tablename); It's important that
	NUM_TABLE_CELLS and UPDATE_RATE are both literal integers, directly
	stated as numbers (eg. "8192") or as predefined macros. AUDIO_RATE is a
	defined literal used by Cuttlefish. It's useful to define CONTROL_RATE
	in your sketches ("eg. #define CONTROL_RATE 128"), to configure
	startCuttlefish(CONTROL_RATE) in setup() and to sync your control
	calculations using the same value in updateControl().
	*/
	Sample(prog_char * TABLE_NAME):table(TABLE_NAME)
	{}



	/** Resets the phase (the playhead) to the beginning of the table.
	*/
	inline
	void start()
	{
		phase_fractional = 0;
	}


	/** Returns the next sample. Updates the phase according to the current frequency
	and returns the sample at the new phase position, or 0 when the phase
	overshoots the end of the table.
	*/
	inline
	char next()
	{
		char out = 0;
		incrementPhase();
		if (phase_fractional < (unsigned long) NUM_TABLE_CELLS<<SAMPLE_F_BITS )
			out = readTable();
		return out;
	}


	/** Returns the next sample given a phase modulation value. The modulation is given
	as a proportion of the wave. The phmod_proportion parameter is a Q15n16
	fixed-point number where to fractional n16 part is -1 to 1, modulating the phase
	by one whole table length in each direction.
	*/
	inline
	char phMod(long phmod_proportion)
	{
		incrementPhase();
		return (char)pgm_read_byte_near(table + (((phase_fractional+(phmod_proportion * NUM_TABLE_CELLS))>>SAMPLE_F_BITS) & (NUM_TABLE_CELLS - 1)));
	}


	/** Set the frequency using 24n8 fixed-point number format. Note: use with caution
	because it's prone to overflow with higher frequencies and larger table sizes.
	This might be faster than the float version for setting low frequencies such as
	1.5 Hz, or others which may not work well with your table size. An n8
	representation of 1.5 is 384 (ie. 1.5 * 256).
	*/
	inline
	void setFreq_n8(unsigned long freq)
	{
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
		{
			phase_increment_fractional = ((freq * NUM_TABLE_CELLS)/UPDATE_RATE) << (SAMPLE_F_BITS-8);
		}
	}


	/** Set the samplelator frequency with an unsigned int. This is faster than using a
	float, so it's useful when processor time is tight, but it can be tricky with
	low and high frequencies, depending on the size of the wavetable being used. If
	you're not getting the results you expect, try explicitly using a float, or try
	setFreq_n8.
	*/
	inline
	void setFreq(unsigned int freq)
	{
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
		{
			phase_increment_fractional = (((unsigned long)freq * NUM_TABLE_CELLS)/UPDATE_RATE) << SAMPLE_F_BITS; // Obvious but slow
		}
	}


	/** Set the samplelator frequency with a float. Using a float is the most reliable
	way to set frequencies, Might be slower than using an int but you need either
	this or setFreq_n8 for fractional frequencies.
	*/
	inline
	void setFreq(float freq)
	{ // 1 us
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
		{
			phase_increment_fractional = (unsigned long)((((float)NUM_TABLE_CELLS * freq)/UPDATE_RATE) * SAMPLE_F_BITS_AS_MULTIPLIER);
		}
	}


	/**  Returns the sample at the given table index between 0 and the table size.The
	index rolls back around to 0 if it's larger than the table size. */
	inline
	char atIndex(unsigned int index)
	{
		return (char)pgm_read_byte_near(table + (index & (NUM_TABLE_CELLS - 1)));
	}


	/**
	phaseIncFromFreq and setPhaseInc are for saving processor time when sliding
	between frequencies. Instead of recalculating the phase increment for each
	frequency in between, you can just calculate each end and use a Line to
	interpolate. (Note: I should really profile this with the sampleloscope to see if
	it's worth the extra confusion.)
	*/
	inline
	unsigned long phaseIncFromFreq(unsigned int freq)
	{
		return (((unsigned long)freq * NUM_TABLE_CELLS)/UPDATE_RATE) << SAMPLE_F_BITS;
	}


	/**
	See phaseIncFromFreq.
	*/
	inline
	void setPhaseInc(unsigned long phaseinc_fractional)
	{
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
		{
			phase_increment_fractional = phaseinc_fractional;
		}
	}


private:

	/**
	Increments the phase of the samplelator without returning a sample.
	*/
	inline
	void incrementPhase()
	{
		phase_fractional += phase_increment_fractional;
	}


	/**
	Returns the current sample.
	*/
	inline
	char readTable()
	{
		return (char)pgm_read_byte_near(table + ((phase_fractional >> SAMPLE_F_BITS) & (NUM_TABLE_CELLS - 1)));
	}

	unsigned long phase_fractional;
	volatile unsigned long phase_increment_fractional;
	prog_char * table;

};

#endif /* SAMPLE_H_ */