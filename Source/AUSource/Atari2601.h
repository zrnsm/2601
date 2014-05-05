/*
 File: SinSynth.h
 Abstract: SinSynth.h
 Version: 1.0.1
 
 Disclaimer: IMPORTANT:  This Apple software is supplied to you by Apple
 Inc. ("Apple") in consideration of your agreement to the following
 terms, and your use, installation, modification or redistribution of
 this Apple software constitutes acceptance of these terms.  If you do
 not agree with these terms, please do not use, install, modify or
 redistribute this Apple software.
 
 In consideration of your agreement to abide by the following terms, and
 subject to these terms, Apple grants you a personal, non-exclusive
 license, under Apple's copyrights in this original Apple software (the
 "Apple Software"), to use, reproduce, modify and redistribute the Apple
 Software, with or without modifications, in source and/or binary forms;
 provided that if you redistribute the Apple Software in its entirety and
 without modifications, you must retain this notice and the following
 text and disclaimers in all such redistributions of the Apple Software.
 Neither the name, trademarks, service marks or logos of Apple Inc. may
 be used to endorse or promote products derived from the Apple Software
 without specific prior written permission from Apple.  Except as
 expressly stated in this notice, no other rights or licenses, express or
 implied, are granted by Apple herein, including but not limited to any
 patent rights that may be infringed by your derivative works or by other
 works in which the Apple Software may be incorporated.
 
 The Apple Software is provided by Apple on an "AS IS" basis.  APPLE
 MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION
 THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS
 FOR A PARTICULAR PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND
 OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS.
 
 IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL
 OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION,
 MODIFICATION AND/OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED
 AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE),
 STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
 
 Copyright (C) 2014 Apple Inc. All Rights Reserved.
 
 */

#include "AUInstrumentBase.h"
#include "Atari2601Version.h"

static const UInt32 kNumNotes = 12;

/* the size (in entries) of the 4 polynomial tables */
#define POLY4_SIZE  0x000f
#define POLY5_SIZE  0x001f
#define POLY9_SIZE  0x01ff

struct TestNote : public SynthNote
{
	virtual					~TestNote() {}
    
	virtual bool			Attack(const MusicDeviceNoteParams &inParams)
    {
#if DEBUG_PRINT
        printf("TestNote::Attack %p %d\n", this, GetState());
#endif
        Tia_sound_init();
        double sampleRate = SampleRate();
        phase = 0.;
        amp = 0.;
        maxamp = 0.4 * pow(inParams.mVelocity/127., 3.);
        up_slope = maxamp / (0.1 * sampleRate);
        dn_slope = -maxamp / (0.9 * sampleRate);
        fast_dn_slope = -maxamp / (0.005 * sampleRate);
        return true;
    }
	virtual void			Kill(UInt32 inFrame); // voice is being stolen.
	virtual void			Release(UInt32 inFrame);
	virtual void			FastRelease(UInt32 inFrame);
	virtual Float32			Amplitude() { return amp; } // used for finding quietest note for voice stealing.
	virtual OSStatus		Render(UInt64 inAbsoluteSampleFrame, UInt32 inNumFrames, AudioBufferList** inBufferList, UInt32 inOutBusCount);
    
	double phase, amp, maxamp;
	double up_slope, dn_slope, fast_dn_slope;
    
    void Tia_sound_init();
    void Update_tia_sound(uint16 addr, uint8 val);
    void Tia_process(float *buffer, uint16 n);
    void Atari2600_next_k(int inNumSamples);
    
    /* to hold the 6 tia sound control bytes */
	uint8 AUDC[2];			/* AUDCx (15, 16) */
	uint8 AUDF[2];			/* AUDFx (17, 18) */
	uint8 AUDV[2];			/* AUDVx (19, 1A) */
	
	/* last output volume for each channel */
	uint8 Outvol[2];
	
	/* Rather than have a table with 511 entries, I use a random number generator. */
	uint8 Bit9[POLY9_SIZE];
	
	uint8 P4[2];			/* Position pointer for the 4-bit POLY array */
	uint8 P5[2];			/* Position pointer for the 5-bit POLY array */
	uint16 P9[2];			/* Position pointer for the 9-bit POLY array */
	uint8 Div_n_cnt[2];		/* Divide by n counter. one for each channel */
	uint8 Div_n_max[2];		/* Divide by n maximum, one for each channel */
	
	/* In my routines, I treat the sample output as another divide by N counter. */
	/* For better accuracy, the Samp_n_cnt has a fixed binary decimal point */
	/* which has 8 binary digits to the right of the decimal point. */
	uint16 Samp_n_max;		/* Sample max, multiplied by 256 */
	int16_t Samp_n_cnt;		/* Sample cnt. */
};

class Atari2601 : public AUMonotimbralInstrumentBase
{
public:
    Atari2601(AudioUnit inComponentInstance);
	virtual						~Atari2601();
    
	virtual OSStatus			Initialize();
	virtual void				Cleanup();
	virtual OSStatus			Version() { return kAtari2601Version; }
    
	virtual AUElement*			CreateElement(			AudioUnitScope					scope,
											  AudioUnitElement				element);
    
	virtual OSStatus			GetParameterInfo(		AudioUnitScope					inScope,
                                                 AudioUnitParameterID			inParameterID,
                                                 AudioUnitParameterInfo &		outParameterInfo);
    
	MidiControls*				GetControls( MusicDeviceGroupID inChannel)
	{
		SynthGroupElement *group = GetElForGroupID(inChannel);
		return (MidiControls *) group->GetMIDIControlHandler();
	}
	
private:	
	TestNote mTestNotes[kNumNotes];
};
