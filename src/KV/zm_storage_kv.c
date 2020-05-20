#include "../zm_utils.h"
#include "../zm_storage.h"
#include "zm_storage_kv.h"

int zmKVDBContainerLoad(struct  zmDB* db, void* dbConf);
int zmKVDBContainerAdd(zmDB* db, void* record);
int zmKVDBContainerDel(zmDB* db, void* primary);
int zmKVDBContainerModify(zmDB* db, void* record);
int zmKVDBContainerQuery(zmDB* db, void* query);
int zmKVDBContainerSave(zmDB* db, void* path);
int zmKVDBContainerDestory(zmDB* db, void* oob);

// ZMKVDB handle talbe
static const zmDBhandle zmKVDBHandleTalbe[ZM_DB_HANDLE_CNT] = {
    zmKVDBContainerLoad,
    zmKVDBContainerAdd,
    zmKVDBContainerDel,
    zmKVDBContainerModify,
    zmKVDBContainerQuery, 
    zmKVDBContainerSave,
    zmKVDBContainerDestory
};

/*************************************************************************** 
 *  ZM-KV-DB data process api 
 ***************************************************************************/
const char* zmKVDBRecordSeparator[] = {
    "\r\n", // KV_FILE_FORMAT
};
int _analizeFromFile(zmKVRecord* record, const char* data);
/*
 * You shuldn't change the analizer order, unless you know what you do.
 * Translate string data to zmKVRecord structure.
 */
static const zmKVDBDataAnalizer analizerList[] = {
    _analizeFromFile,
};

int _displayToFile(zmKVRecord* record, char* buffer, int len);
/*
 * You shuldn't change the analizer order, unless you know what you do.
 * Translate zmKVRecord structure to string.
 */
static const zmKVDBDataDisplayer displayerList[] = {
    _displayToFile,
};

/*************************************************************************** 
 *  ZM-KV-DB base api [The operate of containor, record, node.]
 ***************************************************************************/
/*
 *  @storage: init zm-kv-db conf
 *  @return: 0(Failed)/1(Ok)
 */
int initZmKVDBConf(zmDBConf* conf, const char* name, zmDBDataFormat format, const char* path) {
    int pathLen=0, nameLen=0;
    if (conf==NULL || name==NULL || path==NULL) {
        ZM_LOG( "conf or name[%s] or path[%s] is null\n", name, path);
        return 0;
    }
    
    pathLen = strlen(path);
    nameLen = strlen(name);

    if (pathLen>ZM_DB_PATH_LEN || nameLen>ZM_DB_NAME_LEN) {
        ZM_LOG( "Path len %d [%d] or name len %s [%d] is too long\n", pathLen, ZM_DB_PATH_LEN, name, ZM_DB_NAME_LEN);
        return 0;
    }  
    strncpy(conf->path, path, ZM_KV_DB_PATH_LEN);   
    strncpy(conf->name, name, ZM_KV_DB_NAME_LEN);   
    conf->format = format;
    conf->table = (zmDBhandle*)zmKVDBHandleTalbe;
    ZM_LOG( "zmDBConf init success!!!\n");
    return 1;
}

/*
 * @Brief: To make sure containor size is enough.
 * @primary: New record primary. if primary bigger than containor capacity, containor grow.
 * @return: 0/1
 */
int _containAdpter(zmKVContainer* containor, int primary) {
    int newCapacity = primary + ZM_KV_DB_CAPACITY_INCREASE_STEP;
    zmKVRecord* newTable= NULL;
    if (primary < containor->capacity) {
        return 0;
    }
    
    newTable = (zmKVRecord*)malloc(sizeof(zmKVRecord*)*newCapacity);
    if (newTable == NULL) {
        return 0;
    }
    memset(newTable, 0, sizeof(zmKVRecord*)*newCapacity);
    memcpy(newTable, containor->table, sizeof(zmKVRecord*)*containor->capacity);
    ZM_LOG("zmDB Capacity change from %d to %d  %lu\n", containor->capacity, newCapacity, sizeof(zmKVRecord*)*newCapacity);
    containor->capacity = newCapacity;
    free(containor->table);
    containor->table = (zmKVRecord* (*)[])newTable;
    return 1;
}

/*
 * @Brief: Create KV node
 * @Return: NULL
 */
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
    memset(zmKVnode, 0, sizeof(zmKV));
    zmKVnode->kLen = strlen(key);
    zmKVnode->vLen = strlen(val);
    zmKVnode->next = NULL;

    zmKVnode->key = malloc(zmKVnode->kLen+1);
    if (zmKVnode->key==NULL) {
       ZM_LOG("zmKVnode key malloc faidl\n");
       free(zmKVnode);
       zmKVnode=NULL;
       return zmKVnode;
    }

    zmKVnode->val = malloc(zmKVnode->vLen+1);
    if (zmKVnode->val==NULL) {
       ZM_LOG("zmKVnode val malloc faidl\n");
       free(zmKVnode->key);
       free(zmKVnode);
       zmKVnode=NULL;
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
 * @Brief: Destory KV node
 * @Return: 0/1
 */
int destoryzmKVNode(zmKV* node) {
    if (node==NULL) {
        ZM_LOG("zmKV node is null\n");
        return 0;
    } 
    if (node->key) free(node->key);
    if (node->val) free(node->val);
    free(node);
    return 1;
}

/*
 * @Brief: Record linked-list add KV node
 * @Return: 0/1
 */
int zmKVRecordAddzmKVNode(zmKVRecord* record, zmKV* node) {
    zmKV* temp = NULL;
    if (node==NULL || record==NULL) {
        ZM_LOG("node or record is null\n");
        return 0;
    }
     
    temp = &(record->head);
    while (temp->next) { temp = temp->next;}
    temp->next = node;
    return 1;
}

/*
 *  @Berif: Accord specify format to create record.
 *  @data:
 *  @format: The format of data.
 *  @idPos: The primary(identify) key position.
 *  @Return: NULL (Failed)
 */
zmKVRecord* createZMKVDBRecord(const char* data, zmDBDataFormat format, int idPos) {
    zmKVRecord* record = NULL;
    if (data==NULL) {
        ZM_LOG( "createZMKVDBRecord: Data is null. Do nothing.\n");
        return NULL;
    }

    record = (zmKVRecord*)malloc(sizeof(zmKVRecord));
    if (record == NULL) {
        ZM_LOG( "createZMKVDBRecord: zmKVRecord malloc failed.\n");
        return record;
    }
    memset(record, 0, sizeof(zmKVRecord));    
    record->idPos = idPos;
    record->primary = -1;
 
    // check analizer.   
    if (analizerList[format]==NULL) {
        ZM_LOG( "createZMKVDBRecord: You analizer is null. Data format is %d\n", format);
        free(record);
        return NULL;
    }
    
    // start analize.
    if(analizerList[format](record, data)==0) {
        ZM_LOG( "createZMKVDBRecord: analizer work faild\n");
        free(record);
        return NULL;
    }

    ZM_LOG( "createZMKVDBRecord: success %s\n", data);
    return record;
}

/*
 * @Brief: _destoryZMKVDBRecord
 * @Return: 0/1
 */
int _destoryZMKVDBRecord(zmKVRecord* record) {
    zmKV *item = NULL, *node=NULL;
    if (record == NULL) {
        ZM_LOG("record is null.\n");
        return 0;
    }

    item = &(record->head);
    while (item->next) {
       node = item->next; 
       item->next = item->next->next;
       destoryzmKVNode(node);
    }
    free(record);
    return 1;
}

/*
 * @Brief: getzmKVRecord
 * @primary: the array index of quick-index-table.
 * @Return: NULL
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
 * @Brief: getzmKVRecordVal
 * @Return: NULL/val
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

/*
 * @Brief: getzmKVRecordVal
 * @Return: 0/len
 */
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

/*
 * @Brief: Get primary key string by idPos (identify key position)
 * @Note: You shouldn't change return value.
 * @return: NULL (when failed)
 */
const char* getZmKVRecordPrimaryKey(zmKVRecord* record, int idPos) {
    const char* primaryKey= NULL;
    const zmKV* temp = NULL; 

    // Check
    if (record==NULL) {
        ZM_LOG( "Record is null.\n");
        return NULL;
    }

    if (idPos < 0) {
        ZM_LOG( "Record idPos %d is invalid.\n", idPos);
        return NULL;
    }

    // Move to the target node.
    temp = &(record->head);
    while (idPos && temp->next) {
        temp = temp->next;
        idPos--;
    } 

    if (idPos==0 && temp->next) {
        primaryKey = temp->next->key;
    }
    else {
        ZM_LOG( "Record KV linked-list is too short idPos %d.\n", idPos);
    }
    return primaryKey;
}

/*
 * @Brief: Get primary val string by idPos (identify val position)
 * @Note: You shouldn't change return value.
 * @return: NULL (when failed)
 */
const char* getZmKVRecordPrimaryVal(zmKVRecord* record, int idPos) {
    const char* primaryVal= NULL;
    const zmKV* temp = NULL; 

    // Check
    if (record==NULL) {
        ZM_LOG( "Record is null.\n");
        return NULL;
    }

    if (idPos < 0) {
        ZM_LOG( "Record idPos %d is invalid.\n", idPos);
        return NULL;
    }

    // Move to the target node.
    temp = &(record->head);
    while (idPos && temp->next) {
        temp = temp->next;
        idPos--;
    } 

    if (idPos==0 && temp->next) {
        primaryVal= temp->next->val;
    }
    else {
        ZM_LOG( "Record KV linked-list is too short idPos %d.\n", idPos);
    }
    return primaryVal;
}

/*
 * @Brief: Get the record quick-index-table position.
 * @Note:  Primary is the quick-index-table[array] cursor. like that: table[primary]
 * @Return: -1 or primary.
 */
int getZmKVRecordPrimary(zmKVContainer* containor, zmKVRecord* record) {
    int primary = -1, idPos = 0, i = 0; 
    const char* primaryStr = NULL;
    zmKVRecord** table = NULL;
    if (containor==NULL) {
        ZM_LOG( "ZM-KV-DB containor is null!!!\n");
        return primary;
    }

    if (record == NULL) {
        ZM_LOG( "ZM-KV-DB record is null!!!\n");
        return primary;
    }
   
    if (record->primary != -1) return record->primary;

    table = (zmKVRecord**)containor->table;
    if (table==NULL) {
        ZM_LOG( "containor->table is null\n");
        return primary;
    }

    // Get target record primary val.
    idPos = record->idPos;
    primaryStr = getZmKVRecordPrimaryKey(record, idPos);
    if (primaryStr == NULL) {
        ZM_LOG( "Record can't find primary key!!!\n");
        return primary;
    }

    // Traverse and to match target record primary val.
    for (i=0; i<containor->capacity; i++) {
        const char* tmpPrimaryVal = NULL;
        zmKVRecord* tmpRecord = table[i];
        if (tmpRecord==NULL) { 
           // Store min-empty-primary.
           if (primary == -1) primary = i;
           continue;
        }

        // Find the primary key and compare.
        tmpPrimaryVal = getZmKVRecordPrimaryKey(tmpRecord, idPos);
        if (tmpPrimaryVal==NULL) {
           ZM_LOG( "Record %d [primaryPos:%d] can't find primary val. it may something error.\n", i, idPos);
           continue;
        }

        //ZM_LOG( "tmpPrimaryVal:%s primaryStr:%s", tmpPrimaryVal, primaryStr);
        if (strlen(tmpPrimaryVal)==strlen(primaryStr) && strcmp(tmpPrimaryVal, primaryStr)==0) { 
            primary = i;
            return primary;
        } 
    }
    return primary;
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


/*************************************************************************** 
 *  ZM-KV-DB operate api [load, add, del, modify, query, save, destory]
 ***************************************************************************/
/*
 * @Brief: Init ZM-KV-DB with conf [quick-index-table, storage path, size, capacity].
 * @Note: Create a record-point-array table for quick index.
 * @Return: 0/1
 */
int _initZmKVContainer(zmKVContainer* containor, zmDBConf* conf) {
    zmKVRecord* table= NULL;
    table = (zmKVRecord*)malloc(sizeof(zmKVRecord*)*ZM_KV_DB_CAPACITY);
    if (table==NULL) { 
       ZM_LOG("KV-containor table malloc failed.\n");
       return 0;
    }
    
    //memset(newContainor, 0, sizeof(zmKVContainer));
    memset(table, 0, sizeof(zmKVRecord*)*ZM_KV_DB_CAPACITY);

    containor->size = 0;
    containor->table = (zmKVRecord* (*)[])table;
    containor->capacity = ZM_KV_DB_CAPACITY;
    strncpy(containor->path, conf->path, ZM_KV_DB_PATH_LEN);
    ZM_LOG("KV-containor init success.\n");
    return 1;
}

/*
 * @Brief: Load records from file.
 * @Return: 0/1
 */
int _loadZmKVRecords(zmDB* db, const char* filePath, zmDBDataFormat format) {
    char line[ZM_KV_DB_RECORD_LEN];
    FILE* fp = fopen(filePath, "r");
    if (fp==NULL) {
        ZM_LOG( "ZM-KV-DB file open %s failed\n", filePath);
        return 0;
    }
    // read data from file, per line. 
    while(1) {
        char* readBytes = NULL;
        zmKVRecord* record = NULL;
        memset(line, 0, ZM_KV_DB_RECORD_LEN);
        readBytes = fgets(line, ZM_KV_DB_RECORD_LEN, fp);
        if (readBytes==NULL) {
            if (feof(fp)) {
                ZM_LOG( "Read  ZM-KV-DB file end.%s\n", line);
                break;
            }
            else {
                perror("Load data form ZM-KV-DB file failed");
            }
        }

        record = createZMKVDBRecord(line, format, 0);
        if (record!=NULL && db->add) {
            db->add(db, record);
        }
        else {
            ZM_LOG( "createZMKVDBRecord fialed or ZM-KV-DB add() is null. %s\n", line);
        }
    }
    fclose(fp);
    return 1;
}

/*
 * @Brief: Init db containor and load data.
 * @dbConf: The db conf.
 * @Return: 0/1
 */
int zmKVDBContainerLoad(struct  zmDB* db, void* dbConf) {
    int ret = 0;
    zmDBConf* conf = (zmDBConf*)dbConf;

    if (db==NULL || dbConf==NULL) {
        ZM_LOG( "ZM-KV-DB or Conf is null. Do nothing\n");
        return 0;
    }

    // init db containor.
    db->data = malloc(sizeof(zmKVContainer));
    if (db->data==NULL) {
        ZM_LOG( "ZM-KV-DB containor malloc failed. Do nothing\n");
        return 0;
    }

    ret = _initZmKVContainer((zmKVContainer*)db->data, conf);
    if (ret == 1) {
        _loadZmKVRecords(db, conf->path, conf->format);
    }
    return 1;
}

/*
 * @Brief: zmKVContainerDestory
 * @Return: 0/1
 */
int _zmKVContainerDestory(zmKVContainer* containor, void* oob) {
    int i = 0;   
    if (containor == NULL) {
        ZM_LOG("containor is null. Do things\n");
        return 0;
    }

    if (containor->capacity <= 0) {
        return 1;
    }

    for (i=0; i<containor->capacity; i++) {
        zmKVRecord* record = getzmKVRecord(containor, i);
        if (record) {   
            _destoryZMKVDBRecord(record);
            ZM_LOG("destory record %d\n", i);
        }
    }
    free(containor);
    return 1;
}

/*
 * @Brief: zmKVDBContainerDestory 
 * @Return: 0/1
 */
int zmKVDBContainerDestory(zmDB* db, void* oob) {
    if (db!=NULL) {
        return _zmKVContainerDestory((zmKVContainer*)db->data, oob);
    }
    return 0;
}

/*
 * @Brief: zmKVContainerDel
 * @primary: The quick-index-table cursor.
 * @Return: 0/1
 */
int _zmKVContainerDel(zmKVContainer* containor, void* primary) {
    int index = *((int*)primary);
    zmKVRecord** table = NULL;
    if (containor==NULL || index<0 || index>=containor->capacity) {
        ZM_LOG("containor is null or index %d no in [0, %d)\n", index, containor->capacity);
        return 0;
    }

    table = (zmKVRecord**)(containor->table);
    if (_destoryZMKVDBRecord(table[index])) {
        table[index] = NULL;
        containor->size--;
    }
    return 1;
}

int zmKVDBContainerDel(zmDB* db, void* primary) {
    if (db!=NULL) {
        return _zmKVContainerDel((zmKVContainer*)db->data, primary);
    }
    return 0;
}

/*
 * @Brief: zmKVContainerAdd  
 * @Note: Check if have old record; Yes, destory it and then add new one.
 * @Return: The valid primary of new record.
 */
int _zmKVContainerAdd(zmKVContainer* containor, zmKVRecord* record) {
    int primary = 0;
    zmKVRecord** table = NULL;

    primary = getZmKVRecordPrimary(containor, record);
    record->primary = primary;
    _containAdpter(containor, primary);

    table = (zmKVRecord**)(containor->table);
    _zmKVContainerDel(containor, &primary);
    table[primary] = record;
    containor->size++;
    ZM_LOG("record add success: table[%d]\n", primary);
    return primary;
}

int zmKVDBContainerAdd(zmDB* db, void* record) {
    zmKVContainer* containor = NULL;
    if (db==NULL) {
        return 0;
    }
    containor = (zmKVContainer*)db->data;
    if (containor==NULL || record==NULL) {
        ZM_LOG("containor or record is null\n");
        return 0;
    }
    return _zmKVContainerAdd(containor, (zmKVRecord*)record);
}

/* 
 * @Brief: Delete the old one and add new
 * @Return: -1/primary 
 */
int _zmKVContainerModify(zmKVContainer* containor, zmKVRecord* record) {
    int primary = -1;
    if (containor==NULL || record==NULL) {
        primary = record->primary;
        _zmKVContainerDel(containor, &primary);
        primary = _zmKVContainerAdd(containor, record);
    }
    ZM_LOG("zmKVContainerModify: ret[%d]\n", primary);
    return primary;
}

/* 
 * @Brief: Delete the old one and add new
 * @Return: -1/primary 
 */
int zmKVDBContainerModify(zmDB* db, void* record) {
    if (db!=NULL) {
        return _zmKVContainerModify((zmKVContainer*)db->data, (zmKVRecord*)record);
    }
    return -1;
}

/*
 * @Brief: Init ZM-KV-DB query object.
 * @query: Output parms. 
 * @start-end: If both of them is 0; then query all data in the db.
 * @format: The format of result display. such as udp, http, action.
 * @len: The size of memory that you plan to offer for this query.
 * @Return: 0/1
 */
int initzmKVDBQuery(zmKVDBQuery* query, int start, int end, zmKVDBDataFormat format, int len) {
    char* buffer = NULL;
    if (query==NULL) {
        ZM_LOG( "Query is null\n");
        return 0;
    }
    
    if (start<0 || end<0) {
        ZM_LOG( "Query range is bad\n");
        return 0;
    }

    if (len<=0) {
        ZM_LOG( "Query buffer len is bad. len:%d\n", len);
        return 0;
    }

    buffer = malloc(len);
    if (buffer==NULL) {
        ZM_LOG( "Query buffer is null\n");
        return 0;
    }
    memset(buffer, 0, len);    

    query->start = start;
    query->end = end;
    query->result = buffer;
    query->capacity = len;
    query->used = 0;
    query->format = format;
    ZM_LOG( "initzmKVDBQuery: init query success!!!\n");
    return 1;
}

/*
 * @Brief: Destory ZM-KV-DB query object.
 * @Return: 0/1
 */
int destoryzmKVDBQuery(zmKVDBQuery* query) {
    if (query==NULL) {
        ZM_LOG("query object is null\n");
        return 0;
    }
    if(query->result) free(query->result);
    query->result = NULL;
    return 1;
}

/*
 * @Brief: Query record
 * @query: Out params. query->result storage records-string in specify format.
 * @Return: 0/1
 */
int _zmKVContainerQuery(zmKVContainer* containor, zmKVDBQuery* query) {
    zmKVRecord* record = NULL;
    int ret = 0, i = 0;
    if (query->start==0 && query->end==0) {
        query->end = containor->capacity;
    }
    ZM_LOG( "Start Query start:%d end:%d\n", query->start, query->end);
    query->used = 0;
    for (i=query->start; i<query->end; i++) {
        record = getzmKVRecord(containor, i);
        if (record!=NULL) {
            char* buffer = query->result+query->used;
            int len = query->capacity - query->used - 1;
            ret = zmKVDBRecordDisplay(record, query->format, buffer, len);
            if (ret==0) {
                return ret;
            }
            strcpy(buffer+strlen(buffer), zmKVDBRecordSeparator[query->format]);
            query->used += strlen(buffer);
        }
    }   
    // clear last Separator
    if (query->used>0) {
        *(query->result+query->used-strlen(zmKVDBRecordSeparator[query->format])) = '\0';
    }

    ZM_LOG( "Query success. %s\n", query->result);
    return 1;
}

/*
 * @Brief: Query record
 * @query: Out params. query->result storage records-string in specify format.
 * @Return: 0/1
 */
int zmKVDBContainerQuery(zmDB* db, void* query) {
    if (db!=NULL) {
        return _zmKVContainerQuery((zmKVContainer*)db->data, (zmKVDBQuery*)query);
    }
    return 0;
}

/*
 * @Brief: zm db save
 * @path: The db file path.
 * @Return: 0/1
 */
int _zmKVContainerSave(zmKVContainer* containor, const char* path) {
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

/*
 * @Brief: zm db save
 * @path: The db file path.
 * @Return: 0/1
 */
int zmKVDBContainerSave(zmDB* db, void* path) {
    if (db!=NULL) {
        return _zmKVContainerSave((zmKVContainer*)db->data, (char*)path);
    }
    return 0;
}

//=============================================================================//

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

