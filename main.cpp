extern "C"
{
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/time.h"

}
#include<iostream>
using namespace std;
#pragma comment(lib,"avformat.lib")
#pragma comment(lib,"avutil.lib")
#pragma comment(lib,"avcodec.lib")
int ERROR(int re)
{
	char buf[1024] = { 0 };
	av_strerror(re, buf, sizeof(buf));
	cout << "error:" << buf << endl;
	getchar();
	return -1;
}
static double r2d(AVRational r)
{
	return r.num == 0 || r.den == 0 ? 0. : (double)r.num / (double)r.den;
}
int main()
{
	//1.��ʼ�����еķ�װ�ͽ��װ flv,mp4,mov,mp3
	av_register_all();
	//2.��ʼ�������
	avformat_network_init();
	//3.���ļ�,���װ
	//�����װ������
	AVFormatContext* ictx = NULL;
	const char* iurl = "video.flv";
	const char* ourl = "rtmp://192.168.150.128/live/123456";
	int re = avformat_open_input(&ictx, iurl, 0, 0);
	if (re != 0)//��ʧ��
	{
		return ERROR(re);
	}
	cout << "open file sucess" << endl;
	//4.��ȡ��Ƶ��Ƶ����Ϣ,h264,flv
	re = avformat_find_stream_info(ictx, 0);
	if (re != 0)
	{
		ERROR(re);
	}
	av_dump_format(ictx, 0, iurl, 0);
	///////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////
	//5.���������������
	AVFormatContext* octx;
	re = avformat_alloc_output_context2(&octx, 0, "flv", ourl);
	if (!octx)
	{
		return ERROR(re);
	}
	cout << "ocrx create sucess" << endl;
	//6.���������
	//��������AVStream
	for (int i = 0; i < ictx->nb_streams; i++)
	{
		AVStream* out = avformat_new_stream(octx,ictx->streams[i]->codec->codec);//
		if (!out)
		{
			return ERROR(0);
		}
		//����������Ϣ
		re = avcodec_copy_context(out->codec, ictx->streams[i]->codec);
		//re = avcodec_parameters_copy(out->codecpar, ictx->streams[i]->codecpar);
		out->codec->codec_tag = 0; //��֪����Ҫ����
		av_dump_format(octx, 0, ourl, 1);
	}
	//7.rtmp����
	//��IO����������ͨ��
	re = avio_open(&octx->pb,ourl,AVIO_FLAG_WRITE);
	if (!octx->pb)
	{
		ERROR(re);
	}
	//д��ͷ��Ϣ
	re = avformat_write_header(octx,0);
	if (re < 0)
	{
		ERROR(re);
	}
	cout << "avformat_write_header success,re = " << re << endl;
	//����ÿһ֡����
	AVPacket pkt;
	long long startTime = av_gettime();//��ȡ΢��ʱ���
	for (;;)
	{
		re = av_read_frame(ictx, &pkt);
		if (re != 0 )
		{
			break;
		}
		cout << pkt.pts << " " << flush;
		//����ת�� pts��dts����Ƶ�ļ���pts��dts�����뵽rtmp���ǲ�һ�µ�
		AVRational itime = ictx->streams[pkt.stream_index]->time_base;
		AVRational otime = octx->streams[pkt.stream_index]->time_base;
		pkt.pts = av_rescale_q_rnd(pkt.pts, itime, otime, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		pkt.dts = av_rescale_q_rnd(pkt.dts, itime, otime, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		pkt.duration = av_rescale_q_rnd(pkt.duration, itime, otime, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));//�Ѿ������˶��ʱ��
		pkt.pos = -1;
		//������һ֡����
		av_interleaved_write_frame(octx, &pkt);//�Զ��ͷţ�ʡȥav_packet_unref(&pkt)
		if (ictx->streams[pkt.stream_index]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			AVRational tb = ictx->streams[pkt.stream_index]->time_base;
			long long nowTime = av_gettime() - startTime;
			long long dts = 0;
			dts = pkt.dts * (1000 * 1000) * r2d(tb);
			if (dts > nowTime)
			{
				av_usleep(dts - nowTime);
			}
			
		}
		if (re != 0)
		{
			return ERROR(re);
		}
		//av_packet_unref(&pkt);

	}
	getchar();
	return 0;
}