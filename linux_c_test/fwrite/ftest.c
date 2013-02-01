#include <stdio.h>
#include <string.h>
struct student
{
    int        nID;    //学号
    char    chName[20];    //姓名
    float    fScores[3];    //3门课的成绩
};
void main()
{
    FILE *pRead,*pWrite;
    struct student tStu[4];
    struct student *ptStu = NULL;
    int nCount = 0;

    //ASCII方式打开文件 用于读入
    pRead=fopen("txt","r");
    if(NULL == pRead)
    {
        return;
    }

    //二进制文件打开文件 用于写入
    pWrite=fopen("stu_scores_bin.txt","wb");
    if(NULL == pWrite)
    {
        fclose(pRead);
        return;
    }

    //fscanf读取数据，fwrite写入数据
    ptStu = tStu;
    while(!feof(pRead))
    {
        fscanf(pRead,"%d %s %f %f %f\n",&ptStu->nID,ptStu->chName,&ptStu->fScores[0],&ptStu->fScores[1],&ptStu->fScores[2]);
        fwrite(ptStu,sizeof(struct student),1,pWrite);
        printf("%d %s %.1f %.1f %.1f\n",ptStu->nID,ptStu->chName,ptStu->fScores[0],ptStu->fScores[1],ptStu->fScores[2]);
        ptStu++;
    }

    fclose(pRead);
    fclose(pWrite);

    memset(tStu,0x00,sizeof(tStu)); //清空数据

    //二进制文件打开文件 用于读取
    pRead=fopen("stu_scores_bin.txt","rb");
    if(NULL == pRead)
    {
        printf("open file stu_scores_bin.txt failed");
        return;
    }

    //下面有两种fread的读数据方式，将下面的1换成0，则使用第二种方式
#if 1
    //一条条的读取
    ptStu = tStu;
    nCount = fread(ptStu,sizeof(struct student),1,pRead);
    while(nCount>0)
    {
        printf("%d %s %.1f %.1f %.1f\n",ptStu->nID,ptStu->chName,ptStu->fScores[0],ptStu->fScores[1],ptStu->fScores[2]);
        ptStu++;
        nCount = fread(ptStu,sizeof(struct student),1,pRead);
    }
#else
    //因为事先知道有4条信息，因此可以直接读取四条信息
    fread(tStu,sizeof(struct student),4,pRead);
    for(nCount=0; nCount<4; nCount++)
    {
        printf("%d %s %.1f %.1f %.1f\n",tStu[nCount].nID,tStu[nCount].chName,tStu[nCount].fScores[0],tStu[nCount].fScores[1],tStu[nCount].fScores[2]);
    }
#endif

    fclose(pRead);
}
