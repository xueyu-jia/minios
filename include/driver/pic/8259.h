#pragma once

#include <stdbool.h>
#include <stdint.h>

//! \see https://helppc.netcore2k.net/hardware/8259
//! ATTENTION: ocw4 isr & irr bit in the document above is incorrect

#define PIC_M_PORT 0x20
#define PIC_S_PORT 0xa0

#define PIC_CMD 0x00
#define PIC_DATA 0x01

//! initialization command word
#define PIC_CMD_ICW1_ICW4 0x01             //<! require icw4
#define PIC_CMD_ICW1_SINGLE 0x02           //<! single pic, otherwise cascade
#define PIC_CMD_ICW1_DWORD_INTVAL 0x04     //<! dword int vectors, otherwise qword
#define PIC_CMD_ICW1_LEVEL_TRIGGER 0x08    //<! level trigger, otherwise edge trigger
#define PIC_CMD_ICW1_RESERVED 0x10         //<! reserved
#define PIC_DATA_ICW4_8086 0x01            //<! 80x86 mode, otherwise mcs 80/85 mode
#define PIC_DATA_ICW4_AUTO_EOI 0x02        //<! auto eoi, otherwise normal eoi
#define PIC_DATA_ICW4_BUFFERED_SLAVE 0x08  //<! buffered mode slave
#define PIC_DATA_ICW4_BUFFERED_MASTER 0x0c //<! buffered mode master
#define PIC_DATA_ICW4_SFNM 0x10            //<! special fully nested mode, otherwise sequential

//! operation control word
#define PIC_CMD_OCW2_REQ_LEVEL(n) ((n) & 0x3)
#define PIC_CMD_OCW2_EOI(type) (((type) & 0x3) << 5)
#define PIC_CMD_OCW2_EOI_NON_SPECIFIC PIC_CMD_OCW2_EOI(0x01)
#define PIC_CMD_OCW2_EOI_NOP PIC_CMD_OCW2_EOI(0x02)
#define PIC_CMD_OCW2_EOI_SPECIFIC PIC_CMD_OCW2_EOI(0x03)
#define PIC_CMD_OCW2_EOI_ROTATE_AUTO PIC_CMD_OCW2_EOI(0x04)
#define PIC_CMD_OCW2_EOI_ROTATE_NON_SPECIFIC PIC_CMD_OCW2_EOI(0x05)
#define PIC_CMD_OCW2_EOI_SET_PRIORITY PIC_CMD_OCW2_EOI(0x06)
#define PIC_CMD_OCW2_EOI_ROTATE_SPECIFIC PIC_CMD_OCW2_EOI(0x07)
#define PIC_CMD_OCW3_READ_IRR 0x00
#define PIC_CMD_OCW3_READ_ISR 0x01
#define PIC_CMD_OCW3_ACT_IF_BIT0 0x02
#define PIC_CMD_OCW3_POLL_CMD_ISSUED 0x04
#define PIC_CMD_OCW3_RESERVED 0x08
#define PIC_CMD_OCW3_SET_SPECIAL_MASK 0x20
#define PIC_CMD_OCW3_RESET_SPECIAL_MASK 0x00
#define PIC_CMD_OCW3_ACT_IF_BIT5 0x40

#define PIC_CMD_EOI PIC_CMD_OCW2_EOI_NON_SPECIFIC
#define PIC_CMD_READ_IRR (PIC_CMD_OCW3_READ_IRR | PIC_CMD_OCW3_ACT_IF_BIT0 | PIC_CMD_OCW3_RESERVED)
#define PIC_CMD_READ_ISR (PIC_CMD_OCW3_READ_ISR | PIC_CMD_OCW3_ACT_IF_BIT0 | PIC_CMD_OCW3_RESERVED)

/*!
 * \brief init 8259 pic with remapped irq int vectors
 *
 * \param [in] irq0_off offset of irq 0 in the int vectors table
 * \param [in] irq8_off offset of irq 8 in the int vectors table
 *
 * \return 0 if successed
 */
int pic_8259_init(int irq0_off, int irq8_off);

/*!
 * \brief mask all irq
 */
void pic_disable();

/*!
 * \brief set or clear mask of the specific irq
 */
void pic_set_irq_mask(int irq, bool on);

/*!
 * \brief check mask on target irq
 *
 * \return true if masked
 */
bool pic_is_irq_masked(int irq);

/*!
 * \brief send the non-specific eoi to the target pic
 */
void pic_send_eoi(int irq);

/*!
 * \brief get master:slave in-request irq
 */
uint16_t pic_get_irr();

/*!
 * \brief get master:slave in-service irq
 */
uint16_t pic_get_isr();
