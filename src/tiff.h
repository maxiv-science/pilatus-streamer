#ifndef TIFF_H
#define TIFF_H

#include <stdint.h>

// http://paulbourke.net/dataformats/tiff/tiff_summary.pdf

typedef struct 
{
    int16_t identifier;
    int16_t version;
    int32_t ifd_offset;
} TiffHeader;

typedef struct 
{
    uint16_t tag_id;
    uint16_t data_type;    
    uint32_t data_count;   
    uint32_t data_offset;
} TifTag;

typedef struct
{
    uint16_t bits_per_sample;
    uint32_t width;
    uint32_t height;
    uint32_t strip_offsets;
    uint32_t strip_byte_counts;
} TifInfo;

// Tif tag ids
#define IMAGE_WIDTH 256
#define IMAGE_HEIGHT 257
#define BITS_PER_SAMPLE 258
#define IMAGE_DESCRIPTION 270
#define STRIP_OFFSETS 273
#define STRIP_BYTE_COUNTS 279

void parse_tif(FILE* fp, TifInfo* info);
void read_tif_image(FILE* fp, const TifInfo* info, void* buffer);

#endif // TIFF_H
