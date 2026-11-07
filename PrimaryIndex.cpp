#include "PrimaryIndex.h"
#include <fstream>
#include <algorithm>
#include <climits>

using namespace std;

// Remove padding spaces
string cleanKey(const char* data, int size) {

    string key(data, size);

    while (!key.empty() &&
          (key.back() == ' ' || key.back() == '\0')) {

        key.pop_back();
    }

    return key;
}

PrimaryIndex::PrimaryIndex(StorageManager& manager)
    : storage(manager) {
}


int PrimaryIndex::binarySearch(
    const vector<IndexEntry>& index,
    const string& key) const {

    int left = 0;
    int right = (int)index.size() - 1;

    while (left <= right) {

        int mid = left + (right - left) / 2;

        if (index[mid].key == key)
            return mid;

        if (index[mid].key < key)
            left = mid + 1;
        else
            right = mid - 1;
    }

    return -1;
}


bool PrimaryIndex::buildOneIndex(
    const string& filename,
    int recordSize,
    vector<IndexEntry>& index) {

    ifstream file(filename, ios::binary | ios::ate);

    if (!file.is_open())
        return false;

    long long fileSize = file.tellg();
    file.close();

    if (fileSize < HEADER_SIZE ||
        (fileSize - HEADER_SIZE) % recordSize != 0)
        return false;

    vector<IndexEntry> temp;

    long long count =
        (fileSize - HEADER_SIZE) / recordSize;

    for (long long i = 0; i < count; i++) {

        long long offset =
            HEADER_SIZE + i * recordSize;

        if (offset > INT_MAX)
            return false;

        char record[DOCTOR_RECORD_SIZE];

        storage.readAt(
            filename,
            (int)offset,
            record,
            recordSize
        );

        // Skip deleted records
        if (record[0] == '*')
            continue;


        string key = cleanKey(record, 15);

        if (key.empty())
            continue;

        temp.push_back({key, (int)offset});
    }


    sort(temp.begin(), temp.end(),
        [](const IndexEntry& a, const IndexEntry& b) {
            return a.key < b.key;
        });

    // Check duplicate primary keys
    for (int i = 1; i < (int)temp.size(); i++) {

        if (temp[i].key == temp[i - 1].key)
            return false;
    }

    index = temp;

    return true;
}


bool PrimaryIndex::buildIndexes() {

    vector<IndexEntry> newDoctors;
    vector<IndexEntry> newAppointments;

    if (!buildOneIndex(
        "doctors.dat",
        DOCTOR_RECORD_SIZE,
        newDoctors))
        return false;

    if (!buildOneIndex(
        "appointments.dat",
        APPOINTMENT_RECORD_SIZE,
        newAppointments))
        return false;

    doctorsIndex = newDoctors;
    appointmentsIndex = newAppointments;

    return true;
}


int PrimaryIndex::findDoctor(const string& id) const {

    return binarySearch(doctorsIndex, id);
}

int PrimaryIndex::findAppointment(const string& id) const {

    return binarySearch(appointmentsIndex, id);
}


bool PrimaryIndex::addEntry(
    vector<IndexEntry>& index,
    const string& key,
    int offset) {

    if (key.empty() || offset < HEADER_SIZE ||
        binarySearch(index, key) != -1)
        return false;


    auto position = lower_bound(
        index.begin(),
        index.end(),
        key,
        [](const IndexEntry& entry, const string& value) {
            return entry.key < value;
        }
    );


    index.insert(position, {key, offset});

    return true;
}

bool PrimaryIndex::addDoctor(const string& id, int offset) {

    return addEntry(doctorsIndex, id, offset);
}

bool PrimaryIndex::addAppointment(const string& id, int offset) {

    return addEntry(appointmentsIndex, id, offset);
}


bool PrimaryIndex::deleteEntry(
    vector<IndexEntry>& index,
    const string& key) {

    int position = binarySearch(index, key);

    if (position == -1)
        return false;


    index.erase(index.begin() + position);

    return true;
}

bool PrimaryIndex::deleteDoctor(const string& id) {

    return deleteEntry(doctorsIndex, id);
}

bool PrimaryIndex::deleteAppointment(const string& id) {

    return deleteEntry(appointmentsIndex, id);
}


bool PrimaryIndex::updateKey(
    vector<IndexEntry>& index,
    const string& oldKey,
    const string& newKey) {

    if (newKey.empty())
        return false;

    int position = binarySearch(index, oldKey);

    if (position == -1)
        return false;


    if (oldKey != newKey &&
        binarySearch(index, newKey) != -1)
        return false;


    int oldOffset = index[position].offset;


    index.erase(index.begin() + position);


    auto newPosition = lower_bound(
        index.begin(),
        index.end(),
        newKey,
        [](const IndexEntry& entry, const string& value) {
            return entry.key < value;
        }
    );


    index.insert(newPosition, {newKey, oldOffset});

    return true;
}

bool PrimaryIndex::updateDoctorID(
    const string& oldID,
    const string& newID) {

    return updateKey(doctorsIndex, oldID, newID);
}

bool PrimaryIndex::updateAppointmentID(
    const string& oldID,
    const string& newID) {

    return updateKey(appointmentsIndex, oldID, newID);
}


const vector<IndexEntry>& PrimaryIndex::getDoctorsIndex() const {

    return doctorsIndex;
}

const vector<IndexEntry>& PrimaryIndex::getAppointmentsIndex() const {

    return appointmentsIndex;
}


void PrimaryIndex::printIndexes() const {

    cout << "Doctors Index:\n";

    for (const auto& entry : doctorsIndex) {
        cout << entry.key << "  " << entry.offset << endl;
    }

    cout << "\nAppointments Index:\n";

    for (const auto& entry : appointmentsIndex) {
        cout << entry.key << "  " << entry.offset << endl;
    }
}
