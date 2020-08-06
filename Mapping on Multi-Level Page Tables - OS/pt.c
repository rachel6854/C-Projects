
#include "os.h"
#include <err.h>
uint64_t mask_last_on=1;
uint64_t mask_9_bits=0x1FF0000000000;
uint64_t mask_last_off=0xffffffffffffe;

int is_valid(uint64_t result)
{
	if (result&1)
		{
		return 1;
		}
	return 0;
}
void page_table_update(uint64_t pt, uint64_t vpn, uint64_t ppn)
{
	uint64_t index,*result=phys_to_virt(pt<<12);
	unsigned int i;
	vpn=vpn<<12;
	if (ppn != NO_MAPPING)
	{
		for(i=0;i<4;i++)
		{
		index=((mask_9_bits>>(9*(i+1)))&vpn)>>(39-(9*i));
		if (!is_valid(result[index]))
			{
			result[index]=alloc_page_frame();
			result[index]=((result[index]<<12)|1);
			}
		result=phys_to_virt(result[index]);
		}
		index=((mask_9_bits>>(9*(i+1)))&vpn)>>(39-(9*i));
		result[index]=(ppn<<12)|1;
	}
					
	else
	{
		for(i=0;i<4;i++)
		{
		index=((mask_9_bits>>(9*(i+1)))&vpn)>>(48-(9*(i+1)));
		if (is_valid(result[index]))
			{
			result=phys_to_virt(result[index]);
			}
		else
			{
			return;
			}
		}
		index=((mask_9_bits>>(9*(i+1)))&vpn)>>(39-(9*i));
		result[index]=(result[index]&mask_last_off);
	}	
}

uint64_t page_table_query(uint64_t pt, uint64_t vpn)
{
	uint64_t index,*result=phys_to_virt(pt<<12);
	unsigned int i;
	vpn=vpn<<12;
	for(i=0;i<4;i++)
		{
		index=((mask_9_bits>>(9*(i+1)))&vpn)>>(48-(9*(i+1)));
		if (is_valid(result[index]))
			{
			result=phys_to_virt(result[index]);
			}
		else
			{
			return NO_MAPPING;
			}
		}
	index=((mask_9_bits>>(9*(i+1)))&vpn)>>(39-(9*i));
		if (is_valid(result[index]))
			{
			return result[index]>>12;
			}	
	return NO_MAPPING;
}

