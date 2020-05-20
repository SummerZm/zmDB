#ifndef _ZM_STORAGE_H_
#define _ZM_STORAGE_H_

#define ZM_DB_PATH_LEN   1024
#define ZM_DB_NAME_LEN   1024
#define ZM_DB_HANDLE_CNT    7

typedef struct zmDB zmDB;
typedef int (*zmDBhandle)(struct  zmDB* db, void* argus);

typedef enum zmDBDataFormat{
    ZM_JSON_FORMAT= 0,
} zmDBDataFormat;

typedef struct zmDBConf {
    char path[ZM_DB_PATH_LEN];   
    char name[ZM_DB_NAME_LEN];
    zmDBDataFormat format;
    zmDBhandle* table;
} zmDBConf; 

struct zmDB {
    char* name;
    // Data in the memory. 
    void* data;    
    
    // Add, del, save, query, modify.
    // Don't change the order of those function. 
    zmDBhandle load;
    zmDBhandle add;
    zmDBhandle del;
    zmDBhandle modify;
    zmDBhandle query;
    zmDBhandle save;
    zmDBhandle destory;
};

typedef struct _zmStorage {
  zmDB* db;
  struct _zmStorage* next;
} zmStorage;

/* New a Storage
 * @category: Data storage structure which you need.
 * @agrus: Some init agruments.
 */
int newZmStorage(zmStorage* storage, zmDBConf* conf);

/* Actually, we have many storage objects. every object is a db which implement with different structure.
 * And you choose one of them.
 * @ storage: It's may be a head of a linked-list.
 * @ dbName: The name of db which you want to get.
 */
zmDB* getzmStorage(zmStorage* storage, char* dbName);

/*
 * Destory a storage
 * @storage: The storage which you want destory.
 */
int destoryzmStorage(zmStorage* storage);
#endif


