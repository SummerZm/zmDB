#ifndef _ZM_STORAGE_KV_H_
#define _ZM_STORAGE_KV_H_

#define ZM_KV_DB_CAPACITY                   300
#define ZM_KV_DB_CAPACITY_INCREASE_STEP     60
#define ZM_KV_DB_PATH_LEN                   1024
#define ZM_KV_DB_NAME_LEN                   1024
#define ZM_KV_DB_RECORD_LEN                 1024
#define ZM_KV_DB_RECORD_TIME_LEN            64
#define ZM_KV_DB_RECORD_DATA_KEY            1024
#define ZM_KV_DB_RECORD_DATA_VAL            1024
#define ZM_KV_DB_RECORD_DATA_LEN            1024
#define ZM_KV_DB_RECORD_FILE_FORMAT             "{\"Time\":\"%[^\"]\",\"id\":\"%d\",\"Data\":{%[^}]}}"
#define ZM_KV_DB_RECORD_FILE_FORMAT_DISPLAY     "{\"Time\":\"%s\",\"id\":\"%d\",\"Data\":%s}\r\n"
#define ZM_KV_DB_RECORD_HTTP_FORMAT             "%[^_]_%[^_]_%[^_]_%[^_]"
#define ZM_KV_DB_RECORD_HTTP_FORMAT_DISPLAY     "%s_%s_%s_%s|"
#define ZM_KV_DB_RECORD_UDP_FORMAT              "Action:WebNotify\r\nInterCmd:Data\r\nid:%s\r\nname:%s\r\ngender:%s\r\nage:%s\r\nMsgID:%s\r\n|"

typedef enum zmKVDBDataFormat{
    KV_FILE_FORMAT= 0,
}zmKVDBDataFormat;

/*
 * @start: primary id start.
 * @end: primary id start.
 * @format: data dispaly format.
 */
typedef struct zmKVDBQuery {
    int all;    // 0/1
    int start;
    int end;
    int capacity;   // The size of result.
    int used;
    zmKVDBDataFormat format;   // data display format.
    char* result;   // To storage primary id.
} zmKVDBQuery;

/* KV: Key and Value */
typedef struct zmKV {
    int kLen;
    int vLen;

    char* key;
    char* val;

    struct zmKV* next;
} zmKV;

/* KV: record */
typedef struct zmKVRecord {
    zmKV head;
    int primary;
    int idPos;
} zmKVRecord;

typedef struct zmKVContainer {
    int size;
    int capacity;
    char path[ZM_KV_DB_PATH_LEN];
    zmKVRecord* (*table)[];
}zmKVContainer;

typedef int (*zmKVDBDataAnalizer)(zmKVRecord* record, const char* data);
typedef int (*zmKVDBDataDisplayer)(zmKVRecord* record, char* buffer, int len);

int initZmKVDBConf(zmDBConf* conf, const char* name, zmDBDataFormat format, const char* path);
int initzmKVDBQuery(zmKVDBQuery* query, int start, int end, zmKVDBDataFormat format, int len);
zmKVRecord* createZMKVDBRecord(const char* data, zmDBDataFormat format, int idPos);
#endif
