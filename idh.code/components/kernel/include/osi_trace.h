/* Copyright (C) 2018 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * This software is supplied "AS IS" without any warranties.
 * RDA assumes no responsibility or liability for the use of the software,
 * conveys no license or title under any patent, copyright, or mask work
 * right to the product. RDA reserves the right to make changes in the
 * software without notification.  RDA also make no representation or
 * warranty that such application will be suitable for the specified use
 * without further testing or modification.
 */

#ifndef _OSI_TRACE_H_
#define _OSI_TRACE_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "osi_api.h"

OSI_EXTERN_C_BEGIN

/**
 * \brief trace output device
 */
typedef enum
{
    OSI_TRACE_DEVICE_NONE = 0,                   ///< no output
    OSI_TRACE_DEVICE_DEBUGHOST = 0x01,           ///< output through debughost
    OSI_TRACE_DEVICE_USBSERIAL = 0x02,           ///< output through USB serial
    OSI_TRACE_DEVICE_FILESYS = 0x04,             ///< output to file system, usually on SDCARD
    OSI_TRACE_DEVICE_DEBUGHOST_USBSERIAL = 0x03, ///< output to both debughost and USB serial
} osiTraceDevice_t;

/**
 * \brief function prototype to send out trace data
 *
 * \param data      trace data
 * \param size      trace data size
 * \return
 *      - true if the trace data is output
 *      - false if output fails
 */
typedef bool (*osiTraceSender_t)(const void *data, unsigned size);

/**
 * \brief data structure of host packet
 *
 * *frame length* is the length of host packet, *exclude* header. So,
 * frame length is the packet length minus 4.
 */
typedef struct
{
    uint8_t sync;          ///< sync byte, 0xAD
    uint8_t frame_len_msb; ///< MSB byte of frame length
    uint8_t frame_len_lsb; ///< LSB byte of frame length
    uint8_t flowid;        ///< flow ID
} osiHostPacketHeader_t;

/**
 * \brief tra packet header data structure
 */
typedef struct
{
    uint16_t sync;     ///< sync 16bits word, 0xBBBB
    uint8_t plat_id;   ///< platform id, fill 1 at unsure.
    uint8_t type;      ///< tra packet type
    uint32_t sn;       ///< serial number
    uint32_t fn_wcdma; ///< WCDMA frame number, fill tick count when there are no WCDMA
    uint32_t fn_gge;   ///< GSM frame number
    uint32_t fn_lte;   ///< LTE frame number
} osiTraPacketHeader_t;

/**
 * \brief global trace sequence number
 *
 * *Don't* modify it, unless in trace output engine.
 */
extern uint32_t gTraceSequence;

/**
 * \brief global variable for trace enable/disable
 *
 * *Don't* modify it. It is only for trace API for faster check,
 * to save a little cycles than calling API.
 */
extern bool gTraceEnabled;

/**
 * \brief set trace enable or not
 *
 * @param enable    false for disable, true for enable
 */
void osiTraceSetEnable(bool enable);

/**
 * \brief get GSM frame number
 *
 * When GSM frame number is needed to be embedded into trace, this function
 * should be implemented outside trace module.
 *
 * \return      GSM frame number
 */
uint32_t osiTraceGsmFrameNumber(void);

/**
 * \brief get LTE frame number
 *
 * When LTE frame number is needed to be embedded into trace, this function
 * should be implemented outside trace module.
 *
 * \return      GSM frame number
 */
uint32_t osiTraceLteFrameNumber(void);

/**
 * \brief fill host packet header
 *
 * It is just a helper to fill host packet header.
 *
 * \param [in] header   host packet header
 * \param [in] flowid   host packet flowid
 * \param [in] frame_len    host packet frame length
 */
static inline void osiFillHostHeader(osiHostPacketHeader_t *header, uint8_t flowid, uint16_t frame_len)
{
    header->sync = 0xAD;
    header->frame_len_msb = frame_len >> 8;
    header->frame_len_lsb = frame_len & 0xff;
    header->flowid = flowid;
}

/**
 * \brief trace module initialization
 *
 * It should be called only once at system boot. When this is called,
 * it is possible that RTOS hasn't initialized. So, it will only
 * initialize trace data management data structure, and OS resources
 * won't be initialized.
 *
 * This should be called before \p osiTraceBufRequest.
 */
void osiTraceEarlyInit(void);

/**
 * \brief set trace data sender
 *
 * It should be called only once after RTOS is ready. It also means that
 * trace data sender can't be changed dynamically.
 *
 * Before \p osiTraceSenderInit is called, trace data will be kept in
 * memory. And when trace buffer is full, new trace data will be dropped.
 *
 * Two senders should be set, \p sender is for normal mode, and
 * \p bs_sender is for blue screen mode.
 *
 * When \p sender is NULL, trace data will be dropped directly. When
 * \p bs_sender is NULL, trace data will be dropped directly in blue
 * screeen mode.
 *
 * \param sender    trace data sender
 * \param bs_sender trace data sender in blue screen mode
 */
void osiTraceSenderInit(osiTraceSender_t sender, osiTraceSender_t bs_sender);

/**
 * \brief trace work queue
 *
 * In case that operations should be executed in trace thread, the trace
 * work queue can be get by this API, and then queue work to trace work
 * queue.
 *
 * \return      trace work queue
 */
osiWorkQueue_t *osiTraceWorkQueue(void);

/**
 * \brief request trace buffer
 *
 * To reduce extra copy for trace, trace function can request buffer from
 * trace driver, and write the buffer directly.
 *
 * At trace buffer full or not enough for the requested size, NULL will be
 * returned.
 *
 * The returned buffer is 4 bytes aligned.
 *
 * It is just tarce buffer management, others won't be handled inside. For
 * example, caller should maintain trace sequence number, track tick.
 *
 * \param [in] size     requested buffer size
 * \return
 *      - on buffer full, NULL will be returned
 *      - on success, return trace buffer
 */
uint32_t *osiTraceBufRequest(uint32_t size);

/**
 * \brief request trace buffer inside critical section
 *
 * It is similar to \p osiTraceBufRequest, just it is assumed that this is
 * called inside critical section.
 *
 * \param [in] size     requested buffer size
 * \return
 *      - on buffer full, NULL will be returned
 *      - on success, return trace buffer
 */
uint32_t *osiTraceBufRequestLocked(uint32_t size);

/**
 * \brief request trace buffer in tra packet format
 *
 * Request trace buffer, and manage trace sequence, fill packet header.
 * Caller should overwrite \p type field in header.
 *
 * The returned pointer is pointed to the start of tra packet header,
 * rather than the trace buffer header.
 *
 * \param [in] tra_len      tra packet length, including tra packet header
 * \return
 *      - on buffer full, NULL will be returned
 *      - on success, return tra packet buffer
 */
uint32_t *osiTraceTraBufRequest(uint32_t tra_len);

/**
 * \brief indicate trace buffer is filled
 *
 * After the requested trace buffer is filled, this shall be called to
 * let trace driver send the data to output.
 */
void osiTraceBufFilled(void);

/**
 * \brief trace enter blue screen mode
 *
 * When blue screen occurs during trace, it is possible that trace module
 * will be blocked. This will clean up incomplete status, and make sure
 * trace in blue screen can be ouput.
 */
void osiTraceBlueScreenEnter(void);

/**
 * \brief trace polling on blue screen mode
 *
 * In blue screen mode, there are no thread scheduling and timer. Trace
 * output should work in polling mode.
 */
void osiTraceBlueScreenPoll(void);

/**
 * \brief wait trace output finish
 *
 * It may wait long time for trace output finish. And in rare cases, it may
 * never return.
 *
 * This is for *dirty* debug only, only when some issue is debugging with
 * huge amount trace, and all the traces are needed.
 */
void osiTraceWaitFinish(void);

OSI_EXTERN_C_END
#endif
