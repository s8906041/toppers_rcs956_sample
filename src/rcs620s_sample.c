#include <kernel.h>
#include "kernel_cfg.h"
#include "fm3_mb9bxxx.h"

#include "cq_frk_fm3.h"
#include "rcs620s_sample.h"
#include "st7032i.h"

#include "HkNfcRw.h"
#include "HkNfcA.h"
#include "HkNfcF.h"
#include "HkNfcNdef.h"
#include "HkNfcSnep.h"

const char* STR_TITLE =				"Please card.    ";
const char* STR_SNEP_START =		"SNEP start      ";
const char* STR_FAIL_NDEF = 		"fail:NDEF create";
const char* STR_FAIL_PUTSTART = 	"fail:PutStart   ";
const char* STR_FAIL_PUTRESULT = 	"fail:Put        ";
const char* STR_SNEP_SUCCESS = 		"SNEP success!   ";
const char* STR_NFCA = 			"NFC-A";
const char* STR_NFCB = 			"NFC-B";
const char* STR_NFCF = 			"NFC-F";
const char* STR_TPE = 			":TPE";
const char* STR_CARD_BLANK =	"         ";

static void _snep(const char *pStr);
static void _snep_fail(const char *pStr);


void main_task(intptr_t exinf)
{
	HkNfcType type;

	st7032i_task_init();

	bool b = HkNfcRw_Open();
	if(!b) {
		cq_frk_fm3_led_ctrl(CQFRKFM3_LED_BLINK, 200, 0);
		ext_tsk();
	}

	//最初は(0, 0)にカーソルがある
	st7032i_write_string(STR_TITLE);

	while(1) {
		type = HkNfcRw_Detect(true, true, true);
		HkNfcRw_RfOff();
		st7032i_move_pos(1, 1);
		switch(type) {
		case HKNFCTYPE_A:
			cq_frk_fm3_led_ctrl(CQFRKFM3_LED_BLINK, 250, 3000);
			st7032i_write_string(STR_NFCA);
			{
				HkNfcASelRes selres = HkNfcA_GetSelRes();
				if(HKNFCA_IS_SELRES_TPE(selres)) {
					_snep(STR_NFCA);
				}
			}
			break;
		case HKNFCTYPE_B:
			cq_frk_fm3_led_ctrl(CQFRKFM3_LED_BLINK, 250, 3000);
			st7032i_write_string(STR_NFCB);
			break;
		case HKNFCTYPE_F:
			cq_frk_fm3_led_ctrl(CQFRKFM3_LED_BLINK, 200, 3000);
			st7032i_write_string(STR_NFCF);
			{
				uint8_t idm[NFCID2_LEN];
				if(HkNfcRw_GetNfcId(idm) != 0) {
					if(HKNFCF_IS_NFCID_TPE(idm)) {
						_snep(STR_NFCF);
					}
				}
			}
			break;
		default:
			st7032i_write_string(STR_CARD_BLANK);
			break;
		}
		dly_tsk(1000);
	}

	HkNfcRw_Close();
	ext_tsk();
}


static uint8_t _strlen(const char* pStr)
{
	uint8_t len = 0;
	while(*pStr++ != 0) {
		len++;
	}
	return len;
}

static void _snep(const char* pStr)
{
	HkNfcNdefMsg msg;
	bool b;

	st7032i_write_string(STR_TPE);
	st7032i_move_pos(0, 0);
	st7032i_write_string(STR_SNEP_START);

	b = HkNfcNdef_CreateText(&msg, pStr, _strlen(pStr), LANGCODE_EN);
	if(!b) {
		_snep_fail(STR_FAIL_NDEF);
		return;
	}

	
	/* Initiatorの場合、最後にPollingしたTechnologyでDEPする */
	b = HkNfcSnep_PutStart(HKNFCSNEP_MD_INITIATOR, &msg);
	if(!b) {
		_snep_fail(STR_FAIL_PUTSTART);
		return;
	}
	
	while(HkNfcSnep_Poll()) {
		;
	}
	
	if(HkNfcSnep_GetResult() != HKNFCSNEP_SUCCESS) {
		_snep_fail(STR_FAIL_PUTRESULT);
		return;
	}

	cq_frk_fm3_led_ctrl(CQFRKFM3_LED_BLINK, 500, 5000);
	st7032i_move_pos(0, 0);
	st7032i_write_string(STR_SNEP_SUCCESS);
	
	dly_tsk(5000);
	st7032i_move_pos(0, 0);
	st7032i_write_string(STR_TITLE);
	st7032i_move_pos(1, 1);
	st7032i_write_string(STR_CARD_BLANK);
}


static void _snep_fail(const char *pStr)
{
	cq_frk_fm3_led_ctrl(CQFRKFM3_LED_BLINK, 250, 5000);
	st7032i_move_pos(0, 0);
	st7032i_write_string(pStr);
	
	dly_tsk(10000);

	st7032i_move_pos(0, 0);
	st7032i_write_string(STR_TITLE);
	st7032i_move_pos(1, 1);
	st7032i_write_string(STR_CARD_BLANK);
}
