#include "lib378.h"

#define SZ_DIM_BLK 10
#define IPOS_IMG_WIDTH          0
#define IPOS_IMG_HEIGHT         2
#define IPOS_TYPE_CODE_EXT_AREA 0
#define IPOS_EXT_AREA_LEN       2
#define IPOS_TOTAL_RECORD_LEN   8
#define IPOS_NUM_FINGER_VIEWS  24
#define IPOS_NUM_OF_MINUTIAE   29
#define IPOS_MINUTIAE_BLOCK    30
#define IPOS_X_MINUTIAE         0
#define IPOS_Y_MINUTIAE         2

#define GET_UCHAR_VAL(buffer,ipos) ((buffer)[(ipos)])
#define GET_USHORT_VAL(buffer,ipos) (((unsigned short)((buffer)[(ipos)])<<8) + (buffer)[(ipos)+1])


lib378_blk_s
incits378_dimension_block(void *buffer_in)
{
	unsigned char *buffer = buffer_in;

	const unsigned short total_record_len_small = GET_USHORT_VAL(buffer, IPOS_TOTAL_RECORD_LEN);

	const unsigned short    add_4_byte = total_record_len_small == 0 ? 4 : 0;
	const incits378_block_s blk        = (incits378_block_s){.sz=SZ_DIM_BLK,.buf=buffer+16+add_4_byte};

	if (total_record_len_small == 32)
		return (lib378_blk_s){
			.war=LIB378_WAR_TOO_SHORT,
			.blk=blk
		};

	return (lib378_blk_s){.blk=blk};
}

lib378_blk_s
incits378_minutiae_block(void *buffer_in)
{
	unsigned char *buffer = buffer_in;

	const unsigned short total_record_len_small = GET_USHORT_VAL(buffer, IPOS_TOTAL_RECORD_LEN);

	if (total_record_len_small == 32)
		return (lib378_blk_s){.err=LIB378_ERR_TOO_SHORT};

	const unsigned short add_4_byte = total_record_len_small == 0 ? 4 : 0;

	unsigned char num_finger_views = buffer[IPOS_NUM_FINGER_VIEWS+add_4_byte];
	if (num_finger_views == 0)
		return (lib378_blk_s){.err=LIB378_ERR_NO_VIEW};

	const unsigned char num_of_minutiae = buffer[IPOS_NUM_OF_MINUTIAE+add_4_byte];

	return (lib378_blk_s){
		.blk=(incits378_block_s){
			.num=num_of_minutiae,
			.sz =num_of_minutiae*6,
			.buf=buffer + IPOS_MINUTIAE_BLOCK+add_4_byte
		}
	};
}

lib378_blk_s
incits378_extended_data_block(void *buffer_in)
{
	unsigned char *buffer = buffer_in;

	const unsigned short total_record_len_small = GET_USHORT_VAL(buffer, IPOS_TOTAL_RECORD_LEN);

	if (total_record_len_small == 32)
		return (lib378_blk_s){.err=LIB378_WAR_TOO_SHORT};

	const unsigned short add_4_byte = total_record_len_small == 0 ? 4 : 0;

	unsigned char num_finger_views = buffer[IPOS_NUM_FINGER_VIEWS+add_4_byte];
	if (num_finger_views == 0)
		return (lib378_blk_s){.err=LIB378_ERR_NO_VIEW};

	const unsigned char num_of_minutiae = buffer[IPOS_NUM_OF_MINUTIAE+add_4_byte];

	const int ipos_ext_data_block_len = IPOS_MINUTIAE_BLOCK+add_4_byte+num_of_minutiae*6;

	const unsigned short extended_data_block_len = GET_USHORT_VAL(buffer, ipos_ext_data_block_len);

	if (extended_data_block_len == 0)
		return (lib378_blk_s){.war=LIB378_WAR_NO_EXT_DATA_BLK};

	unsigned char *buf_ext_data_blk   = buffer+ipos_ext_data_block_len+2;
	unsigned char *buf_area_ext_data  = buf_ext_data_blk;
	int            num_ext_data_block = 0;
	for (unsigned short i = 0; i < extended_data_block_len; )
	{
		const unsigned short sz_ext_data_area = GET_USHORT_VAL(buf_area_ext_data, 2);
		if (sz_ext_data_area > 0 && sz_ext_data_area < extended_data_block_len)
		{
			num_ext_data_block++;
			buf_area_ext_data += sz_ext_data_area;
		}
		else
			break;
	}

	return (lib378_blk_s){
		.blk=(incits378_block_s){
			.num = num_ext_data_block,
			.sz  = extended_data_block_len,
			.buf = buf_ext_data_blk
		}
	};
}

incits378_image_sz_s
incits378_image_size(incits378_block_s blk)
{
	const unsigned char *const buffer = blk.buf;
	return (incits378_image_sz_s){
		.w = GET_USHORT_VAL(buffer,IPOS_IMG_WIDTH),
		.h = GET_USHORT_VAL(buffer,IPOS_IMG_HEIGHT)
	};
}

incits378_minutiae_s
incits378_minutiae(incits378_block_s blk, int mnum)
{
	const unsigned char *const buffer = blk.buf + 6*mnum;

	return (incits378_minutiae_s) {
		.x = GET_USHORT_VAL(buffer, IPOS_X_MINUTIAE) & 0x3FFF,
		.y = GET_USHORT_VAL(buffer, IPOS_Y_MINUTIAE) & 0x3FFF
	};
}

unsigned short
incits378_extended_area_type_code(incits378_block_s blk, const int area_num)
{
	unsigned char *area = blk.buf;
	for (unsigned short i = 0, num = 1; i < blk.sz; )
	{
		if (num == area_num)
			return GET_USHORT_VAL(area, 0);

		const unsigned short sz_area = GET_USHORT_VAL(area, 2);
		area += sz_area;
		i    += sz_area;
		num++;
	}

	return 0;
}

incits378_block_s
incits378_core_delta_block(incits378_block_s blk)
{
	unsigned short tc   = 0;
	unsigned short sz   = 0;
	unsigned char *area = blk.buf;

	for (unsigned short i = 0; i < blk.sz; )
	{
		tc = GET_USHORT_VAL(area, IPOS_TYPE_CODE_EXT_AREA);
		sz = GET_USHORT_VAL(area, IPOS_EXT_AREA_LEN);

		if (LIB378_TYPE_CODE_CORE_DELTA == tc)
			break;

		area += sz;
		i    += sz;
	}

	if (LIB378_TYPE_CODE_CORE_DELTA != tc)
		return (incits378_block_s){0};

	return (incits378_block_s){
		.sz =sz  -4,
		.buf=area+4
	};
}

incits378_core_delta_info_s
incits378_core_delta_info(incits378_block_s blk)
{
	unsigned char *buffer = blk.buf;

	incits378_core_delta_info_s cd = {0};

	unsigned char cit  = GET_UCHAR_VAL(buffer,0);
	unsigned char cang =(cit>>6)&0x03;
	unsigned char cnum = cit    &0x0F;

	cd.core = (incits378_core_delta_summary_s){
		.has_angle=cang,
		.num      =cnum,
		.buf      =buffer+1
	};

	const int csz = cang ? 1+cnum*5 : 1+cnum*4;
	buffer += csz;
	unsigned char dit  = GET_UCHAR_VAL(buffer,0);
	unsigned char dang = (dit>>6)&0x03;
	unsigned char dnum =  dit    &0x3F;

	cd.delta = (incits378_core_delta_summary_s){
		.has_angle=dang,
		.num      =dnum,
		.buf      =buffer+1
	};

	return cd;
}

incits378_core_delta_s
incits378_core(incits378_core_delta_summary_s summary, int num)
{
	unsigned char *buffer = summary.buf;

	if (num > 1)
		buffer += summary.has_angle ? 5*(num-1) : 4*(num-1);

	return (incits378_core_delta_s){
		.x  = GET_USHORT_VAL(buffer, 0),
		.y  = GET_USHORT_VAL(buffer, 2),
		.a1 = GET_UCHAR_VAL (buffer, 4)
	};
}

incits378_core_delta_s
incits378_delta(incits378_core_delta_summary_s summary, int num)
{
	unsigned char *buffer = summary.buf;

	if (num > 1)
		buffer += summary.has_angle ? 7*(num-1) : 4*(num-1);

	return (incits378_core_delta_s){
		.x  = GET_USHORT_VAL(buffer, 0),
		.y  = GET_USHORT_VAL(buffer, 2),
		.a1 = GET_UCHAR_VAL (buffer, 4),
		.a2 = GET_UCHAR_VAL (buffer, 5),
		.a3 = GET_UCHAR_VAL (buffer, 6)
	};
}

