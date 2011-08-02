#include "sfc.h"
#include "param.h"

extern const short RTDtable[105];

// zero is an offset, in A/D counts 
short RTDconvert(ushort ad) {
  short rtd;
  short *table;
  long temp;
  
  if (ad >= 104*256) {
    return 32767;
  }
  temp = ad;
  temp *= param[PARAM_RTD_SPAN];
  ad += (short)(temp >> 16);
  ad += param[PARAM_RTD_ZERO];
  if ((short)ad < 0) {
    return RTDtable[0];
  }
  if (ad >= 104*256) {
    ad = 32767;
  }
  
  // table has entries every 256 counts, interpolate inbetween
  table = (short *)&RTDtable[ad >> 8];
  rtd = table[1] - table[0];
  temp = rtd;
  temp *= (ad & 0xFF);
  rtd = (short)(temp >> 8);
  rtd += *table;
  
  return rtd;
} // RTDconvert();


void RTDcal(ushort adc100, ushort adc180) {
  long rtdSpan;
  long rtdZero;
  
  if (adc100 < 5619 || adc100 > 6419) {
    // value is out of expected range
    // logAdd()
    return;
  }
  if (adc180 < 22748 || adc180 > 23548) {
    // value is out of expected range
    // logAdd()
    return;
  }
  
  // Expected A/D for 100ohms is  6019.833
  // Expected A/D for 180ohms is 23148.323
  // rtdSpan = 65536 * (1 - (23148.323-6019.833)/(adc180-adc100))
  // start with 65536 * (23148.323-6019.833)
  rtdSpan = 1122532721;
  rtdSpan /= (adc180 - adc100);
  rtdSpan -= 65536;
  // rtdZero = 6019.833 - adc100*(1+rtdSpan/65536)
  rtdZero = rtdSpan;
  rtdZero += 65536;
  rtdZero *= -(short)adc100;
  rtdZero += 394548543;
  rtdZero >>= 16;
  
  ParamWrite(PARAM_RTD_ZERO, (ulong)rtdZero);
  ParamWrite(PARAM_RTD_SPAN, (ulong)rtdSpan);
} // RTDcal()


// Entries every 256 A/D counts stored as degC10
const short RTDtable[105] = {
  -633,    //     0
  -607,    //   256
  -581,    //   512
  -555,    //   768
  -529,    //  1024
  -503,    //  1280
  -476,    //  1536
  -450,    //  1792
  -423,    //  2048
  -397,    //  2304
  -370,    //  2560
  -343,    //  2816
  -316,    //  3072
  -289,    //  3328
  -262,    //  3584
  -235,    //  3840
  -208,    //  4096
  -181,    //  4352
  -153,    //  4608
  -125,    //  4864
   -98,    //  5120
   -70,    //  5376
   -42,    //  5632
   -14,    //  5888
    14,    //  6144
    42,    //  6400
    70,    //  6656
    98,    //  6912
   127,    //  7168
   155,    //  7424
   184,    //  7680
   212,    //  7936
   241,    //  8192
   270,    //  8448
   299,    //  8704
   328,    //  8960
   357,    //  9216
   387,    //  9472
   416,    //  9728
   445,    //  9984
   475,    // 10240
   505,    // 10496
   534,    // 10752
   564,    // 11008
   594,    // 11264
   624,    // 11520
   654,    // 11776
   685,    // 12032
   715,    // 12288
   746,    // 12544
   776,    // 12800
   807,    // 13056
   838,    // 13312
   869,    // 13568
   900,    // 13824
   931,    // 14080
   962,    // 14336
   994,    // 14592
  1025,    // 14848
  1057,    // 15104
  1089,    // 15360
  1121,    // 15616
  1153,    // 15872
  1185,    // 16128
  1217,    // 16384
  1249,    // 16640
  1282,    // 16896
  1314,    // 17152
  1347,    // 17408
  1380,    // 17664
  1413,    // 17920
  1446,    // 18176
  1479,    // 18432
  1512,    // 18688
  1545,    // 18944
  1579,    // 19200
  1613,    // 19456
  1647,    // 19712
  1681,    // 19968
  1715,    // 20224
  1749,    // 20480
  1783,    // 20736
  1817,    // 20992
  1852,    // 21248
  1887,    // 21504
  1922,    // 21760
  1957,    // 22016
  1992,    // 22272
  2028,    // 22528
  2065,    // 22784
  2102,    // 23040
  2139,    // 23296
  2176,    // 23552
  2213,    // 23808
  2250,    // 24064
  2287,    // 24320
  2324,    // 24576
  2361,    // 24832
  2398,    // 25088
  2435,    // 25344
  2472,    // 25600
  2509,    // 25856
  2546,    // 26112
  2583,    // 26368
  2620     // 26624
};
