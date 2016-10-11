/*
 * *Copyright(c) 2015-2016 Beijing Youbeizaixian Network technology co., LTD 
 * *
 * *Authored by Bepartofyou on: Sun Sep 18 17:52:12 CST 2016
 * *
 * * @desc:
 * *
 * * @history
 * */
#include "_BaudioConfigParse.h"

namespace Bepartofyou{

	CAudioConfigInfo::CAudioConfigInfo()
	{
		memset((void*)&m_sAACInfo, 0, sizeof(STRU_ADTS_HEADER));

		//默认值,针对44100 LC模式
		m_uSequenceHeaderInfo[0] = 0x12;
		m_uSequenceHeaderInfo[1] = 0x10;
	}

	CAudioConfigInfo::~CAudioConfigInfo()
	{

	}

	void CAudioConfigInfo::GenerateSequenceHeader(uint8_t *pBuffer, uint32_t length)
	{
		assert(length >= 7);
		adtsHeaderAnalysis(pBuffer, length);

		uint8_t tmp = 0;
		tmp |= (((uint8_t)m_sAACInfo.profile) << 3);
		tmp |= ((((uint8_t)m_sAACInfo.sampling_frequency_index) >> 1) & 0x07);
		m_uSequenceHeaderInfo[0] = tmp;

		tmp = 0;
		tmp |= (((uint8_t)m_sAACInfo.sampling_frequency_index) << 7);
		tmp |= (((uint8_t)m_sAACInfo.channel_configuration) << 3) & 0x78;
		m_uSequenceHeaderInfo[1] = tmp;

		printf("AudioCodecInfo::makeDecoderSpecificInfoEx()  %x %x", m_uSequenceHeaderInfo[0], m_uSequenceHeaderInfo[1]);
	}

	void CAudioConfigInfo::adtsHeaderAnalysis(uint8_t *pBuffer, uint32_t length)
	{
		if (((pBuffer[0] == 0xFF) && ((pBuffer[1] & 0xF1) == 0xF0)) | // 代表有crc校验，所以adts长度为9
			((pBuffer[0] == 0xFF) && ((pBuffer[1] & 0xF1) == 0xF1)))  // 代表没有crc校验，所以adts长度为7
		{
			m_sAACInfo.syncword = ((uint16_t)pBuffer[0] & 0xFFF0) >> 4;
			printf("AudioCodecInfo::AdtsHeaderAnalysis() m_sAACInfo:syncword  %d", m_sAACInfo.syncword);

			m_sAACInfo.id = ((uint16_t)pBuffer[1] & 0x8) >> 3;
			printf("AudioCodecInfo::AdtsHeaderAnalysis() m_sAACInfo:id  %d", m_sAACInfo.id);

			m_sAACInfo.layer = ((uint16_t)pBuffer[1] & 0x6) >> 1;
			printf("AudioCodecInfo::AdtsHeaderAnalysis() m_sAACInfo:layer  %d", m_sAACInfo.layer);

			m_sAACInfo.protection_absent = (uint16_t)pBuffer[1] & 0x1;
			printf("AudioCodecInfo::AdtsHeaderAnalysis() m_sAACInfo:protection_absent  %d", m_sAACInfo.protection_absent);

			m_sAACInfo.profile = (((uint16_t)pBuffer[2] & 0xc0) >> 6) + 1;
			printf("AudioCodecInfo::AdtsHeaderAnalysis() m_sAACInfo:profile  %d", m_sAACInfo.profile);

			m_sAACInfo.sampling_frequency_index = ((uint16_t)pBuffer[2] & 0x3c) >> 2;
			printf("AudioCodecInfo::AdtsHeaderAnalysis() m_sAACInfo:sampling_frequency_index  %d", m_sAACInfo.sampling_frequency_index);

			m_sAACInfo.private_bit = ((uint16_t)pBuffer[2] & 0x2) >> 1;
			printf("AudioCodecInfo::AdtsHeaderAnalysis() m_sAACInfo:pritvate_bit  %d", m_sAACInfo.private_bit);

			m_sAACInfo.channel_configuration = ((((uint16_t)pBuffer[2] & 0x1) << 2) | (((uint16_t)pBuffer[3] & 0xc0) >> 6));
			printf("AudioCodecInfo::AdtsHeaderAnalysis() m_sAACInfo:channel_configuration  %d", m_sAACInfo.channel_configuration);

			m_sAACInfo.original_copy = ((uint16_t)pBuffer[3] & 0x30) >> 5;
			printf("AudioCodecInfo::AdtsHeaderAnalysis() m_sAACInfo:original_copy  %d", m_sAACInfo.original_copy);

			m_sAACInfo.home = ((uint16_t)pBuffer[3] & 0x10) >> 4;
			printf("AudioCodecInfo::AdtsHeaderAnalysis() m_sAACInfo:home  %d", m_sAACInfo.home);

			//		m_sAACInfo.emphasis = ((unsigned short)pBuffer[3] & 0xc) >> 2;                      
			//printf("m_sAACInfo:emphasis  %d\n", m_sAACInfo.emphasis);
			m_sAACInfo.copyright_identification_bit = ((uint16_t)pBuffer[3] & 0x2) >> 1;
			printf("AudioCodecInfo::AdtsHeaderAnalysis() pAdm_sAACInfots_header:copyright_identification_bit  %d", m_sAACInfo.copyright_identification_bit);

			m_sAACInfo.copyright_identification_start = (uint16_t)pBuffer[3] & 0x1;
			printf("AudioCodecInfo::AdtsHeaderAnalysis() m_sAACInfo:copyright_identification_start  %d", m_sAACInfo.copyright_identification_start);

			m_sAACInfo.aac_frame_length = ((((uint16_t)pBuffer[4]) << 5) | (((uint16_t)pBuffer[5] & 0xf8) >> 3));
			printf("AudioCodecInfo::AdtsHeaderAnalysis() m_sAACInfo:aac_frame_length  %d", m_sAACInfo.aac_frame_length);

			m_sAACInfo.adts_buffer_fullness = (((uint16_t)pBuffer[5] & 0x7) | ((uint16_t)pBuffer[6]));
			printf("AudioCodecInfo::AdtsHeaderAnalysis() m_sAACInfo:adts_buffer_fullness  %d", m_sAACInfo.adts_buffer_fullness);

			m_sAACInfo.no_raw_data_blocks_in_frame = ((uint16_t)pBuffer[7] & 0xc0) >> 6;
			printf("AudioCodecInfo::AdtsHeaderAnalysis() m_sAACInfo:no_raw_data_blocks_in_frame  %d", m_sAACInfo.no_raw_data_blocks_in_frame);

			// The protection_absent bit indicates whether or not the header contains the two extra bytes
			if (m_sAACInfo.protection_absent == 0)
			{
				m_sAACInfo.crc_check = ((((uint16_t)pBuffer[7] & 0x3c) << 10) | (((uint16_t)pBuffer[8]) << 2) | (((uint16_t)pBuffer[9] & 0xc0) >> 6));
				printf("AudioCodecInfo::AdtsHeaderAnalysis() m_sAACInfo:crc_check  %d", m_sAACInfo.crc_check);
			}
		}
	}

} // end namespace



