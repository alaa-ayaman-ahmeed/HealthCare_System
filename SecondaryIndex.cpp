#include "SecondaryIndex.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <algorithm>
using namespace std;

// ---------- helpers ----------

string SecondaryIndex::normalize(const string& s, int size) const {
    string t = s.substr(0, size);
    while (!t.empty() && (t.back() == ' ' || t.back() == '\0'))
        t.pop_back();
    return t;
}

// Binary search on the sorted key array.
// found = true  -> returns index of the key
// found = false -> returns the position where the key should be inserted
int SecondaryIndex::findKey(const string& key, bool& found) const {
    int low = 0, high = (int)entries.size() - 1;
    while (low <= high) {
        int mid = (low + high) / 2;
        int cmp = entries[mid].key.compare(key);
        if (cmp == 0) { found = true; return mid; }
        if (cmp < 0) low = mid + 1;
        else         high = mid - 1;
    }
    found = false;
    return low;
}

int SecondaryIndex::nodeCount() const {
    ifstream f(listFileName, ios::binary | ios::ate);
    if (!f.is_open()) return 0;
    return (int)((long long)f.tellg() / NODE_SIZE);
}

void SecondaryIndex::readNode(int n, string& pk, int& next) const {
    ifstream f(listFileName, ios::binary);
    char buf[PK_SIZE];
    f.seekg((long long)n * NODE_SIZE, ios::beg);
    f.read(buf, PK_SIZE);
    f.read(reinterpret_cast<char*>(&next), sizeof(int));
    pk = normalize(string(buf, PK_SIZE), PK_SIZE);
}

void SecondaryIndex::writeNode(int n, const string& pk, int next) const {
    fstream f(listFileName, ios::in | ios::out | ios::binary);
    char buf[PK_SIZE];
    memset(buf, ' ', PK_SIZE);
    memcpy(buf, pk.c_str(), min((size_t)PK_SIZE, pk.size()));
    f.seekp((long long)n * NODE_SIZE, ios::beg);
    f.write(buf, PK_SIZE);
    f.write(reinterpret_cast<const char*>(&next), sizeof(int));
}

void SecondaryIndex::load() {
    entries.clear();
    ifstream f(keyFileName, ios::binary);
    if (!f.is_open()) return;

    int count = 0;
    f.read(reinterpret_cast<char*>(&count), sizeof(int));
    if (!f) return;

    for (int i = 0; i < count; i++) {
        vector<char> buf(keySize);
        SecEntry e;
        f.read(buf.data(), keySize);
        f.read(reinterpret_cast<char*>(&e.head), sizeof(int));
        if (!f) break;
        e.key = normalize(string(buf.data(), keySize), keySize);
        entries.push_back(e);
    }
}

void SecondaryIndex::save() const {
    ofstream f(keyFileName, ios::binary | ios::trunc);
    int count = (int)entries.size();
    f.write(reinterpret_cast<const char*>(&count), sizeof(int));
    for (const SecEntry& e : entries) {
        vector<char> buf(keySize, ' ');
        memcpy(buf.data(), e.key.c_str(), min((size_t)keySize, e.key.size()));
        f.write(buf.data(), keySize);
        f.write(reinterpret_cast<const char*>(&e.head), sizeof(int));
    }
}

// ---------- public ----------

SecondaryIndex::SecondaryIndex(const string& keyFile, const string& listFile, int keySz)
    : keyFileName(keyFile), listFileName(listFile), keySize(keySz) {

    // make sure the list file exists (fstream in|out fails on a missing file)
    fstream test(listFileName, ios::in | ios::out | ios::binary);
    if (!test.is_open()) {
        ofstream create(listFileName, ios::binary);
    }
    load();
}

bool SecondaryIndex::add(const string& secKey, const string& primaryKey) {
    string key = normalize(secKey, keySize);
    string pk  = normalize(primaryKey, PK_SIZE);
    if (key.empty() || pk.empty()) return false;

    bool found;
    int pos = findKey(key, found);

    if (!found) {
        // brand new secondary key: new node (next = -1) + insert key in sorted place (shifting)
        int n = nodeCount();
        writeNode(n, pk, -1);
        SecEntry e;
        e.key = key;
        e.head = n;
        entries.insert(entries.begin() + pos, e);   // vector::insert shifts the later entries
        save();
        return true;
    }

    // key exists: walk the chain to find the sorted position of pk
    int prev = -1;
    int cur = entries[pos].head;
    while (cur != -1) {
        string curPK;
        int curNext;
        readNode(cur, curPK, curNext);
        if (curPK == pk) return false;     // duplicate pair
        if (curPK > pk) break;             // insert before cur
        prev = cur;
        cur = curNext;
    }

    int n = nodeCount();
    writeNode(n, pk, cur);                 // new node points to cur

    if (prev == -1) {
        entries[pos].head = n;             // new first node
        save();
    }
    else {
        string prevPK;
        int prevNext;
        readNode(prev, prevPK, prevNext);
        writeNode(prev, prevPK, n);        // prev now points to the new node
    }
    return true;
}

bool SecondaryIndex::remove(const string& secKey, const string& primaryKey) {
    string key = normalize(secKey, keySize);
    string pk  = normalize(primaryKey, PK_SIZE);

    bool found;
    int pos = findKey(key, found);
    if (!found) return false;

    int prev = -1;
    int cur = entries[pos].head;
    while (cur != -1) {
        string curPK;
        int curNext;
        readNode(cur, curPK, curNext);

        if (curPK == pk) {
            if (prev == -1) {
                entries[pos].head = curNext;
                if (curNext == -1)
                    entries.erase(entries.begin() + pos);   // list empty: remove key, shift back
                save();
            }
            else {
                string prevPK;
                int prevNext;
                readNode(prev, prevPK, prevNext);
                writeNode(prev, prevPK, curNext);           // skip the removed node
            }
            return true;   // the node stays in the list file, just unreachable
        }
        prev = cur;
        cur = curNext;
    }
    return false;
}

vector<string> SecondaryIndex::search(const string& secKey) const {
    vector<string> result;
    string key = normalize(secKey, keySize);

    bool found;
    int pos = findKey(key, found);
    if (!found) return result;

    int cur = entries[pos].head;
    while (cur != -1) {
        string pk;
        int next;
        readNode(cur, pk, next);
        result.push_back(pk);
        cur = next;
    }
    return result;
}

bool SecondaryIndex::changeKey(const string& oldSec, const string& newSec, const string& primaryKey) {
    if (normalize(oldSec, keySize) == normalize(newSec, keySize)) return true;
    if (!remove(oldSec, primaryKey)) return false;
    return add(newSec, primaryKey);
}

bool SecondaryIndex::replacePrimary(const string& secKey, const string& oldPK, const string& newPK) {
    if (!remove(secKey, oldPK)) return false;
    return add(secKey, newPK);
}

void SecondaryIndex::renameKey(const string& oldSec, const string& newSec) {
    vector<string> pks = search(oldSec);
    for (const string& pk : pks) {
        remove(oldSec, pk);
        add(newSec, pk);
    }
}

void SecondaryIndex::clear() {
    entries.clear();
    ofstream l(listFileName, ios::binary | ios::trunc);   // empty list file
    save();                                               // key file with count 0
}

vector<string> SecondaryIndex::getKeys() const {
    vector<string> keys;
    for (const SecEntry& e : entries) keys.push_back(e.key);
    return keys;
}

void SecondaryIndex::print() const {
    for (const SecEntry& e : entries) {
        cout << e.key << " (head " << e.head << "): ";
        int cur = e.head;
        while (cur != -1) {
            string pk;
            int next;
            readNode(cur, pk, next);
            cout << "[" << pk << " -> " << next << "] ";
            cur = next;
        }
        cout << endl;
    }
}