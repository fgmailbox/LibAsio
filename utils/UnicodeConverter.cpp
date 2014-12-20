#include "UnicodeConverter.h"

using namespace std;

/* -------------------------------------------------------------
					内码转换
   ------------------------------------------------------------- */

// 转换UCS4编码到UTF8编码
int UnicodeConverter::UCS4_To_UTF8( unsigned dwUCS4, unsigned char* pbUTF8 )
{
	const unsigned char	abPrefix[] = {0, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC};
	const unsigned adwCodeUp[] = {
		0x80,			// U+00000000 ～ U+0000007F
		0x800,			// U+00000080 ～ U+000007FF
		0x10000,		// U+00000800 ～ U+0000FFFF
		0x200000,		// U+00010000 ～ U+001FFFFF
		0x4000000,		// U+00200000 ～ U+03FFFFFF
		0x80000000		// U+04000000 ～ U+7FFFFFFF
	};

	int	i, iLen;

	// 根据UCS4编码范围确定对应的UTF-8编码字节数
	iLen = sizeof(adwCodeUp) / sizeof(unsigned);
	for( i = 0; i < iLen; i++ )
	{
		if( dwUCS4 < adwCodeUp[i] )
		{
			break;
		}
	}

	if( i == iLen )return 0;	// 无效的UCS4编码
		
	iLen = i + 1;	// UTF-8编码字节数
	if( pbUTF8 != NULL )
	{	// 转换为UTF-8编码
		for( ; i > 0; i-- )
		{
			pbUTF8[i] = static_cast<unsigned char>((dwUCS4 & 0x3F) | 0x80);
			dwUCS4 >>= 6;
		}

		pbUTF8[0] = static_cast<unsigned char>(dwUCS4 | abPrefix[iLen - 1]);
	}

	return iLen;
}

// 转换UTF8编码到UCS4编码
int UnicodeConverter::UTF8_To_UCS4( const unsigned char* pbUTF8, unsigned& dwUCS4 )
{
	int		i, iLen;
	unsigned char	b;

	if( pbUTF8 == NULL )
	{	// 参数错误
		return 0;
	}

	b = *pbUTF8++;
	if( b < 0x80 )
	{
		dwUCS4 = b;
		return 1;
	}

	if( b < 0xC0 || b > 0xFD )
	{	// 非法UTF8
		return 0; 
	}

	if( b < 0xE0 )
	{
		dwUCS4 = b & 0x1F;
		iLen = 2;
	}
	else if( b < 0xF0 )
	{
		dwUCS4 = b & 0x0F;
		iLen = 3;
	}
	else if( b < 0xF8 )
	{
		dwUCS4 = b & 7;
		iLen = 4;
	}
	else if( b < 0xFC )
	{
		dwUCS4 = b & 3;
		iLen = 5;
	}
	else
	{
		dwUCS4 = b & 1;
		iLen = 6;
	}

	for( i = 1; i < iLen; i++ )
	{
		b = *pbUTF8++;
		if( b < 0x80 || b > 0xBF )
		{	// 非法UTF8
			break;
		}

		dwUCS4 = (dwUCS4 << 6) + (b & 0x3F);
	}

	if( i < iLen )
	{	// 非法UTF8
		return 0;
	}
	else
	{
		return iLen;
	}
}

// 转换UCS4编码到UCS2编码
int UnicodeConverter::UCS4_To_UTF16( unsigned dwUCS4, unsigned short* pwUTF16 )
{
	if( dwUCS4 <= 0xFFFF )
	{
		if( pwUTF16 != NULL )
		{
			*pwUTF16 = static_cast<unsigned short>(dwUCS4);
		}

		return 1;
	}
	else if( dwUCS4 <= 0xEFFFF )
	{
		if( pwUTF16 != NULL )
		{
			pwUTF16[0] = static_cast<unsigned short>( 0xD800 + (dwUCS4 >> 10) - 0x40 );	// 高10位
			pwUTF16[1] = static_cast<unsigned short>( 0xDC00 + (dwUCS4 & 0x03FF) );		// 低10位
		}

		return 2;
	}
	else
	{
		return 0;
	}
}

// 转换UCS2编码到UCS4编码
int UnicodeConverter::UTF16_To_UCS4( const unsigned short* pwUTF16, unsigned& dwUCS4 )
{
	unsigned short	w1, w2;

	if( pwUTF16 == NULL )
	{	// 参数错误
		return 0;
	}

	w1 = pwUTF16[0];
	if( w1 >= 0xD800 && w1 <= 0xDFFF )
	{	// 编码在替代区域（Surrogate Area）
		if( w1 < 0xDC00 )
		{
			w2 = pwUTF16[1];
			if( w2 >= 0xDC00 && w2 <= 0xDFFF )
			{
				dwUCS4 = (w2 & 0x03FF) + (((w1 & 0x03FF) + 0x40) << 10);
				return 2;
			}
		}

		return 0;	// 非法UTF16编码	
	}
	else
	{
		dwUCS4 = w1;
		return 1;
	}
}

// 转换UTF8字符串到UTF16字符串
int UnicodeConverter::UTF8Str_To_UTF16Str( const unsigned char* pbszUTF8Str, unsigned short* pwszUTF16Str )
{
	int		iNum, iLen;
	unsigned	dwUCS4;

	if( pbszUTF8Str == NULL )
	{	// 参数错误
		return 0;
	}

	iNum = 0;	// 统计有效字符个数
	while( *pbszUTF8Str )
	{	// UTF8编码转换为UCS4编码
		iLen = UTF8_To_UCS4( pbszUTF8Str, dwUCS4 );
		if( iLen == 0 )
		{	// 非法的UTF8编码
			return 0;
		}

		pbszUTF8Str += iLen;

		// UCS4编码转换为UTF16编码
		iLen = UCS4_To_UTF16( dwUCS4, pwszUTF16Str );
		if( iLen == 0 )
		{
			return 0;
		}

		if( pwszUTF16Str != NULL )
		{
			pwszUTF16Str += iLen;
		}
		
		iNum += iLen;
	}

	if( pwszUTF16Str != NULL )
	{
		*pwszUTF16Str = 0;	// 写入字符串结束标记
	}

	return iNum;
}

// 转换UTF16字符串到UTF8字符串
int UnicodeConverter::UTF16Str_To_UTF8Str( const unsigned short* pwszUTF16Str, unsigned char* pbszUTF8Str )
{
	int		iNum, iLen;
	unsigned	dwUCS4;

	if( pwszUTF16Str == NULL )
	{	// 参数错误
		return 0;
	}

	iNum = 0;
	while( *pwszUTF16Str )
	{	// UTF16编码转换为UCS4编码
		iLen = UTF16_To_UCS4( pwszUTF16Str, dwUCS4 );
		if( iLen == 0 )
		{	// 非法的UTF16编码
			return 0;	
		}
		
		pwszUTF16Str += iLen;

		// UCS4编码转换为UTF8编码
		iLen = UCS4_To_UTF8( dwUCS4, pbszUTF8Str );
		if( iLen == 0 )
		{
			return 0;
		}

		if( pbszUTF8Str != NULL )
		{
			pbszUTF8Str += iLen;
		}
		
		iNum += iLen;
	}

	if( pbszUTF8Str != NULL )
	{
		*pbszUTF8Str = 0;	// 写入字符串结束标记
	}

	return iNum;
}

/* -------------------------------------------------------------
					C文件写入操作
   ------------------------------------------------------------- */

// 向文件中输出UTF8编码
unsigned int UnicodeConverter::Print_UTF8_By_UCS4( FILE* out, unsigned dwUCS4 )
{
	int		iLen;
	unsigned char	abUTF8[8];

	if( out == NULL )
	{
		return 0;
	}

	iLen = UCS4_To_UTF8( dwUCS4, abUTF8 );
	if( iLen == 0 )return 0;

	fwrite( abUTF8, 1, iLen, out );

	return iLen;
}

// 向文件中输出UTF16编码
unsigned int UnicodeConverter::Print_UTF16_By_UCS4( FILE* out, unsigned dwUCS4, bool isBigEndian )
{
	int		i, iLen;
	unsigned short	wCode, awUTF16[2];

	if( out == NULL )
	{
		return 0;
	}

	iLen = UCS4_To_UTF16( dwUCS4, awUTF16 );
	if( iLen == 0 )return 0;

	for( i = 0; i < iLen; i++ )
	{
		wCode = awUTF16[i];
		if( isBigEndian )
		{
			fputc( wCode >> 8, out );	// 输出高位
			fputc( wCode & 0xFF, out );	// 输出低位
		}
		else
		{
			fputc( wCode & 0xFF, out );	// 输出低位
			fputc( wCode >> 8, out );	// 输出高位
		}
	}

	return (iLen << 1);
}

// 将UTF16字符串以UTF8编码输出到文件中
unsigned int UnicodeConverter::Print_UTF8Str_By_UTF16Str( FILE* out, const unsigned short* pwszUTF16Str )
{
	int		iCount, iLen;
	unsigned	dwUCS4;

	if( (out == NULL) || (pwszUTF16Str == NULL) )
	{
		return 0;
	}

	iCount = 0;
	while( *pwszUTF16Str )
	{	// 将UTF16编码转换成UCS4编码
		iLen = UTF16_To_UCS4( pwszUTF16Str, dwUCS4 );
		if( iLen == 0 )
		{
			break;
		}

		pwszUTF16Str += iLen;

		// 向文件中输出UTF8编码
		iCount += Print_UTF8_By_UCS4( out, dwUCS4 );
	}

	return iCount;	// 输出的字节数
}

// 将UTF8字符串以UTF16编码输出到文件中
unsigned int UnicodeConverter::Print_UTF16Str_By_UTF8Str( FILE* out, const unsigned char* pbszUTF8Str, bool isBigEndian )
{
	int		iCount, iLen;
	unsigned	dwUCS4;

	if( (out == NULL) || (pbszUTF8Str == NULL) )
	{
		return 0;
	}

	iCount = 0;
	while( *pbszUTF8Str )
	{	// 将UTF16编码转换成UCS4编码
		iLen = UTF8_To_UCS4( pbszUTF8Str, dwUCS4 );
		if( iLen == 0 )
		{
			break;
		}

		pbszUTF8Str += iLen;

		// 向文件中输出UTF8编码
		iCount += Print_UTF16_By_UCS4( out, dwUCS4, isBigEndian );
	}

	return iCount;	// 输出的字节数
}

// 向文件中输出UTF8字节序标记
unsigned int UnicodeConverter::Print_UTF8_BOM( FILE* out )
{
	if( out == NULL )
	{
		return 0;
	}

	fputc( 0xEF, out );
	fputc( 0xBB, out );
	fputc( 0xBF, out );

	return 3;
}

// 向文件中输出UTF16字节序标记
unsigned int UnicodeConverter::Print_UTF16_BOM( FILE* out, bool isBigEndian )
{
	if( out == NULL )
	{
		return 0;
	}

	if( isBigEndian )
	{
		fputc( 0xFE, out );
		fputc( 0xFF, out );
	}
	else
	{
		fputc( 0xFF, out );
		fputc( 0xFE, out );
	}

	return 2;
}

/* -------------------------------------------------------------
					C++流输出操作
   ------------------------------------------------------------- */

// 向流中输出UTF8编码
unsigned int UnicodeConverter::Print_UTF8_By_UCS4( ostream& os, unsigned dwUCS4 )
{
	int		iLen;
	unsigned char	abUTF8[8];

	if( !os )return 0;
	
	iLen = UCS4_To_UTF8( dwUCS4, abUTF8 );
	if( iLen == 0 )return 0;

	os.write( reinterpret_cast<char*>(abUTF8), iLen );

	return iLen;	
}

// 向流中输出UTF16编码
unsigned int UnicodeConverter::Print_UTF16_By_UCS4( ostream& os, unsigned dwUCS4, bool isBigEndian )
{
	int		i, iLen;
	unsigned short	wCode, awUTF16[2];

	if( !os )return 0;
	
	iLen = UCS4_To_UTF16( dwUCS4, awUTF16 );
	if( iLen == 0 )return 0;

	for( i = 0; i < iLen; i++ )
	{
		wCode = awUTF16[i];
		if( isBigEndian )
		{
			os.put( wCode >> 8 );		// 输出高位
			os.put( wCode & 0xFF );		// 输出低位
		}
		else
		{
			os.put( wCode & 0xFF );		// 输出低位
			os.put( wCode >> 8 );		// 输出高位
		}
	}

	return (iLen << 1);
}

// 将UTF16字符串以UTF8编码输出到流中
unsigned int UnicodeConverter::Print_UTF8Str_By_UTF16Str( ostream& os, const unsigned short* pwszUTF16Str )
{
	int		iCount, iLen;
	unsigned	dwUCS4;

	if( !os || (pwszUTF16Str == NULL) )return 0;
	
	iCount = 0;
	while( *pwszUTF16Str )
	{	// 将UTF16编码转换成UCS4编码
		iLen = UTF16_To_UCS4( pwszUTF16Str, dwUCS4 );
		if( iLen == 0 )
		{
			break;
		}

		pwszUTF16Str += iLen;

		// 向流中输出UTF8编码
		iCount += Print_UTF8_By_UCS4( os, dwUCS4 );
	}

	return iCount;	// 输出的字节数
}

// 将UTF8字符串以UTF16编码输出到流中
unsigned int UnicodeConverter::Print_UTF16Str_By_UTF8Str( ostream& os, const unsigned char* pbszUTF8Str, bool isBigEndian )
{
	int		iCount, iLen;
	unsigned	dwUCS4;

	if( !os || (pbszUTF8Str == NULL) )return 0;

	iCount = 0;
	while( *pbszUTF8Str )
	{	// 将UTF16编码转换成UCS4编码
		iLen = UTF8_To_UCS4( pbszUTF8Str, dwUCS4 );
		if( iLen == 0 )
		{
			break;
		}

		pbszUTF8Str += iLen;

		// 向流中输出UTF8编码
		iCount += Print_UTF16_By_UCS4( os, dwUCS4, isBigEndian );
	}

	return iCount;	// 输出的字节数
}

// 向流中输出UTF8字节序标记
unsigned int UnicodeConverter::Print_UTF8_BOM( ostream& os )
{
	if( !os )return 0;
	
	os.put( 0xEF );
	os.put( 0xBB );
	os.put( 0xBF );

	return 3;	
}

// 向流中输出UTF16字节序标记
unsigned int UnicodeConverter::Print_UTF16_BOM( ostream& os, bool isBigEndian )
{
	if( !os )return 0;
	
	if( isBigEndian )
	{
		os.put( 0xFE );
		os.put( 0xFF );
	}
	else
	{
		os.put( 0xFF );
		os.put( 0xFE );
	}

	return 2;
}

/* ------------------------------
				END
   ------------------------------ */
