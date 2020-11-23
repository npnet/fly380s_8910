#ifndef __BROAD_OPEN_TYPE_H__
#define __BROAD_OPEN_TYPE_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int			bnd_uid_t;
typedef unsigned int			bnd_gid_t;
typedef unsigned char			boolean;

#ifndef NULL
#define NULL    ((void *)0)
#endif
#ifndef TRUE
#define TRUE    1
#endif
#ifndef FALSE
#define FALSE    0
#endif



#define USER_ONLINE 	1
#define USER_OFFLINE 	2
#define USER_INGROUP	3
#define BRD_NAME_LEN 64

typedef enum BND_CUSTOM_TYPE{
	ACCOUNT_LOGIN_ENABLE,
	WRITE_CUSTOM_ACCOUNT,
	QUERY_ALL_GROUP_MEMBER,
}BND_CUSTOM_TYPE;

//群组类型
typedef enum BRD_GROUP_TYPE {
	GRP_COMMON,
	GRP_SINGLECALL
} BRD_GROUP_TYPE;

//群组结构
typedef struct bnd_group_t
{
	bnd_gid_t		gid;		//群组ID
	char		name[BRD_NAME_LEN];		//群组名
	BRD_GROUP_TYPE	type;		//群组类型
	int			index;
} bnd_group_t;

typedef struct _bnd_member_t {
	bnd_uid_t uid;
	char name[BRD_NAME_LEN];
	int state; //1在线，2,离线，3在线在组
	unsigned int prior;		///< 优先级
	int index;
}bnd_member_t;

typedef struct bnd_time_t {
	unsigned short	year;
	unsigned char	month;
	unsigned char	day;
	unsigned char	hour;
	unsigned char	minute;
	unsigned char	second;
	unsigned short	millisecond;
} bnd_time_t;

#ifdef __cplusplus
}
#endif
#endif

