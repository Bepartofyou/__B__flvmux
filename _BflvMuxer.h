/*
 * * @desc: 使用方法
 * *  1、写文件           CFlvMuxer flvmuxer("test.flv"); 
 * *			         flvmuxer.write_av_data(uint8_t *pData, uint32_t size, uint32_t dts, bool bAudio); 
 * *
 * *  2、返回tag数据      CFlvMuxer flvmuxer(NULL); 
 * *                     flvmuxer.write_av_data(uint8_t *pData, uint32_t size, uint32_t dts, bool bAudio, uint8_t *pTagData, uint32_t &Tagsize);
 * *
 * * @history
 * */
#ifndef _Bepartofyou_FLV_MUXER_H_
#define _Bepartofyou_FLV_MUXER_H_

#include "_BaudioConfigParse.h"
#include "_BvideoConfigParse.h"

#define  FLV_HEADER_SIZE 13
#define  MAX_FLV_DATA_BUFFER 1024 * 1024 * 2  

#define  VV_RTMP_PACKET_TYPE_AUDIO 0x08
#define  VV_RTMP_PACKET_TYPE_VIDEO 0x09
#define  VV_RTMP_PACKET_TYPE_INFO  0x12

namespace Bepartofyou{

	enum VV_AMFDataType{
		VV_AMF_NUMBER = 0, VV_AMF_BOOLEAN, VV_AMF_STRING, VV_AMF_OBJECT, VV_AMF_MOVIECLIP,		/* reserved, not used */
		VV_AMF_NULL, VV_AMF_UNDEFINED, VV_AMF_REFERENCE, VV_AMF_ECMA_ARRAY, VV_AMF_OBJECT_END,
		VV_AMF_STRICT_ARRAY, VV_AMF_DATE, VV_AMF_LONG_STRING, VV_AMF_UNSUPPORTED, VV_AMF_RECORDSET,		/* reserved, not used */
		VV_AMF_XML_DOC, VV_AMF_TYPED_OBJECT, VV_AMF_AVMPLUS,		/* switch to AMF3 */
		VV_AMF_INVALID = 0xff
	};

	class CFlvMuxer
	{
	public:
		CFlvMuxer(const char* filename);
		~CFlvMuxer();

	public:
		void write_av_data(uint8_t *pData, uint32_t size, uint32_t dts, bool bAudio);                                        //写入文件
		void write_av_data(uint8_t *pData, uint32_t size, uint32_t dts, bool bAudio, uint8_t *pTagData, uint32_t &Tagsize);  //返回tag数据

	private:
		void write_to_file();
		void write_headers();
		void write_meta_data();
		void write_audio_header(uint8_t *pData, uint32_t size);
		void write_video_header(uint8_t *pData, uint32_t size);
		void parse_avc_packet(uint8_t *pData, uint32_t size, bool &keyframe);
		
		void write_audio(uint8_t *pData, uint32_t size, uint32_t dts, bool bHeader);
		void write_video(uint8_t *pData, uint32_t size, uint32_t dts, bool bHeader, bool bKeyframe);
		void write_packet(uint8_t *pData, uint32_t size, uint32_t dts, bool bHeader, bool bKeyframe, bool bAudio);

		const uint8_t* avcFindStartcode(const uint8_t *p, const uint8_t *end);
		const uint8_t* ff_avc_find_startcode_internal(const uint8_t *p, const uint8_t *end);
	private:
		char* put_byte(char *output, uint8_t nVal);
		char* put_be16(char *output, uint16_t nVal);
		char* put_be24(char *output, uint32_t nVal);
		char* put_be32(char *output, uint32_t nVal);
		char* put_be64(char *output, uint64_t nVal);
		char* put_amf_string(char *c, const char *str);
		char* put_amf_double(char *c, double d);
	private:
		uint32_t s_write(uint8_t *pData, uint32_t size, bool flag);
		void s_w8(uint8_t u8, bool flag);
		//big endian
		void s_wb16(uint16_t u16, bool flag);
		void s_wb24(uint32_t u24, bool flag);
		void s_wb32(uint32_t u32, bool flag);
		void s_wb64(uint64_t u64, bool flag);
		void s_wbf(float f, bool flag);
		void s_wbd(double d, bool flag);
		//little endian
		void s_wl16(uint16_t u16, bool flag);
		void s_wl24(uint32_t u24, bool flag);
		void s_wl32(uint32_t u32, bool flag);
		void s_wl64(uint64_t u64, bool flag);
		void s_wlf(float f, bool flag);
		void s_wld(double d, bool flag);

	private:
		CAudioConfigInfo m_cAudioConfigInfo;
		CVideoConfigInfo m_cVideoConfigInfo;

	private:
		const char* m_pFileName;
		FILE*       m_pFile;
		uint32_t    m_uFilesize;
		bool        m_bFlag;
		uint32_t    m_uFirstTs;
		uint32_t    m_uLastTs;

		uint8_t*    m_pDataBuffer; //写入文件缓存
		uint32_t    m_uDataBufLen;
		uint8_t*    m_pVideoBuffer; //视频数据缓存
		uint32_t    m_uVideoBufLen;

		bool    m_bHeaderOK;
		bool    m_bMetaDataOK;
		bool    m_bAudioSequenceHeaderOK;
		bool    m_bVideoSequenceHeaderOK;
	};

} // end namespace

#endif // !_Bepartofyou_VIDEO_CONFIG_PARSE_H_
