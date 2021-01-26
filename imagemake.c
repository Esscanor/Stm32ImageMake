#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include "crc32.h"

#define PATH_MAX_SIZE         1024
#define DEFAULT_CFG_FILE      "ImageMake.cfg"
#define DEFAULT_APP_NAME      "app.bin"
#define DEFAULT_BOOT_NAME     "boot.bin"
#define DEFAULT_OUTPUT_NAME   "output.bin"
#define IMAGEMAKE_VERSION     "1.5"
#define FlASH_BLOCK_SIZE      1024

typedef struct SegAttribute
{
    char*    name[16];
    uint32_t Addr_Start;
    uint32_t Segment_Len;
    uint32_t Segment_Crc;
}SegAttribute_t;


typedef struct PackInfo 
{
    char       Segment_Num;
    uint32_t   PackInfo_Crc;
    SegAttribute_t Segment[3];
}PackInfo_t;

static const struct option long_options[]=      //long option
{
    {"help",no_argument,NULL,'?'},
    {"app",required_argument,NULL,'a'},
    {"boot",required_argument,NULL,'b'},
    {"ouput",required_argument,NULL,'o'},
    {"flash",required_argument,NULL,'f'},
};

void usage(void)
{
    printf( "./ImageMake opt:\r\n"
            "exapmle:   ./ImageMake -a app.bin -b boot.bin -o 357A.bin -f 1024\r\n"
    );
}


int file_size(FILE* fp) 
{ 
    int size = 0;
    
    fseek(fp,0L,SEEK_END); 
    size=ftell(fp);
    return size; 
}

uint32_t file_crc32(FILE* fp)
{
    char        buf[FlASH_BLOCK_SIZE];
    int         ret = 0;
    uint32_t     u32Crc = 0;

    fseek(fp,0L,SEEK_SET); 
    
    while( feof(fp) == 0)
    {
        ret  = fread(buf, 1, FlASH_BLOCK_SIZE, fp);
         u32Crc = Crc32_CalcBlock(u32Crc,(uint8_t *)buf,ret);
    }
    return u32Crc;
}


int Flash_BlockSize = FlASH_BLOCK_SIZE;

int main(int argc,char *argv[])
{
    char    Path_Current[PATH_MAX_SIZE] = {0};
    char    App_Name[PATH_MAX_SIZE] = {DEFAULT_APP_NAME};
    char    Boot_Name[PATH_MAX_SIZE] = {DEFAULT_BOOT_NAME};
    char    Output_Name[PATH_MAX_SIZE] = {DEFAULT_OUTPUT_NAME};
    int     i = 0;
    int     Ret = 0;
    int     Opt = 0;                    
    int     Options_index= 0;        //选项的索引号  
    int     size = 0;
    int     pos = 0;
    char    Temp = 0;
    uint8_t buf[ FlASH_BLOCK_SIZE *2 ] = {0};
    FILE*   Fp_Output = NULL;
    FILE*   Fp_App    = NULL;
    FILE*   Fp_Boot   = NULL;
    SegAttribute_t SegAttr[3] = {0};
    PackInfo_t  PackInfo = {0};

    printf("Start ImageMake...\r\n");
    if(argc == 1)
    {
        printf("Use default parameters...\r\n"); 
        usage();
        Ret = -__LINE__;
        goto _exit;
    }
    else
    { 
        getcwd(Path_Current, PATH_MAX_SIZE);
        /* get the current file path */
        strcat(Path_Current, "/");
        printf("Current file path:%s\r\n",Path_Current);

        while((Opt=getopt_long(argc,argv,"f:a:b:o:?h",long_options,&Options_index))!=EOF ) /* 分析参数选项*/
        {
            switch(Opt)
            {
                case 'f':
                    Flash_BlockSize = atoi(optarg);
                    break;

                case 'a':
                    strcpy( App_Name,Path_Current );
                    strcat( App_Name,optarg );
                    strcpy( (char *)SegAttr[1].name, optarg );
                    break;

                case 'b':
                    strcpy( Boot_Name,Path_Current);
                    strcat( Boot_Name,optarg);
                    strcpy( (char *)SegAttr[0].name, optarg);
                    break;

                case 'o':
                    strcpy( (char *)SegAttr[2].name, optarg);
                    strcpy( Output_Name,Path_Current);
                    strcat( Output_Name,optarg);
                    break;

                case 'h': 
                case '?': 
                    usage();
                    return 2;
                    break;
            }  
        }
    }


    Fp_Output = fopen(Output_Name,"wt+");
    if(Fp_Output == NULL)
    {
        printf("open file failed: %s\n\r",Output_Name);
        Ret = -__LINE__;
        goto _exit;
    }
  
    Fp_App = fopen(App_Name,"rt+");
    if(App_Name == NULL)
    {
        printf("open file failed: %s\n\r",App_Name);
        Ret = -__LINE__;
        goto _exit;
    }

    Fp_Boot = fopen(Boot_Name,"rt+");
    if(Fp_Boot == NULL)
    {
        printf("open file failed: %s\n\r",Boot_Name);
        Ret = -__LINE__;
        goto _exit;
    }

    SegAttr[0].Addr_Start  = 0;  
    SegAttr[0].Segment_Len = file_size(Fp_Boot);  
    SegAttr[0].Segment_Crc = file_crc32(Fp_Boot);
    SegAttr[1].Segment_Len = file_size(Fp_App);
    SegAttr[1].Segment_Crc = file_crc32(Fp_App);

    /* copy boot */
    fseek(Fp_Boot, SegAttr[0].Addr_Start,SEEK_SET );
    while( feof(Fp_Boot) == 0)
    {
        Ret  = fread(buf, 1,Flash_BlockSize , Fp_Boot);
        fwrite(buf, Ret, 1, Fp_Output);
    }
    size  = ( ( SegAttr[0].Segment_Len /Flash_BlockSize) + 1 ) * Flash_BlockSize;
    pos = ftell(Fp_Output);
    while(size != pos+1)
    {
        fwrite(&Temp,1,1,Fp_Output);
        pos = ftell(Fp_Output);
    }

    /* copy app */
    fseek(Fp_App, 0,SEEK_SET );
    SegAttr[1].Addr_Start = ftell(Fp_Output);
    while( feof(Fp_App) == 0)
    {
        Ret  = fread(buf,1 , Flash_BlockSize, Fp_App);
        fwrite(buf, Ret, 1, Fp_Output);
    }
    size  =   SegAttr[1].Addr_Start + (( ( SegAttr[1].Segment_Len /Flash_BlockSize) + 1 ) * Flash_BlockSize);
    pos = ftell(Fp_Output);
    while(size != pos)
    {
       
        fwrite(&Temp,1,1,Fp_Output);
        pos = ftell(Fp_Output);
    }

    SegAttr[2].Addr_Start = 0;
    SegAttr[2].Segment_Len = pos;
    SegAttr[2].Segment_Crc =  file_crc32(Fp_Output);

    if(sizeof(PackInfo) >  (uint32_t)Flash_BlockSize)
    {
        printf("the flash block size is too small\n");
        Ret = -__LINE__;
        goto _exit;
    }
   
    /* write packinfo */
    fseek(Fp_Output, Flash_BlockSize - sizeof(PackInfo),SEEK_CUR);
    PackInfo.Segment_Num = 3;
    PackInfo.PackInfo_Crc = SegAttr[2].Segment_Crc;
    memcpy(PackInfo.Segment,SegAttr,sizeof(SegAttribute_t) * PackInfo.Segment_Num  );
    fwrite((char *)&PackInfo, 1, sizeof(PackInfo), Fp_Output);

    for( i = 0; i< 3; i++)
    {
        printf("SegAttr[%d]name: %s\n",i,(char *)SegAttr[i].name);
        printf("SegAttr[%d]Addr_Start: %d\n",i,SegAttr[i].Addr_Start);
        printf("SegAttr[%d]Segment_Len: %d\n",i,SegAttr[i].Segment_Len);
        printf("SegAttr[%d]Segment_Crc: %8x\n",i,SegAttr[i].Segment_Crc);
        printf("\n");
    }

    Ret = 0;
_exit:
    fclose(Fp_Output);
    fclose(Fp_App);
    fclose(Fp_Boot);
    printf("ret = %d\r\n", Ret);
    return Ret;
}