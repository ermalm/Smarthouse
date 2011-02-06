#ifndef DS1307_H_
#define DS1307_H_

#define DS1307_I2C_ADDR	 0xD0

#define BCD2DEC(x)		((((x >> 4) & 0x0F)*10) + (x & 0x0F))
#define	DEC2BCD_HI(x)	((x)/10)
#define	DEC2BCD_LO(x)	((x)%10)
#define	DEC2BCD(x)		(((DEC2BCD_HI(x)) << 4) + (DEC2BCD_LO(x)))
#define true 1
#define false 0

typedef struct {
	u08 sec ;
	u08 min ;
	u08 hr  ;
	u08 day ;
	u08 dat ;
	u08 mon ;
	u08 yr ;
} time_t;

char ds1307_settime(u08 i2cAddr,time_t time);
char ds1307_gettime(u08 i2cAddr,time_t* time);

char ds1307_enable(u08 i2cAddr);
char ds1307_disable(u08 i2cAddr);

char ds1307_24hmode(u08 i2cAddr);
char ds1307_12hmode(u08 i2cAddr);
// 1 means 12 hours mode
// 0 means 24 hours mode
char ds1307_get1224mode(u08 i2cAddr);

#endif /*DS1307_H_*/
