/*Copyright (C) 2010-2013 Motorola, Inc.
 *All Rights Reserved.
 *Motorola Confidential Restricted (MCR).
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cutils/log.h>
#include "linux/msp430.h"

/******************************* # defines **************************************/
#define MSP_DRIVER "/dev/msp430"
#define MSP_FIRMWARE_FILE "/system/etc/firmware/mspfirmware"
#define MSP_VERSION_FILE "/system/etc/firmware/mspversion"
#define MSP_FIRMWARE_FACTORY_FILE "/system/etc/firmware/mspfirmwarefactory.bin"
#define MSP_SUCCESS 0
#define MSP_FAILURE -1
#define MSP_VERSION_MISMATCH -1
#define MSP_VERSION_MATCH 1
#define MSP_DOWNLOADRETRIES 3
#define MSP_MAX_PACKET_LENGTH 256
/* 512 matches the read buffer in kernel */
#define MSP_MAX_GENERIC_DATA 512
#define MSP_MAX_GENERIC_HEADER 4
#define MSP_MAX_GENERIC_COMMAND_LEN 3
#define MSP_FORCE_DOWNLOAD_MSG  "Use -f option to ignore version check eg: msp430 boot -f\n"
#define FLASH_START_ADDRESS 0x08000000


#define CHECK_RETURN_VALUE( ret, msg)  if (ret < 0) {\
					 ALOGE("%s: %s \n",msg, strerror(errno)); \
					 printf("%s: %s \n",msg, strerror(errno)); \
					 goto EXIT; \
					    }

#define CHECKIFHEX(c)  ((c >= 'A' && c <= 'F') || ( c >= '0' && c <='9') || ( c >= 'a' && c <= 'f'))

#define LOGERROR(format, ...) {\
		ALOGE(format,## __VA_ARGS__); \
		printf(format,##__VA_ARGS__); \
}

#define LOGINFO(format, ...) {\
		ALOGI(format,## __VA_ARGS__); \
		printf(format,##__VA_ARGS__); \
}

#ifdef _DEBUG
#define DEBUG(format, ...) ALOGE(format,## __VA_ARGS__);
#else
#define DEBUG(format, ...) ;
#endif

/****************************** structures & enums *****************************/
typedef enum tag_mspmode
{
	BOOTLOADER,
	BOOTFACTORY,
	NORMAL,
	DEBUG,
	FACTORY,
	VERSION,
	TBOOT,
	TREAD,
	TWRITE,
	TMWRITE,
	TMWRRD,
	TUSER_PROFILE,
	TACTIVE_MODE,
	TPASSIVE_MODE,
	READWRITE,
	//LOWPOWER_MODE,
	INVALID
}eMsp_Mode;

/****************************** function defitions ****************************/
int msp_version_check(int fd, bool check)
{
	FILE * vfp = NULL;
	int ret = MSP_VERSION_MISMATCH;
	int newversion;
	int oldversion;
	int temp;
	char ver_string[FW_VERSION_SIZE];
	char ver_file_name[256];

	/*check if a version check is required */
	if ( check == false)
		return MSP_VERSION_MISMATCH;

	/* read new version number from version file*/
	ioctl(fd, MSP430_IOCTL_GET_VERNAME, ver_string);
	sprintf(ver_file_name, "%s%s.txt", MSP_VERSION_FILE, ver_string);
	DEBUG("MSP430 version file name %s\n", ver_file_name);
	vfp = fopen(ver_file_name,"r");
	if(vfp == NULL) {
		LOGERROR(" version file not found at %s\n", ver_file_name)
		DEBUG(MSP_FORCE_DOWNLOAD_MSG);
		return ret;
	}
	fscanf(vfp, "%02x", &newversion);

	/* get old version from firmware */
	oldversion = ioctl(fd, MSP430_IOCTL_GET_VERSION, &temp);

	/* check if the version in hardware is older */
	if( oldversion < newversion)
		ret = MSP_VERSION_MISMATCH;
	else {
		DEBUG(MSP_FORCE_DOWNLOAD_MSG);
		ret = MSP_VERSION_MATCH;
	}

	LOGINFO("Version info: version in filesystem = %d, version in hardware = %d\n",newversion, oldversion)

	fclose(vfp);
	return ret;
}

int msp_convertAsciiToHex(char * input, unsigned char * output, int inlen)
{
	int i=0,outlen=0,x,result;

	if (input != NULL && output != NULL) {
		while(i < inlen) {
			result = sscanf(input+i,"%02x",&x);
			if (result != 1) {
				break;
			}
			output[outlen++] = (unsigned char)x;
			i= i+2;
		}
	}
	return outlen;
}

int msp_getpacket( FILE ** filepointer, unsigned char * databuff)
{
	FILE * fp = *filepointer;
	int len = 0;
	unsigned char c;

	while(1){
		c = fgetc(fp);
		if (feof(fp))
			break;
		databuff[len] = c;
		len++;
		if( len >= MSP_MAX_PACKET_LENGTH) {
			break;
		}
	}

	return len;

}

int msp_downloadFirmware( int fd, FILE *filep)
{

	unsigned int address;
	int ret = MSP_SUCCESS;
	int packetlength;
#ifdef _DEBUG
	int packetno = 0;
#endif
	unsigned char packet[MSP_MAX_PACKET_LENGTH];
	int temp = 100; // this is only a dummy variable for the 3rd parameter of ioctl call

	DEBUG("Ioctl call to switch to bootloader mode\n");
	ret = ioctl(fd, MSP430_IOCTL_BOOTLOADERMODE, &temp);
	CHECK_RETURN_VALUE(ret,"Failed to switch MSP to bootloader mode\n");

	DEBUG("Ioctl call to erase flash on MSP\n");
	ret = ioctl(fd, MSP430_IOCTL_MASSERASE, &temp);
	CHECK_RETURN_VALUE(ret,"Failed to erase MSP \n");

	address = FLASH_START_ADDRESS;
	ret = ioctl(fd, MSP430_IOCTL_SETSTARTADDR, &address);
	CHECK_RETURN_VALUE(ret,"Failed to set address\n");

	DEBUG("Start sending firmware packets to the driver\n");
	do {
		packetlength = msp_getpacket (&filep, packet);
		if( packetlength == 0)
			break;
#ifdef _DEBUG
		DEBUG("Sending packet %d  of length %d:\n", packetno++, packetlength);
		int i;
		for( i=0; i<packetlength; i++)
			DEBUG("%02x ",packet[i]);
#else
		printf(".");
		fflush(stdout);
#endif
		ret = write(fd, packet, packetlength);
		CHECK_RETURN_VALUE(ret,"Packet download failed\n");
	} while(packetlength != 0);

EXIT:
	return ret;
}

int  main(int argc, char *argv[])
{

	int fd = -1, tries, ret = MSP_SUCCESS;
	FILE * filep = NULL;
	eMsp_Mode emode = INVALID;
	int temp = 100; // this is only a dummy variable for the 3rd parameter of ioctl call
	unsigned char hexinput[250];
	int count, i;
	short delay = 0;
	int enabledints = 0;
	bool versioncheck = true;
	char ver_string[FW_VERSION_SIZE];
	char fw_file_name[256];

	DEBUG("Start MSP430  Version-1 service\n");

	/*parse command line arguements */
	if(!strcmp(argv[1], "boot"))
		emode = BOOTLOADER;
	else if( !strcmp(argv[1], "bootfactory"))
		emode = BOOTFACTORY;
	else if( !strcmp(argv[1], "normal"))
		emode = NORMAL;
	else if(!strcmp(argv[1], "tboot"))
		emode = TBOOT;
        else if(!strcmp(argv[1], "tread"))
                emode = TREAD;
        else if(!strcmp(argv[1], "twrite"))
                emode = TWRITE;
	else if(!strcmp(argv[1], "tmread"))
		emode = TMWRRD;
	else if(!strcmp(argv[1], "tmwrite"))
		emode = TMWRITE;
	else if(!strcmp(argv[1], "debug"))
		emode = DEBUG;
	else if(!strcmp(argv[1], "udata"))
		emode = TUSER_PROFILE;
	else if(!strcmp(argv[1], "factory"))
		emode = FACTORY;
	else if(!strcmp(argv[1], "active"))
		emode = TACTIVE_MODE;
	else if(!strcmp(argv[1], "passive"))
		emode = TPASSIVE_MODE;
	else if(!strcmp(argv[1], "getversion"))
		emode = VERSION;
	else if(!strcmp(argv[1], "readwrite"))
		emode = READWRITE;
	/*else if(!strcmp(argv[1], "lowpower"))
		emode = LOWPOWER_MODE;*/

	/* check if its a force download */
	if ((emode == BOOTLOADER || emode == BOOTFACTORY) && (argc == 3)) {
		if(!strcmp(argv[2], "-f"))
			versioncheck = false;
	}

	/* open the device */
	fd = open(MSP_DRIVER,O_RDONLY|O_WRONLY);
	if( fd < 0) {
		LOGERROR("Unable to open msp430 driver: %s\n",strerror(errno))
		ret = MSP_FAILURE;
		goto EXIT;
	}


	if ((emode == BOOTLOADER) || (emode == BOOTFACTORY)) {
		if (emode == BOOTLOADER) {
			ret = ioctl(fd, MSP430_IOCTL_GET_VERNAME, ver_string);
			sprintf(fw_file_name, "%s%s.bin", MSP_FIRMWARE_FILE, ver_string);
			LOGINFO("MSP430 file name %s\n", fw_file_name)
			filep = fopen(fw_file_name,"r");
		}
		else
			filep = fopen(MSP_FIRMWARE_FACTORY_FILE,"r");

		/* check if new firmware available for download */
		if( (filep != NULL) && (msp_version_check(fd, versioncheck) == MSP_VERSION_MISMATCH)) {
	        tries = 0;
	       		while((tries < MSP_DOWNLOADRETRIES )) {
				if( (msp_downloadFirmware(fd, filep)) >= MSP_SUCCESS) {
					fclose(filep);
					filep = NULL;
					/* reset MSP */
					if (emode == BOOTLOADER) {
						ret = ioctl(fd, MSP430_IOCTL_NORMALMODE, &temp);
						printf("\n");
					    if (msp_version_check(fd, true) == MSP_VERSION_MATCH)
						    LOGINFO("Firmware download completed successfully\n")
					    else
						    LOGERROR("Firmware download error\n")

					}
					else
						emode = FACTORY;

					break;
				}
				//point the file pointer to the beginning of the file for the next try
				tries++;
				fseek(filep, 0, SEEK_SET);
				// Need to use sleep as msleep is not available
				sleep(1);
	        	}

			if( tries >= MSP_DOWNLOADRETRIES ) {
				LOGERROR("Firmware download failed.\n")
				ret = MSP_FAILURE;
				ioctl(fd,MSP430_IOCTL_NORMALMODE, &temp);
			}
		} else {
			DEBUG("No new firmware to download \n");
			/* reset MSP in case for soft-reboot of device */
			if (emode == BOOTLOADER)
				emode = NORMAL;
			else
				emode = FACTORY;
		}
	}
	if(emode == NORMAL) {
		DEBUG("Ioctl call to reset MSP\n");
		ret = ioctl(fd,MSP430_IOCTL_NORMALMODE, &temp);
		CHECK_RETURN_VALUE(ret, "MSP reset failed");
	}
	if( emode == TBOOT) {
		DEBUG("Ioctl call to send MSP to boot mode\n");
                ret = ioctl(fd,MSP430_IOCTL_TEST_BOOTMODE, &temp);
                CHECK_RETURN_VALUE(ret, "MSP not in bootloader mode");
	}
	if( emode == TREAD) {
		DEBUG("Test read\n");
		// get the register to read from
                msp_convertAsciiToHex(argv[2],hexinput,strlen(argv[2]));
		DEBUG( "%02x: ", hexinput[0]);
                ret = ioctl(fd,MSP430_IOCTL_TEST_WRITE,hexinput);

		// get the number of bytes to be read
		msp_convertAsciiToHex(argv[3],hexinput,strlen(argv[3]));
		DEBUG( "count = %02x: \n ", hexinput[0]);

		for( i= 0; i< hexinput[0]; i++) {
			ret = ioctl(fd,MSP430_IOCTL_TEST_READ, &temp);
			DEBUG( "%02x ", ret);
		}
	}
	if( emode == TWRITE) {
		DEBUG(" Test write\n");
		for( i=0; i< (argc-2); i++) {
			msp_convertAsciiToHex(argv[i+2],hexinput,strlen(argv[i+2]));
			ret = ioctl(fd,MSP430_IOCTL_TEST_WRITE,hexinput);
			if (ret >= 0) {
				DEBUG( "%02x", hexinput[0]);
			} else {
				DEBUG( "TWrite Error %02x\n", ret);
			}
		}
	}
	if( emode == TMWRITE) {
		count = argc-2;
		DEBUG(" Writing data: ");
		for( i=0; i< count; i++) {
			msp_convertAsciiToHex(argv[i+2],hexinput+i,strlen(argv[i+2]));
			DEBUG(" %02x",hexinput[i]);
		}
                DEBUG("\n");
		ret = write(fd,hexinput,count);
		if( ret != count) {
			DEBUG("Write FAILED\n");
		}
	}
	if( emode == TMWRRD) {
		DEBUG( " Read from address ");
		msp_convertAsciiToHex(argv[2],hexinput,strlen(argv[2]));
		DEBUG (" %02x, ",hexinput[0]);
		msp_convertAsciiToHex(argv[3],hexinput+1,strlen(argv[3]));
		DEBUG (" %02x bytes: \n",hexinput[1]);
		ret = ioctl(fd,MSP430_IOCTL_TEST_WRITE_READ,hexinput);
	}
	if( emode == VERSION) {
		msp_version_check(fd, versioncheck);
	}
	if( emode == DEBUG ) {
		DEBUG( " Set debug to ");
		msp_convertAsciiToHex(argv[2],hexinput,strlen(argv[2]));
		delay = hexinput[0];
		DEBUG(" %d\n", delay);
		ret = ioctl(fd,MSP430_IOCTL_SET_DEBUG,&delay);
		if (delay == 0) {
			system("echo 'file msp430.c -p' > /sys/kernel/debug/dynamic_debug/control");
		}
		else {
			system("echo 'file msp430.c +p' > /sys/kernel/debug/dynamic_debug/control");
		}
	}
	if( emode == FACTORY ) {
		DEBUG( "Switching to factory mode\n");
		ret = ioctl(fd,MSP430_IOCTL_SET_FACTORY_MODE, &temp);
	}
	if( emode == INVALID ) {
		LOGERROR("Invalid arguements passed: %d, %s\n",argc,argv[1])
		ret = MSP_FAILURE;
	}
	if (emode == READWRITE) {
	    //                    1B      2B       2B    ...
	    // msp430 readwrite [type] [address] [size] [data]
	    //
	    // read version example: msp430 readwrite 00 00 01 00 01
	    // write example:        msp430 readwrite 01 00 0D 00 02 CC DD

	    unsigned int arg_index = MSP_MAX_GENERIC_COMMAND_LEN;
	    unsigned char data_header[MSP_MAX_GENERIC_HEADER];
	    unsigned char *data_ptr;
	    int result;

	    if (argc < 7) {
	        printf("READWRITE : Not enough arguments,\n");
	        printf("msp430 readwrite [type] [address] [size] [data]\n");
	        printf("read version example: msp430 readwrite 00 00 01 00 01\n");
	        printf("write example:        msp430 readwrite 01 00 0D 00 02 CC DD\n");
		goto EXIT;
	    }

	    // read in the header 2 bytes address, 2 bytes data_size
	    DEBUG(" Header Input: ");
	    for( i=0; i < MSP_MAX_GENERIC_HEADER; i++) {
	        result = msp_convertAsciiToHex(argv[arg_index],data_header+i,
	                strlen(argv[arg_index]));
	        if (result != 1) {
	            printf("Header Input: msp_convertAsciiToHex failure\n");
		    goto EXIT;
	        }
	        DEBUG(" %02x",data_header[i]);
	        arg_index++;
	    }

	    // read_write, 0 = read, 1 = write
	    unsigned int read_write = atoi(argv[2]);
	    int addr = (data_header[0] << 8) | data_header[1];
	    int data_size = (data_header[2] << 8) | data_header[3];

	    if (data_size > MSP_MAX_GENERIC_DATA - 1) {
	        printf("Data size too large, must be <= %d\n", MSP_MAX_GENERIC_DATA - 1);
		goto EXIT;
	    } else if (data_size <= 0) {
	        printf("Data size invalid,\n");
		goto EXIT;
	    } else if (read_write && data_size != (argc - MSP_MAX_GENERIC_COMMAND_LEN
	             - MSP_MAX_GENERIC_HEADER)) {
	        printf("Not enough data provided,\n");
		goto EXIT;
	    }

	    // allocate data_ptr with read/write size + header
	    data_ptr = (unsigned char *)malloc(data_size + MSP_MAX_GENERIC_HEADER);
	    memset(data_ptr, 0, data_size + MSP_MAX_GENERIC_HEADER);

	    // copy header into data_ptr
	    int data_index = MSP_MAX_GENERIC_HEADER;
	    memcpy(data_ptr, data_header, MSP_MAX_GENERIC_HEADER);

	    // if writing, read in the data
	    if (read_write) {
	        DEBUG(" READWRITE Data Input:");
	        for( i=0; i < data_size; i++) {
	            result = msp_convertAsciiToHex(argv[arg_index],
	                data_ptr + data_index,
	                strlen(argv[arg_index]));
	            if (result != 1) {
	                printf("Data Input: msp_convertAsciiToHex failure\n");
	                free(data_ptr);
		        goto EXIT;
	            }
	            arg_index++;
	            data_index++;
	            DEBUG(" %02x",data_ptr[i]);
	        }
	    }

	    if (read_write) {
	        ret = ioctl(fd,MSP430_IOCTL_WRITE_REG,data_ptr);
	        DEBUG ("Writing data returned: %d", ret);
	    } else {
	        ret = ioctl(fd,MSP430_IOCTL_READ_REG,data_ptr);

	        DEBUG ("Read data:");
	        for ( i = 0; i < data_size; i++) {
	            DEBUG (" %02x", data_ptr[i]);
	        }
	    }

	    free(data_ptr);
	}
	/*if(emode == LOWPOWER_MODE) {
	    unsigned int setting = atoi(argv[2]);
	    if (setting == 0 || setting == 1) {
		LOGINFO(" lowpower mode set to: %d\n", setting);
		ret = ioctl(fd, MSP430_IOCTL_SET_LOWPOWER_MODE, &setting);
	    } else {
		LOGERROR(" lowpower mode incorrect setting\n");
		ret = MSP_FAILURE;
	    }
	}*/

EXIT:
	if( ret < MSP_SUCCESS)
		LOGERROR(" Command execution error \n")
	close(fd);
	if( filep != NULL)
		fclose(filep);
	return ret;
}
