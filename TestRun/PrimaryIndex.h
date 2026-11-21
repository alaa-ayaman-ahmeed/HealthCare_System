#ifndef PRIMARY_INDEX_H
#define PRIMARY_INDEX_H

#include <iostream>
#include <vector>
#include <string>
#include "StorageManager.h"

using namespace std;

struct IndexEntry {
    string key;
    int offset;
};

class PrimaryIndex {

private:

    StorageManager& storage;

    vector<IndexEntry> doctorsIndex;
    vector<IndexEntry> appointmentsIndex;

    int binarySearch(const vector<IndexEntry>& index,
                     const string& key) const;

    bool buildOneIndex(const string& filename,
                       int recordSize,
                       vector<IndexEntry>& index);

    bool addEntry(vector<IndexEntry>& index,
                  const string& key,
                  int offset);

    bool deleteEntry(vector<IndexEntry>& index,
                     const string& key);

    bool updateKey(vector<IndexEntry>& index,
                   const string& oldKey,
                   const string& newKey);

public:

    PrimaryIndex(StorageManager& manager);

    bool buildIndexes();

    // Search
    int findDoctor(const string& id) const;
    int findAppointment(const string& id) const;


    bool addDoctor(const string& id, int offset);
    bool addAppointment(const string& id, int offset);


    bool deleteDoctor(const string& id);
    bool deleteAppointment(const string& id);

    // Update primary keys
    bool updateDoctorID(const string& oldID,
                        const string& newID);

    bool updateAppointmentID(const string& oldID,
                             const string& newID);

    // Access indexes
    const vector<IndexEntry>& getDoctorsIndex() const;
    const vector<IndexEntry>& getAppointmentsIndex() const;


    void printIndexes() const;
};

#endif
