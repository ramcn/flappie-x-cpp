/**********************************************************************************************
   INTEL TOP SECRET
   Copyright 2016 Intel Corporation All Rights Reserved.

   The source code contained or described herein and all documents related to the source code
   ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the
   Material remains with Intel Corporation or its suppliers and licensors. The Material may
   contain trade secrets and proprietary and confidential information of Intel Corporation
   and its suppliers and licensors, and is protected by worldwide copyright and trade secret
   laws and treaty provisions. No part of the Material may be used, copied, reproduced,
   modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way
   without Intel's prior express written permission.
   No license under any patent, copyright, trade secret or other intellectual property right
   is granted to or conferred upon you by disclosure or delivery of the Materials, either
   expressly, by implication, inducement, estoppel or otherwise. Any license under such
   intellectual property rights must be express and approved by Intel in writing.
**********************************************************************************************/

/*
 * VIO HW Accelerators
 * vio_regs.h
 *
 *  Created on: Sep 20, 2016
 *      Author: Dipan K Mondal / Om J Omer
 */

#pragma once

//#define VIO_CONFIG_BASE_MMOFFSET 0x10000000
#define VIO_CONFIG_SIZE          0x00008000
#define ISRC_CONFIG_SIZE         0x00001000

#define VIO_TOP_MMOFFSET         0x00000000
#define VIO_CB1_MMOFFSET         0x00001000
#define VIO_CB2_MMOFFSET         0x00002000
#define VIO_DET1_MMOFFSET        0x00003000
#define VIO_DET2_MMOFFSET        0x00004000
#define VIO_TRK_MMOFFSET         0x00005000
#define VIO_EKF_MMOFFSET         0x00006000
#define VIO_ISRC_MMOFFSET        0x00000000

#define VIO_TOP_SIZE             (VIO_CB1_MMOFFSET  - VIO_TOP_MMOFFSET)
#define VIO_CB1_SIZE             (VIO_CB2_MMOFFSET  - VIO_CB1_MMOFFSET)
#define VIO_CB2_SIZE             (VIO_DET1_MMOFFSET - VIO_CB2_MMOFFSET)
#define VIO_DET1_SIZE            (VIO_DET2_MMOFFSET - VIO_DET1_MMOFFSET)
#define VIO_DET2_SIZE            (VIO_TRK_MMOFFSET  - VIO_DET2_MMOFFSET)
#define VIO_TRK_SIZE             (VIO_EKF_MMOFFSET  - VIO_TRK_MMOFFSET)

typedef union {
    struct {
        uint32_t ConfigWrError : 1; /* RO 0x0 This field is set to '1' when a config MMR write
                                       via Host Configuration Interface causes an error scenario
                                       and an error response is sent out. This is a sticky bit,
                                       once set, it remains set until SW writes a ‘1’ to
                                       VIO_CONTROL::ClearConfigWrError to clear it (note, writing
                                       a ‘0’ has no effect). */
        uint32_t resvd         : 31; /* Reserved SBZ 0x0 Reserved (RAZ, SBZ) */
    } field;
    uint32_t val;
} VIO_TOP_STATUS_T;

typedef union {
    struct {
        uint32_t GlobalIntrDisable   : 1;  /* RW 0x0 Global Interrupt disable. SW may set this field to ‘1’to disable
                                             all interrupts output from VIO HWA. Writing '0' enables all interrupts again. 
                                             Note: this field has higher priority than individual interrupt mask fields. */
        uint32_t ClearConfigWrError  : 1;  /* RW 0x0 Writing '1' to this field clears the config write error flag
                                             (VIO_STATUS::ConfigWrError) Writing '0' clears this bit but has no other effect. */
        uint32_t ForceModuleCLKEn    : 1;  /* RW 0x0 Setting this field to ‘1’ forces modules level clock to be always 
					     enabled (disable module level clock gating).Setting this field to ‘0’ allows 
					     module level clock gating to be enabled when modules reach a non-active state.*/
        uint32_t InterruptPolarity   : 1;  /* RW 0x0 0: ActiveHigh, 1: ActiveLow */
        uint32_t InterruptMode       : 1;  /* RW 0x0 0: Level interrupt, 1: Pulse Interrupt*/
        uint32_t InterruptPulseWidth : 4;  /* RW 0x0 Width of interrupt pulse, (0 based)
					      0x0 -> 1 clock wide pulse
					      ….
					      0xF -> 16 clock wide pulse
					   */
	 
        uint32_t Reserved            : 23; /* SBZ 0x0 Reserved (RAZ, SBZ) */
    } field;
    uint32_t val;
} VIO_TOP_CONTROL_T;

typedef union {
    struct {
        uint32_t TrackerDoneIntrFlag   : 1;/* RW 0x0 This field is set to ‘1’ when the last initiated feature tracking 
					      operation completes (TracekrDoneIntr). This field can also be set by writing 
					      a ‘1’ to VIO_INTR_SET::SetTrackerDoneIntr field. This field is cleared by writing 
					      ‘1’ to VIO_INTR_CLR::ClrTrackerDoneIntr field.*/
        uint32_t Detector1DoneIntrFlag : 1;/* RW 0x0 This field is set to ‘1’ when the last initiated feature detection 
					      operation completes (Detector1DoneIntr). This field can also be set by writing a ‘1’ 
					      to VIO_INTR_SET::SetDetectorDoneIntr field. This field is cleared by writing ‘1’ to 
					      VIO_INTR_CLR::ClrDetectorDoneIntr field.*/
        uint32_t Detector2DoneIntrFlag : 1;/* RW 0x0 This field is set to ‘1’ when the last initiated feature detection 
					      operation completes (Detector2DoneIntr). This field can also be set by writing a ‘1’ 
					      to VIO_INTR_SET::SetDetectorDoneIntr field. This field is cleared by writing ‘1’ to 
					      VIO_INTR_CLR::ClrDetectorDoneIntr field.*/
        uint32_t Sorter1DoneIntrFlag   : 1;/* RW 0x0 This field is set to ‘1’ when the last initiated feature sorting operation 
					      completes (Sorter1DoneIntr). This field can also be set by writing a ‘1’ to 
					      VIO_INTR_SET::SetSorterDoneIntr field. This field is cleared by writing ‘1’ to 
					      VIO_INTR_CLR::ClrSorterDoneIntr field.*/
        uint32_t Sorter2DoneIntrFlag   : 1;/* RW 0x0 This field is set to ‘1’ when the last initiated feature sorting operation 
					      completes (Sorter2DoneIntr). This field can also be set by writing a ‘1’ to 
					      VIO_INTR_SET::SetSorterDoneIntr field. This field is cleared by writing ‘1’ to 
					      VIO_INTR_CLR::ClrSorterDoneIntr field.*/
        uint32_t MatMulDoneIntrFlag    : 1;/* RW 0x0 This field is set to ‘1’ when write corresponding to last element of matrix 
					      is complete (MatMulDoneIntr). This field can also be set by writing a ‘1’ to 
					      VIO_INTR_SET::SetMatMulDoneIntr field.This field is cleared by writing ‘1’ to 
					      VIO_INTR_CLR::ClrMatMulDoneIntr field.*/
        uint32_t CholeskyDoneIntrFlag  : 1;/* RW 0x0 This field is set to ‘1’ when write corresponding to last element of matrix is 
					      complete (CholeskyDoneIntr). This field can also be set by writing a ‘1’ to 
					      VIO_INTR_SET::SetCholeskyDoneIntr field. This field is cleared by writing ‘1’ to 
					      VIO_INTR_CLR::ClrCholeskyDoneIntr field.*/
        uint32_t MatSolveDoneIntrFlag  : 1;/* RW 0x0 This field is set to ‘1’ when write corresponding to last element of matrix is 
					      complete (MatSolveDoneIntr). This field can also be set by writing a ‘1’ to 
					      VIO_INTR_SET::SetMatSolveDoneIntr field. This field is cleared by writing ‘1’ to 
					      VIO_INTR_CLR::ClrMatSolveDoneIntr field. */
        uint32_t EKFStatusIntrFlag     : 1; /* RW 0x0 This field is set to ‘1’ when any of the unmasked Status flag for EKF 
					       Floating point Design Wares is set. This field can also be set by writing a ‘1’ to 
					       VIO_INTR_SET::SetEKFStatusFlag field. This field is cleared by writing ‘1’ to 
					       VIO_INTR_CLR::ClrEKFStatusFlag field.*/
        uint32_t Reserved              : 23;/* RW 0x0 Reserved (RAZ, SBZ)*/
    } field; 
    uint32_t val;
} VIO_INTERRUPT_FLAG_T;

typedef union {
    struct {
        uint32_t TrackerDoneIntrMask    : 1; /* RW 0x0 Interrupt mask field for TracekrDoneIntr. Setting this field to ‘1’ disables the 
						corresponding interrupt, clearing it to ‘0’ enables it.*/
        uint32_t Detector1DoneIntrMask  : 1; /* RW 0x0 Interrupt mask field for Detector1DoneIntr. Setting this field to ‘1’ disables the 
						corresponding interrupt, clearing it to ‘0’ enables it.*/
        uint32_t Detector2DoneIntrMask  : 1; /* RW 0x0 Interrupt mask field for Detector2DoneIntr. Setting this field to ‘1’ disables the 
						corresponding interrupt, clearing it to ‘0’ enables it.*/
        uint32_t Sorter1DoneIntrMask    : 1; /* RW 0x0 Interrupt mask field for Sorter1DoneIntr. Setting this field to ‘1’ disables the 
						corresponding interrupt, clearing it to ‘0’ enables it.*/
        uint32_t Sorter2DoneIntrMask    : 1; /* RW 0x0 Interrupt mask field for Sorter2DoneIntr. Setting this field to ‘1’ disables the 
						corresponding interrupt, clearing it to ‘0’ enables it.*/
        uint32_t MatMulDoneIntrMask     : 1; /* RW 0x0Interrupt mask field for MatMulDoneIntr.
						Setting this field to ‘1’ disables the corresponding interrupt, clearing it to ‘0’ enables it.*/
        uint32_t CholeskyDoneIntrMask   : 1; /* RW 0x0 Interrupt mask field for CholeskyDoneIntr.
						Setting this field to ‘1’ disables the corresponding interrupt, clearing it to ‘0’ enables it.*/
        uint32_t MatSolveDoneIntrMask   : 1; /* RW 0x0 Interrupt mask field for MatSolveDoneIntr.
						Setting this field to ‘1’ disables the corresponding interrupt, clearing it to ‘0’ enables it.*/
        uint32_t EKFStatusIntrMask      : 1; /* RW 0x0 Interrupt mask field for EKFStatusIntr.
						Setting this field to ‘1’ disables the corresponding interrupt, clearing it to ‘0’ enables it.*/
        uint32_t Reserved               : 23;/* RW 0x0 Reserved (RAZ, SBZ)*/ 
    } field;
    uint32_t val;
} VIO_INTERRUPT_MASK_T;

typedef union {
    struct {
        uint32_t SetTrackerDoneIntr     : 1 ; /* WO RAZ 0x0 Interrupt flag set field for TracekrDoneIntr.
						 Writing ‘1’ to this field sets the corresponding interrupt flag. Writing ‘0’ has no effect.*/
        uint32_t SetDetector1DoneIntr   : 1 ; /* WO, RAZ 0x0 Interrupt flag set field for Detector1DoneIntr.
						 Writing ‘1’ to this field sets the corresponding interrupt flag. Writing ‘0’ has no effect.*/
        uint32_t SetDetector2DoneIntr   : 1 ; /* WO, RAZ 0x0 Interrupt flag set field for Detector2DoneIntr.
						 Writing ‘1’ to this field sets the corresponding interrupt flag. Writing ‘0’ has no effect.*/
        uint32_t SetSorter1DoneIntr     : 1 ; /* WO, RAZ 0x0 Interrupt flag set field for Sorter1DoneIntr.
						 Writing ‘1’ to this field sets the corresponding interrupt flag. Writing ‘0’ has no effect.*/
        uint32_t SetSorter2DoneIntr     : 1 ; /* WO, RAZ 0x0 Interrupt flag set field for Sorter2DoneIntr.
						 Writing ‘1’ to this field sets the corresponding interrupt flag. Writing ‘0’ has no effect.*/
        uint32_t SetMatMulDoneIntr      : 1 ; /* WO, RAZ 0x0 Interrupt flag set field for MatMulDoneIntr.
						 Writing ‘1’ to this field sets the corresponding interrupt flag. Writing ‘0’ has no effect.*/
        uint32_t SetCholeskyDoneIntr    : 1 ; /* WO, RAZ 0x0 Interrupt flag set field for CholeskyDoneIntr.
						 Writing ‘1’ to this field sets the corresponding interrupt flag. Writing ‘0’ has no effect.*/
        uint32_t SetMatSolveDoneIntr    : 1 ; /* WO, RAZ 0x0 Interrupt flag set field for MatSolveDoneIntr.
						 Writing ‘1’ to this field sets the corresponding interrupt flag. Writing ‘0’ has no effect.*/
        uint32_t SetEKFStatusIntr       : 1 ; /* WO, RAZ 0x0 Interrupt flag set field for EKFStatusIntr.
						 Writing ‘1’ to this field sets the corresponding interrupt flag. Writing ‘0’ has no effect.*/
        uint32_t Reserved               : 23 ; /* Reserved(RAZ, SBXZ);*/

    } field;
    uint32_t val;
} VIO_INTERRUPT_SET_T;

typedef union {
    struct {
        uint32_t ClrTrackerDoneIntr     : 1 ; /* WO RAZ 0x0 Interrupt flag clear field for TracekrDoneIntr.
						 Writing ‘1’ to this field clears the corresponding interrupt flag. Writing ‘0’ has no effect.*/
        uint32_t ClrDetector1DoneIntr   : 1 ; /* WO, RAZ 0x0 Interrupt flag clear field for Detector1DoneIntr.
						 Writing ‘1’ to this field clears the corresponding interrupt flag. Writing ‘0’ has no effect.*/
        uint32_t ClrDetector2DoneIntr   : 1 ; /* WO, RAZ 0x0 Interrupt flag clear field for Detector2DoneIntr.
						 Writing ‘1’ to this field clears the corresponding interrupt flag. Writing ‘0’ has no effect.*/
        uint32_t ClrSorter1DoneIntr     : 1 ; /* WO, RAZ 0x0 Interrupt flag clear field for Sorter1DoneIntr.
						 Writing ‘1’ to this field clears the corresponding interrupt flag. Writing ‘0’ has no effect.*/
        uint32_t ClrSorter2DoneIntr     : 1 ; /* WO, RAZ 0x0 Interrupt flag clear field for Sorter2DoneIntr.
						 Writing ‘1’ to this field clears the corresponding interrupt flag. Writing ‘0’ has no effect.*/
        uint32_t ClrMatMulDoneIntr      : 1 ; /* WO, RAZ 0x0 Interrupt flag clear field for MatMulDoneIntr.
						 Writing ‘1’ to this field clears the corresponding interrupt flag. Writing ‘0’ has no effect.*/
        uint32_t ClrCholeskyDoneIntr    : 1 ; /* WO, RAZ 0x0 Interrupt flag clear field for CholeskyDoneIntr.
						 Writing ‘1’ to this field clears the corresponding interrupt flag. Writing ‘0’ has no effect.*/
        uint32_t ClrMatSolveDoneIntr    : 1 ; /* WO, RAZ 0x0 Interrupt flag clear field for MatSolveDoneIntr.
						 Writing ‘1’ to this field clears the corresponding interrupt flag. Writing ‘0’ has no effect.*/
        uint32_t ClrEKFStatusIntr       : 1 ; /* WO, RAZ 0x0 Interrupt flag clear field for EKFStatusIntr.
						 Writing ‘1’ to this field clears the corresponding interrupt flag. Writing ‘0’ has no effect.*/
        uint32_t Reserved               : 23 ; /* Reserved(RAZ, SBXZ);*/
    } field;
    uint32_t val;
} VIO_INTERRUPT_CLEAR_T;


typedef union {
    struct {
        uint32_t CircularBuffer1Overflow: 1 ; /* RO 0x0  This field is set to '1' when circular buffer overflows.
						 This field auto clears when VIO_DET1_CONTROL::Sorting1Start field is written with '1' to 
						 start a fresh sorting process. */
        uint32_t Reserved               : 31 ; /* RO 0x0 Reserved (RAZ, SBZ)*/
    } field;
    uint32_t val;
} VIO_CB1_STATUS_T;

typedef union {
    struct {
        uint32_t ClearCirBuf1Overflow   : 1 ; /* RW 0x0 Writing ‘1’ to this field clears Circular Buffer 1 Overflow indication 
                                                 (VIO_CB1_STATUS::CirBuf1Overflow) Writing ‘0’ clears the bit but has no other effect.*/
        uint32_t Reserved0              : 7 ; /* 0x0 Reserved (RAZ, SBZ)*/
        uint32_t CircularBuffer1Depth   : 8 ; /* 8’b0: N=32 lines, 
                                                 8’b1: N=64 lines, 
                                                 Others, reserved*/
        uint32_t Reserved1              : 16 ; /* RO 0x0 Reserved (RAZ, SBZ)*/
    } field;
    uint32_t val;
} VIO_CB1_CONTROL_T;


typedef union {
    struct {
        uint32_t BaseAddress           : 18 ; /* RW 0x0 Base address of the Circular Image Buffer1 in SL2, 128b aligned word address.*/
        uint32_t Reserved              : 16 ; /* RO 0x0 Reserved (RAZ, SBZ)*/
    } field;
    uint32_t val;
} VIO_CB1_IMG_BASE_T;


typedef union {
    struct {
        uint32_t ImageWidth : 11; /*  10:00    RW  0x0 Width of the image in SL2,  must be multiple of 16 */
        uint32_t Reserved   : 21; /*  31:11    SBZ 0x0 Reserved (RAZ, SBZ)    */
    } field;
    uint32_t val;
} VIO_CB1_IMG_WIDTH_T;

typedef union {
    struct {
        uint32_t ImageHeight : 11; /*  10:00   RW  0x0 Height of the image in SL2,  must be >=64 */
        uint32_t Reserved : 21 ; /*  31:11     SBZ 0x0 Reserved (RAZ, SBZ)    */
    } field;
    uint32_t val;
} VIO_CB1_IMG_HEIGHT_T;

typedef union {
    struct {
        uint32_t ImageStride : 11; /*  10:00   RW  0x0 Row Stride of the image in SL2 */
        uint32_t Reserved : 21;    /*  31:11  SBZ 0x0 Reserved (RAZ, SBZ) */
    
    } field;
    uint32_t val;
} VIO_CB1_IMG_STRIDE_T;


typedef union {
    struct {
        uint32_t CircularBuffer2Overflow: 1 ; /* RO 0x0  This field is set to '1' when circular buffer overflows.
						 This field auto clears when VIO_DET2_CONTROL::Sorting1Start field is written with '1' to 
						 start a fresh sorting process. */
        uint32_t Reserved               : 31 ; /* RO 0x0 Reserved (RAZ, SBZ)*/
    } field;
    uint32_t val;
} VIO_CB2_STATUS_T;

typedef union {
    struct {
        uint32_t ClearCirBuf2Overflow   : 1 ; /* RW 0x0 Writing '1' to this field clears Circular Buffer 1 Overflow indication
						 (VIO_CB2_STATUS::CirBuf1Overflow) 
						 Writing '0' clears the bit but has no other effect.*/
        uint32_t Reserved0              : 7 ; /* 0x0 Reserved (RAZ, SBZ)*/
        uint32_t CircularBuffer2Depth   : 8 ; /* 8’b0: N=32 lines, 
						 8’b1: N=64 lines, 
						 Others, reserved*/
        uint32_t Reserved1              : 16 ; /* RO 0x0 Reserved (RAZ, SBZ)*/
    } field;
    uint32_t val;
} VIO_CB2_CONTROL_T;


typedef union {
    struct {
        uint32_t BaseAddress           : 18 ; /* RW 0x0 Base address of the Circular Image Buffer2 in SL2, 128b aligned word address.*/
        uint32_t Reserved              : 16 ; /* RO 0x0 Reserved (RAZ, SBZ)*/
    } field;
    uint32_t val;
} VIO_CB2_IMG_BASE_T;


typedef union {
    struct {
        uint32_t ImageWidth : 11; /*  10:00    RW  0x0 Width of the image in SL2,  must be multiple of 16 */
        uint32_t Reserved   : 21; /*  31:11    SBZ 0x0 Reserved (RAZ, SBZ)    */
    } field;
    uint32_t val;
} VIO_CB2_IMG_WIDTH_T;

typedef union {
    struct {
        uint32_t ImageHeight : 11; /*  10:00   RW  0x0 Height of the image in SL2,  must be >=64 */
        uint32_t Reserved : 21 ; /*  31:11     SBZ 0x0 Reserved (RAZ, SBZ)    */
    } field;
    uint32_t val;
} VIO_CB2_IMG_HEIGHT_T;

typedef union {
    struct {
        uint32_t ImageStride : 11; /*  10:00   RW  0x0 Row Stride of the image in SL2 */
        uint32_t Reserved : 21;    /*  31:11  SBZ 0x0 Reserved (RAZ, SBZ) */
    
    } field;
    uint32_t val;
} VIO_CB2_IMG_STRIDE_T;


typedef union {
    struct {
        uint32_t FeatureDetectorDone    : 1; /* RO 0x0 This field is set to '1' when the last initiated FASTn FeatureDetector operation completes.
                                                This field auto clears when VIO_DET1_CONTROL::FeatureDetectorStart field is written with '1' 
                                                to start a fresh FASTn feature detection process.*/
        uint32_t SorterDone             : 1; /* RO 0x0 This field is set to '1' when the last initiated Sorting (max K-selection) operation completes.
                                                This field auto clears when VIO_DET1_CONTROL::SortingStart field is written with '1' to start a fresh sorting process. */
        uint32_t CircularBufferOverflow : 1; /* RO 0x0 This field is set to '1' when detector circular buffer overflows.
                                                This field auto clears when VIO_DET1_CONTROL:: FeatureDetectorStart field is written with '1' 
                                                to start a fresh detection process.*/
        uint32_t MaxNumFeatureExceeded  : 1; /* RO 0x0 This field is set to '1' when detector exceeds detecting more than max feature points.
                                                This field auto clears when VIO_DET1_CONTROL:: FeatureDetectorStart field is written with '1' 
                                                to start a fresh detection process.*/

        uint32_t Reserved               : 28; /* SBZ 0x0 Reserved (RAZ, SBZ) */    
    } field;
    uint32_t val;
} VIO_DET1_STATUS_T;

typedef union {
  struct {
    uint32_t  FeatureDetectorStart  : 1; /* RW 0x0 Writing '1' to this field starts FASTn feature point detection. Writing '0' clears the bit but has no other effect.*/
    uint32_t  SorterStart           : 1; /* RW 0x0 Writing '1' to this field starts Sorting (max K-selection) operation on detected feature points. */
    uint32_t  Reserved0             : 5; /* SBZ 0x0 Reserved (RAZ, SBZ) */
    uint32_t  AutoStartSorting      : 1; /* RW 0x0 Writing '1' to this field automatically starts 'sorting' and 'feature Patch generation' operations 
					    (in that sequence) after 'FeatureDetection' operation completes. When this field is set, Feature Detection 
					    operation is still controlled via 'FeatureDetectionStart' field, however, effects of 'SorterStart' and 'PatchGenStart' 
					    fields are ignored. Writing '0' clears the bit but has no other effect. */
    uint32_t  SortingWorkingSetSize : 8; /* RW 0x0  In the on-the-fly (OTF) mode of operation, this field sets the size of working set 
					    of features for dynamic heap sort operation. After every W features detected from an image, an intermediate dynamic 
					    heap sorting operation takes place.*/
    uint32_t  CenterWeightingEn     : 1; /* RW 0x0 Writing '1' to this field enables 'center weighting' on candidate pixels during FASTn feature point detection. */
    uint32_t  RegionMaskEn          : 1; /* RW 0x0 Writting '1' to this field enables 8x8 ‘region masks’ on candidate pixels during FASTn feature point detection.*/
    uint32_t  ExportAllFeatures     : 1; /* RW 0x0 Writing ‘1’ to this field makes Feature Detector Export all detected corners (X,Y,S packed in 32b) 
					    in On-the-fly (OTF) mode of operation.*/
    uint32_t  ExportAllDescriptors  : 1; /* RW 0x0 Writing ‘1’ to this field makes Feature Detector Export descriptors of all detected corners in 
					    On-the-fly (OTF) mode of operation.*/
    uint32_t  ImageBufferSpecifier  : 2; /* RW 0x0 This field specifies which image buffer to bind to Detector,
					    0x0 – Detector uses dedicated frame buffer
					    0x1 – Detector uses Circular Buffer-1 
					    0x2 – Detector uses Circular Buffer-2
					    0x3 - Reserved*/
    uint32_t  EnDynamicThreshold    : 1; /* RW 0x0 Writing ‘1’ to this field makes detector update detector threshold during detection dynamically 
					    as per minimum score of feature point detected so far.*/
    uint32_t  Reserved2             : 1; /* SBZ 0x0 Reserved (RAZ, SBZ) */
    uint32_t  DetectorThreshold     : 8; /* RW 0x0 This field contains the initial threshold value for the FASTn feature point detection. 
					    If a pixel is found to be a corner w.r.t. this threshold, further scoring operation is performed. */ 
  } field;
  uint32_t val;
} VIO_DET1_CONTROL_T;

typedef union {
    struct {
        uint32_t BaseAddress  : 18; /*  17:00  RW  0x0 Base address of the Image in SL2, 128b aligned word address.*/
        uint32_t Reserved     : 14; /*  31:18  SBZ 0x0 Reserved (RAZ, SBZ) */
    } field;
    uint32_t val;
} VIO_DET1_IMG_BASE_T;

typedef union {
    struct {
        uint32_t ImageWidth : 11; /*  10:00    RW  0x0 Width of the image in SL2,  must be multiple of 16 */
        uint32_t Reserved   : 21; /*  31:11    SBZ 0x0 Reserved (RAZ, SBZ)    */
    } field;
    uint32_t val;
} VIO_DET1_IMG_WIDTH_T;

typedef union {
    struct {
        uint32_t ImageHeight : 11; /*  10:00   RW  0x0 Height of the image in SL2,  must be >=64 */
        uint32_t Reserved : 21 ; /*  31:11     SBZ 0x0 Reserved (RAZ, SBZ)    */
    } field;
    uint32_t val;
} VIO_DET1_IMG_HEIGHT_T;

typedef union {
    struct {
        uint32_t ImageStride : 11; /*  10:00   RW  0x0 Row Stride of the image in SL2 */
        uint32_t Reserved : 21;    /*  31:11  SBZ 0x0 Reserved (RAZ, SBZ) */
    
    } field;
    uint32_t val;
} VIO_DET1_IMG_STRIDE_T;


typedef union {
    struct {
        uint32_t BaseAddress : 18; /*  17:00   RW  0x0 Base Address of 8x8 pixel region mask table in SL2, 18b word (128b) address. 
				       Needed when VIO_DET1_CONTROL::RegionMaskEn field is set by hostRow Stride of the image in SL2 */
        uint32_t Reserved : 14;    /*  31:18  SBZ 0x0 Reserved (RAZ, SBZ) */
    
    } field;
    uint32_t val;
} VIO_DET1_ROIMASK_BASE_T;


typedef union {
    struct {
        uint32_t FPListLength : 13; /*  12:00  RO  0x0 Required Length/number of ‘top’ (strength sorted) features to be extracted from input image.
					This field is written by detector HW and read by sorter HW. Software can also read/write to this field.*/
        uint32_t Reserved : 19;     /*  31:13  SBZ 0x0 Reserved (RAZ, SBZ)    */
    } field;
    uint32_t val;
} VIO_DET1_SORTED_FPLIST_LENGTH_T;

typedef union {
    struct {
        uint32_t BaseAddress : 18;   /*  17:00   RW  0x0 Base Address of strength sorted feature list 
					 (Feature coordinate X,Y, score S, descriptor D in packed format) in SL2, 18b word (128b) address*/
        uint32_t Reserved : 14;      /*  31:18  SBZ 0x0 Reserved (RAZ, SBZ)    */
    } field;
    uint32_t val;
} VIO_DET1_SORTED_FPLIST_BASE_T;

typedef union {
    struct {
        uint32_t FPListLength : 13;   /*  13:00   RW  0x0 Detector Total Detected Feature Number Register*/
        uint32_t Reserved : 19;      /*  31:18  SBZ 0x0 Reserved (RAZ, SBZ)    */
    } field;
    uint32_t val;
} VIO_DET1_TOT_FNUMBER_T;

typedef union {
    struct {
        uint32_t BaseAddress : 18;   /*  17:00   RW  0x0 Base Address of exported feature (all that were detected) list in SL2, 
					 18b word (128b) address. Needed when VIO_DET1_CONTROL::ExportAllFeatures field is set by host.*/
        uint32_t Reserved : 14;      /*  31:18  SBZ 0x0 Reserved (RAZ, SBZ)    */
    } field;
    uint32_t val;
} VIO_DET1_EXP_FLIST_BASE_T;

typedef union {
    struct {
        uint32_t BaseAddress : 18;   /*  17:00   RW  0x0 Base Base Address of temporary Work BufferA in SL2, 18b word (128b) address. 
					 Used by detector to pass feature descriptors of working feature set (W features) to sorter for dynamic heap sort.*/
        uint32_t Reserved : 14;      /*  31:18  SBZ 0x0 Reserved (RAZ, SBZ)    */
    } field;
    uint32_t val;
} VIO_DET1_WRK_BUFA_BASE_T;

typedef union {
    struct {
        uint32_t BaseAddress : 18;   /*  17:00   RW  0x0 Base Base Address of temporary Work BufferB in SL2, 18b word (128b) address. 
					 Used by detector to pass feature descriptors of working feature set (W features) to sorter for dynamic heap sort.*/
        uint32_t Reserved : 14;      /*  31:18  SBZ 0x0 Reserved (RAZ, SBZ)    */
    } field;
    uint32_t val;
} VIO_DET1_WRK_BUFB_BASE_T;


typedef union {
    struct {
        uint32_t FeatureDetectorDone    : 1; /* RO 0x0 This field is set to '1' when the last initiated FASTn FeatureDetector operation completes.
						This field auto clears when VIO_DET2_CONTROL::FeatureDetectorStart field is written with '1' 
						to start a fresh FASTn feature detection process.*/
        uint32_t SorterDone             : 1; /* RO 0x0 This field is set to '1' when the last initiated Sorting (max K-selection) operation completes.
						This field auto clears when VIO_DET2_CONTROL::SortingStart field is written with '1' to start a fresh sorting process. */
        uint32_t CircularBufferOverflow : 1; /* RO 0x0 This field is set to '1' when detector circular buffer overflows.
						This field auto clears when VIO_DET2_CONTROL:: FeatureDetectorStart field is written with '1' 
						to start a fresh detection process.*/
        uint32_t MaxNumFeatureExceeded  : 1; /* RO 0x0 This field is set to '1' when detector exceeds detecting more than max feature points.
						This field auto clears when VIO_DET2_CONTROL:: FeatureDetectorStart field is written with '1' 
						to start a fresh detection process.*/

        uint32_t Reserved               : 28; /* SBZ 0x0 Reserved (RAZ, SBZ) */    
    } field;
    uint32_t val;
} VIO_DET2_STATUS_T;

typedef union {
  struct {
    uint32_t  FeatureDetectorStart  : 1; /* RW 0x0 Writing '1' to this field starts FASTn feature point detection. Writing '0' clears the bit but has no other effect.*/
    uint32_t  SorterStart           : 1; /* RW 0x0 Writing '1' to this field starts Sorting (max K-selection) operation on detected feature points. */
    uint32_t  Reserved0             : 5; /* SBZ 0x0 Reserved (RAZ, SBZ) */
    uint32_t  AutoStartSorting      : 1; /* RW 0x0 Writing '1' to this field automatically starts 'sorting' and 'feature Patch generation' operations 
					    (in that sequence) after 'FeatureDetection' operation completes. When this field is set, Feature Detection 
					    operation is still controlled via 'FeatureDetectionStart' field, however, effects of 'SorterStart' and 'PatchGenStart' 
					    fields are ignored. Writing '0' clears the bit but has no other effect. */
    uint32_t  SortingWorkingSetSize : 8; /* RW 0x0  In the on-the-fly (OTF) mode of operation, this field sets the size of working set 
					    of features for dynamic heap sort operation. After every W features detected from an image, an intermediate dynamic 
					    heap sorting operation takes place.*/
    uint32_t  CenterWeightingEn     : 1; /* RW 0x0 Writing '1' to this field enables 'center weighting' on candidate pixels during FASTn feature point detection. */
    uint32_t  RegionMaskEn          : 1; /* RW 0x0 Writting '1' to this field enables 8x8 ‘region masks’ on candidate pixels during FASTn feature point detection.*/
    uint32_t  ExportAllFeatures     : 1; /* RW 0x0 Writing ‘1’ to this field makes Feature Detector Export all detected corners (X,Y,S packed in 32b) 
					    in On-the-fly (OTF) mode of operation.*/
    uint32_t  ExportAllDescriptors  : 1; /* RW 0x0 Writing ‘1’ to this field makes Feature Detector Export descriptors of all detected corners in 
					    On-the-fly (OTF) mode of operation.*/
    uint32_t  ImageBufferSpecifier  : 2; /* RW 0x0 This field specifies which image buffer to bind to Detector,
					    0x0 – Detector uses dedicated frame buffer
					    0x1 – Detector uses Circular Buffer-1 
					    0x2 – Detector uses Circular Buffer-2
					    0x3 - Reserved*/
    uint32_t  EnDynamicThreshold    : 1; /* RW 0x0 Writing ‘1’ to this field makes detector update detector threshold during detection dynamically 
					    as per minimum score of feature point detected so far.*/
    uint32_t  Reserved2             : 1; /* SBZ 0x0 Reserved (RAZ, SBZ) */
    uint32_t  DetectorThreshold     : 8; /* RW 0x0 This field contains the initial threshold value for the FASTn feature point detection. 
					    If a pixel is found to be a corner w.r.t. this threshold, further scoring operation is performed. */ 
  } field;
  uint32_t val;
} VIO_DET2_CONTROL_T;

typedef union {
    struct {
        uint32_t BaseAddress  : 18; /*  17:00  RW  0x0 Base address of the Image in SL2, 128b aligned word address.*/
        uint32_t Reserved     : 14; /*  31:18  SBZ 0x0 Reserved (RAZ, SBZ) */
    } field;
    uint32_t val;
} VIO_DET2_IMG_BASE_T;

typedef union {
    struct {
        uint32_t ImageWidth : 11; /*  10:00    RW  0x0 Width of the image in SL2,  must be multiple of 16 */
        uint32_t Reserved   : 21; /*  31:11    SBZ 0x0 Reserved (RAZ, SBZ)    */
    } field;
    uint32_t val;
} VIO_DET2_IMG_WIDTH_T;

typedef union {
    struct {
        uint32_t ImageHeight : 11; /*  10:00   RW  0x0 Height of the image in SL2,  must be >=64 */
        uint32_t Reserved : 21 ; /*  31:11     SBZ 0x0 Reserved (RAZ, SBZ)    */
    } field;
    uint32_t val;
} VIO_DET2_IMG_HEIGHT_T;

typedef union {
    struct {
        uint32_t ImageStride : 11; /*  10:00   RW  0x0 Row Stride of the image in SL2 */
        uint32_t Reserved : 21;    /*  31:11  SBZ 0x0 Reserved (RAZ, SBZ) */
    
    } field;
    uint32_t val;
} VIO_DET2_IMG_STRIDE_T;


typedef union {
    struct {
        uint32_t BaseAddress : 18; /*  17:00   RW  0x0 Base Address of 8x8 pixel region mask table in SL2, 18b word (128b) address. 
				       Needed when VIO_DET2_CONTROL::RegionMaskEn field is set by hostRow Stride of the image in SL2 */
        uint32_t Reserved : 14;    /*  31:18  SBZ 0x0 Reserved (RAZ, SBZ) */
    
    } field;
    uint32_t val;
} VIO_DET2_ROIMASK_BASE_T;


typedef union {
    struct {
        uint32_t FPListLength : 13; /*  12:00  RO  0x0 Required Length/number of ‘top’ (strength sorted) features to be extracted from input image.
					This field is written by detector HW and read by sorter HW. Software can also read/write to this field.*/
        uint32_t Reserved : 19;     /*  31:13  SBZ 0x0 Reserved (RAZ, SBZ)    */
    } field;
    uint32_t val;
} VIO_DET2_SORTED_FPLIST_LENGTH_T;

typedef union {
    struct {
        uint32_t BaseAddress : 18;   /*  17:00   RW  0x0 Base Address of strength sorted feature list 
					 (Feature coordinate X,Y, score S, descriptor D in packed format) in SL2, 18b word (128b) address*/
        uint32_t Reserved : 14;      /*  31:18  SBZ 0x0 Reserved (RAZ, SBZ)    */
    } field;
    uint32_t val;
} VIO_DET2_SORTED_FPLIST_BASE_T;

typedef union {
    struct {
        uint32_t FPListLength : 13;   /*  13:00   RW  0x0 Detector Total Detected Feature Number Register*/
        uint32_t Reserved : 19;      /*  31:18  SBZ 0x0 Reserved (RAZ, SBZ)    */
    } field;
    uint32_t val;
} VIO_DET2_TOT_FNUMBER_T;

typedef union {
    struct {
        uint32_t BaseAddress : 18;   /*  17:00   RW  0x0 Base Address of exported feature (all that were detected) list in SL2, 
					 18b word (128b) address. Needed when VIO_DET2_CONTROL::ExportAllFeatures field is set by host.*/
        uint32_t Reserved : 14;      /*  31:18  SBZ 0x0 Reserved (RAZ, SBZ)    */
    } field;
    uint32_t val;
} VIO_DET2_EXP_FLIST_BASE_T;

typedef union {
    struct {
        uint32_t BaseAddress : 18;   /*  17:00   RW  0x0 Base Base Address of temporary Work BufferA in SL2, 18b word (128b) address. 
					 Used by detector to pass feature descriptors of working feature set (W features) to sorter for dynamic heap sort.*/
        uint32_t Reserved : 14;      /*  31:18  SBZ 0x0 Reserved (RAZ, SBZ)    */
    } field;
    uint32_t val;
} VIO_DET2_WRK_BUFA_BASE_T;

typedef union {
    struct {
        uint32_t BaseAddress : 18;   /*  17:00   RW  0x0 Base Base Address of temporary Work BufferB in SL2, 18b word (128b) address. 
					 Used by detector to pass feature descriptors of working feature set (W features) to sorter for dynamic heap sort.*/
        uint32_t Reserved : 14;      /*  31:18  SBZ 0x0 Reserved (RAZ, SBZ)    */
    } field;
    uint32_t val;
} VIO_DET2_WRK_BUFB_BASE_T;


typedef union {
    struct {
        uint32_t FeatureTrackerDone :1 ; /* 00:00  RO   0x0 This field is set to '1' when the last initiated tracking (of a list of feature position predictions) 
					    operation completes.
                                            This field auto clears when VIO_TRK_CONTROL::FeatureTrackerStart field is written with '1' to start a fresh feature tracking process. */
        uint32_t Reserved :31 ;          /* 31:01  SBZ  0x0 Reserved (RAZ, SBZ) */
    } field;
    uint32_t val;
} VIO_TRK_STATUS_T;

typedef union {
    struct {
        uint32_t FeatureTrackerStart :1 ; /* 00:00 RW   0x0 Writing '1' to this field starts feature tracking operation. Writing '0' clears the bit but has no other effect. */
        uint32_t TrkForceModuleClkEn :1 ; /* Setting this field to ‘1’ forces modules level clock to be always enabled (disable module level clock gating).
					    Setting this field to ‘0’ allows module level clock gating to be enabled when modules reach a non-active state.*/
        uint32_t Reserved            :14; /* 15:01 SBZ  0x0 Reserved (RAZ, SBZ) */
        uint32_t CenterWeightingEn   :1 ; /* 16:16 RW   0x0 Writing '1' to this field enables 'center weighting' on:
                                             1. candidate pixels during FASTn feature detection
                                             2. reference and candidate patches during NCC scoring 
                                             Writing '0' clears the bit but has no other effect. */
        uint32_t Reserved2           :3 ; /* 19:17 SBZ  0x0 Reserved (RAZ, SBZ) */
        uint32_t ImageBufferSpecifier:2 ; /* 21:20 RW 0x0 This field specifies which image buffer to bind to Tracker,
					     0x0 – Tracker uses dedicated frame buffer
					     0x1 – Tracker uses Circular Buffer-1 
					     0x2 – Tracker uses Circular Buffer-2
					     0x3 - Reserved*/
        uint32_t Reserved3           :2 ; /* 23:22 SBZ  0x0 Reserved (RAZ, SBZ) */
        uint32_t TrackerThreshold    :8 ; /* 31:24 RW   0x0 This field contains the initial threshold value for the FASTn feature point detection. 
					     If a pixel is found to be a corner w.r.t. this threshold, it is considered to be candidate for a NCC 
					     based scoring and best candidate patch search. */
    } field;
     uint32_t val;
} VIO_TRK_CONTROL_T;

typedef union {
    struct {
        uint32_t BaseAddress    :18 ; /* 17:00 RW   0x0 Base address of the Image in SL2, 128b aligned word address. */
        uint32_t Reserved       :14 ; /* 31:18 SBZ  0x0 Reserved (RAZ, SBZ) */
    } field;
    uint32_t val;
} VIO_TRK_IMG_BASE_T;

typedef union {
    struct {
        uint32_t ImageWidth     :11 ; /* 10:00 RW   0x0 Width of the image in SL2,  must be multiple of 16 */
        uint32_t Reserved       :21 ; /* 31:11 SBZ  0x0 Reserved (RAZ, SBZ) */
    } field;
    uint32_t val;
} VIO_TRK_IMG_WIDTH_T;

typedef union {
    struct {
        uint32_t ImageHeight    :11 ; /* 10:00 RW   0x0 Height of the image in SL2,  must be >=64 */
        uint32_t Reserved       :21 ; /* 31:11 SBZ  0x0 Reserved (RAZ, SBZ) */
    } field;
    uint32_t val;
} VIO_TRK_IMG_HEIGHT_T;

typedef union {
    struct {
        uint32_t ImageStride    :11 ; /* 10:00 RW   0x0 Row Stride of the image in SL2 */
        uint32_t Reserved       :21 ; /* 31:11 SBZ  0x0 Reserved (RAZ, SBZ) */
    } field;
    uint32_t val;
} VIO_TRK_IMG_STRIDE_T;

typedef union {
    struct {
        uint32_t BaseAddress    :18 ; /* 17:00 RW   0x0 Base Address of feature (to-be-tracked) prediction list, It is a 128b aligned word address. */
        uint32_t Reserved       :14 ; /* 31:18 SBZ  0x0 Reserved (RAZ, SBZ) */
    } field;
    uint32_t val;
} VIO_TRK_FPPREDLIST_BASE_T;

//start from here
typedef union {
    struct {
        uint32_t FPPredListLength   :13 ; /* 12:00 RO   0x0 Length/number of feature (to-be-tracked) prediction list */
        uint32_t Reserved           :19 ; /* 31:13 SBZ  0x0 Reserved (RAZ, SBZ) */
    } field;
    uint32_t val;
} VIO_TRK_FPPREDLIST_LENGTH_T;

typedef union {
    struct {
        uint32_t BaseAddress    :18 ; /* 17:00 RW   0x0 Base Address of feature (to-be-tracked) patch list, it is a 128b aligned address. */
        uint32_t Reserved       :14 ; /* 31:18 SBZ  0x0 Reserved (RAZ, SBZ) */
    } field;
    uint32_t val;
} VIO_TRK_FPPATCHLIST_BASE_T;


typedef union {
    struct {
        uint32_t FPPatchListLength   :13 ; /* 12:00 RO   0x0 Length/number of feature (to-be-tracked) prediction list */
        uint32_t Reserved           :19 ; /* 31:13 SBZ  0x0 Reserved (RAZ, SBZ) */
    } field;
    uint32_t val;
} VIO_TRK_FPPATCHLIST_LENGTH_T;



typedef union {
    struct {
        uint32_t BaseAddress    :18 ; /* 17:00 RW   0x0 Base Address of tracked feature position list, it is a 128b aligned address. */
        uint32_t Reserved       :14 ; /* 31:18 SBZ  0x0 Reserved (RAZ, SBZ) */
    } field;
    uint32_t val;
} VIO_TRK_FPTRACKLIST_BASE_T;

////////////////////////////////////////////////////////////////////////////

typedef union {
    struct {
        uint32_t PredictionCovDone  :1; /* 00:00 RO 0x0 This field is set to '1' when the last initiated Prediction Covariance computation operation completes.
				       This field auto clears when VIO_EKF_CONTROL::PredictionCovStart field is written with '1' 
				       to start a fresh Prediction Covariance computation. */
        uint32_t CholeskyDecompDone :1; /* 01:01 RO 0x0 This field is set to '1' when the last initiated Cholesky decomposition operation completes.
				       This field auto clears when VIO_EKF_CONTROL::CholeskyDecompStart field is written with '1' 
				       to start a fresh Cholesky decomposition operation.*/
        uint32_t MatSolveDone       :1; /* 02:02 RO 0x0 This field is set to '1' when the last initiated Matrix solve operation completes.
				       This field auto clears when VIO_EKF_CONTROL::MatSolveStart field is written with '1' to start a fresh Matrix solve process.*/
        uint32_t MatMulDone         :1; /* 03:03 RO 0x0 This field is set to '1'when the last initiated Matrix Multiplication operation completes. 
				      This field auto clears when VIO_EKF_CONTROL::MatMulStart field is written with '1' to start a fresh MatMul process.*/
        uint32_t Reserved           :28; /* 31:05 SBZ 0x0 Reserved (RAZ, SBZ) */
  } field;
  uint32_t val;
} VIO_EKF_STATUS_T;

typedef union {
    struct {
        uint32_t PredictionCovStart       :1; /* 00:00 RW 0x0 Writing '1' to this field starts Prediction Covariance computation operation.
                                             Writing '0' clears the bit but has no other effect. */
        uint32_t CholeskyDecompStart      :1; /* 01:01 RW 0x0 Writing '1' to this field starts Cholesky Decomposition computation operation.
                                             Writing '0' clears the bit but has no other effect. */
        uint32_t MatSolveStart            :1; /* 02:02 RW 0x0 Writing '1' to this field starts Matrix Solve operation.
                                             Writing '0' clears the bit but has no other effect. */
        uint32_t MatMulStart              :1; /* 03:03 RW 0x0 Writing '1' to this field starts MatMul operation.
                                             Writing '0' clears the bit but has no other effect. */
        uint32_t Cholesky_ForceModuleClkEn:1; /* 04:04  RW 0x0 Setting this field to ‘1’ forces modules level clock to be always enabled 
						 (disable module level clock gating). Setting this field to ‘0’ allows module level clock gating to be 
						 enabled when modules reach a non-active state.*/
        uint32_t MatSolve_ForceModuleClkEn:1; /* 05:05 RW 0x0 Setting this field to ‘1’ forces modules level clock to be always enabled 
						 (disable module level clock gating). Setting this field to ‘0’ allows module level clock gating to be 
						 enabled when modules reach a non-active state. */
        uint32_t MatMul_ForceModuleClkEn  :1; /* 06:06 RW 0x0 Setting this field to ‘1’ forces modules level clock to be always enabled 
						 (disable module level clock gating). Setting this field to ‘0’ allows module level clock gating to be 
						 enabled when modules reach a non-active state. */
        uint32_t MatSolveSkipStage1       :1; /* 07:07 RW 0x0 Setting this field to ‘1’ makes MatSolve operation skip the 1st stage of computation 
						 (Forward substitution step in a LU decomposition based Matrix solver process). Setting this field to ‘0’ 
						 makes MatSolve operation execute 1st stage (Forward substitution step in a LU decomposition based Matrix solver process).  
						 Setting both MatSolveSkipStage1 and MatSolveSkipStage2 to 0 makes MatSolve operation execute both 1st and 
						 2nd stages (Forward and backward substitution steps in a LU decomposition based Matrix Solver process).*/
        uint32_t MatSolveSkipStage2       :1; /* 08:08 Setting this filed to '1' makes MatSolve operation skip the 2nd stage of computation 
						 (Backward subsitution step in  LU decomposition based Matrix solver process).
						 Setting this field to '0' makes MatSolve operation execute both 1st and 2nd stage 
						 (Forward and Backward subsitution step in a LU decomposition based Matrix solver process). */
        uint32_t MatSolveDualWrEn         :1; /* 09:09 Setting this field to '1' makes MatSolve operation write the output(the matrix X 
						 while solving for AX=B to both X and B matrix memory. 
						 Setting this field to '0' makes MatSolve operation write the output (the matrix X while solving for AX=B) 
						 to only X matrix memory. */
        uint32_t MatMulComputeMode        :2; /* 11:10 This field specifies how MatMul operation computes output matrix C 
						 for SGEMM style MatMul operation ( C = Alpha**B + Beta*C);
						 0x00: computes output matrix C in full (default)
						 0x01: computes only lower triangular (LT) part of the C
						 0x02: computes only upper triangular (UT) part of C
						 0x03: Reserved*/
        uint32_t CholWrNonDiagInv         :1; /* 12:12 RW 0x0 Setting this bit to ‘1’ makes Cholesky write diagonal elements as Reciprocal of computed value.
						 Setting this bit to ‘0’ makes Cholesky write diagonal elements as computed value. */
        uint32_t Reserved1                :3; /* 15:13 SBZ 0x0 Reserved (RAZ, SBZ) */
        uint32_t RDMatSlvPA               :1; /* 16:16 RW 0x0 Setting this field to '1' reverses matrix dimension values (row/col flip) 
						 provided to MatrixSolve module PortA (Cholesky Decomposed Matrix)
						 Setting it to '0' maintains/provides unmodified values. */
        uint32_t RDMatSlvPX               :1; /* 17:17 RW 0x0 Setting this field to '1' reverses matrix dimension values (row/col flip) 
						 provided to MatrixSolve module PortX (Kalman Gain Matrix)
						 Setting it to '0' maintains/provides unmodified values. */
        uint32_t RDMatSlvPB               :1; /* 18:18 RW 0x0 Setting this field to '1' reverses matrix dimension values (row/col flip) 
						 provided to MatrixSolve module PortB (LC Matrix)
						 Setting it to '0' maintains/provides unmodified values. */
        uint32_t RDMatMulPA               :1; /* 19:19 RW 0x0 Setting this field to '1' reverses matrix dimension values (row/col flip) 
						 provided to MatrixMul module PortA (LC Matrix) during MatMul process
						 Setting it to '0' maintains/provides unmodified values. */
        uint32_t RDMatMulPB               :1; /* 20:20 RW 0x0 Setting this field to '1' reverses matrix dimension values (row/col flip) 
						 provided to MatrixMul module PortB (Kalman Gain Matrix) during MatrixMul process
						 Setting it to '0' maintains/provides unmodified values. */
        uint32_t RDMatMulPC               :1; /* 21:21 RW 0x0 Setting this field to '1' reverses matrix dimension values (row/col flip) 
						 provided to MatrixMul module PortC (StateCov Matrix) during MatMul process
						 Setting it to '0' maintains/provides unmodified values. */
        uint32_t Reserved2                :10; /* 31:22 SBZ 0x0 Reserved (RAZ, SBZ) */
    } field;
    uint32_t val;
} VIO_EKF_CONTROL_T;

typedef union {
    struct {
        float Alpha ; /* 31:00 RW 0x0 The coefficient Alpha for the EKF State Update matrix multiplication (C = Alpha*A*B + Beta*C) operation. */
    } field;
    uint32_t val;
} VIO_EKF_MATMUL_COEF_A_T;

typedef union {
    struct {
        float Beta; /* 31:00 RW 0x0 The coefficient Beta for the EKF State Update matrix multiplication (C = Alpha*A*B + Beta*C) operation.*/
    } field;
    uint32_t val;
} VIO_EKF_MATMUL_COEF_B_T;

typedef union {
    struct {
        uint32_t Base             :18; /* 17:00 RW 0x0 Base address of EKF Matrix Multiplier module memory access, Multiple of 16 bytes address.*/
        uint32_t Reserved         :14; /* 31:18 Reserved (RAZ, SBZ)*/
    } field;
    uint32_t val;
} VIO_EKF_MMUL_A_BASE_T;

typedef union {
    struct {
        uint32_t NumRows          :16; /* 15:00 RW 0x0 Number of rows of Matrix Multiplier Matrix A.*/
        uint32_t NumCols          :16; /* 31:16 RW 0x0 Number of columns of Matrix Multiplier Matrix A.*/
    } field;
    uint32_t val;
} VIO_EKF_MMUL_A_DIM_T;

typedef union {
    struct {
        uint32_t Stride       :18; /* 17:00 Row/Col Stride of EKF Matrix A, Multiple of 16 bytes address.*/
        uint32_t Matrixlayout :3; /* 20:18 Bits definition 
				     20- compressed,
				     19- Upper Triangular (UT),
				     18-Lower Triangular (LT)
				     Matrix storage type:
				     0x000 – rectangular data stored
				     0x001 – rectangular matrix with only LT valid data 
				     0x010 – rectangular matrix with only UT valid data
				     0x011 - Reserved 
				     0x100 – Reserved
				     0x101 – compressed LT data stored 
				     0x110 – compressed UT data stored 
				     0x111 - Reserved */
        uint32_t Order        :1; /* 21:21 0 – A Operand is Row Major
				       1 – A Operand is Col Major*/
        uint32_t Reserved     :10; /* 31:22 SBZ 0x0 Reserved (RAZ, SBZ) */
    } field;
    uint32_t val;
} VIO_EKF_MMUL_A_FORMAT_T;

typedef union {
    struct {
        uint32_t Base             :18; /* 17:00 RW 0x0 Base address of EKF Matrix Multiplier module memory access 
				       for matrix B, Multiple of 16 bytes address.*/
        uint32_t Reserved         :14; /* 31:18 Reserved (RAZ, SBZ)*/
    } field;
    uint32_t val;
} VIO_EKF_MMUL_B_BASE_T;

typedef union {
    struct {
        uint32_t NumRows          :16; /* 15:00 RW 0x0 Number of rows of Matrix Multiplier Matrix B.*/
        uint32_t NumCols          :16; /* 31:16 RW 0x0 Number of columns of Matrix Multiplier Matrix B.*/
    } field;
    uint32_t val;
} VIO_EKF_MMUL_B_DIM_T;

typedef union {
    struct {
        uint32_t Stride       :18; /* 17:00 Row/Col Stride of EKF input and out Matrix, Multiple of 16 bytes address.*/
        uint32_t Matrixlayout :3; /* 20:18 Bits definition 
				     20- compressed,
				     19- Upper Triangular (UT),
				     18-Lower Triangular (LT)
				     Matrix storage type:
				     0x000 – rectangular data stored
				     0x001 – rectangular matrix with only LT valid data 
				     0x010 – rectangular matrix with only UT valid data
				     0x011 - Reserved 
				     0x100 – Reserved
				     0x101 – compressed LT data stored 
				     0x110 – compressed UT data stored 
				     0x111 - Reserved */
        uint32_t Order        :1; /* 21:21 0 – B Operand is Row Major
				       1 – B Operand is Col Major*/
        uint32_t Reserved     :10; /* 31:22 SBZ 0x0 Reserved (RAZ, SBZ) */
    } field;
    uint32_t val;
} VIO_EKF_MMUL_B_FORMAT_T;

typedef union {
    struct {
        uint32_t Base             :18; /* 17:00 RW 0x0 Base address of EKF Matrix Multiplier module memory access, Multiple of 16 bytes address.*/
        uint32_t Reserved         :14; /* 31:18 Reserved (RAZ, SBZ)*/
    } field;
    uint32_t val;
} VIO_EKF_MMUL_C_BASE_T;

typedef union {
    struct {
        uint32_t NumRows          :16; /* 15:00 RW 0x0 Number of rows of Matrix Multiplier Matrix C.*/
        uint32_t NumCols          :16; /* 31:16 RW 0x0 Number of columns of Matrix Multiplier Matrix C.*/
    } field;
    uint32_t val;
} VIO_EKF_MMUL_C_DIM_T;

typedef union {
    struct {
        uint32_t Stride       :18; /* 17:00 Row/Col Stride of EKF Matrix C, Multiple of 16 bytes address.*/
        uint32_t Matrixlayout :3; /* 20:18 Bits definition 
				     20- compressed,
				     19- Upper Triangular (UT),
				     18-Lower Triangular (LT)
				     Matrix storage type:
				     0x000 – rectangular data stored
				     0x001 – rectangular matrix with only LT valid data 
				     0x010 – rectangular matrix with only UT valid data
				     0x011 - Reserved 
				     0x100 – Reserved
				     0x101 – compressed LT data stored 
				     0x110 – compressed UT data stored 
				     0x111 - Reserved */
        uint32_t Order        :1; /* 21:21 0 – C Operand is Row Major
				       1 – C Operand is Col Major*/
        uint32_t Reserved     :10; /* 31:22 SBZ 0x0 Reserved (RAZ, SBZ) */
    } field;
    uint32_t val;
} VIO_EKF_MMUL_C_FORMAT_T;

typedef union {
    struct {
        uint32_t Base             :18; /* 17:00 RW 0x0 Base address of EKF Matrix Multiplier module memory access, Multiple of 16 bytes address.*/
        uint32_t Reserved         :14; /* 31:18 Reserved (RAZ, SBZ)*/
    } field;
    uint32_t val;
} VIO_EKF_MMUL_Y_BASE_T;


typedef union {
    struct {
        uint32_t Base          :18; /* 17:00 RW 0x0 Base address of EKF Cholesky module memory access, Multiple of 16 bytes address.*/
        uint32_t Reserved      :14; /* 31:18 SBZ 0x0 Reserved (RAZ, SBZ) */
    } field;
    uint32_t val;
} VIO_EKF_CHOL_IN_BASE_T;

typedef union {
    struct {
        uint32_t Base          :18; /* 17:00 RW 0x0 Base address of EKF Cholesky module memory access, Multiple of 16 bytes address.*/
        uint32_t Reserved      :14; /* 31:18 SBZ 0x0 Reserved (RAZ, SBZ) */
    } field;
    uint32_t val;
} VIO_EKF_CHOL_OUT_BASE_T;

typedef union {
    struct {
        uint32_t NumRows       :16; /* 15:00 RW 0x0 Number of rows of Cholesky Matrix.*/
        uint32_t Reserved      :16; /* 31:16 SBZ 0x0 Reserved (RAZ, SBZ) */
    } field;
    uint32_t val;
} VIO_EKF_CHOL_DIM_T;

typedef union {
    struct {
        uint32_t Stride        :18; /* 17:00 RW 0x0 Row/Col Stride of EKF input and output Matrix, Multiple of 16 bytes address.*/
        uint32_t Matrixlayout  :2; /* 19:18 RW 0x0 Matrix storage type:
				      0x00 - matrix stored as square
				      0x01 - Reserved
				      0x10 - matrix stored as lower triangular (LT)
				      0x11 - matrix stored as upper triangular (UT)*/
        uint32_t Reserved      :12; /*31:20 SBZ 0x0 Reserved (RAZ, SBZ)*/
    } field;
    uint32_t val;
} VIO_EKF_CHOL_FORMAT_T;


typedef union {
    struct {
        uint32_t Base          :18; /* 17:00 RW 0x0 Base address of EKF Matrix Solve module memory access, Multiple of 16 bytes address.*/
        uint32_t Reserved      :14; /* 31:18 SBZ 0x0 Reserved (RAZ, SBZ) */
    } field;
    uint32_t val;
} VIO_EKF_MSLV_A_BASE_T;

typedef union {
    struct {
        uint32_t NumRows       :16; /* 15:00 RW 0x0 Number of rows of Cholesky Matrix.*/
        uint32_t Reserved      :16; /* 31:16 SBZ 0x0 Reserved (RAZ, SBZ) */
    } field;
    uint32_t val;
} VIO_EKF_MSLV_A_DIM_T;

typedef union {
    struct {
        uint32_t Stride        :18; /* 17:00 RW 0x0 Row/Col Stride of EKF Matrix A, Multiple of 16 bytes address.*/
        uint32_t Matrixlayout  :2; /* 19:18 RW 0x0 Matrix storage type:
				      0x00 - matrix stored as square
				      0x01 - Reserved
				      0x10 - matrix stored as lower triangular (LT)
				      0x11 - matrix stored as upper triangular (UT)*/
        uint32_t Reserved      :12; /*31:20 SBZ 0x0 Reserved (RAZ, SBZ)*/
    } field;
    uint32_t val;
} VIO_EKF_MSLV_A_FORMAT_T;


typedef union {
    struct {
        uint32_t Base          :18; /* 17:00 RW 0x0 Base address of EKF Matrix Solve module memory access, Multiple of 16 bytes address.*/
        uint32_t Reserved      :14; /* 31:18 SBZ 0x0 Reserved (RAZ, SBZ) */
    } field;
    uint32_t val;
} VIO_EKF_MSLV_X_BASE_T;


typedef union {
    struct {
        uint32_t NumRows       :16; /* 15:00 RW 0x0 Number of rows of Cholesky Matrix.*/
        uint32_t NumCols       :16; /* 31:16 SBZ 0x0 Reserved (RAZ, SBZ) */
    } field;
    uint32_t val;
} VIO_EKF_MSLV_X_DIM_T;

typedef union {
    struct {
        uint32_t Stride        :18; /* 17:00 RW 0x0 Row/Col Stride of EKF Matrix X, Multiple of 16 bytes address.*/
        uint32_t Reserved      :14; /* 31:18 SBZ 0x0 Reserved (RAZ, SBZ)*/
    } field;
    uint32_t val;
} VIO_EKF_MSLV_X_FORMAT_T;

typedef union {
    struct {
        uint32_t Base          :18; /* 17:00 RW 0x0 Base address of EKF Matrix Solve module memory access, Multiple of 16 bytes address.*/
        uint32_t Reserved      :14; /* 31:18 SBZ 0x0 Reserved (RAZ, SBZ) */
    } field;
    uint32_t val;
} VIO_EKF_MSLV_B_BASE_T;

typedef union {
    struct {
        uint32_t NumRows       :16; /* 15:00 RW 0x0 Number of rows of Cholesky Matrix.*/
        uint32_t NumCols       :16; /* 31:16 RW 0x0 Number of columns of Matrix Solve Matrix B*/
    } field;
    uint32_t val;
} VIO_EKF_MSLV_B_DIM_T;

typedef union {
    struct {
        uint32_t Stride        :18; /* 17:00 RW 0x0 Row/Col Stride of EKF input and output Matrix, Multiple of 16 bytes address.*/
        uint32_t Reserved      :14; /* 31:18 SBZ 0x0 Reserved (RAZ, SBZ)*/
    } field;
    uint32_t val;
} VIO_EKF_MSLV_B_FORMAT_T;


typedef union {
    struct {
        uint32_t MatMulError        :8; /* 07:00 RW 0x0 8 bit Error Flag from Matrix Multiplier Floating point operation.*/
        uint32_t MatSolveError      :8; /* 15:08 RW 0x0 8 bit Error Flag from Matrix Solve Floating point operation.*/
        uint32_t CholError          :8; /* 23:16 RW 0x0 8 bit Error Flag from Cholesky Floating point operation.*/
        uint32_t CholSQRTError      :1; /* 24:24 RW 0x0 Set to 1 if –ve value encountered for square root operation.*/
        uint32_t Reserved           :7; /* 31:25 SBZ 0x0 Reserved (RAZ, SBZ)*/
    } field;
    uint32_t val;
} VIO_EKF_ERROR_STATUS_T;


typedef union {
    struct {
        uint32_t MatMulErrorMask    :8; /* 07:00 RW 0x0 Mask Bits for Error Flag. If set Status will not be captured.*/
        uint32_t MatSolveErrorMask  :8; /* 15:08 RW 0x0 Mask Bits for Error Flag. If set Status will not be captured.*/
        uint32_t CholErrorMask      :8; /* 23:16 RW 0x0 Mask Bits for Error Flag. If set Status will not be captured.*/
        uint32_t CholSQRTErrorMask  :1; /* 24:24 RW 0x0 Mask Bits for Cholesky SQRT Error. If set Status will not be captured.*/
        uint32_t Reserved           :7; /* 31:25 SBZ 0x0 Reserved (RAZ, SBZ)*/
    } field;
    uint32_t val;
} VIO_EKF_ERROR_STATUS_MASK_T;

typedef union {
    struct {
        uint32_t CholSQRTErrorRowID :8; /* 07:00 RW 0x0 Mask Bits for Error Flag. If set Status will not be captured.*/
        uint32_t Reserved           :24; /* 31:8 SBZ 0x0 Reserved (RAZ, SBZ)*/
    } field;
    uint32_t val;
} VIO_EKF_CHOL_ERROR_T;



////////////////////////////////////////////////////////////////////////////
typedef struct VIO_TOP_t {
    VIO_TOP_STATUS_T VIO_TOP_STATUS;
    VIO_TOP_CONTROL_T VIO_TOP_CONTROL;
    VIO_INTERRUPT_FLAG_T VIO_INTERRUPT_FLAG;
    VIO_INTERRUPT_MASK_T VIO_INTERRUPT_MASK;
    VIO_INTERRUPT_SET_T VIO_INTERRUPT_SET;
    VIO_INTERRUPT_CLEAR_T VIO_INTERRUPT_CLEAR;
} VIO_TOP_t;

typedef struct VIO_CB1_t{
    VIO_CB1_STATUS_T VIO_CB1_STATUS;
    VIO_CB1_CONTROL_T VIO_CB1_CONTROL;
    VIO_CB1_IMG_BASE_T VIO_CB1_IMG_BASE;
    VIO_CB1_IMG_WIDTH_T VIO_CB1_IMG_WIDTH;
    VIO_CB1_IMG_HEIGHT_T VIO_CB1_IMG_HEIGHT;
    VIO_CB1_IMG_STRIDE_T VIO_CB1_IMG_STRIDE;
}VIO_CB1_t;


typedef struct VIO_CB2_t{
    VIO_CB2_STATUS_T VIO_CB2_STATUS;
    VIO_CB2_CONTROL_T VIO_CB2_CONTROL;
    VIO_CB2_IMG_BASE_T VIO_CB2_IMG_BASE;
    VIO_CB2_IMG_WIDTH_T VIO_CB2_IMG_WIDTH;
    VIO_CB2_IMG_HEIGHT_T VIO_CB2_IMG_HEIGHT;
    VIO_CB2_IMG_STRIDE_T VIO_CB2_IMG_STRIDE;
}VIO_CB2_t;

typedef struct VIO_DET1_t {
    VIO_DET1_STATUS_T VIO_DET1_STATUS;
    VIO_DET1_CONTROL_T VIO_DET1_CONTROL;
    VIO_DET1_IMG_BASE_T VIO_DET1_IMG_BASE;
    VIO_DET1_IMG_WIDTH_T VIO_DET1_IMG_WIDTH;
    VIO_DET1_IMG_HEIGHT_T VIO_DET1_IMG_HEIGHT;
    VIO_DET1_IMG_STRIDE_T VIO_DET1_IMG_STRIDE;
    VIO_DET1_ROIMASK_BASE_T VIO_DET1_ROIMASK_BASE;
    VIO_DET1_SORTED_FPLIST_LENGTH_T  VIO_DET1_SORTED_FPLIST_LENGTH;
    VIO_DET1_SORTED_FPLIST_BASE_T VIO_DET1_SORTED_FPLIST_BASE;
    VIO_DET1_TOT_FNUMBER_T VIO_DET1_TOT_FNUMBER;
    VIO_DET1_EXP_FLIST_BASE_T VIO_DET1_EXP_FLIST_BASE;
    VIO_DET1_WRK_BUFA_BASE_T VIO_DET1_WRK_BUFA_BASE;
    VIO_DET1_WRK_BUFB_BASE_T VIO_DET1_WRK_BUFB_BASE;
} VIO_DET1_t;

typedef struct VIO_DET2_t {
    VIO_DET2_STATUS_T VIO_DET2_STATUS;
    VIO_DET2_CONTROL_T VIO_DET2_CONTROL;
    VIO_DET2_IMG_BASE_T VIO_DET2_IMG_BASE;
    VIO_DET2_IMG_WIDTH_T VIO_DET2_IMG_WIDTH;
    VIO_DET2_IMG_HEIGHT_T VIO_DET2_IMG_HEIGHT;
    VIO_DET2_IMG_STRIDE_T VIO_DET2_IMG_STRIDE;
    VIO_DET2_ROIMASK_BASE_T VIO_DET2_ROIMASK_BASE;
    VIO_DET2_SORTED_FPLIST_LENGTH_T  VIO_DET2_SORTED_FPLIST_LENGTH;
    VIO_DET2_SORTED_FPLIST_BASE_T VIO_DET2_SORTED_FPLIST_BASE;
    VIO_DET2_TOT_FNUMBER_T VIO_DET2_TOT_FNUMBER;
    VIO_DET2_EXP_FLIST_BASE_T VIO_DET2_EXP_FLIST_BASE;
    VIO_DET2_WRK_BUFA_BASE_T VIO_DET2_WRK_BUFA_BASE;
    VIO_DET2_WRK_BUFB_BASE_T VIO_DET2_WRK_BUFB_BASE;
} VIO_DET2_t;


typedef struct VIO_TRK_t {
  VIO_TRK_STATUS_T VIO_TRK_STATUS;
  VIO_TRK_CONTROL_T VIO_TRK_CONTROL;
  VIO_TRK_IMG_BASE_T VIO_TRK_IMG_BASE;
  VIO_TRK_IMG_WIDTH_T VIO_TRK_IMG_WIDTH;
  VIO_TRK_IMG_HEIGHT_T VIO_TRK_IMG_HEIGHT;
  VIO_TRK_IMG_STRIDE_T VIO_TRK_IMG_STRIDE;
  VIO_TRK_FPPREDLIST_BASE_T VIO_TRK_FPPREDLIST_BASE;
  VIO_TRK_FPPREDLIST_LENGTH_T VIO_TRK_FPPREDLIST_LENGTH;
  VIO_TRK_FPPATCHLIST_BASE_T VIO_TRK_FPPATCHLIST_BASE;
  VIO_TRK_FPPATCHLIST_LENGTH_T VIO_TRK_FPPATCHLIST_LENGTH;
  VIO_TRK_FPTRACKLIST_BASE_T VIO_TRK_FPTRACKLIST_BASE;
} VIO_TRK_t;

typedef struct VIO_EKF_t {
  VIO_EKF_STATUS_T VIO_EKF_STATUS;
  VIO_EKF_CONTROL_T VIO_EKF_CONTROL;
  VIO_EKF_MATMUL_COEF_A_T VIO_EKF_MATMUL_COEF_A;
  VIO_EKF_MATMUL_COEF_B_T VIO_EKF_MATMUL_COEF_B;
  VIO_EKF_MMUL_A_BASE_T VIO_EKF_MMUL_A_BASE;
  VIO_EKF_MMUL_A_DIM_T VIO_EKF_MMUL_A_DIM;
  VIO_EKF_MMUL_A_FORMAT_T VIO_EKF_MMUL_A_FORMAT;
  VIO_EKF_MMUL_B_BASE_T VIO_EKF_MMUL_B_BASE;
  VIO_EKF_MMUL_B_DIM_T VIO_EKF_MMUL_B_DIM;
  VIO_EKF_MMUL_B_FORMAT_T VIO_EKF_MMUL_B_FORMAT;
  VIO_EKF_MMUL_C_BASE_T VIO_EKF_MMUL_C_BASE;
  VIO_EKF_MMUL_C_DIM_T VIO_EKF_MMUL_C_DIM;
  VIO_EKF_MMUL_C_FORMAT_T VIO_EKF_MMUL_C_FORMAT;
  VIO_EKF_MMUL_Y_BASE_T VIO_EKF_MMUL_Y_BASE;
  VIO_EKF_CHOL_IN_BASE_T VIO_EKF_CHOL_IN_BASE;
  VIO_EKF_CHOL_OUT_BASE_T VIO_EKF_CHOL_OUT_BASE;
  VIO_EKF_CHOL_DIM_T VIO_EKF_CHOL_DIM;
  VIO_EKF_CHOL_FORMAT_T VIO_EKF_CHOL_FORMAT;
  VIO_EKF_MSLV_A_BASE_T  VIO_EKF_MSLV_A_BASE;
  VIO_EKF_MSLV_A_DIM_T VIO_EKF_MSLV_A_DIM;
  VIO_EKF_MSLV_A_FORMAT_T VIO_EKF_MSLV_A_FORMAT;
  VIO_EKF_MSLV_X_BASE_T VIO_EKF_MSLV_X_BASE;
  VIO_EKF_MSLV_X_DIM_T VIO_EKF_MSLV_X_DIM;
  VIO_EKF_MSLV_X_FORMAT_T VIO_EKF_MSLV_X_FORMAT;
  VIO_EKF_MSLV_B_BASE_T VIO_EKF_MSLV_B_BASE;
  VIO_EKF_MSLV_B_DIM_T VIO_EKF_MSLV_B_DIM;
  VIO_EKF_MSLV_B_FORMAT_T VIO_EKF_MSLV_B_FORMAT;
  VIO_EKF_ERROR_STATUS_T VIO_EKF_ERROR_STATUS;
  VIO_EKF_ERROR_STATUS_MASK_T VIO_EKF_ERROR_STATUS_MASK;
  VIO_EKF_CHOL_ERROR_T VIO_EKF_CHOL_ERROR;
} VIO_EKF_t;


typedef struct {
    union {
        uint8_t top_buf[VIO_TOP_SIZE];
        VIO_TOP_t top;
    };
    union {
        uint8_t det1_buf[VIO_DET1_SIZE];
        VIO_DET1_t det1;
    };
    union {
        uint8_t det2_buf[VIO_DET2_SIZE];
        VIO_DET2_t det2;
    };
    union {
        uint8_t trk_buf[VIO_TRK_SIZE];
        VIO_TRK_t trk;
    };
} VIO_REG_t;


#define VIO_TOP_STATUS_OFFSET      (VIO_TOP_MMOFFSET + 0x00)
#define VIO_TOP_CONTROL_OFFSET     (VIO_TOP_MMOFFSET + 0x04)
#define VIO_INTERRUPT_FLAG_OFFSET  (VIO_TOP_MMOFFSET + 0x08)
#define VIO_INTERRUPT_MASK_OFFSET  (VIO_TOP_MMOFFSET + 0x0C)
#define VIO_INTERRUPT_SET_OFFSET   (VIO_TOP_MMOFFSET + 0x10)
#define VIO_INTERRUPT_CLEAR_OFFSET (VIO_TOP_MMOFFSET + 0x14)

#define VIO_CB1_STATUS_OFFSET               (VIO_CB1_MMOFFSET + 0x00)
#define VIO_CB1_CONTROL_OFFSET              (VIO_CB1_MMOFFSET + 0x04)
#define VIO_CB1_IMG_BASE_OFFSET             (VIO_CB1_MMOFFSET + 0x08)
#define VIO_CB1_IMG_WIDTH_OFFSET            (VIO_CB1_MMOFFSET + 0x0C)
#define VIO_CB1_IMG_HEIGHT_OFFSET           (VIO_CB1_MMOFFSET + 0x10)
#define VIO_CB1_IMG_STRIDE_OFFSET           (VIO_CB1_MMOFFSET + 0x14)

#define VIO_CB2_STATUS_OFFSET               (VIO_CB2_MMOFFSET + 0x00)
#define VIO_CB2_CONTROL_OFFSET              (VIO_CB2_MMOFFSET + 0x04)
#define VIO_CB2_IMG_BASE_OFFSET             (VIO_CB2_MMOFFSET + 0x08)
#define VIO_CB2_IMG_WIDTH_OFFSET            (VIO_CB2_MMOFFSET + 0x0C)
#define VIO_CB2_IMG_HEIGHT_OFFSET           (VIO_CB2_MMOFFSET + 0x10)
#define VIO_CB2_IMG_STRIDE_OFFSET           (VIO_CB2_MMOFFSET + 0x14)

#define VIO_DET1_STATUS_OFFSET               (VIO_DET1_MMOFFSET + 0x00)
#define VIO_DET1_CONTROL_OFFSET              (VIO_DET1_MMOFFSET + 0x04)
#define VIO_DET1_IMG_BASE_OFFSET             (VIO_DET1_MMOFFSET + 0x08)
#define VIO_DET1_IMG_WIDTH_OFFSET            (VIO_DET1_MMOFFSET + 0x0C)
#define VIO_DET1_IMG_HEIGHT_OFFSET           (VIO_DET1_MMOFFSET + 0x10)
#define VIO_DET1_IMG_STRIDE_OFFSET           (VIO_DET1_MMOFFSET + 0x14)
#define VIO_DET1_ROIMASK_BASE_OFFSET         (VIO_DET1_MMOFFSET + 0x18)
#define VIO_DET1_SORTED_FPLIST_LENGTH_OFFSET (VIO_DET1_MMOFFSET + 0x1C)
#define VIO_DET1_SORTED_FPLIST_BASE_OFFSET   (VIO_DET1_MMOFFSET + 0x20)
#define VIO_DET1_TOT_FNUMBER_OFFSET          (VIO_DET1_MMOFFSET + 0x24)
#define VIO_DET1_EXP_FLIST_BASE_OFFSET       (VIO_DET1_MMOFFSET + 0x28)
#define VIO_DET1_WRK_BUFA_BASE_OFFSET        (VIO_DET1_MMOFFSET + 0x2C)
#define VIO_DET1_WRK_BUFB_BASE_OFFSET        (VIO_DET1_MMOFFSET + 0x30)

#define VIO_DET2_STATUS_OFFSET               (VIO_DET2_MMOFFSET + 0x00)
#define VIO_DET2_CONTROL_OFFSET              (VIO_DET2_MMOFFSET + 0x04)
#define VIO_DET2_IMG_BASE_OFFSET             (VIO_DET2_MMOFFSET + 0x08)
#define VIO_DET2_IMG_WIDTH_OFFSET            (VIO_DET2_MMOFFSET + 0x0C)
#define VIO_DET2_IMG_HEIGHT_OFFSET           (VIO_DET2_MMOFFSET + 0x10)
#define VIO_DET2_IMG_STRIDE_OFFSET           (VIO_DET2_MMOFFSET + 0x14)
#define VIO_DET2_ROIMASK_BASE_OFFSET         (VIO_DET2_MMOFFSET + 0x18)
#define VIO_DET2_SORTED_FPLIST_LENGTH_OFFSET (VIO_DET2_MMOFFSET + 0x1C)
#define VIO_DET2_SORTED_FPLIST_BASE_OFFSET   (VIO_DET2_MMOFFSET + 0x20)
#define VIO_DET2_TOT_FNUMBER_OFFSET          (VIO_DET2_MMOFFSET + 0x24)
#define VIO_DET2_EXP_FLIST_BASE_OFFSET       (VIO_DET2_MMOFFSET + 0x28)
#define VIO_DET2_WRK_BUFA_BASE_OFFSET        (VIO_DET2_MMOFFSET + 0x2C)
#define VIO_DET2_WRK_BUFB_BASE_OFFSET        (VIO_DET2_MMOFFSET + 0x30)


#define VIO_TRK_STATUS_OFFSET                (VIO_TRK_MMOFFSET + 0X00)
#define VIO_TRK_CONTROL_OFFSET               (VIO_TRK_MMOFFSET + 0X04)
#define VIO_TRK_IMG_BASE_OFFSET              (VIO_TRK_MMOFFSET + 0X08)
#define VIO_TRK_IMG_WIDTH_OFFSET             (VIO_TRK_MMOFFSET + 0X0C)
#define VIO_TRK_IMG_HEIGHT_OFFSET            (VIO_TRK_MMOFFSET + 0X10)
#define VIO_TRK_IMG_STRIDE_OFFSET            (VIO_TRK_MMOFFSET + 0X14)
#define VIO_TRK_FPPREDLIST_BASE_OFFSET       (VIO_TRK_MMOFFSET + 0X18)
#define VIO_TRK_FPPREDLIST_LENGTH_OFFSET     (VIO_TRK_MMOFFSET + 0X1C)
#define VIO_TRK_FPPATCHLIST_BASE_OFFSET      (VIO_TRK_MMOFFSET + 0X20)
#define VIO_TRK_FPPATCHLIST_LENGTH_OFFSET    (VIO_TRK_MMOFFSET + 0X24)
#define VIO_TRK_FPTRACKLIST_BASE_OFFSET      (VIO_TRK_MMOFFSET + 0X28)

#define VIO_EKF_STATUS_OFFSET		         (VIO_EKF_MMOFFSET + 0x00)
#define VIO_EKF_CONTROL_OFFSET		         (VIO_EKF_MMOFFSET + 0x04)
#define VIO_EKF_MATMUL_COEF_A_OFFSET	     (VIO_EKF_MMOFFSET + 0x08)
#define VIO_EKF_MATMUL_COEF_B_OFFSET	     (VIO_EKF_MMOFFSET + 0x0C)
#define VIO_EKF_MMUL_A_BASE_OFFSET           (VIO_EKF_MMOFFSET + 0x10)
#define VIO_EKF_MMUL_A_DIM_OFFSET            (VIO_EKF_MMOFFSET + 0x14)
#define VIO_EKF_MMUL_A_FORMAT_OFFSET         (VIO_EKF_MMOFFSET + 0x18)
#define VIO_EKF_MMUL_B_BASE_OFFSET           (VIO_EKF_MMOFFSET + 0x1C)
#define VIO_EKF_MMUL_B_DIM_OFFSET            (VIO_EKF_MMOFFSET + 0x20)
#define VIO_EKF_MMUL_B_FORMAT_OFFSET         (VIO_EKF_MMOFFSET + 0x24)
#define VIO_EKF_MMUL_C_BASE_OFFSET           (VIO_EKF_MMOFFSET + 0x28)
#define VIO_EKF_MMUL_C_DIM_OFFSET            (VIO_EKF_MMOFFSET + 0x2C)
#define VIO_EKF_MMUL_C_FORMAT_OFFSET         (VIO_EKF_MMOFFSET + 0x30)
#define VIO_EKF_MMUL_Y_BASE_OFFSET           (VIO_EKF_MMOFFSET + 0x34)
#define VIO_EKF_CHOL_IN_BASE_OFFSET          (VIO_EKF_MMOFFSET + 0x38)
#define VIO_EKF_CHOL_OUT_BASE_OFFSET         (VIO_EKF_MMOFFSET + 0x3C)
#define VIO_EKF_CHOL_DIM_OFFSET              (VIO_EKF_MMOFFSET + 0x40)
#define VIO_EKF_CHOL_FORMAT_OFFSET           (VIO_EKF_MMOFFSET + 0x44)
#define VIO_EKF_MSLV_A_BASE_OFFSET           (VIO_EKF_MMOFFSET + 0x48)
#define VIO_EKF_MSLV_A_DIM_OFFSET            (VIO_EKF_MMOFFSET + 0x4C)
#define VIO_EKF_MSLV_A_FORMAT_OFFSET         (VIO_EKF_MMOFFSET + 0x50)
#define VIO_EKF_MSLV_X_BASE_OFFSET           (VIO_EKF_MMOFFSET + 0x54)
#define VIO_EKF_MSLV_X_DIM_OFFSET            (VIO_EKF_MMOFFSET + 0x58)
#define VIO_EKF_MSLV_X_FORMAT_OFFSET         (VIO_EKF_MMOFFSET + 0x5C)
#define VIO_EKF_MSLV_B_BASE_OFFSET           (VIO_EKF_MMOFFSET + 0x60)
#define VIO_EKF_MSLV_B_DIM_OFFSET            (VIO_EKF_MMOFFSET + 0x64)
#define VIO_EKF_MSLV_B_FORMAT_OFFSET         (VIO_EKF_MMOFFSET + 0x68)
#define VIO_EKF_ERROR_STATUS_OFFSET          (VIO_EKF_MMOFFSET + 0x6C)
#define VIO_EKF_ERROR_STATUS_MASK_OFFSET     (VIO_EKF_MMOFFSET + 0x70)
#define VIO_EKF_CHOL_ERROR_OFFSET            (VIO_EKF_MMOFFSET + 0x74)

#define VIO_ISRC1_CTRL_OFFSET                (VIO_ISRC_MMOFFSET + 0xE0)
#define VIO_ISRC2_CTRL_OFFSET                (VIO_ISRC_MMOFFSET + 0xF0)


#if 1
#define VIO_IMG1_MEM_BA                         0x00000
#define VIO_IMG2_MEM_BA                         0x04B00
#else
#define VIO_DET1_WRK_BUFA_MEM_BA                0x00000
#define VIO_DET1_WRK_BUFB_MEM_BA                0x04B00
#endif

#define VIO_TRK_FPPREDLIST_MEM_BA               0x09600
#define VIO_TRK_FPPATCHLIST_MEM_BA              0x09680
#define VIO_TRK_FPTRACKLIST_MEM_BA              0x09C80

#define VIO_DET1_ROIMASK_MEM_BA                 0x09D00
#define VIO_DET1_SORTEDFPLIST_MEM_BA            0x09D80

#if 1
#define VIO_DET1_WRK_BUFA_MEM_BA                0x0A580
#define VIO_DET1_WRK_BUFB_MEM_BA                0x0AD80
#else
#define VIO_IMG1_MEM_BA                         0x0A580
#define VIO_IMG2_MEM_BA                         0x0AD80
#endif

#define VIO_DET2_ROIMASK_MEM_BA                 0x0B580
#define VIO_DET2_SORTEDFPLIST_MEM_BA            0x0B600

#define VIO_DET2_WRK_BUFA_MEM_BA                0x0BE00
#define VIO_DET2_WRK_BUFB_MEM_BA                0x0C600

#define VIO_EKF_CHOL_IN_MEM_BA                  0x0CE00
#define VIO_EKF_CHOL_OUT_MEM_BA                 0x0CE00
#define VIO_EKF_MSLV_A_MEM_BA                   0x0CE00
#define VIO_EKF_MSLV_B_MEM_BA                   0x10E00
#define VIO_EKF_MSLV_X_MEM_BA                   0x14E00

#define VIO_EKF_MMUL_A_MEM_BA                   0x10E00 
#define VIO_EKF_MMUL_B_MEM_BA                   0x14E00 
#define VIO_EKF_MMUL_C_MEM_BA                   0x18E00 
#define VIO_EKF_MMUL_Y_MEM_BA                   0x18E00 

#define VIO_CB1_MEM_BA                          0x1CE00
#define VIO_CB2_MEM_BA                          0x1D300

#define VIO_DET1_EXP_FLIST_MEM_BA               0x1D800
#define VIO_DET1_EXP_FDESLIST_MEM_BA            0x1E000
#define VIO_DET2_EXP_FLIST_MEM_BA               0x1E000
#define VIO_DET2_EXP_FDESLIST_MEM_BA            0x1E800

#define VIO_MIPI1_BUF                           0x00000000//isrc
#define VIO_MIPI2_BUF                           0x00200000//isrc

#if 1
#define VIO_IMG1_BUF                            0x00000000
#define VIO_IMG2_BUF                            0x0004B000
#else
#define VIO_DET1_WRK_BUFA_BUF                   0x00000000
#define VIO_DET1_WRK_BUFB_BUF                   0x0004B000
#endif 

#define VIO_TRK_FPPREDLIST_BUF                  0x00096000
#define VIO_TRK_FPPATCHLIST_BUF                 0x00096800
#define VIO_TRK_FPTRACKLIST_BUF                 0x0009C800

#define VIO_DET1_ROIMASK_BUF                    0x0009D000
#define VIO_DET1_SORTEDFPLIST_BUF               0x0009D800

#if 1
#define VIO_DET1_WRK_BUFA_BUF                   0x000A5800
#define VIO_DET1_WRK_BUFB_BUF                   0x000AD800
#else
#define VIO_IMG1_BUF                            0x000A5800
#define VIO_IMG2_BUF                            0x000AD800
#endif

#define VIO_DET2_ROIMASK_BUF                    0x000B5800
#define VIO_DET2_SORTEDFPLIST_BUF               0x000B6000
#define VIO_DET2_WRK_BUFA_BUF                   0x000BE000
#define VIO_DET2_WRK_BUFB_BUF                   0x000C6000

#define VIO_EKF_CHOL_IN_BUF                     0x000CE000
#define VIO_EKF_CHOL_OUT_BUF                    0x000CE000
#define VIO_EKF_MSLV_A_BUF                      0x000CE000
#define VIO_EKF_MSLV_X_BUF                      0x0014E000
#define VIO_EKF_MSLV_B_BUF                      0x0010E000

//#define VIO_EKF_MMUL_A_BUF                      0x0010E000 
#define VIO_EKF_MMUL_A_BUF                      0x0000E000 
//#define VIO_EKF_MMUL_B_BUF                      0x0014E000
#define VIO_EKF_MMUL_B_BUF                      0x0010E000 
#define VIO_EKF_MMUL_C_BUF                      0x0018E000 
#define VIO_EKF_MMUL_Y_BUF                      0x0018E000 

#define VIO_CB1_BUF                             0x001CE000
#define VIO_CB2_BUF                             0x001D3000

#define VIO_DET1_EXP_FLIST_BUF                  0x001D8000
#define VIO_DET1_EXP_FDESLIST_BUF               0x001E0000
#define VIO_DET2_EXP_FLIST_BUF                  0x001E0000
#define VIO_DET2_EXP_FDESLIST_BUF               0x001E8000

#define VIO_SRAM_END                            0x00200000
#define VIO_SRAM_SIZE                           (VIO_SRAM_END - VIO_IMG1_BUF)


#if 1
#define VIO_IMG1_BUFSIZE	                (VIO_IMG2_BUF		      - VIO_IMG1_BUF)
#define VIO_IMG2_BUFSIZE	                (VIO_TRK_FPPREDLIST_BUF	      - VIO_IMG2_BUF)
#else
#define VIO_DET1_WRKA_BUFA_BUFSIZE	        (VIO_DET1_WRK_BUFB_BUF	      - VIO_DET1_WRK_BUFA_BUF)
#define VIO_DET1_WRKA_BUFB_BUFSIZE	        (VIO_TRK_FPPREDLIST_BUF	      - VIO_DET1_WRK_BUFB_BUF)
#endif


#define VIO_TRK_FPPREDLIST_BUFSIZE	        (VIO_TRK_FPPATCHLIST_BUF      - VIO_TRK_FPPREDLIST_BUF)
#define VIO_TRK_FPPATCHLIST_BUFSIZE	        (VIO_TRK_FPTRACKLIST_BUF      - VIO_TRK_FPPATCHLIST_BUF)
#define VIO_TRK_FPTRACKLIST_BUFSIZE	        (VIO_DET1_ROIMASK_BUF	      - VIO_TRK_FPTRACKLIST_BUF)
#define VIO_DET1_ROIMASK_BUFSIZE	        (VIO_DET1_SORTEDFPLIST_BUF    - VIO_DET1_ROIMASK_BUF)

#if 1 
#define VIO_DET1_SORTEDFPLIST_BUFSIZE           (VIO_DET1_WRK_BUFA_BUF        - VIO_DET1_SORTEDFPLIST_BUF)
#define VIO_DET1_WRKA_BUFA_BUFSIZE              (VIO_DET1_WRK_BUFB_BUF        - VIO_DET1_WRK_BUFA_BUF)
#define VIO_DET1_WRKB_BUFB_BUFSIZE              (VIO_DET2_ROIMASK_BUF         - VIO_DET1_WRK_BUFB_BUF)
#else
#define VIO_DET1_SORTEDFPLIST_BUFSIZE           (VIO_IMG1_BUF                 - VIO_DET1_SORTEDFPLIST_BUF)
#define VIO_IMG1_BUFSIZE	                (VIO_IMG2_BUF		      - VIO_IMG1_BUF)
#define VIO_IMG2_BUFSIZE	                (VIO_DET2_ROIMASK_BUF	      - VIO_IMG2_BUF)
#endif

#define VIO_DET2_ROIMASK_BUFSIZE                (VIO_DET2_SORTEDFPLIST_BUF    - VIO_DET2_ROIMASK_BUF) 
#define VIO_DET2_SORTEDFPLIST_BUFSIZE           (VIO_DET2_WRK_BUFA_BUF        - VIO_DET2_SORTEDFPLIST_BUF)
#define VIO_DET2_WRKA_BUFA_BUFSIZE              (VIO_DET2_WRK_BUFB_BUF        - VIO_DET2_WRK_BUFA_BUF)
#define VIO_DET2_WRKB_BUFB_BUFSIZE              (VIO_EKF_CHOL_IN_BUF          - VIO_DET2_WRK_BUFB_BUF)
#define VIO_EKF_CHOL_IN_BUFSIZE	                (VIO_EKF_MMUL_A_BUF	      - VIO_EKF_CHOL_IN_BUF)
#define VIO_EKF_MMUL_A_BUFSIZE	                (VIO_EKF_MMUL_B_BUF	      - VIO_EKF_MMUL_A_BUF)
#define VIO_EKF_MMUL_B_BUFSIZE	                (VIO_EKF_MMUL_C_BUF	      - VIO_EKF_MMUL_B_BUF)
#define VIO_EKF_MMUL_C_BUFSIZE	                (VIO_CB1_BUF	              - VIO_EKF_MMUL_C_BUF)
#define VIO_CB1_BUFSIZE                         (VIO_CB2_BUF	              - VIO_CB1_BUF)
#define VIO_CB2_BUFSIZE                         (VIO_DET1_EXP_FPLIST_BUF      - VIO_CB2_BUF)
#define VIO_DET1_EXP_FLIST_BUFSIZE              (VIO_DET1_EXP_FPDESLIST_BUF   - VIO_DET1_EXP_FPLIST_BUF)
#define VIO_DET1_EXP_FDESLIST_BUFSIZE           (VIO_DET2_EXP_FPLIST_BUF      - VIO_DET1_EXP_FPDESLIST_BUF)
#define VIO_DET2_EXP_FLIST_BUFSIZE              (VIO_DET2_EXP_FPDESLIST_BUF   - VIO_DET2_EXP_FPLIST_BUF)
#define VIO_DET2_EXP_FDESLIST_BUFSIZE           (VIO_SRAM_END                 - VIO_DET1_EXP_FPDESLIST_BUF)

