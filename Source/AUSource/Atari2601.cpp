/*
     File: SinSynth.cpp
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

/*****************************************************************************/
/*                                                                           */
/* Module:  TIA Chip Sound Simulator                                         */
/* Purpose: To emulate the sound generation hardware of the Atari TIA chip.  */
/* Author:  Ron Fries                                                        */
/*                                                                           */
/* Revision History:                                                         */
/*    10-Sep-96 - V1.0 - Initial Release                                     */
/*    14-Jan-97 - V1.1 - Cleaned up sound output by eliminating counter      */
/*                       reset.                                              */
/*                                                                           */
/*****************************************************************************/
/*                                                                           */
/*                 License Information and Copyright Notice                  */
/*                 ========================================                  */
/*                                                                           */
/* TiaSound is Copyright(c) 1996 by Ron Fries                                */
/*                                                                           */
/* This library is free software; you can redistribute it and/or modify it   */
/* under the terms of version 2 of the GNU Library General Public License    */
/* as published by the Free Software Foundation.                             */
/*                                                                           */
/* This library is distributed in the hope that it will be useful, but       */
/* WITHOUT ANY WARRANTY; without even the implied warranty of                */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library */
/* General Public License for more details.                                  */
/* To obtain a copy of the GNU Library General Public License, write to the  */
/* Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.   */
/*                                                                           */
/* Any permitted reproduction of these routines, in whole or in part, must   */
/* bear this legend.                                                         */
/*                                                                           */
/*****************************************************************************/

#include "Atari2601.h"

/* definitions for AUDCx (15, 16) */
#define SET_TO_1     0x00      /* 0000 */
#define POLY4        0x01      /* 0001 */
#define DIV31_POLY4  0x02      /* 0010 */
#define POLY5_POLY4  0x03      /* 0011 */
#define PURE         0x04      /* 0100 */
#define PURE2        0x05      /* 0101 */
#define DIV31_PURE   0x06      /* 0110 */
#define POLY5_2      0x07      /* 0111 */
#define POLY9        0x08      /* 1000 */
#define POLY5        0x09      /* 1001 */
#define DIV31_POLY5  0x0a      /* 1010 */
#define POLY5_POLY5  0x0b      /* 1011 */
#define DIV3_PURE    0x0c      /* 1100 */
#define DIV3_PURE2   0x0d      /* 1101 */
#define DIV93_PURE   0x0e      /* 1110 */
#define DIV3_POLY5   0x0f      /* 1111 */

#define DIV3_MASK    0x0c

#define AUDC0        0x15
#define AUDC1        0x16
#define AUDF0        0x17
#define AUDF1        0x18
#define AUDV0        0x19
#define AUDV1        0x1a

/* the size (in entries) of the 4 polynomial tables */
#define POLY4_SIZE  0x000f
#define POLY5_SIZE  0x001f
#define POLY9_SIZE  0x01ff

/* channel definitions */
#define CHAN1       0
#define CHAN2       1

/* Initialze the bit patterns for the polynomials. */

/* The 4bit and 5bit patterns are the identical ones used in the tia chip. */
/* Though the patterns could be packed with 8 bits per byte, using only a */
/* single bit per byte keeps the math simple, which is important for */
/* efficient processing. */

static uint8 Bit4[POLY4_SIZE] =
{ 1,1,0,1,1,1,0,0,0,0,1,0,1,0,0 };

static uint8 Bit5[POLY5_SIZE] =
{ 0,0,1,0,1,1,0,0,1,1,1,1,1,0,0,0,1,1,0,1,1,1,0,1,0,1,0,0,0,0,1 };

/* I've treated the 'Div by 31' counter as another polynomial because of */
/* the way it operates.  It does not have a 50% duty cycle, but instead */
/* has a 13:18 ratio (of course, 13+18 = 31).  This could also be */
/* implemented by using counters. */

static uint8 Div31[POLY5_SIZE] =
{ 0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0 };

static const UInt32 kMaxActiveNotes = 8;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma mark Atari2601 Methods

AUDIOCOMPONENT_ENTRY(AUMusicDeviceFactory, Atari2601)

enum
{
	kAtariParam_audc0,
	kAtariParam_audc1,
    kAtariParam_audf0,
    kAtariParam_audf1,
    kAtariParam_audv0,
    kAtariParam_audv1,
    kAtariParam_rate
};

const float kMinAudc0 = 0.0;
const float kDefaultAudc0 = 1.0;
const float kMaxAudc0 = 15.0;

const float kMinAudc1 = 0.0;
const float kDefaultAudc1 = 2.0;
const float kMaxAudc1 = 15.0;

const float kMinAudf0 = 0.0;
const float kDefaultAudf0 = 3.0;
const float kMaxAudf0 = 31.0;

const float kMinAudf1 = 0.0;
const float kDefaultAudf1 = 4.0;
const float kMaxAudf1 = 31.0;

const float kMinAudv0 = 0.0;
const float kDefaultAudv0 = 5.0;
const float kMaxAudv0 = 31.0;

const float kMinAudv1 = 0.0;
const float kDefaultAudv1 = 5.0;
const float kMaxAudv1 = 31.0;

const float kMinRate = -32767.0;
const float kDefaultRate = 1.0;
const float kMaxRate = 32768.0;


static CFStringRef kAudc0_Name = CFSTR("Tone 1");
static CFStringRef kAudc1_Name = CFSTR("Tone 2");
static CFStringRef kAudf0_Name = CFSTR("Freq 1");
static CFStringRef kAudf1_Name = CFSTR("Freq 2");
static CFStringRef kAudv0_Name = CFSTR("Volume 1");
static CFStringRef kAudv1_Name = CFSTR("Volume 2");
static CFStringRef kRate_Name = CFSTR("Rate");

Atari2601::Atari2601(AudioUnit inComponentInstance)
: AUMonotimbralInstrumentBase(inComponentInstance, 0, 1)
{
	CreateElements();
    Globals()->SetParameter(kAtariParam_audc0, kDefaultAudc0);
    Globals()->SetParameter(kAtariParam_audc1, kDefaultAudc1);
    Globals()->SetParameter(kAtariParam_audf0, kDefaultAudf0);
    Globals()->SetParameter(kAtariParam_audf1, kDefaultAudf1);
    Globals()->SetParameter(kAtariParam_audv0, kDefaultAudv0);
    Globals()->SetParameter(kAtariParam_audv1, kDefaultAudv1);
    Globals()->SetParameter(kAtariParam_rate, kDefaultRate);
}

Atari2601::~Atari2601()
{
}

void Atari2601::Cleanup()
{
#if DEBUG_PRINT
	printf("Atari2601::Cleanup\n");
#endif
}

OSStatus Atari2601::Initialize()
{
#if DEBUG_PRINT
	printf("->Atari2601::Initialize\n");
#endif
	AUMonotimbralInstrumentBase::Initialize();
	
	SetNotes(kNumNotes, kMaxActiveNotes, mTestNotes, sizeof(TestNote));
    
#if DEBUG_PRINT
	printf("<-Atari2601::Initialize\n");
#endif
	
	return noErr;
}

void TestNote::Tia_sound_init()
{
    uint8 chan;
    int16_t n;
    
    /* fill the 9bit polynomial with random bits */
    for (n=0; n < POLY9_SIZE; n++)
    {
        Bit9[n] = rand() & 0x01;       /* fill poly9 with random bits */
    }
    
    uint32 sampleRate = (uint32) SampleRate();
    /* calculate the sample 'divide by N' value based on the playback freq. */
    Samp_n_max = (uint16) ((sampleRate << 8) / sampleRate);
    Samp_n_cnt = 0;  /* initialize all bits of the sample counter */
    
    /* initialize the local globals */
    for (chan = CHAN1; chan <= CHAN2; chan++)
    {
        Outvol[chan] = 0;
        Div_n_cnt[chan] = 0;
        Div_n_max[chan] = 0;
        AUDC[chan] = 0;
        AUDF[chan] = 0;
        AUDV[chan] = 0;
        P4[chan] = 0;
        P5[chan] = 0;
        P9[chan] = 0;
    }
}

void TestNote::Update_tia_sound(uint16 addr, uint8 val)
{
    uint16 new_val = 0;
    uint8 chan;
    
    /* determine which address was changed */
    switch (addr)
    {
        case AUDC0:
            AUDC[0] = val & 0x0f;
            chan = 0;
            break;
            
        case AUDC1:
            AUDC[1] = val & 0x0f;
            chan = 1;
            break;
            
        case AUDF0:
            AUDF[0] = val & 0x1f;
            chan = 0;
            break;
            
        case AUDF1:
            AUDF[1] = val & 0x1f;
            chan = 1;
            break;
            
        case AUDV0:
            AUDV[0] = (val & 0x0f) << 3;
            chan = 0;
            break;
            
        case AUDV1:
            AUDV[1] = (val & 0x0f) << 3;
            chan = 1;
            break;
            
        default:
            chan = 255;
            break;
    }
    
    /* if the output value changed */
    if (chan != 255)
    {
        /* an AUDC value of 0 is a special case */
        if (AUDC[chan] == SET_TO_1)
        {
            /* indicate the clock is zero so no processing will occur */
            new_val = 0;
            
            /* and set the output to the selected volume */
            Outvol[chan] = AUDV[chan];
        }
        else
        {
            /* otherwise calculate the 'divide by N' value */
            new_val = AUDF[chan] + 1;
            
            /* if bits 2 & 3 are set, then multiply the 'div by n' count by 3 */
            if ((AUDC[chan] & DIV3_MASK) == DIV3_MASK)
            {
                new_val *= 3;
            }
        }
        
        /* only reset those channels that have changed */
        if (new_val != Div_n_max[chan])
        {
            /* reset the divide by n counters */
            Div_n_max[chan] = new_val;
            
            /* if the channel is now volume only or was volume only */
            if ((Div_n_cnt[chan] == 0) || (new_val == 0))
            {
                /* reset the counter (otherwise let it complete the previous) */
                Div_n_cnt[chan] = new_val;
            }
        }
    }
}

void TestNote::Tia_process(float *buffer, uint16 n)
{
    uint16 Samp_n_max_next = Samp_n_max;
	int16_t Samp_n_cnt_next = Samp_n_cnt;
	
	register uint8 audc0,audv0,audc1,audv1;
    register uint8 div_n_cnt0,div_n_cnt1;
    register uint8 p5_0, p5_1,outvol_0,outvol_1;
    
    audc0 = AUDC[0];
    audv0 = AUDV[0];
    audc1 = AUDC[1];
    audv1 = AUDV[1];
    
    /* make temporary local copy */
    p5_0 = P5[0];
    p5_1 = P5[1];
    outvol_0 = Outvol[0];
    outvol_1 = Outvol[1];
    div_n_cnt0 = Div_n_cnt[0];
    div_n_cnt1 = Div_n_cnt[1];
    
    /* loop until the buffer is filled */
    while (n)
    {
        /* Process channel 0 */
        if (div_n_cnt0 > 1)
        {
            div_n_cnt0--;
        }
        else if (div_n_cnt0 == 1)
        {
            div_n_cnt0 = Div_n_max[0];
            
            /* the P5 counter has multiple uses, so we inc it here */
            p5_0++;
            if (p5_0 == POLY5_SIZE)
                p5_0 = 0;
            
            /* check clock modifier for clock tick */
            if  (((audc0 & 0x02) == 0) ||
                 (((audc0 & 0x01) == 0) && Div31[p5_0]) ||
                 (((audc0 & 0x01) == 1) &&  Bit5[p5_0]))
            {
                if (audc0 & 0x04)       /* pure modified clock selected */
                {
                    if (outvol_0)        /* if the output was set */
                        outvol_0 = 0;     /* turn it off */
                    else
                        outvol_0 = audv0; /* else turn it on */
                }
                else if (audc0 & 0x08)    /* check for p5/p9 */
                {
                    if (audc0 == POLY9)    /* check for poly9 */
                    {
                        /* inc the poly9 counter */
                        P9[0]++;
                        if (P9[0] == POLY9_SIZE)
                            P9[0] = 0;
                        
                        if (Bit9[P9[0]])
                            outvol_0 = audv0;
                        else
                            outvol_0 = 0;
                    }
                    else                        /* must be poly5 */
                    {
                        if (Bit5[p5_0])
                            outvol_0 = audv0;
                        else
                            outvol_0 = 0;
                    }
                }
                else  /* poly4 is the only remaining option */
                {
                    /* inc the poly4 counter */
                    P4[0]++;
                    if (P4[0] == POLY4_SIZE)
                        P4[0] = 0;
                    
                    if (Bit4[P4[0]])
                        outvol_0 = audv0;
                    else
                        outvol_0 = 0;
                }
            }
        }
        
        
        /* Process channel 1 */
        if (div_n_cnt1 > 1)
        {
            div_n_cnt1--;
        }
        else if (div_n_cnt1 == 1)
        {
            div_n_cnt1 = this->Div_n_max[1];
            
            /* the P5 counter has multiple uses, so we inc it here */
            p5_1++;
            if (p5_1 == POLY5_SIZE)
                p5_1 = 0;
            
            /* check clock modifier for clock tick */
            if  (((audc1 & 0x02) == 0) ||
                 (((audc1 & 0x01) == 0) && Div31[p5_1]) ||
                 (((audc1 & 0x01) == 1) &&  Bit5[p5_1]))
            {
                if (audc1 & 0x04)       /* pure modified clock selected */
                {
                    if (outvol_1)        /* if the output was set */
                        outvol_1 = 0;     /* turn it off */
                    else
                        outvol_1 = audv1; /* else turn it on */
                }
                else if (audc1 & 0x08)    /* check for p5/p9 */
                {
                    if (audc1 == POLY9)    /* check for poly9 */
                    {
                        /* inc the poly9 counter */
                        P9[1]++;
                        if (P9[1] == POLY9_SIZE)
                            P9[1] = 0;
                        
                        if (Bit9[P9[1]])
                            outvol_1 = audv1;
                        else
                            outvol_1 = 0;
                    }
                    else                        /* must be poly5 */
                    {
                        if (Bit5[p5_1])
                            outvol_1 = audv1;
                        else
                            outvol_1 = 0;
                    }
                }
                else  /* poly4 is the only remaining option */
                {
                    /* inc the poly4 counter */
                    P4[1]++;
                    if (P4[1] == POLY4_SIZE)
                        P4[1] = 0;
                    
                    if (Bit4[P4[1]])
                        outvol_1 = audv1;
                    else
                        outvol_1 = 0;
                }
            }
        }
        
        /* decrement the sample counter - value is 256 since the lower
         byte contains the fractional part */
        Samp_n_cnt_next -= 256;
        
        /* if the count down has reached zero */
        if (Samp_n_cnt_next < 256)
        {
            /* adjust the sample counter */
            Samp_n_cnt_next += Samp_n_max_next;
            
            /* calculate the latest output value and place in buffer */
            *(buffer++) = ((outvol_0 + outvol_1)*0.00390625);
            
            /* and indicate one less byte to process */
            n--;
        }
    }
    
    /* save for next round */
    P5[0] = p5_0;
    P5[1] = p5_1;
    Outvol[0] = outvol_0;
    Outvol[1] = outvol_1;
    Div_n_cnt[0] = div_n_cnt0;
    Div_n_cnt[1] = div_n_cnt1;
	Samp_n_max = Samp_n_max_next;
	Samp_n_cnt = Samp_n_cnt_next;
}

AUElement* Atari2601::CreateElement(	AudioUnitScope					scope,
                                   AudioUnitElement				element)
{
	switch (scope)
	{
		case kAudioUnitScope_Group :
			return new SynthGroupElement(this, element, new MidiControls);
		case kAudioUnitScope_Part :
			return new SynthPartElement(this, element);
		default :
			return AUBase::CreateElement(scope, element);
	}
}

OSStatus			Atari2601::GetParameterInfo(		AudioUnitScope					inScope,
                                               AudioUnitParameterID			inParameterID,
                                               AudioUnitParameterInfo &		outParameterInfo)
{
    OSStatus result = noErr;
    outParameterInfo.flags = kAudioUnitParameterFlag_IsWritable + kAudioUnitParameterFlag_IsReadable;
    
	if (inScope == kAudioUnitScope_Global) {
		switch(inParameterID)
		{
			case kAtariParam_audc0:
				AUBase::FillInParameterName (outParameterInfo, kAudc0_Name, false);
				outParameterInfo.unit = kAudioUnitParameterUnit_Generic;
				outParameterInfo.minValue = kMinAudc0;
				outParameterInfo.maxValue = kMaxAudc0;
				outParameterInfo.defaultValue = kDefaultAudc0;
				break;
            case kAtariParam_audc1:
				AUBase::FillInParameterName (outParameterInfo, kAudc1_Name, false);
				outParameterInfo.unit = kAudioUnitParameterUnit_Generic;
				outParameterInfo.minValue = kMinAudc1;
				outParameterInfo.maxValue = kMaxAudc1;
				outParameterInfo.defaultValue = kDefaultAudc1;
				break;
                
            case kAtariParam_audf0:
				AUBase::FillInParameterName (outParameterInfo, kAudf0_Name, false);
				outParameterInfo.unit = kAudioUnitParameterUnit_Generic;
				outParameterInfo.minValue = kMinAudf0;
				outParameterInfo.maxValue = kMaxAudf0;
				outParameterInfo.defaultValue = kDefaultAudf0;
				break;
            case kAtariParam_audf1:
				AUBase::FillInParameterName (outParameterInfo, kAudf1_Name, false);
				outParameterInfo.unit = kAudioUnitParameterUnit_Generic;
				outParameterInfo.minValue = kMinAudf1;
				outParameterInfo.maxValue = kMaxAudf1;
				outParameterInfo.defaultValue = kDefaultAudf1;
				break;
                
            case kAtariParam_audv0:
				AUBase::FillInParameterName (outParameterInfo, kAudv0_Name, false);
				outParameterInfo.unit = kAudioUnitParameterUnit_Generic;
				outParameterInfo.minValue = kMinAudv0;
				outParameterInfo.maxValue = kMaxAudv0;
				outParameterInfo.defaultValue = kDefaultAudv0;
				break;
            case kAtariParam_audv1:
				AUBase::FillInParameterName (outParameterInfo, kAudv1_Name, false);
				outParameterInfo.unit = kAudioUnitParameterUnit_Generic;
				outParameterInfo.minValue = kMinAudv1;
				outParameterInfo.maxValue = kMaxAudv1;
				outParameterInfo.defaultValue = kDefaultAudv1;
				break;
                
            case kAtariParam_rate:
                AUBase::FillInParameterName (outParameterInfo, kRate_Name, false);
				outParameterInfo.unit = kAudioUnitParameterUnit_Generic;
				outParameterInfo.minValue = kMinRate;
				outParameterInfo.maxValue = kMaxRate;
				outParameterInfo.defaultValue = kDefaultRate;
                outParameterInfo.flags += kAudioUnitParameterFlag_IsHighResolution;
                break;
                
			default:
				result = kAudioUnitErr_InvalidParameter;
				break;
		}
	} else {
		result = kAudioUnitErr_InvalidParameter;
	}
	
	return result;
}


#pragma mark TestNote Methods

void			TestNote::Release(UInt32 inFrame)
{
	SynthNote::Release(inFrame);
#if DEBUG_PRINT
	printf("TestNote::Release %p %d\n", this, GetState());
#endif
}

void			TestNote::FastRelease(UInt32 inFrame) // voice is being stolen.
{
	SynthNote::Release(inFrame);
#if DEBUG_PRINT
	printf("TestNote::Release %p %d\n", this, GetState());
#endif
}

void			TestNote::Kill(UInt32 inFrame) // voice is being stolen.
{
	SynthNote::Kill(inFrame);
#if DEBUG_PRINT
	printf("TestNote::Kill %p %d\n", this, GetState());
#endif
}

OSStatus		TestNote::Render(UInt64 inAbsoluteSampleFrame, UInt32 inNumFrames, AudioBufferList** inBufferList, UInt32 inOutBusCount)
{
	float *left, *right;
    
    Update_tia_sound(0x15, GetGlobalParameter(kAtariParam_audc0));
    Update_tia_sound(0x16, GetGlobalParameter(kAtariParam_audc1));
    Update_tia_sound(0x17, GetGlobalParameter(kAtariParam_audf0));
    Update_tia_sound(0x18, GetGlobalParameter(kAtariParam_audf1));
    Update_tia_sound(0x19, GetGlobalParameter(kAtariParam_audv0));
    Update_tia_sound(0x1a, GetGlobalParameter(kAtariParam_audv1));
    uint32 sampleRate = (uint32) SampleRate();
    Samp_n_max = (uint16)((sampleRate << 8) / sampleRate) * GetGlobalParameter(kAtariParam_rate);
	
	// TestNote only writes into the first bus regardless of what is handed to us.
	const int bus0 = 0;
	int numChans = inBufferList[bus0]->mNumberBuffers;
	if (numChans > 2) return -1;
	
	left = (float*) inBufferList[bus0]->mBuffers[0].mData;
	right = numChans == 2 ? (float*) inBufferList[bus0]->mBuffers[1].mData : 0;
	
#if DEBUG_PRINT_RENDER
	printf("TestNote::Render %p %d %g %g\n", this, GetState(), phase, amp);
#endif
	switch (GetState())
	{
		case kNoteState_Attacked :
		case kNoteState_Sostenutoed :
		case kNoteState_ReleasedButSostenutoed :
		case kNoteState_ReleasedButSustained :
        {
            Tia_process(left, inNumFrames);
            if(right)
                std::copy(left, left + inNumFrames, right);
        }
            break;
			
		case kNoteState_Released :
        case kNoteState_FastReleased :
        {
            NoteEnded(0);
    
        }
            break;
			
		default :
			break;
	}
	return noErr;
}
