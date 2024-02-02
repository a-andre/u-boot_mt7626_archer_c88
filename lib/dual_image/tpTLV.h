/*!Copyright(c) 2014 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 *\file		tpTLV.h
 *\brief	This file defines TLV function.  
 *
 *\author	Jiang Lihui
 *\version	1.0.0
 *\date		21Feb14
 *
 *\history \arg 1.0.0, 21Feb14, Jiang Lihui, Create the file. 	
 */

#ifndef _TPTLV_H_
#define _TPTLV_H_

#ifdef __cplusplus
extern "C"
{
#endif

/***************************************************************************/
/*						CONFIGURATIONS					 */
/***************************************************************************/


/***************************************************************************/
/*						INCLUDE_FILES					 */
/***************************************************************************/


/***************************************************************************/
/*						DEFINES						 */
/***************************************************************************/
/*!
*\def   TLV_TYPE_OFFSET
*\brief The offset of tlv's type.
*/
#define TLV_TYPE_OFFSET						0

/*!
*\def   TLV_LENGTH_OFFSET
*\brief The offset of tlv's length.
*/
#define TLV_LENGTH_OFFSET					2

/*!
*\def   TLV_DATA_OFFSET
*\brief The offset of tlv's value.
*/
#define TLV_DATA_OFFSET						4

/*!
*\def   TLV_HDR_LEN
*\brief TLV header size.
*/
#define TLV_HDR_LEN 						4

/***************************************************************************/
/*						TYPES							 */
/***************************************************************************/



/***************************************************************************/
/*						VARIABLES						 */
/***************************************************************************/


/***************************************************************************/
/*						FUNCTIONS						 */
/***************************************************************************/
/*!
*\fn		int getTLVSpaceSizeAfterPadding(unsigned char* ptrTLV, unsigned int dataLen)
*\brief	    Pad the TLV and count the TLV length after padded. 
*\param[in]	ptrTLV		The TLV.
*\param[in]	dataLen		The TLV's real length.
*\return	The padded TLV's length.
*\retval	int
*\note		Example 1: The TLV's real length is 7 bytes, so we should padded with 0xff 0x00 0x00.
*			Example 1: The TLV's real length is 8 bytes, so we should padded with 0xff 0x00 0x00 0x00.
*/
int getTLVSpaceSizeAfterPadding(unsigned char* ptrTLV, unsigned int dataLen);

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
int setTLVData(unsigned char* ptrTLV, unsigned short type, unsigned short len, void *data);

/*!
*\fn		int setTLVUint8(unsigned char* ptrTLV, unsigned short type, unsigned char data)
*\brief	    Build a UINT8 TLV struct.
*\param[in]	ptrTLV		The TLV.
*\param[in]	type		The TLV's type.
*\param[in]	data		The TLV's value.
*\return	The padded TLV's length.
*\retval	int
*/
int setTLVUint8(unsigned char* ptrTLV, unsigned short type, unsigned char data);

/*!
*\fn		int setTLVUint16(unsigned char* ptrTLV, unsigned short type, unsigned short data)
*\brief	    Build a UINT16 TLV struct.
*\param[in]	ptrTLV		The TLV.
*\param[in]	type		The TLV's type.
*\param[in]	data		The TLV's value.
*\return	The padded TLV's length.
*\retval	int
*/
int setTLVUint16(unsigned char* ptrTLV, unsigned short type, unsigned short data);;

/*!
*\fn		int setTLVUint32(unsigned char* ptrTLV, unsigned short type, unsigned int data)
*\brief	    Build a UINT32 TLV struct.
*\param[in]	ptrTLV		The TLV.
*\param[in]	type		The TLV's type.
*\param[in]	data		The TLV's value.
*\return	The padded TLV's length.
*\retval	int
*/
int setTLVUint32(unsigned char* ptrTLV, unsigned short type, unsigned int data);;

/*!
*\fn		int setTLVUint64(unsigned char* ptrTLV, unsigned short type, unsigned int data)
*\brief	    Build a UINT64 TLV struct.
*\param[in]	ptrTLV		The TLV.
*\param[in]	type		The TLV's type.
*\param[in]	data		The TLV's value.
*\return	The padded TLV's length.
*\retval	int
*/
int setTLVUint64(unsigned char* ptrTLV, unsigned short type, unsigned long long data);


/*!
*\fn		int setTLVString(unsigned char* ptrTLV, unsigned short type, unsigned char* data )
*\brief	    Build a string TLV struct.
*\param[in]	ptrTLV		The TLV.
*\param[in]	type		The TLV's type.
*\param[in]	data		The TLV's value.
*\return	The padded TLV's length.
*\retval	int
*/
int setTLVString(unsigned char* ptrTLV, unsigned short type, unsigned char* data );;

/*!
*\fn		int setTLVStruct(unsigned char* ptrTLV, unsigned short type, unsigned short length)
*\brief	    Build a strcut TLV struct.
*\param[in]	ptrTLV		The TLV.
*\param[in]	type		The TLV's type.
*\param[in]	length		The TLV's real length.
*\return	The padded TLV's length.
*\retval	int
*/
int setTLVStruct(unsigned char* ptrTLV, unsigned short type, unsigned short length);

/*!
*\fn		int getTLVType(unsigned char* pTLVBuf)
*\brief	    Get the TLV's type value.
*\param[in]	pTLVBuf		The TLV start position.
*\return	The TLV's type value.
*\retval	int
*/
unsigned int getTLVType(unsigned char* pTLVBuf);

/*!
*\fn		int getTLVLength(unsigned char* pTLVBuf)
*\brief	    Get the TLV's length value.
*\param[in]	pTLVBuf		The TLV start position.
*\return	The TLV's length value.
*\retval	unsigned int
*/
unsigned int getTLVLength(unsigned char* pTLVBuf);

/*!
*\fn		    int getTLVData(unsigned char* pTLVBuf, void *data, unsigned int dataSize)
*\brief	    Get the TLV's data.
*\param[in]	pTLVBuf		The TLV start position.
*\param[in]	data		The getting data.
*\param[in]	dataSize		The getting data size.
*\return	Get data OK or Error.
*\retval	int
*/
int getTLVData(unsigned char* pTLVBuf, void *data, unsigned int dataSize);

/*!
*\fn		int getTLVString(unsigned char* pTLVBuf, unsigned char *data, unsigned int  maxSize)
*\brief	    Get the TLV's data.
*\param[in]	pTLVBuf      The TLV start position.
*\param[in]	data	         The getting data.
*\param[in]	maxSize	    The data buffer length.
*\return	Get data OK or Error.
*\retval	int
*/
int getTLVString(unsigned char* pTLVBuf, unsigned char *data, unsigned int  maxSize);

/*!
*\fn		int getTLVUint32(unsigned char* pTLVBuf, unsigned char *data)
*\brief	    Get the TLV's data that is a unsigned char.
*\param[in]	pTLVBuf		The TLV start position.
*\param[in]	data		The getting data.
*\return	Get data OK or Error.
*\retval	int
*/
int getTLVUint8(unsigned char* pTLVBuf, unsigned char *data);

/*!
*\fn		int getTLVUint32(unsigned char* pTLVBuf, unsigned short *data)
*\brief	    Get the TLV's data that is a unsigned short.
*\param[in]	pTLVBuf		The TLV start position.
*\param[in]	data		The getting data.
*\return	Get data OK or Error.
*\retval	int
*/
int getTLVUint16(unsigned char* pTLVBuf, unsigned short *data);

/*!
*\fn		int getTLVUint32(unsigned char* pTLVBuf, unsigned int *data)
*\brief	    Get the TLV's data that is a unsigned int.
*\param[in]	pTLVBuf		The TLV start position.
*\param[in]	data		The getting data.
*\return	Get data OK or Error.
*\retval	int
*/
int getTLVUint32(unsigned char* pTLVBuf, unsigned int *data);

/*!
*\fn		int getTLVUint64(unsigned char* pTLVBuf, unsigned long long *data)
*\brief		Get the TLV's data that is a unsigned long long.
*\param[in]	pTLVBuf		The TLV start position.
*\param[in]	data		The getting data.
*\return	Get data OK or Error.
*\retval	int
*/
int getTLVUint64(unsigned char* pTLVBuf, unsigned long long *data);

/*!
*\fn		unsigned int getTLVSpaceSize(unsigned char* pTLVBuf)
*\brief	    Get the TLV's space size.
*\param[in]	pTLVBuf		The TLV start position.
*\return	The TLV's space size.
*\retval	unsigned int
*/
unsigned int getTLVSpaceSize(unsigned char* pTLVBuf);

#ifdef __cplusplus
};
#endif
#endif
