/*------------------------------------------------------------------------------------------------*/
/* UNICENS Generated Network Configuration                                                        */
/* Generator: xml2struct for Windows V4.4.0                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_api.h"

uint16_t PacketBandwidth = 12;
uint16_t RoutesSize = 3;
uint16_t NodeSize = 4;

/* Route 1 from source-node=0x200 to sink-node=0x2B0 */
Ucs_Xrm_DefaultCreatedPort_t SrcOfRoute1_DcPort = { 
    UCS_XRM_RC_TYPE_DC_PORT,
    UCS_XRM_PORT_TYPE_MLB,
    0 };
Ucs_Xrm_MlbSocket_t SrcOfRoute1_MlbSocket = { 
    UCS_XRM_RC_TYPE_MLB_SOCKET,
    &SrcOfRoute1_DcPort,
    UCS_SOCKET_DIR_INPUT,
    UCS_MLB_SCKT_SYNC_DATA,
    4,
    0x0A };
Ucs_Xrm_NetworkSocket_t SrcOfRoute1_NetworkSocket = { 
    UCS_XRM_RC_TYPE_NW_SOCKET,
    0x0D00,
    UCS_SOCKET_DIR_OUTPUT,
    UCS_NW_SCKT_SYNC_DATA,
    4 };
Ucs_Xrm_SyncCon_t SrcOfRoute1_SyncCon = { 
    UCS_XRM_RC_TYPE_SYNC_CON,
    &SrcOfRoute1_MlbSocket,
    &SrcOfRoute1_NetworkSocket,
    UCS_SYNC_MUTE_MODE_NO_MUTING,
    0 };
Ucs_Xrm_ResObject_t *SrcOfRoute1_JobList[] = {
    &SrcOfRoute1_DcPort,
    &SrcOfRoute1_MlbSocket,
    &SrcOfRoute1_NetworkSocket,
    &SrcOfRoute1_SyncCon,
    NULL };
Ucs_Xrm_NetworkSocket_t SnkOfRoute1_NetworkSocket = { 
    UCS_XRM_RC_TYPE_NW_SOCKET,
    0x0D00,
    UCS_SOCKET_DIR_INPUT,
    UCS_NW_SCKT_SYNC_DATA,
    4 };
Ucs_Xrm_DefaultCreatedPort_t SnkOfRoute1_DcPort = { 
    UCS_XRM_RC_TYPE_DC_PORT,
    UCS_XRM_PORT_TYPE_MLB,
    0 };
Ucs_Xrm_MlbSocket_t SnkOfRoute1_MlbSocket = { 
    UCS_XRM_RC_TYPE_MLB_SOCKET,
    &SnkOfRoute1_DcPort,
    UCS_SOCKET_DIR_OUTPUT,
    UCS_MLB_SCKT_SYNC_DATA,
    4,
    0x0A };
Ucs_Xrm_SyncCon_t SnkOfRoute1_SyncCon = { 
    UCS_XRM_RC_TYPE_SYNC_CON,
    &SnkOfRoute1_NetworkSocket,
    &SnkOfRoute1_MlbSocket,
    UCS_SYNC_MUTE_MODE_NO_MUTING,
    0 };
Ucs_Xrm_ResObject_t *SnkOfRoute1_JobList[] = {
    &SnkOfRoute1_NetworkSocket,
    &SnkOfRoute1_DcPort,
    &SnkOfRoute1_MlbSocket,
    &SnkOfRoute1_SyncCon,
    NULL };
/* Route 2 from source-node=0x200 to sink-node=0x270 */
Ucs_Xrm_NetworkSocket_t SnkOfRoute2_NetworkSocket = { 
    UCS_XRM_RC_TYPE_NW_SOCKET,
    0x0D00,
    UCS_SOCKET_DIR_INPUT,
    UCS_NW_SCKT_SYNC_DATA,
    4 };
Ucs_Xrm_StrmPort_t SnkOfRoute2_StrmPort0 = { 
    UCS_XRM_RC_TYPE_STRM_PORT,
    0,
    UCS_STREAM_PORT_CLK_CFG_64FS,
    UCS_STREAM_PORT_ALGN_LEFT16BIT };
Ucs_Xrm_StrmPort_t SnkOfRoute2_StrmPort1 = { 
    UCS_XRM_RC_TYPE_STRM_PORT,
    1,
    UCS_STREAM_PORT_CLK_CFG_WILD,
    UCS_STREAM_PORT_ALGN_LEFT16BIT };
Ucs_Xrm_StrmSocket_t SnkOfRoute2_StrmSocket = { 
    UCS_XRM_RC_TYPE_STRM_SOCKET,
    &SnkOfRoute2_StrmPort0,
    UCS_SOCKET_DIR_OUTPUT,
    UCS_STREAM_PORT_SCKT_SYNC_DATA,
    4,
    UCS_STREAM_PORT_PIN_ID_SRXA0 };
Ucs_Xrm_SyncCon_t SnkOfRoute2_SyncCon = { 
    UCS_XRM_RC_TYPE_SYNC_CON,
    &SnkOfRoute2_NetworkSocket,
    &SnkOfRoute2_StrmSocket,
    UCS_SYNC_MUTE_MODE_NO_MUTING,
    0 };
Ucs_Xrm_ResObject_t *SnkOfRoute2_JobList[] = {
    &SnkOfRoute2_NetworkSocket,
    &SnkOfRoute2_StrmPort0,
    &SnkOfRoute2_StrmPort1,
    &SnkOfRoute2_StrmSocket,
    &SnkOfRoute2_SyncCon,
    NULL };
/* Route 3 from source-node=0x200 to sink-node=0x240 */
Ucs_Xrm_NetworkSocket_t SnkOfRoute3_NetworkSocket = { 
    UCS_XRM_RC_TYPE_NW_SOCKET,
    0x0D00,
    UCS_SOCKET_DIR_INPUT,
    UCS_NW_SCKT_SYNC_DATA,
    4 };
Ucs_Xrm_StrmPort_t SnkOfRoute3_StrmPort0 = { 
    UCS_XRM_RC_TYPE_STRM_PORT,
    0,
    UCS_STREAM_PORT_CLK_CFG_64FS,
    UCS_STREAM_PORT_ALGN_LEFT16BIT };
Ucs_Xrm_StrmPort_t SnkOfRoute3_StrmPort1 = { 
    UCS_XRM_RC_TYPE_STRM_PORT,
    1,
    UCS_STREAM_PORT_CLK_CFG_WILD,
    UCS_STREAM_PORT_ALGN_LEFT16BIT };
Ucs_Xrm_StrmSocket_t SnkOfRoute3_StrmSocket = { 
    UCS_XRM_RC_TYPE_STRM_SOCKET,
    &SnkOfRoute3_StrmPort0,
    UCS_SOCKET_DIR_OUTPUT,
    UCS_STREAM_PORT_SCKT_SYNC_DATA,
    4,
    UCS_STREAM_PORT_PIN_ID_SRXA1 };
Ucs_Xrm_SyncCon_t SnkOfRoute3_SyncCon = { 
    UCS_XRM_RC_TYPE_SYNC_CON,
    &SnkOfRoute3_NetworkSocket,
    &SnkOfRoute3_StrmSocket,
    UCS_SYNC_MUTE_MODE_NO_MUTING,
    0 };
Ucs_Xrm_ResObject_t *SnkOfRoute3_JobList[] = {
    &SnkOfRoute3_NetworkSocket,
    &SnkOfRoute3_StrmPort0,
    &SnkOfRoute3_StrmPort1,
    &SnkOfRoute3_StrmSocket,
    &SnkOfRoute3_SyncCon,
    NULL };
UCS_NS_CONST uint8_t PayloadRequest1ForNode270[] = {
    0x00, 0x00, 0x01, 0x01 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Request1ForNode270 = {
    0x00,
    0x01,
    0x06C1,
    0x02,
    0x04,
    PayloadRequest1ForNode270 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Response1ForNode270 = {
    0x00,
    0x01,
    0x06C1,
    0x0C,
    0x00,
    NULL };
UCS_NS_CONST uint8_t PayloadRequest2ForNode270[] = {
    0x0F, 0x00, 0x00, 0x00, 0x2A, 0x02, 0x00, 0x64, 0x1B, 0x80 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Request2ForNode270 = {
    0x00,
    0x01,
    0x06C4,
    0x02,
    0x0A,
    PayloadRequest2ForNode270 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Response2ForNode270 = {
    0x00,
    0x01,
    0x06C4,
    0x0C,
    0x00,
    NULL };
UCS_NS_CONST uint8_t PayloadRequest3ForNode270[] = {
    0x0F, 0x00, 0x00, 0x00, 0x2A, 0x02, 0x00, 0x64, 0x11, 0xB8 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Request3ForNode270 = {
    0x00,
    0x01,
    0x06C4,
    0x02,
    0x0A,
    PayloadRequest3ForNode270 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Response3ForNode270 = {
    0x00,
    0x01,
    0x06C4,
    0x0C,
    0x00,
    NULL };
UCS_NS_CONST uint8_t PayloadRequest4ForNode270[] = {
    0x0F, 0x00, 0x00, 0x00, 0x2A, 0x02, 0x00, 0x64, 0x12, 0x60 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Request4ForNode270 = {
    0x00,
    0x01,
    0x06C4,
    0x02,
    0x0A,
    PayloadRequest4ForNode270 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Response4ForNode270 = {
    0x00,
    0x01,
    0x06C4,
    0x0C,
    0x00,
    NULL };
UCS_NS_CONST uint8_t PayloadRequest5ForNode270[] = {
    0x0F, 0x00, 0x00, 0x00, 0x2A, 0x02, 0x00, 0x64, 0x13, 0xA0 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Request5ForNode270 = {
    0x00,
    0x01,
    0x06C4,
    0x02,
    0x0A,
    PayloadRequest5ForNode270 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Response5ForNode270 = {
    0x00,
    0x01,
    0x06C4,
    0x0C,
    0x00,
    NULL };
UCS_NS_CONST uint8_t PayloadRequest6ForNode270[] = {
    0x0F, 0x00, 0x00, 0x00, 0x2A, 0x02, 0x00, 0x64, 0x14, 0x48 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Request6ForNode270 = {
    0x00,
    0x01,
    0x06C4,
    0x02,
    0x0A,
    PayloadRequest6ForNode270 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Response6ForNode270 = {
    0x00,
    0x01,
    0x06C4,
    0x0C,
    0x00,
    NULL };
UCS_NS_CONST uint8_t PayloadRequest7ForNode270[] = {
    0x0F, 0x00, 0x00, 0x00, 0x2A, 0x05, 0x00, 0x64, 0x20, 0x00, 0x89, 0x77, 0x72 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Request7ForNode270 = {
    0x00,
    0x01,
    0x06C4,
    0x02,
    0x0D,
    PayloadRequest7ForNode270 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Response7ForNode270 = {
    0x00,
    0x01,
    0x06C4,
    0x0C,
    0x00,
    NULL };
UCS_NS_CONST uint8_t PayloadRequest8ForNode270[] = {
    0x0F, 0x00, 0x00, 0x00, 0x2A, 0x02, 0x00, 0x64, 0x06, 0x00 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Request8ForNode270 = {
    0x00,
    0x01,
    0x06C4,
    0x02,
    0x0A,
    PayloadRequest8ForNode270 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Response8ForNode270 = {
    0x00,
    0x01,
    0x06C4,
    0x0C,
    0x00,
    NULL };
UCS_NS_CONST uint8_t PayloadRequest9ForNode270[] = {
    0x0F, 0x00, 0x00, 0x00, 0x2A, 0x02, 0x00, 0x64, 0x05, 0x00 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Request9ForNode270 = {
    0x00,
    0x01,
    0x06C4,
    0x02,
    0x0A,
    PayloadRequest9ForNode270 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Response9ForNode270 = {
    0x00,
    0x01,
    0x06C4,
    0x0C,
    0x00,
    NULL };
UCS_NS_CONST uint8_t PayloadRequest10ForNode270[] = {
    0x0F, 0x00, 0x00, 0x00, 0x2A, 0x03, 0x00, 0x64, 0x07, 0x01, 0x50 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Request10ForNode270 = {
    0x00,
    0x01,
    0x06C4,
    0x02,
    0x0B,
    PayloadRequest10ForNode270 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Response10ForNode270 = {
    0x00,
    0x01,
    0x06C4,
    0x0C,
    0x00,
    NULL };
UCS_NS_CONST Ucs_Ns_Script_t ScriptsForNode270[] = {
    {
        0,
        &Request1ForNode270,
        &Response1ForNode270
    }, {
        0,
        &Request2ForNode270,
        &Response2ForNode270
    }, {
        0,
        &Request3ForNode270,
        &Response3ForNode270
    }, {
        0,
        &Request4ForNode270,
        &Response4ForNode270
    }, {
        0,
        &Request5ForNode270,
        &Response5ForNode270
    }, {
        0,
        &Request6ForNode270,
        &Response6ForNode270
    }, {
        0,
        &Request7ForNode270,
        &Response7ForNode270
    }, {
        0,
        &Request8ForNode270,
        &Response8ForNode270
    }, {
        0,
        &Request9ForNode270,
        &Response9ForNode270
    }, {
        0,
        &Request10ForNode270,
        &Response10ForNode270
    } };
UCS_NS_CONST uint8_t PayloadRequest1ForNode240[] = {
    0x00, 0x00, 0x01, 0x01 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Request1ForNode240 = {
    0x00,
    0x01,
    0x06C1,
    0x02,
    0x04,
    PayloadRequest1ForNode240 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Response1ForNode240 = {
    0x00,
    0x01,
    0x06C1,
    0x0C,
    0x00,
    NULL };
UCS_NS_CONST uint8_t PayloadRequest2ForNode240[] = {
    0x0F, 0x00, 0x02, 0x0A, 0x18, 0x03, 0x00, 0x64, 0x00, 0x0F, 0x02, 0x01, 0x00, 0x00, 0x02, 0xA5, 0xDF, 0x03, 0x3F, 0x3F, 0x04, 0x02, 0x02, 0x10, 0x40, 0x40, 0x11, 0x00, 0x00, 0x12, 0x00, 0x00, 0x13, 0x00, 0x00, 0x14, 0x00, 0x00 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Request2ForNode240 = {
    0x00,
    0x01,
    0x06C4,
    0x02,
    0x26,
    PayloadRequest2ForNode240 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Response2ForNode240 = {
    0x00,
    0x01,
    0x06C4,
    0x0C,
    0x00,
    NULL };
UCS_NS_CONST uint8_t PayloadRequest3ForNode240[] = {
    0x0F, 0x00, 0x02, 0x04, 0x18, 0x03, 0x00, 0x64, 0x20, 0x00, 0x00, 0x21, 0x00, 0x00, 0x22, 0x00, 0x00, 0x23, 0x00, 0x00 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Request3ForNode240 = {
    0x00,
    0x01,
    0x06C4,
    0x02,
    0x14,
    PayloadRequest3ForNode240 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Response3ForNode240 = {
    0x00,
    0x01,
    0x06C4,
    0x0C,
    0x00,
    NULL };
UCS_NS_CONST Ucs_Ns_Script_t ScriptsForNode240[] = {
    {
        0,
        &Request1ForNode240,
        &Response1ForNode240
    }, {
        0,
        &Request2ForNode240,
        &Response2ForNode240
    }, {
        0,
        &Request3ForNode240,
        &Response3ForNode240
    } };
Ucs_Signature_t SignatureForNode200 = { 0x200 };
Ucs_Signature_t SignatureForNode2B0 = { 0x2B0 };
Ucs_Signature_t SignatureForNode270 = { 0x270 };
Ucs_Signature_t SignatureForNode240 = { 0x240 };
Ucs_Rm_Node_t AllNodes[] = {
    {
        &SignatureForNode200,
        NULL,
        0
    }, {
        &SignatureForNode2B0,
        NULL,
        0
    }, {
        &SignatureForNode270,
        ScriptsForNode270,
        10
    }, {
        &SignatureForNode240,
        ScriptsForNode240,
        3
    } };
Ucs_Rm_EndPoint_t SourceEndpointForRoute1 = {
    UCS_RM_EP_SOURCE,
    SrcOfRoute1_JobList,
    &AllNodes[0] };
Ucs_Rm_EndPoint_t SinkEndpointForRoute1 = {
    UCS_RM_EP_SINK,
    SnkOfRoute1_JobList,
    &AllNodes[1] };
Ucs_Rm_EndPoint_t SinkEndpointForRoute2 = {
    UCS_RM_EP_SINK,
    SnkOfRoute2_JobList,
    &AllNodes[2] };
Ucs_Rm_EndPoint_t SinkEndpointForRoute3 = {
    UCS_RM_EP_SINK,
    SnkOfRoute3_JobList,
    &AllNodes[3] };
Ucs_Rm_Route_t AllRoutes[] = { {
        &SourceEndpointForRoute1,
        &SinkEndpointForRoute1,
        1,
        0x0010
    }, {
        &SourceEndpointForRoute1,
        &SinkEndpointForRoute2,
        1,
        0x0011
    }, {
        &SourceEndpointForRoute1,
        &SinkEndpointForRoute3,
        1,
        0x0012
    } };
