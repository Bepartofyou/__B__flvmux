#ifndef _Bepartofyou_VIDEO_CONFIG_PARSE_H_
#define _Bepartofyou_VIDEO_CONFIG_PARSE_H_

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

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

	enum {
		VV_NAL_UNKNOWN = 0,
		VV_NAL_SLICE = 1,
		VV_NAL_SLICE_DPA = 2,
		VV_NAL_SLICE_DPB = 3,
		VV_NAL_SLICE_DPC = 4,
		VV_NAL_SLICE_IDR = 5,
		VV_NAL_SEI = 6,
		VV_NAL_SPS = 7,
		VV_NAL_PPS = 8,
		VV_NAL_AUD = 9,
		VV_NAL_FILLER = 12,
	};

	class CVideoConfigInfo
	{
	public:
		CVideoConfigInfo();
		~CVideoConfigInfo();

	public:
		/* Helpers for parsing AVC NAL units.  */
		bool ParseAvcHeader(const uint8_t *pData, uint32_t size);
		bool IsAvcKeyframe(const uint8_t *pData, uint32_t size);
	private:
		bool hasStartCode(const uint8_t *data);
		void getSpsPps(const uint8_t *pData, uint32_t size);
		void memoryDuplicate(const uint8_t *pData, uint32_t size, bool bSps);

		const uint8_t* avcFindStartcode(const uint8_t *p, const uint8_t *end);
		const uint8_t* ff_avc_find_startcode_internal(const uint8_t *p, const uint8_t *end);
	public:
		uint8_t* GetSpsData(){ return m_pSpsData; }
		uint8_t* GetPpsData(){ return m_pPpsData; }

		uint32_t GetSpsSzie(){ return m_uSpsSize; }
		uint32_t GetPpsSzie(){ return m_uPpsSize; }

		uint32_t GetFps(){ return m_ufps; }
		uint32_t GetWidth(){ return m_uWidth; }
		uint32_t GetHeight(){ return m_uHeight; }
	private:
		uint32_t Ue(uint8_t *pBuff, uint32_t nLen, uint32_t &nStartBit);
		int Se(uint8_t *pBuff, uint32_t nLen, uint32_t &nStartBit);
		unsigned long u(uint32_t BitCount, uint8_t * buf, uint32_t &nStartBit);
		/**
		* H264的NAL起始码防竞争机制
		*
		* @param buf SPS数据内容
		* @无返回值 */
		void de_emulation_prevention(uint8_t* buf, uint32_t* buf_size);
		/**
		* 解码SPS,获取视频图像宽、高信息
		*
		* @param buf SPS数据内容
		* @param nLen SPS数据的长度
		* @param width 图像宽度
		* @param height 图像高度
		* @成功则返回1 ,失败则返回0 */
		int h264_decode_sps(uint8_t * buf, uint32_t nLen, uint32_t &width, uint32_t &height, uint32_t &fps);

	private:
		uint8_t*     m_pSpsData;
		uint32_t     m_uSpsSize;
		uint8_t*     m_pPpsData;
		uint32_t     m_uPpsSize;

		uint32_t     m_uWidth;
		uint32_t     m_uHeight;
		uint32_t     m_ufps;
	};

} // end namespace

#endif // !_Bepartofyou_VIDEO_CONFIG_PARSE_H_
