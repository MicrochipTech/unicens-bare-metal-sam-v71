#include <assert.h>
#include "gmac_init.h"
#include <string.h>

/** Enable/Disable CopyAllFrame */
#define GMAC_CAF_DISABLE    0
#define GMAC_CAF_ENABLE     1

/** Enable/Disable NoBroadCast */
#define GMAC_NBC_DISABLE    0
#define GMAC_NBC_ENABLE     1


#define PTP_RX_BUFFERS          16    /** Must be a power of 2 */
#define PTP_RX_BUFF_SIZE        256    /** Must be a power of 2 */

#define DUMMY_BUFFERS           4     /** Must be a power of 2 */
#define DUMMY_BUFF_SIZE         128    /** Must be a power of 2 */

#define AVB_RX_BUFFERS          8
#define AVB_TX_BUFFERS          8
#define AVB_BUFF_SIZE           1280

#define ETH_RX_BUFFERS          8
#define ETH_TX_BUFFERS          8

#define TSU_INCR_NS         (6)
#define TSU_INCR_SUBNS      ((uint16_t)(0.66667f * 65536.f))


#define DESCRIPTOR COMPILER_SECTION(".ram_nocache") COMPILER_ALIGNED(8)
#define GMACBUFFER COMPILER_SECTION(".ram_nocache") COMPILER_ALIGNED(DEFAULT_CACHELINE)

DESCRIPTOR sGmacRxDescriptor gPtpRxDs[PTP_RX_BUFFERS];
DESCRIPTOR sGmacTxDescriptor gPtpTxDs[DUMMY_BUFFERS];

DESCRIPTOR sGmacRxDescriptor gAvbRxDs[AVB_RX_BUFFERS];
DESCRIPTOR sGmacTxDescriptor gAvbTxDs[AVB_TX_BUFFERS];

DESCRIPTOR sGmacRxDescriptor gEthRxDs[ETH_RX_BUFFERS];
DESCRIPTOR sGmacTxDescriptor gEthTxDs[ETH_TX_BUFFERS];


GMACBUFFER uint8_t gRxPtpBuffer[PTP_RX_BUFFERS * PTP_RX_BUFF_SIZE];
GMACBUFFER uint8_t gTxPtpBuffer[DUMMY_BUFFERS * DUMMY_BUFF_SIZE];

GMACBUFFER uint8_t gRxAvbBuffer[AVB_RX_BUFFERS * AVB_BUFF_SIZE];
GMACBUFFER uint8_t gTxAvbBuffer[AVB_TX_BUFFERS * AVB_BUFF_SIZE];

GMACBUFFER uint8_t gRxEthBuffer[ETH_RX_BUFFERS * ETH_BUFF_SIZE];
GMACBUFFER uint8_t gTxEthBuffer[ETH_TX_BUFFERS * ETH_BUFF_SIZE];

DESCRIPTOR fGmacdTransferCallback gPtpTxCbs[DUMMY_BUFFERS];
DESCRIPTOR fGmacdTransferCallback gAvbTxCbs[AVB_TX_BUFFERS];
DESCRIPTOR fGmacdTransferCallback gEthTxCbs[ETH_TX_BUFFERS];

DESCRIPTOR void *gPtpTxCbTags[DUMMY_BUFFERS];
DESCRIPTOR void *gAvbTxCbTags[AVB_TX_BUFFERS];
DESCRIPTOR void *gEthTxCbTags[ETH_TX_BUFFERS];

#define PTP_ETHER_TYPE (0x88F7u)
#define AVB_ETHER_TYPE (0x22F0u)
#define ARP_ETHER_TYPE (0x0806u)

const gmacQueList_t PTP_QUEUE = GMAC_QUE_0;

static void PtpDataReceived(uint32_t status, void *pTag);
static void gmac_RxPtpEvtMsgIsrCB (ptpMsgType rxEvtMsg, uint32_t efrsh, uint32_t efrsl, uint32_t eftn);
static void gmac_TxPtpEvtMsgIsrCB (ptpMsgType txEvtMsg, uint32_t eftsh, uint32_t eftsl, uint32_t eftn, uint16_t sequenceId);

static sGmacd *spGmacd;
static sGmacInit QuePTP;
static sGmacInit QueAVB;
static sGmacInit QueETH;

void init_gmac(sGmacd  *pGmacd)
{
  assert(NULL != pGmacd);
  spGmacd = pGmacd;

  memset(&QuePTP, 0, sizeof(QuePTP));
  memset(&QueAVB, 0, sizeof(QueAVB));
  memset(&QueETH, 0, sizeof(QueETH));

  /* Initialize GMAC driver structure */
  QuePTP.bIsGem          = 1;
  QuePTP.bDmaBurstLength = 4;
  QuePTP.pRxBuffer       = gRxPtpBuffer;
  QuePTP.pRxD            = gPtpRxDs;
  QuePTP.wRxBufferSize   = PTP_RX_BUFF_SIZE;
  QuePTP.wRxSize         = PTP_RX_BUFFERS;
  QuePTP.pTxBuffer       = gTxPtpBuffer;
  QuePTP.pTxD            = gPtpTxDs;
  QuePTP.wTxBufferSize   = DUMMY_BUFF_SIZE;
  QuePTP.wTxSize         = DUMMY_BUFFERS;
  QuePTP.pTxCb           = gPtpTxCbs;
  QuePTP.pTxCbTag        = gPtpTxCbTags;

  QueETH.bIsGem          = 1;
  QueETH.bDmaBurstLength = 4;
  QueETH.pRxBuffer       = gRxEthBuffer;
  QueETH.pRxD            = gEthRxDs;
  QueETH.wRxBufferSize   = ETH_BUFF_SIZE;
  QueETH.wRxSize         = ETH_RX_BUFFERS;
  QueETH.pTxBuffer       = gTxEthBuffer;
  QueETH.pTxD            = gEthTxDs;
  QueETH.wTxBufferSize   = ETH_BUFF_SIZE;
  QueETH.wTxSize         = ETH_TX_BUFFERS;
  QueETH.pTxCb           = gEthTxCbs;
  QueETH.pTxCbTag        = gEthTxCbTags;

  QueAVB.bIsGem          = 1;
  QueAVB.bDmaBurstLength = 4;
  QueAVB.pRxBuffer       = gRxAvbBuffer;
  QueAVB.pRxD            = gAvbRxDs;
  QueAVB.wRxBufferSize   = AVB_BUFF_SIZE;
  QueAVB.wRxSize         = AVB_RX_BUFFERS;
  QueAVB.pTxBuffer       = gTxAvbBuffer;
  QueAVB.pTxD            = gAvbTxDs;
  QueAVB.wTxBufferSize   = AVB_BUFF_SIZE;
  QueAVB.wTxSize         = AVB_TX_BUFFERS;
  QueAVB.pTxCb           = gAvbTxCbs;
  QueAVB.pTxCbTag        = gAvbTxCbTags;

  GMACD_Init(pGmacd, GMAC, ID_GMAC, GMAC_CAF_ENABLE, GMAC_NBC_DISABLE);
  GMACD_InitTransfer(pGmacd, &QueETH,  GMAC_QUE_0);
  GMACD_InitTransfer(pGmacd, &QueAVB,  GMAC_QUE_1);
  GMACD_InitTransfer(pGmacd, &QuePTP,  GMAC_QUE_2);

  GMAC_SetTsuTmrIncReg(GMAC, TSU_INCR_NS, TSU_INCR_SUBNS);

  /* PTP events can only be registered to QUEUE0! */
  /* The packets can be rerouted to any queue */
  GMACD_RxPtpEvtMsgCBRegister (pGmacd, gmac_RxPtpEvtMsgIsrCB, PTP_QUEUE);
  GMACD_TxPtpEvtMsgCBRegister (pGmacd, gmac_TxPtpEvtMsgIsrCB, PTP_QUEUE);
  GMAC_EnableIt(GMAC, (GMAC_IER_SFR | GMAC_IER_PDRQFR | GMAC_IER_PDRSFR), PTP_QUEUE);
  GMAC_EnableIt(GMAC, (GMAC_IER_PDRQFT | GMAC_IER_PDRSFT), PTP_QUEUE );

  /* QUE must match screener register configuration! */
  GMACD_SetRxCallback(pGmacd, PtpDataReceived, GMAC_QUE_1);
  GMACD_RxPtpEvtMsgCBRegister(pGmacd, gmac_RxPtpEvtMsgIsrCB, PTP_QUEUE);
  GMACD_TxPtpEvtMsgCBRegister(pGmacd, gmac_TxPtpEvtMsgIsrCB, PTP_QUEUE);

  enum {
    AVB_ETHER_TYPE_REG_IDX = 0,
    PTP_ETHER_TYPE_REG_IDX,
    ARP_ETHER_TYPE_REG_IDX,
    unused,
    ETHER_TYPE_REG_MAX
  };

  // AVB
  GMAC_WriteScreener2Reg(pGmacd->pHw, AVB_ETHER_TYPE_REG_IDX, ( GMAC_ST2RPQ_ETHE | GMAC_ST2RPQ_I2ETH(AVB_ETHER_TYPE_REG_IDX) | GMAC_ST2RPQ_QNB(GMAC_QUE_1) ) );
  GMAC_WriteEthTypeReg(pGmacd->pHw, AVB_ETHER_TYPE_REG_IDX, GMAC_ST2ER_COMPVAL(AVB_ETHER_TYPE));

  // PTP
  GMAC_WriteScreener2Reg(pGmacd->pHw, PTP_ETHER_TYPE_REG_IDX, ( GMAC_ST2RPQ_ETHE | GMAC_ST2RPQ_I2ETH(PTP_ETHER_TYPE_REG_IDX) | GMAC_ST2RPQ_QNB(GMAC_QUE_2) ) );
  GMAC_WriteEthTypeReg(pGmacd->pHw, PTP_ETHER_TYPE_REG_IDX, GMAC_ST2ER_COMPVAL(PTP_ETHER_TYPE));

  // ARP
  GMAC_WriteScreener2Reg(pGmacd->pHw, ARP_ETHER_TYPE_REG_IDX, ( GMAC_ST2RPQ_ETHE | GMAC_ST2RPQ_I2ETH(ARP_ETHER_TYPE_REG_IDX) | GMAC_ST2RPQ_QNB(GMAC_QUE_0) ) );
  GMAC_WriteEthTypeReg(pGmacd->pHw, ARP_ETHER_TYPE_REG_IDX, GMAC_ST2ER_COMPVAL(ARP_ETHER_TYPE));

  NVIC_ClearPendingIRQ(GMAC_IRQn);
  NVIC_EnableIRQ(GMAC_IRQn);
  NVIC_ClearPendingIRQ(GMACQ1_IRQn);
  NVIC_EnableIRQ(GMACQ1_IRQn);
  NVIC_ClearPendingIRQ(GMACQ2_IRQn);
  NVIC_EnableIRQ(GMACQ2_IRQn);

  if((CHIPID->CHIPID_CIDR & CHIPID_CIDR_VERSION_Msk) == 0x01)
  {
    //MRLB
  }
}

/**
 * Gmac interrupt handler
 */
void GMAC_Handler(void)
{
    assert(NULL != spGmacd);
    GMACD_Handler(spGmacd, GMAC_QUE_0);
}

void GMACQ1_Handler (void)
{
    assert(NULL != spGmacd);
    GMACD_Handler(spGmacd, GMAC_QUE_1);
}

void GMACQ2_Handler (void)
{
    assert(NULL != spGmacd);
    GMACD_Handler(spGmacd, GMAC_QUE_2);
}

void GMACQ3_Handler(void)
{
    assert(NULL != spGmacd);
    GMACD_Handler(spGmacd, GMAC_QUE_3);
}

void GMACQ4_Handler(void)
{
    assert(NULL != spGmacd);
    GMACD_Handler(spGmacd, GMAC_QUE_4);
}

void GMACQ5_Handler(void)
{
    assert(NULL != spGmacd);
    GMACD_Handler(spGmacd, GMAC_QUE_5);
}

/* call back routine for PTP Queue */
static void PtpDataReceived(uint32_t status, void *pTag)
{
  uint32_t buffIdx;
  uint32_t frmSize;
  uint8_t *msgPtr;
  ptpMsgType ptpMsg;
  pTag = pTag;

  TRACE_INFO("%u ETH_RXCB(%u)\n\r", (unsigned int)GetTicks(), (unsigned int)status);
  assert(NULL != spGmacd);

  while(GMACD_OK == GMACD_GetRxDIdx(spGmacd, &buffIdx, &frmSize, PTP_QUEUE)) {

    msgPtr = (uint8_t *)&gRxPtpBuffer[buffIdx * PTP_RX_BUFF_SIZE];

    ptpMsg = (ptpMsgType)(msgPtr[14] & 0x0Fu);

    switch(ptpMsg) {
    case SYNC_MSG_TYPE:
      //gPtpRxdSyncMsg(msgPtr);
      break;
    case FOLLOW_UP_MSG_TYPE:
      //gPtpRxdFollowUpMsg(msgPtr);
      break;
    case PDELAY_REQ_TYPE:
      //gPtpRxdPdelayReqMsg(msgPtr);
      break;
    case PDELAY_RESP_TYPE:
      //gPtpRxdPdelayRespMsg(msgPtr);
      break;
    case PDELAY_RESP_FOLLOW_UP_MSG_TYPE:
      //gPtpRxdPdelayRespFollowUpMsg(msgPtr);
      break;
    default:
      break;
    }; /* switch (ptpMsg) */

    GMACD_FreeRxDTail(spGmacd, PTP_QUEUE);

  } /* while () */
}

static void gmac_RxPtpEvtMsgIsrCB(ptpMsgType rxEvtMsg, uint32_t efrsh, uint32_t efrsl, uint32_t efrn)
{
    efrsh = efrsh;
    efrsl = efrsl;
    efrn = efrn;
  switch(rxEvtMsg) {
    case SYNC_MSG_TYPE:
    case PDELAY_REQ_TYPE:
    case PDELAY_RESP_TYPE:
    default:
      break;
  }
}

static void gmac_TxPtpEvtMsgIsrCB(ptpMsgType txEvtMsg, uint32_t eftsh, uint32_t eftsl, uint32_t eftn, uint16_t sequenceId)
{
    eftsh = eftsh;
    eftsl = eftsl;
    eftn = eftn;
    sequenceId = sequenceId;
  switch(txEvtMsg) {
    case PDELAY_REQ_TYPE:
    case PDELAY_RESP_TYPE:
      break;
    case DELAY_REQ_MSG_TYPE:
    case FOLLOW_UP_MSG_TYPE:
    case DELAY_RESP_MSG_TYPE:
    default:
        break;
  }
}

