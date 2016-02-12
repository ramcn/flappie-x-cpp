
#pragma once
#define DMA_CONFIG_SIZE          0x00000020
#define DMA_MSG_CONFIG_SIZE      0x00000040
#define DMA_MMOFFSET             0x00000000
#define DMA_MSG_MMOFFSET         0x00000000
#define DESC_MMOFFSET            0x00000020

typedef union {
    struct {
        uint32_t Done                 : 1;  /* Read/Clear; DMA transaction is complete; 
                                               The DONE bit is set to 1 when an end of
                                               packet condition is detected or the specified transaction length is
                                               completed. Write zero to the status register to clear the DONE bit */
        uint32_t Busy                 : 1;  // RO; The BUSY bit is 1 when a DMA transaction is in progress
        uint32_t REOP                 : 1;  /* RO; The REOP bit is 1 when a transaction is completed due to an 
                                               end-ofpacket event on the read side. */
        uint32_t WEOP                 : 1;  /* RO; The WEOP bit is 1 when a transaction is completed due to an 
                                               end-ofpacket event on the write side. */
        uint32_t Len                  : 1;  // The LEN bit is set to 1 when the length register decrements to zero. 
        uint32_t Reserved             : 29; 
    } ;
    uint32_t val;
} DMA_STATUS_T;

typedef union {
    struct {
        uint32_t Address      : 32; /* RW; The readaddress register specifies the first location to be read in a 
                                       DMA transaction. The readaddress register width is determined at system 
                                       generation time. It is wide enough to address the full range of all
                                       slave ports mastered by the read port.*/
    };
    uint32_t val;
} DMA_READ_ADDRESS_T;

typedef union {
    struct {
        uint32_t Address      : 32; /* RW; The writeaddress register specifies the first location to be written in a 
                                       DMA transaction. The writeaddress register width is determined at system 
                                       generation time. It is wide enough to address the full range of all
                                       slave ports mastered by the write port.*/
    };
    uint32_t val;
} DMA_WRITE_ADDRESS_T;

typedef union {
    struct {
        uint32_t Length      : 32; /* RW; The length register specifies the number of bytes to be transferred from 
                                      the read port to the write port. The length register is specified in bytes. 
                                      For example, the value must be a multiple of 4 for word transfers, and a 
                                      multiple of 2 for halfword transfers.  
                                      The length register is decremented as each data value is written by the 
                                      write master port. When length reaches 0 the LEN bit is set. The length 
                                      register does not decrement below 0. 
                                      The length register width is determined at system generation time. It is at 
                                      least wide enough to span any of the slave ports mastered by the read or write 
                                      master ports, and it can be made wider if necessary */
    } ;
    uint32_t val;
} DMA_LENGTH_T;

typedef union {
    struct {
        uint32_t Byte                 : 1;  // RW; Specifies byte transfers
        uint32_t HW                   : 1;  // RW; Specifies halfword (16-bit) transfers
        uint32_t Word                 : 1;  // RW; Specifies word (32-bit) transfers
        uint32_t Go                   : 1;  /* RW; Enables DMA transaction. When the GO bit is set to 0 during idle
                                               stage (before execution starts), the DMA is prevented from 
                                               executing transfers. When the GO bit is set to 1 during idle 
                                               stage and the length register is non-zero, transfers occur. 
						   
                                               If go bit is de-asserted low before write transaction complete, 
                                               done bit will never go high. It is advisable that GO bit is 
                                               modified during idle stage (no execution happened) only. */
        uint32_t I_En                 : 1;  /* RW; Enables interrupt requests (IRQ). When the I_EN bit is 1, the DMA
                                               controller generates an IRQ when the status register's DONE bit 
                                               is set to 1. IRQs are disabled when the I_EN bit is 0. */
        uint32_t ReEn                 : 1; /* RW; Ends transaction on read-side end-of-packet. When the REEN bit is
	                                          set to 1, a slave port with flow control on the read side may end 
                                              the DMA transaction by asserting its end-of-packet signal. */
        uint32_t WrEn                 : 1; /* RW; Ends transaction on write-side end-of-packet. WEEN bit should be 
	                                          set to 0. */
        uint32_t LenEn                : 1; /* RW; Ends transaction when the length register reaches zero. When this
                                              bit is 0, length reaching 0 does not cause a transaction to end. In 
                                              this case, the DMA transaction must be terminated by an end-ofpacket 
                                              signal from either the read or write master port. */
        uint32_t RCon                 : 1; /* RW; Reads from a constant address. When RCON is 0, the read address 
	                                          increments after every data transfer. This is the mechanism for the 
                                              DMA controller to read a range of memory addresses. When RCON is 
                                              1, the read address does not increment. This is the mechanism for 
                                              the DMA controller to read from a peripheral at a constant memory 
                                              address. For details, see the Addressing and Address Incrementing 
                                              section. */
        uint32_t WCon                 : 1; /* RW; Writes to a constant address. Similar to the RCON bit, when WCON 
	                                          is 0 the write address increments after every data transfer; 
                                              when WCON is 1 the write address does not increment */
        uint32_t DoubleWord           : 1;  // RW; Specifies doubleword (64-bit) transfers
        uint32_t QuadWord             : 1;  // RW; Specifies quadword (128-bit) transfers
        uint32_t SoftwareReset        : 1;  /* RW; Software can reset the DMA engine by writing this bit to 1 twice. 
	                                           Upon the second write of 1 to the SOFTWARERESET bit, the DMA control
                                               is reset identically to a system reset. The logic which sequences 
                                               the software reset process then resets itself automatically. */
        uint32_t Reserved             : 19; 
    } ;
    uint32_t val;
} DMA_CONTROL_T;

typedef union {
    struct {
        uint32_t   SeqNum         :16;  /* The sequence number field is available only when using extended descriptors.The sequence number is an
                                           arbitrary value that you assign to a descriptor, so that you can determine which descriptor is being
                                           operated on by the read and write masters. When performing memory-mapped to memory-mapped
                                           transfers, this value is tracked independently for masters since each can be operating on a different
                                           descriptor. To use this functionality, program the descriptors with unique sequence numbers. Then, access
                                           the dispatcher CSR slave port to determine which descriptor is operated on. */
        uint32_t   ReadBurst      : 8;  /* The programmable read and write burst counts are only available when using the extended descriptor
                                           format. The programmable burst count is optional and can be disabled in the read and write masters.
                                           Because the programmable burst count is an eight bit field for each master, the maximum that you can
                                           program burst counts of 1 to 128. Programming a value of zero or anything larger than 128 beats will be
                                           converted to the maximum burst count specified for each master automatically.
                                           The maximum programmable burst count is 128 but when you instantiate the DMA, you can have
                                           different selections up to 1024. Refer to the MAX_BURST_COUNT parameter in the parameter table. You
                                           will still have a burst count of 128 if you program for greater than 128. Programing to 0, gets the
                                           maximum burst count selected during instantiation time.*/

        uint32_t   WriteBurst     : 8;
    } ;
    uint32_t val;
} DMA_BURST_N_ID_T;

typedef union {
    struct {
        uint32_t   ReadStride     : 16; /* The read and write stride fields are optional and only available when using the extended descriptor format.
                                           The stride value determines how the read and write masters increment the address when accessing
                                           memory. The stride value is in terms of words, so the address incrementing is dependent on the master
                                           data width.
                                           When stride is enabled, the master defaults to sequential accesses, which is the equivalent to a stride
                                           distance of one. A stride of zero instructs the master to continuously access the same address. A stride of
                                           two instructs the master to skip every other word in a sequential transfer. You can use this feature to
                                           perform interleaved data accesses, or to perform a frame buffer row and column transpose. The read and
                                           write stride distances are stored independently allowing, you to use different address incrementing for
                                           read and write accesses in memory-to-memory transfers. For example, to perform a 2:1 data decimation
                                           transfer, you would simply configure the read master for a stride distance of two and the write master for a
                                           stride distance of one. To complete the decimation operation you could also insert a filter between the two
                                           masters */
        uint32_t   WriteStride    : 16;
    } ;
    uint32_t val;
} DMA_STRIDE_T;



/*
30:25 <reserved>

 */

typedef union {
    struct {
        uint32_t TransmitChannel   : 8;  /* Emits a channel number during Avalon-MM to Avalon-ST
                                            transfers */
        uint32_t GenerateSOP       : 1;  /* Emits a start of packet on the first beat of a Avalon-MM to
                                           Avalon-ST transfer */
        uint32_t GenerateEOP       : 1;  /* Emits an end of packet on last beat of a Avalon-MM to
                                            Avlaon-ST transfer */ 
        uint32_t ParkReads         : 1;  /* When set, the dispatcher continues to reissue the same
                                            descriptor to the read master when no other descriptors are
                                            buffered. This is commonly used for video frame buffering. */
        uint32_t ParkWrites        : 1;  /* When set, the dispatcher continues to reissue the same
                                            descriptor to the write master when no other descriptors are 
                                            buffered. */
        uint32_t EndOnEOP          : 1;  /* End on end of packet allows the write master to continuously
                                            transfer data during Avalon-ST to Avalon-MM transfers
                                            without knowing how much data is arriving ahead of time.
                                            This bit is commonly set for packet-based traffic such as
                                            Ethernet. */

        uint32_t Resvd0              :1;
        uint32_t TxCompleteIRQEnable :1; /* Signals an interrupt to the host when a transfer completes.
                                            In the case of Avalon-MM to Avalon-ST transfers, this
                                            interrupt is based on the read master completing a transfer.
                                            In the case of Avalon-ST to Avalon-MM or Avalon-MM to
                                            Avalon-MM transfers, this interrupt is based on the write
                                            master completing a transfer. */
        uint32_t EarlyTermIRQEnable : 1; /* Signals an interrupt to the host when a Avalon-ST to AvalonMM
                                            transfer completes early.
                                            For example, if you set this bit and set the length field to 1MB
                                            for Avalon-ST to Avalon-MM transfers, this interrupt asserts
                                            when more than 1MB of data arrives to the write master
                                            without the end of packet being seen. */
        uint32_t TransmitError      : 8; /* For for Avalon-MM to Avalon-ST transfers, this field is used to
                                            specify a transmit error.
                                            This field is commonly used for transmitting error information
                                            downstream to streaming components, such as an Ethernet
                                            MAC.
                                            In this mode, these control bits control the error bits on the
                                            streaming output of the read master.
                                            For Avalon-ST to Avalon-MM transfers, this field is used as an
                                            error interrupt mask.
                                            As errors arrive at the write master streaming sink port, they
                                            are held persistently. When the transfer completes, if any error
                                            bits were set at any time during the transfer and the error
                                            interrupt mask bits are set, then the host receives an interrupt.
                                            In this mode, these control bits are used as an error
                                            encountered interrupt enable. */
        uint32_t EarlyDoneEnable    : 1; /* Hides the latency between read descriptors.
                                            When the read master is set, it does not wait for pending reads
                                            to return before requesting another descriptor.
                                            Typically this bit is set for all descriptors except the last one.
                                            This bit is most effective for hiding high read latency. For
                                            example, it reads from SDRAM, PCIe, and SRIO. */
        uint32_t Resvd1             : 6;
        uint32_t Go                 : 1; /* Commits all the descriptor information into the descriptor
                                            FIFO.
                                            As the host writes different fields in the descriptor, FIFO byte
                                            enables are asserted to transfer the write data to appropriate
                                            byte locations in the FIFO.
                                            However, the data written is not committed until the go bit has
                                            been written.
                                            As a result, ensure that the go bit is the last bit written for each
                                            descriptor.
                                            Writing '1' to the go bit commits the entire descriptor into the
                                            descriptor FIFO(s). */
    } ;
    uint32_t val;
} DMA_DESC_CONTROL_T;

typedef union {
    struct {
        uint32_t Busy         : 1; /* Set when the dispatcher still has commands buffered, or one of the masters is
                                      still transferring data. */
        uint32_t DescBufEmpty : 1; /* Set when both the read and write command buffers are empty. */
        uint32_t DescBufFull  : 1; /* Set when either the read or write command buffers are full. */
        uint32_t RespBufEmpty : 1; /* Set when the response buffer is empty. */
        uint32_t RespBufFull  : 1; /* Set when the response buffer is full. */
        uint32_t Stopped      : 1; /* Set when you either manually stop the SGDMA, or you setup the dispatcher to
                                      stop on errors or early termination and one of those conditions occurred. If you
                                      manually stop the SGDMA this bit is asserted after the master completes any
                                      read or write operations that were already in progress. */
        uint32_t Resetting   : 1;  /* Set when you write to the software reset register and the SGDMA is in the
                                      middle of a reset cycle. This reset cycle is necessary to make sure there are no
                                      incoming transfers on the fabric. When this bit de-asserts you may start using
                                      the SGDMA again. */
        uint32_t StoppedOnError :1;/* When the dispatcher is programmed to stop errors and when an error beat
                                      enters the write master the bit is set. */
        uint32_t StoppedOnEarlyTerm :1; /* When the dispatcher is programmed to stop on early termination, this bit is set.
                                           Also set, when the write master is performing a packet transfer and does not
                                           receive EOP before the pre-determined amount of bytes are transferred, which
                                           is set in the descriptor length field. If you do not wish to use early termination
                                           you should set the transfer length of the descriptor to 0xFFFFFFFF ,which gives
                                           you the maximum packet based transfer possible (early termination is always
                                           enabled for packet transfers). */ 
        uint32_t IRQ           : 1; /* Set when an interrupt condition occurs. */
        uint32_t Resvd         :12; 
    };
    uint32_t val;
} DMA_MSG_STATUS_T;

typedef union {
    struct {
        uint32_t StopDispatcher     : 1; /* Setting this bit stops the SGDMA in the middle of a transaction. If a read or
                                            write operation is occurring, then the access is allowed to complete. Read the
                                            stopped status register to determine when the SGDMA has stopped. After reset,
                                            the dispatcher core defaults to a start mode of operation. */
        uint32_t ResetDispatcher    : 1; /* Setting this bit resets the registers and FIFOs of the dispatcher and master
                                            modules. Since resets can take multiple clock cycles to complete due to
                                            transfers being in flight on the fabric, you should read the resetting status
                                            register to determine when a full reset cycle has completed. */
        uint32_t StopOnError        : 1; /* Setting this bit stops the SGDMA from issuing more read/write commands to
                                            the master modules if an error enters the write master module sink port. */
        uint32_t StopOnEarlyTerm    : 1; /* Setting this bit stops the SGDMA from issuing more read/write commands to
                                            the master modules if the write master attempts to write more data than the
                                            user specifies in the length field for packet transactions. The length field is used
                                            to limit how much data can be sent and is always enabled for packet based 
                                            writes. */
        uint32_t GlobalIntrEnMask   : 1; /* Setting this bit allows interrupts to propagate to the interrupt sender port. This
                                            mask occurs after the register logic so that interrupts are not missed when the
                                            mask is disabled. */
        uint32_t StopDesc           : 1; /* Setting this bit stops the SGDMA dispatcher from issuing more descriptors to
                                            the read or write masters. Read the stopped status register to determine when
                                            the dispatcher stopped issuing commands and the read and write masters are
                                            idle. */
        uint32_t Resvd              :26;
    } ;
    uint32_t val;
} DMA_MSG_CONTROL_T;

typedef union {
    struct {
        uint32_t ReadSeqNum    : 16;
        uint32_t WriteSeqNum   : 16;
    } ;
    uint32_t val;
} DMA_MSG_SEQ_STATUS_T;

typedef struct DMA_t {
    DMA_STATUS_T             DMA_STATUS;
    DMA_READ_ADDRESS_T       DMA_READ_ADDRESS;
    DMA_WRITE_ADDRESS_T      DMA_WRITE_ADDRESS;
    DMA_LENGTH_T             DMA_LENGTH; // in bytes
    DMA_CONTROL_T            DMA_CONTROL;
} DMA_t;

typedef struct DMA_EXT_DESC_t {
    DMA_READ_ADDRESS_T      READ_ADDRESS;
    DMA_WRITE_ADDRESS_T     WRITE_ADDRESS;
    DMA_LENGTH_T            LENGTH; // in bytes
    DMA_BURST_N_ID_T        BURST_N_ID;
    DMA_STRIDE_T            STRIDE;
    DMA_READ_ADDRESS_T      READ_ADDRESS_HI;
    DMA_WRITE_ADDRESS_T     WRITE_ADDRESS_HI;
    DMA_DESC_CONTROL_T      CONTROL;
} DMA_EXT_DESC_t;

typedef struct DMA_MSG_t {
    DMA_MSG_STATUS_T        DMA_STATUS;
    DMA_MSG_CONTROL_T       DMA_CONTROL;
    DMA_MSG_SEQ_STATUS_T    DMA_SEQ_STATUS;
} DMA_MSG_t;

#define DMA_STATUS_OFFSET         (DMA_MMOFFSET + 0x00)
#define DMA_READ_ADDRESS_OFFSET   (DMA_MMOFFSET + 0x04)
#define DMA_WRITE_ADDRESS_OFFSET  (DMA_MMOFFSET + 0x08)
#define DMA_LENGTH_OFFSET         (DMA_MMOFFSET + 0x0C)
#define DMA_RESERVED_1_OFFSET     (DMA_MMOFFSET + 0x10)
#define DMA_RESERVED_2_OFFSET     (DMA_MMOFFSET + 0x14)
#define DMA_CONTROL_OFFSET        (DMA_MMOFFSET + 0x18)

#define DMA_DESC_READ_ADDRESS_OFFSET	   (DESC_MMOFFSET + 0x00)
#define DMA_DESC_WRITE_ADDRESS_OFFSET	   (DESC_MMOFFSET + 0x04)
#define DMA_DESC_LENGTH_OFFSET	           (DESC_MMOFFSET + 0x08)
#define DMA_DESC_BURST_N_ID_OFFSET	       (DESC_MMOFFSET + 0x0c)
#define DMA_DESC_STRIDE_OFFSET	           (DESC_MMOFFSET + 0x10)
#define DMA_DESC_READ_ADDRESS_HI_OFFSET	   (DESC_MMOFFSET + 0x14)
#define DMA_DESC_WRITE_ADDRESS_HI_OFFSET   (DESC_MMOFFSET + 0x18)
#define DMA_DESC_CONTROL_OFFSET	           (DESC_MMOFFSET + 0x1c)

#define DMA_MSG_STATUS_OFFSET      (DMA_MSG_MMOFFSET + 0x00)
#define DMA_MSG_CONTROL_OFFSET     (DMA_MSG_MMOFFSET + 0x04)
#define DMA_MSG_SEQ_STATUS_OFFSET  (DMA_MSG_MMOFFSET + 0x10)

