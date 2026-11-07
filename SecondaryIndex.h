#ifndef SECONDARY_INDEX_H
#define SECONDARY_INDEX_H

#include <string>
#include <vector>
using namespace std;

// One entry of the sorted key file: secondary key + head of its linked list
struct SecEntry {
    string key;   // trimmed (no trailing spaces)
    int head;     // node number of first node in the list file (-1 = empty)
};

// Generic secondary index (linked list on a different file).
//
//  Key file : [int count] then count x { char key[keySize]; int head; }   (sorted ascending)
//  List file: nodes of { char primaryKey[15]; int next; }  (19 bytes each)
//             node n starts at byte n * NODE_SIZE, next = -1 means end of list
//
// List nodes store the PRIMARY KEY (e.g. Doctor ID), never a byte address,
// so the index is bound to the primary index.
class SecondaryIndex {
private:
    static const int PK_SIZE = 15;
    static const int NODE_SIZE = PK_SIZE + sizeof(int);

    string keyFileName;
    string listFileName;
    int keySize;
    vector<SecEntry> entries;   // in-memory copy of the key file

    string normalize(const string& s, int size) const;   // truncate + trim trailing spaces
    int  findKey(const string& key, bool& found) const;  // binary search (returns insert pos if not found)
    int  nodeCount() const;
    void readNode(int n, string& pk, int& next) const;
    void writeNode(int n, const string& pk, int next) const;
    void load();
    void save() const;

public:
    SecondaryIndex(const string& keyFile, const string& listFile, int keySize);

    // Add (secKey -> primaryKey). Returns false if that exact pair already exists.
    bool add(const string& secKey, const string& primaryKey);

    // Remove (secKey -> primaryKey). Returns false if not found.
    bool remove(const string& secKey, const string& primaryKey);

    // All primary keys that have this secondary key (sorted). Empty if none.
    vector<string> search(const string& secKey) const;

    // Record's secondary key changed (e.g. doctor name updated).
    bool changeKey(const string& oldSec, const string& newSec, const string& primaryKey);

    // Record's primary key changed (e.g. Doctor ID updated): fix it inside secKey's list.
    bool replacePrimary(const string& secKey, const string& oldPK, const string& newPK);

    // Move EVERY record from oldSec to newSec (used for Appointments index when a Doctor ID changes).
    void renameKey(const string& oldSec, const string& newSec);

    // Wipe the index (both files). Use before rebuilding from the data files.
    void clear();

    // Helpers for testing / debugging
    int  keyCount() const { return (int)entries.size(); }
    vector<string> getKeys() const;
    void print() const;
};

#endif