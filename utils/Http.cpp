#include "StdAfx.h"
#include "Http.h"

CHttp::CHttp(void)
{
	m_cookies = "";
	m_CodePage = -1;
	//图片和内存流初值
	m_hGlobal = NULL;
	m_pStream = NULL;
}

CHttp::~CHttp(void)
{
	//释放图片内存和流
	if (m_hGlobal != NULL)
		GlobalFree(m_hGlobal);
	if (m_pStream != NULL)
		m_pStream->Release();
}

///////////////////////////////////////////////////////////////////
/*
功能：提交GET请求
参数：URL地址
返回：获得的HTML页面内容
*/
CString CHttp::get(CString url)
{
	return GetHTML(url, "", "");
}

///////////////////////////////////////////////////////////////////
/*
功能：提交GET请求
参数：URL地址，请求头的字符串
返回：获得的HTML页面内容
*/
CString CHttp::get(CString url, CString header)
{
	return GetHTML(url, header, "");
}

///////////////////////////////////////////////////////////////////
/*
功能：提交POST请求
参数：URL地址，POST内容的字符串
返回：获得的HTML页面内容
*/
CString CHttp::post(CString url, CString PostData)
{
	return GetHTML(url, "", PostData, true);
}

///////////////////////////////////////////////////////////////////
/*
功能：提交POST请求
参数：URL地址，请求头字符串，POST内容的字符串
返回：获得的HTML页面内容
*/
CString CHttp::post(CString url, CString header, CString PostData)
{
	return GetHTML(url, header, PostData, true);
}

////////////////////////////////////////////////////////////////////////
/*
功能：提交HTML请求
参数：
	URL地址，
	请求头，
	POST数据，
	是否使用POST方式提交，默认为：是
返回：获得的HTML页面内容
*/
CString CHttp::GetHTML(CString url, CString header, CString PostData, bool isPost)
{
	//提取URL的域名和文件路径
	CString domain;
	CString path;
	DecomposeURL(url, domain, path);

	//建立连接
	CHttpConnection *pConnection = m_session.GetHttpConnection(domain);
	CHttpFile *pFile = pConnection->OpenRequest
	(
		(isPost) ? CHttpConnection::HTTP_VERB_POST : CHttpConnection::HTTP_VERB_GET,
		path, 0, 1, 0, 0, INTERNET_FLAG_NO_COOKIES
	);

	//添加请求头
	if (header != "")
		pFile->AddRequestHeaders(CString(header + "\n"));

	//添加cookies
	if (m_cookies != "")
		pFile->AddRequestHeaders(CString("Cookie: " + m_cookies));

	//把POST数据编码
	if (isPost)
		PostData = EncodePOST(PostData);

	try
	{
		//提交请求
		if (isPost)
			pFile->SendRequest(CString("Content-Type:application/x-www-form-urlencoded"), PostData.GetBuffer(), PostData.GetLength());
		else
			pFile->SendRequest();

		//检测返回码
		DWORD StatusCode;
		pFile->QueryInfoStatusCode(StatusCode);
		//如果返回码不是200，则当做异常返回
		if (StatusCode != HTTP_STATUS_OK)
			throw false;
	}
	catch (...)
	{
		//释放连接

		//读取返回HTML的内容
	    CString RevBuferror;
	    RevBuferror = RevHTML(pFile);


		pFile->Close();
		pConnection->Close();

		//异常返回空串
			return RevBuferror;
	}

	//读取返回HTML的内容
	CString RevBuf;
	RevBuf = RevHTML(pFile);

	//提取并保存返回的cookies
	SaveCookies(pFile);

	//释放连接
	pFile->Close();
	pConnection->Close();

	return RevBuf;
}

////////////////////////////////////////////////////////////////////
/*
功能：提取URL中的域名和文件路径
参数：URL地址，接收域名的字符串，接收文件路径的字符串
返回：无
*/
void CHttp::DecomposeURL(CString &url, CString &domain, CString &path)
{
	int pos1, pos2;
	pos1 = url.Find('/');
	pos1 += 2;
	pos2 = url.Find('/', pos1);
	pos2--;
	domain = url.Mid(pos1, pos2 - pos1 + 1);

	pos2 += 2;
	path = url.Right(url.GetLength() - pos2);
}

/////////////////////////////////////////////////////////////////////
/*
功能：接收返回的HTML信息
参数：CHttpFile文件指针
返回：HTML内容的字符串
*/
CString CHttp::RevHTML(CHttpFile *pFile)
{
	CString RevBuf;
	
	//读取返回的文件内容
	while (true)
	{
		char buf[1024];
		int len;
		
		try
		{
			len= pFile->Read(buf, 1023);
		}
		catch (...)
		{
			//异常返回空串
			return CString("");
		}

		if (len <= 0)
			break;
		buf[len] = '\0';
		RevBuf += buf;
	}

	//对返回的内容正确编码
	RevBuf = EncodeHTML(RevBuf);

	return RevBuf;
}

////////////////////////////////////////////////////////////////////
/*
功能：编码转换
参数：源字符串
返回：转换后的字符串
*/
CString CHttp::EncodeHTML(CString &str)
{
	//如果当前未设置编码
	if (m_CodePage == -1)
	{
		//根据标签，设置编码
		int pos1 = str.Find("charset");
		if (pos1 == -1)
			pos1 = str.Find("encoding");
		if (pos1 > -1)
		{
			int pos2 = str.Find('>', pos1);
			CString CodePageStr;
			CodePageStr = str.Mid(pos1, pos2 - pos1);
			CodePageStr.MakeUpper();
			if (CodePageStr.Find("UTF-8") > -1)
				m_CodePage = CP_UTF8;
			else
				if (CodePageStr.Find("GB2312") > -1)
					m_CodePage = CP_ACP;
				else
					if (CodePageStr.Find("BGK") > -1)
						m_CodePage = CP_ACP;
		}
	}

	//如果设置了其他编码，则转换编码
	if (m_CodePage == CP_UTF8)
	{
		//把字符串转换为UNICODE编码
		CStringW strW;
		//获取UNICODE编码所需的长度
		int len = ::MultiByteToWideChar(m_CodePage, NULL, str.GetBuffer(), str.GetLength(), NULL, NULL);
		//注意别忘了，拷贝字符串结束符
		strW.GetBuffer(len + 1);
		::MultiByteToWideChar(m_CodePage, NULL, str.GetBuffer(), str.GetLength() + 1, strW.GetBuffer(), len + 1);
		strW.ReleaseBuffer();

		//把UNICODE编码转化为GB2312编码
		CString strA;
		BOOL succ;
		//获取GB2312编码所需的长度
		len = ::WideCharToMultiByte(CP_ACP, NULL, strW.GetBuffer(), strW.GetLength(), NULL, NULL, NULL, &succ);
		//注意别忘了，拷贝字符串结束符
		strA.GetBuffer(len + 1);
		::WideCharToMultiByte(CP_ACP, NULL, strW.GetBuffer(), strW.GetLength() + 1, strA.GetBuffer(), len + 1, NULL, &succ);
		strA.ReleaseBuffer();
	
		return strA;
	}

	//不需转换，则返回原串
	return str;
}

////////////////////////////////////////////////////////////////////
/*
功能：POST数据编码转换
参数：源字符串
返回：转换后的字符串
*/
CString CHttp::EncodePOST(CString &str)
{
	//如果设置了其他编码，则转换编码
	if (m_CodePage == CP_UTF8)
	{
		//把GB2312编码转换为UNICODE编码
		CStringW strW;
		//获取UNICODE编码所需的长度
		int len = ::MultiByteToWideChar(CP_ACP, NULL, str.GetBuffer(), str.GetLength(), NULL, NULL);
		//注意别忘了，拷贝字符串结束符
		strW.GetBuffer(len + 1);
		::MultiByteToWideChar(CP_ACP, NULL, str.GetBuffer(), str.GetLength() + 1, strW.GetBuffer(), len + 1);
		strW.ReleaseBuffer();

		//把UNICODE编码转化为目标编码
		CString strA;
		//获取GB2312编码所需的长度
		len = ::WideCharToMultiByte(m_CodePage, NULL, strW.GetBuffer(), strW.GetLength(), NULL, NULL, NULL, NULL);
		//注意别忘了，拷贝字符串结束符
		strA.GetBuffer(len + 1);
		::WideCharToMultiByte(m_CodePage, NULL, strW.GetBuffer(), strW.GetLength() + 1, strA.GetBuffer(), len + 1, NULL, NULL);
		strA.ReleaseBuffer();
	
		return strA;
	}

	//不需转换，则返回原串
	return str;
}

//////////////////////////////////////////////////////////////////////
/*
功能：提取并保存返回的cookies
参数：CHttpFile文件指针
返回：无
*/
void CHttp::SaveCookies(CHttpFile *pFile)
{
	//有多个cookies的时候，需要index参数
	DWORD index = 0;

	while (true)  
	{
		//获取一个cookie
		CString cookie;
		char CookieBuf[1024];
		DWORD BufLen = 1023;
		//QueryInfo方法的CString参数重载函数有BUG，所以使用这个重载函数
		if (pFile->QueryInfo(HTTP_QUERY_SET_COOKIE, CookieBuf, &BufLen, &index) == false) 
			break;
		cookie = CookieBuf;
		//加个分号方便查找和提取
		cookie += ';';

		//提取cookie的项并保存
		int ItemCount = 0;
		//计算项个数
		for (int i = 0; i < cookie.GetLength(); i++)
			if (cookie[i] == ';')
				ItemCount++;
		int pos1 = 0;
		int pos2 = 0;
		//添加每一项
		for (int i = 0; i < ItemCount; i++)
		{
			//提取一项
			pos1 = pos2;
			if (cookie[pos1] == ';')
				pos1++;
			pos2 = cookie.Find(';', pos1);
			CString CookieItem = cookie.Mid(pos1, pos2 - pos1);

			int pos = CookieItem.Find('=');
			//无需处理的cookie项目
			if (pos == -1)
				continue;
			//获取项名
			CString ItemName = CookieItem.Left(pos);
			ItemName.Trim();

			//检测此cookie项是否需要舍弃
			CString ItemNameCheck = ItemName;
			ItemNameCheck.MakeLower();
			if (ItemNameCheck == "expires" || ItemNameCheck == "domain" || ItemNameCheck == "path")
				continue;

			//检测是否已经存在此cookie项
			int ItemPos = m_cookies.Find(ItemName);
			if (ItemPos > -1 && 
				(ItemPos == 0 || m_cookies[ItemPos - 1] == ' ' || m_cookies[ItemPos - 1] == ';') &&
				(m_cookies[ItemPos + ItemName.GetLength()] == ' ' || m_cookies[ItemPos + ItemName.GetLength()] == '='))
			{
				//存在此cookie项，需要替换
				int ItemPosEnd = m_cookies.Find(';', ItemPos);
				CString CutLeft = m_cookies.Left(ItemPos);
				CutLeft.Trim();
				CString CutRight = m_cookies.Right(m_cookies.GetLength() - ItemPosEnd);
				//重新连接cookies串
				m_cookies = CutLeft + CookieItem + CutRight;
			}
			else
				//不存在则直接添加
				m_cookies += CookieItem + ';';
		}
	}
}

/////////////////////////////////////////////////////////////////////
/*
功能：获取图片
参数：图片的URL地址
返回：
	CImage对象指针，在OnDraw或者OnPaint中调用Draw方法即可显示图片。
	失败返回空指针
*/
CImage *CHttp::GetImg(CString url)
{
	//提取URL的域名和文件路径
	CString domain;
	CString path;
	DecomposeURL(url, domain, path);

	//建立连接
	CHttpConnection *pConnection = m_session.GetHttpConnection(domain);
	CHttpFile *pFile = pConnection->OpenRequest(CHttpConnection::HTTP_VERB_GET, path,
		0, 1, 0, 0, INTERNET_FLAG_NO_COOKIES);

	try
	{
		//提交请求
		pFile->SendRequest();

		//检测返回码
		DWORD StatusCode;
		pFile->QueryInfoStatusCode(StatusCode);
		//如果返回码不是200，则当做异常返回
		if (StatusCode != HTTP_STATUS_OK)
			throw false;
	}
	catch (...)
	{
		//释放连接
		pFile->Close();
		pConnection->Close();

		//异常返回空指针
		return NULL;
	}

	//读取图片数据
	char *ImgData = NULL;
	int ImgSize = 0;
	while (true)
	{
		char Data[1024];
		int len;

		try
		{
			len = pFile->Read(Data, 1024);
		}
		catch (...)
		{
			//释放连接
			pFile->Close();
			pConnection->Close();

			//异常返回空指针
			return NULL;
		}		
		
		if (len <= 0)
			break;
		//拷贝图片数据
		ImgSize += len;
		ImgData = (char *)realloc(ImgData, ImgSize);
		memcpy(ImgData + ImgSize - len, Data, len);
	}

	//申请流内存
	m_hGlobal = GlobalAlloc(GMEM_MOVEABLE, ImgSize);
	void *pData = GlobalLock(m_hGlobal);
	//拷贝数据
	memcpy(pData, ImgData, ImgSize);
	free(ImgData);
	GlobalUnlock(m_hGlobal);

	//创建流
	m_pStream = NULL;
	if (CreateStreamOnHGlobal(m_hGlobal, TRUE, &m_pStream) == S_OK)
	{
		//释放已经加载的图片，防止第二次加载错误
		m_img.Destroy();
		//加载图片
		if (!SUCCEEDED(m_img.Load(m_pStream)))
		{
			//释放连接
			pFile->Close();
			pConnection->Close();

			//失败返回空指针
			return NULL;
		}
	}

	//提取并保存返回的cookies
	SaveCookies(pFile);

	//释放连接
	pFile->Close();
	pConnection->Close();

	return &m_img;
}