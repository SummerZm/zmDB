#include "../zm_utils.h"
#include "zm_storage_kv.h"

#define ZM_KV_DB_KEY_ID         "id"
#define ZM_KV_DB_KEY_NAME       "name"
#define ZM_KV_DB_KEY_GENDER     "gender"
#define ZM_KV_DB_KEY_AGE        "age"
#define ZM_KV_DB_KEY_MSGID      "MsgID"
#define ZM_KV_DB_NULL           "null"


/*
 * To make sure containor size is enough or not to big.
 */
int _containAdpter(zmKVContainer* containor, int primary) {
    int ret = 0;
    // todo
    return ret;
}

/*
 *  NOTE: You should change the return val, because it's const.
 */
const char* getzmKVRecordVal(zmKVRecord* record, const char* key) {
    int len = 0;
    const zmKV* temp = NULL;
    const char* val = NULL;
    if (key==NULL || record==NULL) {
        ZM_LOG("record or key is null\n");
        return val;
    }

    len = strlen(key);
    temp = &(record->head);
    while (temp->next) {
        if (len==temp->next->kLen && strncmp(key, temp->next->key, len)==0) {
            return temp->next->val;
        } 
        temp = temp->next;
    }  

   return val;
}

int getzmKVRecordLen(zmKVRecord* record) {
    int ret = 0;
    zmKV* temp = NULL;
    if (record==NULL) {
        ZM_LOG("record is null\n");
        return ret;
    }
    
    temp = &(record->head);
    while (temp->next) {
        ret += temp->next->kLen + temp->next->vLen;
        temp = temp->next;
    }

    return ret;
}

int zmKVRecordAddzmKVNode(zmKVRecord* record, zmKV* node) {
    int ret = 0;
    zmKV* temp = NULL;
    if (node==NULL || record==NULL) {
        ZM_LOG("node or record is null\n");
        return ret;
    }
     
    temp = &(record->head);
    while (temp->next) { temp = temp->next;}
    temp->next = node;
    ret =  1;
    return ret;
}

int destoryzmKVNode(zmKV* node) {
    int ret = 0;
    if (node==NULL) {
        ZM_LOG("zmKV node is null\n");
        return ret;
    } 
    if (node->key) free(node->key);
    if (node->val) free(node->val);
    free(node);
    ret = 1;
    return ret;
}

zmKV* createzmKVNode(const char* key, const char* val) {
    zmKV* zmKVnode = NULL;
    if (key==NULL || val==NULL) {
        ZM_LOG("key or val is null\n");
        return zmKVnode;
    }

    zmKVnode = malloc(sizeof(zmKV));
    if (zmKVnode==NULL) {
        ZM_LOG("zmKVnode malloc failed\n");
        return zmKVnode;
    }
    
    // init 
    memset(zmKVnode, 0, sizeof(zmKVnode));
    zmKVnode->kLen = strlen(key);
    zmKVnode->vLen = strlen(val);
    zmKVnode->next = NULL;

    zmKVnode->key = malloc(zmKVnode->kLen+1);
    if (zmKVnode->key==NULL) {
       ZM_LOG("zmKVnode key malloc faidl\n");
       free(zmKVnode);
       return zmKVnode;
    }

    zmKVnode->val = malloc(zmKVnode->vLen+1);
    if (zmKVnode->val==NULL) {
       ZM_LOG("zmKVnode val malloc faidl\n");
       free(zmKVnode->key);
       free(zmKVnode);
       return zmKVnode;
    }

    // copy
    memset(zmKVnode->key, 0, zmKVnode->kLen+1);
    memset(zmKVnode->val, 0, zmKVnode->vLen+1);
    strncpy(zmKVnode->key, key, zmKVnode->kLen);
    strncpy(zmKVnode->val, val, zmKVnode->vLen);

    return zmKVnode;
}


/*
 * @primary: the array index of zmKVdb.
 */
zmKVRecord* getzmKVRecord(zmKVContainer* containor, int primary) {
    zmKVRecord** table = NULL;
    zmKVRecord* record = NULL;
    if (containor==NULL || primary<0) {
        ZM_LOG("containor is null or primary <0\n");
        return record;
    }
    
    if (primary>=containor->capacity) {
        ZM_LOG(" primary %d >= containor capacity %d\n", primary, containor->capacity);
        return record;
    }
    table = (zmKVRecord**)(containor->table);
    if (table==NULL) {
        ZM_LOG("table is null\n");
        return record;
    }
    
    record = table[primary];
    return record;
}

/*
 * The record will be organize as a zmKV-linked-list.
 * @record: output param.
 * @data: {"K1":"V1","K2":"V2"}
 */
int _analizeFromFile(zmKVRecord* record, const char* data) {
    int ret = 0, primary = 0;
    char time[ZM_KV_DB_RECORD_TIME_LEN];
    char info[ZM_KV_DB_RECORD_DATA_LEN];
    
    memset(time, 0, ZM_KV_DB_RECORD_TIME_LEN);
    memset(info, 0, ZM_KV_DB_RECORD_DATA_LEN);
    
    if (data==NULL || record==NULL) {
        ZM_LOG("Data or record is null.\n");   
        return ret;
    }
    
    if (sscanf(data, ZM_KV_DB_RECORD_FILE_FORMAT, time, &primary, info)==3) {
        ZM_LOG("record: time[%s], primary[%d], data[%s]\n", time, primary, info);
        record->primary = primary;
    }
    else {
        ZM_LOG("data is invaild. format: %s \ndata: %s\n", ZM_KV_DB_RECORD_FILE_FORMAT, data);
        return ret;
    }

    {
        char* tmp = NULL;
        //{"key":"val","key":"val"}  
        char* separator = "\",\"";
        char* format = "\"%[^\"]\":\"%[^\"]\"";

        // "key":"val"
        char* singeleKVFormat = "\"%[^\"]\":\"%[^\"]\"";
        char key[ZM_KV_DB_RECORD_DATA_KEY];
        char val[ZM_KV_DB_RECORD_DATA_VAL];
        zmKV* kvNode = NULL;

        memset(key, 0, ZM_KV_DB_RECORD_DATA_KEY);
        memset(val, 0, ZM_KV_DB_RECORD_DATA_VAL);

        if (info==NULL) {
            ZM_LOG("info is null\n");
            return ret;
        }
        
        { // dealwith "key":"val","key":"val"  
            char *end=NULL;
            char *start = info; 
            while ( (end=strstr(start, separator))!=NULL ) {
                *(end+1) = '\0';
                memset(key, 0, ZM_KV_DB_RECORD_DATA_KEY);
                memset(val, 0, ZM_KV_DB_RECORD_DATA_VAL);
                if (sscanf(start, format, key, val)==2) {
                    kvNode = createzmKVNode(key, val);
                    if (kvNode == NULL) {
                        ZM_LOG("info [%s] create zmKV node failed\n", info);
                    }
                    else {
                        ret = zmKVRecordAddzmKVNode(record, kvNode);
                        if (ret==0) {
                            ZM_LOG("zmKVRecord add zmKVnode failed\n");
                            break;
                        }
                    }
                }
                else {
                   ZM_LOG("zmKVRecord format in invaild\n %s\n", start);
                }
                start = end+2;
            }   

            // The last match [ "xx":"yy"} ]
            memset(key, 0, ZM_KV_DB_RECORD_DATA_KEY);
            memset(val, 0, ZM_KV_DB_RECORD_DATA_VAL);
            if (sscanf(start, format, key, val)==2) {
                kvNode = createzmKVNode(key, val);
                if (kvNode == NULL) {
                    ZM_LOG("info [%s] create zmKV node failed\n", info);
                }
                else {
                    ret = zmKVRecordAddzmKVNode(record, kvNode);
                    if (ret==0) {
                        ZM_LOG("zmKVRecord add zmKVnode failed\n");
                    }
                }
            }
            else {
                ZM_LOG("zmKVRecord format in invaild\n %s\n", start);
            }
        }
    }
    return ret;
}

/*
 * The record will be organize as a zmKV-linked-list.
 * @record: output param.
 * @data: id_name_gender_age|id_name_gender_age|id_name_gender_age
 */
int _analizeFromHttp(zmKVRecord* record, const char* data) {
    int ret = 0;
    char *start = NULL, *end=NULL, *info=NULL;
    char id[ZM_KV_DB_RECORD_DATA_VAL];
    char age[ZM_KV_DB_RECORD_DATA_VAL];
    char name[ZM_KV_DB_RECORD_DATA_VAL];
    char gender[ZM_KV_DB_RECORD_DATA_VAL];
    zmKV*  zmKVNode = NULL;

    if (record==NULL || data==NULL) {
        ZM_LOG("record or data is null\n");
        return ret;
    }   

    // Copy data.
    info = malloc(strlen(data)+1);
    if (info==NULL) {
        ZM_LOG("info malloc failed\n");
        return ret;
    }
    memset(info, 0, strlen(data)+1);
    strncpy(info, data, strlen(data));

    start = info;
    memset(id, 0, ZM_KV_DB_RECORD_DATA_VAL);
    memset(age, 0, ZM_KV_DB_RECORD_DATA_VAL);
    memset(name, 0, ZM_KV_DB_RECORD_DATA_VAL);
    memset(gender, 0, ZM_KV_DB_RECORD_DATA_VAL);
    if (sscanf(start, ZM_KV_DB_RECORD_HTTP_FORMAT, id, name, gender, age)==4) {
        zmKVNode = createzmKVNode(ZM_KV_DB_KEY_ID, id);
        if (zmKVNode) zmKVRecordAddzmKVNode(record, zmKVNode);
        zmKVNode = createzmKVNode(ZM_KV_DB_KEY_NAME, name);
        if (zmKVNode) zmKVRecordAddzmKVNode(record, zmKVNode);
        zmKVNode = createzmKVNode(ZM_KV_DB_KEY_GENDER, gender);
        if (zmKVNode) zmKVRecordAddzmKVNode(record, zmKVNode);
        zmKVNode = createzmKVNode(ZM_KV_DB_KEY_AGE, age);
        if (zmKVNode) zmKVRecordAddzmKVNode(record, zmKVNode);
        record->primary = atoi(id);
    }
    free(info);
    ret = 1;
    return ret;
}

/*
 * The record will be organize as a zmKV-linked-list.
 * @record: output param.
 * @data: 
 */
int _analizeFromUdp(zmKVRecord* record, const char* data) {
    int ret = 0;
    // todo
    return ret;
}

/*
 * You shuldn't change the analizer order, unless you know what you do.
 * Translate string data to zmKVRecord structure.
 */
static const zmKVDBDataAnalizer analizerList[] = {
    _analizeFromFile,
    _analizeFromHttp,
    _analizeFromUdp
};

/*
 * Date format : "{\"Time\":\"%s\", \"id\":\"%d\",\"Data\":%s}"
 */
int _displayToFile(zmKVRecord* record, char* buffer, int len) {
    int ret = 0;
    int primary = 0, recordLen=0;
    char *info = NULL;
    char* time = "00-00-00";
    if (record==NULL || buffer==NULL || len<=0) {
        ZM_LOG("record or buffer is null. len %d\n", len);
        return ret;
    }
    // Add more 64 and double ZM_KV_DB_RECORD_LEN to maka sure space enough.
    recordLen = getzmKVRecordLen(record) + 2*sizeof(ZM_KV_DB_RECORD_FILE_FORMAT) + 64;
    if (recordLen>len) {
        ZM_LOG("buffer len %d may be no enough. need:%d\n", len, recordLen);
        return ret;
    }

    info = malloc(recordLen);
    if (info==NULL) {
        ZM_LOG("record malloc failed. size: %d\n", recordLen);
        return ret;
    }
    memset(info, 0, recordLen);

    primary = record->primary;
    // init record info.
    // record info format is {"%s":"%s", "%s":"%s"}
    {
        zmKV* temp = &(record->head); 
        *info = '{';
        while (temp->next) {
            sprintf(info+strlen(info), "\"%s\":\"%s\",", temp->next->key, temp->next->val);
            temp = temp->next;
        }
        // skip last ','
        if (*(info+strlen(info)-1) == ',') *(info+strlen(info)-1) = '\0';
        *(info+strlen(info)) = '}';
    }

    memset(buffer, 0, len);
    sprintf(buffer, ZM_KV_DB_RECORD_FILE_FORMAT_DISPLAY, time, primary, info);
    free(info);
    ZM_LOG("record to file: %s\n", buffer);
    ret = 1;
    return ret;
}

/*
 * Data format: id_name_gender_age|id_name_gender_age|id_name_gender_age
 */
int _displayToHttp(zmKVRecord* record, char* buffer, int len) {
    int ret = 0, recordLen=0;
    char* info = NULL;
    if (record==NULL || buffer==NULL || len<=0) {
        ZM_LOG("record or buffer is null. len %d\n", len);
        return ret;
    }
    
    // Add more 64 to maka sure space enough.
    recordLen = getzmKVRecordLen(record) + 64;
    if (recordLen>len) {
        ZM_LOG("buffer len %d may be no enough. need:%d\n", len, recordLen);
        return ret;
    }

    info = malloc(recordLen);
    if (info==NULL) {
        ZM_LOG("record malloc failed. size: %d\n", recordLen);
        return ret;
    }

    {
        const char* id = getzmKVRecordVal(record, ZM_KV_DB_KEY_ID); 
        const char* name = getzmKVRecordVal(record, ZM_KV_DB_KEY_NAME); 
        const char* age = getzmKVRecordVal(record, ZM_KV_DB_KEY_AGE); 
        const char* gender = getzmKVRecordVal(record, ZM_KV_DB_KEY_GENDER); 
        sprintf(info, ZM_KV_DB_RECORD_HTTP_FORMAT_DISPLAY, id, name, gender, age);
    }
    // to gbk
    /*if (utf8_to_gbk(info, buffer, len)==0) {
        ZM_LOG("Data to gbk sucess");
    }
    else {
        ZM_LOG("Data to gbk failed");
        strncpy(buffer, info, strlen(info));
    }*/
    strncpy(buffer, info, strlen(info));
    free(info);
    return ret;
}

/*
 * Data format: 
 *  Action:WebNotify\r\n                                              
 *  InterCmd: Data\r\n                             
 *  id:id\r\n
 *  name:姓名\r\n
 *  gender:性别\r\n
 *  age:年龄\r\n
 *  MsgID: <数据包ID>\r\n
 */
int _displayToUdp(zmKVRecord* record, char* buffer, int len) {
    int ret = 0, recordLen=0;
    char* info = NULL;
    if (record==NULL || buffer==NULL || len<=0) {
        ZM_LOG("record or buffer is null. len %d\n", len);
        return ret;
    }
    
    // Add more 64 and double ZM_KV_DB_RECORD_LEN to maka sure space enough.
    recordLen = getzmKVRecordLen(record) + 64;
    if (recordLen>len) {
        ZM_LOG("buffer len %d may be no enough. need:%d\n",len, recordLen);
        return ret;
    }

    info = malloc(recordLen);
    if (info==NULL) {
        ZM_LOG("record malloc failed. size: %d\n", recordLen);
        return ret;
    }

    {
        const char* id = getzmKVRecordVal(record, ZM_KV_DB_KEY_ID); 
        const char* name = getzmKVRecordVal(record, ZM_KV_DB_KEY_NAME); 
        const char* age = getzmKVRecordVal(record, ZM_KV_DB_KEY_AGE); 
        const char* gender = getzmKVRecordVal(record, ZM_KV_DB_KEY_GENDER); 
        int nullLen = strlen(ZM_KV_DB_NULL);
        if (strlen(name)==nullLen && strncmp(name, ZM_KV_DB_NULL, nullLen)==0 ) {
            name = " ";
        }
        if (strlen(age)==nullLen && strncmp(age, ZM_KV_DB_NULL, nullLen)==0 ) {
            age = " ";
        }
        if (strlen(gender)==nullLen && strncmp(gender, ZM_KV_DB_NULL, nullLen)==0 ) {
            gender = " ";
        }

        sprintf(info, ZM_KV_DB_RECORD_UDP_FORMAT, id, name, gender, age, "1");

        // to gbk
        /*if (utf8_to_gbk(info, buffer, len)==0) {
            ZM_LOG("Data to gbk sucess");
        }
        else {
            ZM_LOG("Data to gbk failed");
            strncpy(buffer, info, strlen(info));
        }*/
        strncpy(buffer, info, strlen(info));
        free(info);
        ZM_LOG("%s\n", buffer);
        ret = 1;
    }

    return ret;
}

/*
 * You shuldn't change the analizer order, unless you know what you do.
 * Translate zmKVRecord structure to string.
 */
static const zmKVDBDataDisplayer displayerList[] = {
    _displayToFile,
    _displayToHttp,
    _displayToUdp,
};

/*
 * Create record from data sources.
 * @data: record data.
 * @source: where the data from. such as KV_FILE_DATA, KV_HTTP_DATA, KV_UDP_DATA.
 */
zmKVRecord* createKVRecord(const char* data, zmKVDBDataSource source) {
    zmKVRecord* record = NULL;

    if (data==NULL) {
        ZM_LOG("Data is null. Do nothing.\n");
        return NULL;
    }

    ZM_LOG("record: %s", data);
    record = (zmKVRecord*)malloc(sizeof(zmKVRecord));
    if (record == NULL) {
        ZM_LOG("zmKVRecord malloc failed.\n");
        return record;
    }
    memset(record, 0, sizeof(zmKVRecord));    
 
    // check analizer.   
    if (analizerList[source]==NULL) {
        ZM_LOG("You analizer is null. Data source is %d\n", source);
        free(record);
        return NULL;
    }
    
    // start analize.
    if(analizerList[source](record, data)==0) {
        ZM_LOG("analizer work faild\n");
        free(record);
        return NULL;
    }
    ZM_LOG("createKVRecord success\n");
    return record;
}

int zmKVRecordDestory(zmKVRecord* record) {
    int ret = 0;
    zmKV* item = NULL;
    if (record==NULL) {
        ZM_LOG("record is null.\n");
        return ret;
    }

    item = &(record->head);
    while (item->next) {
       zmKV* tmp = item->next;
       item = tmp->next;
       if (tmp->key) free(tmp->key); 
       if (tmp->val) free(tmp->val);
       free(tmp);
    }
    free(record);
    ret = 1;
    return ret;
}

/*
 *  Check if have old record; Yes, destory it and then add new one.
 *  There is a hook (_containAdpter) to increase/decrease capacity. [todo]
 */
int zmKVContainerAdd(zmKVContainer* containor, zmKVRecord* record) {
    int ret = 0, primary = 0;
    zmKVRecord** table = NULL;

    // Get primary key.
    if (containor==NULL || record==NULL) {
        ZM_LOG("containor or record is null\n");
        return ret;
    }

    primary = record->primary;
    if (primary<0 || primary>=containor->capacity) {
        ZM_LOG("containor is null or primary %d no in [0, %d)\n", primary, containor->capacity);
        return ret;
    }
    
    _containAdpter(containor, primary);
    /* Must after _containAdpter. Bacause _containAdpter may be change containor*/
    table = (zmKVRecord**)(containor->table);
    zmKVContainerDel(containor, &primary);
    table[primary] = record;
    containor->size++;
    ZM_LOG("record add success: table[%d]\n", primary);
    return ret;
}

int zmKVContainerDel(zmKVContainer* containor, void* primary) {
    int ret = 0;
    int index = *((int*)primary);
    zmKVRecord** table = NULL;
    if (containor==NULL || index<0 || index>=containor->capacity) {
        ZM_LOG("containor is null or index %d no in [0, %d)\n", index, containor->capacity);
        return ret;
    }

    table = (zmKVRecord**)(containor->table);
    if (zmKVRecordDestory(table[index])) {
        // change size.
        table[index] = NULL;
        containor->size--;
    }
    return ret;
}

/* del old record 
 * use new data to creare one and add it.
 * change szie and capacity.
 */
int zmKVContainerModify(zmKVContainer* containor, zmKVRecord* record) {
    int ret = 0;
    int primary = 0;
    if (containor==NULL || record==NULL) {
        primary = record->primary;
        zmKVContainerDel(containor, &primary);
        ret = zmKVContainerAdd(containor, record);
    }
    ZM_LOG("zmKVContainerModify: ret[%d]\n", ret);
    return ret;
}

int zmKVDBRecordDisplay(zmKVRecord* record, zmKVDBDataFormat format, char* buffer, int len) {
    int ret = 0;
    if (record==NULL || buffer==NULL || len<0) {
        ZM_LOG("record or buffer is null\n");
        return ret;
    }
    
    if (displayerList[format]) {
        ret = displayerList[format](record, buffer, len);
    }
    else {
        ZM_LOG("displayer handler is null\n");
    }
    return ret;
}

int initzmKVDBQuery(zmKVDBQuery* query, int all, int start, int end, zmKVDBDataFormat format, char* buffer, int len) {
    int ret = 0;
    if (query==NULL) {
        ZM_LOG("Query is null\n");
        return ret;
    }
    
    if (all<0 || start<0 || end<0) {
        ZM_LOG("Query range is bad\n");
        return ret;
    }

    if (buffer==NULL || len<0) {
        ZM_LOG("Query buffer is bad\n");
        return ret;
    }
    
    memset(buffer, 0, len);    
    query->all = all;
    query->start = start;
    query->end = end;
    query->result = buffer;
    query->capacity = len;
    query->used = 0;
    query->format = format;
    return ret;
}

int destoryzmKVDBQuery(zmKVDBQuery* query) {
    int ret = 0;
    if (query==NULL) {
        ZM_LOG("query object is null\n");
        return ret;
    }
    if(query->result) free(query->result);
    query->result = NULL;
    return ret;
}

int zmKVContainerQuery(zmKVContainer* containor, zmKVDBQuery* query) {
    zmKVRecord* record = NULL;
    int ret = 0, i = 0;
    if (query->all) {
        query->start = 0;
        query->end = containor->capacity;
    }
    ZM_LOG("Start Query start:%d end:%d\n", query->start, query->end);
    for (i=query->start; i<query->end; i++) {
        record = getzmKVRecord(containor, i);
        if (record!=NULL) {
            char* buffer = query->result+query->used;
            int len = query->capacity - query->used - 1;
            ret = zmKVDBRecordDisplay(record, query->format, buffer, len);
            query->used += strlen(buffer);
        }
    }   
    ZM_LOG("Query success.\n");
    return ret;
}

int zmKVContainerSave(zmKVContainer* containor, const char* path) {
    int ret = 0, i=0;
    FILE* fp = NULL;
    zmKVRecord* record;
    const char* filePath = path; 
    char recordBuff[ZM_KV_DB_RECORD_LEN];

    if (containor ==NULL) {
        ZM_LOG("containor is null\n");
        return ret;
    }

    if (path == NULL) {
        filePath = containor->path;
    }
    
    ZM_LOG("zmKVdb save data to %s\n", filePath);
    fp = fopen(filePath, "w");
    if (fp==NULL) {
       ZM_LOG("Open filePath %s failed\n", filePath);
       return ret;
    }   

    for(i=0; i<containor->capacity; i++) {
        record = getzmKVRecord(containor, i);
        if (record) {
            memset(recordBuff, 0, ZM_KV_DB_RECORD_LEN);
            ret = zmKVDBRecordDisplay(record, KV_FILE_FORMAT, recordBuff, ZM_KV_DB_RECORD_LEN);
            //ZM_LOG("record:%s\n", recordBuff);
            if (ret == 1) {
                fwrite(recordBuff, 1, strlen(recordBuff), fp);
            }
        }
    }
        
    fclose(fp);   
    ZM_LOG("Sync data ok.\n");
    return ret;
}

int zmKVContainerDestory(zmKVContainer* containor) {
    int ret = 0;
    if (containor==NULL) {
        ZM_LOG("containor is null. Do things\n");
        return ret;
    }
    
    if (containor->capacity>0) {
        int i = 0;   
        for (i=0; i<containor->capacity; i++) {
            zmKVRecord* record = getzmKVRecord(containor, i);
            if (record) {   
                zmKVRecordDestory(record);
            }
            ZM_LOG("Restory record %d\n", i);
        }
        free(containor);
    }
    return ret;
}

int initzmKVContainer(zmKVContainer* containor, zmKVDBConf* conf) {
    int ret = 0;
    {
        zmKVRecord* table= NULL;

        table = (zmKVRecord*)malloc(sizeof(zmKVRecord*)*ZM_KV_DB_CAPACITY);
        if (table==NULL) { 
           ZM_LOG("containor table malloc failed.\n");
           return ret;
        }
        
        //memset(newContainor, 0, sizeof(zmKVContainer));
        memset(table, 0, sizeof(zmKVRecord*)*ZM_KV_DB_CAPACITY);
    
        containor->size = 0;
        containor->table = (zmKVRecord* (*)[])table;
        containor->capacity = ZM_KV_DB_CAPACITY;
        if (conf!=NULL) {
            strncpy(containor->path, conf->path, ZM_KV_DB_PATH_LEN);
        }
        
        ret = 1;
    }
    return ret;
}
//#endif
