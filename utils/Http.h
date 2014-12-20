#pragma once

#include <afxinet.h>
#include <atlimage.h>

///////////////////////////////////////////////////////////////////
/*
HTTP协议操作类
功能：实现HTTP的GET和POST操作
作者：西江月 QQ:2268190059 kjuu@yahoo.cn
时间：2011-10-20 到 2011-10-24
*/
////////////////////////////////////////////////////////////////////
class CHttp
{
public:
	//提交GET请求
	CString get(CString url);
	CString get(CString url, CString header);
	//提交POST请求
	CString post(CString url, CString PostData);
	CString post(CString url, CString header, CString PostData);
	//提交HTML请求
	CString GetHTML(CString url, CString header, CString PostData, bool isPost = false);
	//获取图片
	CImage *GetImg(CString url);

public:
	//保存网站的cookie
	CString m_cookies;
	//保存网站的编码方式
	int m_CodePage;
	//连接的会话对象
	CInternetSession m_session;

private:
	//提取URL中的域名和文件路径
	void DecomposeURL(CString &url, CString &domain, CString &path);
	//接收返回的HTML信息
	CString RevHTML(CHttpFile *pFile);
	//HTML内容编码转换
	CString EncodeHTML(CString &str);
	//POST数据编码转换
	CString EncodePOST(CString &str);
	//提取返回的cookies
	void SaveCookies(CHttpFile *pFile);

private:
	//保存图片
	CImage m_img;
	//图片内存和流
	HGLOBAL m_hGlobal;
	IStream *m_pStream;

public:
	CHttp(void);

public:
	~CHttp(void);
};
