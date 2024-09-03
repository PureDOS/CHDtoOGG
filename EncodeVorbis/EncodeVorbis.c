/*
Copyright (c) 2024 https://github.com/PureDOS
Copyright (c) 2002-2020 Xiph.org Foundation

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdlib.h>
#include <string.h>

#define OGG_IMPL
#define VORBIS_IMPL
#define OV_EXCLUDE_STATIC_CALLBACKS
#include "minivorbisenc.h"

extern uint32_t EncodeVorbisFeedSamples(float **buffer, uint32_t num);
extern void EncodeVorbisOutput(const void* data, uint32_t len);

void EncodeVorbis(int quality)
{
	ogg_stream_state os; /* take physical pages, weld into a logical stream of packets */
	ogg_page         og; /* one Ogg bitstream page.  Vorbis packets are inside */
	ogg_packet       op; /* one raw packet of data for decode */
	vorbis_info      vi; /* struct that stores all the static vorbis bitstream settings */
	vorbis_comment   vc; /* struct that stores all the user comments */
	vorbis_dsp_state vd; /* central working state for the packet->PCM decoder */
	vorbis_block     vb; /* local working space for packet->PCM decode */

	int eos = 0, ret;

	/********** Encode setup ************/
	vorbis_info_init(&vi);

	/* Encoding using a VBR quality mode.  The usable range is -.1
	   (lowest quality, smallest file) to 1. (highest quality, largest file).
	   Example quality mode .4: 44kHz stereo coupled, roughly 128kbps VBR */
	ret = vorbis_encode_init_vbr(&vi, 2, 44100, quality * 0.1f);

	/* do not continue if setup failed; this can happen if we ask for a
	   mode that libVorbis does not support (eg, too low a bitrate, etc,
	   will return 'OV_EIMPL') */
	if (ret) return;

	/* use empty comment */
	vorbis_comment_init(&vc);

	/* set up the analysis state and auxiliary encoding storage */
	vorbis_analysis_init(&vd, &vi);
	vorbis_block_init(&vd, &vb);

	/* set up our packet->stream encoder */
	#if 1
	/* Use fixed serial number 0 for deterministic output */
	enum { OGG_STREAM_SERIAL_NO = 0 };
	ogg_stream_init(&os, OGG_STREAM_SERIAL_NO);
	#else
	/* pick a random serial number; that way we can more likely build chained streams just by concatenation */
	srand(time(NULL));
	ogg_stream_init(&os, rand());
	#endif

	/* Vorbis streams begin with three headers; the initial header (with
	   most of the codec setup parameters) which is mandated by the Ogg
	   bitstream spec.  The second header holds any comment fields.  The
	   third header holds the bitstream codebook.  We merely need to
	   make the headers, then pass them to libvorbis one at a time;
	   libvorbis handles the additional Ogg bitstream constraints */
	{
		ogg_packet header;
		ogg_packet header_comm;
		ogg_packet header_code;

		vorbis_analysis_headerout(&vd, &vc, &header, &header_comm, &header_code);
		ogg_stream_packetin(&os, &header); /* automatically placed in its own page */
		ogg_stream_packetin(&os, &header_comm);
		ogg_stream_packetin(&os, &header_code);

		/* This ensures the actual
		 * audio data will start on a new page, as per spec
		 */
		while (ogg_stream_flush(&os, &og))
		{
			EncodeVorbisOutput(og.header, og.header_len);
			EncodeVorbisOutput(og.body, og.body_len);
		}
	}

	/* data to encode */
	for (uint32_t pos = 0; !eos;)
	{
		/* submit uninterleaved samples */
		vorbis_analysis_wrote(&vd, (int)EncodeVorbisFeedSamples(vorbis_analysis_buffer(&vd, (int)8192), 8192));

		/* vorbis does some data preanalysis, then divvies up blocks for more involved (potentially parallel) processing.  Get a single block for encoding now */
		while (vorbis_analysis_blockout(&vd, &vb) == 1)
		{
			/* analysis, assume we want to use bitrate management */
			vorbis_analysis(&vb, NULL);
			vorbis_bitrate_addblock(&vb);

			while (vorbis_bitrate_flushpacket(&vd, &op))
			{
				/* weld the packet into the bitstream */
				ogg_stream_packetin(&os, &op);

				/* write out pages (if any) */
				while (!eos)
				{
					if (ogg_stream_pageout(&os, &og) == 0) break;
					EncodeVorbisOutput(og.header, og.header_len);
					EncodeVorbisOutput(og.body, og.body_len);

					/* this could be set above, but for illustrative purposes, do it here (to show that vorbis does know where the stream ends) */
					if (ogg_page_eos(&og)) eos = 1;
				}
			}
		}
	}

	#ifndef ENCODE_VORBIS_SKIP_ALL_CLEANUP
	/* clean up and exit.  vorbis_info_clear() must be called last */
	ogg_stream_clear(&os);
	vorbis_block_clear(&vb);
	vorbis_dsp_clear(&vd);
	vorbis_comment_clear(&vc);
	vorbis_info_clear(&vi);
	#endif /* ENCODE_VORBIS_SKIP_ALL_CLEANUP */
}
