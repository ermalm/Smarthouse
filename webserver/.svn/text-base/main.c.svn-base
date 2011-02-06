/*********************************************
* vim:sw=8:ts=8:si:et
* To use the above modeline in vim you must have "set modeline" in your .vimrc
* Author: Guido Socher
* Copyright: GPL V2
* See http://www.gnu.org/licenses/gpl.html
*
* Ethernet webserver with reset push button
*
* Chip type           : Atmega168 or Atmega328 with ENC28J60
* Note: there is a version number in the text. Search for version
*
* --- Modified by Ermal Mujaj <ermal@deface.no>
*
*	This file has been modified to work as a control-center for the slavemodules
*	Features:
*		- Redesigned website with better userinterface
*		- Information about slavemodules displayed on the website
*		- Controlling slavemodules remotely
*		- More pages have been added for our use
*
*
*********************************************/
#include "global.h"
#include <avr/io.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include "ip_arp_udp_tcp.h"
#include "enc28j60.h"
#include "timeout.h"
#include "avr_compat.h"
#include "net.h"
#include "uart.h"
#include "mmcom.h"
#include <util/delay.h>

#define PINGPATTERN 0x42
/* set output to Vcc, LED off */
#define LEDOFF PORTB|=(1<<PINB1)
/* set output to GND, LED on */
#define LEDON PORTB&=~(1<<PINB1)

// please modify the following two lines. mac and ip have to be unique
// in your local area network. You can not have the same numbers in
// two devices:
static uint8_t mymac[6] = {0x54,0x55,0x58,0x11,0x00,0x27};
// how did I get the mac addr? Translate the first 3 numbers into ascii is: TUX
static uint8_t myip[4] = {192,168,1,137};
static uint8_t monitoredhost[4] = {192,168,1,105};
//------------------- do not change below this line
static volatile uint8_t havemac=0; // 0 no mac yet, 1 mac ok, 2 wait for arp reply
static volatile uint8_t hoststatus=1; // 0=all ping/replies OK, otherwise amount of missed counts
static uint8_t resetcount=0; // how often reset was executed
static uint8_t hostbackafterreboot=0; // wait for on answer until we really activate the wd
static uint8_t sendping=1; // 1 we send ping (and receive ping), 0 we receive ping only
static volatile uint8_t pingnow=1; // 1 means time has run out send a ping now
static volatile uint8_t resetnow=0; 
static volatile uint8_t reinitmac=0; 
static uint8_t pinginterval=30; // after how many seconds to send or receive a ping (value range: 2 - 250)
static char *errmsg; // error text
// listen port for tcp/www (max range 1-254)
#define MYWWWPORT 80
//
#define BUFFER_SIZE 1500
static uint8_t buf[BUFFER_SIZE+1];
static volatile uint8_t pingtimer=1; // > 0 means wd running
static volatile uint8_t pagelock=0; // avoid sending icmp and web pages at the same time
static uint8_t arprefesh=0;
// global string buffer
#define STR_BUFFER_SIZE 24
static char strbuf[STR_BUFFER_SIZE+1];
//
// the password string (only the first 7 char checked), (only a-z,0-9,_ characters):
static char password[10]="secret"; // must not be longer than 9 char

time_t time={99,99,99,99,99,99,99};
slaveModule sm[noOfModules]={{0,1,50},{0,1,40},{0,1,20}};
signed char ethPacket [noOfBytes] ={3,9,4,27,13,22,0,0,1,1,15,1,0,0,20};

unsigned char stop_timer=0;
unsigned char timer_flag=0;

uint8_t verify_password(char *str)
{
	// the first characters of the received string are
	// a simple password/cookie:
	if (strncmp(password,str,7)==0){
		return(1);
	}
	return(0);
}
// search for a string of the form key=value in
// a string that looks like q?xyz=abc&uvw=defgh HTTP/1.1\r\n
//
// The returned value is stored in the global var strbuf
uint8_t find_key_val(char *str,char *key)
{
	uint8_t found=0;
	uint8_t i=0;
	char *kp;
	kp=key;
	while(*str &&  *str!=' ' && found==0){
		if (*str == *kp){
			kp++;
			if (*kp == '\0'){
				str++;
				kp=key;
				if (*str == '='){
					found=1;
				}
			}
		}
		else{
			kp=key;
		}
		str++;
	}
	if (found==1){
		// copy the value to a buffer and terminate it with '\0'
		while(*str &&  *str!=' ' && *str!='&' && i<STR_BUFFER_SIZE){
			strbuf[i]=*str;
			i++;
			str++;
		}
		strbuf[i]='\0';
	}
	// return the length of the value
	return(i);
}

// convert a single hex digit character to its integer value
unsigned char h2int(char c)
{
	if (c >= '0' && c <='9'){
		return((unsigned char)c - '0');
	}
	if (c >= 'a' && c <='f'){
		return((unsigned char)c - 'a' + 10);
	}
	if (c >= 'A' && c <='F'){
		return((unsigned char)c - 'A' + 10);
	}
	return(0);
}

// decode a url string e.g "hello%20joe" or "hello+joe" becomes "hello joe"
void urldecode(char *urlbuf)
{
	char c;
	char *dst;
	dst=urlbuf;
	while ((c = *urlbuf)) {
		if (c == '+') c = ' ';
		if (c == '%') {
			urlbuf++;
			c = *urlbuf;
			urlbuf++;
			c = (h2int(c) << 4) | h2int(*urlbuf);
		}
		*dst = c;
		dst++;
		urlbuf++;
	}
	*dst = '\0';
}

// parse a string an extract the IP to bytestr
uint8_t parse_ip(uint8_t *bytestr,char *str)
{
	char *sptr;
	uint8_t i=0;
	sptr=NULL;
	while(i<4){
		bytestr[i]=0;
		i++;
	}
	i=0;
	while(*str && i<4){
		// if a number then start
		if (sptr==NULL && isdigit(*str)){
			sptr=str;
		}
		if (*str == '.'){
			*str ='\0';
			bytestr[i]=(atoi(sptr)&0xff);
			i++;
			sptr=NULL;
		}
		str++;
	}
	*str ='\0';
	if (i==3){
		bytestr[i]=(atoi(sptr)&0xff);
		return(0);
	}
	return(1);
}

// take a byte string and display it  (base is 10 for ip and 16 for mac addr)
void mk_net_str(char *resultstr,uint8_t *bytestr,uint8_t len,char separator,uint8_t base)
{
	uint8_t i=0;
	uint8_t j=0;
	while(i<len){
		itoa((int)bytestr[i],&resultstr[j],base);
		// search end of str:
		while(resultstr[j]){j++;}
		resultstr[j]=separator;
		j++;
		i++;
	}
	j--;
	resultstr[j]='\0';
}

// takes a string of the form ack?pw=xxx&rst=1 and analyse it
// return values:  0 error
//                 1 resetpage and password OK
//                 4 stop wd
//                 5 start wd
//                 2 /mod page
//                 3 /now page
//                 6 /cnf page
//                10 /room1 page
//                11 /room2 page
//                12 /clk page
//

uint8_t analyse_get_url(char *str)
{
	errmsg="invalid pw";
	if (strncmp("now",str,3)==0){
		return(3);
	}
	if (strncmp("cnf",str,3)==0){
		return(6);
	}
	if (strncmp("room1",str,5)==0){
		return(10);
	}
	if (strncmp("room2",str,5)==0){
		return(11);
	}
	if (strncmp("clk",str,3)==0){
		return(12);
	}
	
	// Set the new time or date
	if (strncmp("cdclk",str,4)==0){
		if (find_key_val(str,"pw")){
			urldecode(strbuf);
			if (verify_password(strbuf)){
				if (find_key_val(str,"h")){
					urldecode(strbuf);
					time.hr = atoi(strbuf);
					if (find_key_val(str,"m")){
						urldecode(strbuf);
						time.min = atoi(strbuf);
						if (find_key_val(str,"s")){
							urldecode(strbuf);
							time.sec = atoi(strbuf);
							if (find_key_val(str,"dd")){
								urldecode(strbuf);
								time.dat = atoi(strbuf);
								if (find_key_val(str,"mm")){
									urldecode(strbuf);
									time.mon = atoi(strbuf);
									if (find_key_val(str,"yy")){
										urldecode(strbuf);
										time.yr = atoi(strbuf);
										strbuf[7]='\0';
										return(35);
									}
								}
							}
						}
					}
				}
			}
		}
	}
	// change temp in room1
	if (strncmp("tmpc",str,4)==0){
		if (find_key_val(str,"pw")){
			urldecode(strbuf);
			if (verify_password(strbuf)){
				if (find_key_val(str,"ntemp")){
					urldecode(strbuf);
					strbuf[2]='\0';
					return(20);
				}
			}
		}
	}
	// change temp in room2
	if (strncmp("tmpc2",str,5)==0){
		if (find_key_val(str,"pw")){
			urldecode(strbuf);
			if (verify_password(strbuf)){
				if (find_key_val(str,"ntemp2")){
					urldecode(strbuf);
					strbuf[2]='\0';
					return(21);
				}
			}
		}
	}
	
	// change on / off status on slave-modules
	if (strncmp("stat1",str,5)==0){
		if (find_key_val(str,"pw")){
			urldecode(strbuf);
			if (verify_password(strbuf)){
				if (find_key_val(str,"onoff")){
					urldecode(strbuf);
					strbuf[2]='\0';
					return(34);
				}
			}
		}
		
	}
	// change on / off status on slave-modules
	if (strncmp("stat2",str,5)==0){
		if (find_key_val(str,"pw")){
			urldecode(strbuf);
			if (verify_password(strbuf)){
				if (find_key_val(str,"onoff2")){
					urldecode(strbuf);
					strbuf[2]='\0';
					return(32);
				}
			}
		}
		
	}
	// change own ip and pw
	if (strncmp("ipc",str,3)==0){
		if (find_key_val(str,"pw")){
			urldecode(strbuf);
			if (verify_password(strbuf)){
				if (find_key_val(str,"nip")){
					urldecode(strbuf);
					if (parse_ip(myip,strbuf)){
						errmsg="invalid ip";
						return(0);
					}
					strbuf[7]='\0';
					return(7);
				}
			}
			if (find_key_val(str,"npw")){
				urldecode(strbuf);
				strbuf[7]='\0';
				strcpy(password,strbuf);
				return(2);
			}
		}
		return(0);
	}
	if (strncmp("mod",str,3)==0){
		return(2);
	}
	errmsg="inv. url";
	return(0);
}

// answer HTTP/1.0 301 Moved Permanently\r\nLocation: password/\r\n\r\n
// to redirect to the url ending in a slash
uint16_t moved_perm(uint8_t *buf)
{
	uint16_t plen;
	plen=fill_tcp_data_p(buf,0,PSTR("HTTP/1.0 301 Moved Permanently\r\nLocation: "));
	plen=fill_tcp_data(buf,plen,password);
	plen=fill_tcp_data_p(buf,plen,PSTR("/\r\nContent-Type: text/html\r\nPragma: no-cache\r\n\r\n"));
	plen=fill_tcp_data_p(buf,plen,PSTR("<h1>301 Moved Permanently</h1>\n"));
	plen=fill_tcp_data_p(buf,plen,PSTR("add a trailing slash to the url\n"));
	return(plen);
}

uint16_t http200ok(void)
{
	return(fill_tcp_data_p(buf,0,PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nPragma: no-cache\r\n\r\n")));
}


// prepare the webpage by writing the data to the tcp send buffer
uint16_t print_webpage(uint8_t *buf)
{
	uint16_t plen;
	// periodic refresh every 60sec:
	plen=fill_tcp_data_p(buf,0,PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nPragma: no-cache\r\nRefresh: 60\r\n\r\n"));
	// html code + css for giving the website a better userinterface
	//
	plen=fill_tcp_data_p(buf,plen,PSTR("<html><head><style type=\"text/css\">\n"));
	plen=fill_tcp_data_p(buf,plen,PSTR("* {padding:0;margin:0;} body,input {font-size:10pt;font-family:\"georgia\";background:#74888e;}\n"));
	plen=fill_tcp_data_p(buf,plen,PSTR("#outer {width:350px;border:2px solid #fff;background-color:#fff;margin:0 auto;} #header {background:#2b2b2b;margin-bottom:2px;}#headercontent {bottom:0;padding:0.7em 1em 0.7em 1em;}\n"));
	plen=fill_tcp_data_p(buf,plen,PSTR("h1 {color:#fff;font-size:2.5em;} #menu {position:relative;background:#2b2b2b;height:3.5em;padding:0 1em 0 1em;margin-bottom:2px;} #menu ul {position:absolute;top:1.1em;}\n"));
	plen=fill_tcp_data_p(buf,plen,PSTR("#menu ul li {display:inline;}#menu ul li a {padding:0.5em 1em 0.9em 1em;color:#fff;} #content {padding:2em 2em 0 2em;}\n"));
	plen=fill_tcp_data_p(buf,plen,PSTR("p.klokke {color:blue;font-size:2.5em;text-align:center;} p.dato {font-size:1em;text-align:center;}"));
	plen=fill_tcp_data_p(buf,plen,PSTR("</style></head><body><div id=\"outer\"><div id=\"header\"><div id=\"headercontent\">"));
	// Webserver header
	plen=fill_tcp_data_p(buf,plen,PSTR("<h1>Josematic 1.0b</h1></div></div>\n"));
	plen=fill_tcp_data_p(buf,plen,PSTR("<div id=\"menu\"><ul>\n"));
	//the "menu" or links
	plen=fill_tcp_data_p(buf,plen,PSTR("<li><a href=\"/\">Hoved</a></li>\n"));
	plen=fill_tcp_data_p(buf,plen,PSTR("<li><a href=\"/cnf\">Kontroll</a></li>\n"));
	plen=fill_tcp_data_p(buf,plen,PSTR("<li><a href=\"http://deface.no/hvp2009/\" target=\"_blank\">Hjelp</a></li>\n"));
	plen=fill_tcp_data_p(buf,plen,PSTR("</ul></div><div id=\"menubottom\"></div><div id=\"content\">"));
	
	// This is the start of the clock output
	plen=fill_tcp_data_p(buf,plen,PSTR("<p class=\"klokke\">"));
	
	// print out the hour
	sprintf(strbuf,"%02d",ethPacket[pHour]);
	plen=fill_tcp_data(buf,plen,strbuf);
	plen=fill_tcp_data_p(buf,plen,PSTR(":"));
	
	// minutes and so on..
	sprintf(strbuf,"%02d",ethPacket[pMin]);
	plen=fill_tcp_data(buf,plen,strbuf);
	plen=fill_tcp_data_p(buf,plen,PSTR(":"));
	
	// seconds
	sprintf(strbuf,"%02d",ethPacket[pSec]);
	plen=fill_tcp_data(buf,plen,strbuf);
	
	// This is the start of the date output
	plen=fill_tcp_data_p(buf,plen,PSTR("</p><p class=\"dato\">"));
	
	// date
	sprintf(strbuf,"%02d",ethPacket[pDate]);
	plen=fill_tcp_data(buf,plen,strbuf);
	plen=fill_tcp_data_p(buf,plen,PSTR(" - "));
	
	// month
	sprintf(strbuf,"%02d",ethPacket[pMonth]);
	plen=fill_tcp_data(buf,plen,strbuf);
	plen=fill_tcp_data_p(buf,plen,PSTR(" - "));
	
	// year
	sprintf(strbuf,"%02d",ethPacket[pYear]);
	plen=fill_tcp_data(buf,plen,strbuf);
	plen=fill_tcp_data_p(buf,plen,PSTR("</p><br><br>"));
	
	
	// The temperatur status
	// Room 1 status
	plen=fill_tcp_data_p(buf,plen,PSTR("<br>Rom-1: "));
	// Temperatur is blue for lower then 15 degrees, red for higher
	if (ethPacket[pTemp]<=15)
	{
		plen=fill_tcp_data_p(buf,plen,PSTR("<font color=blue> "));
		itoa(ethPacket[pTemp],strbuf,10);
		plen=fill_tcp_data(buf,plen,strbuf);
		plen=fill_tcp_data_p(buf,plen,PSTR("</font>&deg;C<br>"));
	}
	else
	{
		plen=fill_tcp_data_p(buf,plen,PSTR("<font color=red> "));
		itoa(ethPacket[pTemp],strbuf,10);
		plen=fill_tcp_data(buf,plen,strbuf);
		plen=fill_tcp_data_p(buf,plen,PSTR("</font>&deg;C<br>"));
	}
	plen=fill_tcp_data_p(buf,plen,PSTR("&Oslash;nsket temp: "));
	itoa(sm[0].temp,strbuf,10);
	plen=fill_tcp_data(buf,plen,strbuf);
	plen=fill_tcp_data_p(buf,plen,PSTR("&deg;C<br>"));
	
	// Check for slave status, 0 = off, 1 = on , 2 = not connected
	plen=fill_tcp_data_p(buf,plen,PSTR("Status: "));
	if (ethPacket[pStatus]==0)
	{
		plen=fill_tcp_data_p(buf,plen,PSTR("<font color=red> "));
		plen=fill_tcp_data_p(buf,plen,PSTR("Off"));
	}
	else if (ethPacket[pStatus]==1)
	{
		plen=fill_tcp_data_p(buf,plen,PSTR("<font color=green> "));
		plen=fill_tcp_data_p(buf,plen,PSTR("On"));
	}
	else if (ethPacket[pStatus]==2)
	{
		plen=fill_tcp_data_p(buf,plen,PSTR("<font color=red> "));
		plen=fill_tcp_data_p(buf,plen,PSTR("NC"));
	}
	plen=fill_tcp_data_p(buf,plen,PSTR("</font><br>"));
	
	// Room 2 status
	plen=fill_tcp_data_p(buf,plen,PSTR("<br>Rom-2: "));
	
	if ((ethPacket[pFieldsModules+pTemp])<=15)
	{
		plen=fill_tcp_data_p(buf,plen,PSTR("<font color=blue> "));
		itoa(ethPacket[pFieldsModules+pTemp],strbuf,10);
		plen=fill_tcp_data(buf,plen,strbuf);
	}
	else
	{
		plen=fill_tcp_data_p(buf,plen,PSTR("<font color=red> "));
		itoa(ethPacket[pFieldsModules+pTemp],strbuf,10);
		plen=fill_tcp_data(buf,plen,strbuf);
	}
	plen=fill_tcp_data_p(buf,plen,PSTR("</font> &deg;C<br>"));
	plen=fill_tcp_data_p(buf,plen,PSTR("&Oslash;nsket temp: "));
	itoa(sm[1].temp,strbuf,10);
	plen=fill_tcp_data(buf,plen,strbuf);
	plen=fill_tcp_data_p(buf,plen,PSTR("&deg;C<br>"));
	
	// Check for slave status, 0 = off, 1 = on , 2 = no change
	plen=fill_tcp_data_p(buf,plen,PSTR("Status: "));
	if (ethPacket[pFieldsModules+pStatus]==0)
	{
		plen=fill_tcp_data_p(buf,plen,PSTR("<font color=red> "));
		plen=fill_tcp_data_p(buf,plen,PSTR("Off</font><br>"));
	}
	else if (ethPacket[pFieldsModules+pStatus]==1)
	{
		plen=fill_tcp_data_p(buf,plen,PSTR("<font color=green> "));
		plen=fill_tcp_data_p(buf,plen,PSTR("On</font><br>"));
	}
	else if (ethPacket[pFieldsModules+pStatus]==2)
	{
		plen=fill_tcp_data_p(buf,plen,PSTR("<font color=red> "));
		plen=fill_tcp_data_p(buf,plen,PSTR("NC</font><br>"));
	}
	
	plen=fill_tcp_data_p(buf,plen,PSTR("<br><br></div></body></html>"));
	
	return(plen);
}
// Control area for room-1
uint16_t print_webpage_room1(uint8_t *buf)
{
	uint16_t plen;
	plen=http200ok();
	plen=fill_tcp_data_p(buf,plen,PSTR("<style type=\"text/css\">* {padding:0;margin:0;} body {font-size:10pt;font-family:\"georgia\";color:#333333;background:#74888e;}\n"));
	plen=fill_tcp_data_p(buf,plen,PSTR("h5 {font-size:0.7em;} #outer {width:350px;border:2px solid #fff;background-color:#fff;margin:0 auto;} #header {background:#2b2b2b;margin-bottom:2px;}#headercontent {bottom:0;padding:0.7em 1em 0.7em 1em;}\n"));
	plen=fill_tcp_data_p(buf,plen,PSTR("#headercontent h1 {font-weight:normal;color:#fff;font-size:2.5em;}\n"));
	plen=fill_tcp_data_p(buf,plen,PSTR("#content {padding:2em 2em 0 2em;}\n"));
	plen=fill_tcp_data_p(buf,plen,PSTR("</style></head><body><div id=\"outer\"><div id=\"header\"><div id=\"headercontent\">"));
	// Webserver header
	plen=fill_tcp_data_p(buf,plen,PSTR("<h1>Rom-1</h1></div></div>\n"));
	plen=fill_tcp_data_p(buf,plen,PSTR("<div id=\"content\"><a href=\"/cnf\"><< Tilbake</a><br><br>\n"));
	
	// Room 1 status
	plen=fill_tcp_data_p(buf,plen,PSTR("Temperatur n&aring;:"));
	// Temperatur is blue for lower then 15 degrees, red for higher
	if (ethPacket[pTemp]<=15)
	{
		plen=fill_tcp_data_p(buf,plen,PSTR("<font color=blue> "));
		itoa(ethPacket[pTemp],strbuf,10);
		plen=fill_tcp_data(buf,plen,strbuf);
	}
	else
	{
		plen=fill_tcp_data_p(buf,plen,PSTR("<font color=red> "));
		itoa(ethPacket[pTemp],strbuf,10);
		plen=fill_tcp_data(buf,plen,strbuf);
	}
	plen=fill_tcp_data_p(buf,plen,PSTR("</font>&deg;C<br>"));
	
	plen=fill_tcp_data_p(buf,plen,PSTR("&Oslash;nsket temp: "));
	itoa(sm[0].temp,strbuf,10);
	plen=fill_tcp_data(buf,plen,strbuf);
	plen=fill_tcp_data_p(buf,plen,PSTR("&deg;C<br><br>"));
	
	// Check for slave status, 0 = off, 1 = on , 2 = no change
	plen=fill_tcp_data_p(buf,plen,PSTR("Status satt til: "));
	if (sm[0].status==0)
	{
		plen=fill_tcp_data_p(buf,plen,PSTR("<font color=red> "));
		plen=fill_tcp_data_p(buf,plen,PSTR("Off"));
	}
	else if (sm[0].status==1)
	{
		plen=fill_tcp_data_p(buf,plen,PSTR("<font color=green> "));
		plen=fill_tcp_data_p(buf,plen,PSTR("On"));
	}
	plen=fill_tcp_data_p(buf,plen,PSTR("</font>"));
	
	plen=fill_tcp_data_p(buf,plen,PSTR("<br>"));
	plen=fill_tcp_data_p(buf,plen,PSTR("Status n&aring;:  "));
	if (ethPacket[pStatus]==0)
	{
		plen=fill_tcp_data_p(buf,plen,PSTR("<font color=red> "));
		plen=fill_tcp_data_p(buf,plen,PSTR("Off"));
	}
	else if (ethPacket[pStatus]==1)
	{
		plen=fill_tcp_data_p(buf,plen,PSTR("<font color=green> "));
		plen=fill_tcp_data_p(buf,plen,PSTR("On"));
	}
	else if (ethPacket[pStatus]==2)
	{
		plen=fill_tcp_data_p(buf,plen,PSTR("<font color=red> "));
		plen=fill_tcp_data_p(buf,plen,PSTR("NC"));
	}
	plen=fill_tcp_data_p(buf,plen,PSTR("</font><br><br>"));
	
	// Change status
	plen=fill_tcp_data_p(buf,plen,PSTR("<fieldset><legend><h4>Undermodul</h4></legend><form action=\"/stat1\" method=get>"));
	plen=fill_tcp_data_p(buf,plen,PSTR("<br><input type=\"radio\" name=\"onoff\" value=1 "));
	if (sm[0].status==1)
		plen=fill_tcp_data_p(buf,plen,PSTR("checked "));
	plen=fill_tcp_data_p(buf,plen,PSTR(">Sl&aring;tt p&aring;<br>"));
	plen=fill_tcp_data_p(buf,plen,PSTR("<input type=\"radio\" name=\"onoff\" value=0 "));
	if (sm[0].status==0)
		plen=fill_tcp_data_p(buf,plen,PSTR("checked "));
	plen=fill_tcp_data_p(buf,plen,PSTR(">Sl&aring;tt av<br>"));
	plen=fill_tcp_data_p(buf,plen,PSTR("Passord:</td><td><input type=password size=8 name=pw><br>"));
	plen=fill_tcp_data_p(buf,plen,PSTR("<input type=submit value=\"OK\"></form><br></fieldset>"));
	
	// Set temperatur
	plen=fill_tcp_data_p(buf,plen,PSTR("<br><fieldset><legend><h4>Temp</h4></legend>"));
	plen=fill_tcp_data_p(buf,plen,PSTR("<form action=/tmpc method=get>"));
	plen=fill_tcp_data_p(buf,plen,PSTR("<br>Ny temp: <input type=text size=1 name=ntemp value="));
	plen=fill_tcp_data_p(buf,plen,PSTR("><br>"));
	plen=fill_tcp_data_p(buf,plen,PSTR("Passord:<input type=password size=8 name=pw>"));
	plen=fill_tcp_data_p(buf,plen,PSTR("<br><input type=submit value=\"OK\"><br>"));
	plen=fill_tcp_data_p(buf,plen,PSTR("</form><br></fieldset>"));
	// end
	plen=fill_tcp_data_p(buf,plen,PSTR("</div></body></html>"));
	
	return(plen);
}

// Control area for room-2
uint16_t print_webpage_room2(uint8_t *buf)
{
	uint16_t plen;
	plen=http200ok();
	plen=fill_tcp_data_p(buf,plen,PSTR("<style type=\"text/css\">* {padding:0;margin:0;} body {font-size:10pt;font-family:\"georgia\";color:#333333;background:#74888e;}\n"));
	plen=fill_tcp_data_p(buf,plen,PSTR("h5 {font-size:0.7em;} #outer {width:350px;border:2px solid #fff;background-color:#fff;margin:0 auto;} #header {background:#2b2b2b;margin-bottom:2px;}#headercontent {bottom:0;padding:0.7em 1em 0.7em 1em;}\n"));
	plen=fill_tcp_data_p(buf,plen,PSTR("#headercontent h1 {font-weight:normal;color:#fff;font-size:2.5em;}\n"));
	plen=fill_tcp_data_p(buf,plen,PSTR("#content {padding:2em 2em 0 2em;}\n"));
	plen=fill_tcp_data_p(buf,plen,PSTR("</style></head><body><div id=\"outer\"><div id=\"header\"><div id=\"headercontent\">"));
	// Webserver header
	plen=fill_tcp_data_p(buf,plen,PSTR("<h1>Rom-2</h1></div></div>\n"));
	plen=fill_tcp_data_p(buf,plen,PSTR("<div id=\"content\"><a href=\"/cnf\"><< Tilbake</a><br><br>\n"));
	
	// Room 2 status
	plen=fill_tcp_data_p(buf,plen,PSTR("Temperatur n&aring;:  "));
	// Temperatur is blue for lower then 15 degrees, red for higher
	if (ethPacket[pFieldsModules+pTemp]<=15)
	{
		plen=fill_tcp_data_p(buf,plen,PSTR("<font color=blue> "));
		itoa(ethPacket[pFieldsModules+pTemp],strbuf,10);
		plen=fill_tcp_data(buf,plen,strbuf);
	}
	else
	{
		plen=fill_tcp_data_p(buf,plen,PSTR("<font color=red> "));
		itoa(ethPacket[pFieldsModules+pTemp],strbuf,10);
		plen=fill_tcp_data(buf,plen,strbuf);
	}
	plen=fill_tcp_data_p(buf,plen,PSTR("</font>&deg;C<br>"));
	
	plen=fill_tcp_data_p(buf,plen,PSTR("&Oslash;nsket temp: "));
	itoa(sm[1].temp,strbuf,10);
	plen=fill_tcp_data(buf,plen,strbuf);
	plen=fill_tcp_data_p(buf,plen,PSTR("&deg;C<br><br>"));
	
	// Check for slave status, 0 = off, 1 = on , 2 = no change
	plen=fill_tcp_data_p(buf,plen,PSTR("Status satt til: "));
	if (sm[1].status==0)
	{
		plen=fill_tcp_data_p(buf,plen,PSTR("<font color=red> "));
		plen=fill_tcp_data_p(buf,plen,PSTR("Off"));
	}
	else if (sm[1].status==1)
	{
		plen=fill_tcp_data_p(buf,plen,PSTR("<font color=green> "));
		plen=fill_tcp_data_p(buf,plen,PSTR("On"));
	}
	plen=fill_tcp_data_p(buf,plen,PSTR("</font>"));
	
	plen=fill_tcp_data_p(buf,plen,PSTR("<br>"));
	plen=fill_tcp_data_p(buf,plen,PSTR("Status n&aring;:  "));
	if (ethPacket[pFieldsModules+pStatus]==0)
	{
		plen=fill_tcp_data_p(buf,plen,PSTR("<font color=red> "));
		plen=fill_tcp_data_p(buf,plen,PSTR("Off"));
	}
	else if (ethPacket[pFieldsModules+pStatus]==1)
	{
		plen=fill_tcp_data_p(buf,plen,PSTR("<font color=green> "));
		plen=fill_tcp_data_p(buf,plen,PSTR("On"));
	}
	else if (ethPacket[pFieldsModules+pStatus]==2)
	{
		plen=fill_tcp_data_p(buf,plen,PSTR("<font color=red> "));
		plen=fill_tcp_data_p(buf,plen,PSTR("NC"));
	}
	plen=fill_tcp_data_p(buf,plen,PSTR("</font><br><br>"));
	
	// Change status
	plen=fill_tcp_data_p(buf,plen,PSTR("<fieldset><legend><h4>Undermodul</h4></legend><form action=\"/stat2\" method=get>"));
	plen=fill_tcp_data_p(buf,plen,PSTR("<br><input type=\"radio\" name=\"onoff2\" value=1 "));
	if (sm[1].status==1)
		plen=fill_tcp_data_p(buf,plen,PSTR("checked "));
	plen=fill_tcp_data_p(buf,plen,PSTR(">Sl&aring;tt p&aring;<br>"));
	plen=fill_tcp_data_p(buf,plen,PSTR("<input type=\"radio\" name=\"onoff2\" value=0 "));
	if (sm[1].status==0)
		plen=fill_tcp_data_p(buf,plen,PSTR("checked "));
	plen=fill_tcp_data_p(buf,plen,PSTR(">Sl&aring;tt av<br>"));
	plen=fill_tcp_data_p(buf,plen,PSTR("Passord:</td><td><input type=password size=8 name=pw><br>"));
	plen=fill_tcp_data_p(buf,plen,PSTR("<input type=submit value=\"OK\"></form><br></fieldset>"));
	
	// Set temperatur
	plen=fill_tcp_data_p(buf,plen,PSTR("<br><fieldset><legend><h4>Temp</h4></legend>"));
	plen=fill_tcp_data_p(buf,plen,PSTR("<form action=/tmpc2 method=get>"));
	plen=fill_tcp_data_p(buf,plen,PSTR("<br>Ny temp: <input type=text size=1 name=ntemp2 value="));
	plen=fill_tcp_data_p(buf,plen,PSTR("><br>"));
	plen=fill_tcp_data_p(buf,plen,PSTR("Passord:<input type=password size=8 name=pw>"));
	plen=fill_tcp_data_p(buf,plen,PSTR("<br><input type=submit value=\"OK\"><br>"));
	plen=fill_tcp_data_p(buf,plen,PSTR("</form><br></fieldset>"));
	
	// end
	plen=fill_tcp_data_p(buf,plen,PSTR("</div></body></html>"));
	
	return(plen);
}

// main site for control
uint16_t print_webpage_config(uint8_t *buf)
{
	uint16_t plen;
	plen=http200ok();
	
	plen=fill_tcp_data_p(buf,plen,PSTR("<style type=\"text/css\">*{padding:0;margin:0;}body{font-size:10pt;font-family:\"georgia\";color:#333333;background:#74888e;}"));
	plen=fill_tcp_data_p(buf,plen,PSTR("h5{font-size:0.7em;}#outer{width:350px;border:2px solid #fff;background-color:#fff;margin:0 auto;}#header{background:#2b2b2b;margin-bottom:2px;}#headercontent{bottom:0;padding:0.7em 1em 0.7em 1em;}"));
	plen=fill_tcp_data_p(buf,plen,PSTR("#headercontent h1{font-weight:normal;color:#fff;font-size:2.5em;}#menu{position:relative;background:#2b2b2b;height:3.5em;padding:0 1em 0 1em;margin-bottom:2px;}#menu ul{position:absolute;top:1.1em;}"));
	plen=fill_tcp_data_p(buf,plen,PSTR("#menu ul li{display:inline;}#menu ul li a{padding:0.5em 1em 0.9em 1em;color:#fff;}#content{padding:2em 2em 0 2em;}"));
	plen=fill_tcp_data_p(buf,plen,PSTR("</style></head><body><div id=\"outer\"><div id=\"header\"><div id=\"headercontent\">"));
	// Webserver header
	plen=fill_tcp_data_p(buf,plen,PSTR("<h1>Kontroll</h1></div></div>"));
	plen=fill_tcp_data_p(buf,plen,PSTR("<div id=\"menu\"><ul>"));
	//the "menu" or links
	plen=fill_tcp_data_p(buf,plen,PSTR("<li><a href=\"/\">Hoved</a></li>"));
	plen=fill_tcp_data_p(buf,plen,PSTR("<li><a href=\"/room1\">Rom-1</a></li>"));
	plen=fill_tcp_data_p(buf,plen,PSTR("<li><a href=\"/room2\">Rom-2</a></li>"));
	plen=fill_tcp_data_p(buf,plen,PSTR("<li><a href=\"/clk\">Klokke</a></li>"));
	plen=fill_tcp_data_p(buf,plen,PSTR("</ul></div><div id=\"content\">"));
	
	// Change the IP
	plen=fill_tcp_data_p(buf,plen,PSTR("<fieldset><legend><h4>Endre IP</h4></legend>"));
	plen=fill_tcp_data_p(buf,plen,PSTR("<form action=/ipc method=get>"));
	plen=fill_tcp_data_p(buf,plen,PSTR("<br>Passord:<input type=password size=8 name=pw><br>"));
	plen=fill_tcp_data_p(buf,plen,PSTR("Ny IP: <input type=text size=12 name=nip value="));
	mk_net_str(strbuf,myip,4,'.',10);
	plen=fill_tcp_data(buf,plen,strbuf);
	plen=fill_tcp_data_p(buf,plen,PSTR("><br>"));
	plen=fill_tcp_data_p(buf,plen,PSTR("<input type=submit value=\"OK\"></form><br></fieldset>"));
	
	// Change the password
	plen=fill_tcp_data_p(buf,plen,PSTR("<br><fieldset><form action=/ipc method=get>"));
	plen=fill_tcp_data_p(buf,plen,PSTR("<br>Gammelt passord:<input type=password size=8 name=pw><br>"));
	plen=fill_tcp_data_p(buf,plen,PSTR("Nytt passord: <input type=password size=8 name=npw><br>"));
	plen=fill_tcp_data_p(buf,plen,PSTR("<input type=submit value=\"OK\"></form><br></fieldset>"));
	
	return(plen);
}

// main webpage for setting the time and date
uint16_t print_webpage_clk(uint8_t *buf)
{
	uint16_t plen;
	plen=http200ok();
	
	plen=fill_tcp_data_p(buf,plen,PSTR("<style type=\"text/css\">* {padding:0;margin:0;} body {font-size:10pt;font-family:\"georgia\";color:#333333;background:#74888e;}\n"));
	plen=fill_tcp_data_p(buf,plen,PSTR("h5 {font-size:0.7em;} #outer {width:340px;border:2px solid #fff;background-color:#fff;margin:0 auto;} #header {background:#2b2b2b;margin-bottom:2px;}#headercontent {bottom:0;padding:0.7em 1em 0.7em 1em;}\n"));
	plen=fill_tcp_data_p(buf,plen,PSTR("#headercontent h1 {font-weight:normal;color:#fff;font-size:2.5em;}\n"));
	plen=fill_tcp_data_p(buf,plen,PSTR("#content {padding:2em 2em 0 2em;}\n"));
	plen=fill_tcp_data_p(buf,plen,PSTR("</style></head><body><div id=\"outer\"><div id=\"header\"><div id=\"headercontent\">"));
	// Webserver header
	plen=fill_tcp_data_p(buf,plen,PSTR("<h1>Klokke / dato</h1></div></div>\n"));
	plen=fill_tcp_data_p(buf,plen,PSTR("<div id=\"content\">\n"));
	//content
	plen=fill_tcp_data_p(buf,plen,PSTR("<a href=\"/cnf\"><< Tilbake</a><br><br>"));
	plen=fill_tcp_data_p(buf,plen,PSTR("<fieldset><legend>Sett ny tid og dato</legend>"));
	plen=fill_tcp_data_p(buf,plen,PSTR("<br><form action=/cdclk method=get>"));
	plen=fill_tcp_data_p(buf,plen,PSTR("<input type=text size=1 name=h value="));
	sprintf(strbuf,"%02d",ethPacket[pHour]);
	plen=fill_tcp_data(buf,plen,strbuf);
	plen=fill_tcp_data_p(buf,plen,PSTR(">"));
	plen=fill_tcp_data_p(buf,plen,PSTR(":"));
	plen=fill_tcp_data_p(buf,plen,PSTR("<input type=text size=1 name=m value="));
	sprintf(strbuf,"%02d",ethPacket[pMin]);
	plen=fill_tcp_data(buf,plen,strbuf);
	plen=fill_tcp_data_p(buf,plen,PSTR(">"));
	plen=fill_tcp_data_p(buf,plen,PSTR(":"));
	plen=fill_tcp_data_p(buf,plen,PSTR("<input type=text size=1 name=s value="));
	sprintf(strbuf,"%02d",ethPacket[pSec]);
	plen=fill_tcp_data(buf,plen,strbuf);
	plen=fill_tcp_data_p(buf,plen,PSTR(">"));
	plen=fill_tcp_data_p(buf,plen,PSTR("(hh:tt:ss)<br><br>"));
	plen=fill_tcp_data_p(buf,plen,PSTR("<input type=text size=1 name=dd value="));
	sprintf(strbuf,"%02d",ethPacket[pDate]);
	plen=fill_tcp_data(buf,plen,strbuf);
	plen=fill_tcp_data_p(buf,plen,PSTR(">"));
	plen=fill_tcp_data_p(buf,plen,PSTR("-"));
	plen=fill_tcp_data_p(buf,plen,PSTR("<input type=text size=1 name=mm value="));
	sprintf(strbuf,"%02d",ethPacket[pMonth]);
	plen=fill_tcp_data(buf,plen,strbuf);
	plen=fill_tcp_data_p(buf,plen,PSTR(">"));
	plen=fill_tcp_data_p(buf,plen,PSTR("-"));
	plen=fill_tcp_data_p(buf,plen,PSTR("<input type=text size=1 name=yy value="));
	sprintf(strbuf,"%02d",ethPacket[pYear]);
	plen=fill_tcp_data(buf,plen,strbuf);
	plen=fill_tcp_data_p(buf,plen,PSTR(">"));
	plen=fill_tcp_data_p(buf,plen,PSTR("(dd-mm-yy)<br><br>"));
	plen=fill_tcp_data_p(buf,plen,PSTR("Passord:<input type=password size=8 name=pw><br>"));
	plen=fill_tcp_data_p(buf,plen,PSTR("<input type=submit value=\"OK\"></form><br></fieldset><br>"));
	
	return(plen);
}
uint16_t print_webpage_now(uint8_t *buf)
{
	uint16_t plen;
	plen=http200ok();
	plen=fill_tcp_data_p(buf,plen,PSTR("<style type=\"text/css\">* {padding:0;margin:0;} body {font-size:10pt;font-family:\"georgia\";color:#333333;background:#74888e;}\n"));
	plen=fill_tcp_data_p(buf,plen,PSTR("h5 {font-size:0.7em;} #outer {width:340px;border:2px solid #fff;background-color:#fff;margin:0 auto;} #header {background:#2b2b2b;margin-bottom:2px;}#headercontent {bottom:0;padding:0.7em 1em 0.7em 1em;}\n"));
	plen=fill_tcp_data_p(buf,plen,PSTR("#headercontent h1 {font-weight:normal;color:#fff;font-size:2.5em;}\n"));
	plen=fill_tcp_data_p(buf,plen,PSTR("#content {padding:2em 2em 0 2em;}\n"));
	plen=fill_tcp_data_p(buf,plen,PSTR("</style></head><body><div id=\"outer\"><div id=\"header\"><div id=\"headercontent\">"));
	// Webserver header
	plen=fill_tcp_data_p(buf,plen,PSTR("<h1>Ekstra ting</h1></div></div>\n"));
	plen=fill_tcp_data_p(buf,plen,PSTR("<div id=\"content\">\n"));
	//content
	plen=fill_tcp_data_p(buf,plen,PSTR("<h2>Actions</h2>"));
	plen=fill_tcp_data_p(buf,plen,PSTR("<form action=/ack method=get>"));
	plen=fill_tcp_data_p(buf,plen,PSTR("Passw: <input type=password size=8 name=pw><input type=hidden name=rst value=1><input type=submit value=\"reboot host now\"></form>\n"));
	plen=fill_tcp_data_p(buf,plen,PSTR("<hr><form action=/ack method=get>"));
	if (pingtimer>0){
		plen=fill_tcp_data_p(buf,plen,PSTR("Passw: <input type=password size=8 name=pw><input type=hidden name=stp value=1><input type=submit value=\"stop watchdog now\"></form>"));
	}else{
		plen=fill_tcp_data_p(buf,plen,PSTR("Passw: <input type=password size=8 name=pw><input type=hidden name=srt value=1><input type=submit value=\"start watchdog now\"></form><hr>"));
	}
	return(plen);
}
// This page takes you back to the frontpage
uint16_t print_webpage_confirm(uint8_t *buf)
{
	uint16_t plen;
	plen=http200ok();
	plen=fill_tcp_data_p(buf,plen,PSTR("<h2>OK</h2> <a href=\"/\">-&gt;continue</a></p>\n"));
	return(plen);
}
// This page takes you back to the /clk page after uyou click the link	
uint16_t print_webpage_confirmclk(uint8_t *buf)
{
	uint16_t plen;
	plen=http200ok();
	plen=fill_tcp_data_p(buf,plen,PSTR("<h2>OK</h2> <a href=\"/clk\">-&gt;continue</a></p>\n"));
	return(plen);
}
// Return to room-1
uint16_t print_webpage_confirmrm1(uint8_t *buf)
{
	uint16_t plen;
	plen=http200ok();
	plen=fill_tcp_data_p(buf,plen,PSTR("<h2>OK</h2> <a href=\"/room1\">-&gt;continue</a></p>\n"));
	return(plen);
}
// Return to room-2
uint16_t print_webpage_confirmrm2(uint8_t *buf)
{
	uint16_t plen;
	plen=http200ok();
	plen=fill_tcp_data_p(buf,plen,PSTR("<h2>OK</h2> <a href=\"/room2\">-&gt;continue</a></p>\n"));
	return(plen);
}


void resethost(void)
{
	PORTD|= (1<<PIND7);// transistor on
	resetcount++;
	hostbackafterreboot=0;
	resetnow=1; // trigger counter to clear transistor 1 sec later
}

// timer interrupt, step counter every second
ISR(TIMER1_COMPA_vect){
	if (arprefesh==1 && hoststatus!=0){
		// refresh arp unless hoststatus is ok
		havemac=0;
	}
	// unlock if 2
	if (pagelock>1){
		pagelock=0;
		if (reinitmac){
			reinitmac=0;
			init_ip_arp_udp_tcp(mymac,myip,MYWWWPORT);
		}
	}
	// if we have a lock the wait a second
	if (pagelock==1){
		pagelock=2;
	}
	//-----
	if (resetnow==2){
		PORTD &= ~(1<<PIND7);// transistor off
		resetnow=0;
	}
	if (resetnow==1){
		resetnow=2;
	}
	if (hoststatus>9 && hostbackafterreboot){
		resethost();
	}
	//-----
	if (pingtimer>pinginterval){
		// do not wrap to 0 continue with 1:
		pingtimer=1;
	}
	//----- set a flag to send a ping as soon as possible 
	// this is to avoid interference with web traffic
	if (pingtimer == pinginterval){
		pingnow=1;
	}
	// pingtimer==0 means off
	if (pingtimer>0){
		if (sendping==0 && pingnow){
			// we should receive a ping
			hoststatus++; // get's cleard as soon as we get a ping
			pingnow=0;
			if (hoststatus==0xff){
				hoststatus=1; // wrap to 1
			}
		}
		pingtimer++;
	}
	arprefesh++;
	// if there is a problem with the reachability of the host 
	// then do more  often arp requests. havemac==2 means we don't have
	// a mac reply from the monitored host
	if (havemac==2 && arprefesh> 30){
		arprefesh=0;
	}
	// for security reasons as we increas pingtimer also below in main
	if (pingtimer>252){
		pingtimer=1;
	}
	if (stop_timer!=0)
	{
		stop_timer--;
		if (stop_timer==0)
			timer_flag=1;
	}
	
}


// Generate an interrup about ever 1s form the 12.5MHz system clock
// Since we have that 1024 prescaler we do not really generate a second
// (1.00000256000655361677s) 
void timer_init(void)
{
	/* write high byte first for 16 bit register access: */
	TCNT1H=0;  /* set counter to zero*/
	TCNT1L=0;
	// Mode 4 table 14-4 page 132. CTC mode and top in OCR1A
	// WGM13=0, WGM12=1, WGM11=0, WGM10=0
	TCCR1A=(0<<COM1B1)|(0<<COM1B0)|(0<<WGM11);
	TCCR1B=(1<<CS12)|(1<<CS10)|(1<<WGM12)|(0<<WGM13); // crystal clock/1024
	
	// At what value to cause interrupt. You can use this for calibration
	// of the clock. Theoretical value for 12.5MHz: 12207=0x2f and 0xaf
	OCR1AH=0x2f;
	OCR1AL=0xaf;
	// interrupt mask bit:
	TIMSK1 = (1 << OCIE1A);
}

void store_in_eeprom(void)
{
	eeprom_write_byte((uint8_t *)0x0,19); // magic number
	eeprom_write_block((uint8_t *)myip,(void *)1,sizeof(myip));
	eeprom_write_block((uint8_t *)monitoredhost,(void *)6,sizeof(monitoredhost));
	eeprom_write_byte((uint8_t *)11,pinginterval);
	eeprom_write_byte((uint8_t *)12,sendping);
	password[7]='\0'; // make sure it is terminated, should not be necessary
	eeprom_write_block((char *)password,(void *)13,sizeof(password));
}

int main(void){
	
	uint16_t plen;
	uint16_t dat_p;
	uint8_t cmd;
	
	// set the clock speed to "no pre-scaler" (8MHz with internal osc or
	// full external speed)
	// set the clock prescaler. First write CLKPCE to enable setting of clock the
	// next four instructions.
	CLKPR=(1<<CLKPCE); // change enable
	CLKPR=0; // "no pre-scaler"
	uartInit();
	uartSetBaudRate(9600);
	
	_delay_loop_1(50); // 12ms
	/* enable PD2/INT0, as input */
	DDRD&= ~(1<<DDD2);
	
	
	
	// test button
	cbi(DDRD,PIND6);
	sbi(PORTD,PIND6); // internal pullup resistor on
	_delay_loop_1(50); // 12ms
	// read eeprom values unless, the PD6 pin is connected to GND during bootup
	// PD6 is used to reset to factory default. Note that factory default is
	// not stored back to eeprom.
	if (eeprom_read_byte((uint8_t *)0x0) == 19 && ! bit_is_clear(PIND,PIND6)){
		// ok magic number matches accept values
		eeprom_read_block((uint8_t *)myip,(void *)1,sizeof(myip));
		eeprom_read_block((uint8_t *)monitoredhost,(void *)6,sizeof(monitoredhost));
		pinginterval=eeprom_read_byte((uint8_t *)11);
		sendping=eeprom_read_byte((uint8_t *)12);
		eeprom_read_block((char *)password,(void *)13,sizeof(password));
		password[7]='\0'; // make sure it is terminated, should not be necessary
	}
	
	/*initialize enc28j60*/
	enc28j60Init(mymac);
	enc28j60clkout(2); // change clkout from 6.25MHz to 12.5MHz
	_delay_loop_1(50); // 12ms
	// LED
	/* enable PB1, LED as output */
	//DDRB|= (1<<DDB1);
	/* set output to Vcc, LED off */
	//LEDOFF;
	
	// the transistor on PD7
	DDRD|= (1<<DDD7);
	PORTD &= ~(1<<PIND7);// transistor off
	
	/* Magjack leds configuration, see enc28j60 datasheet, page 11 */
	// LEDB=yellow LEDA=green
	//
	// 0x476 is PHLCON LEDA=links status, LEDB=receive/transmit
	// enc28j60PhyWrite(PHLCON,0b0000 0100 0111 01 10);
	enc28j60PhyWrite(PHLCON,0x476);
	_delay_loop_1(50); // 12ms
	
	
	//init the ethernet/ip layer:
	init_ip_arp_udp_tcp(mymac,myip,MYWWWPORT);
	timer_init();
	_delay_ms(1000);
	uartFlushReceiveBuffer();
	sei(); // interrupt enable
	
	while(1){
		//uartSendByte('a');
		
		//checkForEthPacket(ethPacket);
		stop_timer=2;
		if (checkForEthPacket(ethPacket))
		{
			sendEthPacket(time, sm);
			time.hr = 99;
		}
		stop_timer=0;
		// 			wait_data++;
		// 			if (wait_data>40)
		// 				wait_data=0;
		//_delay_ms(500);
		// spontanious messages must not interfer with
		// web pages
		if (pagelock==0 && enc28j60hasRxPkt()==0){
			if (sendping &&  havemac==0){
				client_arp_whohas(buf,monitoredhost);
				havemac=2;
			}
			if (sendping && havemac==1 && pingnow){
				pingnow=0;
				client_icmp_request(buf,monitoredhost,PINGPATTERN);
				pingtimer++; // otheweise we would call this function again
				hoststatus++;
				if (hoststatus==0xff){
					hoststatus=1; // wrap to 1
				}
			}
		}
		// get the next new packet:
		plen = enc28j60PacketReceive(BUFFER_SIZE, buf);
		
		/*plen will be unequal to zero if there is a valid 
		* packet (without crc error) */
		if(plen==0){
			continue;
		}
		
		// arp is broadcast if unknown but a host may also
		// verify the mac address by sending it to
		// a unicast address.
		if(eth_type_is_arp_and_my_ip(buf,plen)){
			if (eth_type_is_arp_req(buf)){
				make_arp_answer_from_request(buf);
			}
			if (eth_type_is_arp_reply(buf)){
				if (client_store_gw_mac(buf,monitoredhost)){
					havemac=1;
				}
			}
			continue;
		}
		
		// check if ip packets are for us:
		if(eth_type_is_ip_and_my_ip(buf,plen)==0){
			continue;
		}
		
		if(buf[IP_PROTO_P]==IP_PROTO_ICMP_V && buf[ICMP_TYPE_P]==ICMP_TYPE_ECHOREPLY_V){
			if (buf[ICMP_DATA_P]== PINGPATTERN){
				if (check_ip_message_is_from(buf,monitoredhost)){
					// ping reply is from monitored host and ping was from us
					hoststatus=0;
					hostbackafterreboot=1;
				}
			}
		}
		if(buf[IP_PROTO_P]==IP_PROTO_ICMP_V && buf[ICMP_TYPE_P]==ICMP_TYPE_ECHOREQUEST_V){
			if (check_ip_message_is_from(buf,monitoredhost)){
				// ping is from monitored host
				hoststatus=0;
				hostbackafterreboot=1;
			}
			// a ping packet, let's send pong
			make_echo_reply_from_request(buf,plen);
			continue;
		}
		// tcp port www start, compare only the lower byte
		if (buf[IP_PROTO_P]==IP_PROTO_TCP_V&&buf[TCP_DST_PORT_H_P]==0&&buf[TCP_DST_PORT_L_P]==MYWWWPORT){
			if (buf[TCP_FLAGS_P] & TCP_FLAGS_SYN_V){
				make_tcp_synack_from_syn(buf);
				// make_tcp_synack_from_syn does already send the syn,ack
				continue;
			}
			if (buf[TCP_FLAGS_P] & TCP_FLAGS_ACK_V){
				init_len_info(buf); // init some data structures
				// we can possibly have no data, just ack:
				dat_p=get_tcp_data_pointer();
				if (dat_p==0){
					if (buf[TCP_FLAGS_P] & TCP_FLAGS_FIN_V){
						// finack, answer with ack
						make_tcp_ack_from_any(buf);
					}
					// just an ack with no data, wait for next packet
					continue;
				}
				if (strncmp("GET ",(char *)&(buf[dat_p]),4)!=0){
					// head, post and other methods:
					//
					// for possible status codes see:
					// http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html
					plen=fill_tcp_data_p(buf,0,PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n<h1>200 OK</h1>"));
					goto SENDTCP;
				}
				if (strncmp("/ ",(char *)&(buf[dat_p+4]),2)==0){
					plen=print_webpage(buf);
					goto SENDTCP;
				}
				cmd=analyse_get_url((char *)&(buf[dat_p+5]));
				pagelock=1; // stop automatic actions until webpage is displayed
				//
				// error, default, will get overwritte in case 
				// something else is selected:
				plen=fill_tcp_data_p(buf,0,PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n<h1>Error</h1>"));
				plen=fill_tcp_data(buf,plen,errmsg);
				plen=fill_tcp_data_p(buf,plen,PSTR("<br><br><a href=/>-&gt;continue</a>\n"));
				// for possible status codes see:
				// http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html
				if (cmd==1){
					resethost();
					plen=print_webpage_confirm(buf);
				}
				if (cmd==2){
					store_in_eeprom();
					plen=print_webpage_confirm(buf);
				}
				if (cmd==7){
					store_in_eeprom();
					plen=print_webpage_confirm(buf);
					reinitmac=1;
				}
				// start and stop of wd, this is not store in eeprom
				if (cmd==5){
					pingtimer=1; // wd on 
					plen=print_webpage_confirm(buf);
				}
				if (cmd==4){
					pingtimer=0; // wd off 
					plen=print_webpage_confirm(buf);
				}
				if (cmd==3){
					plen=print_webpage_now(buf);
				}
				if (cmd==6){
					plen=print_webpage_config(buf);
				}
				// testing the temp OK / NOK
				if (cmd==20){
					sm[0].temp= atoi(strbuf);
					plen=print_webpage_confirm(buf);
				}
				if (cmd==21){
					sm[1].temp= atoi(strbuf);
					plen=print_webpage_confirm(buf);
				}
				
				// on / off - slave-modules
				// ON slave-module-1
				if (cmd==34){
					sm[0].status = atoi(strbuf);
					plen=print_webpage_confirmrm1(buf);
				}
				// OFF slave-module-1
				if (cmd==31){
					sm[0].status = atoi(strbuf);
					plen=print_webpage_confirmrm1(buf);
				}
				// ON slave-module-2
				if (cmd==32){
					sm[1].status = atoi(strbuf);
					plen=print_webpage_confirmrm2(buf);
				}
				// OFF slave-module-2
				if (cmd==33){
					sm[1].status = atoi(strbuf);
					plen=print_webpage_confirmrm2(buf);
				}
				
				if (cmd==35){
					plen=print_webpage_confirmclk(buf);
				}
				if (cmd==36){
					plen=print_webpage_confirmclk(buf);
				}
				
				// Room-1
				if (cmd==10){
					plen=print_webpage_room1(buf);
				}
				// ROom-2
				if (cmd==11){
					plen=print_webpage_room2(buf);
				}
				// Clock & date
				if (cmd==12){
					plen=print_webpage_clk(buf);
				}
				
				//
				SENDTCP:
				make_tcp_ack_from_any(buf); // send ack for http get
				make_tcp_ack_with_data(buf,plen); // send data
				continue;
			}
			
		}
		// tcp port www end
		//
		// udp not implemented
	}
	return (0);
}

