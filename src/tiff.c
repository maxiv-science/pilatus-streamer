#include <stdio.h>
#include <stdlib.h>
#include "tiff.h"

#ifndef NDEBUG
#  define debug_print printf
#else
#  define debug_print(...)
#endif

void parse_tif(FILE* fp, TifInfo* info)
{
    TiffHeader header;
    if (fread(&header, sizeof(TiffHeader), 1, fp) != 1) {
        printf("Problem reading header\n");
    }
    debug_print("idf offset %d\n", header.ifd_offset);
    
    fseek(fp, header.ifd_offset, SEEK_SET);
    uint16_t tag_count;
    fread(&tag_count, sizeof(uint16_t), 1, fp);
    debug_print("tag count %d\n", tag_count);
    
    TifTag* tags = (TifTag*)malloc(tag_count*sizeof(TifTag));
    fread(tags, sizeof(TifTag), tag_count, fp);
    for (int i=0; i<tag_count; i++) {
        TifTag* tag = &tags[i];
        debug_print("tag id %d | data type %d | data count %d | value %d\n", 
                    tag->tag_id, tag->data_type, tag->data_count, tag->data_offset);
        switch(tag->tag_id) {
            case IMAGE_WIDTH:
                info->width = tag->data_offset;
                break;
                
            case IMAGE_HEIGHT:
                info->height = tag->data_offset;
                break;
                
            case BITS_PER_SAMPLE:
                info->bits_per_sample = tag->data_offset;
                break;
                
            case IMAGE_DESCRIPTION:
                fseek(fp, tag->data_offset, SEEK_SET);
                char desc[1024];
                fread(desc, tag->data_count, 1, fp);
                debug_print("### Image description: ###\n%s\n", desc);
                break;
                
            case STRIP_OFFSETS:
                info->strip_offsets = tag->data_offset;
                break;
                
            case STRIP_BYTE_COUNTS:
                info->strip_byte_counts = tag->data_offset;
                break;
        }
    }
    free(tags);
}

void read_tif_image(FILE* fp, const TifInfo* info, void* buffer)
{
    if (fseek(fp, info->strip_offsets, SEEK_SET) !=0) {
        printf("fseek to image location failed!\n");
    }
    if (fread(buffer, info->strip_byte_counts, 1, fp) != 1) {
        printf("Problem reading image data\n");
    }
}
