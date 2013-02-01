#include <stdio.h>
#include <string.h>
struct student
{
    int        nID;    //ѧ��
    char    chName[20];    //����
    float    fScores[3];    //3�ſεĳɼ�
};
void main()
{
    FILE *pRead,*pWrite;
    struct student tStu[4];
    struct student *ptStu = NULL;
    int nCount = 0;

    //ASCII��ʽ���ļ� ���ڶ���
    pRead=fopen("txt","r");
    if(NULL == pRead)
    {
        return;
    }

    //�������ļ����ļ� ����д��
    pWrite=fopen("stu_scores_bin.txt","wb");
    if(NULL == pWrite)
    {
        fclose(pRead);
        return;
    }

    //fscanf��ȡ���ݣ�fwriteд������
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

    memset(tStu,0x00,sizeof(tStu)); //�������

    //�������ļ����ļ� ���ڶ�ȡ
    pRead=fopen("stu_scores_bin.txt","rb");
    if(NULL == pRead)
    {
        printf("open file stu_scores_bin.txt failed");
        return;
    }

    //����������fread�Ķ����ݷ�ʽ���������1����0����ʹ�õڶ��ַ�ʽ
#if 1
    //һ�����Ķ�ȡ
    ptStu = tStu;
    nCount = fread(ptStu,sizeof(struct student),1,pRead);
    while(nCount>0)
    {
        printf("%d %s %.1f %.1f %.1f\n",ptStu->nID,ptStu->chName,ptStu->fScores[0],ptStu->fScores[1],ptStu->fScores[2]);
        ptStu++;
        nCount = fread(ptStu,sizeof(struct student),1,pRead);
    }
#else
    //��Ϊ����֪����4����Ϣ����˿���ֱ�Ӷ�ȡ������Ϣ
    fread(tStu,sizeof(struct student),4,pRead);
    for(nCount=0; nCount<4; nCount++)
    {
        printf("%d %s %.1f %.1f %.1f\n",tStu[nCount].nID,tStu[nCount].chName,tStu[nCount].fScores[0],tStu[nCount].fScores[1],tStu[nCount].fScores[2]);
    }
#endif

    fclose(pRead);
}
