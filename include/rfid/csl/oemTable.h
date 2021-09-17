#ifndef __OEMTABLE_H__
#define __OEMTABLE_H__


#include "cs203_dc.h"

/*
const char *OEM_Country_N1[2] = {"ETSI", "IN"};
const char *OEM_Country_N2[9] = {"AU", "BR1", "BR2", "FCC", "HK", "SG", "MY", "ZA", "ID"};
const char *OEM_Country_N4[19] = {"AU", "MY", "HK", "SG", "TW", "ID", "CN", "CN1", "CN2", "CN3", "CN4", "CN5", "CN6", "CN7", "CN8", "CN9", "CN10", "CN11", "CN12"};
const char *OEM_Country_N7[16] = {"AU", "HK", "SG", "CN", "CN1", "CN2", "CN3", "CN4", "CN5", "CN6", "CN7", "CN8", "CN9", "CN10", "CN11", "CN12"};
const char *OEM_Country_N8[1] = {"JP"};
*/



const CS203DC_COUNTRY OEM_Country_N1[3] = {CTY_ETSI, CTY_IN, CTY_G800};
const CS203DC_COUNTRY OEM_Country_N2[10] = {CTY_AU, CTY_BR1, CTY_BR2, CTY_FCC, CTY_HK, CTY_SG, CTY_MY, CTY_ZA, CTY_TH, CTY_ID};
const CS203DC_COUNTRY OEM_Country_N4[19] = {CTY_AU, CTY_MY, CTY_HK, CTY_SG, CTY_TW, CTY_ID, CTY_CN, CTY_CN1, CTY_CN2, CTY_CN3, CTY_CN4, CTY_CN5, CTY_CN6, CTY_CN7, CTY_CN8, CTY_CN9, CTY_CN10, CTY_CN11, CTY_CN12};
const CS203DC_COUNTRY OEM_Country_N7[17] = {CTY_AU, CTY_HK, CTY_TH, CTY_SG, CTY_CN, CTY_CN1, CTY_CN2, CTY_CN3, CTY_CN4, CTY_CN5, CTY_CN6, CTY_CN7, CTY_CN8, CTY_CN9, CTY_CN10, CTY_CN11, CTY_CN12};
const CS203DC_COUNTRY OEM_Country_N8[1] = {CTY_JP};

struct{
	const CS203DC_COUNTRY const *country_list;
	int country_count;
} const OEM_Country_N_List[9] =
{
  // OEM Country,  Number Country Support
	{0, 0},
	{OEM_Country_N1, 3},
	{OEM_Country_N2, 10},
	{0, 0},
	{OEM_Country_N4, 19},
	{0, 0},
	{0, 0},
	{OEM_Country_N7, 17},
	{OEM_Country_N8, 1},
};


#endif /* __OEMTABLE_H__ */

