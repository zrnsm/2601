/*	Copyright  2013 Tritone Systems, Inc. All Rights Reserved.
 
 Disclaimer: IMPORTANT:  This software is supplied to you by 
 Tritone Systems Inc. ("Tritone Systems") in consideration of your agreement to the
 following terms, and your use, installation, modification or
 redistribution of this Tritone Systems software constitutes acceptance of these
 terms.  If you do not agree with these terms, please do not use,
 install, modify or redistribute this Tritone Systems software.
 
 The Tritone Systems Software is provided by Tritone Systems on an "AS IS" basis.  
 Tritone Systems:
 MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION
 THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS
 FOR A PARTICULAR PURPOSE, REGARDING THE Tritone Systems SOFTWARE OR ITS USE AND
 OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS.
 
 IN NO EVENT SHALL Tritone Systems BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL
 OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION,
 MODIFICATION AND/OR DISTRIBUTION OF THE Tritone Systems SOFTWARE, HOWEVER CAUSED
 AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE),
 STRICT LIABILITY OR OTHERWISE, EVEN IF Tritone Systems HAS BEEN ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
 
 This software is based on the free sample code provided by Apple called FilterDemo. I modified 
 this demo software. Please see the original source at:
 
 https://developer.apple.com/library/mac/#samplecode/FilterDemo/Introduction/Intro.html#//apple_ref/doc/uid/DTS10003570
 
 for more information.
 */

#ifndef __FilterVersion_h__
#define __FilterVersion_h__


#ifdef DEBUG
	#define kFilterVersion 0xFFFFFFFF
#else
	#define kFilterVersion 0x00010000
#endif

#endif

