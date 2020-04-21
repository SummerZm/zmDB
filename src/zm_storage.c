#include "zm_utils.h"
#include "zm_storage.h"

#include "KV/zm_storage_kv.h"

// The count of common handler.
#define ZM_DB_HANDLE_CNT 7

/*
 * Init db conf (db name), and then load data.
 * @dbConf: The db conf.
 */
int kvLoad(struct  zmDB* db, void* dbConf) {
    int ret = 0;
    FILE* fp = NULL;
    zmKVDBConf* conf = (zmKVDBConf*)dbConf;

    if (db==NULL || dbConf==NULL) {
        ZM_LOG("zmDB or Conf is null. Do nothing");
        return ret;
    }

    if (db->add==NULL) {
        ZM_LOG("zmDB add handler is null. Do nothing");
        return ret;
    }

    // init db containor.
    db->data = malloc(sizeof(zmKVContainer));
    if (db->data==NULL) {
        ZM_LOG("zmDB containor malloc failed. Do nothing");
        return ret;
    }
    initzmKVContainer((zmKVContainer*)db->data, conf);
    
    // init db name
    db->name = (char*)malloc(ZM_KV_DB_NAME_LEN);
    if (db->name) {
        memset(db->name, 0, ZM_KV_DB_NAME_LEN);
        strncpy(db->name, conf->name, ZM_KV_DB_NAME_LEN);
        ZM_LOG("DB name is %s", db->name);
    }
 
    // load data    
    fp = fopen(conf->path, "r");
    if (fp==NULL) {
        ZM_LOG("Open DB file %s failed", conf->path);
        return ret;
    }

    // read data from file, per line. 
    {
        char line[ZM_KV_DB_RECORD_LEN];
        while(1) {
            char* readRet= NULL;
            zmKVRecord* record = NULL;
            memset(line, 0, ZM_KV_DB_RECORD_LEN);
            readRet = fgets(line, ZM_KV_DB_RECORD_LEN, fp);
            if (readRet==NULL) {
                if (feof(fp)) {
                    ZM_LOG("Read DB file end.%s", line);
                    ZM_LOG("DB data file path.%s", conf->path);
                    break;
                }
                else {
                    perror("Load data form db-file fialed");
                }
            }

            record = createKVRecord(line, conf->dataSource);
            if (record!=NULL ) {
                db->add(db, record);
            }
            else {
                ZM_LOG("createKVRecord fialed. %s", line);
            }
        }
    }
    fclose(fp);
    ret = 1;
    ZM_LOG("DB load success");
    return ret;
}

/*
 * Call zmKVRecord add function.
 */
int kvAdd(struct  zmDB* db, void* kvRecord) {
    int ret = 0;
    zmKVRecord* record = (zmKVRecord*)kvRecord;
    if (db==NULL || record==NULL) {
        ZM_LOG("db or kvNode is null");
        return ret;
    }
    
    {
        zmKVContainer* containor = (zmKVContainer*)(db->data);
        if (containor==NULL) return ret;
        ret = zmKVContainerAdd(containor, record);    
    }
    return ret;
}

/*
 * Call zmKVRecord del function.
 */
int kvDel(struct  zmDB* db, void* cmd) {
    int ret = 0;
    if (db==NULL || cmd==NULL) {
        ZM_LOG("db or cmd is null");
        return ret;
    }

    {
        zmKVContainer* containor = (zmKVContainer*)db->data;
        if (containor!=NULL) return ret;
        ret = zmKVContainerDel(containor, (zmKVRecord*)cmd);    
    }
    return ret;
}

/*
 * Call zmKVRecord odify function.
 */
int kvModify(struct zmDB* db, void* cmd) {
    int ret = 0;
    if (db==NULL || cmd==NULL) {
        ZM_LOG("db or cmd is null");
        return ret;
    }

    {
        zmKVContainer* containor = (zmKVContainer*)db->data;
        if (containor!=NULL) return ret;
        ret = zmKVContainerModify(containor, cmd);    
    }
    return ret;
}

/*
 * Call zmKVRecord query function.
 */
int kvQuery(struct zmDB* db, void* cmd) {
    int ret = 0;
    if (db==NULL || cmd==NULL) {
        ZM_LOG("db or cmd is null");
        return ret;
    }

    {
        zmKVContainer* containor = (zmKVContainer*)db->data;
        if (containor==NULL) return ret;
        ret = zmKVContainerQuery(containor, (zmKVDBQuery*)cmd);    
    }
    return ret;
}

/*
 * Sync data to file
 */
int kvSave(struct zmDB* db, void* path) {
    int ret = 0;
    if (db==NULL) {
        ZM_LOG("db is null");
        return ret;
    }

    {
        zmKVContainer* containor = (zmKVContainer*)(db->data);
        if (containor==NULL) return ret;
        ret = zmKVContainerSave(containor, path);    
    }
    return ret;
}

/*
 * Destory
 */
int kvDestory(struct zmDB* db, void* path) {
    int ret = 0;
    if (db==NULL) {
        ZM_LOG("db is null");
        return ret;
    }

    {
        zmKVContainer* containor = (zmKVContainer*)db->data;
        if (containor!=NULL) return ret;
        ret = zmKVContainerDestory(containor);
        db->data = NULL;
        // free name;
        // free data;
    }
    return ret;
}

/* DB handle table. Different data-storage-implenment need different handler.
 * And you can specify it here.
 * {load, add, del, modify, query, save, destory} 
 */
const static zmDBhandle zmDBhandleTable[][ZM_DB_HANDLE_CNT] = { 
    {kvLoad, kvAdd, kvDel, kvModify, kvQuery, kvSave, kvDestory},
    {NULL, NULL, NULL, NULL, NULL, NULL, NULL}
};

/* If you want to use defuat handleTable, set handleTable=NULL.
 * Load data and init handle. 
 * @category        : Storage structure type.
 * @handleTable     : Different structure have different handles. Their stroaged in a table. 
 * @handleTableLen  : The len of handleTable.
 * @argus           : Some init argument such as: file path.
 */
int zmDBInit(struct zmDB* db, zmDBType category, const zmDBhandle** handleTable, int handleTableLen, void* argus) {
    int ret = 0;
    int tableSize = 0;
    zmDBhandle (*table)[ZM_DB_HANDLE_CNT] = NULL;

    if (db==NULL) {
        ZM_LOG("zmDBInit: db is null");
        return ret;
    }
    
    // Sure handle table and table size.
    if (handleTable!=NULL) {
        table = (zmDBhandle(*)[ZM_DB_HANDLE_CNT])(handleTable);
        tableSize = handleTableLen;
    }
    else {
        // Defualt handle
        table = (zmDBhandle(*)[ZM_DB_HANDLE_CNT])zmDBhandleTable;
        tableSize = sizeof(zmDBhandleTable)/sizeof(zmDBhandle[ZM_DB_HANDLE_CNT]);
    }

    if (category<0 || category>=tableSize) {
        ZM_LOG("zmDBInit: error [ 0 < %d < %d ]", category, tableSize);
        return ret;
    }

    // init db handle.
    db->load = table[category][0];
    db->add = table[category][1];
    db->del = table[category][2];

    db->modify = table[category][3];
    db->query = table[category][4];
    db->save = table[category][5];
    db->destory = table[category][6];

    db->category = category;

    // load data
    ZM_LOG("zmDBInit: success");
    if (db->load) {
        ret = db->load(db, argus);
    }

    return ret;
}

/*
 * Note: Malloc new storag and then head insert; binding handle function; call db load.
 * @storage     : The place where DB mout in.
 * @category    : The type of DB-Data-Storage-Structure.
 * @argus       : Some init agruments.
 */
int newzmStorage(zmStorage* storage, zmDBType category, void* argus) {
    int ret = 0;
    if (storage != NULL)
    {
        // new
        zmStorage* storageNode = (zmStorage*)malloc(sizeof(zmStorage));
        if (storageNode==NULL) {
            ZM_LOG("newzmStorage: Malloc storage fail");
            return ret;
        }

        {
            const zmDBhandle** table = (const zmDBhandle**)zmDBhandleTable;
            zmDB* db = (zmDB*)malloc(sizeof(zmDB));
            if (db==NULL) {
                free(storageNode);
                ZM_LOG("newzmStorage: Malloc DB fail");
                return ret;
            }
            memset(storageNode, 0, sizeof(zmStorage));
            memset(db, 0, sizeof(zmDB));
            zmDBInit(db, category, table, sizeof(zmDBhandleTable)/sizeof(zmDBhandle[ZM_DB_HANDLE_CNT]), argus);

            // add to storage.
            storageNode->db = db;
            storageNode->next = storage->next;
            storage->next = storageNode;
            ret = 1;
        }
        ZM_LOG("new storage success.");
    }    
    return ret;
}

/*
 * @ dbName : The db which you want operate.
 */
zmDB* getzmStorage(zmStorage* storage, char* dbName) {
    zmStorage* tmp = NULL;
    if (storage!=NULL && dbName!=NULL) {
        tmp = storage->next;
        while (tmp!=NULL) {
            zmDB* db = tmp->db;
            if (db!=NULL && db->name!=NULL) {
                int curDBNameLen = strlen(db->name);
                int targetNameLen = strlen(dbName);
                if (curDBNameLen==targetNameLen && strcmp(db->name, dbName)==0) {
                    ZM_LOG("Find: targetDB [%s] success", dbName);
                    return db;
                }
            }
            tmp = tmp->next;
        }
    }
    return NULL;
}

int destoryzmStorage(zmStorage* storage) {
    int ret = 0;
    zmStorage *item = NULL, *tmp = NULL;
    if (storage==NULL) {
        ZM_LOG("storage is null");
        return ret;
    }

    item = storage;     
    while (item->next) {
        tmp = item->next;

        if (tmp->db) {
            if (tmp->db->destory==NULL) {
                ZM_LOG("storage db %s destory function is null", tmp->db->name);
            }
            else {
                tmp->db->destory(tmp->db, NULL);
                tmp->db = NULL;
            }

            if (tmp->db->name) { 
                free(tmp->db->name);
                tmp->db->name = NULL;
            }
        } 

        item = tmp->next;
        free(tmp);
    }
    return ret;
}

