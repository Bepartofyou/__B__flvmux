#include "_BflvMuxer.h"

namespace Bepartofyou{

	CFlvMuxer::CFlvMuxer(const char* filename)
	{
		m_pFileName = NULL;
		m_pFile = NULL;

		m_pFileName = filename;
		if (m_pFileName)
		{
			m_pFile = fopen(m_pFileName, "wb");

			if (!m_pFile)
			{
				printf("CFlvMuxer::CFlvMuxer() open file error");
			}
		}

		m_pDataBuffer = (uint8_t*)malloc(sizeof(uint8_t)*MAX_FLV_DATA_BUFFER);
		m_uDataBufLen = 0;
		m_pVideoBuffer = (uint8_t*)malloc(sizeof(uint8_t)*MAX_FLV_DATA_BUFFER);
		m_uVideoBufLen = 0;

		m_uFilesize = 0;
		m_bFlag = false;
		m_uFirstTs = 0;
		m_uLastTs = 0;

		m_bHeaderOK = false;
		m_bMetaDataOK = false;
		m_bAudioSequenceHeaderOK = false;
		m_bVideoSequenceHeaderOK = false;
	}

	CFlvMuxer::~CFlvMuxer()
	{
		if (m_pFile)
		{
			//复写duration,fileszie...... 
			fseek(m_pFile, FLV_HEADER_SIZE, SEEK_SET);
			write_meta_data();
			fseek(m_pFile, 0, SEEK_END);

			fclose(m_pFile);
		}

		if (m_pVideoBuffer)
		{
			free(m_pVideoBuffer);
			m_pVideoBuffer = NULL;
		}
		if (m_pDataBuffer)
		{
			free(m_pDataBuffer);
			m_pDataBuffer = NULL;
		}
	}
	void CFlvMuxer::write_to_file()
	{
		//不是写文件的时候该接口无效
		if (!m_pFile)
			return;

		printf("CFlvMuxer::write_to_file() m_uDataBufLen: %u", m_uDataBufLen);

		m_uFilesize += m_uDataBufLen;

		fwrite(m_pDataBuffer, m_uDataBufLen, 1, m_pFile);
		m_uDataBufLen = 0;
	}

	void  CFlvMuxer::write_headers()
	{
		s_write((uint8_t*)"FLV", 3, true);
		s_w8(1, true);
		s_w8(5, true);
		s_wb32(9, true);
		s_wb32(0, true);

		write_to_file();
	}

	void CFlvMuxer::write_meta_data()
	{
		char body[4096] = { 0 };

		char * p = (char *)body;
		/*p = put_byte(p, VV_AMF_STRING);
		p = put_amf_string(p, "@setDataFrame");*/

		p = put_byte(p, VV_AMF_STRING);
		p = put_amf_string(p, "onMetaData");

		p = put_byte(p, VV_AMF_OBJECT);

		p = put_amf_string(p, "author");
		p = put_byte(p, VV_AMF_STRING);
		p = put_amf_string(p, "Bepartofyou");

		p = put_amf_string(p, "email");
		p = put_byte(p, VV_AMF_STRING);
		p = put_amf_string(p, "309554135@qq.com");

		p = put_amf_string(p, "company");
		p = put_byte(p, VV_AMF_STRING);
		p = put_amf_string(p, "Beijing  Network technology co., LTD");

		p = put_amf_string(p, "duration");
		p = put_amf_double(p, (m_uLastTs - m_uFirstTs) / 1000);

		p = put_amf_string(p, "filesize");
		p = put_amf_double(p, m_uFilesize);

		p = put_amf_string(p, "width");
		p = put_amf_double(p, m_cVideoConfigInfo.GetWidth());

		p = put_amf_string(p, "height");
		p = put_amf_double(p, m_cVideoConfigInfo.GetHeight());

		p = put_amf_string(p, "framerate");
		p = put_amf_double(p, m_cVideoConfigInfo.GetFps());

		//STRU_ADTS_HEADER audioInfo;
		//m_cAudioConfigInfo.GetAudioInfo(&audioInfo);

		//p = put_amf_string(p, "samplerate");
		//p = put_amf_double(p, audioInfo.sampling_frequency_index);

		//p = put_amf_string(p, "channel");
		//p = put_amf_double(p, audioInfo.channel_configuration);

		p = put_amf_string(p, "");
		p = put_byte(p, VV_AMF_OBJECT_END);

		uint32_t metaDataSize = p - body;

		s_w8(VV_RTMP_PACKET_TYPE_INFO, true);
		s_wb24((uint32_t)metaDataSize, true);
		s_wb32(0, true);
		s_wb24(0, true);
		s_write((uint8_t*)body, metaDataSize, true);

		s_wb32((uint32_t)(metaDataSize + 11), true);  //11 Ϊ tag header size

		write_to_file();
	}

	void CFlvMuxer::write_av_data(uint8_t *pData, uint32_t size, uint32_t dts, bool bAudio, uint8_t *pTagData, uint32_t &Tagsize)
	{
		if(!pData || !size || !pTagData)
		{
			Tagsize = 0;
			return;
		}

		if (0 != m_uDataBufLen)
			printf("CFlvMuxer::write_av_data() m_uDataBufLen:%u \n", m_uDataBufLen);
		if (size > MAX_FLV_DATA_BUFFER)
			printf("CFlvMuxer::write_av_data() size:%u > MAX_FLV_DATA_BUFFER:%u \n", size, MAX_FLV_DATA_BUFFER);

		//flv header
		if (!m_bHeaderOK) {
			write_headers();
			m_bHeaderOK = true;
		}
		//flv metadata
		if (!m_bMetaDataOK) {
			write_meta_data();
			m_bMetaDataOK = true;
		}
		//flv audio sequence header
		if (!m_bAudioSequenceHeaderOK && bAudio)
		{
			write_audio_header(pData, size);
			m_bAudioSequenceHeaderOK = true;
		}
		//flv video sequence header
		if (!m_bVideoSequenceHeaderOK && !bAudio)
		{
			//return;
			//在第一个IDR到来之前,前面的PB帧都丢掉
			if (!m_cVideoConfigInfo.IsAvcKeyframe(pData, size)){
				Tagsize = 0;
				return;
			}

			write_video_header(pData, size);
			m_bVideoSequenceHeaderOK = true;
		}
		//flv audio and video data
		if (!bAudio) {
			bool keyframe = false;
			parse_avc_packet(pData, size, keyframe);
			if (m_uVideoBufLen > 0)
			{
				write_packet(m_pVideoBuffer, m_uVideoBufLen, dts, false, keyframe, false);
				m_uVideoBufLen = 0;
			}
		}
		else {
			write_packet(pData, size, dts, false, false, true);
		}

		//返回tag数据
		if (Tagsize >= m_uDataBufLen)
		{
			Tagsize = m_uDataBufLen;
			memcpy(pTagData, m_pDataBuffer, m_uDataBufLen);
		}
		else
		{
			Tagsize = 0;
		}
		m_uDataBufLen = 0;
	}

	void CFlvMuxer::write_av_data(uint8_t *pData, uint32_t size, uint32_t dts, bool bAudio)
	{
		if(!pData || !size)
			return;

		//record timestamp 
		m_uLastTs = dts;
		if (!m_bFlag)
		{
			m_uFirstTs = dts;
			m_bFlag = true;
		}	

		//flv header
		if (!m_bHeaderOK) {
			write_headers();
			m_bHeaderOK = true;
		}
		//flv metadata
		if (!m_bMetaDataOK) {
			write_meta_data();
			m_bMetaDataOK = true;
		}
		//flv audio sequence header
		if (!m_bAudioSequenceHeaderOK && bAudio)
		{
			write_audio_header(pData, size);
			m_bAudioSequenceHeaderOK = true;
		}
		//flv video sequence header
		if (!m_bVideoSequenceHeaderOK && !bAudio)
		{
			//return;
			//在第一个IDR到来之前,前面的PB帧都丢掉
			if (!m_cVideoConfigInfo.IsAvcKeyframe(pData, size))
				return;

			write_video_header(pData, size);
			m_bVideoSequenceHeaderOK = true;
		}
		//flv audio and video data
		if (!bAudio) {
			bool keyframe = false;
			parse_avc_packet(pData, size, keyframe);
			if (m_uVideoBufLen > 0)
			{
				write_packet(m_pVideoBuffer, m_uVideoBufLen, dts, false, keyframe, false);
				m_uVideoBufLen = 0;
			}
		}
		else {
			write_packet(pData, size, dts, false, false, true);
		}
	}

	void CFlvMuxer::parse_avc_packet(uint8_t *pData, uint32_t size, bool &keyframe)
	{
		const uint8_t *nal_start, *nal_end;
		const uint8_t *end = pData + size;
		int type;

		nal_start = avcFindStartcode(pData, end);
		while (true) {
			while (nal_start < end && !*(nal_start++));

			if (nal_start == end)
				break;

			type = nal_start[0] & 0x1F;
			nal_end = avcFindStartcode(nal_start, end);

			if (type == VV_NAL_SLICE_IDR || type == VV_NAL_SLICE)
			{
				keyframe = (type == VV_NAL_SLICE_IDR);

				s_wb32((uint32_t)(nal_end - nal_start), false);
				s_write(const_cast<uint8_t *>(nal_start), nal_end - nal_start, false);
			}

			nal_start = nal_end;
		}
	}

	void CFlvMuxer::write_audio_header(uint8_t *pData, uint32_t size)
	{
		char configinfo[10] = { 0 };
		uint32_t configsize = 0;

		m_cAudioConfigInfo.GenerateSequenceHeader(pData, size);
		m_cAudioConfigInfo.GetSequenceHeaderInfo((uint8_t*)configinfo, configsize);

		write_packet((uint8_t*)configinfo, configsize, 0, true, false, true);
	}

	void CFlvMuxer::write_video_header(uint8_t *pData, uint32_t size)
	{
		m_cVideoConfigInfo.ParseAvcHeader(pData, size);

		s_w8(0x01, false);
		s_write(m_cVideoConfigInfo.GetSpsData() + 1, 3, false);
		s_w8(0xff, false);
		s_w8(0xe1, false);

		s_wb16((uint16_t)m_cVideoConfigInfo.GetSpsSzie(), false);
		s_write(m_cVideoConfigInfo.GetSpsData(), m_cVideoConfigInfo.GetSpsSzie(), false);
		s_w8(0x01, false);
		s_wb16((uint16_t)m_cVideoConfigInfo.GetPpsSzie(), false);
		s_write(m_cVideoConfigInfo.GetPpsData(), m_cVideoConfigInfo.GetPpsSzie(), false);

		write_packet(m_pVideoBuffer, m_uVideoBufLen, 0, true, true, false);
		m_uVideoBufLen = 0;
	}

	void CFlvMuxer::write_packet(uint8_t *pData, uint32_t size, uint32_t dts, bool bHeader, bool bKeyframe, bool bAudio)
	{
		if (!bAudio)
		{
			write_video(pData, size, dts, bHeader, bKeyframe);
		}
		else
		{
			write_audio(pData, size, dts, bHeader);
		}
	}

	void CFlvMuxer::write_video(uint8_t *pData, uint32_t size, uint32_t dts, bool bHeader, bool bKeyframe)
	{
		if (!pData || !size)
			return;

		s_w8(VV_RTMP_PACKET_TYPE_VIDEO, true);
		s_wb24((uint32_t)size + 5, true);
		s_wb24(dts, true);
		s_w8((dts >> 24) & 0x7F, true);
		s_wb24(0, true);

		/* these are the 5 extra bytes mentioned above */
		s_w8(bKeyframe ? 0x17 : 0x27, true);
		s_w8(bHeader ? 0 : 1, true);
		s_wb24(0, true);
		s_write(pData, size, true);

		/* write tag size (starting byte doesnt count) */
		s_wb32((uint32_t)(size + 5 + 11), true);  //11 Ϊ tag header size

		write_to_file();
	}

	void CFlvMuxer::write_audio(uint8_t *pData, uint32_t size, uint32_t dts, bool bHeader)
	{
		if (!pData || !size)
			return;

		//header is 0; data is remove adts 7 bytes
		uint32_t datapos = bHeader ? 0 : 7;  
		uint32_t datasize = bHeader ? size : size - 7;
		
		s_w8(VV_RTMP_PACKET_TYPE_AUDIO, true);
		s_wb24((uint32_t)datasize + 2, true);
		s_wb24(dts, true);
		s_w8((dts >> 24) & 0x7F, true);
		s_wb24(0, true);

		/* these are the two extra bytes mentioned above */
		s_w8(0xaf, true);
		s_w8(bHeader ? 0 : 1, true);
		s_write(&pData[datapos], datasize, true);

		/* write tag size (starting byte doesnt count) */
		s_wb32((uint32_t)(datasize + 2 + 11), true);  //11 Ϊ tag header size

		write_to_file();
	}

	const uint8_t* CFlvMuxer::avcFindStartcode(const uint8_t *p, const uint8_t *end)
	{
		const uint8_t *out = ff_avc_find_startcode_internal(p, end);
		if (p < out && out < end && !out[-1]) out--;
		return out;
	}

	/* NOTE: I noticed that FFmpeg does some unusual special handling of certain
	* scenarios that I was unaware of, so instead of just searching for {0, 0, 1}
	* we'll just use the code from FFmpeg - http://www.ffmpeg.org/ */
	const uint8_t* CFlvMuxer::ff_avc_find_startcode_internal(const uint8_t *p, const uint8_t *end)
	{
		const uint8_t *a = p + 4 - ((intptr_t)p & 3);

		for (end -= 3; p < a && p < end; p++) {
			if (p[0] == 0 && p[1] == 0 && p[2] == 1)
				return p;
		}

		for (end -= 3; p < end; p += 4) {
			uint32_t x = *(const uint32_t*)p;

			if ((x - 0x01010101) & (~x) & 0x80808080) {
				if (p[1] == 0) {
					if (p[0] == 0 && p[2] == 1)
						return p;
					if (p[2] == 0 && p[3] == 1)
						return p + 1;
				}

				if (p[3] == 0) {
					if (p[2] == 0 && p[4] == 1)
						return p + 2;
					if (p[4] == 0 && p[5] == 1)
						return p + 3;
				}
			}
		}

		for (end += 3; p < end; p++) {
			if (p[0] == 0 && p[1] == 0 && p[2] == 1)
				return p;
		}

		return end + 3;
	}

	char* CFlvMuxer::put_byte(char *output, uint8_t nVal)
	{
		output[0] = nVal;
		return output + 1;
	}
	char* CFlvMuxer::put_be16(char *output, uint16_t nVal)
	{
		output[1] = nVal & 0xff;
		output[0] = nVal >> 8;
		return output + 2;
	}
	char* CFlvMuxer::put_be24(char *output, uint32_t nVal)
	{
		output[2] = nVal & 0xff;
		output[1] = nVal >> 8;
		output[0] = nVal >> 16;
		return output + 3;
	}
	char* CFlvMuxer::put_be32(char *output, uint32_t nVal)
	{
		output[3] = nVal & 0xff;
		output[2] = nVal >> 8;
		output[1] = nVal >> 16;
		output[0] = nVal >> 24;
		return output + 4;
	}
	char*  CFlvMuxer::put_be64(char *output, uint64_t nVal)
	{
		output = put_be32(output, nVal >> 32);
		output = put_be32(output, nVal);
		return output;
	}
	char* CFlvMuxer::put_amf_string(char *c, const char *str)
	{
		uint16_t len = strlen(str);
		c = put_be16(c, len);
		memcpy(c, str, len);
		return c + len;
	}
	char* CFlvMuxer::put_amf_double(char *c, double d)
	{
		*c++ = VV_AMF_NUMBER;  /* type: Number */
		{
			unsigned char *ci, *co;
			ci = (unsigned char *)&d;
			co = (unsigned char *)c;
			co[0] = ci[7];
			co[1] = ci[6];
			co[2] = ci[5];
			co[3] = ci[4];
			co[4] = ci[3];
			co[5] = ci[2];
			co[6] = ci[1];
			co[7] = ci[0];
		}
		return c + 8;
	}

	uint32_t CFlvMuxer::s_write(uint8_t *pData, uint32_t size, bool flag)
	{
		uint8_t * tmp = flag ? m_pDataBuffer : m_pVideoBuffer;
		uint32_t& tmpsize = flag ? m_uDataBufLen : m_uVideoBufLen;

		if (tmp && pData && size && MAX_FLV_DATA_BUFFER - tmpsize > size)
		{
			memcpy(tmp + tmpsize, pData, size);
			tmpsize += size;

			return size;
		}
		else
		{
			printf("CFlvMuxer::s_write() memory is not enough !!! tmp:%p tmpsize:%u pData:%p size:%u flag:%d leftsize:%u m_uDataBufLen:%u \n",
									tmp, tmpsize, pData, size, flag, MAX_FLV_DATA_BUFFER - tmpsize, m_uDataBufLen);
			return 0;
		}
	}

	void CFlvMuxer::s_w8(uint8_t u8, bool flag)
	{
		s_write(&u8, sizeof(uint8_t), flag);
	}
	//big endian
	void CFlvMuxer::s_wb16(uint16_t u16, bool flag)
	{
		s_w8(u16 >> 8, flag);
		s_w8((uint8_t)u16, flag);
	}

	void CFlvMuxer::s_wb24(uint32_t u24, bool flag)
	{
		s_wb16((uint16_t)(u24 >> 8), flag);
		s_w8((uint8_t)u24, flag);
	}

	void CFlvMuxer::s_wb32(uint32_t u32, bool flag)
	{
		s_wb16((uint16_t)(u32 >> 16), flag);
		s_wb16((uint16_t)u32, flag);
	}

	void CFlvMuxer::s_wb64(uint64_t u64, bool flag)
	{
		s_wb32((uint32_t)(u64 >> 32), flag);
		s_wb32((uint32_t)u64, flag);
	}

	void CFlvMuxer::s_wbf(float f, bool flag)
	{
		s_wb32(*(uint32_t*)&f, flag);
	}

	void CFlvMuxer::s_wbd(double d, bool flag)
	{
		s_wb64(*(uint64_t*)&d, flag);
	}
	//little endian
	void CFlvMuxer::s_wl16(uint16_t u16, bool flag)
	{
		s_w8((uint8_t)u16, flag);
		s_w8(u16 >> 8, flag);
	}

	void CFlvMuxer::s_wl24(uint32_t u24, bool flag)
	{
		s_w8((uint8_t)u24, flag);
		s_wl16((uint16_t)(u24 >> 8), flag);
	}

	void CFlvMuxer::s_wl32(uint32_t u32, bool flag)
	{
		s_wl16((uint16_t)u32, flag);
		s_wl16((uint16_t)(u32 >> 16), flag);
	}

	void CFlvMuxer::s_wl64(uint64_t u64, bool flag)
	{
		s_wl32((uint32_t)u64, flag);
		s_wl32((uint32_t)(u64 >> 32), flag);
	}

	void CFlvMuxer::s_wlf(float f, bool flag)
	{
		s_wl32(*(uint32_t*)&f, flag);
	}

	void CFlvMuxer::s_wld(double d, bool flag)
	{
		s_wl64(*(uint64_t*)&d, flag);
	}

} // end namespace
