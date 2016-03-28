


#ifndef  __PROTOCAL_CONN__H
#define  __PROTOCAL_CONN__H0

#include "data.h"


extern ConnectREQ_INFO 	ConnectREQ;
extern ConnectComfirm	*ConnectCF;
extern CServer_REQ	*CServer_req;
extern  CServer_HB	CS_HB;
extern CServer_FULL_UPDATE	*CS_FUP;
extern DateTime		HB_DateTime[30];
extern  int		HB_DateTime_cnt;

int protocol_init(unsigned  int cid, int flag);
void protocol_comfirm_ready(struct connect_serv *conn_serv, u8 PresentMode, u16 SubMode);
void protocol_req_ready(ConnectREQ_INFO *req);
void protocol_CSREQ_ready(void);
void protocol_CSHB_ready(void);
void protocol_CSFUP_ready(void);
void protocol_CSUP_ready(void);

Present_INFO  DayofYear(void);	
DateTime	get_DateTime(void);
int 		diff_time(char CMD, DateTime *last, DateTime *now);


#endif  // protocal_conn.h



