#include "audio-encoder.h"
#include "opus/opus.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct opus_encoder_t
{
	OpusEncoder* opus;
	struct avpacket_t pkt;
	size_t capacity;
};

static void opus_destroy(void* audio)
{
	struct opus_encoder_t* enc;
	enc = (struct opus_encoder_t*)audio;
	if (enc->opus)
	{
		opus_encoder_destroy(enc->opus);
		enc->opus = NULL;
	}
	free(enc);
}

static void* opus_create(const struct audio_parameter_t* param)
{
	int r = OPUS_OK;
	struct opus_encoder_t* enc;

	r = param->frequency / 2 * param->channels * PCM_SAMPLE_BITS(param->format) / 8; // 500ms
	enc = (struct opus_encoder_t*)malloc(sizeof(*enc) + r);
	if (NULL == enc)
		return NULL;

	memset(enc, 0, sizeof(*enc));
	enc->pkt.data = (uint8_t*)(enc + 1);
	enc->capacity = r;
	enc->opus = opus_encoder_create(param->frequency, param->channels, OPUS_APPLICATION_VOIP, &r);
	if (NULL == enc->opus)
	{
		opus_destroy(enc);
		return NULL;
	}
	return enc;
}

static int opus_input(void* audio, const struct avframe_t* pic)
{
	struct opus_encoder_t* enc;
	enc = (struct opus_encoder_t*)audio;
	enc->pkt.bytes = opus_encode(enc->opus, (const opus_int16*)pic->data[0], pic->samples, enc->pkt.data, enc->capacity);
	if (enc->pkt.bytes > 0)
	{
		enc->pkt.flags = 0;
		enc->pkt.pic_type = 0;
		enc->pkt.pts = pic->pts;
		enc->pkt.dts = pic->dts;
	}
	return enc->pkt.bytes;
}

static int opus_getpacket(void* audio, struct avpacket_t* pkt)
{
	struct opus_encoder_t* enc;
	enc = (struct opus_encoder_t*)audio;
	if (enc->pkt.bytes > 0)
	{
		memcpy(pkt, &enc->pkt, sizeof(*pkt));
		enc->pkt.bytes = 0; // clear
		return 0;
	}
	return -1;
}

struct audio_encoder_t* opus_encoder()
{
	static struct audio_encoder_t s_encoder = {
		opus_create,
		opus_destroy,
		opus_input,
		opus_getpacket,
	};
	return &s_encoder;
}