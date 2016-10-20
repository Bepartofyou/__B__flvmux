#include "_BvideoConfigParse.h"

namespace Bepartofyou{

	CVideoConfigInfo::CVideoConfigInfo()
	{
		m_pSpsData = NULL;
		m_uSpsSize = 0;
		m_pPpsData = NULL;
		m_uPpsSize = 0;

		m_uWidth = 0;
		m_uHeight = 0;
		m_ufps = 0;
	}

	CVideoConfigInfo::~CVideoConfigInfo()
	{
		if (m_pSpsData)
		{
			free(m_pSpsData);
			m_pSpsData = NULL;
		}
		if (m_pPpsData)
		{
			free(m_pPpsData);
			m_pPpsData = NULL;
		}
	}

	bool CVideoConfigInfo::ParseAvcHeader(const uint8_t *pData, uint32_t size)
	{
		if (size <= 6) 
			return false;

		if (!hasStartCode(pData))
			return false;

		this->getSpsPps(pData, size);
		if (!m_pSpsData || !m_pPpsData || m_uSpsSize < 4)
			return false;

		h264_decode_sps(m_pSpsData, m_uSpsSize, m_uWidth, m_uHeight, m_ufps);

		return true;
	}

	bool CVideoConfigInfo::hasStartCode(const uint8_t *pdata)
	{
		if (pdata[0] != 0 || pdata[1] != 0)
			return false;

		return pdata[2] == 1 || (pdata[2] == 0 && pdata[3] == 1);
	}

	void CVideoConfigInfo::getSpsPps(const uint8_t *pData, uint32_t size)
	{
		const uint8_t *nal_start, *nal_end;
		const uint8_t *end = pData + size;
		int type;

		nal_start = avcFindStartcode(pData, end);
		while (true) {
			while (nal_start < end && !*(nal_start++));

			if (nal_start == end)
				break;

			nal_end = avcFindStartcode(nal_start, end);

			type = nal_start[0] & 0x1F;
			if (type == VV_NAL_SPS) {

				m_uSpsSize = nal_end - nal_start;
				memoryDuplicate(nal_start, m_uSpsSize, true);
			}
			else if (type == VV_NAL_PPS) {

				m_uPpsSize = nal_end - nal_start;
				memoryDuplicate(nal_start, m_uPpsSize, false);
			}

			nal_start = nal_end;
		}
	}

	const uint8_t* CVideoConfigInfo::avcFindStartcode(const uint8_t *p, const uint8_t *end)
	{
		const uint8_t *out = ff_avc_find_startcode_internal(p, end);
		if (p < out && out < end && !out[-1]) out--;
		return out;
	}

	/* NOTE: I noticed that FFmpeg does some unusual special handling of certain
	* scenarios that I was unaware of, so instead of just searching for {0, 0, 1}
	* we'll just use the code from FFmpeg - http://www.ffmpeg.org/ */
	const uint8_t* CVideoConfigInfo::ff_avc_find_startcode_internal(const uint8_t *p, const uint8_t *end)
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

	void CVideoConfigInfo::memoryDuplicate(const uint8_t *pData, uint32_t size, bool bSps)
	{
		if (bSps)
		{
			if (m_pSpsData)
			{
				free(m_pSpsData);
				m_pSpsData = NULL;
			}
			if (!m_pSpsData)
			{
				m_pSpsData = (uint8_t*)malloc(size*sizeof(uint8_t));
				memcpy(m_pSpsData, pData, size);
			}
		}
		else
		{
			if (m_pPpsData)
			{
				free(m_pPpsData);
				m_pPpsData = NULL;
			}
			if (!m_pPpsData)
			{
				m_pPpsData = (uint8_t*)malloc(size*sizeof(uint8_t));
				memcpy(m_pPpsData, pData, size);
			}
		}
	}

	bool CVideoConfigInfo::IsAvcKeyframe(const uint8_t *pData, uint32_t size)
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

			if (type == VV_NAL_SLICE_IDR || type == VV_NAL_SLICE)
				return (type == VV_NAL_SLICE_IDR);

			nal_end = avcFindStartcode(nal_start, end);
			nal_start = nal_end;
		}
		return false;
	}

	uint32_t CVideoConfigInfo::Ue(uint8_t *pBuff, uint32_t nLen, uint32_t &nStartBit)
	{
		//计算0bit的个数
		uint32_t nZeroNum = 0;
		while (nStartBit < nLen * 8)
		{
			if (pBuff[nStartBit / 8] & (0x80 >> (nStartBit % 8))) 
			{
				break;
			}
			nZeroNum++;
			nStartBit++;
		}
		nStartBit++;

		//计算结果
		unsigned long dwRet = 0;
		for (uint32_t i = 0; i < nZeroNum; i++)
		{
			dwRet <<= 1;
			if (pBuff[nStartBit / 8] & (0x80 >> (nStartBit % 8)))
			{
				dwRet += 1;
			}
			nStartBit++;
		}
		return (1 << nZeroNum) - 1 + dwRet;
	}

	int CVideoConfigInfo::Se(uint8_t *pBuff, uint32_t nLen, uint32_t &nStartBit)
	{
		int UeVal = Ue(pBuff, nLen, nStartBit);
		double k = UeVal;
		int nValue = ceil(k / 2);//ceil函数:ceil函数的作用是求不小于给定实数的最小整数.ceil(2)=ceil(1.2)=cei(1.5)=2.00
		if (UeVal % 2 == 0)
			nValue = -nValue;
		return nValue;
	}

	unsigned long CVideoConfigInfo::u(uint32_t BitCount, uint8_t * buf, uint32_t &nStartBit)
	{
		unsigned long dwRet = 0;
		for (uint32_t i = 0; i < BitCount; i++)
		{
			dwRet <<= 1;
			if (buf[nStartBit / 8] & (0x80 >> (nStartBit % 8)))
			{
				dwRet += 1;
			}
			nStartBit++;
		}
		return dwRet;
	}

	void CVideoConfigInfo::de_emulation_prevention(uint8_t* buf, uint32_t* buf_size)
	{
		size_t i = 0, j = 0;
		uint8_t* tmp_ptr = NULL;
		uint32_t tmp_buf_size = 0;
		int val = 0;

		tmp_ptr = buf;
		tmp_buf_size = *buf_size;
		for (i = 0; i < (tmp_buf_size - 2); i++)
		{
			//check for 0x000003
			val = (tmp_ptr[i] ^ 0x00) + (tmp_ptr[i + 1] ^ 0x00) + (tmp_ptr[i + 2] ^ 0x03);
			if (val == 0)
			{
				//kick out 0x03
				for (j = i + 2; j < tmp_buf_size - 1; j++)
					tmp_ptr[j] = tmp_ptr[j + 1];

				//and so we should devrease bufsize
				(*buf_size)--;
			}
		}

		return;
	}

	int CVideoConfigInfo::h264_decode_sps(uint8_t * buf, uint32_t nLen, uint32_t &width, uint32_t &height, uint32_t &fps)
	{
		uint32_t StartBit = 0;
		fps = 0;
		de_emulation_prevention(buf, &nLen);

		int forbidden_zero_bit;
		int nal_ref_idc;
		int nal_unit_type;
		int profile_idc;
		int constraint_set0_flag;
		int constraint_set1_flag;
		int constraint_set2_flag;
		int constraint_set3_flag;
		int reserved_zero_4bits;
		int level_idc;
		int seq_parameter_set_id;
		int chroma_format_idc;
		int residual_colour_transform_flag;
		int bit_depth_luma_minus8;
		int bit_depth_chroma_minus8;
		int qpprime_y_zero_transform_bypass_flag;
		int seq_scaling_matrix_present_flag;
		int seq_scaling_list_present_flag[8];
		int log2_max_frame_num_minus4;
		int pic_order_cnt_type;
		int log2_max_pic_order_cnt_lsb_minus4;
		int delta_pic_order_always_zero_flag;
		int offset_for_non_ref_pic;
		int offset_for_top_to_bottom_field;
		int num_ref_frames_in_pic_order_cnt_cycle;
		int num_ref_frames;
		int gaps_in_frame_num_value_allowed_flag;
		int pic_width_in_mbs_minus1;
		int pic_height_in_map_units_minus1;
		int frame_mbs_only_flag;
		int mb_adaptive_frame_field_flag;
		int direct_8x8_inference_flag;
		int frame_cropping_flag;
		int frame_crop_left_offset;
		int frame_crop_right_offset;
		int frame_crop_top_offset;
		int frame_crop_bottom_offset;
		int vui_parameter_present_flag;
		int aspect_ratio_info_present_flag;
		int aspect_ratio_idc;
		int sar_width;
		int sar_height;
		int overscan_info_present_flag;
		int overscan_appropriate_flagu;
		int video_signal_type_present_flag;
		int video_format;
		int video_full_range_flag;
		int colour_description_present_flag;
		int colour_primaries;
		int transfer_characteristics;
		int matrix_coefficients;
		int chroma_loc_info_present_flag;
		int chroma_sample_loc_type_top_field;
		int chroma_sample_loc_type_bottom_field;
		int timing_info_present_flag;
		int num_units_in_tick;
		int time_scale;

		forbidden_zero_bit = u(1, buf, StartBit);
		nal_ref_idc = u(2, buf, StartBit);
		nal_unit_type = u(5, buf, StartBit);
		if (nal_unit_type == 7)
		{
			profile_idc = u(8, buf, StartBit);
			constraint_set0_flag = u(1, buf, StartBit);//(buf[1] & 0x80)>>7;
			constraint_set1_flag = u(1, buf, StartBit);//(buf[1] & 0x40)>>6;
			constraint_set2_flag = u(1, buf, StartBit);//(buf[1] & 0x20)>>5;
			constraint_set3_flag = u(1, buf, StartBit);//(buf[1] & 0x10)>>4;
			reserved_zero_4bits = u(4, buf, StartBit);
			level_idc = u(8, buf, StartBit);

			seq_parameter_set_id = Ue(buf, nLen, StartBit);

			if (profile_idc == 100 || profile_idc == 110 ||
				profile_idc == 122 || profile_idc == 144)
			{
				chroma_format_idc = Ue(buf, nLen, StartBit);
				if (chroma_format_idc == 3)
					residual_colour_transform_flag = u(1, buf, StartBit);
				bit_depth_luma_minus8 = Ue(buf, nLen, StartBit);
				bit_depth_chroma_minus8 = Ue(buf, nLen, StartBit);
				qpprime_y_zero_transform_bypass_flag = u(1, buf, StartBit);
				seq_scaling_matrix_present_flag = u(1, buf, StartBit);

				if (seq_scaling_matrix_present_flag)
				{
					for (int i = 0; i < 8; i++) {
						seq_scaling_list_present_flag[i] = u(1, buf, StartBit);
					}
				}
			}
			log2_max_frame_num_minus4 = Ue(buf, nLen, StartBit);
			pic_order_cnt_type = Ue(buf, nLen, StartBit);
			if (pic_order_cnt_type == 0)
				log2_max_pic_order_cnt_lsb_minus4 = Ue(buf, nLen, StartBit);
			else if (pic_order_cnt_type == 1)
			{
				delta_pic_order_always_zero_flag = u(1, buf, StartBit);
				offset_for_non_ref_pic = Se(buf, nLen, StartBit);
				offset_for_top_to_bottom_field = Se(buf, nLen, StartBit);
				num_ref_frames_in_pic_order_cnt_cycle = Ue(buf, nLen, StartBit);

				int *offset_for_ref_frame = new int[num_ref_frames_in_pic_order_cnt_cycle];
				for (int i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++)
					offset_for_ref_frame[i] = Se(buf, nLen, StartBit);
				delete[] offset_for_ref_frame;
			}
			num_ref_frames = Ue(buf, nLen, StartBit);
			gaps_in_frame_num_value_allowed_flag = u(1, buf, StartBit);
			pic_width_in_mbs_minus1 = Ue(buf, nLen, StartBit);
			pic_height_in_map_units_minus1 = Ue(buf, nLen, StartBit);

			frame_mbs_only_flag = u(1, buf, StartBit);
			if (!frame_mbs_only_flag)
				mb_adaptive_frame_field_flag = u(1, buf, StartBit);

			direct_8x8_inference_flag = u(1, buf, StartBit);
			frame_cropping_flag = u(1, buf, StartBit);
			if (frame_cropping_flag)
			{
				frame_crop_left_offset = Ue(buf, nLen, StartBit);
				frame_crop_right_offset = Ue(buf, nLen, StartBit);
				frame_crop_top_offset = Ue(buf, nLen, StartBit);
				frame_crop_bottom_offset = Ue(buf, nLen, StartBit);
			}

			//计算视频分辨率,有的视频尺寸可能不是完整划分,所以需要offset计算
			width = (pic_width_in_mbs_minus1 + 1) * 16;
			height = (2 - frame_mbs_only_flag) * (pic_height_in_map_units_minus1 + 1) * 16;
			if (frame_cropping_flag)
			{
				uint32_t crop_unit_x;
				uint32_t crop_unit_y;
				if (0 == chroma_format_idc) // monochrome
				{
					crop_unit_x = 1;
					crop_unit_y = 2 - frame_mbs_only_flag;
				}
				else if (1 == chroma_format_idc) // 4:2:0
				{
					crop_unit_x = 2;
					crop_unit_y = 2 * (2 - frame_mbs_only_flag);
				}
				else if (2 == chroma_format_idc) // 4:2:2
				{
					crop_unit_x = 2;
					crop_unit_y = 2 - frame_mbs_only_flag;
				}
				else // 3 == chroma_format_idc   // 4:4:4
				{
					crop_unit_x = 1;
					crop_unit_y = 2 - frame_mbs_only_flag;
				}

				width -= crop_unit_x * (frame_crop_left_offset + frame_crop_right_offset);
				height -= crop_unit_y * (frame_crop_top_offset + frame_crop_bottom_offset);
			}

			vui_parameter_present_flag = u(1, buf, StartBit);
			if (vui_parameter_present_flag)
			{
				aspect_ratio_info_present_flag = u(1, buf, StartBit);
				if (aspect_ratio_info_present_flag)
				{
					aspect_ratio_idc = u(8, buf, StartBit);
					if (aspect_ratio_idc == 255)
					{
						sar_width = u(16, buf, StartBit);
						sar_height = u(16, buf, StartBit);
					}
				}
				overscan_info_present_flag = u(1, buf, StartBit);
				if (overscan_info_present_flag)
					overscan_appropriate_flagu = u(1, buf, StartBit);
				video_signal_type_present_flag = u(1, buf, StartBit);
				if (video_signal_type_present_flag)
				{
					video_format = u(3, buf, StartBit);
					video_full_range_flag = u(1, buf, StartBit);
					colour_description_present_flag = u(1, buf, StartBit);
					if (colour_description_present_flag)
					{
						colour_primaries = u(8, buf, StartBit);
						transfer_characteristics = u(8, buf, StartBit);
						matrix_coefficients = u(8, buf, StartBit);
					}
				}
				chroma_loc_info_present_flag = u(1, buf, StartBit);
				if (chroma_loc_info_present_flag)
				{
					chroma_sample_loc_type_top_field = Ue(buf, nLen, StartBit);
					chroma_sample_loc_type_bottom_field = Ue(buf, nLen, StartBit);
				}
				timing_info_present_flag = u(1, buf, StartBit);
				if (timing_info_present_flag)
				{
					num_units_in_tick = u(32, buf, StartBit);
					time_scale = u(32, buf, StartBit);
					fps = time_scale / (2 * num_units_in_tick);
				}
			}
			return true;
		}
		else
			return false;
	}

} // end namespace
