/*

   Porting: Search for WIN32 to find windows-specific code
            snippets which need to be replaced or removed.

modify started: 22/02/2013 16:00
finished modifying: 22/02/2013 18:13

*/

#ifdef WIN32
#include "ddrawkit.h"
#else
#include "sdlkit.h"
#endif
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#ifdef WIN32
#include "DPInput.h" // WIN32
#include "pa/portaudio.h"
#include "fileselector.h" // WIN32
#else
#include "SDL.h"
#endif
#include "util.h"

#define rnd(n) (rand()%(n+1))

#define PI 3.14159265f

float frnd(float range)
{
	return (float)rnd(10000)/10000*range;
}

struct Spriteset
{
	DWORD *data;
	int width;
	int height;
	int pitch;
};

Spriteset font;
Spriteset ld48;

struct Category
{
	char name[32];
};

Category categories[10];

const int PHASER_BUFFER_SIZE = 4096; //2 [power] 12
const int PHASER_BUFFER_MASK = 4096-1;

float master_vol=0.5f;
float master_vol_multiplier=0.1f;

struct Channel{
	bool enabled;
	int wave_type;

	float p_base_freq;
	float p_freq_limit;
	float p_freq_ramp;
	float p_freq_dramp;
	float p_duty;
	float p_duty_ramp;

	float p_vib_strength;
	float p_vib_speed;
	float p_vib_delay;

	float p_env_attack;
	float p_env_sustain;
	float p_env_decay;
	float p_env_punch;
	float p_env_delay;

	bool filter_on;
	float p_lpf_resonance;
	float p_lpf_freq;
	float p_lpf_ramp;
	float p_hpf_freq;
	float p_hpf_ramp;

	float p_pha_offset;
	float p_pha_ramp;

	float p_repeat_speed;

	float p_arp_speed;
	float p_arp_mod;

	int phase;
	double fperiod;
	double fmaxperiod;
	double fslide;
	double fdslide;
	int period;
	float square_duty;
	float square_slide;
	int env_stage;
	int env_time;
	int env_length[4];
	float env_vol;
	float fphase;
	float fdphase;
	int iphase;
	float phaser_buffer[PHASER_BUFFER_SIZE];
	int ipp;
	float noise_buffer[32];
	float fltp;
	float fltdp;
	float fltw;
	float fltw_d;
	float fltdmp;
	float fltphp;
	float flthp;
	float flthp_d;
	float vib_phase;
	float vib_speed;
	float vib_amp;
	int rep_time;
	int rep_limit;
	int arp_time;
	int arp_limit;
	double arp_mod;

	float sound_vol;

	bool playing_sample;
};
const int CHANNEL_N = 16;
Channel channels[CHANNEL_N];
int currentChannel = 0;

float* vselected=NULL;
int vcurbutton=-1;

int wav_bits=16;
int wav_freq=44100;

int file_sampleswritten;
float filesample=0.0f;
int fileacc=0;

void ResetParams(int channel=-1)
{
	for(int i=0;i<CHANNEL_N;i++)
	{

		if(channel==-1)
			channels[i].enabled=(i==0);
		else if(channel!=i)
			continue;
		else
			channels[i].enabled=true;

		channels[i].wave_type=0;
		channels[i].sound_vol=0.5f;

		channels[i].p_base_freq=0.3f;
		channels[i].p_freq_limit=0.0f;
		channels[i].p_freq_ramp=0.0f;
		channels[i].p_freq_dramp=0.0f;
		channels[i].p_duty=0.0f;
		channels[i].p_duty_ramp=0.0f;

		channels[i].p_vib_strength=0.0f;
		channels[i].p_vib_speed=0.0f;
		channels[i].p_vib_delay=0.0f;

		channels[i].p_env_attack=0.0f;
		channels[i].p_env_sustain=0.3f;
		channels[i].p_env_decay=0.4f;
		channels[i].p_env_punch=0.0f;
		channels[i].p_env_delay=0.0f;

		channels[i].filter_on=false;
		channels[i].p_lpf_resonance=0.0f;
		channels[i].p_lpf_freq=1.0f;
		channels[i].p_lpf_ramp=0.0f;
		channels[i].p_hpf_freq=0.0f;
		channels[i].p_hpf_ramp=0.0f;
	
		channels[i].p_pha_offset=0.0f;
		channels[i].p_pha_ramp=0.0f;

		channels[i].p_repeat_speed=0.0f;

		channels[i].p_arp_speed=0.0f;
		channels[i].p_arp_mod=0.0f;

	}
}

bool LoadSettings(char* filename)
{
	FILE* file=fopen(filename, "rb");
	if(!file)
		return false;

	int version=0;
	fread(&version, 1, sizeof(int), file);
	if(version!=100 && version!=101 && version!=102 && version!=111)
		return false;
	if(version==111)
	{
		for(int i=0;i<CHANNEL_N;i++)
		{
			fread(&channels[i].enabled, 1, sizeof(bool), file);
			fread(&channels[i].wave_type, 1, sizeof(int), file);

			channels[i].sound_vol=0.5f;
			fread(&channels[i].sound_vol, 1, sizeof(float), file);

			fread(&channels[i].p_base_freq, 1, sizeof(float), file);
			fread(&channels[i].p_freq_limit, 1, sizeof(float), file);
			fread(&channels[i].p_freq_ramp, 1, sizeof(float), file);
			fread(&channels[i].p_freq_dramp, 1, sizeof(float), file);
			fread(&channels[i].p_duty, 1, sizeof(float), file);
			fread(&channels[i].p_duty_ramp, 1, sizeof(float), file);

			fread(&channels[i].p_vib_strength, 1, sizeof(float), file);
			fread(&channels[i].p_vib_speed, 1, sizeof(float), file);
			fread(&channels[i].p_vib_delay, 1, sizeof(float), file);

			fread(&channels[i].p_env_attack, 1, sizeof(float), file);
			fread(&channels[i].p_env_sustain, 1, sizeof(float), file);
			fread(&channels[i].p_env_decay, 1, sizeof(float), file);
			fread(&channels[i].p_env_punch, 1, sizeof(float), file);
			fread(&channels[i].p_env_delay, 1, sizeof(float), file);

			fread(&channels[i].filter_on, 1, sizeof(bool), file);
			fread(&channels[i].p_lpf_resonance, 1, sizeof(float), file);
			fread(&channels[i].p_lpf_freq, 1, sizeof(float), file);
			fread(&channels[i].p_lpf_ramp, 1, sizeof(float), file);
			fread(&channels[i].p_hpf_freq, 1, sizeof(float), file);
			fread(&channels[i].p_hpf_ramp, 1, sizeof(float), file);
	
			fread(&channels[i].p_pha_offset, 1, sizeof(float), file);
			fread(&channels[i].p_pha_ramp, 1, sizeof(float), file);

			fread(&channels[i].p_repeat_speed, 1, sizeof(float), file);

			fread(&channels[i].p_arp_speed, 1, sizeof(float), file);
			fread(&channels[i].p_arp_mod, 1, sizeof(float), file);
		}
	}else{
		fread(&channels[0].wave_type, 1, sizeof(int), file);

		channels[0].sound_vol=0.5f;
		if(version==102)
			fread(&channels[0].sound_vol, 1, sizeof(float), file);

		fread(&channels[0].p_base_freq, 1, sizeof(float), file);
		fread(&channels[0].p_freq_limit, 1, sizeof(float), file);
		fread(&channels[0].p_freq_ramp, 1, sizeof(float), file);
		if(version>=101)
			fread(&channels[0].p_freq_dramp, 1, sizeof(float), file);
		fread(&channels[0].p_duty, 1, sizeof(float), file);
		fread(&channels[0].p_duty_ramp, 1, sizeof(float), file);

		fread(&channels[0].p_vib_strength, 1, sizeof(float), file);
		fread(&channels[0].p_vib_speed, 1, sizeof(float), file);
		fread(&channels[0].p_vib_delay, 1, sizeof(float), file);

		fread(&channels[0].p_env_attack, 1, sizeof(float), file);
		fread(&channels[0].p_env_sustain, 1, sizeof(float), file);
		fread(&channels[0].p_env_decay, 1, sizeof(float), file);
		fread(&channels[0].p_env_punch, 1, sizeof(float), file);

		fread(&channels[0].filter_on, 1, sizeof(bool), file);
		fread(&channels[0].p_lpf_resonance, 1, sizeof(float), file);
		fread(&channels[0].p_lpf_freq, 1, sizeof(float), file);
		fread(&channels[0].p_lpf_ramp, 1, sizeof(float), file);
		fread(&channels[0].p_hpf_freq, 1, sizeof(float), file);
		fread(&channels[0].p_hpf_ramp, 1, sizeof(float), file);
	
		fread(&channels[0].p_pha_offset, 1, sizeof(float), file);
		fread(&channels[0].p_pha_ramp, 1, sizeof(float), file);

		fread(&channels[0].p_repeat_speed, 1, sizeof(float), file);

		if(version>=101)
		{
			fread(&channels[0].p_arp_speed, 1, sizeof(float), file);
			fread(&channels[0].p_arp_mod, 1, sizeof(float), file);
		}
	}

	fclose(file);
	return true;
}

bool SaveSettings(char* filename)
{
	FILE* file=fopen(filename, "wb");
	if(!file)
		return false;

	int version=111;

	fwrite(&version, 1, sizeof(int), file);
	for(int i=0;i<CHANNEL_N;i++)
	{
		fwrite(&channels[i].enabled, 1, sizeof(bool), file);
		fwrite(&channels[i].wave_type, 1, sizeof(int), file);

		fwrite(&channels[i].sound_vol, 1, sizeof(float), file);

		fwrite(&channels[i].p_base_freq, 1, sizeof(float), file);
		fwrite(&channels[i].p_freq_limit, 1, sizeof(float), file);
		fwrite(&channels[i].p_freq_ramp, 1, sizeof(float), file);
		fwrite(&channels[i].p_freq_dramp, 1, sizeof(float), file);
		fwrite(&channels[i].p_duty, 1, sizeof(float), file);
		fwrite(&channels[i].p_duty_ramp, 1, sizeof(float), file);

		fwrite(&channels[i].p_vib_strength, 1, sizeof(float), file);
		fwrite(&channels[i].p_vib_speed, 1, sizeof(float), file);
		fwrite(&channels[i].p_vib_delay, 1, sizeof(float), file);

		fwrite(&channels[i].p_env_attack, 1, sizeof(float), file);
		fwrite(&channels[i].p_env_sustain, 1, sizeof(float), file);
		fwrite(&channels[i].p_env_decay, 1, sizeof(float), file);
		fwrite(&channels[i].p_env_punch, 1, sizeof(float), file);
		fwrite(&channels[i].p_env_delay, 1, sizeof(float), file);

		fwrite(&channels[i].filter_on, 1, sizeof(bool), file);
		fwrite(&channels[i].p_lpf_resonance, 1, sizeof(float), file);
		fwrite(&channels[i].p_lpf_freq, 1, sizeof(float), file);
		fwrite(&channels[i].p_lpf_ramp, 1, sizeof(float), file);
		fwrite(&channels[i].p_hpf_freq, 1, sizeof(float), file);
		fwrite(&channels[i].p_hpf_ramp, 1, sizeof(float), file);
	
		fwrite(&channels[i].p_pha_offset, 1, sizeof(float), file);
		fwrite(&channels[i].p_pha_ramp, 1, sizeof(float), file);

		fwrite(&channels[i].p_repeat_speed, 1, sizeof(float), file);

		fwrite(&channels[i].p_arp_speed, 1, sizeof(float), file);
		fwrite(&channels[i].p_arp_mod, 1, sizeof(float), file);
	}

	fclose(file);
	return true;
}

void ResetSample(bool restart, int channel=-1)
{
	for(int i=0;i<CHANNEL_N;i++)
	{
		if(channel!=-1&&channel!=i)
			continue;

		if(!restart)
			channels[i].phase=0;
		channels[i].fperiod=100.0/(channels[i].p_base_freq*channels[i].p_base_freq+0.001);
		channels[i].period=(int)channels[i].fperiod;
		channels[i].fmaxperiod=100.0/(channels[i].p_freq_limit*channels[i].p_freq_limit+0.001);
		channels[i].fslide=1.0-pow((double)channels[i].p_freq_ramp, 3.0)*0.01;
		channels[i].fdslide=-pow((double)channels[i].p_freq_dramp, 3.0)*0.000001;
		channels[i].square_duty=0.5f-channels[i].p_duty*0.5f;
		channels[i].square_slide=-channels[i].p_duty_ramp*0.00005f;
		if(channels[i].p_arp_mod>=0.0f)
			channels[i].arp_mod=1.0-pow((double)channels[i].p_arp_mod, 2.0)*0.9;
		else
			channels[i].arp_mod=1.0+pow((double)channels[i].p_arp_mod, 2.0)*10.0;
		channels[i].arp_time=0;
		channels[i].arp_limit=(int)(pow(1.0f-channels[i].p_arp_speed, 2.0f)*20000+32);
		if(channels[i].p_arp_speed==1.0f)
			channels[i].arp_limit=0;
		if(!restart)
		{
			// reset filter
			channels[i].fltp=0.0f;
			channels[i].fltdp=0.0f;
			channels[i].fltw=pow(channels[i].p_lpf_freq, 3.0f)*0.1f;
			channels[i].fltw_d=1.0f+channels[i].p_lpf_ramp*0.0001f;
			channels[i].fltdmp=5.0f/(1.0f+pow(channels[i].p_lpf_resonance, 2.0f)*20.0f)*(0.01f+channels[i].fltw);
			if(channels[i].fltdmp>0.8f) channels[i].fltdmp=0.8f;
			channels[i].fltphp=0.0f;
			channels[i].flthp=pow(channels[i].p_hpf_freq, 2.0f)*0.1f;
			channels[i].flthp_d=1.0+channels[i].p_hpf_ramp*0.0003f;
			// reset vibrato
			channels[i].vib_phase=0.0f;
			channels[i].vib_speed=pow(channels[i].p_vib_speed, 2.0f)*0.01f;
			channels[i].vib_amp=channels[i].p_vib_strength*0.5f;
			// reset envelope
			channels[i].env_vol=0.0f;
			channels[i].env_stage=0;
			channels[i].env_time=0;
			channels[i].env_length[0]=(int)(channels[i].p_env_delay*channels[i].p_env_delay*100000.0f);
			channels[i].env_length[1]=(int)(channels[i].p_env_attack*channels[i].p_env_attack*100000.0f);
			channels[i].env_length[2]=(int)(channels[i].p_env_sustain*channels[i].p_env_sustain*100000.0f);
			channels[i].env_length[3]=(int)(channels[i].p_env_decay*channels[i].p_env_decay*100000.0f);

			channels[i].fphase=pow(channels[i].p_pha_offset, 2.0f)*1020.0f;
			if(channels[i].p_pha_offset<0.0f) channels[i].fphase=-channels[i].fphase;
			channels[i].fdphase=pow(channels[i].p_pha_ramp, 2.0f)*1.0f;
			if(channels[i].p_pha_ramp<0.0f) channels[i].fdphase=-channels[i].fdphase;
			channels[i].iphase=abs((int)channels[i].fphase);
			channels[i].ipp=0;
			for(int j=0;j<PHASER_BUFFER_SIZE;j++)
				channels[i].phaser_buffer[j]=0.0f;

			for(int j=0;j<32;j++)
				channels[i].noise_buffer[j]=frnd(2.0f)-1.0f;

			channels[i].rep_time=0;
			channels[i].rep_limit=(int)(pow(1.0f-channels[i].p_repeat_speed, 2.0f)*20000+32);
			if(channels[i].p_repeat_speed==0.0f)
				channels[i].rep_limit=0;
		}
	}
}

bool havePlayingSample()
{
	for(int i=0;i<CHANNEL_N;i++)
		if(channels[i].playing_sample)
			return true;
	return false;
}

void PlaySample()
{
	ResetSample(false);
	for(int i=0;i<CHANNEL_N;i++)
		channels[i].playing_sample=channels[i].enabled;
}

void SynthSample(int length, float* buffer, FILE* file)
{
	for(int i=0;i<length;i++)
	{
		if(!havePlayingSample())
			break;

		float ssample=0.0f;
		for(int j=0;j<CHANNEL_N;j++)
		{
			if(!channels[j].playing_sample)
				continue;

			// volume envelope
			channels[j].env_time++;
			if(channels[j].env_time>channels[j].env_length[channels[j].env_stage])
			{
				channels[j].env_time=0;
				channels[j].env_stage++;
				if(channels[j].env_stage==4)
					channels[j].playing_sample=false;
			}
			if(channels[j].env_stage==0) continue;

			switch(channels[j].env_stage)
			{
				case 1: channels[j].env_vol=(float)channels[j].env_time/channels[j].env_length[1]; break;
				case 2: channels[j].env_vol=1.0f+pow(1.0f-(float)channels[j].env_time/channels[j].env_length[2], 1.0f)*2.0f*channels[j].p_env_punch; break;
				case 3: channels[j].env_vol=1.0f-(float)channels[j].env_time/channels[j].env_length[3]; break;
			}

			channels[j].rep_time++;
			if(channels[j].rep_limit!=0 && channels[j].rep_time>=channels[j].rep_limit)
			{
				channels[j].rep_time=0;
				ResetSample(true, j);
			}

			// frequency envelopes/arpeggios
			channels[j].arp_time++;
			if(channels[j].arp_limit!=0 && channels[j].arp_time>=channels[j].arp_limit)
			{
				channels[j].arp_limit=0;
				channels[j].fperiod*=channels[j].arp_mod;
			}
			channels[j].fslide+=channels[j].fdslide;
			channels[j].fperiod*=channels[j].fslide;
			if(channels[j].fperiod>channels[j].fmaxperiod)
			{
				channels[j].fperiod=channels[j].fmaxperiod;
				if(channels[j].p_freq_limit>0.0f)
					channels[j].playing_sample=false;
			}
			float rfperiod=channels[j].fperiod;
			if(channels[j].vib_amp>0.0f)
			{
				channels[j].vib_phase+=channels[j].vib_speed;
				rfperiod=channels[j].fperiod*(1.0+sin(channels[j].vib_phase)*channels[j].vib_amp);
			}
			channels[j].period=(int)rfperiod;
			if(channels[j].period<8) channels[j].period=8;
			channels[j].square_duty+=channels[j].square_slide;
			if(channels[j].square_duty<0.0f) channels[j].square_duty=0.0f;
			if(channels[j].square_duty>0.5f) channels[j].square_duty=0.5f;

			// phaser step
			channels[j].fphase+=channels[j].fdphase;
			channels[j].iphase=abs((int)channels[j].fphase);
			if(channels[j].iphase>=PHASER_BUFFER_SIZE) channels[j].iphase=PHASER_BUFFER_SIZE;

			if(channels[j].flthp_d!=0.0f)
			{
				channels[j].flthp*=channels[j].flthp_d;
				if(channels[j].flthp<0.00001f) channels[j].flthp=0.00001f;
				if(channels[j].flthp>0.1f) channels[j].flthp=0.1f;
			}
			float sub_ssample = 0.0f;
			for(int si=0;si<8;si++) // 8x supersampling
			{
				float sample=0.0f;
				channels[j].phase++;
				if(channels[j].phase>=channels[j].period)
				{
	//				phase=0;
					channels[j].phase%=channels[j].period;
					if(channels[j].wave_type==3)
						for(int i=0;i<32;i++)
							channels[j].noise_buffer[i]=frnd(2.0f)-1.0f;
				}
				// base waveform
				float fp=(float)channels[j].phase/channels[j].period;
				switch(channels[j].wave_type)
				{
				case 0: // square
					if(fp<channels[j].square_duty)
						sample=0.5f;
					else
						sample=-0.5f;
					break;
				case 1: // sawtooth
					sample=1.0f-fp*2;
					break;
				case 2: // sine
					sample=(float)sin(fp*2*PI);
					break;
				case 3: // noise
					sample=channels[j].noise_buffer[channels[j].phase*32/channels[j].period];
					break;
				}
				// lp filter
				float pp=channels[j].fltp;
				channels[j].fltw*=channels[j].fltw_d;
				if(channels[j].fltw<0.0f) channels[j].fltw=0.0f;
				if(channels[j].fltw>0.1f) channels[j].fltw=0.1f;
				if(channels[j].p_lpf_freq!=1.0f)
				{
					channels[j].fltdp+=(sample-channels[j].fltp)*channels[j].fltw;
					channels[j].fltdp-=channels[j].fltdp*channels[j].fltdmp;
				}
				else
				{
					channels[j].fltp=sample;
					channels[j].fltdp=0.0f;
				}


				channels[j].fltp+=channels[j].fltdp;
				// hp filter
				channels[j].fltphp+=channels[j].fltp-pp;
				channels[j].fltphp-=channels[j].fltphp*channels[j].flthp;
				sample=channels[j].fltphp;
				// phaser
				channels[j].phaser_buffer[channels[j].ipp&PHASER_BUFFER_MASK]=sample;
				sample+=channels[j].phaser_buffer[(channels[j].ipp-channels[j].iphase+PHASER_BUFFER_SIZE)&PHASER_BUFFER_MASK];
				channels[j].ipp=(channels[j].ipp+1)&PHASER_BUFFER_MASK;
				// final accumulation and envelope application
				sub_ssample+=sample*channels[j].env_vol;
			}
			ssample+=(sub_ssample/8*master_vol*master_vol_multiplier)*(2.0f*channels[j].sound_vol);
		}
		if(buffer!=NULL)
		{
			if(ssample>1.0f) ssample=1.0f;
			if(ssample<-1.0f) ssample=-1.0f;
			*buffer++=ssample;
		}
		if(file!=NULL)
		{
			// quantize depending on format
			// accumulate/count to accomodate variable sample rate?
			ssample*=4.0f; // arbitrary gain to get reasonable output volume...
			if(ssample>1.0f) ssample=1.0f;
			if(ssample<-1.0f) ssample=-1.0f;
			filesample+=ssample;
			fileacc++;
			if(wav_freq==44100 || fileacc==2)
			{
				filesample/=fileacc;
				fileacc=0;
				if(wav_bits==16)
				{
					short isample=(short)(filesample*32000);
					fwrite(&isample, 1, 2, file);
				}
				else
				{
					unsigned char isample=(unsigned char)(filesample*127+128);
					fwrite(&isample, 1, 1, file);
				}
				filesample=0.0f;
			}
			file_sampleswritten++;
		}
	}
}

DPInput *input;
#ifdef WIN32
PortAudioStream *stream;
#endif
bool mute_stream;

#ifdef WIN32
//ancient portaudio stuff
static int AudioCallback(void *inputBuffer, void *outputBuffer,
						 unsigned long framesPerBuffer,
						 PaTimestamp outTime, void *userData)
{
	float *out=(float*)outputBuffer;
	float *in=(float*)inputBuffer;
	(void)outTime;

	if(havePlayingSample() && !mute_stream)
		SynthSample(framesPerBuffer, out, NULL);
	else
		for(int i=0;i<framesPerBuffer;i++)
			*out++=0.0f;
	
	return 0;
}
#else
//lets use SDL in stead
static void SDLAudioCallback(void *userdata, Uint8 *stream, int len)
{
	if (havePlayingSample() && !mute_stream)
	{
		unsigned int l = len/2;
		float fbuf[l];
		memset(fbuf, 0, sizeof(fbuf));
		SynthSample(l, fbuf, NULL);
		while (l--)
		{
			float f = fbuf[l];
			if (f < -1.0) f = -1.0;
			if (f > 1.0) f = 1.0;
			((Sint16*)stream)[l] = (Sint16)(f * 32767);
		}
	}
	else memset(stream, 0, len);
}
#endif

bool ExportWAV(char* filename)
{
	FILE* foutput=fopen(filename, "wb");
	if(!foutput)
		return false;
	// write wav header
	char string[32];
	unsigned int dword=0;
	unsigned short word=0;
	fwrite("RIFF", 4, 1, foutput); // "RIFF"
	dword=0;
	fwrite(&dword, 1, 4, foutput); // remaining file size
	fwrite("WAVE", 4, 1, foutput); // "WAVE"

	fwrite("fmt ", 4, 1, foutput); // "fmt "
	dword=16;
	fwrite(&dword, 1, 4, foutput); // chunk size
	word=1;
	fwrite(&word, 1, 2, foutput); // compression code
	word=1;
	fwrite(&word, 1, 2, foutput); // channels
	dword=wav_freq;
	fwrite(&dword, 1, 4, foutput); // sample rate
	dword=wav_freq*wav_bits/8;
	fwrite(&dword, 1, 4, foutput); // bytes/sec
	word=wav_bits/8;
	fwrite(&word, 1, 2, foutput); // block align
	word=wav_bits;
	fwrite(&word, 1, 2, foutput); // bits per sample

	fwrite("data", 4, 1, foutput); // "data"
	dword=0;
	int foutstream_datasize=ftell(foutput);
	fwrite(&dword, 1, 4, foutput); // chunk size

	// write sample data
	mute_stream=true;
	file_sampleswritten=0;
	filesample=0.0f;
	fileacc=0;
	PlaySample();
	while(havePlayingSample())
		SynthSample(256, NULL, foutput);
	mute_stream=false;

	// seek back to header and write size info
	fseek(foutput, 4, SEEK_SET);
	dword=0;
	dword=foutstream_datasize-4+file_sampleswritten*wav_bits/8;
	fwrite(&dword, 1, 4, foutput); // remaining file size
	fseek(foutput, foutstream_datasize, SEEK_SET);
	dword=file_sampleswritten*wav_bits/8;
	fwrite(&dword, 1, 4, foutput); // chunk size (data)
	fclose(foutput);
	
	return true;
}

#include "tools.h"

bool firstframe=true;
int refresh_counter=0;

void Slider(int x, int y, float& value, bool bipolar, const char* text)
{
	if(MouseInBox(x, y, 100, 10))
	{
		if(mouse_leftclick)
			vselected=&value;
		else if(mouse_rightclick)
			value=0.0f;
		else if(mouse_scrollup)
			value+=0.001f;
		else if(mouse_scrolldown)
			value-=0.001f;
	}
	float mv=(float)(mouse_x-mouse_px);
	if(vselected!=&value)
		mv=0.0f;
	if(bipolar)
	{
		value+=mv*0.005f;
		if(value<-1.0f) value=-1.0f;
		if(value>1.0f) value=1.0f;
	}
	else
	{
		value+=mv*0.0025f;
		if(value<0.0f) value=0.0f;
		if(value>1.0f) value=1.0f;
	}
	DrawBar(x-1, y, 102, 10, 0x000000);
	int ival=(int)(value*99);
	if(bipolar)
		ival=(int)(value*49.5f+49.5f);
	DrawBar(x, y+1, ival, 8, 0xF0C090);
	DrawBar(x+ival, y+1, 100-ival, 8, 0x807060);
	DrawBar(x+ival, y+1, 1, 8, 0xFFFFFF);
	if(bipolar)
	{
		DrawBar(x+50, y-1, 1, 3, 0x000000);
		DrawBar(x+50, y+8, 1, 3, 0x000000);
	}
	DWORD tcol=0x000000;
	if(channels[currentChannel].wave_type!=0 && (&value==&channels[currentChannel].p_duty || &value==&channels[currentChannel].p_duty_ramp))
		tcol=0x808080;
	DrawText(x-4-strlen(text)*8, y+1, tcol, text);
	char* strValue = ftoa(value);
	DrawText(x+100-4-strlen(strValue)*8, y+1, 0xFFFFFF, strValue);
}

bool Button(int x, int y, bool highlight, const char* text, int id)
{
	DWORD color1=0x000000;
	DWORD color2=0xA09088;
	DWORD color3=0x000000;
	bool hover=MouseInBox(x, y, strlen(text)*8+10, 17);
	if(hover && mouse_leftclick)
		vcurbutton=id;
	bool current=(vcurbutton==id);
	if(highlight)
	{
		color1=0x000000;
		color2=0x988070;
		color3=0xFFF0E0;
	}
	if(current && hover)
	{
		color1=0xA09088;
		color2=0xFFF0E0;
		color3=0xA09088;
	}
	DrawBar(x-1, y-1, strlen(text)*8+12, 19, color1);
	DrawBar(x, y, strlen(text)*8+10, 17, color2);
	DrawText(x+5, y+5, color3, text);
	if(current && hover && !mouse_left)
		return true;
	return false;
}

int drawcount=0;

void DrawScreen()
{

	bool redraw=true;
	if(!firstframe && mouse_x-mouse_px==0 && mouse_y-mouse_py==0 && !mouse_left && !mouse_right && !mouse_scrollup && !mouse_scrolldown)
		redraw=false;
	if(!mouse_left)
	{
		if(vselected!=NULL || vcurbutton>-1)
		{
			redraw=true;
			refresh_counter=2;
		}
		vselected=NULL;
	}
	if(refresh_counter>0)
	{
		refresh_counter--;
		redraw=true;
	}

	if(havePlayingSample())
		redraw=true;

	if(drawcount++>20)
	{
		redraw=true;
		drawcount=0;
	}

	if(!redraw)
		return;

	firstframe=false;

	ddkLock();

	ClearScreen(0xC0B090);

	DrawText(10, 10, 0x504030, "GENERATOR");
	for(int i=0;i<7;i++)
	{
		if(Button(5, 35+i*30, false, categories[i].name, 300+i))
		{
			switch(i)
			{
			case 0: // pickup/coin
				ResetParams(currentChannel);
				channels[currentChannel].p_base_freq=0.4f+frnd(0.5f);
				channels[currentChannel].p_env_attack=0.0f;
				channels[currentChannel].p_env_sustain=frnd(0.1f);
				channels[currentChannel].p_env_decay=0.1f+frnd(0.4f);
				channels[currentChannel].p_env_punch=0.3f+frnd(0.3f);
				if(rnd(1))
				{
					channels[currentChannel].p_arp_speed=0.5f+frnd(0.2f);
					channels[currentChannel].p_arp_mod=0.2f+frnd(0.4f);
				}
				break;
			case 1: // laser/shoot
				ResetParams(currentChannel);
				channels[currentChannel].wave_type=rnd(2);
				if(channels[currentChannel].wave_type==2 && rnd(1))
					channels[currentChannel].wave_type=rnd(1);
				channels[currentChannel].p_base_freq=0.5f+frnd(0.5f);
				channels[currentChannel].p_freq_limit=channels[currentChannel].p_base_freq-0.2f-frnd(0.6f);
				if(channels[currentChannel].p_freq_limit<0.2f) channels[currentChannel].p_freq_limit=0.2f;
				channels[currentChannel].p_freq_ramp=-0.15f-frnd(0.2f);
				if(rnd(2)==0)
				{
					channels[currentChannel].p_base_freq=0.3f+frnd(0.6f);
					channels[currentChannel].p_freq_limit=frnd(0.1f);
					channels[currentChannel].p_freq_ramp=-0.35f-frnd(0.3f);
				}
				if(rnd(1))
				{
					channels[currentChannel].p_duty=frnd(0.5f);
					channels[currentChannel].p_duty_ramp=frnd(0.2f);
				}
				else
				{
					channels[currentChannel].p_duty=0.4f+frnd(0.5f);
					channels[currentChannel].p_duty_ramp=-frnd(0.7f);
				}
				channels[currentChannel].p_env_attack=0.0f;
				channels[currentChannel].p_env_sustain=0.1f+frnd(0.2f);
				channels[currentChannel].p_env_decay=frnd(0.4f);
				if(rnd(1))
					channels[currentChannel].p_env_punch=frnd(0.3f);
				if(rnd(2)==0)
				{
					channels[currentChannel].p_pha_offset=frnd(0.2f);
					channels[currentChannel].p_pha_ramp=-frnd(0.2f);
				}
				if(rnd(1))
					channels[currentChannel].p_hpf_freq=frnd(0.3f);
				break;
			case 2: // explosion
				ResetParams(currentChannel);
				channels[currentChannel].wave_type=3;
				if(rnd(1))
				{
					channels[currentChannel].p_base_freq=0.1f+frnd(0.4f);
					channels[currentChannel].p_freq_ramp=-0.1f+frnd(0.4f);
				}
				else
				{
					channels[currentChannel].p_base_freq=0.2f+frnd(0.7f);
					channels[currentChannel].p_freq_ramp=-0.2f-frnd(0.2f);
				}
				channels[currentChannel].p_base_freq*=channels[currentChannel].p_base_freq;
				if(rnd(4)==0)
					channels[currentChannel].p_freq_ramp=0.0f;
				if(rnd(2)==0)
					channels[currentChannel].p_repeat_speed=0.3f+frnd(0.5f);
				channels[currentChannel].p_env_attack=0.0f;
				channels[currentChannel].p_env_sustain=0.1f+frnd(0.3f);
				channels[currentChannel].p_env_decay=frnd(0.5f);
				if(rnd(1)==0)
				{
					channels[currentChannel].p_pha_offset=-0.3f+frnd(0.9f);
					channels[currentChannel].p_pha_ramp=-frnd(0.3f);
				}
				channels[currentChannel].p_env_punch=0.2f+frnd(0.6f);
				if(rnd(1))
				{
					channels[currentChannel].p_vib_strength=frnd(0.7f);
					channels[currentChannel].p_vib_speed=frnd(0.6f);
				}
				if(rnd(2)==0)
				{
					channels[currentChannel].p_arp_speed=0.6f+frnd(0.3f);
					channels[currentChannel].p_arp_mod=0.8f-frnd(1.6f);
				}
				break;
			case 3: // powerup
				ResetParams(currentChannel);
				if(rnd(1))
					channels[currentChannel].wave_type=1;
				else
					channels[currentChannel].p_duty=frnd(0.6f);
				if(rnd(1))
				{
					channels[currentChannel].p_base_freq=0.2f+frnd(0.3f);
					channels[currentChannel].p_freq_ramp=0.1f+frnd(0.4f);
					channels[currentChannel].p_repeat_speed=0.4f+frnd(0.4f);
				}
				else
				{
					channels[currentChannel].p_base_freq=0.2f+frnd(0.3f);
					channels[currentChannel].p_freq_ramp=0.05f+frnd(0.2f);
					if(rnd(1))
					{
						channels[currentChannel].p_vib_strength=frnd(0.7f);
						channels[currentChannel].p_vib_speed=frnd(0.6f);
					}
				}
				channels[currentChannel].p_env_attack=0.0f;
				channels[currentChannel].p_env_sustain=frnd(0.4f);
				channels[currentChannel].p_env_decay=0.1f+frnd(0.4f);
				break;
			case 4: // hit/hurt
				ResetParams(currentChannel);
				channels[currentChannel].wave_type=rnd(2);
				if(channels[currentChannel].wave_type==2)
					channels[currentChannel].wave_type=3;
				if(channels[currentChannel].wave_type==0)
					channels[currentChannel].p_duty=frnd(0.6f);
				channels[currentChannel].p_base_freq=0.2f+frnd(0.6f);
				channels[currentChannel].p_freq_ramp=-0.3f-frnd(0.4f);
				channels[currentChannel].p_env_attack=0.0f;
				channels[currentChannel].p_env_sustain=frnd(0.1f);
				channels[currentChannel].p_env_decay=0.1f+frnd(0.2f);
				if(rnd(1))
					channels[currentChannel].p_hpf_freq=frnd(0.3f);
				break;
			case 5: // jump
				ResetParams(currentChannel);
				channels[currentChannel].wave_type=0;
				channels[currentChannel].p_duty=frnd(0.6f);
				channels[currentChannel].p_base_freq=0.3f+frnd(0.3f);
				channels[currentChannel].p_freq_ramp=0.1f+frnd(0.2f);
				channels[currentChannel].p_env_attack=0.0f;
				channels[currentChannel].p_env_sustain=0.1f+frnd(0.3f);
				channels[currentChannel].p_env_decay=0.1f+frnd(0.2f);
				if(rnd(1))
					channels[currentChannel].p_hpf_freq=frnd(0.3f);
				if(rnd(1))
					channels[currentChannel].p_lpf_freq=1.0f-frnd(0.6f);
				break;
			case 6: // blip/select
				ResetParams(currentChannel);
				channels[currentChannel].wave_type=rnd(1);
				if(channels[currentChannel].wave_type==0)
					channels[currentChannel].p_duty=frnd(0.6f);
				channels[currentChannel].p_base_freq=0.2f+frnd(0.4f);
				channels[currentChannel].p_env_attack=0.0f;
				channels[currentChannel].p_env_sustain=0.1f+frnd(0.1f);
				channels[currentChannel].p_env_decay=frnd(0.2f);
				channels[currentChannel].p_hpf_freq=0.1f;
				break;
			default:
				break;
			}

			PlaySample();
		}
	}
	DrawBar(110, 0, 2, 500, 0x000000);

	char buf[64];
	sprintf(buf, "CHANNEL: %i", currentChannel+1);
	DrawText(5, 255, 0x504030, buf);
	for(int row=0;row<4;row++)
		{
		for(int col=0;col<4;col++)
		{
			int id = row*4+col;
			if(Button(5+25*col, 280+row*25, channels[id].enabled, itoa(id+1), 500+id))
			{
				currentChannel = id;
				break;
			}
		}
	}
	if(Button(5, 380, channels[currentChannel].enabled, "EN-", 10))
		channels[currentChannel].enabled = true;
	if(Button(40, 380, !channels[currentChannel].enabled, "DISABLE", 10))
		channels[currentChannel].enabled = false;

	if(Button(130, 30, channels[currentChannel].wave_type==0, "SQUAREWAVE", 10))
		channels[currentChannel].wave_type=0;
	if(Button(250, 30, channels[currentChannel].wave_type==1, "SAWTOOTH", 11))
		channels[currentChannel].wave_type=1;
	if(Button(370, 30, channels[currentChannel].wave_type==2, "SINEWAVE", 12))
		channels[currentChannel].wave_type=2;
	if(Button(490, 30, channels[currentChannel].wave_type==3, "NOISE", 13))
		channels[currentChannel].wave_type=3;

	bool do_play=false;

	if(Button(5, 460, false, "RANDOMIZE", 40))
	{
		channels[currentChannel].p_base_freq=pow(frnd(2.0f)-1.0f, 2.0f);
		if(rnd(1))
			channels[currentChannel].p_base_freq=pow(frnd(2.0f)-1.0f, 3.0f)+0.5f;
		channels[currentChannel].p_freq_limit=0.0f;
		channels[currentChannel].p_freq_ramp=pow(frnd(2.0f)-1.0f, 5.0f);
		if(channels[currentChannel].p_base_freq>0.7f && channels[currentChannel].p_freq_ramp>0.2f)
			channels[currentChannel].p_freq_ramp=-channels[currentChannel].p_freq_ramp;
		if(channels[currentChannel].p_base_freq<0.2f && channels[currentChannel].p_freq_ramp<-0.05f)
			channels[currentChannel].p_freq_ramp=-channels[currentChannel].p_freq_ramp;
		channels[currentChannel].p_freq_dramp=pow(frnd(2.0f)-1.0f, 3.0f);
		channels[currentChannel].p_duty=frnd(2.0f)-1.0f;
		channels[currentChannel].p_duty_ramp=pow(frnd(2.0f)-1.0f, 3.0f);
		channels[currentChannel].p_vib_strength=pow(frnd(2.0f)-1.0f, 3.0f);
		channels[currentChannel].p_vib_speed=frnd(2.0f)-1.0f;
		channels[currentChannel].p_vib_delay=frnd(2.0f)-1.0f;
		channels[currentChannel].p_env_attack=pow(frnd(2.0f)-1.0f, 3.0f);
		channels[currentChannel].p_env_sustain=pow(frnd(2.0f)-1.0f, 2.0f);
		channels[currentChannel].p_env_decay=frnd(2.0f)-1.0f;
		channels[currentChannel].p_env_punch=pow(frnd(0.8f), 2.0f);
		if(channels[currentChannel].p_env_attack+channels[currentChannel].p_env_sustain+channels[currentChannel].p_env_decay<0.2f)
		{
			channels[currentChannel].p_env_sustain+=0.2f+frnd(0.3f);
			channels[currentChannel].p_env_decay+=0.2f+frnd(0.3f);
		}
		channels[currentChannel].p_lpf_resonance=frnd(2.0f)-1.0f;
		channels[currentChannel].p_lpf_freq=1.0f-pow(frnd(1.0f), 3.0f);
		channels[currentChannel].p_lpf_ramp=pow(frnd(2.0f)-1.0f, 3.0f);
		if(channels[currentChannel].p_lpf_freq<0.1f && channels[currentChannel].p_lpf_ramp<-0.05f)
			channels[currentChannel].p_lpf_ramp=-channels[currentChannel].p_lpf_ramp;
		channels[currentChannel].p_hpf_freq=pow(frnd(1.0f), 5.0f);
		channels[currentChannel].p_hpf_ramp=pow(frnd(2.0f)-1.0f, 5.0f);
		channels[currentChannel].p_pha_offset=pow(frnd(2.0f)-1.0f, 3.0f);
		channels[currentChannel].p_pha_ramp=pow(frnd(2.0f)-1.0f, 3.0f);
		channels[currentChannel].p_repeat_speed=frnd(2.0f)-1.0f;
		channels[currentChannel].p_arp_speed=frnd(2.0f)-1.0f;
		channels[currentChannel].p_arp_mod=frnd(2.0f)-1.0f;
		do_play=true;
	}

	if(Button(5, 435, false, "MUTATE", 30))
	{
		if(rnd(1)) channels[currentChannel].p_base_freq+=frnd(0.1f)-0.05f;
//		if(rnd(1)) channels[currentChannel].p_freq_limit+=frnd(0.1f)-0.05f;
		if(rnd(1)) channels[currentChannel].p_freq_ramp+=frnd(0.1f)-0.05f;
		if(rnd(1)) channels[currentChannel].p_freq_dramp+=frnd(0.1f)-0.05f;
		if(rnd(1)) channels[currentChannel].p_duty+=frnd(0.1f)-0.05f;
		if(rnd(1)) channels[currentChannel].p_duty_ramp+=frnd(0.1f)-0.05f;
		if(rnd(1)) channels[currentChannel].p_vib_strength+=frnd(0.1f)-0.05f;
		if(rnd(1)) channels[currentChannel].p_vib_speed+=frnd(0.1f)-0.05f;
		if(rnd(1)) channels[currentChannel].p_vib_delay+=frnd(0.1f)-0.05f;
		if(rnd(1)) channels[currentChannel].p_env_attack+=frnd(0.1f)-0.05f;
		if(rnd(1)) channels[currentChannel].p_env_sustain+=frnd(0.1f)-0.05f;
		if(rnd(1)) channels[currentChannel].p_env_decay+=frnd(0.1f)-0.05f;
		if(rnd(1)) channels[currentChannel].p_env_punch+=frnd(0.1f)-0.05f;
		if(rnd(1)) channels[currentChannel].p_lpf_resonance+=frnd(0.1f)-0.05f;
		if(rnd(1)) channels[currentChannel].p_lpf_freq+=frnd(0.1f)-0.05f;
		if(rnd(1)) channels[currentChannel].p_lpf_ramp+=frnd(0.1f)-0.05f;
		if(rnd(1)) channels[currentChannel].p_hpf_freq+=frnd(0.1f)-0.05f;
		if(rnd(1)) channels[currentChannel].p_hpf_ramp+=frnd(0.1f)-0.05f;
		if(rnd(1)) channels[currentChannel].p_pha_offset+=frnd(0.1f)-0.05f;
		if(rnd(1)) channels[currentChannel].p_pha_ramp+=frnd(0.1f)-0.05f;
		if(rnd(1)) channels[currentChannel].p_repeat_speed+=frnd(0.1f)-0.05f;
		if(rnd(1)) channels[currentChannel].p_arp_speed+=frnd(0.1f)-0.05f;
		if(rnd(1)) channels[currentChannel].p_arp_mod+=frnd(0.1f)-0.05f;
		do_play=true;
	}

	DrawText(490, 120, 0x000000, "MASTER VOLUME");
	DrawBar(490-1-1+60, 130-1+5, 70, 2, 0x000000);
	DrawBar(490-1-1+60+68, 130-1+5, 2, 205, 0x000000);
	DrawBar(490-1-1+60, 130-1, 42+2, 10+2, 0xFF0000);
	Slider(490, 130, master_vol, false, " ");

	DrawText(515, 170, 0x000000, "VOLUME");
	DrawBar(490-1-1+60, 180-1+5, 70, 2, 0x000000);
	DrawBar(490-1-1+60+68, 180-1+5, 2, 205, 0x000000);
	DrawBar(490-1-1+60, 180-1, 42+2, 10+2, 0xFF0000);
	Slider(490, 180, channels[currentChannel].sound_vol, false, " ");
	if(Button(490, 200, false, "PLAY SOUND", 20))
		PlaySample();

	if(Button(490, 290, false, "LOAD SOUND", 14))
	{
		char filename[256];
		if(FileSelectorLoad(hWndMain, filename, 1)) // WIN32
		{
			ResetParams();
			LoadSettings(filename);
			PlaySample();
		}
	}
	if(Button(490, 320, false, "SAVE SOUND", 15))
	{
		char filename[256];
		if(FileSelectorSave(hWndMain, filename, 1)) // WIN32
			SaveSettings(filename);
	}

	DrawBar(490-1-1+60, 380-1+9, 70, 2, 0x000000);
	DrawBar(490-1-2, 380-1-2, 102+4, 19+4, 0x000000);
	if(Button(490, 380, false, "EXPORT .WAV", 16))
	{
		char filename[256];
		if(FileSelectorSave(hWndMain, filename, 0)) // WIN32
			ExportWAV(filename);
	}
	char str[10];
	sprintf(str, "%i HZ", wav_freq);
	if(Button(490, 410, false, str, 18))
	{
		if(wav_freq==44100)
			wav_freq=22050;
		else
			wav_freq=44100;
	}
	sprintf(str, "%i-BIT", wav_bits);
	if(Button(490, 440, false, str, 19))
	{
		if(wav_bits==16)
			wav_bits=8;
		else
			wav_bits=16;
	}

	int ypos=4;

	int xpos=350;

	DrawBar(xpos-190, ypos*18-5, 300, 2, 0x0000000);

	Slider(xpos, (ypos++)*18, channels[currentChannel].p_env_attack, false, "ATTACK TIME");
	Slider(xpos, (ypos++)*18, channels[currentChannel].p_env_sustain, false, "SUSTAIN TIME");
	Slider(xpos, (ypos++)*18, channels[currentChannel].p_env_punch, false, "SUSTAIN PUNCH");
	Slider(xpos, (ypos++)*18, channels[currentChannel].p_env_decay, false, "DECAY TIME");
	Slider(xpos, (ypos++)*18, channels[currentChannel].p_env_delay, false, "DELAY");

	DrawBar(xpos-190, ypos*18-5, 300, 2, 0x0000000);

	Slider(xpos, (ypos++)*18, channels[currentChannel].p_base_freq, false, "START FREQUENCY");
	Slider(xpos, (ypos++)*18, channels[currentChannel].p_freq_limit, false, "MIN FREQUENCY");
	Slider(xpos, (ypos++)*18, channels[currentChannel].p_freq_ramp, true, "SLIDE");
	Slider(xpos, (ypos++)*18, channels[currentChannel].p_freq_dramp, true, "DELTA SLIDE");

	Slider(xpos, (ypos++)*18, channels[currentChannel].p_vib_strength, false, "VIBRATO DEPTH");
	Slider(xpos, (ypos++)*18, channels[currentChannel].p_vib_speed, false, "VIBRATO SPEED");

	DrawBar(xpos-190, ypos*18-5, 300, 2, 0x0000000);

	Slider(xpos, (ypos++)*18, channels[currentChannel].p_arp_mod, true, "CHANGE AMOUNT");
	Slider(xpos, (ypos++)*18, channels[currentChannel].p_arp_speed, false, "CHANGE SPEED");

	DrawBar(xpos-190, ypos*18-5, 300, 2, 0x0000000);

	Slider(xpos, (ypos++)*18, channels[currentChannel].p_duty, false, "SQUARE DUTY");
	Slider(xpos, (ypos++)*18, channels[currentChannel].p_duty_ramp, true, "DUTY SWEEP");

	DrawBar(xpos-190, ypos*18-5, 300, 2, 0x0000000);

	Slider(xpos, (ypos++)*18, channels[currentChannel].p_repeat_speed, false, "REPEAT SPEED");

	DrawBar(xpos-190, ypos*18-5, 300, 2, 0x0000000);

	Slider(xpos, (ypos++)*18, channels[currentChannel].p_pha_offset, true, "PHASER OFFSET");
	Slider(xpos, (ypos++)*18, channels[currentChannel].p_pha_ramp, true, "PHASER SWEEP");

	DrawBar(xpos-190, ypos*18-5, 300, 2, 0x0000000);

	Slider(xpos, (ypos++)*18, channels[currentChannel].p_lpf_freq, false, "LP FILTER CUTOFF");
	Slider(xpos, (ypos++)*18, channels[currentChannel].p_lpf_ramp, true, "LP FILTER CUTOFF SWEEP");
	Slider(xpos, (ypos++)*18, channels[currentChannel].p_lpf_resonance, false, "LP FILTER RESONANCE");
	Slider(xpos, (ypos++)*18, channels[currentChannel].p_hpf_freq, false, "HP FILTER CUTOFF");
	Slider(xpos, (ypos++)*18, channels[currentChannel].p_hpf_ramp, true, "HP FILTER CUTOFF SWEEP");

	DrawBar(xpos-190, ypos*18-5, 300, 2, 0x0000000);

	DrawBar(xpos-190, 4*18-5, 1, (ypos-4)*18, 0x0000000);
	DrawBar(xpos-190+299, 4*18-5, 1, (ypos-4)*18, 0x0000000);

	if(do_play)
		PlaySample();

	ddkUnlock();

	if(!mouse_left)
		vcurbutton=-1;
}

bool keydown=false;

bool ddkCalcFrame()
{
	input->Update(); // WIN32 (for keyboard input)

	if(input->KeyPressed(DIK_SPACE) || input->KeyPressed(DIK_RETURN)) // WIN32 (keyboard input only for convenience, ok to remove)
	{
		if(!keydown)
		{
			PlaySample();
			keydown=true;
		}
	}
	else
		keydown=false;

	DrawScreen();

	Sleep(5); // WIN32
	return true;
}

void ddkInit()
{
	srand(time(NULL));

	ddkSetMode(640, 500, 32, 60, DDK_WINDOW, "sfxr Enhanced"); // requests window size etc from ddrawkit

	if (LoadTGA(font, "/usr/share/sfxr/font.tga")) {
        	/* Try again in cwd */
		if (LoadTGA(font, "font.tga")) {
			fprintf(stderr,
				"Error could not open /usr/share/sfxr/font.tga"
				" nor font.tga\n");
			exit(1);
		}
	}

	if (LoadTGA(ld48, "/usr/share/sfxr/ld48.tga")) {
        	/* Try again in cwd */
		if (LoadTGA(ld48, "ld48.tga")) {
			fprintf(stderr,
				"Error could not open /usr/share/sfxr/ld48.tga"
				" nor ld48.tga\n");
			exit(1);
		}
	}

	ld48.width=ld48.pitch;

	input=new DPInput(hWndMain, hInstanceMain); // WIN32

	strcpy(categories[0].name, "PICKUP/COIN");
	strcpy(categories[1].name, "LASER/SHOOT");
	strcpy(categories[2].name, "EXPLOSION");
	strcpy(categories[3].name, "POWERUP");
	strcpy(categories[4].name, "HIT/HURT");
	strcpy(categories[5].name, "JUMP");
	strcpy(categories[6].name, "BLIP/SELECT");

	ResetParams();

#ifdef WIN32
	// Init PortAudio
	SetEnvironmentVariable("PA_MIN_LATENCY_MSEC", "75"); // WIN32
	Pa_Initialize();
	Pa_OpenDefaultStream(
				&stream,
				0,
				1,
				paFloat32,	// output type
				44100,
				512,		// samples per buffer
				0,			// # of buffers
				AudioCallback,
				NULL);
	Pa_StartStream(stream);
#else
	SDL_AudioSpec des;
	des.freq = 44100;
	des.format = AUDIO_S16SYS;
	des.channels = 1;
	des.samples = 512;
	des.callback = SDLAudioCallback;
	des.userdata = NULL;
	VERIFY(!SDL_OpenAudio(&des, NULL));
	SDL_PauseAudio(0);
#endif
}

void ddkFree()
{
	delete input;
	free(ld48.data);
	free(font.data);
	
#ifdef WIN32
	// Close PortAudio
    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();
#endif
}

