#include "zm_utils.h"
#include "zm_storage.h"

#include "KV/zm_storage_kv.h"

// The count of common handler.
#define ZM_DB_HANDLE_CNT 7

/* 
 * Brief: Init zmDB name, handles and load data. 
 * Note:  You can choose which structure you want to used by zmDBConf.
 * @conf: Some init argument such as: file path. 
 * @return：0(Failed)
 */
int zmDBInit(struct zmDB* db,  zmDBConf* conf) {
    int ret = 0;
    // zmDBhandle (*table)[ZM_DB_HANDLE_CNT] = NULL;
    const zmDBhandle* table = NULL;
    if (db==NULL || conf==NULL) {
        ZM_LOG("zmDBInit: db or conf is null!!!\n");
        return ret;
    }
    
    if (conf->table==NULL) {
        ZM_LOG("zmDBInit: zmDB handle table is null!!!\n");
        return ret;
    }

    // init db name
    db->name = (char*)malloc(ZM_DB_NAME_LEN);
    if (db->name==NULL) {
        ZM_LOG("zmDBInit: zmDB name is null!!!\n");
        return ret;
    }
    memset(db->name, 0, ZM_DB_NAME_LEN);
    strncpy(db->name, conf->name, ZM_DB_NAME_LEN);
    ZM_LOG("zmDBInit: zmDB name is %s.\n", db->name);

    // init db handle.
    table = conf->table;
    db->load = table[0];
    db->add = table[1];
    db->del = table[2];
    db->modify = table[3];
    db->query = table[4];
    db->save = table[5];
    db->destory = table[6];

    // load data
    if (db->load) {
        ZM_LOG( "zmDBInit: Data loading...\n");
        ret = db->load(db, conf);
        ZM_LOG( "zmDBInit: Data load OK.\n");
    }
    ZM_LOG( "zmDBInit: success\n");
    return ret;
}

/*
 * Brief: Malloc new storage. call zmDBInit. 
 * @storage     : The place where DB mout in.
 * @conf        : zm DB config [name, path, handles, format]
 * @return      : 0/1
 */
int newZmStorage(zmStorage* storage, zmDBConf* conf) {
    if (storage != NULL) {
        // new
        zmStorage* storageNode = (zmStorage*)malloc(sizeof(zmStorage));
        if (storageNode==NULL) {
            ZM_LOG( "newzmStorage: Malloc storage fail\n");
            return 0;
        }

        {
            zmDB* db = (zmDB*)malloc(sizeof(zmDB));
            if (db==NULL) {
                free(storageNode);
                ZM_LOG( "newzmStorage: Malloc DB fail\n");
                return 0;
            }
            memset(storageNode, 0, sizeof(zmStorage));
            memset(db, 0, sizeof(zmDB));
            zmDBInit(db, conf);

            // add to httpd storage.
            storageNode->db = db;
            storageNode->next = storage->next;
            storage->next = storageNode;
        }
        ZM_LOG( "new http storage success.\n");
    }    
    return 1;
}

/*
 *  @storage: zmStorage head node [important].
 *  @dbName : The db which you want operate.
 *  @returm : NULL or DB
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
                    ZM_LOG("Find: targetDB [%s] success\n", dbName);
                    return db;
                }
            }
            tmp = tmp->next;
        }
    }
    return NULL;
}

/*
 *  @storage: zmStorage head node [important].
 *  @return: 0(Failed)/1(Ok)
 */
int destoryzmStorage(zmStorage* storage) {
    zmStorage *item = NULL, *tmp = NULL;
    if (storage==NULL) {
        ZM_LOG("storage is null\n");
        return 0;
    }

    item = storage;     
    while (item->next) {
        tmp = item->next;

        if (tmp->db) {
            if (tmp->db->destory==NULL) {
                ZM_LOG("storage db %s destory function is null\n", tmp->db->name);
            }
            else {
                tmp->db->destory(tmp->db, NULL);
            }
            
            if (tmp->db->name) {
                free(tmp->db->name);
                tmp->db->name = NULL;
            }
            tmp->db = NULL;
        }
        item = tmp;
        free(tmp);
    }
    return 1;
}

