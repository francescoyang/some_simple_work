#include <stdio.h>
#include <string.h>
typedef struct senddate{
	char  name[6];
	char sex[3];
	int age;
	char hobby[10];
	char birthday[10];
}senddate_t;

typedef struct __blogsinfo{
	char  name[6];
	char sex[3];
	int age;
	char hobby[10];
	char birthday[10];
}blogsinfo_t;
typedef struct __personinfo {
	char  name[6];
	char sex[3];
	int age;
	char hobby[10];
	char birthday[10];
}personinfo_t;

int main()
{
	char data[1024];
	struct senddate date;
	memcpy(date.name,"acanoe",strlen("acanoe"));
	memcpy(date.sex,"man",strlen("man"));
	date.age = 22;
	memcpy(date.hobby,"Linux_c",strlen("Linux_c"));
	memcpy(date.birthday ,"19900305",strlen("19900305"));

	memcpy(data,&date,sizeof(senddate_t));

	personinfo_t personinfo;

	memset(&personinfo,0,sizeof(personinfo_t));

	blogsinfo_t *blogsinfo=(blogsinfo_t *)data;

	memcpy(personinfo.name, blogsinfo->name, 6);
	printf("personinfo_name is %s\n",personinfo.name);

	memcpy(personinfo.sex, blogsinfo->sex, 3);
	printf("personinfo_sex is %s\n",personinfo.sex);

	personinfo.age = blogsinfo->age;
	printf("personinfo_age is %d\n",personinfo.age);

	memcpy(personinfo.hobby, blogsinfo->hobby, 10);
	printf("personinfo_hobby is %s\n",personinfo.hobby);

	memcpy(personinfo.birthday, blogsinfo->birthday, 10);
	printf("personinfo_birthday is %s\n",personinfo.birthday);
}
