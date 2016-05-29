#ifndef _UPLOAD_ERROR_H_
#define _UPLOAD_ERROR_H_
#include "slice_upload.h"
const string& error_info(int ret);
template<class Rsp>
void set_result(Rsp& rsp, int ret, SLICE_UPLOAD::Flag flag)
{
    rsp.result.ret = ret;
    rsp.result.flag = flag;
    char buf[32] = {0};
    if(ret<0)
    {
    	snprintf(buf, sizeof(buf), "(%d)", ret);
  	}
    rsp.result.msg = error_info(ret)+buf;
}

//proxy -1xx
const int ERR_ENCODE = -101;						//到process打包出错
const int ERR_DECODE = -102;						//到process解包出错
const int ERR_SEND_RECEIVE = -103;				//到process收发包失败
const int ERR_TIMEOUT = -104;					//到process超时
const int ERROR_WRONG_SESSION = -105;			//session没填或没按控制包返回的填
const int ERROR_PROXY_GET_PROCESS_L5 = -106;		//取process ip出错
const int ERROR_UNKNOWN_CMD = -107;				//错误的命令字，目前支持control和upload
const int ERROR_PARAM_EMPTY = -108;				//uin或appid或其他参数没填
//proxy -11x 登录态相关
const int ERROR_LOGIN = -110;                    //登录失败，请重新登录
const int ERROR_LOGIN_TOKEN = -111;              //登录已过期，请重新登录
const int ERROR_LOGIN_MOD_PWD = -112;            //密码已被修改，请重新登录
const int ERROR_LOGIN_OVERDATE = -113;           //密码已过期，请重新登录
const int ERROR_LOGIN_OTHER = -114;              //登录失败，请重新登录
const int ERROR_LOGIN_RETRY = -115;              //登录检查失败，请重试

//process -2xx
const int ERROR_PROCESS_WRONG_SLICE_SIZE = -201;	//分片大小错误，没按控制包返回的分片大小传
const int ERROR_PROCESS_QUERY_OFFSET = -202;		//查本地偏移出错
const int ERROR_PROCESS_DECODE_SESSION = -203;	//打包session出错
const int ERROR_PROCESS_ENCODE_SESSION = -204;	//解包session出错
const int ERROR_PROCESS_SLICE_TOO_SMALL = -205;	//分片过小
const int ERROR_PROCESS_WRONG_OFFSET = -206;		//offset异常
const int ERROR_PROCESS_OFFSET_FULL = -207;		//文件已收齐
const int ERROR_PROCESS_PARAM = -208;			//参数错误
const int ERROR_CREATE_STATE_FILE = -209;		//创建state文件失败
const int ERROR_READ_STATE_FILE = -210;			//读state文件失败
const int ERROR_WRITE_STATE_FILE = -211;			//写state文件失败
const int ERROR_CREATE_CONTROL_FILE = -212;		//创建control文件失败
const int ERROR_READ_CONTROL_FILE = -213;		//读control文件失败
const int ERROR_CREATE_DATA_FILE = -214;			//创建data缓存失败
const int ERROR_READ_DATE_FILE = -215;			//读data文件失败
const int ERROR_WRITE_DATA_FILE = -216;			//写data文件失败
const int ERROR_SLICE_CHECKSUM = -217;			//分片校验和失败
const int ERROR_FILE_CHECKSUM = -218;			//文件校验和失败
const int ERROR_FILE_NOT_COMPLETE = -219;		//commit时文件没收齐

//weiyun plugin -3xx
const int ERROR_CREATE_FTN_FILE = -301;			//创建ftn本地记录出错
const int ERROR_READ_FTN_FILE = -302;			//读ftn本地记录出错
const int ERROR_WRITE_FTN_FILE = -303;			//写ftn本地记录出错
const int ERROR_LOCAL_QUERY_OFFSET = -304;		//查本地ftn偏移出错
const int ERROR_GET_PART_FILE = -305;			//读取ftn分片数据出错
const int ERROR_WEIYUN_JSON_DECODE = -306;		//json解析失败
const int ERROR_WEIYUN_CHECK_TYPE = -307;		//check type必须是SHA
const int ERROR_FTN_SEND_RECEIVE = -308;			//发送给ftn失败
const int ERROR_FTN_SEQ = -309;					//串包
const int ERROR_MSG_TIMEOUT = -310;				//累积的缓存过多
const int ERROR_UKEY_TMEM_SET = -311;			//设置ukey失败
const int ERROR_UKEY_TMEM_GET = -312;			//读取ukey失败
const int ERROR_NO_SUCH_UKEY = -313;			//没有这个ukey

//photo plugin -4xx
const int ERROR_NO_CAPACITY = -401;   //用户容量不足
const int ERROR_FILE_TYPE = -402;     //"对不起，空间相册仅支持JPG、GIF、PNG、BMP等格式的图片"
const int ERROR_EXCEED_COMPRESS_SIZE = -403; //"对不起，后台压缩超过限制"
const int ERROR_SERVER_NOT_RETRY = -404; //发说说后台错误，不可重试
const int ERROR_FILE_MQZONE_UP = -405;  //上传存储错误
const int ERROR_FILE_UIN_ERROR = -406;  //"QQ号码不正确，请重新输入"
const int ERROR_CGI_ERROR_PARAM = -407;
const int ERROR_CANRETRY_TIME_OUT = -408; //后台可重试的超时
const int ERROR_FILE_NOEXIST = -409;
const int ERROR_DESC_MD5_EMPTY = -410;
const int ERROR_PHOTO_REACH_MAXSIZE = -411; //"对不起，上传图片超过最大限制"
const int ERR_FILE_PIC_DIRTY = -412;  //"内容含有敏感词"

const int ERR_FILE_PIC_FULL = -413; //"相册中相片不能超过10000张"
const int ERR_FILE_ALBUMID_NOT_EXSIT = -414; //"相册不存在，点击封面重选相册"
const int ERR_FILE_ALBUMID_EMPTY = -415; //"相册信息为空，请重新选择相册"
const int ERR_ALBUM_CHECK = -416; //相册有效性检验失败
const int ERR_FILE_UPLOAD_PREUPLOAD_FASTUPLOAD = -417; //能秒传秒传错误
const int ERROR_FILE_MD5_EMPTY = -418; //"上传的文件有误，请重新上传"
const int ERR_FILE_ALBUMID_FULL = -419; //"上传的相册超过数量限制"
const int ERR_UPLOAD_NO_RETRY = -420; //"架平入库失败，不可重试错误码"

//photo 架平入库错误码


//photo 发说说返回错误码
const int ERROR_SHUOSHUO_CONTENT_LENGTH_EXCEED = -491;
const int ERROR_SHUOSHUO_SYSTEM_UPDATING = -492;
const int ERROR_SHUOSHUO_THE_SAME_CONTENT = -493;
const int ERROR_PICSHUOSHUO_SVR_ERROR = -495;
const int ERROR_MULTI_PICSHUOSHUO_ERROR = -496;
const int ERROR_COMMIT_SHUOSHUO_ERROR = -497;
const int ERROR_UNKOWUN = -499;

//video 错误码
const int ERROR_DECODE  = -501;
const int ERROR_VIDEO_FASTUPLOAD = -502;
const int ERROR_APPLYVID = -503;
const int ERROR_APPLYUPLOAD = -504;
const int ERROR_QUERYOFFSET = -505;
const int ERROR_CREATEFTNFILE = -506;
const int ERROR_OPENPARAMFILE = -507;
const int ERROR_READPARAMFILE = -508;
const int ERROR_READFILE = -509;
const int ERROR_UPLOADERR = -510;
const int ERROR_CHECKBUFFER = -511;
const int ERROR_NOTIFYFEEDS = -512;
const int ERROR_VIDEO_NOTIFYSTART = -513;
const int ERROR_VIDEO_NOTIFYDONE= -514;

//FTN
const int ERR_FTN_TIMEOUT	= -7102;				//架平超时
const int ERR_FTN_STORE_PART = -7104;			//架平分片部分失败
const int ERR_FTN_OFFSET_BACK	= -7105;			//架平分片回退
const int ERR_FTN_SHA = -38020;					//SHA校验失败


#endif

