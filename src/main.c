#include "zm_utils.h"
#include "zm_storage.h"
#include "KV/zm_storage_kv.h"

void storageTest(zmStorage* storage){
    zmDBConf conf;
    initZmKVDBConf(&conf, "KVDB", ZM_JSON_FORMAT, "./demo.db");
    newZmStorage(storage, &conf);
    char *data = "{\"Time\":\"123\",\"id\":\"11\",\"Data\":{\"id\":\"11\",\"name\":\"ewe\",\"gender\":\"grd\",\"age\":\"34\"}}";
    char *data1 = "{\"Time\":\"123\",\"id\":\"12\",\"Data\":{\"id\":\"13\",\"name\":\"ewe\",\"gender\":\"grd\",\"age\":\"34\"}}";
    char *data2 = "{\"Time\":\"123\",\"id\":\"13\",\"Data\":{\"id\":\"12\",\"name\":\"ewe\",\"gender\":\"grd\",\"age\":\"34\"}}";
    zmDB* db =  getzmStorage(storage, "KVDB");

    zmKVRecord* record = createZMKVDBRecord(data, KV_FILE_FORMAT, 0);
    zmKVRecord* record1 = createZMKVDBRecord(data1, KV_FILE_FORMAT, 0);
    zmKVRecord* record2 = createZMKVDBRecord(data2, KV_FILE_FORMAT, 0);
    db->add(db, record);
    db->add(db, record1);
    db->add(db, record2);
    db->save(db, conf.path);

    zmKVDBQuery query;
    char* queryBuff = malloc(ZM_KV_DB_RECORD_LEN);
    initzmKVDBQuery(&query, 1, 20, KV_FILE_FORMAT, ZM_KV_DB_RECORD_LEN);
    printf("query init sucess %p\n", record);
    db->query(db, &query);
    printf("query sucess %s\n", query.result);
    //db->destory(db, NULL);
    destoryzmStorage(storage);
    return;
}

int main(int argc, char** argv) {
    zmStorage storage;
    storage.db = NULL;
    storage.next = NULL;
    storageTest(&storage);
    return 0;
}

