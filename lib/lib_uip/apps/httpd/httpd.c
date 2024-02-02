#include "uip.h"
#include "httpd.h"
#include "fs.h"
#include "fsdata.h"
#include "common.h"

DECLARE_GLOBAL_DATA_PTR;

#define STATE_NONE				0		// empty state (waiting for request...)
#define STATE_FILE_REQUEST		1		// remote host sent GET request
#define STATE_UPLOAD_REQUEST	2		// remote host sent POST request

// ASCII characters
#define ISO_G					0x47	// GET
#define ISO_E					0x45
#define ISO_T					0x54
#define ISO_P					0x50	// POST
#define ISO_O					0x4f
#define ISO_S					0x53
#define ISO_T					0x54
#define ISO_slash				0x2f	// control and other characters
#define ISO_space				0x20
#define ISO_nl					0x0a
#define ISO_cr					0x0d
#define ISO_tab					0x09

#define HWID_DES_DEVMODEL		"devModel:"
#define HWID_DES_HWVER			"hwVer:"
#define DEVMODEL_STR_MAX_LEN		64
#define HWVER_STR_MAX_LEN		8
#define DEVNAME_STR_MAX_LEN		32

#define HWIDDES_OFFSET			0x44
#define HWIDDES_LEN			128
#define INDEX_SCRIPT			"<script>Init('%s');</script>"

// we use this so that we can do without the ctype library
#define is_digit(c)				((c) >= '0' && (c) <= '9')

// debug
//#define DEBUG_UIP

// html files
extern const struct fsdata_file file_index_html;
extern const struct fsdata_file file_404_html;
extern const struct fsdata_file file_flashing_html;
extern const struct fsdata_file file_fail_html;

extern int webfailsafe_ready_for_upgrade;
extern int webfailsafe_firmware_chk_success;
extern ulong NetBootFileXferSize;
extern unsigned char *webfailsafe_data_pointer;

// http app state
struct httpd_state *hs;

static int webfailsafe_post_done = 0;
static int webfailsafe_upload_failed = 0;
static int data_start_found = 0;

static unsigned char post_packet_counter = 0;

// 0x0D -> CR 0x0A -> LF
static char eol[3] = { 0x0d, 0x0a, 0x00 };
static char eol2[5] = { 0x0d, 0x0a, 0x0d, 0x0a, 0x00 };
char dev_name[DEVNAME_STR_MAX_LEN] = {0};
static char *boundary_value;
char *index_file = NULL;

// str to int
static int atoi(const char *s){
	int i = 0;

	while(is_digit(*s)){
		i = i * 10 + *(s++) - '0';
	}

	return(i);
}

// print downloading progress
static void httpd_download_progress(void){
	if(post_packet_counter == 194){
		puts("\n         ");
		post_packet_counter = 0;
	}

	if (post_packet_counter%3 == 0)
		puts("#");
	
	post_packet_counter++;
}

/* ¸Ãº¯Êý´ÓSDMPÔÆÆ½Ì¨deviceInfo.cÖÐµÄgetInfoFromProfileº¯ÊýÑÝ±ä¶øÀ´ */
static int getInfoFromHwIdDesStr(char *pHrdDesStr, char *targetStr, char *strBuf, int bufLen)
{
	char *pStr = NULL;
	int len = 0;

	if (NULL == (pStr = strstr(pHrdDesStr, targetStr)))
	{
		printf("Strstr failed!\n");
		return -1;
	}

	/* Ìø¹ý¿Õ°××Ö·û */
	pStr += strlen(targetStr);
	while ((*pStr == ' ') || (*pStr == '\t'))
	{
		pStr++;
	}

	/* »ñÈ¡Öµ */
	targetStr = pStr;
	while ('\0' != *pStr)
	{
		if ((*pStr == ' ') || (*pStr == '\t') || (*pStr == ',') || (*pStr == '}'))
		{
			break;
		}

		pStr++;
	}

	if ((len = (pStr - targetStr)) >= bufLen)
	{
		printf("No enough buff size!\n");
		return -1;
	}

	memcpy(strBuf, targetStr, len);
	strBuf[len] = '\0';

	return 0;
}

/* ´Óprofile»ñµÃÉè±¸ÐÍºÅ */
static void dev_name_read()
{
	char hw_id_des[HWIDDES_LEN] = {0};
	char devModel[DEVMODEL_STR_MAX_LEN] = {0};
	char hwVer[HWVER_STR_MAX_LEN] = {0};
	int ret = 0;
	int len = 0;

	memmove(hw_id_des, (char *)(CFG_FLASH_BASE + FACTORY_BOOT_LEN + HWIDDES_OFFSET), HWIDDES_LEN);

	/* read devMode from hardware description  */
	ret = getInfoFromHwIdDesStr(hw_id_des, HWID_DES_DEVMODEL, devModel, DEVMODEL_STR_MAX_LEN);
	if (ret != 0)
	{
		strcpy(dev_name, "unknown");
		printf("ERROR: can not get the 'devModel' from the profile!\n");
		return;
	}

	/* read hwVer from hardware description  */
	ret = getInfoFromHwIdDesStr(hw_id_des, HWID_DES_HWVER, hwVer, HWVER_STR_MAX_LEN);
	if (ret != 0)
	{
		strcpy(dev_name, "unknown");
		printf("ERROR: can not get the 'hwVer' from the profile!\n");
		return;
	}

	/* format device name: devMode + ¿Õ¸ñ + hwVer + '\0' */
	len = strlen(devModel) + 1 + strlen(hwVer) + 1;
	if (len > DEVNAME_STR_MAX_LEN)
	{
		strcpy(dev_name, "unknown");
		printf("ERROR: device name is too long! (devModel: %s, hwVer: %s)\n", devModel, hwVer);
		return;
	}

	sprintf(dev_name, "%s %s", devModel, hwVer);
	printf("Device name: %s\n", dev_name);
}

// http server init
void httpd_init(void)
{
	fs_init();
	upgrade_init();
	uip_listen(HTONS(HTTPD_LISTEN_PORT));

	/* we do not store device name in flash */
	#if 0
	dev_name_read();
	#endif
}

// reset app state
static void httpd_state_reset(void){
	hs->state = STATE_NONE;
	hs->count = 0;
	hs->dataptr = 0;
	hs->upload = 0;
	hs->upload_total = 0;

	data_start_found = 0;
	post_packet_counter = 0;

	if(boundary_value){
		free(boundary_value);
		boundary_value = NULL;
	}
}

// find and get first chunk of data
static int httpd_findandstore_firstchunk(void){
	char *start = NULL;
	char *end = NULL;

	if(!boundary_value){
		return(0);
	}

	// chek if we have data in packet
	start = (char *)strstr((char *)uip_appdata, (char *)boundary_value);

	if(start){

		// ok, we have data in this packet!
		// find start position of the data!
		end = (char *)strstr((char *)start, eol2);

		if(end){
			if((end - (char *)uip_appdata) < uip_len){

				// move pointer over CR LF CR LF
				end += 4;

				// how much data we expect?
				// last part (magic value 6): [CR][LF](boundary length)[-][-][CR][LF]
				hs->upload_total = hs->upload_total - (int)(end - start) - strlen(boundary_value) - 6;

				printf("Upload file size: %d bytes\n", hs->upload_total);

				printf("Loading: ");

				// how much data we are storing now?
				hs->upload = (unsigned int)(uip_len - (end - (char *)uip_appdata));

				memcpy((void *)webfailsafe_data_pointer, (void *)end, hs->upload);
				webfailsafe_data_pointer += hs->upload;

				httpd_download_progress();

				return(1);

			}

		} else {
			printf("## Error: couldn't find start of data!\n");
		}

	}

	return(0);
}

// called for http server app
void httpd_appcall(void){
	struct fs_file fsfile;
	unsigned int i;

	switch(uip_conn->lport){

		case HTONS(80):

			// app state
			hs = (struct httpd_state *)(uip_conn->appstate);

			// closed connection
			if(uip_closed()){
				httpd_state_reset();
				uip_close();
				return;
			}

			// aborted connection or time out occured
			if(uip_aborted() || uip_timedout()){
				httpd_state_reset();
				uip_abort();
				return;
			}

			// if we are pooled
			if(uip_poll()){
				if(hs->count++ >= 100){
					httpd_state_reset();
					uip_abort();
				}
				return;
			}

			// new connection
			if(uip_connected()){
				httpd_state_reset();
				return;
			}

			// new data in STATE_NONE
			if(uip_newdata() && hs->state == STATE_NONE){

				// GET or POST request?
				if(	((u8_t*)uip_appdata)[0] == ISO_G 
					&& ((u8_t*)uip_appdata)[1] == ISO_E 
					&& ((u8_t*)uip_appdata)[2] == ISO_T 
					&& (((u8_t*)uip_appdata)[3] == ISO_space
					|| ((u8_t*)uip_appdata)[3] == ISO_tab))
				{
					hs->state = STATE_FILE_REQUEST;
				} else if(((u8_t*)uip_appdata)[0] == ISO_P 
						&& ((u8_t*)uip_appdata)[1] == ISO_O 
						&& ((u8_t*)uip_appdata)[2] == ISO_S 
						&& ((u8_t*)uip_appdata)[3] == ISO_T 
						&& (((u8_t*)uip_appdata)[4] == ISO_space 
						|| ((u8_t*)uip_appdata)[4] == ISO_tab)){
					hs->state = STATE_UPLOAD_REQUEST;
				}

				// anything else -> abort the connection!
				if(hs->state == STATE_NONE)
				{
					httpd_state_reset();
					uip_abort();
					return;
				}

				// get file or firmware upload?
				if(hs->state == STATE_FILE_REQUEST){

					// we are looking for GET file name
					for(i = 4; i < 30; i++){
						if(	((u8_t*)uip_appdata)[i] == ISO_space 
							|| ((u8_t*)uip_appdata)[i] == ISO_cr 
							|| ((u8_t*)uip_appdata)[i] == ISO_nl 
							|| ((u8_t*)uip_appdata)[i] == ISO_tab){
							((u8_t*)uip_appdata)[i] = 0;
							i = 0;
							break;
						}
					}

					if(i != 0){
						printf("## Error: request file name too long!\n");
						httpd_state_reset();
						uip_abort();
						return;
					}

					printf("rport:%d; Request for: ", uip_conn->rport);
					printf("%s\n", &((u8_t*)uip_appdata)[4]);

					// request for /
					if(((u8_t*)uip_appdata)[4] == ISO_slash && ((u8_t*)uip_appdata)[5] == 0){
						char *s = INDEX_SCRIPT;
						int script_len = 0;

						i = fs_open(file_index_html.name, &fsfile);

						index_file = malloc(fsfile.len + strlen(INDEX_SCRIPT) + strlen(dev_name));
						if (index_file == NULL) {
							printf("ERROR: index_file MEM malloc error");
						}

						memcpy(index_file, fsfile.data, fsfile.len);
						script_len = sprintf(index_file + fsfile.len, s, dev_name);

						hs->dataptr = (u8_t *)index_file;
						hs->upload = fsfile.len + script_len;

						// send first (and maybe the last) chunk of data
						uip_send(hs->dataptr, (hs->upload > uip_mss() ? uip_mss() : hs->upload));
						return;
					} else {
						// check if we have requested file
						if(!fs_open((const char *)&((u8_t*)uip_appdata)[4], &fsfile)){
							printf("## Error: file not found!\n");
							fs_open(file_404_html.name, &fsfile);
						}
					}

					hs->state = STATE_FILE_REQUEST;
					hs->dataptr = (u8_t *)fsfile.data;
					hs->upload = fsfile.len;

					// send first (and maybe the last) chunk of data
					uip_send(hs->dataptr, (hs->upload > uip_mss() ? uip_mss() : hs->upload));
					return;
				}
				else if(hs->state == STATE_UPLOAD_REQUEST)
				{

					char *start = NULL;
					char *end = NULL;

					// end bufor data with NULL
					((u8_t*)uip_appdata)[uip_len] = '\0';

					/*
					 * We got first packet with POST request
					 *
					 * Some browsers don't include first chunk of data in the first
					 * POST request packet (like Google Chrome, IE and Safari)!
					 * So we must now find two values:
					 * - Content-Length
					 * - boundary
					 * Headers with these values can be in any order!
					 * If we don't find these values in first packet, connection will be aborted!
					 *
					 */

					// Content-Length pos
					start = (char *)strstr((char*)uip_appdata, "Content-Length:");

					if(start){

						start += sizeof("Content-Length:");

						// find end of the line with "Content-Length:"
						end = (char *)strstr(start, eol);

						if(end){

							hs->upload_total = atoi(start);
#ifdef DEBUG_UIP
							printf("Expecting %d bytes in body request message\n", hs->upload_total);
#endif

						} else {
							printf("## Error: couldn't find \"Content-Length\"!\n");
							httpd_state_reset();
							uip_abort();
							return;
						}

					}
					else {					
						printf("## Error: couldn't find \"Content-Length\"!\n");
						httpd_state_reset();
						uip_abort();
						return;
					}

					// we don't support very small files (< 10 KB)
					if(hs->upload_total < 10240){
						printf("## Error: request for upload < 10 KB data!\n");
						httpd_state_reset();
						uip_abort();
						return;
					}

					// boundary value
					start = NULL;
					end = NULL;

					start = (char *)strstr((char *)uip_appdata, "boundary=");

					if(start){

						// move pointer over "boundary="
						start += 9;

						// find end of line with boundary value
						end = (char *)strstr((char *)start, eol);

						if(end){

							// malloc space for boundary value + '--' and '\0'
							boundary_value = (char*)malloc(end - start + 3);

							if(boundary_value){

								memcpy(&boundary_value[2], start, end - start);

								// add -- at the begin and 0 at the end
								boundary_value[0] = '-';
								boundary_value[1] = '-';
								boundary_value[end - start + 2] = 0;

#ifdef DEBUG_UIP
								printf("Found boundary value: \"%s\"\n", boundary_value);
#endif

							} else {
								printf("## Error: couldn't allocate memory for boundary!\n");
								httpd_state_reset();
								uip_abort();
								return;
							}

						} else {
							printf("## Error: couldn't find boundary!\n");
							httpd_state_reset();
							uip_abort();
							return;
						}

					} else {
						printf("## Error: couldn't find boundary!\n");
						httpd_state_reset();
						uip_abort();
						return;
					}

					/*
					 * OK, if we are here, it means that we found
					 * Content-Length and boundary values in headers
					 *
					 * We can now try to 'allocate memory' and
					 * find beginning of the data in first packet
					 */

					webfailsafe_data_pointer = (u8_t *)WEBFAILSAFE_UPLOAD_RAM_ADDRESS;

					if(!webfailsafe_data_pointer){
						printf("## Error: couldn't allocate RAM for data!\n");
						httpd_state_reset();
						uip_abort();
						return;
					} else {
						printf("Data will be downloaded at 0x%X in RAM\n", WEBFAILSAFE_UPLOAD_RAM_ADDRESS);
					}

					if(httpd_findandstore_firstchunk()){
						data_start_found = 1;
					} else {
						data_start_found = 0;
					}

					return;

				} /* else if(hs->state == STATE_UPLOAD_REQUEST) */

			} /* uip_newdata() && hs->state == STATE_NONE */

			// if we got ACK from remote host
			if(uip_acked()){

				// if we are in STATE_FILE_REQUEST state
				if(hs->state == STATE_FILE_REQUEST){

					// data which we send last time was received (we got ACK)
					// if we send everything last time -> gently close the connection
					if(hs->upload <= uip_mss()){

						// post upload completed?
						if(webfailsafe_post_done){

							if(!webfailsafe_upload_failed){
								webfailsafe_ready_for_upgrade = 1;
							}

							webfailsafe_post_done = 0;
							webfailsafe_upload_failed = 0;
						}

						httpd_state_reset();
						uip_close();
						if (index_file != NULL) {
							free(index_file);
							index_file = NULL;
						}
						return;
					}

					// otherwise, send another chunk of data
					// last time we sent uip_conn->len size of data
					hs->dataptr += uip_conn->len;
					hs->upload -= uip_conn->len;

					UIP_LOG("line: %d==:rport:%d;hs->upload %d; "
							"last send uip_conn->len %d;"
							"now will send %d;"
							"  --debug by HouXB\n",
							__LINE__, uip_conn->rport,  hs->upload, uip_conn->len, 
							(hs->upload > uip_mss() ? uip_mss() : hs->upload));
					uip_send(hs->dataptr, (hs->upload > uip_mss() ? uip_mss() : hs->upload));
				}

				return;

			}

			// if we need to retransmit
			if(uip_rexmit()){

				// if we are in STATE_FILE_REQUEST state
				if(hs->state == STATE_FILE_REQUEST){
					// send again chunk of data without changing pointer and length of data left to send
					uip_send(hs->dataptr, (hs->upload > uip_mss() ? uip_mss() : hs->upload));
				}

				return;

			}

			// if we got new data frome remote host
			if(uip_newdata()){

				// if we are in STATE_UPLOAD_REQUEST state
				if(hs->state == STATE_UPLOAD_REQUEST){

					// end bufor data with NULL
					((u8_t*)uip_appdata)[uip_len] = '\0';

					// do we have to find start of data?
					if(!data_start_found){

						if(!httpd_findandstore_firstchunk()){
							printf("## Error: couldn't find start of data in next packet!\n");
							httpd_state_reset();
							uip_abort();
							return;
						} else {
							data_start_found = 1;
						}

						return;
					}

					hs->upload += (unsigned int)uip_len;

					if(!webfailsafe_upload_failed){
						memcpy((void *)webfailsafe_data_pointer, (void *)uip_appdata, uip_len);
						webfailsafe_data_pointer += uip_len;
					}

					httpd_download_progress();

					// if we have collected all data
					if(hs->upload >= hs->upload_total){

						printf("\n\n");

						// end of post upload
						webfailsafe_post_done = 1;
						NetBootFileXferSize = (ulong)hs->upload_total;

						if(do_http_check(NetBootFileXferSize)<0)
						{
							webfailsafe_upload_failed = 1;
						}
						else
						{
							webfailsafe_firmware_chk_success = 1;
						}

						// which website will be returned
						if(!webfailsafe_upload_failed){
							fs_open(file_flashing_html.name, &fsfile);
						} else {
							fs_open(file_fail_html.name, &fsfile);
						}

						httpd_state_reset();

						hs->state = STATE_FILE_REQUEST;
						hs->dataptr = (u8_t *)fsfile.data;
						hs->upload = fsfile.len;

						uip_send(hs->dataptr, (hs->upload > uip_mss() ? uip_mss() : hs->upload));
					}

				}

				return;
			}

			break;

		default:
			// we shouldn't get here... we are listening only on port 80
			uip_abort();
			break;
	}
}
