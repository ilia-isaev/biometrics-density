#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define LIB378_TYPE_CODE_CORE_DELTA 0x0002

#define LIB378_WAR_NO_EXT_DATA_BLK 4
#define LIB378_WAR_TOO_SHORT       2

#define LIB378_ERR_TOO_SHORT      -2
#define LIB378_ERR_NO_VIEW        -3
#define LIB378_ERR_NO_HAVE_DELTA_CORE -5

//--- Core Delata ----------------------------
typedef struct {
	unsigned char has_angle;
	unsigned char num;
	void         *buf;
} incits378_core_delta_summary_s;

typedef struct {
	incits378_core_delta_summary_s core;
	incits378_core_delta_summary_s delta;
} incits378_core_delta_info_s;

typedef struct {
	unsigned short x;
	unsigned short y;
	unsigned char  a1;
	unsigned char  a2;
	unsigned char  a3;
} incits378_core_delta_s;
//--- ------------------------------------ ---

typedef struct {
	unsigned short x;
	unsigned short y;
} incits378_minutiae_s;

//--- Dimensions -----------------------------
typedef struct {
	unsigned short w;
	unsigned short h;
} incits378_image_sz_s;
//--- ------------------------------------ ---

typedef struct {
	short  num;  /**< number of elemet in block */
	short  sz;   /**< size of block in byte */
	void  *buf;  /**< pointer to start of block */
} incits378_block_s;

typedef struct {
	int               war;  /**< warning code */
	int               err;  /**< error code */
	incits378_block_s blk;  /**< data block */
} lib378_blk_s;


lib378_blk_s
incits378_dimension_block    (void *buffer_in);

lib378_blk_s
incits378_minutiae_block     (void *buffer_in);

lib378_blk_s
incits378_extended_data_block(void *buffer_in);


incits378_image_sz_s
incits378_image_size(incits378_block_s blk);

incits378_minutiae_s
incits378_minutiae(incits378_block_s blk, int mnum);

unsigned short
incits378_extended_area_type_code(incits378_block_s blk, const int area_num);

incits378_block_s
incits378_core_delta_block(incits378_block_s blk);

incits378_core_delta_info_s
incits378_core_delta_info(incits378_block_s blk);

incits378_core_delta_s
incits378_core(incits378_core_delta_summary_s summary, int num);

incits378_core_delta_s
incits378_delta(incits378_core_delta_summary_s summary, int num);


#ifdef __cplusplus
}
#endif

