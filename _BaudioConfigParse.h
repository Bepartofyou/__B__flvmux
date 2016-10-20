#ifndef _Bepartofyou_AUDIO_CONFIG_PARSE_H_
#define _Bepartofyou_AUDIO_CONFIG_PARSE_H_

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>

#ifndef uint8_t
typedef  unsigned char     uint8_t;
#endif
#ifndef uint16_t
typedef  unsigned short    uint16_t;
#endif
#ifndef uint32_t
typedef  unsigned int      uint32_t;
#endif

namespace Bepartofyou{

	typedef struct{
		short syncword;
		short id;
		short layer;
		short protection_absent;
		short profile;
		short sampling_frequency_index;
		short private_bit;
		short channel_configuration;
		short original_copy;
		short home;
		//  short emphasis;
		short copyright_identification_bit;
		short copyright_identification_start;
		short aac_frame_length;
		short adts_buffer_fullness;
		short no_raw_data_blocks_in_frame;
		short crc_check;

		/* control param */
		short old_format;

	} STRU_ADTS_HEADER;

	class CAudioConfigInfo
	{
	public:
		CAudioConfigInfo();
		~CAudioConfigInfo();

	public:
		void GenerateSequenceHeader(uint8_t *pBuffer, uint32_t length);

		void GetAudioInfo(STRU_ADTS_HEADER *audioInfo)
		{
			memcpy((void*)audioInfo, &m_sAACInfo, sizeof(STRU_ADTS_HEADER));
		}

		void GetSequenceHeaderInfo(uint8_t *pBuffer, uint32_t &length)
		{
			memcpy(pBuffer, m_uSequenceHeaderInfo, 2);
			length = 2;
		}
	private:
		void adtsHeaderAnalysis(uint8_t *pBuffer, uint32_t length);

	private:
		STRU_ADTS_HEADER  m_sAACInfo;
		uint8_t m_uSequenceHeaderInfo[2];
	};

} // end namespace

#endif // !_Bepartofyou_AUDIO_CONFIG_PARSE_H_