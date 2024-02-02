#include <common.h>
#include "tpTLV.h"

#ifdef __cplusplus
extern "C"
{
#endif

/***************************************************************************/
/*						CONFIGURATIONS				 	 */
/***************************************************************************/


/***************************************************************************/
/*						INCLUDE_FILES					 */
/***************************************************************************/


/***************************************************************************/
/*						DEFINES						 */
/***************************************************************************/



/***************************************************************************/
/*						TYPES							 */
/***************************************************************************/


/***************************************************************************/
/*						EXTERN_PROTOTYPES					 */
/***************************************************************************/


/***************************************************************************/
/*						LOCAL_PROTOTYPES					 */
/***************************************************************************/


/***************************************************************************/
/*						VARIABLES						 */
/***************************************************************************/
/*!
 *\var		gTLVPaddings
 *\brief	TLV padding bytes.
*/	
static char gTLVPaddings[] = {0xff, 0x00, 0x00, 0x00};

/***************************************************************************/
/*						LOCAL_FUNCTIONS					 */
/***************************************************************************/


/***************************************************************************/
/*						PUBLIC_FUNCTIONS					 */
/***************************************************************************/
/*!
*\fn		int getTLVSpaceSizeAfterPadding(unsigned char* ptrTLV, unsigned int dataLen)
*\brief	    Pad the TLV and count the TLV length after padded. 
*\param[in]	ptrTLV		The TLV.
*\param[in]	dataLen		The TLV's real length.
*\return	The padded TLV's length.
*\retval	int
*\note		Example 1: The TLV's real length is 7 bytes, so we should padded with 0xff.
*			Example 1: The TLV's real length is 8 bytes, so we should padded with 0xff 0x00 0x00 0x00.
*/
int getTLVSpaceSizeAfterPadding(unsigned char* ptrTLV, unsigned int dataLen)
{
	unsigned int totalLen = dataLen + TLV_HDR_LEN;
	unsigned int paddingLen = 0;
	
	paddingLen = 4 - (totalLen&0x3);
	memcpy(ptrTLV + totalLen, gTLVPaddings, paddingLen);
	totalLen += paddingLen;	
	
	return totalLen;
}

/*!
*\fn		int setTLVData(unsigned char* ptrTLV, 
*							unsigned short type, unsigned short len, void *data)
*\brief	    Build a TLV struct. This is a basic function.
*\param[in]	ptrTLV		The TLV.
*\param[in]	type		The TLV's type.
*\param[in]	len			The TLV's real length.
*\param[in]	data		The TLV's value.
*\return	The padded TLV's length.
*\retval	int
*/
int setTLVData(unsigned char* ptrTLV, 
	unsigned short type, unsigned short len, void *data)
{
	if( NULL == ptrTLV || NULL == data)
	{
		return -1;
	}

	type = htons( type );	
	memcpy(ptrTLV + TLV_TYPE_OFFSET, &type, sizeof(type));

	memcpy(ptrTLV + TLV_DATA_OFFSET, data, len);
	
	len = htons( len );	
	memcpy(ptrTLV + TLV_LENGTH_OFFSET, &len, sizeof(len));

	return getTLVSpaceSizeAfterPadding(ptrTLV, ntohs(len));
}

/*!
*\fn		int setTLVUint8(unsigned char* ptrTLV, unsigned short type, unsigned char data)
*\brief	    Build a UINT8 TLV struct.
*\param[in]	ptrTLV		The TLV.
*\param[in]	type		The TLV's type.
*\param[in]	data		The TLV's value.
*\return	The padded TLV's length.
*\retval	int
*/
int setTLVUint8(unsigned char* ptrTLV, unsigned short type, unsigned char data)
{	
	if( NULL == ptrTLV )
	{
		return -1;
	}

	return setTLVData( ptrTLV, type, sizeof(data), &data);
}

/*!
*\fn		int setTLVUint16(unsigned char* ptrTLV, unsigned short type, unsigned short data)
*\brief	    Build a UINT16 TLV struct.
*\param[in]	ptrTLV		The TLV.
*\param[in]	type		The TLV's type.
*\param[in]	data		The TLV's value.
*\return	The padded TLV's length.
*\retval	int
*/
int setTLVUint16(unsigned char* ptrTLV, unsigned short type, unsigned short data)
{
	if( NULL == ptrTLV )
	{
		return -1;
	}
	data = htons(data);	
	return setTLVData( ptrTLV, type, sizeof(data), &data);
}

/*!
*\fn		int setTLVUint32(unsigned char* ptrTLV, unsigned short type, unsigned int data)
*\brief	    Build a UINT32 TLV struct.
*\param[in]	ptrTLV		The TLV.
*\param[in]	type		The TLV's type.
*\param[in]	data		The TLV's value.
*\return	The padded TLV's length.
*\retval	int
*/
int setTLVUint32(unsigned char* ptrTLV, unsigned short type, unsigned int data)
{
	if( NULL == ptrTLV )
	{
		return -1;
	}
	data = htonl(data);	
	return setTLVData( ptrTLV, type, sizeof(data), &data);
}

/*!
*\fn		int setTLVUint64(unsigned char* ptrTLV, unsigned short type, unsigned int data)
*\brief	    Build a UINT64 TLV struct.
*\param[in]	ptrTLV		The TLV.
*\param[in]	type		The TLV's type.
*\param[in]	data		The TLV's value.
*\return	The padded TLV's length.
*\retval	int
*/
int setTLVUint64(unsigned char* ptrTLV, unsigned short type, unsigned long long data)
{
	if( NULL == ptrTLV )
	{
		return -1;
	}

#if	__BYTE_ORDER==__BIG_ENDIAN
    /* 大端：不做字节序改变 */
#else
    /* 小端：需转换为大端存储
    例如：主机字节序data=0x1122334455667788, data本身保存为(地址由低到高)88 77 66 55 44 33 22 11
    存储在TLV中需转变为11 22 33 44 55 66 77 88，对应主机字节序data=0x8877665544332211 */
	unsigned long  high, low;

	low  = data & 0xFFFFFFFF;
	high = (data >> 32) & 0xFFFFFFFF;

	low  = htonl(low);
	high = htonl(high);

	data = low;
	data <<= 32;
	data |= high;
#endif

	return setTLVData( ptrTLV, type, sizeof(data), &data);
}

/*!
*\fn		int setTLVString(unsigned char* ptrTLV, unsigned short type, unsigned char* data )
*\brief	    Build a string TLV struct.
*\param[in]	ptrTLV		The TLV.
*\param[in]	type		The TLV's type.
*\param[in]	data		The TLV's value.
*\return	The padded TLV's length.
*\retval	int
*/
int setTLVString(unsigned char* ptrTLV, unsigned short type, unsigned char* data )
{
	if( NULL == ptrTLV )
	{
		return -1;
	}
	return setTLVData( ptrTLV, type, (unsigned short)strlen( (char *)data ), data);
}

/*!
*\fn		int setTLVStruct(unsigned char* ptrTLV, unsigned short type, unsigned short length)
*\brief	    Build a strcut TLV struct.
*\param[in]	ptrTLV		The TLV.
*\param[in]	type		The TLV's type.
*\param[in]	length		The TLV's real length.
*\return	The padded TLV's length.
*\retval	int
*/
int setTLVStruct(unsigned char* ptrTLV, unsigned short type, unsigned short length)
{	
	if( NULL == ptrTLV )
	{
		return -1;
	}

	type = htons( type );	
	memcpy(ptrTLV + TLV_TYPE_OFFSET, &type, sizeof(type));

	
	length = htons( length );	
	memcpy(ptrTLV + TLV_LENGTH_OFFSET, &length, sizeof(length));

	return getTLVSpaceSizeAfterPadding(ptrTLV, ntohs(length));
}

/*!
*\fn		int getTLVType(unsigned char* pTLVBuf)
*\brief	    Get the TLV's type value.
*\param[in]	pTLVBuf		The TLV start position.
*\return	The TLV's type value.
*\retval	int
*/
unsigned int getTLVType(unsigned char* pTLVBuf)
{	
	unsigned short tlvType = 0;
	if( NULL == pTLVBuf )
	{
		return -1;
	}

	memcpy( &tlvType, pTLVBuf + TLV_TYPE_OFFSET, sizeof( unsigned short));
	tlvType = ntohs( tlvType);
	return tlvType; 
}

/*!
*\fn		int getTLVLength(unsigned char* pTLVBuf)
*\brief	    Get the TLV's length value.
*\param[in]	pTLVBuf		The TLV start position.
*\return	The TLV's length value.
*\retval	unsigned int
*/
unsigned int getTLVLength(unsigned char* pTLVBuf)
{	
	unsigned short tlvLength = 0;
	if( NULL == pTLVBuf )
	{
		return -1;
	}

	memcpy( &tlvLength, pTLVBuf + TLV_LENGTH_OFFSET, sizeof( unsigned short));
	tlvLength = ntohs( tlvLength);

	return tlvLength; 
}

/*!
*\fn		    int getTLVData(unsigned char* pTLVBuf, void *data, unsigned int dataSize)
*\brief	    Get the TLV's data.
*\param[in]	pTLVBuf		The TLV start position.
*\param[in]	data		The getting data.
*\param[in]	dataSize		The getting data size.
*\return	Get data OK or Error.
*\retval	int
*/
int getTLVData(unsigned char* pTLVBuf, void *data, unsigned int dataSize)
{	
	if( NULL == pTLVBuf )
	{
		return -1;
	}

    if( getTLVLength(pTLVBuf) > dataSize)
    {
        return -1;
    }

    memset( data, 0, dataSize);
	memcpy(data, pTLVBuf + TLV_DATA_OFFSET, getTLVLength(pTLVBuf));
	return 0;
}

/*!
*\fn		int getTLVString(unsigned char* pTLVBuf, unsigned char *data, unsigned int  maxSize)
*\brief	    Get the TLV's data.
*\param[in]	pTLVBuf      The TLV start position.
*\param[in]	data	         The getting data.
*\param[in]	maxSize	    The data buffer length.
*\return	Get data OK or Error.
*\retval	int
*/
int getTLVString(unsigned char* pTLVBuf, unsigned char *data, unsigned int  maxSize)
{	
    unsigned int length = 0;
	if( NULL == pTLVBuf )
	{
		return -1;
	}

    memset( data, 0, maxSize);
    if( getTLVData(pTLVBuf, data, maxSize - 1) < 0)
    {
        return -1;
    }

    length = getTLVLength(pTLVBuf);
    data[length] = '\0';        
	return 0;
}



/*!
*\fn		int getTLVUint32(unsigned char* pTLVBuf, unsigned char *data)
*\brief	    Get the TLV's data that is a unsigned char.
*\param[in]	pTLVBuf		The TLV start position.
*\param[in]	data		The getting data.
*\return	Get data OK or Error.
*\retval	int
*/
int getTLVUint8(unsigned char* pTLVBuf, unsigned char *data)
{	
	if( NULL == pTLVBuf )
	{
		return -1;
	}

	if( getTLVData(pTLVBuf, data, sizeof( unsigned char)) < 0)
	{
	    return -1;
	}
	return 0; 
}

/*!
*\fn		int getTLVUint32(unsigned char* pTLVBuf, unsigned short *data)
*\brief	    Get the TLV's data that is a unsigned short.
*\param[in]	pTLVBuf		The TLV start position.
*\param[in]	data		The getting data.
*\return	Get data OK or Error.
*\retval	int
*/
int getTLVUint16(unsigned char* pTLVBuf, unsigned short *data)
{	
	if( NULL == pTLVBuf )
	{
		return -1;
	}

	if( getTLVData(pTLVBuf, data, sizeof( unsigned short)) < 0)
	{
	    return -1;
	}

	*data = ntohs(*data);
	return 0; 
}

/*!
*\fn		int getTLVUint32(unsigned char* pTLVBuf, unsigned int *data)
*\brief	    Get the TLV's data that is a unsigned int.
*\param[in]	pTLVBuf		The TLV start position.
*\param[in]	data		The getting data.
*\return	Get data OK or Error.
*\retval	int
*/
int getTLVUint32(unsigned char* pTLVBuf, unsigned int *data)
{	
	if( NULL == pTLVBuf )
	{
		return -1;
	}

	if( getTLVData(pTLVBuf, data, sizeof( unsigned int)) < 0)
	{
	    return -1;
	}

	*data = ntohl(*data);
	return 0; 
}

/*!
*\fn		int getTLVUint64(unsigned char* pTLVBuf, unsigned long long *data)
*\brief		Get the TLV's data that is a unsigned long long.
*\param[in]	pTLVBuf		The TLV start position.
*\param[in]	data		The getting data.
*\return	Get data OK or Error.
*\retval	int
*/
int getTLVUint64(unsigned char* pTLVBuf, unsigned long long *data)
{
	if( NULL == pTLVBuf)
	{
		return -1;
	}

	if( getTLVData(pTLVBuf, data, sizeof(unsigned long long)) < 0)
	{
	    return -1;
	}

#if	__BYTE_ORDER==__BIG_ENDIAN
    /* 大端：不做字节序改变 */
#else
    /* 小端：大端格式应转换为主机字节序
    例如：存储在TLV中(地址由低到高)的数据为11 22 33 44 55 66 77 88，对应的UINT64数据应为0x1122334455667788
    但小端系统读取到的data值为0x8877665544332211 */
    unsigned long  high, low;

    low  = *data & 0xFFFFFFFF;
    high = (*data >> 32) & 0xFFFFFFFF;

    low  = ntohl(low);
    high = ntohl(high);

    *data = low;
    *data <<= 32;
    *data |= high;
#endif

	return 0; 
}

/*!
*\fn		unsigned int getTLVSpaceSize(unsigned char* pTLVBuf)
*\brief	    Get the TLV's space size.
*\param[in]	pTLVBuf		The TLV start position.
*\return	The TLV's space size.
*\retval	unsigned int
*/
unsigned int getTLVSpaceSize(unsigned char* pTLVBuf)
{	
	unsigned int totalLength = 0;
	if( NULL == pTLVBuf )
	{
		return -1;
	}

	totalLength = getTLVLength(pTLVBuf) + TLV_HDR_LEN;
	return (totalLength + 4 - (totalLength&0x3)); // return the next start pos
}


/***************************************************************************/
/*						GLOBAL_FUNCTIONS					 */
/***************************************************************************/


#ifdef __cplusplus
};
#endif
